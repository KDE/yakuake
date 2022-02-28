/*
  SPDX-FileCopyrightText: 2008-2014 Eike Hein <hein@kde.org>
  SPDX-FileCopyrightText: 2009 Juan Carlos Torres <carlosdgtorres@gmail.com>

  SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
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
class KStatusNotifierItem;

#if HAVE_KWAYLAND
namespace KWayland
{
namespace Client
{
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
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    KActionCollection *actionCollection()
    {
        return m_actionCollection;
    }
    SessionStack *sessionStack()
    {
        return m_sessionStack;
    }

    Skin *skin()
    {
        return m_skin;
    }
    QMenu *menu()
    {
        return m_menu;
    }

    bool useTranslucency()
    {
        return m_useTranslucency;
    }

    void setContextDependentActionsQuiet(bool quiet);

public Q_SLOTS:
    Q_SCRIPTABLE void toggleWindowState();

    void handleContextDependentAction(QAction *action = nullptr, int sessionId = -1);
    void handleContextDependentToggleAction(bool checked, QAction *action = nullptr, int sessionId = -1);
    void handleToggleTerminalKeyboardInput(bool checked);
    void handleToggleTerminalMonitorActivity(bool checked);
    void handleToggleTerminalMonitorSilence(bool checked);
    void handleTerminalActivity(Terminal *terminal);
    void handleTerminalSilence(Terminal *terminal);
    void handleLastTabClosed();

Q_SIGNALS:
    void windowOpened();
    void windowClosed();

protected:
    void paintEvent(QPaintEvent *) override;
    void moveEvent(QMoveEvent *) override;
    void changeEvent(QEvent *event) override;
    void closeEvent(QCloseEvent *event) override;
    bool focusNextPrevChild(bool) override;

    virtual bool queryClose() override;

private Q_SLOTS:
    void applySettings();
    void applySkin();
    void applyWindowProperties();

    void applyWindowGeometry();
    void setWindowGeometry(int width, int height, int position);

    void updateScreenMenu();
    void setScreen(QAction *action);

    void setWindowWidth(int width);
    void setWindowHeight(int height);
    void setWindowWidth(QAction *action);
    void setWindowHeight(QAction *action);

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

    void handleToggleTitlebar();

    void whatsThis();

    void configureKeys();
    void configureNotifications();
    void configureApp();
    void updateTrayTooltip();

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
    QRect getScreenGeometry();
    QRect getDesktopGeometry();

    // get a better value from plasmashell through dbus in wayland case
    QRect m_availableScreenRect;
    void _toggleWindowState();

    void slideWindow();

    void updateUseTranslucency();
    bool m_useTranslucency;
    bool m_isFullscreen;

    KActionCollection *m_actionCollection;
    QList<QAction *> m_contextDependentActions;

    Skin *m_skin;
    TitleBar *m_titleBar;
    TabBar *m_tabBar;
    SessionStack *m_sessionStack;

    QMenu *m_menu;
    KHelpMenu *m_helpMenu;
    QMenu *m_screenMenu;
    QMenu *m_windowWidthMenu;
    QMenu *m_windowHeightMenu;

    FirstRunDialog *m_firstRunDialog;
    KStatusNotifierItem *m_notifierItem;

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
