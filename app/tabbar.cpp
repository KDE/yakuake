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


#include "tabbar.h"
#include "mainwindow.h"
#include "skin.h"
#include "session.h"
#include "sessionstack.h"
#include "settings.h"

#include <KLineEdit>
#include <KMenu>
#include <KPushButton>
#include <KActionCollection>
#include <KGlobalSettings>
#include <KLocalizedString>

#include <QBitmap>
#include <QPainter>
#include <QtDBus/QtDBus>
#include <QToolButton>
#include <QWheelEvent>
#include <QWhatsThis>

#include <QMimeData>
#include <QDrag>
#include <QLabel>


TabBar::TabBar(MainWindow* mainWindow) : QWidget(mainWindow)
{
    QDBusConnection::sessionBus().registerObject("/yakuake/tabs", this, QDBusConnection::ExportScriptableSlots);

    setWhatsThis(i18nc("@info:whatsthis",
                       "<title>Tab Bar</title>"
                       "<para>The tab bar allows you to switch between sessions. You can double-click a tab to edit its label.</para>"));

    m_selectedSessionId = -1;
    m_renamingSessionId = -1;

    m_mousePressed = false;
    m_mousePressedIndex = -1;

    m_dropIndicator = 0;

    m_mainWindow = mainWindow;

    m_skin = mainWindow->skin();
    connect(m_skin, SIGNAL(iconChanged()), this, SLOT(repaint()));

    m_tabContextMenu = new KMenu(this);
    connect(m_tabContextMenu, SIGNAL(hovered(QAction*)), this, SLOT(contextMenuActionHovered(QAction*)));

    m_toggleKeyboardInputMenu = new KMenu(i18nc("@title:menu", "Disable Keyboard Input"), this);
    m_toggleMonitorActivityMenu = new KMenu(i18nc("@title:menu", "Monitor for Activity"), this);
    m_toggleMonitorSilenceMenu = new KMenu(i18nc("@title:menu", "Monitor for Silence"), this);

    m_sessionMenu = new KMenu(this);
    connect(m_sessionMenu, SIGNAL(aboutToShow()), this, SLOT(readySessionMenu()));

    m_newTabButton = new QToolButton(this);
    m_newTabButton->setFocusPolicy(Qt::NoFocus);
    m_newTabButton->setMenu(m_sessionMenu);
    m_newTabButton->setPopupMode(QToolButton::DelayedPopup);
    m_newTabButton->setToolTip(i18nc("@info:tooltip", "New Session"));
    m_newTabButton->setWhatsThis(i18nc("@info:whatsthis", "Adds a new session. Press and hold to select session type from menu."));
    connect(m_newTabButton, SIGNAL(clicked()), this, SIGNAL(newTabRequested()));

    m_closeTabButton = new KPushButton(this);
    m_closeTabButton->setFocusPolicy(Qt::NoFocus);
    m_closeTabButton->setToolTip(i18nc("@info:tooltip", "Close Session"));
    m_closeTabButton->setWhatsThis(i18nc("@info:whatsthis", "Closes the active session."));
    connect(m_closeTabButton, SIGNAL(clicked()), this, SLOT(closeTabButtonClicked()));

    m_lineEdit = new KLineEdit(this);
    m_lineEdit->setFrame(false);
    m_lineEdit->setClearButtonShown(false);
    m_lineEdit->setAlignment(Qt::AlignHCenter);
    m_lineEdit->hide();

    connect(m_lineEdit, SIGNAL(editingFinished()), m_lineEdit, SLOT(hide()));
    connect(m_lineEdit, SIGNAL(returnPressed()), this, SLOT(interactiveRenameDone()));

    setAcceptDrops(true);
}

TabBar::~TabBar()
{
}

void TabBar::applySkin()
{
    resize(width(), m_skin->tabBarBackgroundImage().height());

    m_newTabButton->setStyleSheet(m_skin->tabBarNewTabButtonStyleSheet());
    m_closeTabButton->setStyleSheet(m_skin->tabBarCloseTabButtonStyleSheet());

    m_newTabButton->move( m_skin->tabBarNewTabButtonPosition().x(), m_skin->tabBarNewTabButtonPosition().y());
    m_closeTabButton->move(width() - m_skin->tabBarCloseTabButtonPosition().x(), m_skin->tabBarCloseTabButtonPosition().y());

    repaint();
}

void TabBar::readyTabContextMenu()
{
    if (m_tabContextMenu->isEmpty())
    {
        m_tabContextMenu->addAction(m_mainWindow->actionCollection()->action("split-left-right"));
        m_tabContextMenu->addAction(m_mainWindow->actionCollection()->action("split-top-bottom"));
        m_tabContextMenu->addSeparator();
        m_tabContextMenu->addAction(m_mainWindow->actionCollection()->action("edit-profile"));
        m_tabContextMenu->addAction(m_mainWindow->actionCollection()->action("rename-session"));
        m_tabContextMenu->addAction(m_mainWindow->actionCollection()->action("toggle-session-prevent-closing"));
        m_tabContextMenu->addMenu(m_toggleKeyboardInputMenu);
        m_tabContextMenu->addMenu(m_toggleMonitorActivityMenu);
        m_tabContextMenu->addMenu(m_toggleMonitorSilenceMenu);
        m_tabContextMenu->addSeparator();
        m_tabContextMenu->addAction(m_mainWindow->actionCollection()->action("move-session-left"));
        m_tabContextMenu->addAction(m_mainWindow->actionCollection()->action("move-session-right"));
        m_tabContextMenu->addSeparator();
        m_tabContextMenu->addAction(m_mainWindow->actionCollection()->action("close-active-terminal"));
        m_tabContextMenu->addAction(m_mainWindow->actionCollection()->action("close-session"));
    }
}

void TabBar::readySessionMenu()
{
    if (m_sessionMenu->isEmpty())
    {
        m_sessionMenu->addAction(m_mainWindow->actionCollection()->action("new-session"));
        m_sessionMenu->addSeparator();
        m_sessionMenu->addAction(m_mainWindow->actionCollection()->action("new-session-two-horizontal"));
        m_sessionMenu->addAction(m_mainWindow->actionCollection()->action("new-session-two-vertical"));
        m_sessionMenu->addAction(m_mainWindow->actionCollection()->action("new-session-quad"));
    }
}

void TabBar::updateMoveActions(int index)
{
    if (index == -1) return;

    m_mainWindow->actionCollection()->action("move-session-left")->setEnabled(false);
    m_mainWindow->actionCollection()->action("move-session-right")->setEnabled(false);

    if (index != m_tabs.indexOf(m_tabs.first()))
        m_mainWindow->actionCollection()->action("move-session-left")->setEnabled(true);

    if (index != m_tabs.indexOf(m_tabs.last()))
        m_mainWindow->actionCollection()->action("move-session-right")->setEnabled(true);
}

void TabBar::updateToggleActions(int sessionId)
{
    if (sessionId == -1) return;

    KActionCollection* actionCollection = m_mainWindow->actionCollection();
    SessionStack* sessionStack = m_mainWindow->sessionStack();

    QAction* toggleAction = actionCollection->action("toggle-session-prevent-closing");
    toggleAction->setChecked(!sessionStack->isSessionClosable(sessionId));

    toggleAction = actionCollection->action("toggle-session-keyboard-input");
    toggleAction->setChecked(!sessionStack->hasTerminalsWithKeyboardInputEnabled(sessionId));

    toggleAction = actionCollection->action("toggle-session-monitor-activity");
    toggleAction->setChecked(!sessionStack->hasTerminalsWithMonitorActivityDisabled(sessionId));

    toggleAction = actionCollection->action("toggle-session-monitor-silence");
    toggleAction->setChecked(!sessionStack->hasTerminalsWithMonitorSilenceDisabled(sessionId));
}

void TabBar::updateToggleKeyboardInputMenu(int sessionId)
{
    if (!m_tabs.contains(sessionId)) return;

    QAction* toggleKeyboardInputAction = m_mainWindow->actionCollection()->action("toggle-session-keyboard-input");
    QAction* anchor = m_toggleKeyboardInputMenu->menuAction();

    SessionStack* sessionStack = m_mainWindow->sessionStack();

    QStringList terminalIds = sessionStack->terminalIdsForSessionId(sessionId).split(',', QString::SkipEmptyParts);

    m_toggleKeyboardInputMenu->clear();

    if (terminalIds.count() <= 1)
    {
        toggleKeyboardInputAction->setText(i18nc("@action", "Disable Keyboard Input"));
        m_tabContextMenu->insertAction(anchor, toggleKeyboardInputAction);
        m_toggleKeyboardInputMenu->menuAction()->setVisible(false);
    }
    else
    {
        toggleKeyboardInputAction->setText(i18nc("@action", "For This Session"));
        m_toggleKeyboardInputMenu->menuAction()->setVisible(true);

        m_tabContextMenu->removeAction(toggleKeyboardInputAction);
        m_toggleKeyboardInputMenu->addAction(toggleKeyboardInputAction);

        m_toggleKeyboardInputMenu->addSeparator();

        int count = 0;

        QStringListIterator i(terminalIds);

        while (i.hasNext())
        {
            int terminalId = i.next().toInt();

            ++count;

            QAction* action = m_toggleKeyboardInputMenu->addAction(i18nc("@action", "For Terminal %1", count));
            action->setCheckable(true);
            action->setChecked(!sessionStack->isTerminalKeyboardInputEnabled(terminalId));
            action->setData(terminalId);
            connect(action, SIGNAL(triggered(bool)), m_mainWindow, SLOT(handleToggleTerminalKeyboardInput(bool)));
        }
    }
}

void TabBar::updateToggleMonitorActivityMenu(int sessionId)
{
    if (!m_tabs.contains(sessionId)) return;

    QAction* toggleMonitorActivityAction = m_mainWindow->actionCollection()->action("toggle-session-monitor-activity");
    QAction* anchor = m_toggleMonitorActivityMenu->menuAction();

    SessionStack* sessionStack = m_mainWindow->sessionStack();

    QStringList terminalIds = sessionStack->terminalIdsForSessionId(sessionId).split(',', QString::SkipEmptyParts);

    m_toggleMonitorActivityMenu->clear();

    if (terminalIds.count() <= 1)
    {
        toggleMonitorActivityAction->setText(i18nc("@action", "Monitor for Activity"));
        m_tabContextMenu->insertAction(anchor, toggleMonitorActivityAction);
        m_toggleMonitorActivityMenu->menuAction()->setVisible(false);
    }
    else
    {
        toggleMonitorActivityAction->setText(i18nc("@action", "In This Session"));
        m_toggleMonitorActivityMenu->menuAction()->setVisible(true);

        m_tabContextMenu->removeAction(toggleMonitorActivityAction);
        m_toggleMonitorActivityMenu->addAction(toggleMonitorActivityAction);

        m_toggleMonitorActivityMenu->addSeparator();

        int count = 0;

        QStringListIterator i(terminalIds);

        while (i.hasNext())
        {
            int terminalId = i.next().toInt();

            ++count;

            QAction* action = m_toggleMonitorActivityMenu->addAction(i18nc("@action", "In Terminal %1", count));
            action->setCheckable(true);
            action->setChecked(sessionStack->isTerminalMonitorActivityEnabled(terminalId));
            action->setData(terminalId);
            connect(action, SIGNAL(triggered(bool)), m_mainWindow, SLOT(handleToggleTerminalMonitorActivity(bool)));
        }
    }
}

void TabBar::updateToggleMonitorSilenceMenu(int sessionId)
{
    if (!m_tabs.contains(sessionId)) return;

    QAction* toggleMonitorSilenceAction = m_mainWindow->actionCollection()->action("toggle-session-monitor-silence");
    QAction* anchor = m_toggleMonitorSilenceMenu->menuAction();

    SessionStack* sessionStack = m_mainWindow->sessionStack();

    QStringList terminalIds = sessionStack->terminalIdsForSessionId(sessionId).split(',', QString::SkipEmptyParts);

    m_toggleMonitorSilenceMenu->clear();

    if (terminalIds.count() <= 1)
    {
        toggleMonitorSilenceAction->setText(i18nc("@action", "Monitor for Silence"));
        m_tabContextMenu->insertAction(anchor, toggleMonitorSilenceAction);
        m_toggleMonitorSilenceMenu->menuAction()->setVisible(false);
    }
    else
    {
        toggleMonitorSilenceAction->setText(i18nc("@action", "In This Session"));
        m_toggleMonitorSilenceMenu->menuAction()->setVisible(true);

        m_tabContextMenu->removeAction(toggleMonitorSilenceAction);
        m_toggleMonitorSilenceMenu->addAction(toggleMonitorSilenceAction);

        m_toggleMonitorSilenceMenu->addSeparator();

        int count = 0;

        QStringListIterator i(terminalIds);

        while (i.hasNext())
        {
            int terminalId = i.next().toInt();

            ++count;

            QAction* action = m_toggleMonitorSilenceMenu->addAction(i18nc("@action", "In Terminal %1", count));
            action->setCheckable(true);
            action->setChecked(sessionStack->isTerminalMonitorSilenceEnabled(terminalId));
            action->setData(terminalId);
            connect(action, SIGNAL(triggered(bool)), m_mainWindow, SLOT(handleToggleTerminalMonitorSilence(bool)));
        }
    }
}

void TabBar::contextMenuActionHovered(QAction* action)
{
    bool ok = false;

    if (!action->data().isNull())
    {
        int terminalId = action->data().toInt(&ok);

        if (ok) emit requestTerminalHighlight(terminalId);
    }
    else if (!ok)
        emit requestRemoveTerminalHighlight();
}

void TabBar::contextMenuEvent(QContextMenuEvent* event)
{
    if (event->x() < 0) return;

    int index = tabAt(event->x());

    if (index == -1)
        m_sessionMenu->exec(QCursor::pos());
    else
    {
        readyTabContextMenu();

        updateMoveActions(index);

        int sessionId = sessionAtTab(index);
        updateToggleActions(sessionId);
        updateToggleKeyboardInputMenu(sessionId);
        updateToggleMonitorActivityMenu(sessionId);
        updateToggleMonitorSilenceMenu(sessionId);

        m_mainWindow->setContextDependentActionsQuiet(true);

        QAction* action = m_tabContextMenu->exec(QCursor::pos());

        emit tabContextMenuClosed();

        if (action)
        {
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

void TabBar::resizeEvent(QResizeEvent* event)
{
    m_newTabButton->move(m_skin->tabBarNewTabButtonPosition().x(), m_skin->tabBarNewTabButtonPosition().y());
    m_closeTabButton->move(width() - m_skin->tabBarCloseTabButtonPosition().x(), m_skin->tabBarCloseTabButtonPosition().y());

    QWidget::resizeEvent(event);
}

void TabBar::paintEvent(QPaintEvent*)
{
    QPainter painter(this);
    painter.setPen(m_skin->tabBarTextColor());

    int x = m_skin->tabBarPosition().x();
    int y = m_skin->tabBarPosition().y();
    m_tabWidths.clear();

    QRect tabsClipRect(x, y, m_closeTabButton->x() - x, height() - y);
    painter.setClipRect(tabsClipRect);

    for (int index = 0; index < m_tabs.count(); ++index)
    {
        x = drawButton(x, y, index, painter);
        m_tabWidths << x;
    }

    const QPixmap& backgroundImage = m_skin->tabBarBackgroundImage();
    const QPixmap& leftCornerImage = m_skin->tabBarLeftCornerImage();
    const QPixmap& rightCornerImage = m_skin->tabBarRightCornerImage();

    x = x > tabsClipRect.right() ? tabsClipRect.right() + 1 : x;

    QRegion backgroundClipRegion(rect());
    backgroundClipRegion = backgroundClipRegion.subtracted(m_newTabButton->geometry());
    backgroundClipRegion = backgroundClipRegion.subtracted(m_closeTabButton->geometry());
    QRect tabsRect(m_skin->tabBarPosition().x(), y, x - m_skin->tabBarPosition().x(),
        height() - m_skin->tabBarPosition().y());
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
}

int TabBar::drawButton(int x, int y, int index, QPainter& painter)
{
    QString title;
    int sessionId;
    bool selected;
    QFont font = KGlobalSettings::generalFont();
    int textWidth = 0;

    sessionId = m_tabs.at(index);
    selected = (sessionId == m_selectedSessionId);
    title = m_tabTitles[sessionId];

    if (selected)
    {
        painter.drawPixmap(x, y, m_skin->tabBarSelectedLeftCornerImage());
        x += m_skin->tabBarSelectedLeftCornerImage().width();
    }
    else if (index != m_tabs.indexOf(m_selectedSessionId) + 1)
    {
        painter.drawPixmap(x, y, m_skin->tabBarSeparatorImage());
        x += m_skin->tabBarSeparatorImage().width();
    }

    if (selected) font.setBold(true);
    else font.setBold(false);

    painter.setFont(font);

    QFontMetrics fontMetrics(font);
    textWidth = fontMetrics.width(title) + 10;

    // Draw the Prevent Closing image in the tab button.
    if (m_mainWindow->sessionStack()->isSessionClosable(sessionId) == false)
    {
        if (selected)
            painter.drawTiledPixmap(x, y,
                    m_skin->tabBarPreventClosingImagePosition().x() +
                    m_skin->tabBarPreventClosingImage().width(), height(),
                    m_skin->tabBarSelectedBackgroundImage());
        else
            painter.drawTiledPixmap(x, y,
                    m_skin->tabBarPreventClosingImagePosition().x() +
                    m_skin->tabBarPreventClosingImage().width(), height(),
                    m_skin->tabBarUnselectedBackgroundImage());

        painter.drawPixmap(x + m_skin->tabBarPreventClosingImagePosition().x(),
                           m_skin->tabBarPreventClosingImagePosition().y(),
                           m_skin->tabBarPreventClosingImage());

        x += m_skin->tabBarPreventClosingImagePosition().x();
        x += m_skin->tabBarPreventClosingImage().width();
    }

    if (selected)
        painter.drawTiledPixmap(x, y, textWidth, height(), m_skin->tabBarSelectedBackgroundImage());
    else
        painter.drawTiledPixmap(x, y, textWidth, height(), m_skin->tabBarUnselectedBackgroundImage());

    painter.drawText(x, y, textWidth + 1, height() + 2, Qt::AlignHCenter | Qt::AlignVCenter, title);

    x += textWidth;

    if (selected)
    {
        painter.drawPixmap(x, m_skin->tabBarPosition().y(), m_skin->tabBarSelectedRightCornerImage());
        x += m_skin->tabBarSelectedRightCornerImage().width();
    }
    else if (index != m_tabs.indexOf(m_selectedSessionId) - 1)
    {
        painter.drawPixmap(x, m_skin->tabBarPosition().y(), m_skin->tabBarSeparatorImage());
        x += m_skin->tabBarSeparatorImage().width();
    }

    return x;
}

int TabBar::tabAt(int x)
{
    for (int index = 0; index < m_tabWidths.count(); ++index)
    {
        if (x >  m_skin->tabBarPosition().x() && x < m_tabWidths.at(index))
            return index;
    }

    return -1;
}

void TabBar::wheelEvent(QWheelEvent* event)
{
    if (event->delta() < 0)
        selectNextTab();
    else
        selectPreviousTab();
}

void TabBar::keyPressEvent(QKeyEvent* event)
{
    if (event->key() == Qt::Key_Escape && m_lineEdit->isVisible())
        m_lineEdit->hide();

    QWidget::keyPressEvent(event);
}

void TabBar::mousePressEvent(QMouseEvent* event)
{
    if (QWhatsThis::inWhatsThisMode()) return;

    if (event->x() < m_skin->tabBarPosition().x()) return;

    int index = tabAt(event->x());

    if (index == -1) return;

    if (event->button() == Qt::LeftButton || event->button() == Qt::MidButton)
    {
        m_startPos = event->pos();
        if (index != m_tabs.indexOf(m_selectedSessionId) || event->button() == Qt::MidButton)
        {
            m_mousePressed = true;
            m_mousePressedIndex = index;
        }
        return;
    }

    QWidget::mousePressEvent(event);
}

void TabBar::mouseReleaseEvent(QMouseEvent* event)
{
    if (QWhatsThis::inWhatsThisMode()) return;

    if (event->x() < m_skin->tabBarPosition().x()) return;

    int index = tabAt(event->x());

    if (m_mousePressed && m_mousePressedIndex == index)
    {
        if (event->button() == Qt::LeftButton && index != m_tabs.indexOf(m_selectedSessionId))
            emit tabSelected(m_tabs.at(index));

        if (event->button() == Qt::MidButton)
            emit tabClosed(m_tabs.at(index));
    }

    m_mousePressed = false;

    m_startPos.setX(0);
    m_startPos.setY(0);

    QWidget::mouseReleaseEvent(event);
}

void TabBar::mouseMoveEvent(QMouseEvent* event)
{
    if (!m_startPos.isNull() && ((event->buttons() & Qt::LeftButton) || (event->buttons() & Qt::MidButton)))
    {
        int distance = (event->pos() - m_startPos).manhattanLength();

        if (distance >= KGlobalSettings::dndEventDelay())
        {
            int index = tabAt(m_startPos.x());

            if (index >= 0 && !m_lineEdit->isVisible())
                startDrag(index);
        }
    }

    QWidget::mouseMoveEvent(event);
}

void TabBar::dragEnterEvent(QDragEnterEvent* event)
{
    TabBar* eventSource = qobject_cast<TabBar*>(event->source());

    if (eventSource)
    {
        event->setDropAction(Qt::MoveAction);
        event->acceptProposedAction();
    }
    else
    {
        drawDropIndicator(-1);
        event->ignore();
    }

    return;
}

void TabBar::dragMoveEvent(QDragMoveEvent* event)
{
    TabBar* eventSource = qobject_cast<TabBar*>(event->source());

    if (eventSource && event->pos().x() > m_skin->tabBarPosition().x() && event->pos().x() < m_closeTabButton->x())
    {
        int index = dropIndex(event->pos());

        if (index == -1)
            index = m_tabs.count();

        drawDropIndicator(index, isSameTab(event));

        event->setDropAction(Qt::MoveAction);
        event->accept();
    }
    else
    {
        drawDropIndicator(-1);
        event->ignore();
    }

    return;
}

void TabBar::dragLeaveEvent(QDragLeaveEvent* event)
{
    drawDropIndicator(-1);
    event->ignore();

    return;
}

void TabBar::dropEvent(QDropEvent* event)
{
    drawDropIndicator(-1);

    int x = event->pos().x();

    if (isSameTab(event) || x < m_skin->tabBarPosition().x() || x > m_closeTabButton->x())
        event->ignore();
    else
    {
        int targetIndex = dropIndex(event->pos());
        int sourceSessionId = event->mimeData()->text().toInt();
        int sourceIndex = m_tabs.indexOf(sourceSessionId);

        if (targetIndex == -1)
            targetIndex = m_tabs.count() - 1;
        else if (targetIndex < 0)
            targetIndex = 0;
        else if (sourceIndex < targetIndex)
            --targetIndex;

        m_tabs.move(sourceIndex, targetIndex);
        emit tabSelected(m_tabs.at(targetIndex));

        event->accept();
    }

    return;
}

void TabBar::mouseDoubleClickEvent(QMouseEvent* event)
{
    if (QWhatsThis::inWhatsThisMode()) return;

    m_lineEdit->hide();

    if (event->x() < 0) return;

    int index = tabAt(event->x());

    if (event->button() == Qt::LeftButton)
    {
        if (event->x() <= m_tabWidths.last())
            interactiveRename(m_tabs.at(index));
        else if (event->x() > m_tabWidths.last())
            emit newTabRequested();
    }

    QWidget::mouseDoubleClickEvent(event);
}

void TabBar::leaveEvent(QEvent* event)
{
    m_mousePressed = false;
    drawDropIndicator(-1);
    event->ignore();

    QWidget::leaveEvent(event);
}

void TabBar::addTab(int sessionId, const QString& title)
{
    m_tabs.append(sessionId);

    if (title.isEmpty())
        m_tabTitles.insert(sessionId, standardTabTitle());
    else
        m_tabTitles.insert(sessionId, title);

    emit tabSelected(sessionId);
}

void TabBar::removeTab(int sessionId)
{
    if (sessionId == -1) sessionId = m_selectedSessionId;
    if (sessionId == -1) return;
    if (!m_tabs.contains(sessionId)) return;

    int index = m_tabs.indexOf(sessionId);

    if (m_lineEdit->isVisible() && sessionId == m_renamingSessionId)
        m_lineEdit->hide();

    m_tabs.removeAt(index);
    m_tabTitles.remove(sessionId);

    if (m_tabs.count() == 0)
        emit lastTabClosed();
    else
        emit tabSelected(m_tabs.last());
}

void TabBar::interactiveRename(int sessionId)
{
    if (sessionId == -1) return;
    if (!m_tabs.contains(sessionId)) return;

    m_renamingSessionId = sessionId;

    int index = m_tabs.indexOf(sessionId);
    int x = index ? m_tabWidths.at(index - 1) : m_skin->tabBarPosition().x();
    int y = m_skin->tabBarPosition().y();
    int width = m_tabWidths.at(index) - x;

    m_lineEdit->setText(m_tabTitles[sessionId]);
    m_lineEdit->setGeometry(x-1, y-1, width+3, height()+2);
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
    if (!m_tabs.contains(sessionId)) return;

    m_selectedSessionId = sessionId;

    updateMoveActions(m_tabs.indexOf(sessionId));
    updateToggleActions(sessionId);

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

    emit tabSelected(newSelectedSessionId);
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

    emit tabSelected(newSelectedSessionId);
}

void TabBar::moveTabLeft(int sessionId)
{
    if (sessionId == -1) sessionId = m_selectedSessionId;

    int index = m_tabs.indexOf(sessionId);

    if (index < 1) return;

    m_tabs.swap(index, index - 1);

    repaint();

    updateMoveActions(index - 1);
}

void TabBar::moveTabRight(int sessionId)
{
    if (sessionId == -1) sessionId = m_selectedSessionId;

    int index = m_tabs.indexOf(sessionId);

    if (index == -1 || index == m_tabs.count() - 1) return;

    m_tabs.swap(index, index + 1);

    repaint();

    updateMoveActions(index + 1);
}

void TabBar::closeTabButtonClicked()
{
    emit tabClosed(m_selectedSessionId);
}

QString TabBar::tabTitle(int sessionId)
{
    if (m_tabTitles.contains(sessionId))
        return m_tabTitles[sessionId];
    else
        return QString();
}

void TabBar::setTabTitle(int sessionId, const QString& newTitle)
{
    if (sessionId == -1) return;
    if (!m_tabTitles.contains(sessionId)) return;

    if (!newTitle.isEmpty())
        m_tabTitles[sessionId] = newTitle;

    update();
}

int TabBar::sessionAtTab(int index)
{
    if (index > m_tabs.count() - 1)
        return -1;
    else
        return m_tabs.at(index);
}

QString TabBar::standardTabTitle()
{
    QString newTitle = makeTabTitle(0);

    bool nameOk;
    int count = 0;

    do
    {
        nameOk = true;

        QHashIterator<int, QString> it(m_tabTitles);

        while (it.hasNext())
        {
            it.next();

            if (newTitle == it.value())
            {
                nameOk = false;
                break;
            }
        }

        if (!nameOk)
        {
            count++;
            newTitle = makeTabTitle(count);
        }
    }
    while (!nameOk);

    return newTitle;
}

QString TabBar::makeTabTitle(int id)
{
    if (id == 0)
    {
        return i18nc("@title:tab", "Shell");
    }
    else
    {
        return i18nc("@title:tab", "Shell No. <numid>%1</numid>", id+1);
    }
}

void TabBar::startDrag(int index)
{
    int sessionId = sessionAtTab(index);

    m_startPos.setX(0);
    m_startPos.setY(0);

    int x = index ? m_tabWidths.at(index - 1) : m_skin->tabBarPosition().x();
    int tabWidth = m_tabWidths.at(index) - x;
    QString title = tabTitle(sessionId);

    QPixmap tab(tabWidth, height());
    QColor fillColor(Settings::backgroundColor());

    if (m_mainWindow->useTranslucency())
        fillColor.setAlphaF(qreal(Settings::backgroundColorOpacity()) / 100);

    tab.fill(fillColor);

    QPainter painter(&tab);
    painter.initFrom(this);
    painter.setPen(m_skin->tabBarTextColor());

    drawButton(0, 0, index, painter);
    painter.end();

    QMimeData* mimeData = new QMimeData;
    mimeData->setText(QVariant(sessionId).toString());

    QDrag* drag = new QDrag(this);
    drag->setMimeData(mimeData);
    drag->setPixmap(tab);
    drag->exec(Qt::MoveAction);

    return;
}

void TabBar::drawDropIndicator(int index, bool disabled)
{
    const int arrowSize = 16;

    if (!m_dropIndicator)
    {
        m_dropIndicator = new QLabel(parentWidget());
        m_dropIndicator->resize(arrowSize, arrowSize);
    }

    QIcon::Mode drawMode = disabled ? QIcon::Disabled : QIcon::Normal;
    m_dropIndicator->setPixmap(KIcon("arrow-down").pixmap(arrowSize, arrowSize, drawMode));

    if (index < 0)
    {
        m_dropIndicator->hide();
        return;
    }

    int temp_index;
    if (index == m_tabs.count())
        temp_index = index - 1;
    else
        temp_index = index;

    int x = temp_index ? m_tabWidths.at(temp_index - 1) : m_skin->tabBarPosition().x();
    int tabWidth = m_tabWidths.at(temp_index) - x;
    int y = m_skin->tabBarPosition().y();

    m_dropRect = QRect(x, y - height(), tabWidth, height() - y);
    QPoint pos;

    if (index < m_tabs.count())
        pos = m_dropRect.topLeft();
    else
        pos = m_dropRect.topRight();

    pos.rx() -= arrowSize/2;

    m_dropIndicator->move(mapTo(parentWidget(),pos));
    m_dropIndicator->show();

    return;
}

int TabBar::dropIndex(const QPoint pos)
{
    int index = tabAt(pos.x());
    if (index < 0)
        return index;

    int x = index ? m_tabWidths.at(index - 1) : m_skin->tabBarPosition().x();
    int tabWidth = m_tabWidths.at(index) - x;
    int y = m_skin->tabBarPosition().y();
    m_dropRect = QRect(x, y - height(), tabWidth, height() - y);

    if ((pos.x()-m_dropRect.left()) > (m_dropRect.width()/2))
        ++index;

    if (index == m_tabs.count())
        return -1;

    return index;
}

bool TabBar::isSameTab(const QDropEvent* event)
{
    int index = dropIndex(event->pos());
    int sourceSessionId = event->mimeData()->text().toInt();
    int sourceIndex = m_tabs.indexOf(sourceSessionId);

    bool isLastTab = (sourceIndex == m_tabs.count()-1) && (index == -1);

    if ((sourceIndex == index) || (sourceIndex == index-1) || isLastTab)
        return true;
    else
        return false;
}
