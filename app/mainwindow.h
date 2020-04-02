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
  along with this program. If not, see https://www.gnu.org/licenses/.
*/


#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <config-yakuake.h>

#include <KMainWindow>

#include <QTimer>


class FirstRunDialog;
class SessionStack;
class Skin;
class TabBar;
class Terminal;
class TitleBar;

class KHelpMenu;
class KActionCollection;

#if HAVE_KWAYLAND
namespace KWayland {
    namespace Client {
        class PlasmaShell;
        class PlasmaShellSurface;
    }
}
#endif

class MainWindow : public KMainWindow
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.kde.yakuake")

    friend class TitleBar;

    public:
        explicit MainWindow(QWidget* parent = 0);
        ~MainWindow();

        KActionCollection* actionCollection() { return m_actionCollection; }
        SessionStack* sessionStack() { return m_sessionStack; }

        Skin* skin() { return m_skin; }
        QMenu* menu() { return m_menu; }

        bool useTranslucency() { return m_useTranslucency; }

        void setContextDependentActionsQuiet(bool quiet);


    public Q_SLOTS:
        Q_SCRIPTABLE void toggleWindowState();

        void handleContextDependentAction(QAction* action = 0, int sessionId = -1);
        void handleContextDependentToggleAction(bool checked, QAction* action = 0, int sessionId = -1);
        void handleToggleTerminalKeyboardInput(bool checked);
        void handleToggleTerminalMonitorActivity(bool checked);
        void handleToggleTerminalMonitorSilence(bool checked);
        void handleTerminalActivity(Terminal* terminal);
        void handleTerminalSilence(Terminal* terminal);
        void handleLastTabClosed();


    Q_SIGNALS:
        void windowOpened();
        void windowClosed();


    protected:
        void paintEvent(QPaintEvent*) override;
        void moveEvent(QMoveEvent*) override;
        void changeEvent(QEvent* event) override;
        void closeEvent(QCloseEvent* event) override;
        bool event(QEvent* event) override;

        virtual bool queryClose() override;


    private Q_SLOTS:
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

        void wmActiveWindowChanged();

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

#if HAVE_X11
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
        bool m_isFullscreen;

        KActionCollection* m_actionCollection;
        QList<QAction*> m_contextDependentActions;

        Skin* m_skin;
        TitleBar* m_titleBar;
        TabBar* m_tabBar;
        SessionStack* m_sessionStack;

        QMenu* m_menu;
        KHelpMenu* m_helpMenu;
        QMenu* m_screenMenu;
        QMenu* m_windowWidthMenu;
        QMenu* m_windowHeightMenu;

        FirstRunDialog* m_firstRunDialog;

        QTimer m_animationTimer;
        QTimer m_mousePoller;
        int m_animationFrame;
        int m_animationStepSize;

        bool m_toggleLock;

        bool m_isX11;
        bool m_isWayland;

#if HAVE_KWAYLAND
        void initWayland();
        void initWaylandSurface();
        KWayland::Client::PlasmaShell *m_plasmaShell;
        KWayland::Client::PlasmaShellSurface *m_plasmaShellSurface;
#endif
};

#endif
