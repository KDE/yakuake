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
  along with this program. If not, see http://www.gnu.org/licenses/.
*/


#ifndef TABBAR_H
#define TABBAR_H

#include <QList>
#include <QHash>
#include <QWidget>


class MainWindow;
class Skin;

class QLineEdit;
class QMenu;
class QPushButton;
class QToolButton;
class QLabel;


class TabBar : public QWidget
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.kde.yakuake")

    public:
        explicit TabBar(MainWindow* mainWindow);
        ~TabBar();

        void applySkin();


    public Q_SLOTS:
        void addTab(int sessionId, const QString& title);
        void removeTab(int sessionId = -1);

        void interactiveRename(int sessionId);

        void selectTab(int sessionId);
        void selectNextTab();
        void selectPreviousTab();

        void moveTabLeft(int sessionId = -1);
        void moveTabRight(int sessionId = -1);

        Q_SCRIPTABLE QString tabTitle(int sessionId);
        Q_SCRIPTABLE void setTabTitle(int sessionId, const QString& newTitle);

        Q_SCRIPTABLE int sessionAtTab(int index);


    Q_SIGNALS:
        void newTabRequested();
        void tabSelected(int sessionId);
        void tabClosed(int sessionId);

        void requestTerminalHighlight(int terminalId);
        void requestRemoveTerminalHighlight();
        void tabContextMenuClosed();
        void lastTabClosed();


    protected:
        void resizeEvent(QResizeEvent*) override;
        void paintEvent(QPaintEvent*) override;
        void wheelEvent(QWheelEvent*) override;
        void keyPressEvent(QKeyEvent*) override;
        void mousePressEvent(QMouseEvent*) override;
        void mouseReleaseEvent(QMouseEvent*) override;
        void mouseMoveEvent(QMouseEvent*) override;
        void dragMoveEvent(QDragMoveEvent*) override;
        void dragEnterEvent(QDragEnterEvent*) override;
        void dragLeaveEvent(QDragLeaveEvent*) override;
        void dropEvent(QDropEvent*) override;
        void mouseDoubleClickEvent(QMouseEvent*) override;
        void contextMenuEvent(QContextMenuEvent*) override;
        void leaveEvent(QEvent*) override;

        void setTabTitleInteractive(int sessionId, const QString& newTitle);


    private Q_SLOTS:
        void readySessionMenu();

        void contextMenuActionHovered(QAction* action);

        void closeTabButtonClicked();

        void interactiveRenameDone();


    private:
        QString standardTabTitle();
        QString makeTabTitle(int number);
        int tabAt(int x);

        void readyTabContextMenu();

        void updateMoveActions(int index);
        void updateToggleActions(int sessionId);
        void updateToggleKeyboardInputMenu(int sessionId = -1);
        void updateToggleMonitorSilenceMenu(int sessionId = -1);
        void updateToggleMonitorActivityMenu(int sessionId = -1);

        int drawButton(int x, int y, int index, QPainter& painter);

        void startDrag(int index);
        void drawDropIndicator(int index, bool disabled = false);
        int dropIndex(const QPoint pos);
        bool isSameTab(const QDropEvent*);

        MainWindow* m_mainWindow;
        Skin* m_skin;

        QToolButton* m_newTabButton;
        QPushButton* m_closeTabButton;

        QMenu* m_tabContextMenu;
        QMenu* m_toggleKeyboardInputMenu;
        QMenu* m_toggleMonitorActivityMenu;
        QMenu* m_toggleMonitorSilenceMenu;
        QMenu* m_sessionMenu;

        QLineEdit* m_lineEdit;
        int m_renamingSessionId;

        QList<int> m_tabs;
        QHash<int, QString> m_tabTitles;
        QHash<int, bool> m_tabTitlesSetInteractive;
        QList<int> m_tabWidths;

        int m_selectedSessionId;

        int m_mousePressed;
        int m_mousePressedIndex;

        QPoint m_startPos;
        QLabel* m_dropIndicator;
        QRect m_dropRect;
};

#endif
