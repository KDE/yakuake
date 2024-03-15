/*
  SPDX-FileCopyrightText: 2008-2014 Eike Hein <hein@kde.org>
  SPDX-FileCopyrightText: 2009 Juan Carlos Torres <carlosdgtorres@gmail.com>
  SPDX-FileCopyrightText: 2020 Ryan McCoskrie <work@ryanmccoskrie.me>

  SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#include "mainwindow.h"
#include "config/appearancesettings.h"
#include "config/windowsettings.h"
#include "firstrundialog.h"
#include "sessionstack.h"
#include "settings.h"
#include "skin.h"
#include "tabbar.h"
#include "terminal.h"
#include "titlebar.h"
#include "ui_behaviorsettings.h"

#include <KAboutData>
#include <KActionCollection>
#include <KConfigDialog>
#include <KGlobalAccel>
#include <KHelpMenu>
#include <KLocalizedString>
#include <KMessageBox>
#include <KNotification>
#include <KNotifyConfigWidget>
#include <KShortcutsDialog>
#include <KStandardAction>
#include <KStatusNotifierItem>
#include <KToggleFullScreenAction>
#include <KWindowEffects>
#include <KWindowInfo>
#include <KWindowSystem>
#include <KX11Extras>

#include <QApplication>
#include <QDBusConnection>
#include <QDBusPendingReply>
#include <QDBusReply>
#include <QMenu>
#include <QPainter>
#include <QScreen>
#include <QWhatsThis>
#include <QWindow>

#if HAVE_X11
#include <private/qtx11extras_p.h>

#include <X11/Xlib.h>
#include <fixx11h.h>
#endif

#if HAVE_KWAYLAND
#include <KWayland/Client/connection_thread.h>
#include <KWayland/Client/plasmashell.h>
#include <KWayland/Client/registry.h>
#include <KWayland/Client/surface.h>
#endif

MainWindow::MainWindow(QWidget *parent)
    : KMainWindow(parent, Qt::CustomizeWindowHint | Qt::FramelessWindowHint | Qt::Tool)
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
    m_notifierItem = nullptr;

    m_firstRunDialog = nullptr;
    m_isFullscreen = false;

#if HAVE_X11
    m_kwinAssistPropSet = false;
    m_isX11 = KWindowSystem::isPlatformX11();
#else
    m_isX11 = false;
#endif
    m_isWayland = KWindowSystem::isPlatformWayland();
#if HAVE_KWAYLAND
    m_plasmaShell = nullptr;
    m_plasmaShellSurface = nullptr;
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
    connect(m_tabBar, &TabBar::tabTitleEdited, m_sessionStack, [&](int, QString) {
        m_sessionStack->raiseSession(m_sessionStack->activeSessionId());
    });
    connect(m_tabBar, SIGNAL(requestTerminalHighlight(int)), m_sessionStack, SLOT(handleTerminalHighlightRequest(int)));
    connect(m_tabBar, SIGNAL(requestRemoveTerminalHighlight()), m_sessionStack, SIGNAL(removeTerminalHighlight()));
    connect(m_tabBar, SIGNAL(tabContextMenuClosed()), m_sessionStack, SIGNAL(removeTerminalHighlight()));

    connect(m_sessionStack, SIGNAL(sessionAdded(int, QString)), m_tabBar, SLOT(addTab(int, QString)));
    connect(m_sessionStack, SIGNAL(sessionRaised(int)), m_tabBar, SLOT(selectTab(int)));
    connect(m_sessionStack, SIGNAL(sessionRemoved(int)), m_tabBar, SLOT(removeTab(int)));
    connect(m_sessionStack, SIGNAL(activeTitleChanged(QString)), m_titleBar, SLOT(setTitle(QString)));
    connect(m_sessionStack, SIGNAL(activeTitleChanged(QString)), this, SLOT(setWindowTitle(QString)));
    connect(m_sessionStack, &SessionStack::wantsBlurChanged, this, &MainWindow::applyWindowProperties);

    connect(&m_mousePoller, SIGNAL(timeout()), this, SLOT(pollMouse()));

    if (KWindowSystem::isPlatformX11()) {
        connect(KX11Extras::self(), &KX11Extras::workAreaChanged, this, &MainWindow::applyWindowGeometry);
    }
    connect(qApp, &QGuiApplication::screenAdded, this, &MainWindow::updateScreenMenu);
    connect(qApp, &QGuiApplication::screenRemoved, this, &MainWindow::updateScreenMenu);

    applySettings();

    m_sessionStack->addSession();

    if (Settings::firstRun()) {
        QMetaObject::invokeMethod(this, "toggleWindowState", Qt::QueuedConnection);
        QMetaObject::invokeMethod(this, "showFirstRunDialog", Qt::QueuedConnection);
    } else {
        if (Settings::pollMouse())
            toggleMousePoll(true);
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
    QObject::connect(registry, &Registry::interfacesAnnounced, this, [registry, this] {
        const auto interface = registry->interface(Registry::Interface::PlasmaShell);
        if (interface.name != 0) {
            m_plasmaShell = registry->createPlasmaShell(interface.name, interface.version, this);
        }
    });

    registry->setup();
    connection->roundtrip();
}

void MainWindow::initWaylandSurface()
{
    if (m_plasmaShellSurface) {
        m_plasmaShellSurface->setPosition(pos());
        return;
    }
    if (!m_plasmaShell) {
        return;
    }
    if (auto surface = KWayland::Client::Surface::fromWindow(windowHandle())) {
        m_plasmaShellSurface = m_plasmaShell->createSurface(surface, this);
        m_plasmaShellSurface->setPosition(pos());
        m_plasmaShellSurface->setSkipTaskbar(true);
        m_plasmaShellSurface->setSkipSwitcher(true);
    }
}

#endif

bool MainWindow::queryClose()
{
    bool confirmQuit = Settings::confirmQuit();
    bool hasUnclosableSessions = m_sessionStack->hasUnclosableSessions();

    QString closeQuestion = xi18nc("@info", "Are you sure you want to quit?");
    QString warningMessage;

    if ((confirmQuit && m_sessionStack->count() > 1) || hasUnclosableSessions) {
        if (confirmQuit && m_sessionStack->count() > 1) {
            if (hasUnclosableSessions)
                warningMessage = xi18nc("@info",
                                        "<warning>There are multiple open sessions, <emphasis>some of which you have locked to prevent closing them "
                                        "accidentally.</emphasis> These will be killed if you continue.</warning>");
            else
                warningMessage = xi18nc("@info", "<warning>There are multiple open sessions. These will be killed if you continue.</warning>");
        } else if (hasUnclosableSessions) {
            warningMessage = xi18nc("@info",
                                    "<warning>There are one or more open sessions that you have locked to prevent closing them accidentally. These will be "
                                    "killed if you continue.</warning>");
        }

        int result = KMessageBox::warningContinueCancel(this,
                                                        warningMessage + QStringLiteral("<br /><br />") + closeQuestion,
                                                        xi18nc("@title:window", "Really Quit?"),
                                                        KStandardGuiItem::quit(),
                                                        KStandardGuiItem::cancel());

        return result != KMessageBox::Cancel;
    }

    return true;
}

void MainWindow::setupActions()
{
    m_actionCollection = new KActionCollection(this);

    KToggleFullScreenAction *fullScreenAction = new KToggleFullScreenAction(this);
    fullScreenAction->setWindow(this);
    actionCollection()->setDefaultShortcut(fullScreenAction, QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_F11));
    m_actionCollection->addAction(QStringLiteral("view-full-screen"), fullScreenAction);
    connect(fullScreenAction, SIGNAL(toggled(bool)), this, SLOT(setFullScreen(bool)));

    QAction *action = KStandardAction::quit(this, SLOT(close()), actionCollection());
    actionCollection()->setDefaultShortcut(action, QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_Q));
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
    KGlobalAccel::self()->setGlobalShortcut(action, QList<QKeySequence>() << QKeySequence(Qt::Key_F12));
    connect(action, SIGNAL(triggered()), this, SLOT(toggleWindowState()));
    connect(action, SIGNAL(changed()), this, SLOT(updateTrayTooltip()));
    connect(KGlobalAccel::self(), SIGNAL(globalShortcutChanged(QAction *, const QKeySequence &)), this, SLOT(updateTrayTooltip()));
    updateTrayTooltip();

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
    actionCollection()->setDefaultShortcut(action, QKeySequence(Qt::ALT | Qt::SHIFT | Qt::Key_Right));
    connect(action, SIGNAL(triggered()), this, SLOT(increaseWindowWidth()));

    action = actionCollection()->addAction(QStringLiteral("decrease-window-width"));
    action->setText(xi18nc("@action", "Decrease Window Width"));
    actionCollection()->setDefaultShortcut(action, QKeySequence(Qt::ALT | Qt::SHIFT | Qt::Key_Left));
    connect(action, SIGNAL(triggered()), this, SLOT(decreaseWindowWidth()));

    action = actionCollection()->addAction(QStringLiteral("increase-window-height"));
    action->setText(xi18nc("@action", "Increase Window Height"));
    actionCollection()->setDefaultShortcut(action, QKeySequence(Qt::ALT | Qt::SHIFT | Qt::Key_Down));
    connect(action, SIGNAL(triggered()), this, SLOT(increaseWindowHeight()));

    action = actionCollection()->addAction(QStringLiteral("decrease-window-height"));
    action->setText(xi18nc("@action", "Decrease Window Height"));
    actionCollection()->setDefaultShortcut(action, QKeySequence(Qt::ALT | Qt::SHIFT | Qt::Key_Up));
    connect(action, SIGNAL(triggered()), this, SLOT(decreaseWindowHeight()));

    action = actionCollection()->addAction(QStringLiteral("new-session"));
    action->setText(xi18nc("@action", "New Session"));
    action->setIcon(QIcon::fromTheme(QStringLiteral("tab-new")));
    actionCollection()->setDefaultShortcut(action, QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_T));
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
    actionCollection()->setDefaultShortcut(action, QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_W));
    connect(action, SIGNAL(triggered()), this, SLOT(handleContextDependentAction()));
    m_contextDependentActions << action;

    action = actionCollection()->addAction(QStringLiteral("previous-session"));
    action->setText(xi18nc("@action", "Previous Session"));
    action->setIcon(QIcon::fromTheme(QStringLiteral("go-previous")));
    actionCollection()->setDefaultShortcut(action, QKeySequence(Qt::SHIFT | Qt::Key_Left));
    connect(action, SIGNAL(triggered()), m_tabBar, SLOT(selectPreviousTab()));

    action = actionCollection()->addAction(QStringLiteral("next-session"));
    action->setText(xi18nc("@action", "Next Session"));
    action->setIcon(QIcon::fromTheme(QStringLiteral("go-next")));
    actionCollection()->setDefaultShortcut(action, QKeySequence(Qt::SHIFT | Qt::Key_Right));
    connect(action, SIGNAL(triggered()), m_tabBar, SLOT(selectNextTab()));

    action = actionCollection()->addAction(QStringLiteral("move-session-left"));
    action->setText(xi18nc("@action", "Move Session Left"));
    action->setIcon(QIcon::fromTheme(QStringLiteral("arrow-left")));
    actionCollection()->setDefaultShortcut(action, QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_Left));
    connect(action, SIGNAL(triggered()), this, SLOT(handleContextDependentAction()));
    m_contextDependentActions << action;

    action = actionCollection()->addAction(QStringLiteral("move-session-right"));
    action->setText(xi18nc("@action", "Move Session Right"));
    action->setIcon(QIcon::fromTheme(QStringLiteral("arrow-right")));
    actionCollection()->setDefaultShortcut(action, QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_Right));
    connect(action, SIGNAL(triggered()), this, SLOT(handleContextDependentAction()));
    m_contextDependentActions << action;

    action = actionCollection()->addAction(QStringLiteral("grow-terminal-right"));
    action->setText(xi18nc("@action", "Grow Terminal to the Right"));
    action->setIcon(QIcon::fromTheme(QStringLiteral("arrow-right")));
    actionCollection()->setDefaultShortcut(action, QKeySequence(Qt::CTRL | Qt::ALT | Qt::Key_Right));
    connect(action, SIGNAL(triggered()), this, SLOT(handleContextDependentAction()));
    m_contextDependentActions << action;

    action = actionCollection()->addAction(QStringLiteral("grow-terminal-left"));
    action->setText(xi18nc("@action", "Grow Terminal to the Left"));
    action->setIcon(QIcon::fromTheme(QStringLiteral("arrow-left")));
    actionCollection()->setDefaultShortcut(action, QKeySequence(Qt::CTRL | Qt::ALT | Qt::Key_Left));
    connect(action, SIGNAL(triggered()), this, SLOT(handleContextDependentAction()));
    m_contextDependentActions << action;

    action = actionCollection()->addAction(QStringLiteral("grow-terminal-top"));
    action->setText(xi18nc("@action", "Grow Terminal to the Top"));
    action->setIcon(QIcon::fromTheme(QStringLiteral("arrow-up")));
    actionCollection()->setDefaultShortcut(action, QKeySequence(Qt::CTRL | Qt::ALT | Qt::Key_Up));
    connect(action, SIGNAL(triggered()), this, SLOT(handleContextDependentAction()));
    m_contextDependentActions << action;

    action = actionCollection()->addAction(QStringLiteral("grow-terminal-bottom"));
    action->setText(xi18nc("@action", "Grow Terminal to the Bottom"));
    action->setIcon(QIcon::fromTheme(QStringLiteral("arrow-down")));
    actionCollection()->setDefaultShortcut(action, QKeySequence(Qt::CTRL | Qt::ALT | Qt::Key_Down));
    connect(action, SIGNAL(triggered()), this, SLOT(handleContextDependentAction()));
    m_contextDependentActions << action;

    action = actionCollection()->addAction(QStringLiteral("rename-session"));
    action->setText(xi18nc("@action", "Rename Session..."));
    action->setIcon(QIcon::fromTheme(QStringLiteral("edit-rename")));
    actionCollection()->setDefaultShortcut(action, QKeySequence(Qt::CTRL | Qt::ALT | Qt::Key_S));
    connect(action, SIGNAL(triggered()), this, SLOT(handleContextDependentAction()));
    m_contextDependentActions << action;

    action = actionCollection()->addAction(QStringLiteral("previous-terminal"));
    action->setText(xi18nc("@action", "Previous Terminal"));
    action->setIcon(QIcon::fromTheme(QStringLiteral("go-previous")));
    actionCollection()->setDefaultShortcut(action, QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_Tab));
    connect(action, SIGNAL(triggered()), m_sessionStack, SIGNAL(previousTerminal()));

    action = actionCollection()->addAction(QStringLiteral("next-terminal"));
    action->setText(xi18nc("@action", "Next Terminal"));
    action->setIcon(QIcon::fromTheme(QStringLiteral("go-next")));
    actionCollection()->setDefaultShortcut(action, QKeySequence(Qt::CTRL | Qt::Key_Tab));
    connect(action, SIGNAL(triggered()), m_sessionStack, SIGNAL(nextTerminal()));

    action = actionCollection()->addAction(QStringLiteral("close-active-terminal"));
    action->setText(xi18nc("@action", "Close Active Terminal"));
    action->setIcon(QIcon::fromTheme(QStringLiteral("view-close")));
    actionCollection()->setDefaultShortcut(action, QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_R));
    connect(action, SIGNAL(triggered()), this, SLOT(handleContextDependentAction()));
    m_contextDependentActions << action;

    action = actionCollection()->addAction(QStringLiteral("split-left-right"));
    action->setText(xi18nc("@action", "Split Left/Right"));
    action->setIcon(QIcon::fromTheme(QStringLiteral("view-split-left-right")));
    actionCollection()->setDefaultShortcut(action, QKeySequence(Qt::CTRL | Qt::Key_ParenLeft));
    connect(action, SIGNAL(triggered()), this, SLOT(handleContextDependentAction()));
    m_contextDependentActions << action;

    action = actionCollection()->addAction(QStringLiteral("split-top-bottom"));
    action->setText(xi18nc("@action", "Split Top/Bottom"));
    action->setIcon(QIcon::fromTheme(QStringLiteral("view-split-top-bottom")));
    actionCollection()->setDefaultShortcut(action, QKeySequence(Qt::CTRL | Qt::Key_ParenRight));
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
    actionCollection()->setDefaultShortcut(action, QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_A));
    action->setCheckable(true);
    connect(action, SIGNAL(triggered(bool)), this, SLOT(handleContextDependentToggleAction(bool)));
    m_contextDependentActions << action;

    action = actionCollection()->addAction(QStringLiteral("toggle-session-monitor-silence"));
    action->setText(xi18nc("@action", "Monitor for Silence"));
    actionCollection()->setDefaultShortcut(action, QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_I));
    action->setCheckable(true);
    connect(action, SIGNAL(triggered(bool)), this, SLOT(handleContextDependentToggleAction(bool)));
    m_contextDependentActions << action;

    action = actionCollection()->addAction(QStringLiteral("toggle-titlebar"));
    action->setText(xi18nc("@action", "Toggle Titlebar"));
    actionCollection()->setDefaultShortcut(action, QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_M));
    action->setCheckable(true);
    connect(action, SIGNAL(triggered()), this, SLOT(handleToggleTitlebar()));

    for (uint i = 1; i <= 10; ++i) {
        action = actionCollection()->addAction(QStringLiteral("switch-to-session-%1").arg(i));
        action->setText(xi18nc("@action", "Switch to Session %1", i));
        action->setData(i - 1);
        connect(action, SIGNAL(triggered()), this, SLOT(handleSwitchToAction()));

        if (i < 10) {
            // add default shortcut bindings for the first 9 sessions
            actionCollection()->setDefaultShortcut(action, QStringLiteral("Alt+%1").arg(i));
        } else {
            // add default shortcut bindings for the 10th session
            actionCollection()->setDefaultShortcut(action, Qt::ALT | Qt::Key_0);
        }
    }

    m_actionCollection->associateWidget(this);
    m_actionCollection->readSettings();
}

void MainWindow::handleContextDependentAction(QAction *action, int sessionId)
{
    if (sessionId == -1)
        sessionId = m_sessionStack->activeSessionId();
    if (sessionId == -1)
        return;

    if (!action)
        action = qobject_cast<QAction *>(QObject::sender());

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

void MainWindow::handleContextDependentToggleAction(bool checked, QAction *action, int sessionId)
{
    if (sessionId == -1)
        sessionId = m_sessionStack->activeSessionId();
    if (sessionId == -1)
        return;

    if (!action)
        action = qobject_cast<QAction *>(QObject::sender());

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
    QListIterator<QAction *> i(m_contextDependentActions);

    while (i.hasNext())
        i.next()->blockSignals(quiet);
}

void MainWindow::handleToggleTerminalKeyboardInput(bool checked)
{
    QAction *action = qobject_cast<QAction *>(QObject::sender());

    if (!action || action->data().isNull())
        return;

    bool ok = false;
    int terminalId = action->data().toInt(&ok);
    if (!ok)
        return;

    m_sessionStack->setTerminalKeyboardInputEnabled(terminalId, !checked);
}

void MainWindow::handleToggleTerminalMonitorActivity(bool checked)
{
    QAction *action = qobject_cast<QAction *>(QObject::sender());

    if (!action || action->data().isNull())
        return;

    bool ok = false;
    int terminalId = action->data().toInt(&ok);
    if (!ok)
        return;

    m_sessionStack->setTerminalMonitorActivityEnabled(terminalId, checked);
}

void MainWindow::handleToggleTerminalMonitorSilence(bool checked)
{
    QAction *action = qobject_cast<QAction *>(QObject::sender());

    if (!action || action->data().isNull())
        return;

    bool ok = false;
    int terminalId = action->data().toInt(&ok);
    if (!ok)
        return;

    m_sessionStack->setTerminalMonitorSilenceEnabled(terminalId, checked);
}

void MainWindow::handleTerminalActivity(Terminal *terminal)
{
    Session *session = qobject_cast<Session *>(sender());

    if (session) {
        disconnect(terminal, SIGNAL(activityDetected(Terminal *)), session, SIGNAL(activityDetected(Terminal *)));

        QString message(xi18nc("@info", "Activity detected in monitored terminal in session \"%1\".", m_tabBar->tabTitle(session->id())));

        KNotification *n = new KNotification(QLatin1String("activity"), KNotification::CloseWhenWindowActivated);
        n->setWindow(terminal->partWidget()->window()->windowHandle());
        n->setText(message);
        n->sendEvent();
    }
}

void MainWindow::handleTerminalSilence(Terminal *terminal)
{
    Session *session = qobject_cast<Session *>(sender());

    if (session) {
        QString message(xi18nc("@info", "Silence detected in monitored terminal in session \"%1\".", m_tabBar->tabTitle(session->id())));

        KNotification *n = new KNotification(QLatin1String("silence"), KNotification::CloseWhenWindowActivated);
        n->setWindow(terminal->partWidget()->window()->windowHandle());
        n->setText(message);
        n->sendEvent();
    }
}

void MainWindow::handleLastTabClosed()
{
    if (isVisible() && !Settings::keepOpenAfterLastSessionCloses())
        toggleWindowState();
}

void MainWindow::handleSwitchToAction()
{
    QAction *action = qobject_cast<QAction *>(QObject::sender());

    if (action && !action->data().isNull())
        m_sessionStack->raiseSession(m_tabBar->sessionAtTab(action->data().toInt()));
}

void MainWindow::handleToggleTitlebar()
{
    auto toggleFunc = [this]() {
        bool showTitleBar = !Settings::showTitleBar();
        m_titleBar->setVisible(showTitleBar);
        Settings::setShowTitleBar(showTitleBar);
        Settings::self()->save();
        applyWindowGeometry();
    };

    if (Settings::showTitleBar()) { // If the title bar is hidden don't ask if toggling is ok

        const char *message =
            "You are about to hide the title bar. This will keep you "
            "from accessing the settings menu via the mouse. To show "
            "the title bar again press the keyboard shortcut (default "
            "Ctrl+Shift+m) or access the settings menu via keyboard "
            "shortcut (default: Ctrl+Shift+,).";

        const int result = KMessageBox::warningContinueCancel(this,
                                                              xi18nc("@info", message),
                                                              xi18nc("@title:window", "Hiding Title Bar"),
                                                              KStandardGuiItem::cont(),
                                                              KStandardGuiItem::cancel(),
                                                              QStringLiteral("hinding_title_bar"));

        if (result == KMessageBox::ButtonCode::Continue) {
            toggleFunc();
        }
    } else {
        toggleFunc();
    }
}

void MainWindow::setupMenu()
{
    m_menu->insertSection(nullptr, xi18nc("@title:menu", "Help"));
    m_menu->addAction(actionCollection()->action(KStandardAction::name(KStandardAction::WhatsThis)));
    m_menu->addAction(actionCollection()->action(KStandardAction::name(KStandardAction::ReportBug)));
    m_menu->addAction(actionCollection()->action(KStandardAction::name(KStandardAction::AboutApp)));
    m_menu->addAction(actionCollection()->action(KStandardAction::name(KStandardAction::AboutKDE)));

    m_menu->insertSection(nullptr, xi18nc("@title:menu", "Quick Options"));
    m_menu->addAction(actionCollection()->action(QStringLiteral("view-full-screen")));
    m_menu->addAction(actionCollection()->action(QStringLiteral("keep-open")));

    m_screenMenu = new QMenu(this);
    connect(m_screenMenu, SIGNAL(triggered(QAction *)), this, SLOT(setScreen(QAction *)));
    m_screenMenu->setTitle(xi18nc("@title:menu", "Screen"));
    m_menu->addMenu(m_screenMenu);

    m_windowWidthMenu = new QMenu(this);
    connect(m_windowWidthMenu, SIGNAL(triggered(QAction *)), this, SLOT(setWindowWidth(QAction *)));
    m_windowWidthMenu->setTitle(xi18nc("@title:menu", "Width"));
    m_menu->addMenu(m_windowWidthMenu);

    m_windowHeightMenu = new QMenu(this);
    connect(m_windowHeightMenu, SIGNAL(triggered(QAction *)), this, SLOT(setWindowHeight(QAction *)));
    m_windowHeightMenu->setTitle(xi18nc("@title:menu", "Height"));
    m_menu->addMenu(m_windowHeightMenu);

    m_menu->insertSection(nullptr, xi18nc("@title:menu", "Settings"));
    m_menu->addAction(actionCollection()->action(QStringLiteral("manage-profiles")));
    m_menu->addAction(actionCollection()->action(KStandardAction::name(KStandardAction::KeyBindings)));
    m_menu->addAction(actionCollection()->action(KStandardAction::name(KStandardAction::ConfigureNotifications)));
    m_menu->addAction(actionCollection()->action(KStandardAction::name(KStandardAction::Preferences)));

    m_menu->addSeparator();
    m_menu->addAction(actionCollection()->action(KStandardAction::name(KStandardAction::Quit)));
}

void MainWindow::updateScreenMenu()
{
    QAction *action;

    m_screenMenu->clear();

    action = m_screenMenu->addAction(xi18nc("@item:inmenu", "At mouse location"));
    action->setCheckable(true);
    action->setData(0);
    action->setChecked(Settings::screen() == 0);

    for (int i = 1; i <= QGuiApplication::screens().count(); i++) {
        action = m_screenMenu->addAction(xi18nc("@item:inmenu", "Screen %1", i));
        action->setCheckable(true);
        action->setData(i);
        action->setChecked(i == Settings::screen());
    }

    action = m_screenMenu->menuAction();
    action->setVisible(QGuiApplication::screens().count() > 1);
}

void MainWindow::updateWindowSizeMenus()
{
    updateWindowWidthMenu();
    updateWindowHeightMenu();
}

void MainWindow::updateWindowWidthMenu()
{
    QAction *action = nullptr;

    if (m_windowWidthMenu->isEmpty()) {
        for (int i = 10; i <= 100; i += 10) {
            action = m_windowWidthMenu->addAction(i18n("%1%", i));
            action->setCheckable(true);
            action->setData(i);
            action->setChecked(i == Settings::width());
        }
    } else {
        QListIterator<QAction *> i(m_windowWidthMenu->actions());

        while (i.hasNext()) {
            action = i.next();

            action->setChecked(action->data().toInt() == Settings::width());
        }
    }
}

void MainWindow::updateWindowHeightMenu()
{
    QAction *action = nullptr;

    if (m_windowHeightMenu->isEmpty()) {
        for (int i = 10; i <= 100; i += 10) {
            action = m_windowHeightMenu->addAction(i18n("%1%", i));
            action->setCheckable(true);
            action->setData(i);
            action->setChecked(i == Settings::height());
        }
    } else {
        QListIterator<QAction *> i(m_windowHeightMenu->actions());

        while (i.hasNext()) {
            action = i.next();

            action->setChecked(action->data().toInt() == Settings::height());
        }
    }
}

void MainWindow::configureKeys()
{
    KShortcutsDialog dialog(KShortcutsEditor::AllActions, KShortcutsEditor::LetterShortcutsAllowed, this);
    dialog.addCollection(actionCollection());

    const auto collections = m_sessionStack->getPartActionCollections();

    if (collections.size() >= 1) {
        dialog.addCollection(collections.at(0), QStringLiteral("Konsolepart"));
    }

    if (!dialog.configure()) {
        return;
    }

    if (collections.size() >= 1) {
        // We need to update all the other collections
        // rootCollection is the collection which got updatet by the dialog
        const auto rootCollection = collections.at(0);

        // For all the other collections
        for (auto i = 1; i < collections.size(); ++i) {
            // Update all the action they share with rootCollection
            const auto rootActions = rootCollection->actions();
            for (const auto *action : rootActions) {
                if (auto *destaction = collections.at(i)->action(action->objectName())) {
                    destaction->setShortcuts(action->shortcuts());
                }
            }
        }
    }
}

void MainWindow::configureNotifications()
{
    KNotifyConfigWidget::configure(this);
}

void MainWindow::configureApp()
{
    if (KConfigDialog::showDialog(QStringLiteral("settings")))
        return;

    KConfigDialog *settingsDialog = new KConfigDialog(this, QStringLiteral("settings"), Settings::self());
    settingsDialog->setMinimumHeight(560);
    settingsDialog->setFaceType(KPageDialog::List);
    connect(settingsDialog, &KConfigDialog::settingsChanged, this, &MainWindow::applySettings);

    WindowSettings *windowSettings = new WindowSettings(settingsDialog);
    settingsDialog->addPage(windowSettings, xi18nc("@title Preferences page name", "Window"), QStringLiteral("preferences-system-windows-move"));
    connect(windowSettings, SIGNAL(updateWindowGeometry(int, int, int)), this, SLOT(setWindowGeometry(int, int, int)));

    QWidget *behaviorSettings = new QWidget(settingsDialog);
    Ui::BehaviorSettings behaviorSettingsUi;
    behaviorSettingsUi.setupUi(behaviorSettings);
    settingsDialog->addPage(behaviorSettings, xi18nc("@title Preferences page name", "Behavior"), QStringLiteral("preferences-system-windows-actions"));

    AppearanceSettings *appearanceSettings = new AppearanceSettings(settingsDialog);
    settingsDialog->addPage(appearanceSettings, xi18nc("@title Preferences page name", "Appearance"), QStringLiteral("preferences-desktop-theme"));
    connect(settingsDialog, &QDialog::rejected, appearanceSettings, &AppearanceSettings::resetSelection);

    settingsDialog->button(QDialogButtonBox::Help)->hide();
    settingsDialog->button(QDialogButtonBox::Cancel)->setFocus();

    connect(settingsDialog, &QDialog::finished, [=, this]() {
        m_toggleLock = true;
        KWindowSystem::activateWindow(windowHandle());

        if (KWindowSystem::isPlatformX11()) {
            KX11Extras::forceActiveWindow(winId());
        }
    });

    settingsDialog->show();
}

void MainWindow::applySettings()
{
    if (Settings::dynamicTabTitles()) {
        connect(m_sessionStack, SIGNAL(titleChanged(int, QString)), m_tabBar, SLOT(setTabTitleAutomated(int, QString)));

        m_sessionStack->emitTitles();
    } else {
        disconnect(m_sessionStack, SIGNAL(titleChanged(int, QString)), m_tabBar, SLOT(setTabTitleAutomated(int, QString)));
    }

    m_animationTimer.setInterval(Settings::frames() ? 10 : 0);

    m_tabBar->setVisible(Settings::showTabBar());
    m_titleBar->setVisible(Settings::showTitleBar());

    if (!Settings::showSystrayIcon() && m_notifierItem) {
        delete m_notifierItem;
        m_notifierItem = nullptr;

        // Removing the notifier item deletes the menu
        // add a new one
        m_menu = new QMenu(this);
        setupMenu();
        m_titleBar->updateMenu();
    } else if (Settings::showSystrayIcon() && !m_notifierItem) {
        m_notifierItem = new KStatusNotifierItem(this);
        m_notifierItem->setStandardActionsEnabled(false);
        m_notifierItem->setIconByName(QStringLiteral("yakuake-symbolic"));
        m_notifierItem->setStatus(KStatusNotifierItem::Active);
        m_notifierItem->setContextMenu(m_menu);

        // Prevent the default implementation of showing
        // and instead run toggleWindowState
        m_notifierItem->setAssociatedWindow(nullptr);
        connect(m_notifierItem, &KStatusNotifierItem::activateRequested, this, &MainWindow::toggleWindowState);
        updateTrayTooltip();
    }

    repaint(); // used to repaint skin borders if Settings::hideSkinBorders has been changed

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

    if (!gotSkin) {
        Settings::setSkin(QStringLiteral("default"));
        gotSkin = m_skin->load(Settings::skin());
    }

    if (!gotSkin) {
        KMessageBox::error(parentWidget(),
                           xi18nc("@info",
                                  "<application>Yakuake</application> was unable to load a skin. It is likely that it was installed incorrectly.<nl/><nl/>"
                                  "The application will now quit."),
                           xi18nc("@title:window", "Cannot Load Skin"));

        QMetaObject::invokeMethod(qApp, "quit", Qt::QueuedConnection);
    }

    m_titleBar->applySkin();
    m_tabBar->applySkin();
}

void MainWindow::applyWindowProperties()
{
    if (m_isX11) {
        if (Settings::keepOpen() && !Settings::keepAbove()) {
            KX11Extras::clearState(winId(), NET::KeepAbove);
            KX11Extras::setState(winId(), NET::SkipTaskbar | NET::SkipPager);
        } else {
            KX11Extras::setState(winId(), NET::KeepAbove | NET::SkipTaskbar | NET::SkipPager);
        }
        KX11Extras::setOnAllDesktops(winId(), Settings::showOnAllDesktops());
    }

#if HAVE_KWAYLAND
    if (m_isWayland && m_plasmaShellSurface) {
        m_plasmaShellSurface->setSkipTaskbar(true);
        m_plasmaShellSurface->setSkipSwitcher(true);
    }
#endif

    winId(); // make sure windowHandle() is created
    KWindowEffects::enableBlurBehind(windowHandle(), m_sessionStack->wantsBlur());
}

void MainWindow::applyWindowGeometry()
{
    int width, height;

    QAction *action = actionCollection()->action(QStringLiteral("view-full-screen"));

    if (action->isChecked()) {
        width = 100;
        height = 100;
    } else {
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

    setGeometry(workArea.x() + workArea.width() * newPosition * (100 - newWidth) / 10000, workArea.y(), targetWidth, maxHeight);
#if HAVE_KWAYLAND
    initWaylandSurface();
#endif

    maxHeight -= m_titleBar->height();
    m_titleBar->setGeometry(0, maxHeight, targetWidth, m_titleBar->height());
    if (!isVisible())
        m_titleBar->updateMask();

    if (Settings::frames() > 0)
        m_animationStepSize = maxHeight / Settings::frames();
    else
        m_animationStepSize = maxHeight;

    auto borderWidth = Settings::hideSkinBorders() ? 0 : m_skin->borderWidth();

    if (Settings::showTabBar()) {
        if (m_skin->tabBarCompact()) {
            m_tabBar->setGeometry(m_skin->tabBarLeft(), maxHeight, width() - m_skin->tabBarLeft() - m_skin->tabBarRight(), m_tabBar->height());
        } else {
            maxHeight -= m_tabBar->height();
            m_tabBar->setGeometry(borderWidth, maxHeight - borderWidth, width() - 2 * borderWidth, m_tabBar->height());
        }
    }

    m_sessionStack->setGeometry(borderWidth, 0, width() - 2 * borderWidth, maxHeight - borderWidth);

    updateMask();
}

void MainWindow::setScreen(QAction *action)
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

void MainWindow::setWindowWidth(QAction *action)
{
    setWindowWidth(action->data().toInt());
}

void MainWindow::setWindowHeight(QAction *action)
{
    setWindowHeight(action->data().toInt());
}

void MainWindow::increaseWindowWidth()
{
    if (Settings::width() <= 90)
        setWindowWidth(Settings::width() + 10);
}

void MainWindow::decreaseWindowWidth()
{
    if (Settings::width() >= 20)
        setWindowWidth(Settings::width() - 10);
}

void MainWindow::increaseWindowHeight()
{
    if (Settings::height() <= 90)
        setWindowHeight(Settings::height() + 10);
}

void MainWindow::decreaseWindowHeight()
{
    if (Settings::height() >= 20)
        setWindowHeight(Settings::height() - 10);
}

void MainWindow::updateMask()
{
    QRegion region = m_titleBar->mask();

    region.translate(0, m_titleBar->y());

    region += QRegion(0, 0, width(), m_titleBar->y());

    setMask(region);
}

void MainWindow::paintEvent(QPaintEvent *event)
{
    QPainter painter(this);

    if (useTranslucency()) {
        painter.setOpacity(qreal(Settings::backgroundColorOpacity()) / 100);
        painter.fillRect(rect(), Settings::backgroundColor());
        painter.setOpacity(1.0);
    } else
        painter.fillRect(rect(), Settings::backgroundColor());

    if (!Settings::hideSkinBorders()) {
        const QRect leftBorder(0, 0, m_skin->borderWidth(), height() - m_titleBar->height());
        painter.fillRect(leftBorder, m_skin->borderColor());

        const QRect rightBorder(width() - m_skin->borderWidth(), 0, m_skin->borderWidth(), height() - m_titleBar->height());
        painter.fillRect(rightBorder, m_skin->borderColor());

        const QRect bottomBorder(0, height() - m_skin->borderWidth() - m_titleBar->height(), width(), m_skin->borderWidth());
        painter.fillRect(bottomBorder, m_skin->borderColor());
    }

    KMainWindow::paintEvent(event);
}

void MainWindow::moveEvent(QMoveEvent *event)
{
    const QList<QScreen *> screens = qApp->screens();
    const QScreen *widgetScreen = QGuiApplication::screenAt(pos());
    auto it = std::find(screens.begin(), screens.end(), widgetScreen);
    const int currentScreenNumber = std::distance(screens.begin(), it);

    if (Settings::screen() && currentScreenNumber != -1 && currentScreenNumber != getScreen()) {
        Settings::setScreen(currentScreenNumber + 1);

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

    const QWindow *focusWindow = QGuiApplication::focusWindow();

    // Don't retract when opening one of our config windows
    if (focusWindow && focusWindow->transientParent() == windowHandle()) {
        return;
    }

    if (!Settings::keepOpen() && isVisible() && !isActiveWindow()) {
        toggleWindowState();
    }
}

void MainWindow::changeEvent(QEvent *event)
{
    if (event->type() == QEvent::WindowStateChange && !m_isFullscreen) {
        if (windowState().testFlag(Qt::WindowMaximized)) {
            // Don't alter settings to new size so unmaximizing restores previous geometry.
            setWindowGeometry(100, 100, Settings::position());
            setWindowState(Qt::WindowMaximized);
        } else {
            setWindowGeometry(Settings::width(), Settings::height(), Settings::position());
        }
    }

    KMainWindow::changeEvent(event);
}

bool MainWindow::focusNextPrevChild(bool)
{
    return false;
}

void MainWindow::toggleWindowState()
{
    if (m_isWayland) {
        auto message = QDBusMessage::createMethodCall(QStringLiteral("org.kde.plasmashell"),
                                                      QStringLiteral("/StrutManager"),
                                                      QStringLiteral("org.kde.PlasmaShell.StrutManager"),
                                                      QStringLiteral("availableScreenRect"));
        message.setArguments({QGuiApplication::screens().at(getScreen())->name()});
        QDBusPendingCall call = QDBusConnection::sessionBus().asyncCall(message);
        QDBusPendingCallWatcher *watcher = new QDBusPendingCallWatcher(call, this);

        QObject::connect(watcher, &QDBusPendingCallWatcher::finished, this, [=, this]() {
            QDBusPendingReply<QRect> reply = *watcher;
            m_availableScreenRect = reply.isValid() ? reply.value() : QRect();
            setWindowGeometry(Settings::width(), Settings::height(), Settings::position());
            watcher->deleteLater();
        });

        _toggleWindowState();
    } else {
        _toggleWindowState();
    }
}

void MainWindow::_toggleWindowState()
{
    bool visible = isVisible();

    if (visible && !isActiveWindow() && Settings::keepOpen()) {
        // Window is open but doesn't have focus; it's set to stay open
        // regardless of focus loss.

        if (Settings::toggleToFocus()) {
            // The open/retract action is set to focus the window when it's
            // open but lacks focus. The following will cause it to receive
            // focus, and in an environment with multiple virtual desktops
            // will also cause the window manager to switch to the virtual
            // desktop the window resides on.

            KWindowSystem::activateWindow(windowHandle());
            if (KWindowSystem::isPlatformX11()) {
                KX11Extras::forceActiveWindow(winId());
            }

            return;
        } else if (!Settings::showOnAllDesktops() && KWindowInfo(winId(), NET::WMDesktop).desktop() != KX11Extras::currentDesktop()) {
            // The open/retract action isn't set to focus the window, but
            // the window is currently on another virtual desktop (the option
            // to show it on all of them is disabled), so closing it doesn't
            // make sense and we're opting to show it instead to avoid
            // requiring the user to invoke the action twice to get to see
            // Yakuake. Just forcing focus would cause the window manager to
            // switch to the virtual desktop the window currently resides on,
            // so move the window to the current desktop before doing so.

            if (KWindowSystem::isPlatformX11()) {
                KX11Extras::setOnDesktop(winId(), KX11Extras::currentDesktop());
            }

            KWindowSystem::activateWindow(windowHandle());
            if (KWindowSystem::isPlatformX11()) {
                KX11Extras::forceActiveWindow(winId());
            }

            return;
        }
    }

#if HAVE_X11
    if (!Settings::useWMAssist() && m_kwinAssistPropSet)
        kwinAssistPropCleanup();

    if (m_isX11 && Settings::useWMAssist() && KX11Extras::compositingActive())
        kwinAssistToggleWindowState(visible);
    else
#endif
        if (!m_isWayland) {
        xshapeToggleWindowState(visible);
    } else {
        if (visible) {
            sharedPreHideWindow();

            hide();

            sharedAfterHideWindow();
        } else {
            sharedPreOpenWindow();
            if (!m_isWayland) {
                slideWindow();
            }

            show();
            if (m_isWayland) {
                slideWindow();
            }

            sharedAfterOpenWindow();
        }
    }
}

void MainWindow::slideWindow()
{
    if (KWindowEffects::isEffectAvailable(KWindowEffects::Slide)) {
        KWindowEffects::slideWindow(windowHandle(), KWindowEffects::TopEdge);
    }
}

#if HAVE_X11
void MainWindow::kwinAssistToggleWindowState(bool visible)
{
    bool gotEffect = false;

    Display *display = QX11Info::display();
    Atom atom = XInternAtom(display, "_KDE_SLIDE", false);
    int count;
    Atom *list = XListProperties(display, DefaultRootWindow(display), &count);

    if (list != nullptr) {
        gotEffect = (std::find(list, list + count, atom) != list + count);

        XFree(list);
    }

    if (gotEffect) {
        Atom atom = XInternAtom(display, "_KDE_SLIDE", false);

        if (Settings::frames() > 0) {
            QVarLengthArray<long, 1024> data(4);

            data[0] = 0;
            data[1] = 1;
            data[2] = Settings::frames() * 10;
            data[3] = Settings::frames() * 10;

            XChangeProperty(display, winId(), atom, atom, 32, PropModeReplace, reinterpret_cast<unsigned char *>(data.data()), data.size());

            m_kwinAssistPropSet = true;
        } else
            XDeleteProperty(display, winId(), atom);

        if (visible) {
            sharedPreHideWindow();

            hide();

            sharedAfterHideWindow();
        } else {
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

    Display *display = QX11Info::display();
    Atom atom = XInternAtom(display, "_KDE_SLIDE", false);

    XDeleteProperty(display, winId(), atom);

    m_kwinAssistPropSet = false;
}
#endif

void MainWindow::xshapeToggleWindowState(bool visible)
{
    if (m_animationTimer.isActive())
        return;

    if (visible) {
        sharedPreHideWindow();

        m_animationFrame = Settings::frames();

        connect(&m_animationTimer, SIGNAL(timeout()), this, SLOT(xshapeRetractWindow()));
        m_animationTimer.start();
    } else {
        m_animationFrame = 0;

        connect(&m_animationTimer, SIGNAL(timeout()), this, SLOT(xshapeOpenWindow()));
        m_animationTimer.start();
    }
}

void MainWindow::xshapeOpenWindow()
{
    if (m_animationFrame == 0) {
        sharedPreOpenWindow();

        show();

        sharedAfterOpenWindow();
    }

    if (m_animationFrame == Settings::frames()) {
        m_animationTimer.stop();
        m_animationTimer.disconnect();

        m_titleBar->move(0, height() - m_titleBar->height());
        updateMask();
    } else {
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
    if (m_animationFrame == 0) {
        m_animationTimer.stop();
        m_animationTimer.disconnect();

        hide();

        sharedAfterHideWindow();
    } else {
        m_titleBar->move(0, m_titleBar->y() - m_animationStepSize);
        setMask(QRegion(mask()).translated(0, -m_animationStepSize));

        --m_animationFrame;
    }
}

void MainWindow::sharedPreOpenWindow()
{
    applyWindowGeometry();

    updateUseTranslucency();

    if (Settings::pollMouse())
        toggleMousePoll(false);
    if (Settings::rememberFullscreen())
        setFullScreen(m_isFullscreen);
}

void MainWindow::sharedAfterOpenWindow()
{
    if (!Settings::firstRun() && KWindowSystem::isPlatformX11()) {
        KX11Extras::forceActiveWindow(winId());
    }

    connect(qGuiApp, &QGuiApplication::focusWindowChanged, this, &MainWindow::wmActiveWindowChanged);

    applyWindowProperties();

#if HAVE_KWAYLAND
    initWaylandSurface();
#endif

    Q_EMIT windowOpened();
}

void MainWindow::sharedPreHideWindow()
{
    disconnect(qGuiApp, &QGuiApplication::focusWindowChanged, this, &MainWindow::wmActiveWindowChanged);
}

void MainWindow::sharedAfterHideWindow()
{
    if (Settings::pollMouse())
        toggleMousePoll(true);

#if HAVE_KWAYLAND
    delete m_plasmaShellSurface;
    m_plasmaShellSurface = nullptr;
#endif

    Q_EMIT windowClosed();
}

void MainWindow::activate()
{
    KWindowSystem::activateWindow(windowHandle());
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
    if (Settings::keepOpen() != keepOpen) {
        Settings::setKeepOpen(keepOpen);
        Settings::self()->save();

        applyWindowProperties();
    }

    actionCollection()->action(QStringLiteral("keep-open"))->setChecked(keepOpen);
    m_titleBar->setFocusButtonState(keepOpen);
}

void MainWindow::setFullScreen(bool state)
{
    if (isVisible())
        m_isFullscreen = state;
    if (state) {
        setWindowState(windowState() | Qt::WindowFullScreen);
        setWindowGeometry(100, 100, Settings::position());
    } else {
        setWindowState(windowState() & ~Qt::WindowFullScreen);
        setWindowGeometry(Settings::width(), Settings::height(), Settings::position());
    }
}

int MainWindow::getScreen()
{
    if (Settings::screen() <= 0 || Settings::screen() > QGuiApplication::screens().length()) {
        auto message = QDBusMessage::createMethodCall(QStringLiteral("org.kde.KWin"),
                                                      QStringLiteral("/KWin"),
                                                      QStringLiteral("org.kde.KWin"),
                                                      QStringLiteral("activeOutputName"));
        QDBusReply<QString> reply = QDBusConnection::sessionBus().call(message);

        if (reply.isValid()) {
            const auto screens = QGuiApplication::screens();
            for (int i = 0; i < screens.size(); ++i) {
                if (screens[i]->name() == reply.value())
                    return i;
            }
        }
        // Right after unplugging an external monitor and the Yakuake window was on
        // that monitor, QGuiApplication::screenAt() can return nullptr so we fallback on
        // the first monitor.
        QScreen *screen = QGuiApplication::screenAt(QCursor::pos());
        return screen ? QGuiApplication::screens().indexOf(screen) : 0;
    } else {
        return Settings::screen() - 1;
    }
}

QRect MainWindow::getScreenGeometry()
{
    QScreen *screen = QGuiApplication::screens().at(getScreen());
    QRect screenGeometry = screen->geometry();
    screenGeometry.moveTo(screenGeometry.topLeft() / screen->devicePixelRatio());
    return screenGeometry;
}

QRect MainWindow::getDesktopGeometry()
{
    QRect screenGeometry = getScreenGeometry();

    QAction *action = actionCollection()->action(QStringLiteral("view-full-screen"));

    if (action->isChecked())
        return screenGeometry;

    if (m_isWayland) {
        // on Wayland it's not possible to get the work area from KWindowSystem
        // but plasmashell provides this through dbus
        return m_availableScreenRect.isValid() ? m_availableScreenRect : screenGeometry;
    }

    if (QGuiApplication::screens().count() > 1) {
        const QList<WId> allWindows = KX11Extras::windows();
        QList<WId> offScreenWindows;

        QListIterator<WId> i(allWindows);

        while (i.hasNext()) {
            WId windowId = i.next();

            if (KX11Extras::hasWId(windowId)) {
                KWindowInfo windowInfo = KWindowInfo(windowId, NET::WMDesktop | NET::WMGeometry, NET::WM2ExtendedStrut);

                // If windowInfo is valid and the window is located at the same (current)
                // desktop with the yakuake window...
                if (windowInfo.valid() && windowInfo.isOnCurrentDesktop()) {
                    NETExtendedStrut strut = windowInfo.extendedStrut();

                    // Get the area covered by each strut.
                    QRect topStrut(strut.top_start, 0, strut.top_end - strut.top_start, strut.top_width);
                    QRect bottomStrut(strut.bottom_start,
                                      screenGeometry.bottom() - strut.bottom_width,
                                      strut.bottom_end - strut.bottom_start,
                                      strut.bottom_width);
                    QRect leftStrut(0, strut.left_start, strut.left_width, strut.left_end - strut.left_start);
                    QRect rightStrut(screenGeometry.right() - strut.right_width, strut.right_start, strut.right_width, strut.right_end - strut.right_start);

                    // If the window has no strut, no need to bother further.
                    if (topStrut.isEmpty() && bottomStrut.isEmpty() && leftStrut.isEmpty() && rightStrut.isEmpty())
                        continue;

                    // If any of the strut and the window itself intersects with our screen geometry,
                    // it will be correctly handled by workArea(). If the window doesn't intersect
                    // with our screen geometry it's most likely a plasma panel and can/should be
                    // ignored
                    if ((topStrut.intersects(screenGeometry) || bottomStrut.intersects(screenGeometry) || leftStrut.intersects(screenGeometry)
                         || rightStrut.intersects(screenGeometry))
                        && windowInfo.geometry().intersects(screenGeometry)) {
                        continue;
                    }

                    // This window has a strut on the same desktop as us but which does not cover our screen
                    // geometry. It should be ignored, otherwise the returned work area will wrongly include
                    // the strut.
                    offScreenWindows << windowId;
                }
            }
        }

        return KX11Extras::workArea(offScreenWindows).intersected(screenGeometry);
    }

#if HAVE_X11
    return KX11Extras::workArea();
#else
    return QRect();
#endif
}

void MainWindow::whatsThis()
{
    QWhatsThis::enterWhatsThisMode();
}

void MainWindow::showFirstRunDialog()
{
    if (!m_firstRunDialog) {
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

    if (KWindowSystem::isPlatformX11()) {
        KX11Extras::forceActiveWindow(winId());
    }
}

void MainWindow::firstRunDialogOk()
{
    QAction *action = static_cast<QAction *>(actionCollection()->action(QStringLiteral("toggle-window-state")));

    KGlobalAccel::self()->setShortcut(action, QList<QKeySequence>() << m_firstRunDialog->keySequence(), KGlobalAccel::NoAutoloading);

    actionCollection()->writeSettings();
}

void MainWindow::updateUseTranslucency()
{
    m_useTranslucency = (Settings::translucency() && (m_isX11 ? KX11Extras::compositingActive() : true));
}

void MainWindow::updateTrayTooltip()
{
    if (!m_notifierItem) {
        return;
    }

    auto *action = actionCollection()->action(QStringLiteral("toggle-window-state"));
    const QList<QKeySequence> &shortcuts = KGlobalAccel::self()->shortcut(action);
    if (!shortcuts.isEmpty()) {
        const QString shortcut(shortcuts.first().toString(QKeySequence::NativeText));
        m_notifierItem->setToolTip(QStringLiteral("yakuake"), QStringLiteral("Yakuake"), xi18nc("@info", "Press <shortcut>%1</shortcut> to open", shortcut));
    }
}

#include "moc_mainwindow.cpp"
