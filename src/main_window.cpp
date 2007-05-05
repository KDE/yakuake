/*
    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.
*/

/*
  Copyright (C) 2005 Francois Chazal <neptune3k@free.fr>
  Copyright (C) 2006-2007 Eike Hein <hein@kde.org>
*/


#include "main_window.h"
#include "main_window.moc"
#include "settings.h"
#include "general_settings.h"
#include "skin_settings.h"
#include "first_run_dialog.h"
#include "session.h"

#include <qsignalmapper.h>
#include <qwhatsthis.h>

#include <kaboutapplication.h>
#include <kaboutkde.h>
#include <kmessagebox.h>
#include <kconfigdialog.h>
#include <kiconloader.h>
#include <kwin.h>


MainWindow::MainWindow(QWidget * parent, const char * name) :
        DCOPObject("DCOPInterface"),
        KMainWindow(parent, name, Qt::WStyle_Customize | Qt::WStyle_NoBorder),
        step(0)
{
    first_run_dialog = 0;
    about_app = 0;
    about_kde = 0;
    full_screen = false;
    is_shutting_down = false;
    background_changed = false;;

    KConfig config(CONFIG_FILE);

    initWindowProps();

    back_widget = new QWidget(this);
    widgets_stack = new QWidgetStack(this);

    // Register with DCOP.
    if (!kapp->dcopClient()->isRegistered())
    {
        kapp->dcopClient()->registerAs("dcopinterface");
        kapp->dcopClient()->setDefaultObject(objId());
    }

    // Revert to default skin if selected skin can't be located.
    if (!locate("appdata", Settings::skin() + "/title.skin"))
        Settings::setSkin("default");

    // Initialize access key.
    global_key = new KGlobalAccel(this);
    global_key->insert("AccessKey", i18n("Open/Close Yakuake"),
                       i18n("Slides in and out the Yakuake window"),
                       Key_F12, 0, this, SLOT(slotToggleState()));

    global_key->readSettings(&config);
    global_key->updateConnections();

    // Initialize shortcuts.
    KAction* action;

    KShortcut shortcut(Qt::CTRL+Qt::ALT+Qt::Key_N);
    shortcut.append(KShortcut(Qt::CTRL+Qt::SHIFT+Qt::Key_N));
    action = new KAction(i18n("New Session"), SmallIcon("tab_new"), shortcut,
                             this, SLOT(slotAddSession()),
                             actionCollection(), "add_tab");

    action = new KAction(i18n("Two Terminals, Horizontal"), SmallIcon("tab_new"),
                             0, this, SLOT(slotAddSessionTwoHorizontal()),
                             actionCollection(), "add_tab_twohorizontal");

    action = new KAction(i18n("Two Terminals, Vertical"), SmallIcon("tab_new"),
                             0, this, SLOT(slotAddSessionTwoVertical()),
                             actionCollection(), "add_tab_twovertical");

    action = new KAction(i18n("Four Terminals, Quad"), SmallIcon("tab_new"),
                             0, this, SLOT(slotAddSessionQuad()),
                             actionCollection(), "add_tab_quad");

    action = new KAction(i18n("Go to Next Terminal"), SmallIcon("next"),
                             "Ctrl+Shift+Up", this, SLOT(slotFocusNextSplit()),
                             actionCollection(), "focus_next_terminal");

    action = new KAction(i18n("Go to Previous Terminal"), SmallIcon("previous"),
                             "Ctrl+Shift+Down", this, SLOT(slotFocusPreviousSplit()),
                             actionCollection(), "focus_previous_terminal");

    action = new KAction(i18n("Paste"), SmallIcon("editpaste"), SHIFT+Key_Insert,
                               this, SLOT(slotPasteClipboard()),
                               actionCollection(), "paste_clipboard");

    action = new KAction(i18n("Paste Selection"), SmallIcon("editpaste"),
                               CTRL+SHIFT+Key_Insert, this, SLOT(slotPasteSelection()),
                               actionCollection(), "paste_selection");

    action = new KAction(i18n("Rename Session..."), SmallIcon("edit"),
                               "Alt+Ctrl+S", this, SLOT(slotInteractiveRename()),
                               actionCollection(), "edit_name");

    action = new KAction(i18n("Increase Width"), SmallIcon("viewmag+"),
                               "Alt+Shift+Right", this, SLOT(slotIncreaseSizeW()),
                               actionCollection(), "increasew");
    action = new KAction(i18n("Decrease Width"), SmallIcon("viewmag-"),
                               "Alt+Shift+Left", this, SLOT(slotDecreaseSizeW()),
                               actionCollection(), "decreasew");
    action = new KAction(i18n("Increase Height"), SmallIcon("viewmag+"),
                               "Alt+Shift+Down", this, SLOT(slotIncreaseSizeH()),
                               actionCollection(), "increaseh");
    action = new KAction(i18n("Decrease Height"), SmallIcon("viewmag-"),
                               "Alt+Shift+Up", this, SLOT(slotDecreaseSizeH()),
                               actionCollection(), "decreaseh");

    action = new KAction(i18n("Configure Global Shortcuts..."),
                               SmallIcon("configure_shortcuts"), 0,
                               this, SLOT(slotSetAccessKey()),
                               actionCollection(), "global_shortcuts");

    action = new KAction(i18n("Quit"), SmallIcon("exit"), 0, this,
                               SLOT(close()), actionCollection(), "quit");

    KStdAction::keyBindings(this, SLOT(slotSetControlKeys()), actionCollection());
    KStdAction::preferences(this, SLOT(slotOpenSettingsDialog()), actionCollection());
    KStdAction::aboutApp(this, SLOT(slotOpenAboutApp()), actionCollection());
    KStdAction::aboutKDE(this, SLOT(slotOpenAboutKDE()), actionCollection());
    KStdAction::whatsThis(this, SLOT(whatsThis()), actionCollection());

    full_screen_action = KStdAction::fullScreen(this, SLOT(slotUpdateFullScreen()), actionCollection(), this);
    connect(full_screen_action, SIGNAL(toggled(bool)), this, SLOT(slotSetFullScreen(bool)));

    createMenu();
    createSessionMenu();
    createTitleBar();
    createTabsBar();

    action = new KAction(i18n("Go to Next Session"), SmallIcon("next"),
                              "Shift+Right", tab_bar, SLOT(slotSelectNextItem()),
                              actionCollection(), "next_tab");
    action = new KAction(i18n("Go to Previous Session"), SmallIcon("previous"),
                              "Shift+Left", tab_bar, SLOT(slotSelectPreviousItem()),
                              actionCollection(), "previous_tab");

    action = new KAction(i18n("Move Session Left"), SmallIcon("back"),
                              "Ctrl+Shift+Left", tab_bar, SLOT(slotMoveItemLeft()),
                              actionCollection(), "move_tab_left");

    action = new KAction(i18n("Move Session Right"), SmallIcon("forward"),
                              "Ctrl+Shift+Right", tab_bar, SLOT(slotMoveItemRight()),
                              actionCollection(), "move_tab_right");

    remove_tab_action = new KAction(i18n("Close Session"), SmallIcon("fileclose"), 0,
                                    this, 0, actionCollection(), "remove_tab");
    connect(remove_tab_action, SIGNAL(activated(KAction::ActivationReason, Qt::ButtonState)),
        this, SLOT(slotHandleRemoveSession(KAction::ActivationReason, Qt::ButtonState)));

    split_horiz_action = new KAction(i18n("Split Terminal Horizontally"), SmallIcon("view_left_right"),
                                     CTRL+SHIFT+Key_L, this, 0, actionCollection(), "split_horizontally");
    connect(split_horiz_action, SIGNAL(activated(KAction::ActivationReason, Qt::ButtonState)),
        this, SLOT(slotHandleHorizontalSplit(KAction::ActivationReason, Qt::ButtonState)));

    split_vert_action = new KAction(i18n("Split Terminal Vertically"), SmallIcon("view_top_bottom"),
                                    CTRL+SHIFT+Key_T, this, 0, actionCollection(), "split_vertically");
    connect(split_vert_action, SIGNAL(activated(KAction::ActivationReason, Qt::ButtonState)),
        this, SLOT(slotHandleVerticalSplit(KAction::ActivationReason, Qt::ButtonState)));

    remove_term_action = new KAction(i18n("Close Terminal"), SmallIcon("view_remove"),
                                     CTRL+SHIFT+Key_R, this, 0, actionCollection(), "remove_terminal");
    connect(remove_term_action, SIGNAL(activated(KAction::ActivationReason, Qt::ButtonState)),
        this, SLOT(slotHandleRemoveTerminal(KAction::ActivationReason, Qt::ButtonState)));

    QSignalMapper* tab_selection_mapper = new QSignalMapper(this);
    connect(tab_selection_mapper, SIGNAL(mapped(int)), this, SLOT(slotSelectTabPosition(int)));

    for (uint i = 1; i <= 12; ++i)
    {
        KAction* tab_selection_action = new KAction(i18n("Switch to Session %1").arg(i), 0, 0,
            tab_selection_mapper, SLOT(map()), actionCollection(), QString("go_to_tab_%1").arg(i).local8Bit());
        tab_selection_mapper->setMapping(tab_selection_action, i-1);
    }

    actionCollection()->readShortcutSettings("Shortcuts", &config);

    // Initialize settings.
    slotUpdateSettings();

    // Add first session.
    slotAddSession();

    connect(kapp, SIGNAL(aboutToQuit()), this, SLOT(slotAboutToQuit()));
    connect(kapp, SIGNAL(backgroundChanged(int)), this, SLOT(slotUpdateBackgroundState()));
    connect(tab_bar, SIGNAL(addItem()), this, SLOT(slotAddSession()));
    connect(tab_bar, SIGNAL(removeItem()), this, SLOT(slotRemoveSession()));
    connect(tab_bar, SIGNAL(itemSelected(int)), this, SLOT(slotSelectSession(int)));
    connect(&desk_info, SIGNAL(workAreaChanged()), this, SLOT(slotUpdateSize()));

    // Startup notification popup.
    if (Settings::popup())
        showPopup(i18n("Application successfully started!\nPress %1 to use it...").arg(global_key->shortcut("AccessKey").toString()));

    // First run dialog.
    if (Settings::firstrun())
    {
        QTimer::singleShot(0, this, SLOT(slotToggleState()));
        QTimer::singleShot(0, this, SLOT(slotOpenFirstRunDialog()));
    }
}

MainWindow::~MainWindow()
{

    if (!is_shutting_down)
        slotAboutToQuit();

    delete remove_tab_action;
    delete split_horiz_action;
    delete split_vert_action;
    delete remove_term_action;
    delete full_screen_action;
    delete screen_menu;
    delete width_menu;
    delete height_menu;
    delete session_menu;
    delete menu;
    delete about_app;
    delete about_kde;
}

void MainWindow::slotAboutToQuit()
{
    is_shutting_down = true;

    Settings::writeConfig();

    delete tab_bar;
    tab_bar = 0L;
    delete title_bar;
    title_bar = 0L;
    delete global_key;
    global_key = 0L;
    delete back_widget;
    back_widget = 0L;
    delete widgets_stack;
    widgets_stack = 0L;
}

bool MainWindow::queryClose()
{
    /* Ask before closing with multiple open sessions. */

    if (sessions_stack.size() > 1 && Settings::confirmquit())
    {
        if (focus_policy == false) focus_policy = true;

        int result = KMessageBox::warningYesNoCancel(
            this,
            i18n("You have multiple open sessions. These will be killed if you continue.\n\nAre you sure you want to quit?"),
            i18n("Really Quit?"),
            KStdGuiItem::quit(),
            KGuiItem(i18n("C&lose Session")));

        switch (result)
        {
            case KMessageBox::Yes:
                focus_policy = Settings::focus();
                KWin::activateWindow(winId());
                return true;
                break;
            case KMessageBox::No:
                focus_policy = Settings::focus();
                KWin::activateWindow(winId());
                slotRemoveSession();
                return false;
                break;

            default:
                focus_policy = Settings::focus();
                KWin::activateWindow(winId());
                return false;
                break;
        }
    }
    else
    {
        return true;
    }
}

void MainWindow::updateWindowMask()
{
    QRegion mask = title_bar->getWidgetMask();

    mask.translate(0, mask_height);
    mask += QRegion(0, 0, width(), mask_height);

    setMask(mask);
}

void MainWindow::showPopup(const QString& text, int time)
{
    /* Show a passive popup with the given text. */

    popup.setView(i18n("Yakuake Notification"), text, KApplication::kApplication()->miniIcon());
    popup.setTimeout(time);
    popup.show();
}

void MainWindow::slotAddSession()
{
    slotAddSession(Session::Single);
}

void MainWindow::slotAddSessionTwoHorizontal()
{
    slotAddSession(Session::TwoHorizontal);
}

void MainWindow::slotAddSessionTwoVertical()
{
    slotAddSession(Session::TwoVertical);
}

void MainWindow::slotAddSessionQuad()
{
    slotAddSession(Session::Quad);
}

void MainWindow::slotAddSession(Session::SessionType type)
{
    Session* session = new Session(widgets_stack, type);
    connect(session, SIGNAL(destroyed(int)), this, SLOT(slotSessionDestroyed(int)));
    connect(session, SIGNAL(titleChanged(const QString&)), this, SLOT(slotUpdateTitle(const QString&)));

    widgets_stack->addWidget(session->widget());
    sessions_stack.insert(session->id(), session);

    selected_id = session->id();

    tab_bar->addItem(selected_id);
    widgets_stack->raiseWidget(session->widget());
    title_bar->setTitleText(session->title());
}

void MainWindow::slotRemoveSession()
{
    sessions_stack[selected_id]->deleteLater();
}

void MainWindow::slotRemoveSession(int session_id)
{
    if (!sessions_stack[session_id]) return;

    sessions_stack[session_id]->deleteLater();
}

void MainWindow::slotHandleRemoveSession(KAction::ActivationReason reason, Qt::ButtonState /* state */)
{
    if (reason == KAction::PopupMenuActivation
        && tab_bar->pressedPosition() != -1)
    {
        slotRemoveSession(tab_bar->sessionIdForTabPosition(tab_bar->pressedPosition()));
        tab_bar->resetPressedPosition();
    }
    else
        slotRemoveSession();
}

void MainWindow::slotRemoveTerminal()
{
    sessions_stack[selected_id]->removeTerminal();
}

void MainWindow::slotRemoveTerminal(int session_id)
{
    if (!sessions_stack[session_id]) return;

    sessions_stack[session_id]->removeTerminal();
}

void MainWindow::slotRemoveTerminal(int session_id, int terminal_id)
{
    if (!sessions_stack[session_id]) return;

    sessions_stack[session_id]->removeTerminal(terminal_id);
}

void MainWindow::slotRenameSession(int session_id, const QString& name)
{
    tab_bar->renameItem(session_id, name);
}

void MainWindow::slotInteractiveRename()
{
    /* Open inline edit for the current item and show tab bar if necessary. */

    if (!Settings::tabs() && tab_bar->isHidden())
    {
        Settings::setTabs(true);
        tab_bar->show();
    }

    tab_bar->interactiveRename();
}

int MainWindow::selectedSession()
{
    return selected_id;
}

int MainWindow::selectedTerminal()
{
    return sessions_stack[selected_id]->activeTerminalId();
}


int MainWindow::tabPositionForSessionId(int session_id)
{
    return tab_bar->tabPositionForSessionId(session_id);
}

int MainWindow::sessionIdForTabPosition(int position)
{
    return tab_bar->sessionIdForTabPosition(position);
}

void MainWindow::slotSelectSession(int session_id)
{
    if (selected_id == session_id) return;
    if (!sessions_stack[session_id]) return;
    if (!sessions_stack[session_id]->widget()) return;

    selected_id = session_id;

    Session* session = sessions_stack[session_id];

    tab_bar->selectItem(session_id);
    widgets_stack->raiseWidget(session->widget());
    session->widget()->setFocus();
    title_bar->setTitleText(session->title());
}

void MainWindow::slotSelectTabPosition(int position)
{
    tab_bar->selectPosition(position);
}

const QString MainWindow::sessionTitle()
{
    return sessions_stack[selected_id]->title();
}

const QString MainWindow::sessionTitle(int session_id)
{
    if (!sessions_stack[session_id]) return 0;

    return sessions_stack[session_id]->title();
}

const QString MainWindow::sessionTitle(int session_id, int terminal_id)
{
    if (!sessions_stack[session_id]) return 0;

    return sessions_stack[session_id]->title(terminal_id);
}

void MainWindow::slotSetSessionTitleText(const QString& title)
{
    sessions_stack[selected_id]->setTitle(title);
}

void MainWindow::slotSetSessionTitleText(int session_id, const QString& title)
{
    if (!sessions_stack[session_id]) return;

    sessions_stack[session_id]->setTitle(title);
}

void MainWindow::slotSetSessionTitleText(int session_id, int terminal_id, const QString& title)
{
    if (!sessions_stack[session_id]) return;

    sessions_stack[session_id]->setTitle(terminal_id, title);
}

void MainWindow::slotPasteClipboard()
{
    sessions_stack[selected_id]->pasteClipboard();
}

void MainWindow::slotPasteClipboard(int session_id)
{
    if (!sessions_stack[session_id]) return;

    sessions_stack[session_id]->pasteClipboard();
}

void MainWindow::slotPasteClipboard(int session_id, int terminal_id)
{
    if (!sessions_stack[session_id]) return;

    sessions_stack[session_id]->pasteClipboard(terminal_id);
}

void MainWindow::slotPasteSelection()
{
    sessions_stack[selected_id]->pasteSelection();
}

void MainWindow::slotPasteSelection(int session_id)
{
    if (!sessions_stack[session_id]) return;

    sessions_stack[session_id]->pasteSelection();
}

void MainWindow::slotPasteSelection(int session_id, int terminal_id)
{
    if (!sessions_stack[session_id]) return;

    sessions_stack[session_id]->pasteSelection(terminal_id);
}

void MainWindow::slotRunCommandInSession(const QString& command)
{
    sessions_stack[selected_id]->runCommand(command);
}

void MainWindow::slotRunCommandInSession(int session_id, const QString& command)
{
    if (!sessions_stack[session_id]) return;

    sessions_stack[session_id]->runCommand(command);
}

void MainWindow::slotRunCommandInSession(int session_id, int terminal_id, const QString& command)
{
    if (!sessions_stack[session_id]) return;

    sessions_stack[session_id]->runCommand(terminal_id, command);
}

void MainWindow::slotSplitHorizontally()
{
    sessions_stack[selected_id]->splitHorizontally();
}

void MainWindow::slotSplitHorizontally(int session_id)
{
    if (!sessions_stack[session_id]) return;

    sessions_stack[session_id]->splitHorizontally();
}

void MainWindow::slotSplitHorizontally(int session_id, int terminal_id)
{
    if (!sessions_stack[session_id]) return;

    sessions_stack[session_id]->splitHorizontally(terminal_id);
}

void MainWindow::slotSplitVertically()
{
    sessions_stack[selected_id]->splitVertically();
}

void MainWindow::slotSplitVertically(int session_id)
{
    if (!sessions_stack[session_id]) return;

    sessions_stack[session_id]->splitVertically();
}

void MainWindow::slotSplitVertically(int session_id, int terminal_id)
{
    if (!sessions_stack[session_id]) return;

    sessions_stack[session_id]->splitVertically(terminal_id);
}


void MainWindow::slotFocusNextSplit()
{
    sessions_stack[selected_id]->focusNextSplit();
}

void MainWindow::slotFocusPreviousSplit()
{
    sessions_stack[selected_id]->focusPreviousSplit();
}

void MainWindow::windowActivationChange(bool old_active)
{
    /* Retract the window when focus changes. */

    if (!focus_policy && old_active && step)
        slotToggleState();
}

void MainWindow::slotHandleHorizontalSplit(KAction::ActivationReason reason, Qt::ButtonState /* state */)
{
    if (reason == KAction::PopupMenuActivation
        && tab_bar->pressedPosition() != -1)
    {
        slotSplitHorizontally(tab_bar->sessionIdForTabPosition(tab_bar->pressedPosition()));
        tab_bar->resetPressedPosition();
    }
    else
        slotSplitHorizontally();
}

void MainWindow::slotHandleVerticalSplit(KAction::ActivationReason reason, Qt::ButtonState /* state */)
{
    if (reason == KAction::PopupMenuActivation
        && tab_bar->pressedPosition() != -1)
    {
        slotSplitVertically(tab_bar->sessionIdForTabPosition(tab_bar->pressedPosition()));
        tab_bar->resetPressedPosition();
    }
    else
        slotSplitVertically();
}


void MainWindow::slotHandleRemoveTerminal(KAction::ActivationReason reason, Qt::ButtonState /* state */)
{
    if (reason == KAction::PopupMenuActivation
        && tab_bar->pressedPosition() != -1)
    {
        slotRemoveTerminal(tab_bar->sessionIdForTabPosition(tab_bar->pressedPosition()));
        tab_bar->resetPressedPosition();
    }
    else
        slotRemoveTerminal();
}

void MainWindow::initWindowProps()
{
    /* Initializes the window properties. */

    KWin::setState(winId(), NET::KeepAbove | NET::Sticky | NET::SkipTaskbar | NET::SkipPager);
    KWin::setOnAllDesktops(winId(), true);
}

int MainWindow::getMouseScreen()
{
    /* Gets the screen where the mouse pointer is located. */

    return QApplication::desktop()->screenNumber(QCursor::pos());
}

QRect MainWindow::getDesktopGeometry()
{
    /* Computes the desktop geometry. */

    if (full_screen)
    {
        if (!Settings::screen())
            return QApplication::desktop()->screenGeometry(getMouseScreen());
        else
            return QApplication::desktop()->screenGeometry(Settings::screen()-1);
    }

    QRect result;
    result = desk_info.workArea();

    KConfigGroup group(KGlobal::config(), "Windows");

    if (QApplication::desktop()->isVirtualDesktop() &&
            group.readBoolEntry("XineramaEnabled", true) &&
            group.readBoolEntry("XineramaPlacementEnabled", true))
    {
        if (!Settings::screen())
            return result.intersect(QApplication::desktop()->screenGeometry(getMouseScreen()));
        else
            return result.intersect(QApplication::desktop()->screenGeometry(Settings::screen()-1));
    }

    return result;
}

void MainWindow::createTabsBar()
{
    /* Creates the tabs frame. */

    tab_bar = new TabBar(this, "Session tab bar", Settings::skin());
    connect(this, SIGNAL(updateBackground()), tab_bar, SIGNAL(updateBackground()));

    tab_bar->setSessionMenu(session_menu);

    tab_bar->resize(width(), tab_bar->height());
}

void MainWindow::createTitleBar()
{
    /* Creates the title frame. */

    title_bar = new TitleBar(this, "Application title bar", Settings::skin());
    title_bar->setConfigurationMenu(menu);

    title_bar->resize(width(), title_bar->height());
}

void MainWindow::createMenu()
{
    /* Creates the main menu. */

    menu = new KPopupMenu();

    menu->insertTitle(i18n("Help"));

    actionCollection()->action(KStdAction::stdName(KStdAction::WhatsThis))->plug(menu);
    actionCollection()->action(KStdAction::stdName(KStdAction::AboutApp))->plug(menu);
    actionCollection()->action(KStdAction::stdName(KStdAction::AboutKDE))->plug(menu);

    menu->insertTitle(i18n("Quick Options"));

    actionCollection()->action(KStdAction::stdName(KStdAction::FullScreen))->plug(menu);

    screen_menu = new KPopupMenu(menu);

    if (QApplication::desktop()->numScreens() > 1)
    {
        menu->insertItem(i18n("Open on screen"), screen_menu);
        connect(screen_menu, SIGNAL(activated(int)), this, SLOT(slotSetScreen(int)));
    }

    width_menu = new KPopupMenu(menu);
    menu->insertItem(i18n("Width"), width_menu);
    connect(width_menu, SIGNAL(activated(int)), this, SLOT(slotSetWidth(int)));

    height_menu = new KPopupMenu(menu);
    menu->insertItem(i18n("Height"), height_menu);
    connect(height_menu, SIGNAL(activated(int)), this, SLOT(slotSetHeight(int)));

    menu->insertItem(i18n("Keep open on focus change"), this, SLOT(slotSetFocusPolicy()), 0, Focus);

    menu->insertTitle(i18n("Settings"));

    actionCollection()->action("global_shortcuts")->plug(menu);
    actionCollection()->action(KStdAction::stdName(KStdAction::KeyBindings))->plug(menu);
    actionCollection()->action(KStdAction::stdName(KStdAction::Preferences))->plug(menu);
}

void MainWindow::updateWidthMenu()
{
    width_menu->clear();
    for (int i = 10; i <= 100; i += 10) width_menu->insertItem(QString::number(i) + '%', i);
    width_menu->setItemChecked(Settings::width(), true);
}

void MainWindow::updateHeightMenu()
{
    height_menu->clear();
    for (int i = 10; i <= 100; i += 10) height_menu->insertItem(QString::number(i) + '%', i);
    height_menu->setItemChecked(Settings::height(), true);
}

void MainWindow::createSessionMenu()
{
    session_menu = new KPopupMenu();

    actionCollection()->action("add_tab")->plug(session_menu);
    actionCollection()->action("add_tab_twohorizontal")->plug(session_menu);
    actionCollection()->action("add_tab_twovertical")->plug(session_menu);
    actionCollection()->action("add_tab_quad")->plug(session_menu);
}

void MainWindow::slotUpdateTitle(const QString& title)
{
    title_bar->setTitleText(title);
}


void MainWindow::slotIncreaseSizeW()
{
    /* Increase the window's width. */

    if (Settings::width() < 100)
    {
        Settings::setWidth(Settings::width() + 10);
        updateWidthMenu();
        slotUpdateSize();
    }
}

void MainWindow::slotDecreaseSizeW()
{
    /* Decrease the window's width. */

    if (Settings::width() > 10)
    {
        Settings::setWidth(Settings::width() - 10);
        updateWidthMenu();
        slotUpdateSize();
    }
}

void MainWindow::slotIncreaseSizeH()
{
    /* Increase the window's height. */

    if (Settings::height() < 100)
    {
        Settings::setHeight(Settings::height() + 10);
        updateHeightMenu();
        slotUpdateSize();
    }
}

void MainWindow::slotDecreaseSizeH()
{
    /* Decrease the window's height. */

    if (Settings::height() > 10)
    {
        Settings::setHeight(Settings::height() - 10);
        updateHeightMenu();
        slotUpdateSize();
    }
}

void MainWindow::slotSessionDestroyed(int id)
{
    if (is_shutting_down) return;

    int session_id = (id != -1) ? id : selected_id;

    sessions_stack.remove(session_id);

    if (tab_bar->removeItem(session_id) == -1)
        slotAddSession();
}

void MainWindow::slotToggleState()
{
    /* Toggles the window's open/closed state. */

    static int state = 1;

    if (timer.isActive())
        return;

    KWinModule kwin(this);

    if (state)
    {
        initWindowProps();

        slotUpdateSize();

        show();

        KWin::forceActiveWindow(winId());
        connect(&timer, SIGNAL(timeout()), this, SLOT(slotIncreaseHeight()));
                initWindowProps();
        timer.start(10, false);
        state = !state;
    }
    else
    {
        if (!this->isActiveWindow() && focus_policy)
        {
            KWin::forceActiveWindow(winId());
            return;
        }
        else if (full_screen)
            this->setWindowState( this->windowState() & ~Qt::WindowFullScreen);

        connect(&timer, SIGNAL(timeout()), this, SLOT(slotDecreaseHeight()));
        timer.start(10, false);
        state = !state;
    }
}

void MainWindow::slotIncreaseHeight()
{
    /* Increases the window's height. */

    int steps = (Settings::steps() == 0) ? 1 : Settings::steps();

    mask_height = (step++ * max_height) / steps;

    if (step >= steps)
    {
        step = steps;
        timer.stop();
        disconnect(&timer, SIGNAL(timeout()), 0, 0);

        mask_height = max_height;

        if (background_changed)
        {
            emit updateBackground();
            background_changed = false;
        }
    }

    updateWindowMask();
    title_bar->move(0, mask_height);
}

void MainWindow::slotDecreaseHeight()
{
    /* Decreases the window's height. */

    int steps = (Settings::steps() == 0) ? 1 : Settings::steps();

    mask_height = (--step * max_height) / steps;

    if (step <= 0)
    {
        step = 0;
        timer.stop();
        disconnect(&timer, SIGNAL(timeout()), 0, 0);

        hide();
    }

    updateWindowMask();
    title_bar->move(0, mask_height);
}

void MainWindow::slotInitSkin()
{
    KConfig config(locate("appdata", Settings::skin() + "/title.skin"));

    config.setGroup("Border");

    margin = config.readNumEntry("width", 0);

    back_widget->setBackgroundColor(QColor(config.readNumEntry("red", 0),
                                           config.readNumEntry("green", 0),
                                           config.readNumEntry("blue", 0)));
}

void MainWindow::slotUpdateSize()
{
    if (full_screen) full_screen_action->activate();
    slotUpdateSize(Settings::width(), Settings::height(), Settings::location());
}

void MainWindow::slotUpdateSize(int new_width, int new_height, int new_location)
{
    /* Updates the window size. */

    int tmp_height;
    QRect desk_area;

    // Xinerama aware work area.
    desk_area = getDesktopGeometry();
    max_height = (desk_area.height() - 1) * new_height / 100;

    // Update the size of the components.
    setGeometry(desk_area.x() + desk_area.width() * new_location * (100 - new_width) / 10000,
                desk_area.y(), desk_area.width() * new_width / 100, max_height);

    max_height -= title_bar->height();
    title_bar->setGeometry(0, max_height, width(), title_bar->height());

    tmp_height = max_height;

    if (Settings::tabs())
    {
        tmp_height -= tab_bar->height();
        tab_bar->setGeometry(margin, tmp_height, width() - 2 * margin, tab_bar->height());
    }

    widgets_stack->setGeometry(margin, 0, width() - 2 * margin, tmp_height);

    back_widget->setGeometry(0, 0, width(), height());

    // Update the window mask.
    mask_height = (isVisible()) ? max_height : 0;
    updateWindowMask();
}

void MainWindow::slotSetFullScreen(bool state)
{
     if (state)
     {
        full_screen = true;
        slotUpdateSize(100, 100, Settings::location());
     }
     else
     {
        full_screen = false;
        slotUpdateSize();
     }
}

void MainWindow::slotUpdateFullScreen()
{
    if (full_screen_action->isChecked())
        showFullScreen();
    else
        this->setWindowState( this->windowState() & ~Qt::WindowFullScreen);
}

void MainWindow::slotSetFocusPolicy()
{
    slotSetFocusPolicy(!focus_policy);
}

void MainWindow::slotSetFocusPolicy(bool focus)
{
    Settings::setFocus(focus);
    focus_policy = Settings::focus();
    menu->setItemChecked(Focus, Settings::focus());
    title_bar->setFocusButtonEnabled(Settings::focus());
}

void MainWindow::slotSetWidth(int width)
{
    Settings::setWidth(width);
    slotUpdateSettings();
}

void MainWindow::slotSetHeight(int height)
{
    Settings::setHeight(height);
    slotUpdateSettings();
}

void MainWindow::slotSetScreen(int screen)
{
    Settings::setScreen(screen);
    slotUpdateSettings();
}

void MainWindow::slotSetAccessKey()
{
    if (full_screen) full_screen_action->activate();

    if (focus_policy == false) focus_policy = true;

    KConfig config(CONFIG_FILE);

    KKeyDialog::configure(global_key);

    global_key->updateConnections();
    global_key->writeSettings(&config);

    slotDialogFinished();
}

void MainWindow::slotSetControlKeys()
{
    if (full_screen) full_screen_action->activate();

    if (focus_policy == false) focus_policy = true;

    KConfig config(CONFIG_FILE);

    KKeyDialog::configure(actionCollection());

    actionCollection()->writeShortcutSettings("Shortcuts", &config);

    slotDialogFinished();
}

void MainWindow::slotUpdateBackgroundState()
{
    background_changed = true;
}

void MainWindow::slotUpdateSettings()
{
    slotInitSkin();

    title_bar->reloadSkin(Settings::skin());
    tab_bar->reloadSkin(Settings::skin());

    step = (isVisible()) ? Settings::steps() : 0;

    focus_policy = Settings::focus();

    if (Settings::tabs())
        tab_bar->show();
    else
        tab_bar->hide();

    slotUpdateSize();

    menu->setItemChecked(Focus, Settings::focus());
    title_bar->setFocusButtonEnabled(Settings::focus());

    updateWidthMenu();
    updateHeightMenu();

    screen_menu->clear();
    screen_menu->insertItem(i18n("At mouse location"), 0);
    screen_menu->insertSeparator();
    for (int i = 1; i <= QApplication::desktop()->numScreens(); i++)
        screen_menu->insertItem(i18n("Screen %1").arg(QString::number(i)), i);
    screen_menu->setItemChecked(Settings::screen(), true);
}

void MainWindow::slotOpenSettingsDialog()
{
    if (full_screen) full_screen_action->activate();

    if (focus_policy == false)
        focus_policy = true;

    if (KConfigDialog::showDialog("settings"))
        return;

    KConfigDialog* settings_dialog = new KConfigDialog(this, "settings", Settings::self());

    GeneralSettings* general_settings = new GeneralSettings(0, "General");
    settings_dialog->addPage(general_settings, i18n("General"), "package_settings");
    connect(general_settings, SIGNAL(updateSize(int, int, int)), this, SLOT(slotUpdateSize(int, int, int)));

    SkinSettings* skin_settings = new SkinSettings(0, "Skins");
    settings_dialog->addPage(skin_settings, i18n("Skins"), "style");
    connect(skin_settings, SIGNAL(settingsChanged()), settings_dialog, SIGNAL(settingsChanged()));
    connect(settings_dialog, SIGNAL(closeClicked()), skin_settings, SLOT(slotResetSelection()));
    connect(settings_dialog, SIGNAL(cancelClicked()), skin_settings, SLOT(slotResetSelection()));

    connect(settings_dialog, SIGNAL(settingsChanged()), this, SLOT(slotUpdateSettings()));
    connect(settings_dialog, SIGNAL(hidden()), this, SLOT(slotDialogFinished()));

    settings_dialog->show();
}

void MainWindow::slotOpenFirstRunDialog()
{
    if (!first_run_dialog)
    {
        first_run_dialog = new KDialogBase(this,
            "First Run Dialog", true, i18n("First Run"),
            KDialogBase::Ok|KDialogBase::Cancel, KDialogBase::Ok, true);
        connect(first_run_dialog, SIGNAL(okClicked()), this, SLOT(slotFirstRunDialogOK()));
        connect(first_run_dialog, SIGNAL(cancelClicked()), this, SLOT(slotFirstRunDialogCancel()));
        connect(first_run_dialog, SIGNAL(closeClicked()), this, SLOT(slotFirstRunDialogCancel()));
        connect(first_run_dialog, SIGNAL(hidden()), this, SLOT(slotDialogFinished()));

        FirstRunDialog* first_run_dialog_page = new FirstRunDialog(first_run_dialog);
        first_run_dialog_page->setMinimumSize(first_run_dialog_page->sizeHint());
        first_run_dialog_page->setShortcut(global_key->shortcut("AccessKey"));

        first_run_dialog->setMainWidget(first_run_dialog_page);
        first_run_dialog->adjustSize();
        first_run_dialog->disableResize();
    }

    if (focus_policy == false)
        focus_policy = true;

    first_run_dialog->show();
}

void MainWindow::slotFirstRunDialogOK()
{
    if (!first_run_dialog)
        return;

    FirstRunDialog* first_run_dialog_page =
        static_cast<FirstRunDialog*>(first_run_dialog->mainWidget());

    if (!first_run_dialog_page)
        return;


    if (first_run_dialog_page->shortcut() != global_key->shortcut("AccessKey"))
    {
        KConfig config(CONFIG_FILE);
        global_key->setShortcut("AccessKey", first_run_dialog_page->shortcut());
        global_key->updateConnections();
        global_key->writeSettings(&config);
    }

    Settings::setFirstrun(false);
    Settings::writeConfig();
}

void MainWindow::slotFirstRunDialogCancel()
{
    Settings::setFirstrun(false);
    Settings::writeConfig();
}

void MainWindow::slotOpenAboutApp()
{
    if (!about_app)
    {
        about_app = new KAboutApplication(this, "About Yakuake");
        connect(about_app, SIGNAL(hidden()), this, SLOT(slotDialogFinished()));
    }

    if (full_screen) full_screen_action->activate();

    if (focus_policy == false)
        focus_policy = true;

    about_app->show();
}

void MainWindow::slotOpenAboutKDE()
{
    if (!about_kde)
    {
        about_kde = new KAboutKDE(this, "About KDE");
        connect(about_kde, SIGNAL(hidden()), this, SLOT(slotDialogFinished()));
    }

    if (full_screen) full_screen_action->activate();

    if (focus_policy == false)
        focus_policy = true;

    about_kde->show();
}

void MainWindow::slotDialogFinished()
{
    slotUpdateSize();

    focus_policy = Settings::focus();

    KWin::activateWindow(winId());
}
