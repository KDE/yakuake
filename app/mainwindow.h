/*
  Copyright (C) 2008-2009 by Eike Hein <hein@kde.org>
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


#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <KMainWindow>

#include <QTimer>


class FirstRunDialog;
class SessionStack;
class Skin;
class TabBar;
class Terminal;
class TitleBar;

class KHelpMenu;
class KMenu;
class KAction;
class KActionCollection;


class MainWindow : public KMainWindow
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.kde.yakuake")

    public:
        explicit MainWindow(QWidget* parent = 0);
        ~MainWindow();

        KActionCollection* actionCollection() { return m_actionCollection; }
        SessionStack* sessionStack() { return m_sessionStack; }

        Skin* skin() { return m_skin; }
        KMenu* menu() { return m_menu; }

        bool useTranslucency() { return m_useTranslucency; }

        void setContextDependentActionsQuiet(bool quiet);


    public slots:
        Q_SCRIPTABLE void toggleWindowState();

        void handleContextDependentAction(QAction* action = 0, int sessionId = -1);
        void handleContextDependentToggleAction(bool checked, QAction* action = 0, int sessionId = -1);
        void handleToggleTerminalKeyboardInput(bool checked);
        void handleToggleTerminalMonitorActivity(bool checked);
        void handleToggleTerminalMonitorSilence(bool checked);
        void handleTerminalActivity(Terminal* terminal);
        void handleTerminalSilence(Terminal* terminal);
        void handleLastTabClosed();


    signals:
        void windowOpened();
        void windowClosed();


    protected:
        virtual void paintEvent(QPaintEvent*);
        virtual void moveEvent(QMoveEvent*);
        virtual void changeEvent(QEvent* event);

        virtual bool queryClose();


    private slots:
        void applySettings();
        void applySkin();
        void applyWindowProperties();

        void applyWindowGeometry();
        void setWindowGeometry(int width, int height, int position);

        void updateScreenMenu();
        void setScreen(QAction* action);

        void setWindowWidth(int width);
        void setWindowHeight(int height);
        void setWindowWidth(QAction* action);
        void setWindowHeight(QAction* action);

        void increaseWindowWidth();
        void decreaseWindowWidth();
        void increaseWindowHeight();
        void decreaseWindowHeight();

        void xshapeOpenWindow();
        void xshapeRetractWindow();

        void activate();

        void toggleMousePoll(bool poll);
        void pollMouse();

        void setKeepOpen(bool keepOpen);

        void setFullScreen(bool state);

        void handleSwitchToAction();

        void whatsThis();

        void configureKeys();
        void configureNotifications();
        void configureApp();

        void showFirstRunDialog();
        void firstRunDialogFinished();
        void firstRunDialogOk();


    private:
        void setupActions();

        void setupMenu();

        void updateWindowSizeMenus();
        void updateWindowHeightMenu();
        void updateWindowWidthMenu();

#if defined(Q_WS_X11)
        void kwinAssistToggleWindowState(bool visible);
        void kwinAssistPropCleanup();
        bool m_kwinAssistPropSet;
#endif

        void xshapeToggleWindowState(bool visible);

        void sharedPreOpenWindow();
        void sharedAfterOpenWindow();
        void sharedPreHideWindow();
        void sharedAfterHideWindow();

        void updateMask();

        int getScreen();
        QRect getDesktopGeometry();

        void showStartupPopup();

        void updateUseTranslucency();
        bool m_useTranslucency;

        KActionCollection* m_actionCollection;
        QList<KAction*> m_contextDependentActions;

        Skin* m_skin;
        TitleBar* m_titleBar;
        TabBar* m_tabBar;
        SessionStack* m_sessionStack;

        KMenu* m_menu;
        KHelpMenu* m_helpMenu;
        KMenu* m_screenMenu;
        KMenu* m_windowWidthMenu;
        KMenu* m_windowHeightMenu;

        FirstRunDialog* m_firstRunDialog;

        QTimer m_animationTimer;
        QTimer m_mousePoller;
        int m_animationFrame;
        int m_animationStepSize;

        bool m_listenForActivationChanges;
};

#endif
