/*****************************************************************************
 *                                                                           *
 *   Copyright (C) 2005 by Chazal Francois             <neptune3k@free.fr>   *
 *   website : http://workspace.free.fr                                      *
 *                                                                           *
 *                     =========  GPL License  =========                     *
 *    This program is free software; you can redistribute it and/or modify   *
 *   it under the terms of the  GNU General Public License as published by   *
 *   the  Free  Software  Foundation ; either version 2 of the License, or   *
 *   (at your option) any later version.                                     *
 *                                                                           *
 *****************************************************************************/

//== INCLUDE REQUIREMENTS =====================================================

/*
** KDE libraries */
#include <kmessagebox.h>

/*
** Local libraries */
#include "main_window.h"
#include "main_window.moc"



//== CONSTRUCTORS AND DESTRUCTORS =============================================

MainWindow::MainWindow(QWidget * parent, const char * name) :
        DCOPObject("DCOPInterface"),
        KMainWindow(parent, name, Qt::WStyle_Customize | Qt::WStyle_NoBorder),
        step(0)
{
    isShuttingDown = false;
    KConfig config(CONFIG_FILE);

    initWindowProps();

    back_widget = new QWidget(this);
    widgets_stack = new QWidgetStack(this);

    // Register with DCOP ---------------------------------

    if (!kapp->dcopClient()->isRegistered())
    {
        kapp->dcopClient()->registerAs("dcopinterface");
        kapp->dcopClient()->setDefaultObject(objId());
    }

    // Initializes the skin (1) ---------------------------

    config.setGroup("Options");
    skin = config.readEntry("skin", "default");

    // Initializes the skin (2) ---------------------------

    KConfig config2(locate("appdata", skin + "/title.skin"));

    config2.setGroup("Border");

    margin = config2.readNumEntry("width", 0);

    back_widget->setBackgroundColor(QColor(config2.readNumEntry("red", 0),
                                           config2.readNumEntry("green", 0),
                                           config2.readNumEntry("blue", 0)));

    // Creates the widgets --------------------------------

    createMenu();
    createTabsBar();
    createTitleBar();

    // Initializes the properties -------------------------

    slotSetSpeed(config.readNumEntry("steps", 20));
    slotSetSizeW(config.readNumEntry("width", 100));
    slotSetSizeH(config.readNumEntry("height", 50));
    slotSetScreen(config.readNumEntry("screen", 1));
    slotSetLocationH(config.readNumEntry("location", 50));
    slotSetTabsPolicy(config.readBoolEntry("tabs", true));
    slotSetFocusPolicy(config.readBoolEntry("focus", true));
    slotSetBackgroundPolicy(config.readBoolEntry("background", false));

    // Add first session --------------------------------

    slotAddSession();

    // Initializes access key ---------------------------

    global_key = new KGlobalAccel(this);
    global_key->insert("AccessKey", i18n("Access key"),
                       i18n("Toggles the open/close state of Yakuake"),
                       Key_F12, 0, this, SLOT(slotToggleState()));

    global_key->readSettings(&config);
    global_key->updateConnections();

    // Initializes controls keys ------------------------

    action_new = new KAction(i18n("New Session"), "Ctrl+Shift+N",
                             this, SLOT(slotAddSession()),
                             actionCollection(), "add_tab");
    action_del = new KAction(i18n("Close Session"), 0,
                             this, SLOT(slotRemoveSession()),
                             actionCollection(), "remove_tab");

    action_next = new KAction(i18n("Go to Next Session"), "Shift+Right",
                              tabs_bar, SLOT(slotSelectNextItem()),
                              actionCollection(), "next_tab");
    action_prev = new KAction(i18n("Go to Previous Session"), "Shift+Left",
                              tabs_bar, SLOT(slotSelectPreviousItem()),
                              actionCollection(), "previous_tab");
    action_paste = new KAction(i18n("Paste"), SHIFT + Key_Insert,
                               this, SLOT(slotPasteClipboard()),
                               actionCollection(), "paste_clipboard");
    action_rename = new KAction(i18n("Rename Session..."), "Alt+Ctrl+S",
                               this, SLOT(slotInteractiveRename()),
                               actionCollection(), "edit_name");
    action_increasew = new KAction(i18n("Increase Width"), "Alt+Shift+Right",
                               this, SLOT(slotIncreaseSizeW()),
                    actionCollection(), "increasew");
    action_decreasew = new KAction(i18n("Decrease Width"), "Alt+Shift+Left",
                               this, SLOT(slotDecreaseSizeW()),
                    actionCollection(), "decreasew");
    action_increaseh = new KAction(i18n("Increase Height"), "Alt+Shift+Down",
                               this, SLOT(slotIncreaseSizeH()),
                    actionCollection(), "increaseh");
    action_decreaseh = new KAction(i18n("Decrease Height"), "Alt+Shift+Up",
                               this, SLOT(slotDecreaseSizeH()),
                    actionCollection(), "decreaseh");

    actionCollection()->readShortcutSettings("Shortcuts", &config);

    // Connects slots to signals --------------------------

    connect(kapp, SIGNAL(aboutToQuit()), this, SLOT(slotAboutToQuit()));
    connect(tabs_bar, SIGNAL(addItem()), this, SLOT(slotAddSession()));
    connect(tabs_bar, SIGNAL(removeItem()), this, SLOT(slotRemoveSession()));
    connect(tabs_bar, SIGNAL(itemSelected(int)), this, SLOT(slotSelectSession(int)));

    connect(&desk_info, SIGNAL(workAreaChanged()), this, SLOT(slotUpdateSize()));

    // Displays a popup window ----------------------------

    showPopup(i18n("Application successfully started!\nPress %1 to use it...").arg(global_key->shortcut("AccessKey").toString()));
}

void MainWindow::slotAboutToQuit()
{
    isShuttingDown = true;
    delete tabs_bar;
    tabs_bar = 0L;
    delete title_bar;
    title_bar = 0L;
    delete global_key;
    global_key = 0L;
    delete back_widget;
    back_widget = 0L;
    delete widgets_stack;
    widgets_stack = 0L;
}

MainWindow::~MainWindow()
{
    if (!isShuttingDown)
        slotAboutToQuit();

    delete action_new;
    delete action_del;
    delete action_next;
    delete action_prev;
    delete action_paste;
    delete action_rename;
    delete action_increaseh;
    delete action_decreaseh;

    delete menu;
    delete sizeH_menu;
    delete sizeW_menu;
    delete speed_menu;
    delete locationH_menu;
}



//== PUBLIC METHODS ===========================================================


/******************************************************************************
** Returns the selected id
****************************/

int    MainWindow::selectedSession()
{
    return selected_id;
}


/******************************************************************************
** Updates the window's mask
******************************/

void    MainWindow::updateWindowMask()
{
    QRegion mask = title_bar->getWidgetMask();

    mask.translate(0, mask_height);
    mask += QRegion(0, 0, width(), mask_height);

    setMask(mask);
}


/******************************************************************************
** Show a passive popup with the given text
************************************************/

void    MainWindow::showPopup(const QString& text, int time)
{
    popup.setView(i18n("Yakuake Notification"), text, KApplication::kApplication()->miniIcon());
    popup.setTimeout(time);
    popup.show();
}



//== PUBLIC SLOTS =============================================================


/******************************************************************************
** Adds a session
*******************/

void    MainWindow::slotAddSession()
{
    selected_id = createSession();

    tabs_bar->addItem(selected_id);
    widgets_stack->raiseWidget(selected_id);
    title_bar->setTitleText(sessions_stack[selected_id]->session_title);
}


/******************************************************************************
** Selects a given session
****************************/

void    MainWindow::slotSelectSession(int id)
{
    QWidget* widget = widgets_stack->widget(id);

    if (widget == NULL)
        return;

    selected_id = id;

    tabs_bar->selectItem(id);
    widgets_stack->raiseWidget(id);
    widgets_stack->widget(id)->setFocus();
    title_bar->setTitleText(sessions_stack[id]->session_title);
}


/******************************************************************************
** Removes a session
**********************/

void    MainWindow::slotRemoveSession()
{
    QWidget *   widget = widgets_stack->widget(selected_id);

    if (widget == NULL)
        return;

    widgets_stack->removeWidget(widget);
    sessions_stack.remove(selected_id);
    delete widget;

    if (tabs_bar->removeItem(selected_id) == -1)
        slotAddSession();
}


/******************************************************************************
** Paste the clipboard contents
*********************************/

void    MainWindow::slotPasteClipboard()
{
    TerminalInterface * terminal;

    terminal = sessions_stack[selected_id]->session_terminal;
    if (terminal != NULL)
        terminal->sendInput(QApplication::clipboard()->text(QClipboard::Clipboard));
}


/******************************************************************************
** Renames an item given its id
*********************************/

void    MainWindow::slotRenameSession(int id, const QString & name)
{
    tabs_bar->renameItem(id, name);
}


/******************************************************************************
** Open inline edit for the current item and show tab bar if necessary
************************************************************************/

void    MainWindow::slotInteractiveRename()
{
    if (!tabs_policy && tabs_bar->isHidden())
    {
        slotSetTabsPolicy();
        tabs_bar->show();
    }

    tabs_bar->interactiveRename();
}


/******************************************************************************
** Sets the session titlebar text
***********************************/

void    MainWindow::slotSetSessionTitleText(int id, const QString & text)
{
    sessions_stack[id]->session_title = text;
    title_bar->setTitleText(text);
}


/******************************************************************************
** Runs a given command in the selected session
*************************************************/

void    MainWindow::slotRunCommandInSession(int id, const QString & value)
{
    TerminalInterface * terminal;

    terminal = sessions_stack[id]->session_terminal;
    if (terminal != NULL)
        terminal->sendInput(value + '\n');
}



//== PROTECTED METHODS ========================================================


/******************************************************************************
** Retract the window when activation changes
***********************************************/

void    MainWindow::windowActivationChange(bool old_active)
{
    if (!focus_policy && old_active && step)
        slotToggleState();
}


/******************************************************************************
** Ask before closing with multiple open sessions
***************************************************/

bool    MainWindow::queryClose()
{
    if (sessions_stack.size() > 1)
    {
        this->focus_policy = !focus_policy;

        int result = KMessageBox::warningYesNoCancel(
            this,
            i18n("You have multiple open sessions. These will be killed if you continue.\n\nDo you really want to quit?"),
            i18n("Really Quit?"),
            KStdGuiItem::quit(),
            KGuiItem(i18n("C&lose Session")),
            "QuitMultiple");

        switch (result)
        {
            case KMessageBox::Yes:
                this->focus_policy = !focus_policy;
                return true;
                break;
            case KMessageBox::No:
                this->focus_policy = !focus_policy;
                slotRemoveSession();
                return false;
                break;

            default:
                this->focus_policy = !focus_policy;
                return false;
                break;
        }
    }
    else
    {
        return true;
    }
}



//== PRIVATE METHODS ==========================================================


/******************************************************************************
** Initializes the window properties
**************************************/

void    MainWindow::initWindowProps()
{
    KWin::setState(winId(), NET::KeepAbove | NET::Sticky | NET::SkipTaskbar | NET::SkipPager);
    KWin::setOnAllDesktops(winId(), true);
}


/******************************************************************************
** Gets the mouse screen where the mouse is located
*****************************************************/

int    MainWindow::getMouseScreen()
{
    return QApplication::desktop()->screenNumber(QCursor::pos());
}


/******************************************************************************
** Computes the desktop geometry
**********************************/

QRect MainWindow::getDesktopGeometry()
{
    QRect           result;
    KConfigGroup    group(KGlobal::config(), "Windows");

    result = desk_info.workArea();

    if (QApplication::desktop()->isVirtualDesktop() &&
            group.readBoolEntry("XineramaEnabled", true) &&
            group.readBoolEntry("XineramaPlacementEnabled", true))
        return result.intersect(QApplication::desktop()->screenGeometry(screen));

    return result;
}


/******************************************************************************
** Creates the tabs frame
***************************/

void    MainWindow::createTabsBar()
{
    tabs_bar = new TabsBar(this, "Session tabs bar", skin);

    tabs_bar->resize(width(), tabs_bar->height());
}


/******************************************************************************
** Creates the title frame
****************************/

void    MainWindow::createTitleBar()
{
    title_bar = new TitleBar(this, "Application title bar", skin);
    title_bar->setConfigurationMenu(menu);

    title_bar->resize(width(), title_bar->height());
}


/******************************************************************************
** Creates a sessions objects
*******************************/

int    MainWindow::createSession()
{
    int             index;
    QWidget *       widget;
    ShellSession *  session;

    widget = new QWidget(widgets_stack);

    // Adds the widget to stacks --------------------------

    index = widgets_stack->addWidget(widget);
    setenv("DCOP_YAKUAKE_SESSION", QString::number(index).ascii(), 1);
    putenv((char*)"COLORTERM="); // Trigger mc's color detection.

    if ((session = new ShellSession(widget)) == NULL)
        widgets_stack->removeWidget(widget);
    else
    {
        widget->setFocusProxy(session->session_widget);

        QBoxLayout * l = new QVBoxLayout(widget);
        l->addWidget(session->session_widget);

        sessions_stack.insert(index, session);

        session->setId(index);

        connect(session, SIGNAL(destroyed(int)), this, SLOT(slotSessionDestroyed(int)));
        connect(session, SIGNAL(titleUpdated()), this, SLOT(slotUpdateTitle()));

        return  index;
    }
    return 0;
}


/******************************************************************************
** Creates the configuration menu
***********************************/

void    MainWindow::createMenu()
{
    menu = new KPopupMenu();

    menu->insertTitle(i18n("Properties"));

    // Creates the screen menu ----------------------------

    screen_menu = new KPopupMenu(menu);
    for (int i = 1; i <= QApplication::desktop()->numScreens(); i++)
        screen_menu->insertItem(i18n("Screen: %1").arg(QString::number(i)), i);

    screen_menu->insertSeparator();
    screen_menu->insertItem(i18n("Use Mouse Location"), 0);

    if (QApplication::desktop()->numScreens() > 1)
    {
        menu->insertItem(i18n("Screen Display"), screen_menu);
        connect(screen_menu, SIGNAL(activated(int)), this, SLOT(slotSetScreen(int)));
    }

    // Creates the sizeW menu -----------------------------


    sizeW_menu = new KPopupMenu(menu);
    for (int i = 10; i <= 100; i += 10)
        sizeW_menu->insertItem(QString::number(i) + '%', i);

    menu->insertItem(i18n("Terminal Width"), sizeW_menu);
    connect(sizeW_menu, SIGNAL(activated(int)), this, SLOT(slotSetSizeW(int)));

    // Creates the sizeH menu -----------------------------

    sizeH_menu = new KPopupMenu(menu);
    for (int i = 10; i <= 100; i += 10)
        sizeH_menu->insertItem(QString::number(i) + '%', i);

    menu->insertItem(i18n("Terminal Height"), sizeH_menu);
    connect(sizeH_menu, SIGNAL(activated(int)), this, SLOT(slotSetSizeH(int)));

    // Creates the locationH menu -------------------------

    locationH_menu = new KPopupMenu(menu);
    for (int i = 0; i <= 100; i += 10)
        locationH_menu->insertItem(QString::number(i) + '%', i);

    menu->insertItem(i18n("Horizontal Location"), locationH_menu);
    connect(locationH_menu, SIGNAL(activated(int)), this, SLOT(slotSetLocationH(int)));

    // Creates the speed menu -----------------------------

    speed_menu = new KPopupMenu(menu);
    speed_menu->insertItem(i18n("None"), 1);
    for (int i = 50; i <= 500; i += 50)
        speed_menu->insertItem('~' + QString::number(i) + "ms", i/10);

    menu->insertItem(i18n("Animation Duration"), speed_menu);
    connect(speed_menu, SIGNAL(activated(int)), this, SLOT(slotSetSpeed(int)));

    // Adds the options modifier --------------------------

    menu->insertTitle(i18n("Options"));

    menu->insertItem(i18n("Show the Tab Bar"), this, SLOT(slotSetTabsPolicy()), 0, 1);
    menu->insertItem(i18n("Retract when Focus is Lost"), this, SLOT(slotSetFocusPolicy()), 0, 2);
    menu->insertItem(i18n("Force Background Refresh"), this, SLOT(slotSetBackgroundPolicy()), 0, 3);

    // Adds the shortcuts modifiers -----------------------

    menu->insertTitle(i18n("Shortcuts"));

    menu->insertItem(i18n("Change Access Key..."), this, SLOT(slotSetAccessKey()));
    menu->insertItem(i18n("Change Control Keys..."), this, SLOT(slotSetControlKeys()));
}



//== PRIVATE SLOTS ============================================================


/******************************************************************************
** Sets the session titlebar text
***********************************/

void    MainWindow::slotUpdateTitle()
{
    title_bar->setTitleText(sessions_stack[selected_id]->session_title);
}


/******************************************************************************
** Sets the access key
************************/

void    MainWindow::slotSetAccessKey()
{
    KConfig config(CONFIG_FILE);

    KKeyDialog::configure(global_key);

    global_key->updateConnections();
    global_key->writeSettings(&config);
}


/******************************************************************************
** Sets the control keys
**************************/

void    MainWindow::slotSetControlKeys()
{
    KConfig config(CONFIG_FILE);

    KKeyDialog::configure(actionCollection());

    actionCollection()->writeShortcutSettings("Shortcuts", &config);
}


/******************************************************************************
** Sets the tabs policy
*************************/

void    MainWindow::slotSetTabsPolicy()
{
    slotSetTabsPolicy(!tabs_policy);
}

void    MainWindow::slotSetTabsPolicy(bool tabs_policy)
{
    menu->setItemChecked(1, tabs_policy);
    this->tabs_policy = tabs_policy;

    if (tabs_policy)
        tabs_bar->show();
    else
        tabs_bar->hide();

    slotUpdateSize();

    // Updates the configuration --------------------------

    KConfig config(CONFIG_FILE);

    config.setGroup("Options");
    config.writeEntry("tabs", tabs_policy);
}


/******************************************************************************
** Sets the focus policy
**************************/

void    MainWindow::slotSetFocusPolicy()
{
    slotSetFocusPolicy(!focus_policy);
}

void    MainWindow::slotSetFocusPolicy(bool focus_policy)
{
    menu->setItemChecked(2, !focus_policy);
    this->focus_policy = focus_policy;

    // Updates the configuration --------------------------

    KConfig config(CONFIG_FILE);

    config.setGroup("Options");
    config.writeEntry("focus", focus_policy);
}


/******************************************************************************
** Sets the background policy
*******************************/

void    MainWindow::slotSetBackgroundPolicy()
{
    slotSetBackgroundPolicy(!background_policy);
}

void    MainWindow::slotSetBackgroundPolicy(bool background_policy)
{
    menu->setItemChecked(3, background_policy);
    this->background_policy = background_policy;

    // Updates the configuration --------------------------

    KConfig config(CONFIG_FILE);

    config.setGroup("Options");
    config.writeEntry("background", background_policy);
}


/******************************************************************************
** Sets the animation speed
*****************************/

void    MainWindow::slotSetSpeed(int steps)
{
    speed_menu->setItemChecked(this->steps, false);
    speed_menu->setItemChecked(steps, true);
    this->steps = steps;
    step = (isVisible()) ? steps : 0;

    // Updates the configuration --------------------------

    KConfig config(CONFIG_FILE);

    config.setGroup("Options");
    config.writeEntry("steps", steps);
}


/******************************************************************************
** Sets the window's height
*****************************/

void    MainWindow::slotSetSizeH(int sizeH)
{
    sizeH_menu->setItemChecked(this->sizeH, false);
    sizeH_menu->setItemChecked(sizeH, true);
    this->sizeH = sizeH;

    // Updates the configuration --------------------------

    KConfig config(CONFIG_FILE);

    config.setGroup("Options");
    config.writeEntry("height", sizeH);

    // Updates the size of the window ---------------------

    slotUpdateSize();
}

/******************************************************************************
** Increase the window's width
****************************/

void    MainWindow::slotIncreaseSizeW()
{
    int sizeW = this->sizeW;

    if (sizeW < 100)
        slotSetSizeW(sizeW + 10);
}


/******************************************************************************
** Decrease the window's width
****************************/

void    MainWindow::slotDecreaseSizeW()
{
    int sizeW = this->sizeW;

    if (sizeW > 10)
        slotSetSizeW(sizeW - 10);
}

/******************************************************************************
** Increase the window's height
****************************/

void	MainWindow::slotIncreaseSizeH()
{
    int sizeH = this->sizeH;

    if (sizeH < 100)
        slotSetSizeH(sizeH + 10);
}


/******************************************************************************
** Decrease the window's height
****************************/

void	MainWindow::slotDecreaseSizeH()
{
    int sizeH = this->sizeH;

    if (sizeH > 10)
        slotSetSizeH(sizeH - 10);
}


/******************************************************************************
** Sets the window's width
****************************/

void    MainWindow::slotSetSizeW(int sizeW)
{
    sizeW_menu->setItemChecked(this->sizeW, false);
    sizeW_menu->setItemChecked(sizeW, true);
    this->sizeW = sizeW;

    // Updates the configuration --------------------------

    KConfig config(CONFIG_FILE);

    config.setGroup("Options");
    config.writeEntry("width", sizeW);

    // Updates the size of the window ---------------------

    slotUpdateSize();
}


/******************************************************************************
** Sets the window to a specific screen (xinerama option)
***********************************************************/

void MainWindow::slotSetScreen(int screen)
{
    screen_menu->setItemChecked(this->screen, false);
    screen_menu->setItemChecked(screen, true);
    this->screen = screen;

    this->screen_policy = (!screen) ? true : false;

    // Updates the configuration --------------------------

    KConfig config(CONFIG_FILE);

    config.setGroup("Options");
    config.writeEntry("screen", screen);

    // Updates the size of the window ---------------------

    slotUpdateSize();
}


/******************************************************************************
** Sets the window's location
*******************************/

void    MainWindow::slotSetLocationH(int locationH)
{
    locationH_menu->setItemChecked(this->locationH, false);
    locationH_menu->setItemChecked(locationH, true);
    this->locationH = locationH;

    // Updates the configuration --------------------------

    KConfig config(CONFIG_FILE);

    config.setGroup("Options");
    config.writeEntry("location", locationH);

    // Updates the size of the window ---------------------

    slotUpdateSize();
}


/******************************************************************************
** Recreates the konsole kpart
********************************/

void    MainWindow::slotSessionDestroyed(int id)
{
    if (isShuttingDown)
        return;

    int session_id = (id != -1) ? id : selected_id;

    QWidget* widget = widgets_stack->widget(session_id);

    if (widget == 0L)
        return;

    widgets_stack->removeWidget(widget);
    sessions_stack.remove(session_id);

    if (tabs_bar->removeItem(session_id) == -1)
        slotAddSession();
}


/******************************************************************************
** Toggles the window's state
*******************************/

void    MainWindow::slotToggleState()
{
    static int  state = 1;

    if (timer.isActive())
        return ;

    KWinModule  kwin(this);

    if (state)
    {
        initWindowProps();

        if (screen_policy)
        {
            screen = getMouseScreen();
            slotUpdateSize();
        }

        show();
        if (background_policy)
            move(x(), 0);

        KWin::forceActiveWindow(winId());
        connect(&timer, SIGNAL(timeout()), this, SLOT(slotIncreaseHeight()));
    }
    else
        connect(&timer, SIGNAL(timeout()), this, SLOT(slotDecreaseHeight()));

    timer.start(10, false);
    state = !state;
}


/******************************************************************************
** Increases the window's height
**********************************/

void    MainWindow::slotIncreaseHeight()
{
    mask_height = (step++ * max_height) / steps;

    if (step >= steps)
    {
        step = steps;
        timer.stop();
        disconnect(&timer, SIGNAL(timeout()), 0, 0);

        mask_height = max_height;
    }

    updateWindowMask();
    title_bar->move(0, mask_height);
}


/******************************************************************************
** Decreases the window's height
**********************************/

void    MainWindow::slotDecreaseHeight()
{
    mask_height = (--step * max_height) / steps;

    if (step <= 0)
    {
        step = 0;
        timer.stop();
        disconnect(&timer, SIGNAL(timeout()), 0, 0);

        if (background_policy)
            move(x(), -1);
        hide();
    }

    updateWindowMask();
    title_bar->move(0, mask_height);
}


/******************************************************************************
** Updates the window size
****************************/

void    MainWindow::slotUpdateSize()
{
    int     tmp_height;
    QRect   desk_area;

    // Xinerama aware work area ---------------------------

    desk_area = getDesktopGeometry();
    max_height = (desk_area.height() - 1) * sizeH / 100;

    // Updates the size of the components -----------------

    setGeometry(desk_area.x() + desk_area.width() * locationH * (100 - sizeW) / 10000,
                desk_area.y(), desk_area.width() * sizeW / 100, max_height);

    max_height -= title_bar->height();
    title_bar->setGeometry(0, max_height, width(), title_bar->height());

    tmp_height = max_height;

    if (tabs_policy)
    {
        tmp_height -= tabs_bar->height();
        tabs_bar->setGeometry(margin, tmp_height, width() - 2 * margin, tabs_bar->height());
    }

    widgets_stack->setGeometry(margin, 0, width() - 2 * margin, tmp_height);

    back_widget->setGeometry(0, 0, width(), height());

    // Updates the mask of the window ---------------------

    mask_height = (isVisible()) ? max_height : 0;
    updateWindowMask();
}
