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


#ifndef MAINWINDOW_H
#define MAINWINDOW_H


#include "settings.h"

#include <KAction>
#include <KActionCollection>
#include <KGlobalSettings>
#include <KLocalizedString>
#include <KMainWindow>
#include <KWindowSystem>

#include <QTimer>
#include <QWhatsThis>


class FirstRunDialog;
class SessionStack;
class Skin;
class TabBar;
class TitleBar;

class KHelpMenu;
class KMenu;


class MainWindow : public KMainWindow
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.kde.yakuake")

    public:
        explicit MainWindow(QWidget* parent = 0);
        ~MainWindow();

        KActionCollection* actionCollection() { return m_actionCollection; }

        Skin* skin() { return m_skin; }
        KMenu* menu() { return m_menu; }

        bool useTranslucency() { return m_useTranslucency; }


    public slots:
        Q_SCRIPTABLE void toggleWindowState();


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

        void setScreen(QAction* action);
        void setWindowWidth(int width);
        void setWindowHeight(int height);
        void setWindowWidth(QAction* action);
        void setWindowHeight(QAction* action);

        void increaseWindowWidth();
        void decreaseWindowWidth();
        void increaseWindowHeight();
        void decreaseWindowHeight();

        void openWindow();
        void retractWindow();
        void activate();

        void toggleMousePoll(bool poll);
        void pollMouse();

        void setKeepOpen(bool keepOpen);

        void setFullScreen(bool state);
        void updateFullScreen();

        void handleSpecialAction();
    
        void whatsThis();

        void configureKeys();
        void configureApp();

        void showFirstRunDialog();
        void firstRunDialogFinished();
        void firstRunDialogOk();


    private:
        void setupActions();

        void setupMenu();
        void updateScreenMenu();
        void updateWindowSizeMenus();
        void updateWindowHeightMenu();
        void updateWindowWidthMenu();

        void updateMask();

        int getScreen();
        QRect getDesktopGeometry();

        void showStartupPopup();

        void updateUseTranslucency();
        bool m_useTranslucency;

        KActionCollection* m_actionCollection;

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
};

#endif
