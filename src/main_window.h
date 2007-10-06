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


#ifndef MAIN_WINDOW_H
#define MAIN_WINDOW_H


#include "tab_bar.h"
#include "title_bar.h"
#include "dcop_interface.h"
#include "session.h"

#include <qmap.h>
#include <qcolor.h>
#include <qtimer.h>
#include <qlayout.h>
#include <qwidget.h>
#include <qapplication.h>
#include <qwidgetstack.h>

#include <kwin.h>
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


#define CONFIG_FILE "yakuakerc"


class KAboutApplication;
class KAboutKDE;

class MainWindow : public KMainWindow, virtual public DCOPInterface
{
    Q_OBJECT

    public:
        explicit MainWindow(QWidget* parent = 0, const char* name = 0);
        ~MainWindow();

        int selectedSession();
        int selectedTerminal();

        const QString sessionIdList();
        const QString terminalIdList(int session_id);

        int tabPositionForSessionId(int session_id);
        int sessionIdForTabPosition(int position);

        void updateWindowMask();

        void showPopup(const QString & text, int time = 5000);


    public slots:
        void slotAboutToQuit();

        void slotToggleState();

        void slotAddSession();
        void slotAddSessionTwoHorizontal();
        void slotAddSessionTwoVertical();
        void slotAddSessionQuad();
        void slotAddSession(Session::SessionType type);

        void slotRemoveSession();
        void slotRemoveSession(int session_id);

        void slotRemoveTerminal();
        void slotRemoveTerminal(int session_id);
        void slotRemoveTerminal(int session_id, int terminal_id);

        void slotSelectSession(int session_id);
        void slotSelectTabPosition(int position);

        void slotRenameSession(int session_id, const QString& name);
        void slotInteractiveRename();

        const QString slotSessionName();
        const QString slotSessionName(int session_id);

        const QString slotSessionTitle();
        const QString slotSessionTitle(int session_id);
        const QString slotSessionTitle(int session_id, int terminal_id);

        void slotSetSessionTitleText(const QString& title);
        void slotSetSessionTitleText(int session_id, const QString& title);
        void slotSetSessionTitleText(int session_id, int terminal_id, const QString& title);

        void slotPasteClipboard();
        void slotPasteClipboard(int session_id);
        void slotPasteClipboard(int session_id, int terminal_id);

        void slotPasteSelection();
        void slotPasteSelection(int session_id);
        void slotPasteSelection(int session_id, int terminal_id);

        void slotRunCommandInSession(const QString& command);
        void slotRunCommandInSession(int session_id, const QString& command);
        void slotRunCommandInSession(int session_id, int terminal_id, const QString& command);

        void slotSplitHorizontally();
        void slotSplitHorizontally(int session_id);
        void slotSplitHorizontally(int session_id, int terminal_id);

        void slotSplitVertically();
        void slotSplitVertically(int session_id);
        void slotSplitVertically(int session_id, int terminal_id);

        void slotFocusNextSplit();
        void slotFocusPreviousSplit();


    signals:
        void updateBackground();


    protected:
        virtual void windowActivationChange(bool old_active);
        virtual void moveEvent(QMoveEvent* e);
        bool queryClose();


    private:
        void createMenu();
        void updateWidthMenu();
        void updateHeightMenu();
        void updateScreenMenu();
        void createSessionMenu();
        void createTabsBar();
        void createTitleBar();

        void initWindowProps();

        int getMouseScreen();
        QRect getDesktopGeometry();

        bool full_screen;

        /* Animation step. */
        int step;

        /* Focus policy. */
        bool focus_policy;

        /* Maximum height value. */
        int max_height;
        int mask_height;

        /* Application border. */
        int margin;

        /* Interface modification timer. */
        QTimer timer;

        /* Passive popup window. */
        KPassivePopup popup;

        /* Desktop information. */
        KWinModule desk_info;

        /* Main menu. */
        KPopupMenu* menu;
        KPopupMenu* session_menu;
        KPopupMenu* screen_menu;
        KPopupMenu* width_menu;
        KPopupMenu* height_menu;

        /* Global Key shortcut. */
        KGlobalAccel* global_key;

        /* Background widget. */
        QWidget* back_widget;

        TabBar* tab_bar;
        TitleBar* title_bar;

        /* Inner konsole. */
        int selected_id;
        QWidgetStack* widgets_stack;
        QMap<int, Session*> sessions_stack;

        bool is_shutting_down;
        bool background_changed;
        bool use_translucency;

        enum PopupIDs { Focus };

        KAction* remove_tab_action;
        KAction* split_horiz_action;
        KAction* split_vert_action;
        KAction* remove_term_action;
        KToggleFullScreenAction* full_screen_action;

        KDialogBase* first_run_dialog;

        KAboutApplication* about_app;
        KAboutKDE* about_kde;


    private slots:
        void slotHandleRemoveSession(KAction::ActivationReason, Qt::ButtonState);
        void slotHandleHorizontalSplit(KAction::ActivationReason, Qt::ButtonState);
        void slotHandleVerticalSplit(KAction::ActivationReason, Qt::ButtonState);
        void slotHandleRemoveTerminal(KAction::ActivationReason, Qt::ButtonState);

        void slotInitSkin();
        void slotUpdateSize();
        void slotUpdateSize(int new_width, int new_height, int new_location);
        void slotUpdateTitle(const QString& title);

        void slotIncreaseHeight();
        void slotDecreaseHeight();
        void slotSessionDestroyed(int id = -1);

        void slotSetAccessKey();
        void slotSetControlKeys();

        void slotIncreaseSizeW();
        void slotDecreaseSizeW();
        void slotIncreaseSizeH();
        void slotDecreaseSizeH();
        void slotSetFocusPolicy();
        void slotSetFocusPolicy(bool);
        void slotSetWidth(int);
        void slotSetHeight(int);
        void slotSetScreen(int);
        void slotSetFullScreen(bool state);
        void slotUpdateFullScreen();

        void slotUpdateBackgroundState();
        void slotUpdateSettings();
        void slotOpenSettingsDialog();

        void slotOpenFirstRunDialog();
        void slotFirstRunDialogOK();
        void slotFirstRunDialogCancel();

        void slotOpenAboutApp();
        void slotOpenAboutKDE();

        void slotDialogFinished();
};

#endif /* MAIN_WINDOW_H */
