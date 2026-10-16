/*
  SPDX-FileCopyrightText: 2008-2014 Eike Hein <hein@kde.org>
  SPDX-FileCopyrightText: 2009 Juan Carlos Torres <carlosdgtorres@gmail.com>

  SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#include "tabbar.h"
#include "mainwindow.h"
#include "session.h"
#include "sessionstack.h"
#include "settings.h"
#include "skin.h"

#include <KActionCollection>
#include <KLocalizedString>

#include <QApplication>
#include <QDBusConnection>
#include <QEasingCurve>
#include <QEvent>
#include <QFontDatabase>
#include <QIcon>
#include <QLineEdit>
#include <QLinearGradient>
#include <QMenu>
#include <QPainter>
#include <QPushButton>
#include <QStyle>
#include <QTimer>
#include <QToolButton>
#include <QToolTip>
#include <QVariantAnimation>
#include <QWhatsThis>
#include <QWheelEvent>
#include <QWidgetAction>

#include <QDrag>
#include <QLabel>
#include <QMimeData>

namespace
{
// Tab titles stay raw in storage; control characters such as newlines and tabs
// are painted and listed as plain spaces so a title always occupies one line.
QString singleLineTitle(const QString &title)
{
    QString result = title;
    for (QChar &c : result) {
        if (c.category() == QChar::Other_Control || c.category() == QChar::Separator_Line || c.category() == QChar::Separator_Paragraph)
            c = u' ';
    }
    return result;
}

constexpr int edgeFadeWidth = 28;
constexpr int dragAutoScrollEdgeZone = 28;
constexpr int dragAutoScrollStep = 12;
constexpr int dragAutoScrollInterval = 16;
constexpr int minimumTabTextChars = 14;
}

TabBar::TabBar(MainWindow *mainWindow)
    : QWidget(mainWindow)
{
    QDBusConnection::sessionBus().registerObject(QStringLiteral("/yakuake/tabs"), this, QDBusConnection::ExportScriptableSlots);

    setWhatsThis(xi18nc("@info:whatsthis",
                        "<title>Tab Bar</title>"
                        "<para>The tab bar allows you to switch between sessions. You can double-click a tab to edit its label.</para>"));

    m_selectedSessionId = -1;
    m_renamingSessionId = -1;
    m_scrollOffset = 0;
    m_overflowing = false;

    m_mousePressed = false;
    m_mousePressedIndex = -1;

    m_mainWindow = mainWindow;

    m_skin = mainWindow->skin();
    connect(m_skin, SIGNAL(iconChanged()), this, SLOT(repaint()));

    m_tabContextMenu = new QMenu(this);
    connect(m_tabContextMenu, SIGNAL(hovered(QAction *)), this, SLOT(contextMenuActionHovered(QAction *)));

    m_toggleKeyboardInputMenu = new QMenu(xi18nc("@title:menu", "Disable Keyboard Input"), this);
    m_toggleMonitorActivityMenu = new QMenu(xi18nc("@title:menu", "Monitor for Activity"), this);
    m_toggleMonitorSilenceMenu = new QMenu(xi18nc("@title:menu", "Monitor for Silence"), this);

    m_sessionMenu = new QMenu(this);
    connect(m_sessionMenu, SIGNAL(aboutToShow()), this, SLOT(readySessionMenu()));

    m_newTabButton = new QToolButton(this);
    m_newTabButton->setFocusPolicy(Qt::NoFocus);
    m_newTabButton->setMenu(m_sessionMenu);
    m_newTabButton->setPopupMode(QToolButton::DelayedPopup);
    m_newTabButton->setToolTip(xi18nc("@info:tooltip", "New Session"));
    m_newTabButton->setWhatsThis(xi18nc("@info:whatsthis", "Adds a new session. Press and hold to select session type from menu."));
    connect(m_newTabButton, SIGNAL(clicked()), this, SIGNAL(newTabRequested()));

    m_closeTabButton = new QPushButton(this);
    m_closeTabButton->setFocusPolicy(Qt::NoFocus);
    m_closeTabButton->setToolTip(xi18nc("@info:tooltip", "Close Session"));
    m_closeTabButton->setWhatsThis(xi18nc("@info:whatsthis", "Closes the active session."));
    connect(m_closeTabButton, SIGNAL(clicked()), this, SLOT(closeTabButtonClicked()));

    m_scrollLeftButton = new QToolButton(this);
    m_scrollLeftButton->setFocusPolicy(Qt::NoFocus);
    m_scrollLeftButton->setAutoRepeat(true);
    m_scrollLeftButton->setToolTip(xi18nc("@info:tooltip", "Scroll tabs left"));
    m_scrollLeftButton->setIcon(overlayIcon(QStringLiteral("go-previous"), QStyle::SP_ArrowLeft));
    m_scrollLeftButton->hide();
    connect(m_scrollLeftButton, &QToolButton::clicked, this, &TabBar::scrollTabsLeft);

    m_scrollRightButton = new QToolButton(this);
    m_scrollRightButton->setFocusPolicy(Qt::NoFocus);
    m_scrollRightButton->setAutoRepeat(true);
    m_scrollRightButton->setToolTip(xi18nc("@info:tooltip", "Scroll tabs right"));
    m_scrollRightButton->setIcon(overlayIcon(QStringLiteral("go-next"), QStyle::SP_ArrowRight));
    m_scrollRightButton->hide();
    connect(m_scrollRightButton, &QToolButton::clicked, this, &TabBar::scrollTabsRight);

    m_allTabsMenu = new QMenu(this);
    connect(m_allTabsMenu, &QMenu::aboutToShow, this, &TabBar::readyAllTabsMenu);
    connect(m_allTabsMenu, &QMenu::triggered, this, &TabBar::allTabsMenuTriggered);

    m_allTabsFilter = new QLineEdit;
    m_allTabsFilter->setPlaceholderText(i18nc("@info:placeholder", "Filter tabs…"));
    m_allTabsFilter->setClearButtonEnabled(true);
    connect(m_allTabsFilter, &QLineEdit::textChanged, this, &TabBar::filterAllTabsMenu);
    connect(m_allTabsFilter, &QLineEdit::returnPressed, this, [this]() {
        // Activate the entry highlighted with the arrow keys, else the first visible tab.
        QAction *action = m_allTabsMenu->activeAction();
        if (!action || action == m_allTabsFilterAction || !action->isVisible() || !action->data().isValid()) {
            action = nullptr;
            const auto actions = m_allTabsMenu->actions();
            for (QAction *candidate : actions) {
                if (candidate != m_allTabsFilterAction && candidate->isVisible() && candidate->isEnabled() && candidate->data().isValid()) {
                    action = candidate;
                    break;
                }
            }
        }
        if (action) {
            Q_EMIT m_allTabsMenu->triggered(action);
            m_allTabsMenu->hide();
        }
    });

    // Parent to TabBar so QMenu::clear() cannot destroy the filter widget/action.
    m_allTabsFilterAction = new QWidgetAction(this);
    m_allTabsFilterAction->setDefaultWidget(m_allTabsFilter);

    m_allTabsButton = new QToolButton(this);
    m_allTabsButton->setFocusPolicy(Qt::NoFocus);
    m_allTabsButton->setToolTip(xi18nc("@info:tooltip", "All tabs"));
    m_allTabsButton->setIcon(overlayIcon(QStringLiteral("view-list-symbolic"), QStyle::SP_FileDialogDetailedView));
    m_allTabsButton->setMenu(m_allTabsMenu);
    m_allTabsButton->setPopupMode(QToolButton::InstantPopup);
    m_allTabsButton->hide();

    m_scrollAnimation = new QVariantAnimation(this);
    m_scrollAnimation->setEasingCurve(QEasingCurve::OutCubic);
    connect(m_scrollAnimation, &QVariantAnimation::valueChanged, this, &TabBar::onScrollAnimationValueChanged);

    m_dragAutoScrollTimer = new QTimer(this);
    m_dragAutoScrollTimer->setInterval(dragAutoScrollInterval);
    connect(m_dragAutoScrollTimer, &QTimer::timeout, this, &TabBar::onDragAutoScroll);

    m_lineEdit = new QLineEdit(this);
    m_lineEdit->setFrame(false);
    m_lineEdit->setClearButtonEnabled(false);
    m_lineEdit->setAlignment(Qt::AlignHCenter);
    m_lineEdit->hide();

    connect(m_lineEdit, SIGNAL(editingFinished()), m_lineEdit, SLOT(hide()));
    connect(m_lineEdit, SIGNAL(returnPressed()), this, SLOT(interactiveRenameDone()));

    setAcceptDrops(true);
    setAttribute(Qt::WA_OpaquePaintEvent, false);
}

TabBar::~TabBar()
{
}

void TabBar::applySkin()
{
    resize(width(), m_skin->tabBarBackgroundImage().height());
    const QImage tabBackground = m_skin->tabBarUnselectedBackgroundImage().toImage();
    m_edgeFadeColor = tabBackground.isNull() ? palette().window().color() : tabBackground.pixelColor(0, tabBackground.height() / 2);

    m_newTabButton->setStyleSheet(m_skin->tabBarNewTabButtonStyleSheet());
    m_closeTabButton->setStyleSheet(m_skin->tabBarCloseTabButtonStyleSheet());

    const int buttonSide = qMax(16, m_skin->tabBarBackgroundImage().height());
    const QSize buttonSize(buttonSide, buttonSide);
    m_scrollLeftButton->setFixedSize(buttonSize);
    m_scrollRightButton->setFixedSize(buttonSize);
    m_allTabsButton->setFixedSize(buttonSize);
    m_scrollLeftButton->setIconSize(QSize(buttonSide - 6, buttonSide - 6));
    m_scrollRightButton->setIconSize(QSize(buttonSide - 6, buttonSide - 6));
    m_allTabsButton->setIconSize(QSize(buttonSide - 6, buttonSide - 6));

    m_closeTabButton->move(width() - m_skin->tabBarCloseTabButtonPosition().x(), m_skin->tabBarCloseTabButtonPosition().y());
    recalculateTabLayout();
    moveNewTabButton();
    positionOverflowControls();
    repaint();
}

void TabBar::readyTabContextMenu()
{
    if (m_tabContextMenu->isEmpty()) {
        m_tabContextMenu->addAction(m_mainWindow->actionCollection()->action(QStringLiteral("split-left-right")));
        m_tabContextMenu->addAction(m_mainWindow->actionCollection()->action(QStringLiteral("split-top-bottom")));
        m_tabContextMenu->addSeparator();
        m_tabContextMenu->addAction(m_mainWindow->actionCollection()->action(QStringLiteral("edit-profile")));
        m_tabContextMenu->addAction(m_mainWindow->actionCollection()->action(QStringLiteral("rename-session")));
        m_tabContextMenu->addAction(m_mainWindow->actionCollection()->action(QStringLiteral("toggle-session-prevent-closing")));
        m_tabContextMenu->addMenu(m_toggleKeyboardInputMenu);
        m_tabContextMenu->addMenu(m_toggleMonitorActivityMenu);
        m_tabContextMenu->addMenu(m_toggleMonitorSilenceMenu);
        m_tabContextMenu->addSeparator();
        m_tabContextMenu->addAction(m_mainWindow->actionCollection()->action(QStringLiteral("move-session-left")));
        m_tabContextMenu->addAction(m_mainWindow->actionCollection()->action(QStringLiteral("move-session-right")));
        m_tabContextMenu->addSeparator();
        m_tabContextMenu->addAction(m_mainWindow->actionCollection()->action(QStringLiteral("close-active-terminal")));
        m_tabContextMenu->addAction(m_mainWindow->actionCollection()->action(QStringLiteral("close-session")));
    }
}

void TabBar::readySessionMenu()
{
    if (m_sessionMenu->isEmpty()) {
        m_sessionMenu->addAction(m_mainWindow->actionCollection()->action(QStringLiteral("new-session")));
        m_sessionMenu->addSeparator();
        m_sessionMenu->addAction(m_mainWindow->actionCollection()->action(QStringLiteral("new-session-two-horizontal")));
        m_sessionMenu->addAction(m_mainWindow->actionCollection()->action(QStringLiteral("new-session-two-vertical")));
        m_sessionMenu->addAction(m_mainWindow->actionCollection()->action(QStringLiteral("new-session-quad")));
    }
}

void TabBar::updateMoveActions(int index)
{
    if (index == -1)
        return;

    m_mainWindow->actionCollection()->action(QStringLiteral("move-session-left"))->setEnabled(false);
    m_mainWindow->actionCollection()->action(QStringLiteral("move-session-right"))->setEnabled(false);

    if (index != m_tabs.indexOf(m_tabs.first()))
        m_mainWindow->actionCollection()->action(QStringLiteral("move-session-left"))->setEnabled(true);

    if (index != m_tabs.indexOf(m_tabs.last()))
        m_mainWindow->actionCollection()->action(QStringLiteral("move-session-right"))->setEnabled(true);
}

void TabBar::updateToggleActions(int sessionId)
{
    if (sessionId == -1)
        return;

    KActionCollection *actionCollection = m_mainWindow->actionCollection();
    SessionStack *sessionStack = m_mainWindow->sessionStack();

    QAction *toggleAction = actionCollection->action(QStringLiteral("toggle-session-prevent-closing"));
    toggleAction->setChecked(!sessionStack->isSessionClosable(sessionId));

    toggleAction = actionCollection->action(QStringLiteral("toggle-session-keyboard-input"));
    toggleAction->setChecked(!sessionStack->hasTerminalsWithKeyboardInputEnabled(sessionId));

    toggleAction = actionCollection->action(QStringLiteral("toggle-session-monitor-activity"));
    toggleAction->setChecked(!sessionStack->hasTerminalsWithMonitorActivityDisabled(sessionId));

    toggleAction = actionCollection->action(QStringLiteral("toggle-session-monitor-silence"));
    toggleAction->setChecked(!sessionStack->hasTerminalsWithMonitorSilenceDisabled(sessionId));
}

void TabBar::updateToggleKeyboardInputMenu(int sessionId)
{
    if (!m_tabs.contains(sessionId))
        return;

    QAction *toggleKeyboardInputAction = m_mainWindow->actionCollection()->action(QStringLiteral("toggle-session-keyboard-input"));
    QAction *anchor = m_toggleKeyboardInputMenu->menuAction();

    SessionStack *sessionStack = m_mainWindow->sessionStack();

    QStringList terminalIds = sessionStack->terminalIdsForSessionId(sessionId).split(QLatin1Char(','), Qt::SkipEmptyParts);

    m_toggleKeyboardInputMenu->clear();

    if (terminalIds.count() <= 1) {
        toggleKeyboardInputAction->setText(xi18nc("@action", "Disable Keyboard Input"));
        m_tabContextMenu->insertAction(anchor, toggleKeyboardInputAction);
        m_toggleKeyboardInputMenu->menuAction()->setVisible(false);
    } else {
        toggleKeyboardInputAction->setText(xi18nc("@action", "For This Session"));
        m_toggleKeyboardInputMenu->menuAction()->setVisible(true);

        m_tabContextMenu->removeAction(toggleKeyboardInputAction);
        m_toggleKeyboardInputMenu->addAction(toggleKeyboardInputAction);

        m_toggleKeyboardInputMenu->addSeparator();

        int count = 0;

        QStringListIterator i(terminalIds);

        while (i.hasNext()) {
            int terminalId = i.next().toInt();

            ++count;

            QAction *action = m_toggleKeyboardInputMenu->addAction(xi18nc("@action", "For Terminal %1", count));
            action->setCheckable(true);
            action->setChecked(!sessionStack->isTerminalKeyboardInputEnabled(terminalId));
            action->setData(terminalId);
            connect(action, SIGNAL(triggered(bool)), m_mainWindow, SLOT(handleToggleTerminalKeyboardInput(bool)));
        }
    }
}

void TabBar::updateToggleMonitorActivityMenu(int sessionId)
{
    if (!m_tabs.contains(sessionId))
        return;

    QAction *toggleMonitorActivityAction = m_mainWindow->actionCollection()->action(QStringLiteral("toggle-session-monitor-activity"));
    QAction *anchor = m_toggleMonitorActivityMenu->menuAction();

    SessionStack *sessionStack = m_mainWindow->sessionStack();

    QStringList terminalIds = sessionStack->terminalIdsForSessionId(sessionId).split(QLatin1Char(','), Qt::SkipEmptyParts);

    m_toggleMonitorActivityMenu->clear();

    if (terminalIds.count() <= 1) {
        toggleMonitorActivityAction->setText(xi18nc("@action", "Monitor for Activity"));
        m_tabContextMenu->insertAction(anchor, toggleMonitorActivityAction);
        m_toggleMonitorActivityMenu->menuAction()->setVisible(false);
    } else {
        toggleMonitorActivityAction->setText(xi18nc("@action", "In This Session"));
        m_toggleMonitorActivityMenu->menuAction()->setVisible(true);

        m_tabContextMenu->removeAction(toggleMonitorActivityAction);
        m_toggleMonitorActivityMenu->addAction(toggleMonitorActivityAction);

        m_toggleMonitorActivityMenu->addSeparator();

        int count = 0;

        QStringListIterator i(terminalIds);

        while (i.hasNext()) {
            int terminalId = i.next().toInt();

            ++count;

            QAction *action = m_toggleMonitorActivityMenu->addAction(xi18nc("@action", "In Terminal %1", count));
            action->setCheckable(true);
            action->setChecked(sessionStack->isTerminalMonitorActivityEnabled(terminalId));
            action->setData(terminalId);
            connect(action, SIGNAL(triggered(bool)), m_mainWindow, SLOT(handleToggleTerminalMonitorActivity(bool)));
        }
    }
}

void TabBar::updateToggleMonitorSilenceMenu(int sessionId)
{
    if (!m_tabs.contains(sessionId))
        return;

    QAction *toggleMonitorSilenceAction = m_mainWindow->actionCollection()->action(QStringLiteral("toggle-session-monitor-silence"));
    QAction *anchor = m_toggleMonitorSilenceMenu->menuAction();

    SessionStack *sessionStack = m_mainWindow->sessionStack();

    QStringList terminalIds = sessionStack->terminalIdsForSessionId(sessionId).split(QLatin1Char(','), Qt::SkipEmptyParts);

    m_toggleMonitorSilenceMenu->clear();

    if (terminalIds.count() <= 1) {
        toggleMonitorSilenceAction->setText(xi18nc("@action", "Monitor for Silence"));
        m_tabContextMenu->insertAction(anchor, toggleMonitorSilenceAction);
        m_toggleMonitorSilenceMenu->menuAction()->setVisible(false);
    } else {
        toggleMonitorSilenceAction->setText(xi18nc("@action", "In This Session"));
        m_toggleMonitorSilenceMenu->menuAction()->setVisible(true);

        m_tabContextMenu->removeAction(toggleMonitorSilenceAction);
        m_toggleMonitorSilenceMenu->addAction(toggleMonitorSilenceAction);

        m_toggleMonitorSilenceMenu->addSeparator();

        int count = 0;

        QStringListIterator i(terminalIds);

        while (i.hasNext()) {
            int terminalId = i.next().toInt();

            ++count;

            QAction *action = m_toggleMonitorSilenceMenu->addAction(xi18nc("@action", "In Terminal %1", count));
            action->setCheckable(true);
            action->setChecked(sessionStack->isTerminalMonitorSilenceEnabled(terminalId));
            action->setData(terminalId);
            connect(action, SIGNAL(triggered(bool)), m_mainWindow, SLOT(handleToggleTerminalMonitorSilence(bool)));
        }
    }
}

void TabBar::contextMenuActionHovered(QAction *action)
{
    bool ok = false;

    if (!action->data().isNull()) {
        int terminalId = action->data().toInt(&ok);

        if (ok)
            Q_EMIT requestTerminalHighlight(terminalId);
    } else if (!ok)
        Q_EMIT requestRemoveTerminalHighlight();
}

void TabBar::contextMenuEvent(QContextMenuEvent *event)
{
    if (event->x() < 0)
        return;

    int index = tabAt(event->x());

    if (index == -1)
        m_sessionMenu->exec(QCursor::pos());
    else {
        readyTabContextMenu();

        updateMoveActions(index);

        int sessionId = sessionAtTab(index);
        updateToggleActions(sessionId);
        updateToggleKeyboardInputMenu(sessionId);
        updateToggleMonitorActivityMenu(sessionId);
        updateToggleMonitorSilenceMenu(sessionId);

        m_mainWindow->setContextDependentActionsQuiet(true);

        QAction *action = m_tabContextMenu->exec(QCursor::pos());

        Q_EMIT tabContextMenuClosed();

        if (action) {
            if (action->isCheckable())
                m_mainWindow->handleContextDependentToggleAction(action->isChecked(), action, sessionId);
            else
                m_mainWindow->handleContextDependentAction(action, sessionId);
        }

        m_mainWindow->setContextDependentActionsQuiet(false);

        updateMoveActions(m_tabs.indexOf(m_selectedSessionId));
        updateToggleActions(m_selectedSessionId);
        updateToggleKeyboardInputMenu(m_selectedSessionId);
        updateToggleMonitorActivityMenu(m_selectedSessionId);
        updateToggleMonitorSilenceMenu(m_selectedSessionId);
    }

    QWidget::contextMenuEvent(event);
}

void TabBar::resizeEvent(QResizeEvent *event)
{
    m_closeTabButton->move(width() - m_skin->tabBarCloseTabButtonPosition().x(), m_skin->tabBarCloseTabButtonPosition().y());
    recalculateTabLayout();
    moveNewTabButton();
    positionOverflowControls();
    QWidget::resizeEvent(event);
}

void TabBar::moveNewTabButton()
{
    int newTabButtonX = m_skin->tabBarNewTabButtonPosition().x();
    if (m_skin->tabBarNewTabButtonIsAtEndOfTabs() && !m_tabWidths.isEmpty()) {
        const int contentEnd = totalTabsWidth();
        const int screenEnd = tabsStartX() + contentEnd - m_scrollOffset;
        const int viewportRight = tabsAreaRight();
        newTabButtonX = qBound(tabsStartX(), screenEnd, viewportRight);
    }
    m_newTabButton->move(newTabButtonX, m_skin->tabBarNewTabButtonPosition().y());
}

QFont TabBar::tabFont(int index) const
{
    QFont font = QFontDatabase::systemFont(QFontDatabase::GeneralFont);
    const int sessionId = m_tabs.at(index);
    const bool selected = (sessionId == m_selectedSessionId);
    font.setBold(selected && m_skin->tabBarSelectedTextBold());
    return font;
}

int TabBar::tabLeftChromeWidth(int index) const
{
    const int sessionId = m_tabs.at(index);
    const bool selected = (sessionId == m_selectedSessionId);

    if (selected)
        return m_skin->tabBarSelectedLeftCornerImage().width();
    if (!m_skin->tabBarUnselectedLeftCornerImage().isNull())
        return m_skin->tabBarUnselectedLeftCornerImage().width();
    if (index != m_tabs.indexOf(m_selectedSessionId) + 1)
        return m_skin->tabBarSeparatorImage().width();
    return 0;
}

int TabBar::tabRightChromeWidth(int index) const
{
    const int sessionId = m_tabs.at(index);
    const bool selected = (sessionId == m_selectedSessionId);

    if (selected)
        return m_skin->tabBarSelectedRightCornerImage().width();
    if (!m_skin->tabBarUnselectedRightCornerImage().isNull())
        return m_skin->tabBarUnselectedRightCornerImage().width();
    if (index != m_tabs.indexOf(m_selectedSessionId) - 1)
        return m_skin->tabBarSeparatorImage().width();
    return 0;
}

int TabBar::tabLockWidth(int index) const
{
    const int sessionId = m_tabs.at(index);
    if (m_mainWindow->sessionStack()->isSessionClosable(sessionId))
        return 0;
    return m_skin->tabBarPreventClosingImagePosition().x() + m_skin->tabBarPreventClosingImage().width();
}

int TabBar::naturalTabWidth(int index) const
{
    const int sessionId = m_tabs.at(index);
    const QFontMetrics fontMetrics(tabFont(index));
    const int textWidth = fontMetrics.horizontalAdvance(singleLineTitle(m_tabTitles.value(sessionId))) + 10;
    return tabLeftChromeWidth(index) + tabLockWidth(index) + textWidth + tabRightChromeWidth(index);
}

int TabBar::minimumTabWidth(int index, int viewportWidth) const
{
    const QFontMetrics fontMetrics(tabFont(index));
    const int textFloor = qRound(static_cast<qreal>(fontMetrics.averageCharWidth()) * minimumTabTextChars) + 10;
    int minWidth = tabLeftChromeWidth(index) + tabLockWidth(index) + textFloor + tabRightChromeWidth(index);
    minWidth = qMin(minWidth, naturalTabWidth(index));
    // Active tab must always be able to fit fully inside the viewport.
    if (m_tabs.at(index) == m_selectedSessionId && viewportWidth > 0)
        minWidth = qMin(minWidth, viewportWidth);
    return qMax(minWidth, 1);
}

int TabBar::overflowControlsWidth() const
{
    return m_scrollLeftButton->width() + m_scrollRightButton->width() + m_allTabsButton->width();
}

int TabBar::tabsStartX() const
{
    return m_skin->tabBarPosition().x();
}

int TabBar::tabsAreaRight() const
{
    int right = m_closeTabButton->x();
    if (m_overflowing)
        right -= overflowControlsWidth();
    if (m_skin->tabBarNewTabButtonIsAtEndOfTabs())
        right -= m_newTabButton->width();
    return right;
}

int TabBar::totalTabsWidth() const
{
    int total = 0;
    for (int width : m_tabWidths)
        total += width;
    return total;
}

int TabBar::maxScrollOffset() const
{
    return qMax(0, totalTabsWidth() - qMax(0, tabsAreaRight() - tabsStartX()));
}

int TabBar::contentLeft(int index) const
{
    int left = 0;
    for (int i = 0; i < index && i < m_tabWidths.count(); ++i)
        left += m_tabWidths.at(i);
    return left;
}

QRect TabBar::tabRect(int index) const
{
    if (index < 0 || index >= m_tabWidths.count())
        return QRect();

    const int x = tabsStartX() + contentLeft(index) - m_scrollOffset;
    return QRect(x, m_skin->tabBarPosition().y(), m_tabWidths.at(index), height() - m_skin->tabBarPosition().y());
}

QIcon TabBar::overlayIcon(const QString &themeName, QStyle::StandardPixmap fallback) const
{
    const qreal dpr = devicePixelRatioF();
    const int side = qMax(16, height() > 0 ? height() - 4 : 18);
    const QSize pixSize(qRound(side * dpr), qRound(side * dpr));

    QIcon themeIcon = QIcon::fromTheme(themeName);
    if (themeIcon.isNull())
        themeIcon = style()->standardIcon(fallback);

    QPixmap pixmap = themeIcon.pixmap(pixSize);
    pixmap.setDevicePixelRatio(dpr);
    QIcon icon;
    icon.addPixmap(pixmap);
    return icon;
}

void TabBar::positionOverflowControls()
{
    if (!m_overflowing) {
        m_scrollLeftButton->hide();
        m_scrollRightButton->hide();
        m_allTabsButton->hide();
        return;
    }

    const int y = qMax(0, (height() - m_scrollLeftButton->height()) / 2);
    int x = m_closeTabButton->x();

    x -= m_allTabsButton->width();
    m_allTabsButton->move(x, y);
    x -= m_scrollRightButton->width();
    m_scrollRightButton->move(x, y);
    x -= m_scrollLeftButton->width();
    m_scrollLeftButton->move(x, y);

    m_scrollLeftButton->show();
    m_scrollRightButton->show();
    m_allTabsButton->show();
    m_scrollLeftButton->raise();
    m_scrollRightButton->raise();
    m_allTabsButton->raise();
}

void TabBar::updateOverflowControls()
{
    positionOverflowControls();

    if (!m_overflowing)
        return;

    const int maxOffset = maxScrollOffset();
    m_scrollLeftButton->setEnabled(m_scrollOffset > 0);
    m_scrollRightButton->setEnabled(m_scrollOffset < maxOffset);
}

void TabBar::setScrollOffset(int offset, bool animate)
{
    const int target = qBound(0, offset, maxScrollOffset());
    if (target == m_scrollOffset && (!m_scrollAnimation || m_scrollAnimation->state() != QAbstractAnimation::Running)) {
        updateOverflowControls();
        return;
    }

    int duration = 0;
    if (animate && style())
        duration = style()->styleHint(QStyle::SH_Widget_Animation_Duration, nullptr, this);

    if (!animate || duration <= 0 || !m_scrollAnimation) {
        if (m_scrollAnimation)
            m_scrollAnimation->stop();
        m_scrollOffset = target;
        updateOverflowControls();
        moveNewTabButton();
        update();
        return;
    }

    m_scrollAnimation->stop();
    m_scrollAnimation->setDuration(duration);
    m_scrollAnimation->setStartValue(m_scrollOffset);
    m_scrollAnimation->setEndValue(target);
    m_scrollAnimation->start();
}

void TabBar::panScrollBy(int delta)
{
    if (!m_overflowing || delta == 0)
        return;
    setScrollOffset(m_scrollOffset + delta, false);
}

void TabBar::onScrollAnimationValueChanged(const QVariant &value)
{
    m_scrollOffset = qBound(0, value.toInt(), maxScrollOffset());
    updateOverflowControls();
    moveNewTabButton();
    update();
}

void TabBar::onDragAutoScroll()
{
    if (m_dragAutoScrollDirection == 0 || !m_overflowing) {
        m_dragAutoScrollTimer->stop();
        return;
    }
    panScrollBy(m_dragAutoScrollDirection * dragAutoScrollStep);
}

void TabBar::paintEdgeFades(QPainter &painter, const QRect &clipRect)
{
    if (!m_overflowing)
        return;

    const int maxOffset = maxScrollOffset();
    const int w = qMin(edgeFadeWidth, clipRect.width());

    QColor opaque = m_edgeFadeColor;
    opaque.setAlpha(255);
    QColor clear = m_edgeFadeColor;
    clear.setAlpha(0);

    painter.save();
    painter.setClipRect(clipRect);
    painter.setPen(Qt::NoPen);

    if (m_scrollOffset > 0) {
        const QRect fadeRect(clipRect.left(), clipRect.top(), w, clipRect.height());
        QLinearGradient gradient(fadeRect.topLeft(), fadeRect.topRight());
        gradient.setColorAt(0.0, opaque);
        gradient.setColorAt(1.0, clear);
        painter.fillRect(fadeRect, gradient);
    }

    if (m_scrollOffset < maxOffset) {
        const QRect fadeRect(clipRect.right() - w + 1, clipRect.top(), w, clipRect.height());
        QLinearGradient gradient(fadeRect.topLeft(), fadeRect.topRight());
        gradient.setColorAt(0.0, clear);
        gradient.setColorAt(1.0, opaque);
        painter.fillRect(fadeRect, gradient);
    }

    painter.restore();
}

void TabBar::recalculateTabLayout(bool revealActiveTab)
{
    m_tabWidths.clear();
    if (m_tabs.isEmpty()) {
        m_overflowing = false;
        resetScrollOffset();
        updateOverflowControls();
        return;
    }

    // Base available width without overflow controls.
    int baseRight = m_closeTabButton->x();
    if (m_skin->tabBarNewTabButtonIsAtEndOfTabs())
        baseRight -= m_newTabButton->width();
    const int baseAvailable = qMax(0, baseRight - tabsStartX());

    QList<int> naturals;
    naturals.reserve(m_tabs.count());
    qint64 naturalSum = 0;
    for (int index = 0; index < m_tabs.count(); ++index) {
        const int natural = naturalTabWidth(index);
        naturals << natural;
        naturalSum += natural;
    }

    if (naturalSum <= baseAvailable) {
        m_overflowing = false;
        resetScrollOffset();
        m_tabWidths = naturals;
        updateOverflowControls();
        return;
    }

    // Provisional mins against the full base viewport.
    QList<int> baseMins;
    baseMins.reserve(m_tabs.count());
    qint64 baseMinSum = 0;
    for (int index = 0; index < m_tabs.count(); ++index) {
        const int minWidth = minimumTabWidth(index, baseAvailable);
        baseMins << minWidth;
        baseMinSum += minWidth;
    }

    if (baseMinSum <= baseAvailable) {
        // Water-fill: largest cap c such that sum(min(natural_i, c)) <= available,
        // with c never below each tab's floor (encoded by clamping naturals to >= min
        // via using max(min_i, min(natural_i, c)) == min(natural_i, max(c, min_i))
        // when natural_i >= min_i always holds).
        m_overflowing = false;
        resetScrollOffset();

        int low = 0;
        int high = 0;
        for (int natural : naturals)
            high = qMax(high, natural);

        // Binary search the maximum cap that still fits.
        while (low < high) {
            const int mid = low + (high - low + 1) / 2;
            qint64 sum = 0;
            for (int i = 0; i < naturals.count(); ++i) {
                const int width = qMax(baseMins.at(i), qMin(naturals.at(i), mid));
                sum += width;
            }
            if (sum <= baseAvailable)
                low = mid;
            else
                high = mid - 1;
        }

        m_tabWidths.clear();
        m_tabWidths.reserve(naturals.count());
        qint64 used = 0;
        for (int i = 0; i < naturals.count(); ++i) {
            const int width = qMax(baseMins.at(i), qMin(naturals.at(i), low));
            m_tabWidths << width;
            used += width;
        }

        // Distribute any leftover pixels to tabs still below natural width.
        int leftover = static_cast<int>(baseAvailable - used);
        while (leftover > 0) {
            bool grew = false;
            for (int i = 0; i < m_tabWidths.count() && leftover > 0; ++i) {
                if (m_tabWidths[i] < naturals.at(i)) {
                    m_tabWidths[i] += 1;
                    --leftover;
                    grew = true;
                }
            }
            if (!grew)
                break;
        }

        updateOverflowControls();
        return;
    }

    // Even minimum widths do not fit: overflow mode with scroll controls.
    m_overflowing = true;
    const int overflowRight = baseRight - overflowControlsWidth();
    const int overflowAvailable = qMax(0, overflowRight - tabsStartX());

    m_tabWidths.clear();
    m_tabWidths.reserve(m_tabs.count());
    for (int index = 0; index < m_tabs.count(); ++index)
        m_tabWidths << minimumTabWidth(index, overflowAvailable);

    const int maxOffset = qMax(0, totalTabsWidth() - overflowAvailable);
    m_scrollOffset = qBound(0, m_scrollOffset, maxOffset);
    if (revealActiveTab)
        ensureActiveTabVisible(false);
    updateOverflowControls();
}

void TabBar::resetScrollOffset()
{
    m_scrollAnimation->stop();
    m_scrollOffset = 0;
}

void TabBar::refreshTabLayout()
{
    recalculateTabLayout();
    repaint();
}

void TabBar::ensureActiveTabVisible(bool animate)
{
    if (!m_overflowing || m_tabs.isEmpty())
        return;

    const int index = m_tabs.indexOf(m_selectedSessionId);
    if (index < 0 || index >= m_tabWidths.count())
        return;

    const int viewportWidth = qMax(0, tabsAreaRight() - tabsStartX());
    if (viewportWidth <= 0)
        return;

    const int tabLeft = contentLeft(index);
    const int tabRight = tabLeft + m_tabWidths.at(index);
    const int viewLeft = m_scrollOffset;
    const int viewRight = m_scrollOffset + viewportWidth;

    int target = m_scrollOffset;
    if (tabLeft < viewLeft)
        target = tabLeft;
    else if (tabRight > viewRight)
        target = tabRight - viewportWidth;

    target = qBound(0, target, maxScrollOffset());
    if (target != m_scrollOffset)
        setScrollOffset(target, animate);
    else
        updateOverflowControls();
}

void TabBar::scrollTabsLeft()
{
    if (!m_overflowing || m_tabWidths.isEmpty())
        return;

    int edgeIndex = 0;
    for (int index = 0; index < m_tabWidths.count(); ++index) {
        const int left = contentLeft(index);
        const int right = left + m_tabWidths.at(index);
        if (right > m_scrollOffset) {
            edgeIndex = index;
            break;
        }
    }

    const int target = (edgeIndex > 0) ? contentLeft(edgeIndex - 1) : 0;
    setScrollOffset(target, true);
}

void TabBar::scrollTabsRight()
{
    if (!m_overflowing || m_tabWidths.isEmpty())
        return;

    const int viewportWidth = qMax(0, tabsAreaRight() - tabsStartX());
    const int viewRight = m_scrollOffset + viewportWidth;

    int edgeIndex = m_tabWidths.count() - 1;
    for (int index = 0; index < m_tabWidths.count(); ++index) {
        if (contentLeft(index) + m_tabWidths.at(index) >= viewRight) {
            edgeIndex = index;
            break;
        }
    }

    const int edgeRight = contentLeft(edgeIndex) + m_tabWidths.at(edgeIndex);
    int target = m_scrollOffset;
    if (edgeRight > viewRight)
        target = edgeRight - viewportWidth;
    else if (edgeIndex < m_tabWidths.count() - 1)
        target = contentLeft(edgeIndex + 1) + m_tabWidths.at(edgeIndex + 1) - viewportWidth;

    setScrollOffset(target, true);
}

void TabBar::readyAllTabsMenu()
{
    // Remove previous entries without deleting the durable filter action.
    const auto existing = m_allTabsMenu->actions();
    for (QAction *action : existing) {
        if (action != m_allTabsFilterAction) {
            m_allTabsMenu->removeAction(action);
            delete action;
        }
    }

    if (!m_allTabsMenu->actions().contains(m_allTabsFilterAction))
        m_allTabsMenu->addAction(m_allTabsFilterAction);

    m_allTabsFilter->clear();

    for (int index = 0; index < m_tabs.count(); ++index) {
        const int sessionId = m_tabs.at(index);
        const QString label = singleLineTitle(m_tabTitles.value(sessionId)).replace(u'&', QStringLiteral("&&"));
        QAction *action = m_allTabsMenu->addAction(i18nc("@item:inmenu tab number and title", "%1. %2", index + 1, label));
        action->setCheckable(true);
        action->setChecked(sessionId == m_selectedSessionId);
        action->setData(sessionId);
    }

    m_allTabsFilter->setFocus(Qt::PopupFocusReason);
}

void TabBar::filterAllTabsMenu(const QString &text)
{
    const QString needle = text.trimmed();
    const auto actions = m_allTabsMenu->actions();
    for (QAction *action : actions) {
        if (action == m_allTabsFilterAction)
            continue;
        if (!action->data().isValid())
            continue;
        if (needle.isEmpty()) {
            action->setVisible(true);
            continue;
        }
        const QString title = m_tabTitles.value(action->data().toInt());
        action->setVisible(title.contains(needle, Qt::CaseInsensitive));
    }
}

void TabBar::allTabsMenuTriggered(QAction *action)
{
    if (!action || action == m_allTabsFilterAction)
        return;

    bool ok = false;
    const int sessionId = action->data().toInt(&ok);
    if (ok)
        Q_EMIT tabSelected(sessionId);
}

void TabBar::showAllTabsMenu()
{
    readyAllTabsMenu();

    QPoint pos;
    if (m_allTabsButton->isVisible())
        pos = m_allTabsButton->mapToGlobal(QPoint(0, m_allTabsButton->height()));
    else
        pos = mapToGlobal(QPoint(width() / 2, height()));

    m_allTabsMenu->popup(pos);
    m_allTabsFilter->setFocus(Qt::PopupFocusReason);
}

bool TabBar::event(QEvent *event)
{
    if (event->type() == QEvent::ToolTip) {
        QHelpEvent *helpEvent = static_cast<QHelpEvent *>(event);
        const int index = tabAt(helpEvent->x());
        if (index >= 0) {
            const int sessionId = m_tabs.at(index);
            const QString title = m_tabTitles.value(sessionId);
            const int natural = naturalTabWidth(index);
            if (index < m_tabWidths.count() && m_tabWidths.at(index) < natural) {
                QToolTip::showText(helpEvent->globalPos(), title.toHtmlEscaped(), this, tabRect(index));
            } else {
                QToolTip::hideText();
            }
            return true;
        }
        QToolTip::hideText();
    }

    return QWidget::event(event);
}

void TabBar::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.setPen(m_skin->tabBarTextColor());

    const int y = m_skin->tabBarPosition().y();
    const int startX = tabsStartX();
    const int areaRight = tabsAreaRight();
    const QRect tabsClipRect(startX, y, qMax(0, areaRight - startX), height() - y);
    painter.setClipRect(tabsClipRect);

    int x = startX - m_scrollOffset;
    for (int index = 0; index < m_tabs.count(); ++index) {
        const int tabWidth = (index < m_tabWidths.count()) ? m_tabWidths.at(index) : naturalTabWidth(index);
        // Skip drawing tabs that are completely outside the clip for a bit of savings.
        if (x + tabWidth >= startX && x <= areaRight)
            drawTab(x, y, index, tabWidth, painter);
        x += tabWidth;
    }

    paintEdgeFades(painter, tabsClipRect);

    const QPixmap &backgroundImage = m_skin->tabBarBackgroundImage();
    const QPixmap &leftCornerImage = m_skin->tabBarLeftCornerImage();
    const QPixmap &rightCornerImage = m_skin->tabBarRightCornerImage();

    const int paintedEnd = qBound(startX, startX + totalTabsWidth() - m_scrollOffset, areaRight);

    QRegion backgroundClipRegion(rect());
    backgroundClipRegion = backgroundClipRegion.subtracted(m_newTabButton->geometry());
    backgroundClipRegion = backgroundClipRegion.subtracted(m_closeTabButton->geometry());
    if (m_overflowing) {
        backgroundClipRegion = backgroundClipRegion.subtracted(m_scrollLeftButton->geometry());
        backgroundClipRegion = backgroundClipRegion.subtracted(m_scrollRightButton->geometry());
        backgroundClipRegion = backgroundClipRegion.subtracted(m_allTabsButton->geometry());
    }
    QRect tabsRect(startX, y, qMax(0, paintedEnd - startX), height() - y);
    backgroundClipRegion = backgroundClipRegion.subtracted(tabsRect);
    painter.setClipRegion(backgroundClipRegion);

    painter.drawImage(0, 0, leftCornerImage.toImage());
    QRect leftCornerImageRect(0, 0, leftCornerImage.width(), height());
    backgroundClipRegion = backgroundClipRegion.subtracted(leftCornerImageRect);

    painter.drawImage(width() - rightCornerImage.width(), 0, rightCornerImage.toImage());
    QRect rightCornerImageRect(width() - rightCornerImage.width(), 0, rightCornerImage.width(), height());
    backgroundClipRegion = backgroundClipRegion.subtracted(rightCornerImageRect);

    painter.setClipRegion(backgroundClipRegion);

    painter.drawTiledPixmap(0, 0, width(), height(), backgroundImage);

    painter.end();

    if (m_skin->tabBarNewTabButtonIsAtEndOfTabs()) {
        moveNewTabButton();
    }
}

int TabBar::drawTab(int x, int y, int index, int tabWidth, QPainter &painter)
{
    const int startX = x;
    const int sessionId = m_tabs.at(index);
    const bool selected = (sessionId == m_selectedSessionId);
    const QString title = m_tabTitles[sessionId];

    QFont font = tabFont(index);
    painter.setFont(font);

    if (selected) {
        painter.drawPixmap(x, y, m_skin->tabBarSelectedLeftCornerImage());
        x += m_skin->tabBarSelectedLeftCornerImage().width();
    } else if (!m_skin->tabBarUnselectedLeftCornerImage().isNull()) {
        painter.drawPixmap(x, y, m_skin->tabBarUnselectedLeftCornerImage());
        x += m_skin->tabBarUnselectedLeftCornerImage().width();
    } else if (index != m_tabs.indexOf(m_selectedSessionId) + 1) {
        painter.drawPixmap(x, y, m_skin->tabBarSeparatorImage());
        x += m_skin->tabBarSeparatorImage().width();
    }

    if (!m_mainWindow->sessionStack()->isSessionClosable(sessionId)) {
        const int lockSpan = m_skin->tabBarPreventClosingImagePosition().x() + m_skin->tabBarPreventClosingImage().width();
        if (selected)
            painter.drawTiledPixmap(x, y, lockSpan, height(), m_skin->tabBarSelectedBackgroundImage());
        else
            painter.drawTiledPixmap(x, y, lockSpan, height(), m_skin->tabBarUnselectedBackgroundImage());

        painter.drawPixmap(x + m_skin->tabBarPreventClosingImagePosition().x(),
                           m_skin->tabBarPreventClosingImagePosition().y(),
                           m_skin->tabBarPreventClosingImage());

        x += lockSpan;
    }

    const int rightChrome = tabRightChromeWidth(index);
    int textWidth = tabWidth - (x - startX) - rightChrome;
    if (textWidth < 0)
        textWidth = 0;

    if (selected)
        painter.drawTiledPixmap(x, y, textWidth, height(), m_skin->tabBarSelectedBackgroundImage());
    else
        painter.drawTiledPixmap(x, y, textWidth, height(), m_skin->tabBarUnselectedBackgroundImage());

    QFontMetrics fontMetrics(font);
    const QString elided = fontMetrics.elidedText(singleLineTitle(title), Qt::ElideMiddle, qMax(0, textWidth - 2));
    painter.drawText(x, y, textWidth + 1, height() + 2, Qt::AlignHCenter | Qt::AlignVCenter, elided);

    x += textWidth;

    if (selected) {
        painter.drawPixmap(x, m_skin->tabBarPosition().y(), m_skin->tabBarSelectedRightCornerImage());
    } else if (!m_skin->tabBarUnselectedRightCornerImage().isNull()) {
        painter.drawPixmap(x, m_skin->tabBarPosition().y(), m_skin->tabBarUnselectedRightCornerImage());
    } else if (index != m_tabs.indexOf(m_selectedSessionId) - 1) {
        painter.drawPixmap(x, m_skin->tabBarPosition().y(), m_skin->tabBarSeparatorImage());
    }

    return startX + tabWidth;
}

int TabBar::tabAt(int x)
{
    const int startX = tabsStartX();
    const int areaRight = tabsAreaRight();
    if (x < startX || x >= areaRight)
        return -1;

    const int contentX = x - startX + m_scrollOffset;
    int acc = 0;
    for (int index = 0; index < m_tabWidths.count(); ++index) {
        const int width = m_tabWidths.at(index);
        if (contentX >= acc && contentX < acc + width)
            return index;
        acc += width;
    }

    return -1;
}

void TabBar::wheelEvent(QWheelEvent *event)
{
    // Horizontal pan: pixel delta, angle X, or Shift+vertical wheel, only while tabs overflow.
    int panDelta = 0;
    if (!event->pixelDelta().isNull() && event->pixelDelta().x() != 0)
        panDelta = -event->pixelDelta().x();
    else if (event->angleDelta().x() != 0)
        panDelta = -event->angleDelta().x();
    else if (event->modifiers() & Qt::ShiftModifier)
        panDelta = -event->angleDelta().y();

    if (m_overflowing && panDelta != 0) {
        panScrollBy(panDelta);
        event->accept();
        return;
    }

    // Plain vertical notched wheel: keep session cycling. Accumulate to 120 for hi-res wheels.
    m_wheelAccumulator += event->angleDelta().y();
    while (m_wheelAccumulator >= 120) {
        m_wheelAccumulator -= 120;
        selectPreviousTab();
    }
    while (m_wheelAccumulator <= -120) {
        m_wheelAccumulator += 120;
        selectNextTab();
    }
    event->accept();
}

void TabBar::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Escape && m_lineEdit->isVisible())
        m_lineEdit->hide();

    QWidget::keyPressEvent(event);
}

void TabBar::mousePressEvent(QMouseEvent *event)
{
    if (QWhatsThis::inWhatsThisMode())
        return;

    if (event->position().x() < m_skin->tabBarPosition().x())
        return;

    int index = tabAt(event->position().x());

    if (index == -1)
        return;

    if (event->button() == Qt::LeftButton || event->button() == Qt::MiddleButton) {
        m_startPos = event->pos();
        if (index != m_tabs.indexOf(m_selectedSessionId) || event->button() == Qt::MiddleButton) {
            m_mousePressed = true;
            m_mousePressedIndex = index;
        }
        return;
    }

    QWidget::mousePressEvent(event);
}

void TabBar::mouseReleaseEvent(QMouseEvent *event)
{
    if (QWhatsThis::inWhatsThisMode())
        return;

    if (event->position().x() < m_skin->tabBarPosition().x())
        return;

    int index = tabAt(event->position().x());

    if (m_mousePressed && m_mousePressedIndex == index) {
        if (event->button() == Qt::LeftButton && index != m_tabs.indexOf(m_selectedSessionId))
            Q_EMIT tabSelected(m_tabs.at(index));

        if (event->button() == Qt::MiddleButton)
            Q_EMIT tabClosed(m_tabs.at(index));
    }

    m_mousePressed = false;

    m_startPos.setX(0);
    m_startPos.setY(0);

    QWidget::mouseReleaseEvent(event);
}

void TabBar::mouseMoveEvent(QMouseEvent *event)
{
    if (!m_startPos.isNull() && ((event->buttons() & Qt::LeftButton) || (event->buttons() & Qt::MiddleButton))) {
        int distance = (event->pos() - m_startPos).manhattanLength();

        if (distance >= QApplication::startDragDistance()) {
            int index = tabAt(m_startPos.x());

            if (index >= 0 && !m_lineEdit->isVisible())
                startDrag(index);
        }
    }

    QWidget::mouseMoveEvent(event);
}

void TabBar::dragEnterEvent(QDragEnterEvent *event)
{
    TabBar *eventSource = qobject_cast<TabBar *>(event->source());

    if (eventSource) {
        event->setDropAction(Qt::MoveAction);
        event->acceptProposedAction();
    } else {
        drawDropIndicator(-1);
        event->ignore();
    }

    return;
}

void TabBar::dragMoveEvent(QDragMoveEvent *event)
{
    TabBar *eventSource = qobject_cast<TabBar *>(event->source());

    if (eventSource && event->position().toPoint().x() > tabsStartX() && event->position().toPoint().x() < tabsAreaRight()) {
        int index = dropIndex(event->position().toPoint());

        if (index == -1)
            index = m_tabs.count();

        drawDropIndicator(index, isSameTab(event));

        if (m_overflowing) {
            const int x = event->position().toPoint().x();
            const int startX = tabsStartX();
            const int areaRight = tabsAreaRight();
            if (x < startX + dragAutoScrollEdgeZone)
                m_dragAutoScrollDirection = -1;
            else if (x > areaRight - dragAutoScrollEdgeZone)
                m_dragAutoScrollDirection = 1;
            else
                m_dragAutoScrollDirection = 0;

            if (m_dragAutoScrollDirection != 0) {
                if (!m_dragAutoScrollTimer->isActive())
                    m_dragAutoScrollTimer->start();
            } else {
                m_dragAutoScrollTimer->stop();
            }
        }

        event->setDropAction(Qt::MoveAction);
        event->accept();
    } else {
        m_dragAutoScrollDirection = 0;
        m_dragAutoScrollTimer->stop();
        drawDropIndicator(-1);
        event->ignore();
    }

    return;
}

void TabBar::dragLeaveEvent(QDragLeaveEvent *event)
{
    m_dragAutoScrollDirection = 0;
    m_dragAutoScrollTimer->stop();
    drawDropIndicator(-1);
    event->ignore();

    return;
}

void TabBar::dropEvent(QDropEvent *event)
{
    m_dragAutoScrollDirection = 0;
    m_dragAutoScrollTimer->stop();
    drawDropIndicator(-1);

    int x = event->position().toPoint().x();

    if (isSameTab(event) || x < tabsStartX() || x > tabsAreaRight())
        event->ignore();
    else {
        int targetIndex = dropIndex(event->position().toPoint());
        int sourceSessionId = event->mimeData()->text().toInt();
        int sourceIndex = m_tabs.indexOf(sourceSessionId);

        if (targetIndex == -1)
            targetIndex = m_tabs.count() - 1;
        else if (targetIndex < 0)
            targetIndex = 0;
        else if (sourceIndex < targetIndex)
            --targetIndex;

        m_tabs.move(sourceIndex, targetIndex);
        recalculateTabLayout();
        Q_EMIT tabSelected(m_tabs.at(targetIndex));

        event->accept();
    }

    return;
}

void TabBar::mouseDoubleClickEvent(QMouseEvent *event)
{
    if (QWhatsThis::inWhatsThisMode())
        return;

    m_lineEdit->hide();

    if (event->position().x() < 0)
        return;

    int index = tabAt(event->position().x());

    if (event->button() == Qt::LeftButton) {
        if (index != -1)
            interactiveRename(m_tabs.at(index));
        else if (event->position().x() >= tabsStartX() && event->position().x() < tabsAreaRight())
            Q_EMIT newTabRequested();
    }

    QWidget::mouseDoubleClickEvent(event);
}

void TabBar::leaveEvent(QEvent *event)
{
    m_mousePressed = false;
    drawDropIndicator(-1);
    event->ignore();

    QWidget::leaveEvent(event);
}

void TabBar::addTab(int sessionId, const QString &title)
{
    m_tabs.append(sessionId);

    if (title.isEmpty())
        m_tabTitles.insert(sessionId, standardTabTitle());
    else
        m_tabTitles.insert(sessionId, title);

    recalculateTabLayout();
    Q_EMIT tabSelected(sessionId);
}

void TabBar::removeTab(int sessionId)
{
    if (sessionId == -1)
        sessionId = m_selectedSessionId;
    if (sessionId == -1)
        return;
    if (!m_tabs.contains(sessionId))
        return;

    int index = m_tabs.indexOf(sessionId);

    if (m_lineEdit->isVisible() && sessionId == m_renamingSessionId)
        m_lineEdit->hide();

    m_tabs.removeAt(index);
    m_tabTitles.remove(sessionId);
    m_tabTitlesSetInteractive.remove(sessionId);

    if (m_tabs.isEmpty()) {
        m_tabWidths.clear();
        resetScrollOffset();
        m_overflowing = false;
        updateOverflowControls();
        Q_EMIT lastTabClosed();
    } else if (sessionId == m_selectedSessionId)
        Q_EMIT tabSelected(m_tabs.last());
    else {
        recalculateTabLayout();
        Q_EMIT tabSelected(m_selectedSessionId);
    }
}

void TabBar::interactiveRename(int sessionId)
{
    if (sessionId == -1)
        return;
    if (!m_tabs.contains(sessionId))
        return;

    m_renamingSessionId = sessionId;

    const int index = m_tabs.indexOf(sessionId);
    const QRect rect = tabRect(index);

    m_lineEdit->setText(m_tabTitles[sessionId]);
    const QRect area(tabsStartX(), 0, qMax(0, tabsAreaRight() - tabsStartX()), height());
    m_lineEdit->setGeometry(QRect(rect.x() - 1, rect.y() - 1, rect.width() + 3, height() + 2).intersected(area));
    m_lineEdit->selectAll();
    m_lineEdit->setFocus();
    m_lineEdit->show();
}

void TabBar::interactiveRenameDone()
{
    int sessionId = m_renamingSessionId;

    m_renamingSessionId = -1;

    setTabTitle(sessionId, m_lineEdit->text().trimmed());
}

void TabBar::selectTab(int sessionId)
{
    if (!m_tabs.contains(sessionId))
        return;

    m_selectedSessionId = sessionId;

    updateMoveActions(m_tabs.indexOf(sessionId));
    updateToggleActions(sessionId);

    recalculateTabLayout();
    repaint();
}

void TabBar::selectNextTab()
{
    int index = m_tabs.indexOf(m_selectedSessionId);
    int newSelectedSessionId = m_selectedSessionId;

    if (index == -1)
        return;
    else if (index == m_tabs.count() - 1)
        newSelectedSessionId = m_tabs.at(0);
    else
        newSelectedSessionId = m_tabs.at(index + 1);

    Q_EMIT tabSelected(newSelectedSessionId);
}

void TabBar::selectPreviousTab()
{
    int index = m_tabs.indexOf(m_selectedSessionId);
    int newSelectedSessionId = m_selectedSessionId;

    if (index == -1)
        return;
    else if (index == 0)
        newSelectedSessionId = m_tabs.at(m_tabs.count() - 1);
    else
        newSelectedSessionId = m_tabs.at(index - 1);

    Q_EMIT tabSelected(newSelectedSessionId);
}

void TabBar::moveTabLeft(int sessionId)
{
    if (sessionId == -1)
        sessionId = m_selectedSessionId;

    int index = m_tabs.indexOf(sessionId);

    if (index < 1)
        return;

    m_tabs.swapItemsAt(index, index - 1);

    recalculateTabLayout();
    repaint();

    updateMoveActions(index - 1);
}

void TabBar::moveTabRight(int sessionId)
{
    if (sessionId == -1)
        sessionId = m_selectedSessionId;

    int index = m_tabs.indexOf(sessionId);

    if (index == -1 || index == m_tabs.count() - 1)
        return;

    m_tabs.swapItemsAt(index, index + 1);

    recalculateTabLayout();
    repaint();

    updateMoveActions(index + 1);
}

void TabBar::closeTabButtonClicked()
{
    Q_EMIT tabClosed(m_selectedSessionId);
}

QString TabBar::tabTitle(int sessionId)
{
    if (m_tabTitles.contains(sessionId))
        return m_tabTitles[sessionId];
    else
        return QString();
}

void TabBar::setTabTitle(int sessionId, const QString &newTitle, InteractiveType interactive)
{
    if (sessionId == -1)
        return;
    if (!m_tabTitles.contains(sessionId))
        return;
    if (!interactive && m_tabTitlesSetInteractive.value(sessionId, false))
        return;
    if (interactive)
        m_tabTitlesSetInteractive[sessionId] = interactive;

    if (!newTitle.isEmpty()) {
        m_tabTitles[sessionId] = newTitle;
    } else
        m_tabTitlesSetInteractive.remove(sessionId);

    Q_EMIT tabTitleEdited(sessionId, newTitle);
    recalculateTabLayout(sessionId == m_selectedSessionId);
    update();
}

void TabBar::setTabTitleAutomated(int sessionId, const QString &newTitle)
{
    setTabTitle(sessionId, newTitle, NonInteractive);
}

int TabBar::sessionAtTab(int index)
{
    if (index < 0 || index > m_tabs.count() - 1)
        return -1;
    else
        return m_tabs.at(index);
}

QString TabBar::standardTabTitle()
{
    QString newTitle = makeTabTitle(0);

    bool nameOk;
    int count = 0;

    do {
        nameOk = true;

        QHashIterator<int, QString> it(m_tabTitles);

        while (it.hasNext()) {
            it.next();

            if (newTitle == it.value()) {
                nameOk = false;
                break;
            }
        }

        if (!nameOk) {
            count++;
            newTitle = makeTabTitle(count);
        }
    } while (!nameOk);

    return newTitle;
}

QString TabBar::makeTabTitle(int id)
{
    if (id == 0) {
        return xi18nc("@title:tab", "Shell");
    } else {
        return xi18nc("@title:tab", "Shell No. %1", id + 1);
    }
}

void TabBar::startDrag(int index)
{
    int sessionId = sessionAtTab(index);

    m_startPos.setX(0);
    m_startPos.setY(0);

    if (index < 0 || index >= m_tabWidths.count())
        return;

    const int tabWidth = m_tabWidths.at(index);

    QPixmap tab(tabWidth, height());
    QColor fillColor(Settings::backgroundColor());

    if (m_mainWindow->useTranslucency())
        fillColor.setAlphaF(qreal(Settings::backgroundColorOpacity()) / 100);

    tab.fill(fillColor);

    QPainter painter(&tab);
    painter.setPen(m_skin->tabBarTextColor());

    drawTab(0, 0, index, tabWidth, painter);
    painter.end();

    QMimeData *mimeData = new QMimeData;
    mimeData->setText(QVariant(sessionId).toString());

    QDrag *drag = new QDrag(this);
    drag->setMimeData(mimeData);
    drag->setPixmap(tab);
    drag->exec(Qt::MoveAction);

    return;
}

void TabBar::drawDropIndicator(int index, bool disabled)
{
    const int arrowSize = 16;

    if (!m_dropIndicator) {
        m_dropIndicator = new QLabel(parentWidget());
        m_dropIndicator->resize(arrowSize, arrowSize);
    }

    QIcon::Mode drawMode = disabled ? QIcon::Disabled : QIcon::Normal;
    m_dropIndicator->setPixmap(QIcon::fromTheme(QStringLiteral("arrow-down")).pixmap(arrowSize, arrowSize, drawMode));

    if (index < 0) {
        m_dropIndicator->hide();
        return;
    }

    int temp_index;
    if (index == m_tabs.count())
        temp_index = index - 1;
    else
        temp_index = index;

    if (temp_index < 0 || temp_index >= m_tabWidths.count()) {
        m_dropIndicator->hide();
        return;
    }

    const QRect rect = tabRect(temp_index);
    m_dropRect = QRect(rect.x(), m_skin->tabBarPosition().y() - height(), rect.width(), height() - m_skin->tabBarPosition().y());
    QPoint pos;

    if (index < m_tabs.count())
        pos = m_dropRect.topLeft();
    else
        pos = m_dropRect.topRight();

    pos.rx() -= arrowSize / 2;

    m_dropIndicator->move(mapTo(parentWidget(), pos));
    m_dropIndicator->show();

    return;
}

int TabBar::dropIndex(const QPoint pos)
{
    int index = tabAt(pos.x());
    if (index < 0)
        return index;

    const QRect rect = tabRect(index);
    m_dropRect = QRect(rect.x(), m_skin->tabBarPosition().y() - height(), rect.width(), height() - m_skin->tabBarPosition().y());

    if ((pos.x() - m_dropRect.left()) > (m_dropRect.width() / 2))
        ++index;

    if (index == m_tabs.count())
        return -1;

    return index;
}

bool TabBar::isSameTab(const QDropEvent *event)
{
    int index = dropIndex(event->position().toPoint());
    int sourceSessionId = event->mimeData()->text().toInt();
    int sourceIndex = m_tabs.indexOf(sourceSessionId);

    bool isLastTab = (sourceIndex == m_tabs.count() - 1) && (index == -1);

    if ((sourceIndex == index) || (sourceIndex == index - 1) || isLastTab)
        return true;
    else
        return false;
}

#include "moc_tabbar.cpp"
