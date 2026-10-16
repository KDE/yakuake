/*
  SPDX-FileCopyrightText: 2008-2014 Eike Hein <hein@kde.org>
  SPDX-FileCopyrightText: 2009 Juan Carlos Torres <carlosdgtorres@gmail.com>

  SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#ifndef TABBAR_H
#define TABBAR_H

#include <QHash>
#include <QList>
#include <QStyle>
#include <QWidget>

class MainWindow;
class Skin;

class QLineEdit;
class QMenu;
class QPushButton;
class QToolButton;
class QLabel;
class QVariantAnimation;
class QWidgetAction;
class QTimer;
class TabBar : public QWidget
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.kde.yakuake")

public:
    enum InteractiveType {
        NonInteractive = 0,
        Interactive = 1,
    };
    explicit TabBar(MainWindow *mainWindow);
    ~TabBar() override;

    void applySkin();
    void refreshTabLayout();

public Q_SLOTS:
    void addTab(int sessionId, const QString &title);
    void removeTab(int sessionId = -1);

    void interactiveRename(int sessionId);

    void selectTab(int sessionId);
    void selectNextTab();
    void selectPreviousTab();

    void moveTabLeft(int sessionId = -1);
    void moveTabRight(int sessionId = -1);

    Q_SCRIPTABLE QString tabTitle(int sessionId);
    Q_SCRIPTABLE void setTabTitle(int sessionId, const QString &newTitle, InteractiveType interactive = Interactive);
    void setTabTitleAutomated(int sessionId, const QString &newTitle);

    Q_SCRIPTABLE int sessionAtTab(int index);
    void showAllTabsMenu();

Q_SIGNALS:
    void newTabRequested();
    void tabSelected(int sessionId);
    void tabClosed(int sessionId);

    void requestTerminalHighlight(int terminalId);
    void requestRemoveTerminalHighlight();
    void tabContextMenuClosed();
    void lastTabClosed();
    void tabTitleEdited(int sessionId, QString title);

protected:
    void resizeEvent(QResizeEvent *) override;
    void paintEvent(QPaintEvent *) override;
    void wheelEvent(QWheelEvent *) override;
    void keyPressEvent(QKeyEvent *) override;
    void mousePressEvent(QMouseEvent *) override;
    void mouseReleaseEvent(QMouseEvent *) override;
    void mouseMoveEvent(QMouseEvent *) override;
    void dragMoveEvent(QDragMoveEvent *) override;
    void dragEnterEvent(QDragEnterEvent *) override;
    void dragLeaveEvent(QDragLeaveEvent *) override;
    void dropEvent(QDropEvent *) override;
    void mouseDoubleClickEvent(QMouseEvent *) override;
    void contextMenuEvent(QContextMenuEvent *) override;
    void leaveEvent(QEvent *) override;
    bool event(QEvent *event) override;

private Q_SLOTS:
    void readySessionMenu();

    void contextMenuActionHovered(QAction *action);

    void closeTabButtonClicked();

    void interactiveRenameDone();

    void scrollTabsLeft();
    void scrollTabsRight();
    void readyAllTabsMenu();
    void allTabsMenuTriggered(QAction *action);
    void filterAllTabsMenu(const QString &text);
    void onScrollAnimationValueChanged(const QVariant &value);
    void onDragAutoScroll();

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

    QFont tabFont(int index) const;
    int tabLeftChromeWidth(int index) const;
    int tabRightChromeWidth(int index) const;
    int tabLockWidth(int index) const;
    int naturalTabWidth(int index) const;
    int minimumTabWidth(int index, int viewportWidth) const;
    int overflowControlsWidth() const;
    int tabsStartX() const;
    int tabsAreaRight() const;
    int totalTabsWidth() const;
    int maxScrollOffset() const;
    int contentLeft(int index) const;
    QRect tabRect(int index) const;
    void recalculateTabLayout(bool revealActiveTab = true);
    void resetScrollOffset();
    void ensureActiveTabVisible(bool animate = true);
    void updateOverflowControls();
    void positionOverflowControls();
    void setScrollOffset(int offset, bool animate);
    void panScrollBy(int delta);
    void paintEdgeFades(QPainter &painter, const QRect &clipRect);
    QIcon overlayIcon(const QString &themeName, QStyle::StandardPixmap fallback) const;

    int drawTab(int x, int y, int index, int tabWidth, QPainter &painter);
    void moveNewTabButton();

    void startDrag(int index);
    void drawDropIndicator(int index, bool disabled = false);
    int dropIndex(const QPoint pos);
    bool isSameTab(const QDropEvent *);

    MainWindow *m_mainWindow = nullptr;
    Skin *m_skin = nullptr;
    QColor m_edgeFadeColor;

    QToolButton *m_newTabButton = nullptr;
    QPushButton *m_closeTabButton = nullptr;
    QToolButton *m_scrollLeftButton = nullptr;
    QToolButton *m_scrollRightButton = nullptr;
    QToolButton *m_allTabsButton = nullptr;

    QMenu *m_tabContextMenu = nullptr;
    QMenu *m_toggleKeyboardInputMenu = nullptr;
    QMenu *m_toggleMonitorActivityMenu = nullptr;
    QMenu *m_toggleMonitorSilenceMenu = nullptr;
    QMenu *m_sessionMenu = nullptr;
    QMenu *m_allTabsMenu = nullptr;
    QLineEdit *m_allTabsFilter = nullptr;
    QWidgetAction *m_allTabsFilterAction = nullptr;

    QLineEdit *m_lineEdit = nullptr;
    int m_renamingSessionId;

    QList<int> m_tabs;
    QHash<int, QString> m_tabTitles;
    QHash<int, bool> m_tabTitlesSetInteractive;
    QList<int> m_tabWidths; // individual fitted widths, content space

    int m_selectedSessionId;
    int m_scrollOffset = 0;
    bool m_overflowing = false;

    QVariantAnimation *m_scrollAnimation = nullptr;
    int m_wheelAccumulator = 0;
    QTimer *m_dragAutoScrollTimer = nullptr;
    int m_dragAutoScrollDirection = 0;

    bool m_mousePressed;
    int m_mousePressedIndex;

    QPoint m_startPos;
    QLabel *m_dropIndicator = nullptr;
    QRect m_dropRect;
};

#endif
