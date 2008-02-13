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


#include <tabbar.h>
#include <mainwindow.h>
#include <skin.h>

#include <KLineEdit>
#include <KMenu>
#include <KPushButton>

#include <QBitmap>
#include <QPainter>
#include <QtDBus/QtDBus>
#include <QToolButton>
#include <QWheelEvent>


TabBar::TabBar(MainWindow* mainWindow) : QWidget(mainWindow)
{
    QDBusConnection::sessionBus().registerObject("/yakuake/tabs", this, QDBusConnection::ExportScriptableSlots);

    setWhatsThis(i18nc("@info:whatsthis", 
                       "<title>Tab Bar</title>"
                       "<para>The tab bar allows you to switch between sessions. You can double-click a tab to edit its label.</para>"));

    m_mousePressed = false;
    m_mousePressedIndex = -1;

    m_mainWindow = mainWindow;
    m_skin = mainWindow->skin();

    m_tabContextMenu = new KMenu(this);
    connect(m_tabContextMenu, SIGNAL(aboutToShow()), this, SLOT(readyTabContextMenu()));

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
    connect(m_lineEdit, SIGNAL(returnPressed()), this, SLOT(renameTab()));
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
        m_tabContextMenu->addAction(m_mainWindow->actionCollection()->action("edit-profile"));
        m_tabContextMenu->addAction(m_mainWindow->actionCollection()->action("close-active-terminal"));
        m_tabContextMenu->addSeparator();
        m_tabContextMenu->addAction(m_mainWindow->actionCollection()->action("move-session-left"));
        m_tabContextMenu->addAction(m_mainWindow->actionCollection()->action("move-session-right"));
        m_tabContextMenu->addSeparator();
        m_tabContextMenu->addAction(m_mainWindow->actionCollection()->action("rename-session"));
        m_tabContextMenu->addAction(m_mainWindow->actionCollection()->action("close-session"));
    }

    updateTabContextMenuActions(m_mousePressedIndex);
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

    updateTabContextMenuActions(m_mousePressedIndex);
}

void TabBar::updateTabContextMenuActions(int index)
{
    m_mainWindow->actionCollection()->action("move-session-left")->setEnabled(false);
    m_mainWindow->actionCollection()->action("move-session-right")->setEnabled(false);

    if (index != m_tabs.indexOf(m_tabs.first()))
        m_mainWindow->actionCollection()->action("move-session-left")->setEnabled(true);

    if (index != m_tabs.indexOf(m_tabs.last()))
        m_mainWindow->actionCollection()->action("move-session-right")->setEnabled(true);
}

void TabBar::contextMenuEvent(QContextMenuEvent* event)
{
    if (event->x() < 0) return;

    int index = tabAt(event->x());

    if (index == -1)
        m_sessionMenu->exec(QCursor::pos());
    else
    {
        m_mousePressedIndex = index;

        m_tabContextMenu->exec(QCursor::pos());
    }

    QWidget::contextMenuEvent(event);
}

int TabBar::retrieveContextMenuSessionId()
{
    if (m_mousePressedIndex < 0 || m_mousePressedIndex > m_tabs.size())
    {
        m_mousePressedIndex = -1;

        return m_mousePressedIndex;
    }

    int tmpSessionId = m_tabs.at(m_mousePressedIndex);

    m_mousePressedIndex = -1;

    return tmpSessionId;
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

    if (m_mainWindow->useTranslucency())
    {
        painter.setCompositionMode(QPainter::CompositionMode_Source);
        painter.fillRect(rect(), Qt::transparent);
    }
    else
    {
        painter.setCompositionMode(QPainter::CompositionMode_SourceOver);
        painter.fillRect(rect(), Settings::backgroundColor());
    }

    QString title;
    int sessionId;
    bool selected;
    int x = m_skin->tabBarPosition().x();
    int y = m_skin->tabBarPosition().y();
    QFont font = KGlobalSettings::generalFont();
    int textWidth = 0;
    m_tabWidths.clear();

    QRect tabsClipRect(x, y, m_closeTabButton->x() - x, height() - y);
    painter.setClipRect(tabsClipRect);

    for (int index = 0; index < m_tabs.size(); ++index) 
    {
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

int TabBar::tabAt(int x)
{
    for (int index = 0; index < m_tabWidths.size(); ++index)
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

    if (event->button() == Qt::LeftButton && index != m_tabs.indexOf(m_selectedSessionId))
    {
        m_mousePressed = true;
        m_mousePressedIndex = index;
    }

    QWidget::mousePressEvent(event);
}

void TabBar::mouseReleaseEvent(QMouseEvent* event)
{
    if (QWhatsThis::inWhatsThisMode()) return;

    if (event->x() < m_skin->tabBarPosition().x()) return;

    int index = tabAt(event->x());

    if (event->button() == Qt::LeftButton && index != m_tabs.indexOf(m_selectedSessionId))
    {
        if (m_mousePressed && m_mousePressedIndex == index) 
            emit tabSelected(m_tabs.at(index));
    }

    m_mousePressed = false;

    QWidget::mouseReleaseEvent(event);
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

    QWidget::leaveEvent(event);
}

void TabBar::addTab(int sessionId)
{
    m_tabs.append(sessionId);
    m_tabTitles.insert(sessionId, standardTabTitle());

    emit tabSelected(sessionId);
}

void TabBar::removeTab(int sessionId)
{
    if (sessionId == -1) sessionId = m_selectedSessionId;

    int index = m_tabs.indexOf(sessionId);

    if (index == -1) return;

    if (m_lineEdit->isVisible() && index == m_editingSessionId)
        m_lineEdit->hide();

    m_tabs.removeAt(index);
    m_tabTitles.remove(sessionId);

    if (m_tabs.count() == 0) 
        emit newTabRequested();
    else
        emit tabSelected(m_tabs.last());
}

void TabBar::renameTab(int sessionId, const QString& newTitle)
{
    if (sessionId == -1) sessionId = m_selectedSessionId;
    if (sessionId == -1) return;
    if (!m_tabTitles.contains(sessionId)) return;

    if (newTitle.isEmpty() && !m_lineEdit->text().isEmpty())
        m_tabTitles[sessionId] = m_lineEdit->text().trimmed();
    else
        m_tabTitles[sessionId] = newTitle;

    repaint();
}

void TabBar::interactiveRename(int sessionId)
{
    if (sessionId == -1) return;
    if (!m_tabs.contains(sessionId)) return;

    int index = m_tabs.indexOf(sessionId);
    int x = index ? m_tabWidths.at(index - 1) : m_skin->tabBarPosition().x();
    int y = m_skin->tabBarPosition().y();
    int width = m_tabWidths.at(index) - x;

    m_editingSessionId = index;

    m_lineEdit->setText(m_tabTitles[sessionId]);
    m_lineEdit->setGeometry(x-1, y-1, width+3, height()+2);
    m_lineEdit->selectAll();
    m_lineEdit->setFocus();
    m_lineEdit->show();
}

void TabBar::selectTab(int sessionId)
{
    if (!m_tabs.contains(sessionId)) return;

    m_selectedSessionId = sessionId;

    updateTabContextMenuActions(m_tabs.indexOf(m_selectedSessionId));

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
    if (!m_tabs.contains(sessionId)) return;

    if (m_tabs.indexOf(sessionId) == 0) return;

    int index = m_tabs.indexOf(sessionId);

    m_tabs.swap(index, index - 1);

    repaint();
}

void TabBar::moveTabRight(int sessionId)
{
    if (sessionId == -1) sessionId = m_selectedSessionId;
    if (!m_tabs.contains(sessionId)) return;

    if (m_tabs.indexOf(sessionId) == m_tabs.count() - 1) return;

    int index = m_tabs.indexOf(sessionId);

    m_tabs.swap(index, index + 1);

    repaint();
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
    if (!newTitle.isEmpty()) renameTab(sessionId, newTitle);
}

int TabBar::sessionAtTab(int index)
{
    for (int i = 0; i < m_tabs.size(); ++i)
    {
        if (i == index) return m_tabs.at(i);
    }   

    return -1;
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
            newTitle = newTitle = makeTabTitle(count);
        }
    }
    while (!nameOk);

    return newTitle;
}

QString TabBar::makeTabTitle(int id)
{
    return i18ncp("@title:tab", "Shell", "Shell No. <numid>%1</numid>", id+1);
}
