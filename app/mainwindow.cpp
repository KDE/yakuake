/*
  Copyright (C) 2008-2014 by Eike Hein <hein@kde.org>
  Copyright (C) 2009 by Juan Carlos Torres <carlosdgtorres@gmail.com>

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License as
  published by the Free Software Foundation; either version 2 of
  the License or (at your option) version 3 or any later version
  accepted by the membership of KDE e.V. (or its successor appro-
  ved by the membership of KDE e.V.), which shall act as a proxy
  defined in Section 14 of version 3 of the license.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program. If not, see http://www.gnu.org/licenses/.
*/


#include "mainwindow.h"
#include "settings.h"
#include "config/appearancesettings.h"
#include "config/windowsettings.h"
#include "firstrundialog.h"
#include "sessionstack.h"
#include "skin.h"
#include "tabbar.h"
#include "terminal.h"
#include "titlebar.h"
#include "ui_behaviorsettings.h"

#include <KAboutData>
#include <KConfigDialog>
#include <KGlobalAccel>
#include <KHelpMenu>
#include <KMessageBox>
#include <KNotification>
#include <KNotifyConfigWidget>
#include <KShortcutsDialog>
#include <KStandardAction>
#include <KToggleFullScreenAction>
#include <KActionCollection>
#include <KWindowSystem>
#include <KWindowEffects>
#include <KLocalizedString>

#include <QApplication>
#include <QDesktopWidget>
#include <QMenu>
#include <QPainter>
#include <QWhatsThis>
#include <QWindow>
#include <QDBusConnection>
#include <QPlatformSurfaceEvent>

#if HAVE_X11
#include <QX11Info>

#include <X11/Xlib.h>
#include <fixx11h.h>
#endif

#if HAVE_KWAYLAND
#include <KWayland/Client/connection_thread.h>
#include <KWayland/Client/registry.h>
#include <KWayland/Client/surface.h>
#include <KWayland/Client/plasmashell.h>
#endif


MainWindow::MainWindow(QWidget* parent)
    : KMainWindow(parent, Qt::CustomizeWindowHint | Qt::FramelessWindowHint)
{
    QDBusConnection::sessionBus().registerObject(QStringLiteral("/yakuake/window"), this, QDBusConnection::ExportScriptableSlots);

    setAttribute(Qt::WA_TranslucentBackground, true);
    setAttribute(Qt::WA_DeleteOnClose, false);
    setAttribute(Qt::WA_QuitOnClose, true);

    m_skin = new Skin();
    m_menu = new QMenu(this);
    m_helpMenu = new KHelpMenu(this, KAboutData::applicationData());
    m_sessionStack = new SessionStack(this);
    m_titleBar = new TitleBar(this);
    m_tabBar = new TabBar(this);

    m_firstRunDialog = NULL;
    m_isFullscreen = false;

#if HAVE_X11
    m_kwinAssistPropSet = false;
    m_isX11 = KWindowSystem::isPlatformX11();
#else
    m_isX11 = false;
#endif
    m_isWayland = KWindowSystem::isPlatformWayland();
#if HAVE_KWAYLAND
    m_plasmaShell = Q_NULLPTR;
    m_plasmaShellSurface = Q_NULLPTR;
    initWayland();
#endif

    m_toggleLock = false;

    setupActions();
    setupMenu();

    connect(m_tabBar, SIGNAL(newTabRequested()), m_sessionStack, SLOT(addSession()));
    connect(m_tabBar, SIGNAL(lastTabClosed()), m_tabBar, SIGNAL(newTabRequested()));
    connect(m_tabBar, SIGNAL(lastTabClosed()), this, SLOT(handleLastTabClosed()));
    connect(m_tabBar, SIGNAL(tabSelected(int)), m_sessionStack, SLOT(raiseSession(int)));
    connect(m_tabBar, SIGNAL(tabClosed(int)), m_sessionStack, SLOT(removeSession(int)));
    connect(m_tabBar, SIGNAL(requestTerminalHighlight(int)), m_sessionStack, SLOT(handleTerminalHighlightRequest(int)));
    connect(m_tabBar, SIGNAL(requestRemoveTerminalHighlight()), m_sessionStack, SIGNAL(removeTerminalHighlight()));
    connect(m_tabBar, SIGNAL(tabContextMenuClosed()), m_sessionStack, SIGNAL(removeTerminalHighlight()));

    connect(m_sessionStack, SIGNAL(sessionAdded(int,QString)),
        m_tabBar, SLOT(addTab(int,QString)));
    connect(m_sessionStack, SIGNAL(sessionRaised(int)), m_tabBar, SLOT(selectTab(int)));
    connect(m_sessionStack, SIGNAL(sessionRemoved(int)), m_tabBar, SLOT(removeTab(int)));
    connect(m_sessionStack, SIGNAL(activeTitleChanged(QString)),
        m_titleBar, SLOT(setTitle(QString)));
    connect(m_sessionStack, SIGNAL(activeTitleChanged(QString)),
        this, SLOT(setWindowTitle(QString)));

    connect(&m_mousePoller, SIGNAL(timeout()), this, SLOT(pollMouse()));

    connect(KWindowSystem::self(), SIGNAL(workAreaChanged()), this, SLOT(applyWindowGeometry()));
    connect(QApplication::desktop(), SIGNAL(screenCountChanged(int)), this, SLOT(updateScreenMenu()));

    applySettings();

    m_sessionStack->addSession();

    if (Settings::firstRun())
    {
        QMetaObject::invokeMethod(this, "toggleWindowState", Qt::QueuedConnection);
        QMetaObject::invokeMethod(this, "showFirstRunDialog", Qt::QueuedConnection);
    }
    else
    {
        showStartupPopup();
        if (Settings::pollMouse()) toggleMousePoll(true);
    }

    if (Settings::openAfterStart())
        QMetaObject::invokeMethod(this, "toggleWindowState", Qt::QueuedConnection);
}

MainWindow::~MainWindow()
{
    Settings::self()->save();

    delete m_skin;
}

#if HAVE_KWAYLAND
void MainWindow::initWayland()
{
    if (!m_isWayland) {
        return;
    }

    using namespace KWayland::Client;
    auto connection = ConnectionThread::fromApplication(this);
    if (!connection) {
        return;
    }
    Registry *registry = new Registry(this);
    registry->create(connection);
    QObject::connect(registry, &Registry::interfacesAnnounced, this,
        [registry, this] {
            const auto interface = registry->interface(Registry::Interface::PlasmaShell);
            if (interface.name != 0) {
                m_plasmaShell = registry->createPlasmaShell(interface.name, interface.version, this);
            }
        }
    );

    registry->setup();
    connection->roundtrip();
}

void MainWindow::initWaylandSurface()
{
    if (m_plasmaShellSurface) {
        return;
    }
    if (!m_plasmaShell) {
        return;
    }
    if (auto surface = KWayland::Client::Surface::fromWindow(windowHandle())) {
        m_plasmaShellSurface = m_plasmaShell->createSurface(surface, this);
        m_plasmaShellSurface->setPosition(pos());
    }
}

#endif

bool MainWindow::queryClose()
{
    bool confirmQuit = Settings::confirmQuit();
    bool hasUnclosableSessions = m_sessionStack->hasUnclosableSessions();

    QString closeQuestion = xi18nc("@info","Are you sure you want to quit?");
    QString warningMessage;

    if ((confirmQuit && m_sessionStack->count() > 1) || hasUnclosableSessions)
    {
        if (confirmQuit && m_sessionStack->count() > 1)
        {
            if (hasUnclosableSessions)
                warningMessage = xi18nc("@info", "<warning>There are multiple open sessions, <emphasis>some of which you have locked to prevent closing them accidentally.</emphasis> These will be killed if you continue.</warning>");
            else
                warningMessage = xi18nc("@info", "<warning>There are multiple open sessions. These will be killed if you continue.</warning>");
        }
        else if (hasUnclosableSessions)
        {
            warningMessage = xi18nc("@info", "<warning>There are one or more open sessions that you have locked to prevent closing them accidentally. These will be killed if you continue.</warning>");
        }

        int result = KMessageBox::warningContinueCancel(this,
            warningMessage + QStringLiteral("<br /><br />") + closeQuestion,
            xi18nc("@title:window", "Really Quit?"), KStandardGuiItem::quit(), KStandardGuiItem::cancel());

        return result != KMessageBox::Cancel;
    }

    return true;
}

void MainWindow::setupActions()
{
    m_actionCollection = new KActionCollection(this);

    KToggleFullScreenAction* fullScreenAction = new KToggleFullScreenAction(this);
    fullScreenAction->setWindow(this);
    actionCollection()->setDefaultShortcut(fullScreenAction, QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_F11));
    m_actionCollection->addAction(QStringLiteral("view-full-screen"), fullScreenAction);
    connect(fullScreenAction, SIGNAL(toggled(bool)), this, SLOT(setFullScreen(bool)));

    QAction* action = KStandardAction::quit(this, SLOT(close()), actionCollection());
    actionCollection()->setDefaultShortcut(action, QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_Q));
    action = KStandardAction::aboutApp(m_helpMenu, SLOT(aboutApplication()), actionCollection());
    action = KStandardAction::reportBug(m_helpMenu, SLOT(reportBug()), actionCollection());
    action = KStandardAction::aboutKDE(m_helpMenu, SLOT(aboutKDE()), actionCollection());
    action = KStandardAction::keyBindings(this, SLOT(configureKeys()), actionCollection());
    action = KStandardAction::configureNotifications(this, SLOT(configureNotifications()), actionCollection());
    action = KStandardAction::preferences(this, SLOT(configureApp()), actionCollection());

    action = KStandardAction::whatsThis(this, SLOT(whatsThis()), actionCollection());

    action = actionCollection()->addAction(QStringLiteral("toggle-window-state"));
    action->setText(xi18nc("@action", "Open/Retract Yakuake"));
    action->setIcon(QIcon::fromTheme(QStringLiteral("yakuake")));
#ifndef Q_OS_WIN /* PORT */
    KGlobalAccel::self()->setGlobalShortcut(action, QList<QKeySequence>() << QKeySequence(Qt::Key_F12));
#else
    KGlobalAccel::self()->setGlobalShortcut(action, QList<QKeySequence>() << QKeySequence(Qt::Key_F11));
#endif
    connect(action, SIGNAL(triggered()), this, SLOT(toggleWindowState()));

    action = actionCollection()->addAction(QStringLiteral("keep-open"));
    action->setText(xi18nc("@action", "Keep window open when it loses focus"));
    action->setCheckable(true);
    connect(action, SIGNAL(toggled(bool)), this, SLOT(setKeepOpen(bool)));

    action = actionCollection()->addAction(QStringLiteral("manage-profiles"));
    action->setText(xi18nc("@action", "Manage Profiles..."));
    action->setIcon(QIcon::fromTheme(QStringLiteral("configure")));
    connect(action, SIGNAL(triggered()), m_sessionStack, SIGNAL(manageProfiles()));

    action = actionCollection()->addAction(QStringLiteral("edit-profile"));
    action->setText(xi18nc("@action", "Edit Current Profile..."));
    action->setIcon(QIcon::fromTheme(QStringLiteral("document-properties")));
    connect(action, SIGNAL(triggered()), this, SLOT(handleContextDependentAction()));
    m_contextDependentActions << action;

    action = actionCollection()->addAction(QStringLiteral("increase-window-width"));
    action->setText(xi18nc("@action", "Increase Window Width"));
    actionCollection()->setDefaultShortcut(action, QKeySequence(Qt::ALT + Qt::SHIFT + Qt::Key_Right));
    connect(action, SIGNAL(triggered()), this, SLOT(increaseWindowWidth()));

    action = actionCollection()->addAction(QStringLiteral("decrease-window-width"));
    action->setText(xi18nc("@action", "Decrease Window Width"));
    actionCollection()->setDefaultShortcut(action, QKeySequence(Qt::ALT + Qt::SHIFT + Qt::Key_Left));
    connect(action, SIGNAL(triggered()), this, SLOT(decreaseWindowWidth()));

    action = actionCollection()->addAction(QStringLiteral("increase-window-height"));
    action->setText(xi18nc("@action", "Increase Window Height"));
    actionCollection()->setDefaultShortcut(action, QKeySequence(Qt::ALT + Qt::SHIFT + Qt::Key_Down));
    connect(action, SIGNAL(triggered()), this, SLOT(increaseWindowHeight()));

    action = actionCollection()->addAction(QStringLiteral("decrease-window-height"));
    action->setText(xi18nc("@action", "Decrease Window Height"));
    actionCollection()->setDefaultShortcut(action, QKeySequence(Qt::ALT + Qt::SHIFT + Qt::Key_Up));
    connect(action, SIGNAL(triggered()), this, SLOT(decreaseWindowHeight()));

    action = actionCollection()->addAction(QStringLiteral("new-session"));
    action->setText(xi18nc("@action", "New Session"));
    action->setIcon(QIcon::fromTheme(QStringLiteral("tab-new")));
    actionCollection()->setDefaultShortcut(action, QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_T));
    connect(action, SIGNAL(triggered()), m_sessionStack, SLOT(addSession()));

    action = actionCollection()->addAction(QStringLiteral("new-session-two-horizontal"));
    action->setText(xi18nc("@action", "Two Terminals, Horizontally"));
    action->setIcon(QIcon::fromTheme(QStringLiteral("tab-new")));
    connect(action, SIGNAL(triggered()), m_sessionStack, SLOT(addSessionTwoHorizontal()));

    action = actionCollection()->addAction(QStringLiteral("new-session-two-vertical"));
    action->setText(xi18nc("@action", "Two Terminals, Vertically"));
    action->setIcon(QIcon::fromTheme(QStringLiteral("tab-new")));
    connect(action, SIGNAL(triggered()), m_sessionStack, SLOT(addSessionTwoVertical()));

    action = actionCollection()->addAction(QStringLiteral("new-session-quad"));
    action->setText(xi18nc("@action", "Four Terminals, Grid"));
    action->setIcon(QIcon::fromTheme(QStringLiteral("tab-new")));
    connect(action, SIGNAL(triggered()), m_sessionStack, SLOT(addSessionQuad()));

    action = actionCollection()->addAction(QStringLiteral("close-session"));
    action->setText(xi18nc("@action", "Close Session"));
    action->setIcon(QIcon::fromTheme(QStringLiteral("tab-close")));
    actionCollection()->setDefaultShortcut(action, QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_W));
    connect(action, SIGNAL(triggered()), this, SLOT(handleContextDependentAction()));
    m_contextDependentActions << action;

    action = actionCollection()->addAction(QStringLiteral("previous-session"));
    action->setText(xi18nc("@action", "Previous Session"));
    action->setIcon(QIcon::fromTheme(QStringLiteral("go-previous")));
    actionCollection()->setDefaultShortcut(action, QKeySequence(Qt::SHIFT + Qt::Key_Left));
    connect(action, SIGNAL(triggered()), m_tabBar, SLOT(selectPreviousTab()));

    action = actionCollection()->addAction(QStringLiteral("next-session"));
    action->setText(xi18nc("@action", "Next Session"));
    action->setIcon(QIcon::fromTheme(QStringLiteral("go-next")));
    actionCollection()->setDefaultShortcut(action, QKeySequence(Qt::SHIFT + Qt::Key_Right));
    connect(action, SIGNAL(triggered()), m_tabBar, SLOT(selectNextTab()));

    action = actionCollection()->addAction(QStringLiteral("move-session-left"));
    action->setText(xi18nc("@action", "Move Session Left"));
    action->setIcon(QIcon::fromTheme(QStringLiteral("arrow-left")));
    actionCollection()->setDefaultShortcut(action, QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_Left));
    connect(action, SIGNAL(triggered()), this, SLOT(handleContextDependentAction()));
    m_contextDependentActions << action;

    action = actionCollection()->addAction(QStringLiteral("move-session-right"));
    action->setText(xi18nc("@action", "Move Session Right"));
    action->setIcon(QIcon::fromTheme(QStringLiteral("arrow-right")));
    actionCollection()->setDefaultShortcut(action, QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_Right));
    connect(action, SIGNAL(triggered()), this, SLOT(handleContextDependentAction()));
    m_contextDependentActions << action;

    action = actionCollection()->addAction(QStringLiteral("grow-terminal-right"));
    action->setText(xi18nc("@action", "Grow Terminal to the Right"));
    action->setIcon(QIcon::fromTheme(QStringLiteral("arrow-right")));
    actionCollection()->setDefaultShortcut(action, QKeySequence(Qt::CTRL + Qt::ALT + Qt::Key_Right));
    connect(action, SIGNAL(triggered()), this, SLOT(handleContextDependentAction()));
    m_contextDependentActions << action;

    action = actionCollection()->addAction(QStringLiteral("grow-terminal-left"));
    action->setText(xi18nc("@action", "Grow Terminal to the Left"));
    action->setIcon(QIcon::fromTheme(QStringLiteral("arrow-left")));
    actionCollection()->setDefaultShortcut(action, QKeySequence(Qt::CTRL + Qt::ALT + Qt::Key_Left));
    connect(action, SIGNAL(triggered()), this, SLOT(handleContextDependentAction()));
    m_contextDependentActions << action;

    action = actionCollection()->addAction(QStringLiteral("grow-terminal-top"));
    action->setText(xi18nc("@action", "Grow Terminal to the Top"));
    action->setIcon(QIcon::fromTheme(QStringLiteral("arrow-up")));
    actionCollection()->setDefaultShortcut(action, QKeySequence(Qt::CTRL + Qt::ALT + Qt::Key_Up));
    connect(action, SIGNAL(triggered()), this, SLOT(handleContextDependentAction()));
    m_contextDependentActions << action;

    action = actionCollection()->addAction(QStringLiteral("grow-terminal-bottom"));
    action->setText(xi18nc("@action", "Grow Terminal to the Bottom"));
    action->setIcon(QIcon::fromTheme(QStringLiteral("arrow-down")));
    actionCollection()->setDefaultShortcut(action, QKeySequence(Qt::CTRL + Qt::ALT + Qt::Key_Down));
    connect(action, SIGNAL(triggered()), this, SLOT(handleContextDependentAction()));
    m_contextDependentActions << action;

    action = actionCollection()->addAction(QStringLiteral("rename-session"));
    action->setText(xi18nc("@action", "Rename Session..."));
    action->setIcon(QIcon::fromTheme(QStringLiteral("edit-rename")));
    actionCollection()->setDefaultShortcut(action, QKeySequence(Qt::CTRL + Qt::ALT + Qt::Key_S));
    connect(action, SIGNAL(triggered()), this, SLOT(handleContextDependentAction()));
    m_contextDependentActions << action;

    action = actionCollection()->addAction(QStringLiteral("previous-terminal"));
    action->setText(xi18nc("@action", "Previous Terminal"));
    action->setIcon(QIcon::fromTheme(QStringLiteral("go-previous")));
    actionCollection()->setDefaultShortcut(action, QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_Up));
    connect(action, SIGNAL(triggered()), m_sessionStack, SIGNAL(previousTerminal()));

    action = actionCollection()->addAction(QStringLiteral("next-terminal"));
    action->setText(xi18nc("@action", "Next Terminal"));
    action->setIcon(QIcon::fromTheme(QStringLiteral("go-next")));
    actionCollection()->setDefaultShortcut(action, QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_Down));
    connect(action, SIGNAL(triggered()), m_sessionStack, SIGNAL(nextTerminal()));

    action = actionCollection()->addAction(QStringLiteral("close-active-terminal"));
    action->setText(xi18nc("@action", "Close Active Terminal"));
    action->setIcon(QIcon::fromTheme(QStringLiteral("view-close")));
    actionCollection()->setDefaultShortcut(action, QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_R));
    connect(action, SIGNAL(triggered()), this, SLOT(handleContextDependentAction()));
    m_contextDependentActions << action;

    action = actionCollection()->addAction(QStringLiteral("split-left-right"));
    action->setText(xi18nc("@action", "Split Left/Right"));
    action->setIcon(QIcon::fromTheme(QStringLiteral("view-split-left-right")));
    actionCollection()->setDefaultShortcut(action, QKeySequence(Qt::CTRL+ Qt::Key_ParenLeft));
    connect(action, SIGNAL(triggered()), this, SLOT(handleContextDependentAction()));
    m_contextDependentActions << action;

    action = actionCollection()->addAction(QStringLiteral("split-top-bottom"));
    action->setText(xi18nc("@action", "Split Top/Bottom"));
    action->setIcon(QIcon::fromTheme(QStringLiteral("view-split-top-bottom")));
    actionCollection()->setDefaultShortcut(action, QKeySequence(Qt::CTRL + Qt::Key_ParenRight));
    connect(action, SIGNAL(triggered()), this, SLOT(handleContextDependentAction()));
    m_contextDependentActions << action;

    action = actionCollection()->addAction(QStringLiteral("toggle-session-prevent-closing"));
    action->setText(xi18nc("@action", "Prevent Closing"));
    action->setCheckable(true);
    connect(action, SIGNAL(triggered(bool)), this, SLOT(handleContextDependentToggleAction(bool)));
    m_contextDependentActions << action;

    action = actionCollection()->addAction(QStringLiteral("toggle-session-keyboard-input"));
    action->setText(xi18nc("@action", "Disable Keyboard Input"));
    action->setCheckable(true);
    connect(action, SIGNAL(triggered(bool)), this, SLOT(handleContextDependentToggleAction(bool)));
    m_contextDependentActions << action;

    action = actionCollection()->addAction(QStringLiteral("toggle-session-monitor-activity"));
    action->setText(xi18nc("@action", "Monitor for Activity"));
    actionCollection()->setDefaultShortcut(action, QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_A));
    action->setCheckable(true);
    connect(action, SIGNAL(triggered(bool)), this, SLOT(handleContextDependentToggleAction(bool)));
    m_contextDependentActions << action;

    action = actionCollection()->addAction(QStringLiteral("toggle-session-monitor-silence"));
    action->setText(xi18nc("@action", "Monitor for Silence"));
    actionCollection()->setDefaultShortcut(action, QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_I));
    action->setCheckable(true);
    connect(action, SIGNAL(triggered(bool)), this, SLOT(handleContextDependentToggleAction(bool)));
    m_contextDependentActions << action;

    for (uint i = 1; i <= 10; ++i)
    {
        action = actionCollection()->addAction(QString(QStringLiteral("switch-to-session-%1")).arg(i));
        action->setText(xi18nc("@action", "Switch to Session %1", i));
        action->setData(i);
        connect(action, SIGNAL(triggered()), this, SLOT(handleSwitchToAction()));
    }

    m_actionCollection->associateWidget(this);
    m_actionCollection->readSettings();
}

void MainWindow::handleContextDependentAction(QAction* action, int sessionId)
{
    if (sessionId == -1) sessionId = m_sessionStack->activeSessionId();
    if (sessionId == -1) return;

    if (!action) action = qobject_cast<QAction*>(QObject::sender());

    if (action == actionCollection()->action(QStringLiteral("edit-profile")))
        m_sessionStack->editProfile(sessionId);

    if (action == actionCollection()->action(QStringLiteral("close-session")))
        m_sessionStack->removeSession(sessionId);

    if (action == actionCollection()->action(QStringLiteral("move-session-left")))
        m_tabBar->moveTabLeft(sessionId);

    if (action == actionCollection()->action(QStringLiteral("move-session-right")))
        m_tabBar->moveTabRight(sessionId);

    if (action == actionCollection()->action(QStringLiteral("rename-session")))
        m_tabBar->interactiveRename(sessionId);

    if (action == actionCollection()->action(QStringLiteral("close-active-terminal")))
        m_sessionStack->closeActiveTerminal(sessionId);

    if (action == actionCollection()->action(QStringLiteral("split-left-right")))
        m_sessionStack->splitSessionLeftRight(sessionId);

    if (action == actionCollection()->action(QStringLiteral("split-top-bottom")))
        m_sessionStack->splitSessionTopBottom(sessionId);

    if (action == actionCollection()->action(QStringLiteral("grow-terminal-right")))
        m_sessionStack->tryGrowTerminalRight(m_sessionStack->activeTerminalId());

    if (action == actionCollection()->action(QStringLiteral("grow-terminal-left")))
        m_sessionStack->tryGrowTerminalLeft(m_sessionStack->activeTerminalId());

    if (action == actionCollection()->action(QStringLiteral("grow-terminal-top")))
        m_sessionStack->tryGrowTerminalTop(m_sessionStack->activeTerminalId());

    if (action == actionCollection()->action(QStringLiteral("grow-terminal-bottom")))
        m_sessionStack->tryGrowTerminalBottom(m_sessionStack->activeTerminalId());
}

void MainWindow::handleContextDependentToggleAction(bool checked, QAction* action, int sessionId)
{
    if (sessionId == -1) sessionId = m_sessionStack->activeSessionId();
    if (sessionId == -1) return;

    if (!action) action = qobject_cast<QAction*>(QObject::sender());

    if (action == actionCollection()->action(QStringLiteral("toggle-session-prevent-closing"))) {
        m_sessionStack->setSessionClosable(sessionId, !checked);

        // Repaint the tab bar when the Prevent Closing action is toggled
        // so the lock icon is added to or removed from the tab label.
        m_tabBar->repaint();
    }

    if (action == actionCollection()->action(QStringLiteral("toggle-session-keyboard-input")))
        m_sessionStack->setSessionKeyboardInputEnabled(sessionId, !checked);

    if (action == actionCollection()->action(QStringLiteral("toggle-session-monitor-activity")))
        m_sessionStack->setSessionMonitorActivityEnabled(sessionId, checked);

    if (action == actionCollection()->action(QStringLiteral("toggle-session-monitor-silence")))
        m_sessionStack->setSessionMonitorSilenceEnabled(sessionId, checked);
}

void MainWindow::setContextDependentActionsQuiet(bool quiet)
{
    QListIterator<QAction*> i(m_contextDependentActions);

    while (i.hasNext()) i.next()->blockSignals(quiet);
}

void MainWindow::handleToggleTerminalKeyboardInput(bool checked)
{
    QAction* action = qobject_cast<QAction*>(QObject::sender());

    if (!action || action->data().isNull()) return;

    bool ok = false;
    int terminalId = action->data().toInt(&ok);
    if (!ok) return;

    m_sessionStack->setTerminalKeyboardInputEnabled(terminalId, !checked);
}

void MainWindow::handleToggleTerminalMonitorActivity(bool checked)
{
    QAction* action = qobject_cast<QAction*>(QObject::sender());

    if (!action || action->data().isNull()) return;

    bool ok = false;
    int terminalId = action->data().toInt(&ok);
    if (!ok) return;

    m_sessionStack->setTerminalMonitorActivityEnabled(terminalId, checked);
}

void MainWindow::handleToggleTerminalMonitorSilence(bool checked)
{
    QAction* action = qobject_cast<QAction*>(QObject::sender());

    if (!action || action->data().isNull()) return;

    bool ok = false;
    int terminalId = action->data().toInt(&ok);
    if (!ok) return;

    m_sessionStack->setTerminalMonitorSilenceEnabled(terminalId, checked);
}

void MainWindow::handleTerminalActivity(Terminal* terminal)
{
    Session* session = qobject_cast<Session*>(sender());

    if (session)
    {
        disconnect(terminal, SIGNAL(activityDetected(Terminal*)), session, SIGNAL(activityDetected(Terminal*)));

        QString message(xi18nc("@info", "Activity detected in monitored terminal in session \"%1\".",
            m_tabBar->tabTitle(session->id())));

        KNotification::event(QLatin1String("activity"), message, QPixmap(), terminal->partWidget(),
            KNotification::CloseWhenWidgetActivated);
    }
}

void MainWindow::handleTerminalSilence(Terminal* terminal)
{
    Session* session = qobject_cast<Session*>(sender());

    if (session)
    {
        QString message(xi18nc("@info", "Silence detected in monitored terminal in session \"%1\".",
            m_tabBar->tabTitle(session->id())));

        KNotification::event(QLatin1String("silence"), message, QPixmap(), terminal->partWidget(),
            KNotification::CloseWhenWidgetActivated);
    }
}

void MainWindow::handleLastTabClosed()
{
    if (isVisible() && !Settings::keepOpenAfterLastSessionCloses())
        toggleWindowState();
}

void MainWindow::handleSwitchToAction()
{
    QAction* action = qobject_cast<QAction*>(QObject::sender());

    if (action && !action->data().isNull())
        m_sessionStack->raiseSession(m_tabBar->sessionAtTab(action->data().toInt()-1));
}

void MainWindow::setupMenu()
{
    m_menu->insertSection(0, xi18nc("@title:menu", "Help"));
    m_menu->addAction(actionCollection()->action(QLatin1String(KStandardAction::name(KStandardAction::WhatsThis))));
    m_menu->addAction(actionCollection()->action(QLatin1String(KStandardAction::name(KStandardAction::ReportBug))));
    m_menu->addAction(actionCollection()->action(QLatin1String(KStandardAction::name(KStandardAction::AboutApp))));
    m_menu->addAction(actionCollection()->action(QLatin1String(KStandardAction::name(KStandardAction::AboutKDE))));

    m_menu->insertSection(0, xi18nc("@title:menu", "Quick Options"));
    m_menu->addAction(actionCollection()->action(QStringLiteral("view-full-screen")));
    m_menu->addAction(actionCollection()->action(QStringLiteral("keep-open")));

    m_screenMenu = new QMenu(this);
    connect(m_screenMenu, SIGNAL(triggered(QAction*)), this, SLOT(setScreen(QAction*)));
    m_screenMenu->setTitle(xi18nc("@title:menu", "Screen"));
    m_menu->addMenu(m_screenMenu);

    m_windowWidthMenu = new QMenu(this);
    connect(m_windowWidthMenu, SIGNAL(triggered(QAction*)), this, SLOT(setWindowWidth(QAction*)));
    m_windowWidthMenu->setTitle(xi18nc("@title:menu", "Width"));
    m_menu->addMenu(m_windowWidthMenu);

    m_windowHeightMenu = new QMenu(this);
    connect(m_windowHeightMenu, SIGNAL(triggered(QAction*)), this, SLOT(setWindowHeight(QAction*)));
    m_windowHeightMenu->setTitle(xi18nc("@title:menu", "Height"));
    m_menu->addMenu(m_windowHeightMenu);

    m_menu->insertSection(0, xi18nc("@title:menu", "Settings"));
    m_menu->addAction(actionCollection()->action(QStringLiteral("manage-profiles")));
    m_menu->addAction(actionCollection()->action(QLatin1String(KStandardAction::name(KStandardAction::KeyBindings))));
    m_menu->addAction(actionCollection()->action(QLatin1String(KStandardAction::name(KStandardAction::ConfigureNotifications))));
    m_menu->addAction(actionCollection()->action(QLatin1String(KStandardAction::name(KStandardAction::Preferences))));

    m_menu->addSeparator();
    m_menu->addAction(actionCollection()->action(QLatin1String(KStandardAction::name(KStandardAction::Quit))));
}

void MainWindow::updateScreenMenu()
{
    QAction* action;

    m_screenMenu->clear();

    action = m_screenMenu->addAction(xi18nc("@item:inmenu", "At mouse location"));
    action->setCheckable(true);
    action->setData(0);
    action->setChecked(Settings::screen() == 0);

    for (int i = 1; i <= QApplication::desktop()->screenCount(); i++)
    {
        action = m_screenMenu->addAction(xi18nc("@item:inmenu", "Screen %1", i));
        action->setCheckable(true);
        action->setData(i);
        action->setChecked(i == Settings::screen());
    }

    action = m_screenMenu->menuAction();
    action->setVisible(QApplication::desktop()->screenCount() > 1);
}

void MainWindow::updateWindowSizeMenus()
{
    updateWindowWidthMenu();
    updateWindowHeightMenu();
}

void MainWindow::updateWindowWidthMenu()
{
    QAction* action = 0;

    if (m_windowWidthMenu->isEmpty())
    {
        for (int i = 10; i <= 100; i += 10)
        {
            action = m_windowWidthMenu->addAction(QString::number(i) + QStringLiteral("%"));
            action->setCheckable(true);
            action->setData(i);
            action->setChecked(i == Settings::width());
        }
    }
    else
    {
        QListIterator<QAction*> i(m_windowWidthMenu->actions());

        while (i.hasNext())
        {
            action = i.next();

            action->setChecked(action->data().toInt() == Settings::width());
        }
    }
}

void MainWindow::updateWindowHeightMenu()
{
    QAction* action = 0;

    if (m_windowHeightMenu->isEmpty())
    {
        for (int i = 10; i <= 100; i += 10)
        {
            action = m_windowHeightMenu->addAction(QString::number(i) + QStringLiteral("%"));
            action->setCheckable(true);
            action->setData(i);
            action->setChecked(i == Settings::height());
        }
    }
    else
    {
        QListIterator<QAction*> i(m_windowHeightMenu->actions());

        while (i.hasNext())
        {
            action = i.next();

            action->setChecked(action->data().toInt() == Settings::height());
        }
    }
}

void MainWindow::configureKeys()
{
    KShortcutsDialog::configure(actionCollection());
}

void MainWindow::configureNotifications()
{
    KNotifyConfigWidget::configure(this);
}

void MainWindow::configureApp()
{
    if (KConfigDialog::showDialog(QStringLiteral("settings"))) return;

    KConfigDialog* settingsDialog = new KConfigDialog(this, QStringLiteral("settings"), Settings::self());
    settingsDialog->setMinimumHeight(560);
    settingsDialog->setFaceType(KPageDialog::List);
    connect(settingsDialog, &KConfigDialog::settingsChanged, this, &MainWindow::applySettings);

    WindowSettings* windowSettings = new WindowSettings(settingsDialog);
    settingsDialog->addPage(windowSettings, xi18nc("@title Preferences page name", "Window"), QStringLiteral("preferences-system-windows-move"));
    connect(windowSettings, SIGNAL(updateWindowGeometry(int,int,int)),
        this, SLOT(setWindowGeometry(int,int,int)));

    QWidget* behaviorSettings = new QWidget(settingsDialog);
    Ui::BehaviorSettings behaviorSettingsUi;
    behaviorSettingsUi.setupUi(behaviorSettings);
    settingsDialog->addPage(behaviorSettings, xi18nc("@title Preferences page name", "Behavior"),
        QStringLiteral("preferences-system-windows-actions"));

    AppearanceSettings* appearanceSettings = new AppearanceSettings(settingsDialog);
    settingsDialog->addPage(appearanceSettings, xi18nc("@title Preferences page name", "Appearance"),
        QStringLiteral("preferences-desktop-theme"));
    connect(settingsDialog, &QDialog::rejected, appearanceSettings, &AppearanceSettings::resetSelection);

    settingsDialog->button(QDialogButtonBox::Help)->hide();
    settingsDialog->button(QDialogButtonBox::Cancel)->setFocus();

    connect(settingsDialog, &QDialog::finished, [=]() {
        m_toggleLock = true;
        KWindowSystem::activateWindow(winId());
        KWindowSystem::forceActiveWindow(winId());
    });

    settingsDialog->show();
}

void MainWindow::applySettings()
{
    if (Settings::dynamicTabTitles())
    {
        connect(m_sessionStack, SIGNAL(titleChanged(int,QString)),
            m_tabBar, SLOT(setTabTitle(int,QString)));

        m_sessionStack->emitTitles();
    }
    else
    {
        disconnect(m_sessionStack, SIGNAL(titleChanged(int,QString)),
            m_tabBar, SLOT(setTabTitle(int,QString)));
    }

    m_animationTimer.setInterval(Settings::frames() ? 10 : 0);

    m_tabBar->setVisible(Settings::showTabBar());

    setKeepOpen(Settings::keepOpen());

    updateScreenMenu();
    updateWindowSizeMenus();

    updateUseTranslucency();

    applySkin();
    applyWindowGeometry();
    applyWindowProperties();
}

void MainWindow::applySkin()
{
    bool gotSkin = m_skin->load(Settings::skin(), Settings::skinInstalledWithKns());

    if (!gotSkin)
    {
        Settings::setSkin(QStringLiteral("default"));
        gotSkin = m_skin->load(Settings::skin());
    }

    if (!gotSkin)
    {
        KMessageBox::error(parentWidget(),
            xi18nc("@info", "<application>Yakuake</application> was unable to load a skin. It is likely that it was installed incorrectly.<nl/><nl/>"
                           "The application will now quit."),
            xi18nc("@title:window", "Cannot Load Skin"));

        QMetaObject::invokeMethod(qApp, "quit", Qt::QueuedConnection);
    }

    m_titleBar->applySkin();
    m_tabBar->applySkin();
}

void MainWindow::applyWindowProperties()
{
    if (Settings::keepOpen() && !Settings::keepAbove())
    {
        KWindowSystem::clearState(winId(), NET::KeepAbove);
        KWindowSystem::setState(winId(), NET::Sticky | NET::SkipTaskbar | NET::SkipPager);
    }
    else
        KWindowSystem::setState(winId(), NET::KeepAbove | NET::Sticky | NET::SkipTaskbar | NET::SkipPager);

    KWindowSystem::setOnAllDesktops(winId(), Settings::showOnAllDesktops());
    KWindowEffects::enableBlurBehind(winId(), Settings::blur());
}

void MainWindow::applyWindowGeometry()
{
    int width, height;

    QAction* action = actionCollection()->action(QStringLiteral("view-full-screen"));

    if (action->isChecked())
    {
        width = 100;
        height = 100;
    }
    else
    {
        width = Settings::width();
        height = Settings::height();
    }

    setWindowGeometry(width, height, Settings::position());
}

void MainWindow::setWindowGeometry(int newWidth, int newHeight, int newPosition)
{
    QRect workArea = getDesktopGeometry();

    int maxHeight = workArea.height() * newHeight / 100;

    int targetWidth = workArea.width() * newWidth / 100;

    setGeometry(workArea.x() + workArea.width() * newPosition * (100 - newWidth) / 10000,
                workArea.y(), targetWidth, maxHeight);
#if HAVE_KWAYLAND
    initWaylandSurface();
#endif

    maxHeight -= m_titleBar->height();
    m_titleBar->setGeometry(0, maxHeight, targetWidth, m_titleBar->height());
    if (!isVisible()) m_titleBar->updateMask();

    if (Settings::frames() > 0)
        m_animationStepSize = maxHeight / Settings::frames();
    else
        m_animationStepSize = maxHeight;

    if (Settings::showTabBar())
    {
        if (m_skin->tabBarCompact())
        {
            m_tabBar->setGeometry(m_skin->tabBarLeft(), maxHeight,
                width() - m_skin->tabBarLeft() - m_skin->tabBarRight(), m_tabBar->height());
        }
        else
        {
            maxHeight -= m_tabBar->height();
            m_tabBar->setGeometry(m_skin->borderWidth(), maxHeight,
                width() - 2 * m_skin->borderWidth(), m_tabBar->height());
        }
    }

    m_sessionStack->setGeometry(m_skin->borderWidth(), 0,
        width() - 2 * m_skin->borderWidth(), maxHeight);

    updateMask();
}

void MainWindow::setScreen(QAction* action)
{
    Settings::setScreen(action->data().toInt());
    Settings::self()->save();

    applyWindowGeometry();

    updateScreenMenu();
}

void MainWindow::setWindowWidth(int width)
{
    Settings::setWidth(width);
    Settings::self()->save();

    applyWindowGeometry();

    updateWindowWidthMenu();
}

void MainWindow::setWindowHeight(int height)
{
    Settings::setHeight(height);
    Settings::self()->save();

    applyWindowGeometry();

    updateWindowHeightMenu();
}

void MainWindow::setWindowWidth(QAction* action)
{
    setWindowWidth(action->data().toInt());
}

void MainWindow::setWindowHeight(QAction* action)
{
    setWindowHeight(action->data().toInt());
}

void MainWindow::increaseWindowWidth()
{
    if (Settings::width() <= 90) setWindowWidth(Settings::width() + 10);
}

void MainWindow:: decreaseWindowWidth()
{
    if (Settings::width() >= 20) setWindowWidth(Settings::width() - 10);
}

void MainWindow::increaseWindowHeight()
{
    if (Settings::height() <= 90) setWindowHeight(Settings::height() + 10);
}

void MainWindow::decreaseWindowHeight()
{
    if (Settings::height() >= 20) setWindowHeight(Settings::height() - 10);
}

void MainWindow::updateMask()
{
    QRegion region = m_titleBar->mask();

    region.translate(0, m_titleBar->y());

    region += QRegion(0, 0, width(), m_titleBar->y());

    setMask(region);
}

void MainWindow::paintEvent(QPaintEvent* event)
{
    QPainter painter(this);

    if (useTranslucency())
    {
        painter.setOpacity(qreal(Settings::backgroundColorOpacity()) / 100);
        painter.fillRect(rect(), Settings::backgroundColor());
        painter.setOpacity(1.0);
    }
    else
        painter.fillRect(rect(), Settings::backgroundColor());

    QRect leftBorder(0, 0, m_skin->borderWidth(), height() - m_titleBar->height());
    painter.fillRect(leftBorder, m_skin->borderColor());

    QRect rightBorder(width() - m_skin->borderWidth(), 0, m_skin->borderWidth(),
        height() - m_titleBar->height());
    painter.fillRect(rightBorder, m_skin->borderColor());

    KMainWindow::paintEvent(event);
}

void MainWindow::moveEvent(QMoveEvent* event)
{
    if (Settings::screen() && QApplication::desktop()->screenNumber(this) != getScreen())
    {
        Settings::setScreen(QApplication::desktop()->screenNumber(this)+1);

        updateScreenMenu();

        applyWindowGeometry();
    }

    KMainWindow::moveEvent(event);
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    KMainWindow::closeEvent(event);
    if (event->isAccepted()) {
        QApplication::quit();
    }
}

void MainWindow::wmActiveWindowChanged()
{
    if (m_toggleLock) {
        m_toggleLock = false;
        return;
    }

    KWindowInfo info(KWindowSystem::activeWindow(), 0, NET::WM2TransientFor);

    if (info.valid() && info.transientFor() == winId()) {
        return;
    }


    if (m_isX11) {
        if (!Settings::keepOpen() && isVisible() && KWindowSystem::activeWindow() != winId()) {
            toggleWindowState();
        }
    } else {
        if (!Settings::keepOpen() && hasFocus()) {
            toggleWindowState();
        }
    }
}

void MainWindow::changeEvent(QEvent* event)
{
    if (event->type() == QEvent::WindowStateChange
        && (windowState() & Qt::WindowMaximized)
        && Settings::width() != 100
        && Settings::height() != 100)
    {
        Settings::setWidth(100);
        Settings::setHeight(100);

        applyWindowGeometry();

        updateWindowWidthMenu();
        updateWindowHeightMenu();
    }

    KMainWindow::changeEvent(event);
}

bool MainWindow::event(QEvent* event)
{
    if (event->type() == QEvent::Expose) {
        // FIXME TODO: We can remove this once we depend on Qt 5.6.1+.
        // See: https://bugreports.qt.io/browse/QTBUG-26978
        applyWindowProperties();
#if (QT_VERSION > QT_VERSION_CHECK(5, 5, 0))
    } else if (event->type() == QEvent::PlatformSurface) {
        const QPlatformSurfaceEvent *pSEvent = static_cast<QPlatformSurfaceEvent *>(event);

        if (pSEvent->surfaceEventType() == QPlatformSurfaceEvent::SurfaceCreated) {
            applyWindowProperties();
        }
#endif
    }

    return KMainWindow::event(event);
}

void MainWindow::toggleWindowState()
{
    bool visible = isVisible();

    if (visible && KWindowSystem::activeWindow() != winId() && Settings::keepOpen())
    {
        // Window is open but doesn't have focus; it's set to stay open
        // regardless of focus loss.

        if (Settings::toggleToFocus())
        {
            // The open/retract action is set to focus the window when it's
            // open but lacks focus. The following will cause it to receive
            // focus, and in an environment with multiple virtual desktops
            // will also cause the window manager to switch to the virtual
            // desktop the window resides on.

            KWindowSystem::activateWindow(winId());
            KWindowSystem::forceActiveWindow(winId());

            return;
        }
        else if (!Settings::showOnAllDesktops()
                 &&  KWindowInfo(winId(), NET::WMDesktop).desktop() != KWindowSystem::currentDesktop())
        {
            // The open/restrict action isn't set to focus the window, but
            // the window is currently on another virtual desktop (the option
            // to show it on all of them is disabled), so closing it doesn't
            // make sense and we're opting to show it instead to avoid
            // requiring the user to invoke the action twice to get to see
            // Yakuake. Just forcing focus would cause the window manager to
            // switch to the virtual desktop the window currently resides on,
            // so move the window to the current desktop before doing so.

            KWindowSystem::setOnDesktop(winId(), KWindowSystem::currentDesktop());

            KWindowSystem::activateWindow(winId());
            KWindowSystem::forceActiveWindow(winId());

            return;
        }
    }

#if HAVE_X11
    if (!Settings::useWMAssist() && m_kwinAssistPropSet)
        kwinAssistPropCleanup();

    if (m_isX11 && Settings::useWMAssist() && KWindowSystem::compositingActive())
        kwinAssistToggleWindowState(visible);
    else
#endif
    if (!m_isWayland) {
        xshapeToggleWindowState(visible);
    } else {
        if (visible)
        {
            sharedPreHideWindow();

            hide();

            sharedAfterHideWindow();
        }
        else
        {
            sharedPreOpenWindow();
            if (KWindowEffects::isEffectAvailable(KWindowEffects::Slide)) {
                KWindowEffects::slideWindow(this, KWindowEffects::TopEdge);
            }

            show();

            sharedAfterOpenWindow();
        }
    }
}

#if HAVE_X11
void MainWindow::kwinAssistToggleWindowState(bool visible)
{
    bool gotEffect = false;

    Display* display = QX11Info::display();
    Atom atom = XInternAtom(display, "_KDE_SLIDE", false);
    int count;
    Atom* list = XListProperties(display, DefaultRootWindow(display), &count);

    if (list != NULL)
    {
        gotEffect = (qFind(list, list + count, atom) != list + count);

        XFree(list);
    }

    if (gotEffect)
    {
        Atom atom = XInternAtom(display, "_KDE_SLIDE", false);

        if (Settings::frames() > 0)
        {
            QVarLengthArray<long, 1024> data(4);

            data[0] = 0;
            data[1] = 1;
            data[2] = Settings::frames() * 10;
            data[3] = Settings::frames() * 10;

            XChangeProperty(display, winId(), atom, atom, 32, PropModeReplace,
                reinterpret_cast<unsigned char *>(data.data()), data.size());

            m_kwinAssistPropSet = true;
        }
        else
            XDeleteProperty(display, winId(), atom);

        if (visible)
        {
            sharedPreHideWindow();

            hide();

            sharedAfterHideWindow();
        }
        else
        {
            sharedPreOpenWindow();

            show();

            sharedAfterOpenWindow();
        }

        return;
    }

    // Fall back to legacy animation strategy if kwin doesn't have the
    // effect loaded.
    xshapeToggleWindowState(visible);
}

void MainWindow::kwinAssistPropCleanup()
{
    if (!QX11Info::isPlatformX11())
        return;

    Display* display = QX11Info::display();
    Atom atom = XInternAtom(display, "_KDE_SLIDE", false);

    XDeleteProperty(display, winId(), atom);

    m_kwinAssistPropSet = false;
}
#endif

void MainWindow::xshapeToggleWindowState(bool visible)
{
    if (m_animationTimer.isActive()) return;

    if (visible)
    {
        sharedPreHideWindow();

        m_animationFrame = Settings::frames();

        connect(&m_animationTimer, SIGNAL(timeout()), this, SLOT(xshapeRetractWindow()));
        m_animationTimer.start();
    }
    else
    {
        m_animationFrame = 0;

        connect(&m_animationTimer, SIGNAL(timeout()), this, SLOT(xshapeOpenWindow()));
        m_animationTimer.start();
    }
}

void MainWindow::xshapeOpenWindow()
{
    if (m_animationFrame == 0)
    {
        sharedPreOpenWindow();

        show();

        sharedAfterOpenWindow();
    }

    if (m_animationFrame == Settings::frames())
    {
        m_animationTimer.stop();
        m_animationTimer.disconnect();

        m_titleBar->move(0, height() - m_titleBar->height());
        updateMask();
    }
    else
    {
        int maskHeight = m_animationStepSize * m_animationFrame;

        QRegion newMask = m_titleBar->mask();
        newMask.translate(0, maskHeight);
        newMask += QRegion(0, 0, width(), maskHeight);

        m_titleBar->move(0, maskHeight);
        setMask(newMask);

        m_animationFrame++;
    }
}

void MainWindow::xshapeRetractWindow()
{
    if (m_animationFrame == 0)
    {
        m_animationTimer.stop();
        m_animationTimer.disconnect();

        hide();

        sharedAfterHideWindow();
    }
    else
    {
        m_titleBar->move(0,m_titleBar->y() - m_animationStepSize);
        setMask(QRegion(mask()).translated(0, -m_animationStepSize));

        --m_animationFrame;
    }
}

void MainWindow::sharedPreOpenWindow()
{
    applyWindowGeometry();

    updateUseTranslucency();

    if (Settings::pollMouse()) toggleMousePoll(false);
    if (Settings::rememberFullscreen()) setFullScreen(m_isFullscreen);
}

void MainWindow::sharedAfterOpenWindow()
{
    if (!Settings::firstRun()) KWindowSystem::forceActiveWindow(winId());

    connect(KWindowSystem::self(), &KWindowSystem::activeWindowChanged,
        this, &MainWindow::wmActiveWindowChanged);

    applyWindowProperties();

#if HAVE_KWAYLAND
    initWaylandSurface();
#endif

    emit windowOpened();
}

void MainWindow::sharedPreHideWindow()
{
    disconnect(KWindowSystem::self(), &KWindowSystem::activeWindowChanged,
        this, &MainWindow::wmActiveWindowChanged);
}

void MainWindow::sharedAfterHideWindow()
{
    if (Settings::pollMouse()) toggleMousePoll(true);

#if HAVE_KWAYLAND
    delete m_plasmaShellSurface;
    m_plasmaShellSurface = Q_NULLPTR;
#endif

    emit windowClosed();
}

void MainWindow::activate()
{
    KWindowSystem::activateWindow(winId());
}

void MainWindow::toggleMousePoll(bool poll)
{
    if (poll)
        m_mousePoller.start(Settings::pollInterval());
    else
        m_mousePoller.stop();
}

void MainWindow::pollMouse()
{
    QPoint pos = QCursor::pos();
    QRect workArea = getDesktopGeometry();

    int windowX = workArea.x() + workArea.width() * Settings::position() * (100 - Settings::width()) / 10000;
    int windowWidth = workArea.width() * Settings::width() / 100;

    if (pos.y() == 0 && pos.x() >= windowX && pos.x() <= (windowX + windowWidth))
        toggleWindowState();
}

void MainWindow::setKeepOpen(bool keepOpen)
{
    if (Settings::keepOpen() != keepOpen)
    {
        Settings::setKeepOpen(keepOpen);
        Settings::self()->save();

        applyWindowProperties();
    }

    actionCollection()->action(QStringLiteral("keep-open"))->setChecked(keepOpen);
    m_titleBar->setFocusButtonState(keepOpen);
}

void MainWindow::setFullScreen(bool state)
{
    if (isVisible()) m_isFullscreen = state;
    if (state)
    {
        setWindowState(windowState() | Qt::WindowFullScreen);
        setWindowGeometry(100, 100, Settings::position());
    }
    else
    {
        setWindowState(windowState() & ~Qt::WindowFullScreen);
        setWindowGeometry(Settings::width(), Settings::height(), Settings::position());
    }
}

int MainWindow::getScreen()
{
    if (!Settings::screen())
        return QApplication::desktop()->screenNumber(QCursor::pos());
    else
        return Settings::screen() - 1;
}

QRect MainWindow::getDesktopGeometry()
{
    QRect screenGeometry = QApplication::desktop()->screenGeometry(getScreen());

    QAction* action = actionCollection()->action(QStringLiteral("view-full-screen"));

    if (action->isChecked())
        return screenGeometry;

    if (m_isWayland) {
        // on Wayland it's not possible to get the work area
        return screenGeometry;
    }

    if (QApplication::desktop()->screenCount() > 1)
    {
        const QList<WId> allWindows = KWindowSystem::windows();
        QList<WId> offScreenWindows;

        QListIterator<WId> i(allWindows);

        while (i.hasNext())
        {
            WId windowId = i.next();

            if (KWindowSystem::hasWId(windowId))
            {
                KWindowInfo windowInfo = KWindowInfo(windowId, NET::WMDesktop, NET::WM2ExtendedStrut);

                // If windowInfo is valid and the window is located at the same (current)
                // desktop with the yakuake window...
                if (windowInfo.valid() && windowInfo.isOnCurrentDesktop())
                {
                    NETExtendedStrut strut = windowInfo.extendedStrut();

                    // Get the area covered by each strut.
                    QRect topStrut(strut.top_start, 0, strut.top_end - strut.top_start, strut.top_width);
                    QRect bottomStrut(strut.bottom_start, screenGeometry.bottom() - strut.bottom_width,
                                      strut.bottom_end - strut.bottom_start, strut.bottom_width);
                    QRect leftStrut(0, strut.left_width, strut.left_start, strut.left_end - strut.left_start);
                    QRect rightStrut(screenGeometry.right() - strut.right_width, strut.right_start,
                                     strut.right_end - strut.right_start, strut.right_width);

                    // If the window has no strut, no need to bother further.
                    if (topStrut.isEmpty() && bottomStrut.isEmpty() && leftStrut.isEmpty() && rightStrut.isEmpty())
                        continue;

                    // If any of the strut intersects with our screen geometry, it will be correctly handled
                    // by workArea().
                    if (topStrut.intersects(screenGeometry) || bottomStrut.intersects(screenGeometry) ||
                        leftStrut.intersects(screenGeometry) || rightStrut.intersects(screenGeometry))
                        continue;

                    // This window has a strut on the same desktop as us but which does not cover our screen
                    // geometry. It should be ignored, otherwise the returned work area will wrongly include
                    // the strut.
                    offScreenWindows << windowId;
                }
            }
        }

        return KWindowSystem::workArea(offScreenWindows).intersected(screenGeometry);
    }

    return KWindowSystem::workArea();
}

void MainWindow::whatsThis()
{
    QWhatsThis::enterWhatsThisMode();
}

void MainWindow::showStartupPopup()
{
    QAction* action = static_cast<QAction*>(actionCollection()->action(QStringLiteral("toggle-window-state")));

    const QList<QKeySequence> &shortcuts = KGlobalAccel::self()->shortcut(action);

    if (shortcuts.isEmpty())
        return;

    QString shortcut(shortcuts.first().toString(QKeySequence::NativeText));
    QString message(xi18nc("@info", "Application successfully started.<nl/>" "Press <shortcut>%1</shortcut> to use it ...", shortcut));

    KNotification::event(QLatin1String("startup"), message, QPixmap(), this);
}

void MainWindow::showFirstRunDialog()
{
    if (!m_firstRunDialog)
    {
        m_firstRunDialog = new FirstRunDialog(this);

        connect(m_firstRunDialog, &QDialog::finished, this, &MainWindow::firstRunDialogFinished);
        connect(m_firstRunDialog, &QDialog::accepted, this, &MainWindow::firstRunDialogOk);
    }

    m_firstRunDialog->show();
}

void MainWindow::firstRunDialogFinished()
{
    Settings::setFirstRun(false);
    Settings::self()->save();

    m_firstRunDialog->deleteLater();

    KWindowSystem::forceActiveWindow(winId());
}

void MainWindow::firstRunDialogOk()
{
    QAction* action = static_cast<QAction*>(actionCollection()->action(QStringLiteral("toggle-window-state")));

    KGlobalAccel::self()->setShortcut(action, QList<QKeySequence>() << m_firstRunDialog->keySequence(),
        KGlobalAccel::NoAutoloading);

    actionCollection()->writeSettings();
}

void MainWindow::updateUseTranslucency()
{
    m_useTranslucency = (Settings::translucency() && KWindowSystem::compositingActive());
}
