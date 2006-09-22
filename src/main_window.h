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

#ifndef MAIN_WINDOW_H
# define MAIN_WINDOW_H

//== INCLUDE REQUIREMENTS ===================================================//

/*
** Qt libraries */
#include <qmap.h>
#include <qcolor.h>
#include <qtimer.h>
#include <qcursor.h>
#include <qlayout.h>
#include <qwidget.h>
#include <qclipboard.h>
#include <qapplication.h>
#include <qwidgetstack.h>

/*
** KDE libraries */
#include <kwin.h>
#include <kdebug.h>
#include <kaction.h>
#include <kconfig.h>
#include <klocale.h>
#include <dcopclient.h>
#include <kkeydialog.h>
#include <kpopupmenu.h>
#include <kwinmodule.h>
#include <kmainwindow.h>
#include <kapplication.h>
#include <kglobalaccel.h>
#include <kpassivepopup.h>

/*
** Local libraries */
#include "tabs_bar.h"
#include "title_bar.h"
#include "shell_session.h"
#include "dcop_interface.h"


//== DEFINE PREPROCESSOR VARIABLES ==========================================//

#define CONFIG_FILE     "yakuakerc"


//== DEFINE CLASS & DATATYPES ===============================================//

/*
** Class 'MainWindow' defines the main window of the application
*****************************************************************/

class MainWindow : public KMainWindow, virtual public DCOPInterface
{
    Q_OBJECT

private:

    //-- PRIVATE ATTRIBUTES ---------------------------------------------//

    /*
    ** Animation steps */
    int             step;
    int             steps;


    /*
    ** Width/Height sizes */
    int             sizeH;
    int             sizeW;

    /*
    ** horizontal location */
    int             locationH;


    /*
    ** Tabs & Focus policies */
    int             tabs_policy;
    int             focus_policy;
    int             background_policy;


    /*
    ** Maximum height value */
    int             max_height;
    int             mask_height;


    /*
    ** Xinerama screen */
    int             screen;


    /*
    ** Mouse position usage */
    bool            screen_policy;


    /*
    ** Application skin */
    QString         skin;


    /*
    ** Application border */
    int             margin;


    /*
    ** Interface modification timer */
    QTimer          timer;


    /*
    ** Passive popup window */
    KPassivePopup   popup;


    /*
    ** Desktop information */
    KWinModule      desk_info;


    /*
    ** Keyboard actions */
    KAction *       action_new;
    KAction *       action_del;
    KAction *       action_next;
    KAction *       action_prev;
    KAction *       action_paste;


    /*
    ** Configuration menus */
    KPopupMenu *    menu;
    KPopupMenu *    sizeH_menu;
    KPopupMenu *    sizeW_menu;
    KPopupMenu *    speed_menu;
    KPopupMenu *    screen_menu;
    KPopupMenu *    locationH_menu;


    /*
    ** Global Key shortcut */
    KGlobalAccel *  global_key;


    /*
    ** Background widget */
    QWidget *       back_widget;


    /*
    ** Tabs bar of the application */
    TabsBar *       tabs_bar;


    /*
    ** Title bar of the application */
    TitleBar *      title_bar;


    /*
    ** Inner konsole of the application */
    int                     selected_id;
    QWidgetStack *          widgets_stack;
    QMap<int, ShellSession*> sessions_stack;


    bool isShuttingDown;

    //-- PRIVATE METHODS ------------------------------------------------//

    void    createMenu();
    int     createSession();
    void    createTabsBar();
    void    createTitleBar();

    void    initWindowProps();

    int     getMouseScreen();
    QRect   getDesktopGeometry();



private slots:

    //-- PRIVATE SLOTS --------------------------------------------------//

    void    slotUpdateSize();
    void    slotUpdateTitle();

    void    slotIncreaseHeight();
    void    slotDecreaseHeight();
    void    slotSessionDestroyed();

    void    slotSetAccessKey();
    void    slotSetControlKeys();

    void    slotSetSizeW(int);
    void    slotSetSizeH(int);
    void    slotSetSpeed(int);
    void    slotSetScreen(int);
    void    slotSetLocationH(int);
    void    slotSetTabsPolicy();
    void    slotSetTabsPolicy(bool);
    void    slotSetFocusPolicy();
    void    slotSetFocusPolicy(bool);
    void    slotSetBackgroundPolicy();
    void    slotSetBackgroundPolicy(bool);



protected:

    //-- PROTECTED METHODS ----------------------------------------------//

    /*
    ** Retract the window when activation changes */
    virtual void    windowActivationChange(bool old_active);
    bool queryClose();



public:

    //-- CONSTRUCTORS AND DESTRUCTORS -----------------------------------//

    MainWindow(QWidget * parent = 0, const char * name = 0);
    ~MainWindow();


    //-- PUBLIC METHODS -------------------------------------------------//

    /*
    ** Gets the selected id */
    int    selectedSession();


    /*
    ** Updates the window mask */
    void    updateWindowMask();


    /*
    ** Shows a passive popup with the given text */
    void    showPopup(const QString & text, int time = 5000);


    /*
    ** Gets the tabs policy */
    bool    getTabsPolicy() { return tabs_policy; }


public slots:

    //-- PUBLIC SLOTS ---------------------------------------------------//

    void    slotAboutToQuit();
    void    slotAddSession();
    void    slotRemoveSession();
    void    slotSelectSession(int id);

    void    slotToggleState();

    void    slotPasteClipboard();

    void    slotRenameSession(int id, const QString & name);
    void    slotInteractiveRename();
    void    slotSetSessionTitleText(int id, const QString & name);
    void    slotRunCommandInSession(int id, const QString & value);
};

#endif /* MAIN_WINDOW_H */
