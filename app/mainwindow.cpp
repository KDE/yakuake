/*
  Copyright (C) 2008 by Eike Hein <hein@kde.org>

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
#include "config/appearancesettings.h"
#include "config/windowsettings.h"
#include "firstrundialog.h"
#include "sessionstack.h"
#include "skin.h"
#include "tabbar.h"
#include "titlebar.h"
#include "ui_behaviorsettings.h"

#include <KAboutApplicationDialog>
#include <KApplication>
#include <KConfigDialog>
#include <KHelpMenu>
#include <KMenu>
#include <KMessageBox>
#include <KPassivePopup>
#include <KShortcutsDialog>
#include <KStandardAction>
#include <KToggleFullScreenAction>
#include <KVBox>

#include <QDesktopWidget>
#include <QPainter>
#include <QPalette>
#include <QtDBus/QtDBus>
#include <QTimer>

#ifdef Q_WS_X11
#include <QX11Info>

#include <X11/Xlib.h>
#include <X11/extensions/Xrender.h>
#endif


MainWindow::MainWindow(QWidget* parent) 
    : KMainWindow(parent, Qt::CustomizeWindowHint | Qt::FramelessWindowHint)
{
    QDBusConnection::sessionBus().registerObject("/yakuake/window", this, QDBusConnection::ExportScriptableSlots);

    m_skin = new Skin();
    m_menu = new KMenu(this);
    m_helpMenu = new KHelpMenu(this, KGlobal::mainComponent().aboutData());
    m_sessionStack = new SessionStack(this);
    m_tabBar = new TabBar(this);
    m_titleBar = new TitleBar(this);
    m_firstRunDialog = NULL;

    setupActions();
    setupMenu();

    connect(m_tabBar, SIGNAL(newTabRequested()), m_sessionStack, SLOT(addSession()));
    connect(m_tabBar, SIGNAL(tabSelected(int)), m_sessionStack, SLOT(raiseSession(int)));
    connect(m_tabBar, SIGNAL(tabClosed(int)), m_sessionStack, SLOT(removeSession(int)));

    connect(m_sessionStack, SIGNAL(sessionAdded(int)), m_tabBar, SLOT(addTab(int)));
    connect(m_sessionStack, SIGNAL(sessionRaised(int)), m_tabBar, SLOT(selectTab(int)));
    connect(m_sessionStack, SIGNAL(sessionRemoved(int)), m_tabBar, SLOT(removeTab(int)));
    connect(m_sessionStack, SIGNAL(activeTitleChanged(const QString&)),  m_titleBar, SLOT(setTitle(const QString&)));

    connect(&m_mousePoller, SIGNAL(timeout()), this, SLOT(pollMouse()));

    connect(KWindowSystem::self(), SIGNAL(workAreaChanged()), this, SLOT(applyWindowGeometry()));

    applySettings();

    m_sessionStack->addSession();

    if (Settings::firstRun())
    {
        QMetaObject::invokeMethod(this, "toggleWindowState", Qt::QueuedConnection);
        QMetaObject::invokeMethod(this, "showFirstRunDialog", Qt::QueuedConnection);
    }
    else
    {
        if (Settings::showPopup()) showStartupPopup();
        if (Settings::pollMouse()) toggleMousePoll(true);
    }
}

MainWindow::~MainWindow()
{
    Settings::self()->writeConfig();

    delete m_skin;
}

bool MainWindow::queryClose()
{
    if (Settings::confirmQuit() && m_sessionStack->count() > 1)
    {
        int result = KMessageBox::warningContinueCancel(this,
            i18nc("@info", "<warning>You have multiple open sessions. These will be killed if you continue.</warning><nl/><nl/>"
                           "Are you sure you want to quit?"),
            i18nc("@title:window", "Really Quit?"), KStandardGuiItem::quit(), KStandardGuiItem::cancel());

        if (result == KMessageBox::Continue)
            return true;
        else
            return false;
    }

    return true;
}

void MainWindow::setupActions()
{
    m_actionCollection = new KActionCollection(this);

    KAction* action = KStandardAction::quit(kapp, SLOT(quit()),actionCollection());

    action = KStandardAction::aboutApp(m_helpMenu, SLOT(aboutApplication()), actionCollection());
    action = KStandardAction::aboutKDE(m_helpMenu, SLOT(aboutKDE()), actionCollection());
 
    action = KStandardAction::keyBindings(this, SLOT(configureKeys()), actionCollection());
    action = KStandardAction::preferences(this, SLOT(configureApp()), actionCollection());

    action = KStandardAction::fullScreen(this, SLOT(updateFullScreen()), this, actionCollection());
    action->setShortcut(QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_F11));
    connect(action, SIGNAL(toggled(bool)), this, SLOT(setFullScreen(bool)));

    action = KStandardAction::whatsThis(this, SLOT(whatsThis()), actionCollection());

    action = new KAction(KIcon("yakuake"), i18nc("@action", "Open/Retract Yakuake"), this);
    action->setGlobalShortcut(KShortcut(Qt::Key_F12));
    actionCollection()->addAction("toggle-window-state", action);
    connect(action, SIGNAL(triggered()), this, SLOT(toggleWindowState()));

    action = new KAction(i18nc("@action", "Keep window open when it loses focus"), this);
    action->setCheckable(true);
    actionCollection()->addAction("keep-open", action);
    connect(action, SIGNAL(toggled(bool)), this, SLOT(setKeepOpen(bool)));

    action = new KAction(KIcon("configure"), i18nc("@action", "Manage Profiles..."), this);
    actionCollection()->addAction("manage-profiles", action);
    connect(action, SIGNAL(triggered()), m_sessionStack, SIGNAL(manageProfiles()));

    action = new KAction(KIcon("document-properties"), i18nc("@action", "Edit Current Profile..."), this);
    actionCollection()->addAction("edit-profile", action);
    connect(action, SIGNAL(triggered()), this, SLOT(handleSpecialAction()));

    action = new KAction(i18nc("@action", "Increase Window Width"), this);
    action->setShortcut(QKeySequence(Qt::ALT + Qt::SHIFT + Qt::Key_Right));
    actionCollection()->addAction("increase-window-width", action);
    connect(action, SIGNAL(triggered()), this, SLOT(increaseWindowWidth()));

    action = new KAction(i18nc("@action", "Decrease Window Width"), this);
    action->setShortcut(QKeySequence(Qt::ALT + Qt::SHIFT + Qt::Key_Left));
    actionCollection()->addAction("decrease-window-width", action);
    connect(action, SIGNAL(triggered()), this, SLOT(decreaseWindowWidth()));

    action = new KAction(i18nc("@action", "Increase Window Height"), this);
    action->setShortcut(QKeySequence(Qt::ALT + Qt::SHIFT + Qt::Key_Down));
    actionCollection()->addAction("increase-window-height", action);
    connect(action, SIGNAL(triggered()), this, SLOT(increaseWindowHeight()));

    action = new KAction(i18nc("@action", "Decrease Window Height"), this);
    action->setShortcut(QKeySequence(Qt::ALT + Qt::SHIFT + Qt::Key_Up));
    actionCollection()->addAction("decrease-window-height", action);
    connect(action, SIGNAL(triggered()), this, SLOT(decreaseWindowHeight()));

    action = new KAction(KIcon("tab-new"), i18nc("@action", "New Session"), this);
    action->setShortcut(QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_N));
    actionCollection()->addAction("new-session", action);
    connect(action, SIGNAL(triggered()), m_sessionStack, SLOT(addSession()));

    action = new KAction(KIcon("tab-new"), i18nc("@action", "Two Terminals, Horizontally"), this);
    actionCollection()->addAction("new-session-two-horizontal", action);
    connect(action, SIGNAL(triggered()), m_sessionStack, SLOT(addSessionTwoHorizontal()));

    action = new KAction(KIcon("tab-new"), i18nc("@action", "Two Terminals, Vertically"), this);
    actionCollection()->addAction("new-session-two-vertical", action);
    connect(action, SIGNAL(triggered()), m_sessionStack, SLOT(addSessionTwoVertical()));

    action = new KAction(KIcon("tab-new"), i18nc("@action", "Four Terminals, Grid"), this);
    actionCollection()->addAction("new-session-quad", action);
    connect(action, SIGNAL(triggered()), m_sessionStack, SLOT(addSessionQuad()));

    action = new KAction(KIcon("tab-close"), i18nc("@action", "Close Session"), this);
    action->setShortcut(QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_S));
    actionCollection()->addAction("close-session", action);
    connect(action, SIGNAL(triggered()), this, SLOT(handleSpecialAction()));

    action = new KAction(KIcon("go-previous"), i18nc("@action", "Previous Session"), this);
    action->setShortcut(QKeySequence(Qt::SHIFT + Qt::Key_Left));
    actionCollection()->addAction("previous-session", action);
    connect(action, SIGNAL(triggered()), m_tabBar, SLOT(selectPreviousTab()));

    action = new KAction(KIcon("go-next"), i18nc("@action", "Next Session"), this);
    action->setShortcut(QKeySequence(Qt::SHIFT + Qt::Key_Right));
    actionCollection()->addAction("next-session", action);
    connect(action, SIGNAL(triggered()), m_tabBar, SLOT(selectNextTab()));

    action = new KAction(KIcon("arrow-left"), i18nc("@action", "Move Session Left"), this);
    action->setShortcut(QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_Left));
    actionCollection()->addAction("move-session-left", action);
    connect(action, SIGNAL(triggered()), this, SLOT(handleSpecialAction()));

    action = new KAction(KIcon("arrow-right"), i18nc("@action", "Move Session Right"), this);
    action->setShortcut(QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_Right));
    actionCollection()->addAction("move-session-right", action);
    connect(action, SIGNAL(triggered()), this, SLOT(handleSpecialAction()));

    action = new KAction(KIcon("edit-rename"), i18nc("@action", "Rename Session..."), this);
    actionCollection()->addAction("rename-session", action);
    connect(action, SIGNAL(triggered()), this, SLOT(handleSpecialAction()));

    action = new KAction(KIcon("go-previous"), i18nc("@action", "Previous Terminal"), this);
    action->setShortcut(QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_Up));
    actionCollection()->addAction("previous-terminal", action);
    connect(action, SIGNAL(triggered()), m_sessionStack, SIGNAL(previousTerminal()));

    action = new KAction(KIcon("go-next"), i18nc("@action", "Next Terminal"), this);
    action->setShortcut(QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_Down));
    actionCollection()->addAction("next-terminal", action);
    connect(action, SIGNAL(triggered()), m_sessionStack, SIGNAL(nextTerminal()));

    action = new KAction(KIcon("view-close"), i18nc("@action", "Close Active Terminal"), this);
    action->setShortcut(QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_R));
    actionCollection()->addAction("close-active-terminal", action);
    connect(action, SIGNAL(triggered()), m_sessionStack, SIGNAL(closeTerminal()));

    action = new KAction(KIcon("view-split-left-right"), i18nc("@action", "Split Left/Right"), this);
    action->setShortcut(QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_L));
    actionCollection()->addAction("split-left-right", action);
    connect(action, SIGNAL(triggered()), m_sessionStack, SIGNAL(splitLeftRight()));

    action = new KAction(KIcon("view-split-top-bottom"), i18nc("@action", "Split Top/Bottom"), this);
    action->setShortcut(QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_T));
    actionCollection()->addAction("split-top-bottom", action);
    connect(action, SIGNAL(triggered()), m_sessionStack, SIGNAL(splitTopBottom()));

    for (uint i = 1; i <= 10; ++i)
    {
        action = new KAction(this);
        action->setText(i18nc("@action", "Switch to Session <numid>%1</numid>", i));
        action->setData(i);
        actionCollection()->addAction(QString("switch-to-session-%1").arg(i), action);
        connect(action, SIGNAL(triggered()), this, SLOT(handleSpecialAction()));
    }

    m_actionCollection->associateWidget(this);
    m_actionCollection->readSettings();
}

void MainWindow::handleSpecialAction()
{
    QAction* action = static_cast<QAction*>(QObject::sender());

    if (action == actionCollection()->action("move-session-left"))
        m_tabBar->moveTabLeft(m_tabBar->retrieveContextMenuSessionId());

    if (action == actionCollection()->action("move-session-right"))
        m_tabBar->moveTabRight(m_tabBar->retrieveContextMenuSessionId());

    if (action == actionCollection()->action("rename-session"))
        m_tabBar->interactiveRename(m_tabBar->retrieveContextMenuSessionId());

    if (action == actionCollection()->action("close-session"))
        m_sessionStack->removeSession(m_tabBar->retrieveContextMenuSessionId());

    if (action == actionCollection()->action("edit-profile"))
        m_sessionStack->editProfile(m_tabBar->retrieveContextMenuSessionId());

    if (!action->data().isNull())
        m_sessionStack->raiseSession(m_tabBar->sessionAtTab(action->data().toInt()-1));
}

void MainWindow::setupMenu()
{
    m_menu->addTitle(i18nc("@title:menu", "Help"));
    m_menu->addAction(actionCollection()->action(KStandardAction::stdName(KStandardAction::WhatsThis)));
    m_menu->addAction(actionCollection()->action(KStandardAction::stdName(KStandardAction::AboutApp)));
    m_menu->addAction(actionCollection()->action(KStandardAction::stdName(KStandardAction::AboutKDE)));

    m_menu->addTitle(i18nc("@title:menu", "Quick Options"));
    m_menu->addAction(actionCollection()->action(KStandardAction::stdName(KStandardAction::FullScreen)));
    m_menu->addAction(actionCollection()->action("keep-open"));

    m_screenMenu = new KMenu(this);
    connect(m_screenMenu, SIGNAL(triggered(QAction*)), this, SLOT(setScreen(QAction*)));
    m_screenMenu->setTitle(i18nc("@title:menu", "Screen"));
    m_menu->addMenu(m_screenMenu);

    m_windowWidthMenu = new KMenu(this);
    connect(m_windowWidthMenu, SIGNAL(triggered(QAction*)), this, SLOT(setWindowWidth(QAction*)));
    m_windowWidthMenu->setTitle(i18nc("@title:menu", "Width"));
    m_menu->addMenu(m_windowWidthMenu);

    m_windowHeightMenu = new KMenu(this);
    connect(m_windowHeightMenu, SIGNAL(triggered(QAction*)), this, SLOT(setWindowHeight(QAction*)));
    m_windowHeightMenu->setTitle(i18nc("@title:menu", "Height"));
    m_menu->addMenu(m_windowHeightMenu);

    m_menu->addTitle(i18nc("@title:menu", "Settings"));
    m_menu->addAction(actionCollection()->action("manage-profiles"));
    m_menu->addAction(actionCollection()->action(KStandardAction::stdName(KStandardAction::KeyBindings)));
    m_menu->addAction(actionCollection()->action(KStandardAction::stdName(KStandardAction::Preferences)));
}

void MainWindow::updateScreenMenu()
{
    QAction* action;

    m_screenMenu->clear();

    action = m_screenMenu->addAction(i18nc("@item:inmenu", "At mouse location"));
    action->setCheckable(true);
    action->setData(0);
    action->setChecked(Settings::screen() == 0);

    for (int i = 1; i <= KApplication::desktop()->numScreens(); i++)
    {
        action = m_screenMenu->addAction(i18nc("@item:inmenu", "Screen <numid>%1</numid>", i));
        action->setCheckable(true);
        action->setData(i);
        action->setChecked(i == Settings::screen());
    }

    action = m_screenMenu->menuAction();
    action->setVisible(KApplication::desktop()->numScreens() > 1);
}

void MainWindow::updateWindowSizeMenus()
{
    updateWindowWidthMenu();
    updateWindowHeightMenu();
}

void MainWindow::updateWindowWidthMenu()
{
    QAction* action;

    m_windowWidthMenu->clear(); 

    for (int i = 10; i <= 100; i += 10)
    {
        action = m_windowWidthMenu->addAction(QString::number(i) + '%');
        action->setCheckable(true);
        action->setData(i);
        action->setChecked(i == Settings::width());
    }
}

void MainWindow::updateWindowHeightMenu()
{
    QAction* action;

    m_windowHeightMenu->clear(); 

    for (int i = 10; i <= 100; i += 10)
    {
        action = m_windowHeightMenu->addAction(QString::number(i) + '%');
        action->setCheckable(true);
        action->setData(i);
        action->setChecked(i == Settings::height());
    }
}

void MainWindow::configureKeys()
{
    KShortcutsDialog::configure(actionCollection());
    KWindowSystem::activateWindow(winId());
}

void MainWindow::configureApp()
{
    if (KConfigDialog::showDialog("settings")) return;

    KConfigDialog* settingsDialog = new KConfigDialog(this, "settings", Settings::self());
    settingsDialog->setFaceType(KPageDialog::List);
    connect(settingsDialog, SIGNAL(settingsChanged(const QString&)), this, SLOT(applySettings()));
    connect(settingsDialog, SIGNAL(hidden()), this, SLOT(activate()));

    WindowSettings* windowSettings = new WindowSettings(settingsDialog);
    settingsDialog->addPage(windowSettings, i18nc("@title Preferences page name", "Window"), "yakuake");
    connect(windowSettings, SIGNAL(updateWindowGeometry(int, int, int)), 
        this, SLOT(setWindowGeometry(int, int, int)));

    QWidget* behaviorSettings = new QWidget(settingsDialog);
    Ui::BehaviorSettings behaviorSettingsUi;
    behaviorSettingsUi.setupUi(behaviorSettings);
    settingsDialog->addPage(behaviorSettings, i18nc("@title Preferences page name", "Behavior"),
        "preferences-other");

    AppearanceSettings* appearanceSettings = new AppearanceSettings(settingsDialog);
    settingsDialog->addPage(appearanceSettings, i18nc("@title Preferences page name", "Appearance"), 
        "preferences-desktop-theme");
    connect(appearanceSettings, SIGNAL(settingsChanged()), this, SLOT(applySettings()));
    connect(settingsDialog, SIGNAL(closeClicked()), appearanceSettings, SLOT(resetSelection()));
    connect(settingsDialog, SIGNAL(cancelClicked()), appearanceSettings, SLOT(resetSelection()));

    settingsDialog->show();
}

void MainWindow::applySettings()
{
    if (Settings::dynamicTabTitles())
    {
        connect(m_sessionStack, SIGNAL(titleChanged(int, const QString&)), 
            m_tabBar, SLOT(setTabTitle(int, const QString&)));
    }
    else
    {
        disconnect(m_sessionStack, SIGNAL(titleChanged(int, const QString&)), 
            m_tabBar, SLOT(setTabTitle(int, const QString&)));
    }

    m_animationTimer.setInterval(10);

    m_tabBar->setShown(Settings::showTabBar());

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
    bool gotSkin = m_skin->load(Settings::skin());

    if (!gotSkin)
    {
        Settings::setSkin("default");
        gotSkin = m_skin->load(Settings::skin());
    }

    if (!gotSkin)
    {
        KMessageBox::error(parentWidget(), 
            i18nc("@info", "Yakuake was unable to load a skin. It was likely installed incorrectly.<nl/><nl/>"
                           "The application will now quit."),
            i18nc("@title:window", "Cannot Load Skin"));

        QMetaObject::invokeMethod(kapp, "quit", Qt::QueuedConnection);
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

    KWindowSystem::setOnAllDesktops(winId(), true);
}

void MainWindow::applyWindowGeometry()
{
    QAction* action =  actionCollection()->action(KStandardAction::stdName(KStandardAction::FullScreen));
    if (action->isChecked()) action->activate(KAction::Trigger);

    setWindowGeometry(Settings::width(), Settings::height(), Settings::position());
}

void MainWindow::setWindowGeometry(int newWidth, int newHeight, int newPosition)
{
    QRect workArea = getDesktopGeometry();

    int maxHeight = workArea.height() * newHeight / 100;

    setGeometry(workArea.x() + workArea.width() * newPosition * (100 - newWidth) / 10000,
                workArea.y(), workArea.width() * newWidth / 100, maxHeight);

    maxHeight -= m_titleBar->height();
    m_titleBar->setGeometry(0, maxHeight, width(), m_titleBar->height());

    if (Settings::frames() > 0)
        m_animationStepSize = maxHeight / Settings::frames();
    else
        m_animationStepSize = maxHeight;

    if (Settings::showTabBar())
    {
        maxHeight -= m_tabBar->height();
        m_tabBar->setGeometry(m_skin->borderWidth(), maxHeight, 
            width() - 2 * m_skin->borderWidth(), m_tabBar->height());
    }

    m_sessionStack->setGeometry(m_skin->borderWidth(), 0, 
        width() - 2 * m_skin->borderWidth(), maxHeight);

    updateMask();
}

void MainWindow::setScreen(QAction* action)
{
    Settings::setScreen(action->data().toInt());

    applyWindowGeometry();
    
    updateScreenMenu();
}

void MainWindow::setWindowWidth(int width)
{
    Settings::setWidth(width);

    applyWindowGeometry();

    updateWindowWidthMenu();
}

void MainWindow::setWindowHeight(int height)
{
    Settings::setHeight(height);

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
    if (Settings::width() > 10) setWindowWidth(Settings::width() - 10);
}

void MainWindow::increaseWindowHeight()
{
    if (Settings::height() <= 90) setWindowHeight(Settings::height() + 10);
}

void MainWindow::decreaseWindowHeight()
{
    if (Settings::height() > 10) setWindowHeight(Settings::height() - 10);
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

    QRect leftBorder(0, 0, m_skin->borderWidth(), height() - m_titleBar->height());
    painter.fillRect(leftBorder, m_skin->borderColor());

    QRect rightBorder(width() - m_skin->borderWidth(), 0, m_skin->borderWidth(), 
        height() - m_titleBar->height());
    painter.fillRect(rightBorder, m_skin->borderColor());

    KMainWindow::paintEvent(event);
}

void MainWindow::moveEvent(QMoveEvent* event)
{
    if (Settings::screen() && KApplication::desktop()->screenNumber(this) != getScreen())
    {
        Settings::setScreen(KApplication::desktop()->screenNumber(this)+1);

        updateScreenMenu();

        applyWindowGeometry();
    }

    KMainWindow::moveEvent(event);
}

void MainWindow::changeEvent(QEvent* event)
{
    if (event->type() == QEvent::ActivationChange)
    {
        if (isVisible() && !KApplication::activeWindow() && !Settings::keepOpen())
            toggleWindowState();
    }

    KMainWindow::changeEvent(event);
}

void MainWindow::toggleWindowState()
{
    if (m_animationTimer.isActive()) return;

    if (isVisible())
    {
        if (!isActiveWindow() && Settings::keepOpen() && Settings::toggleToFocus())
        {
            KWindowSystem::forceActiveWindow(winId());
            return;
        }

        m_animationFrame = Settings::frames();

        connect(&m_animationTimer, SIGNAL(timeout()), this, SLOT(retractWindow()));
        m_animationTimer.start();
    }
    else
    {
        applyWindowGeometry();

        m_animationFrame = 0;

        connect(&m_animationTimer, SIGNAL(timeout()), this, SLOT(openWindow()));
        m_animationTimer.start();
    }
}

void MainWindow::openWindow()
{
    if (m_animationFrame == 0)
    {
        updateUseTranslucency();
    
        applyWindowProperties();
    
        show();

        if (Settings::pollMouse()) toggleMousePoll(false);
    }

    if (m_animationFrame == Settings::frames())
    {
        m_animationTimer.stop();
        m_animationTimer.disconnect();

        m_titleBar->move(0, height() - m_titleBar->height());
        updateMask();

        if (!Settings::firstRun()) KWindowSystem::forceActiveWindow(winId());

        return;
    }
    else 
    {
        int maskHeight = m_animationStepSize * m_animationFrame;

        QRegion newMask = m_titleBar->mask();
        newMask.translate(0, maskHeight);
        newMask += QRegion(0, 0, width(), maskHeight);

        setMask(newMask);

        m_titleBar->move(0, maskHeight);
    }

    m_animationFrame++;
}

void MainWindow::retractWindow()
{
    if (m_animationFrame == 0)
    {
        m_animationTimer.stop();
        m_animationTimer.disconnect();

        hide();

        if (Settings::pollMouse()) toggleMousePoll(true);

        return;
    }
    else
    {
        setMask(QRegion(mask()).translated(0, -m_animationStepSize));

        m_titleBar->move(0, m_titleBar->y() - m_animationStepSize);
    }

    --m_animationFrame;
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

    if (pos.y() == 0)
    {
        if (Settings::screen() == 0) 
            toggleWindowState();
        else if (pos.x() >= getDesktopGeometry().x() && pos.x() <= (getDesktopGeometry().x() + getDesktopGeometry().width()))
            toggleWindowState();
    }
}

void MainWindow::setKeepOpen(bool keepOpen)
{
    Settings::setKeepOpen(keepOpen);

    actionCollection()->action("keep-open")->setChecked(keepOpen);
    m_titleBar->setFocusButtonState(keepOpen);
}

void MainWindow::setFullScreen(bool state)
{
     if (state)
        setWindowGeometry(100, 100, Settings::position());
     else
        applyWindowGeometry();
}

void MainWindow::updateFullScreen()
{
    QAction* action =  actionCollection()->action(KStandardAction::stdName(KStandardAction::FullScreen));

    if (action->isChecked())
    {
        setWindowGeometry(100, 100, Settings::position());
        this->setWindowState(windowState() | Qt::WindowFullScreen);
    }
    else
        this->setWindowState(windowState() & ~Qt::WindowFullScreen);
}

int MainWindow::getScreen()
{
    if (!Settings::screen())
        return KApplication::desktop()->screenNumber(QCursor::pos());
    else
        return Settings::screen() - 1;
}

QRect MainWindow::getDesktopGeometry()
{
    QAction* action = actionCollection()->action(KStandardAction::stdName(KStandardAction::FullScreen));

    if (action->isChecked()) 
        return KApplication::desktop()->screenGeometry(getScreen());

    if (KApplication::desktop()->numScreens() > 1) 
        return KWindowSystem::workArea().intersect(KApplication::desktop()->screenGeometry(getScreen()));

    return KWindowSystem::workArea();
}

void MainWindow::whatsThis()
{
    QWhatsThis::enterWhatsThisMode();
}

void MainWindow::showStartupPopup()
{
    KAction* action = static_cast<KAction*>(actionCollection()->action("toggle-window-state"));
    QString shortcut = action->globalShortcut().toString();

    KPassivePopup* popup = new KPassivePopup();

    popup->setAutoDelete(true);
    popup->setTimeout(5000);
    popup->setView(popup->standardView(i18nc("@title:window", 
        "<application>Yakuake</application> Notification"), 
        i18nc("@info", "Application successfully started!<nl/>"
                       "Press <shortcut>%1</shortcut> to use it...", shortcut),
        KIconLoader::global()->loadIcon("yakuake", KIconLoader::Small)));

    popup->show(getDesktopGeometry().topLeft());
}

void MainWindow::showFirstRunDialog()
{
    if (!m_firstRunDialog)
    {
        m_firstRunDialog = new FirstRunDialog(this);
        connect(m_firstRunDialog, SIGNAL(finished()), this, SLOT(firstRunDialogFinished()));
        connect(m_firstRunDialog, SIGNAL(okClicked()), this, SLOT(firstRunDialogOk()));
    }

    m_firstRunDialog->show();
}

void MainWindow::firstRunDialogFinished()
{
    Settings::setFirstRun(false);
    Settings::self()->writeConfig();

    m_firstRunDialog->deleteLater();

    KWindowSystem::forceActiveWindow(winId());
}

void MainWindow::firstRunDialogOk()
{
    KAction* action = static_cast<KAction*>(actionCollection()->action("toggle-window-state"));

    action->setGlobalShortcut(KShortcut(m_firstRunDialog->keySequence()), 
        KAction::ActiveShortcut, KAction::NoAutoloading);

    actionCollection()->writeSettings();
}

void MainWindow::updateUseTranslucency()
{
#ifdef Q_WS_X11
    bool ARGB = false;

    int screen = QX11Info::appScreen();
    bool depth = (QX11Info::appDepth() == 32);

    Display* display = QX11Info::display();
    Visual* visual = static_cast<Visual*>(QX11Info::appVisual(screen));

    XRenderPictFormat* format = XRenderFindVisualFormat(display, visual);

    if (depth && format->type == PictTypeDirect && format->direct.alphaMask)
    {
        ARGB = true;
    }

    if (ARGB && Settings::translucency())
    {
        m_useTranslucency = KWindowSystem::compositingActive();
    }
    else
#endif
    {
        m_useTranslucency = false;
    }
}
