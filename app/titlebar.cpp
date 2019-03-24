/*
  Copyright (C) 2008-2014 by Eike Hein <hein@kde.org>

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


#include "titlebar.h"
#include "mainwindow.h"
#include "skin.h"

#include <QFontDatabase>
#include <QMenu>
#include <QPushButton>
#include <KLocalizedString>

#include <QApplication>
#include <QBitmap>
#include <QPainter>

TitleBar::TitleBar(MainWindow* mainWindow) : QWidget(mainWindow)
{
    setWhatsThis(xi18nc("@info:whatsthis",
                       "<title>Title Bar</title>"
                       "<para>The title bar displays the session title if available.</para>"));

    setAttribute(Qt::WA_OpaquePaintEvent);

    m_mainWindow = mainWindow;
    m_skin = mainWindow->skin();

    m_focusButton = new QPushButton(this);
    m_focusButton->setFocusPolicy(Qt::NoFocus);
    m_focusButton->setCheckable(true);
    m_focusButton->setToolTip(xi18nc("@info:tooltip", "Keep window open when it loses focus"));
    m_focusButton->setWhatsThis(xi18nc("@info:whatsthis", "If this is checked, the window will stay open when it loses focus."));
    connect(m_focusButton, SIGNAL(toggled(bool)), mainWindow, SLOT(setKeepOpen(bool)));

    m_menuButton = new QPushButton(this);
    m_menuButton->setFocusPolicy(Qt::NoFocus);
    m_menuButton->setMenu(mainWindow->menu());
    m_menuButton->setToolTip(xi18nc("@info:tooltip", "Open Menu"));
    m_menuButton->setWhatsThis(xi18nc("@info:whatsthis", "Opens the main menu."));

    m_quitButton = new QPushButton(this);
    m_quitButton->setFocusPolicy(Qt::NoFocus);
    m_quitButton->setToolTip(xi18nc("@info:tooltip Quits the application", "Quit"));
    m_quitButton->setWhatsThis(xi18nc("@info:whatsthis", "Quits the application."));
    connect(m_quitButton, SIGNAL(clicked()), mainWindow, SLOT(close()));
}

TitleBar::~TitleBar()
{
}

void TitleBar::applySkin()
{
    resize(width(), m_skin->titleBarBackgroundImage().height());

    m_focusButton->setStyleSheet(m_skin->titleBarFocusButtonStyleSheet());
    m_menuButton->setStyleSheet(m_skin->titleBarMenuButtonStyleSheet());
    m_quitButton->setStyleSheet(m_skin->titleBarQuitButtonStyleSheet());

    moveButtons();

    repaint();

    updateMask();
}

void TitleBar::moveButtons()
{
    if (m_skin->titleBarFocusButtonAnchor() == Qt::AnchorLeft)
        m_focusButton->move(m_skin->titleBarFocusButtonPosition().x(), m_skin->titleBarFocusButtonPosition().y());
    else if (m_skin->titleBarFocusButtonAnchor() == Qt::AnchorRight)
        m_focusButton->move(width() - m_skin->titleBarFocusButtonPosition().x(), m_skin->titleBarFocusButtonPosition().y());

    if (m_skin->titleBarMenuButtonAnchor() == Qt::AnchorLeft)
        m_menuButton->move(m_skin->titleBarMenuButtonPosition().x(), m_skin->titleBarMenuButtonPosition().y());
    else if (m_skin->titleBarMenuButtonAnchor() == Qt::AnchorRight)
        m_menuButton->move(width() - m_skin->titleBarMenuButtonPosition().x(), m_skin->titleBarMenuButtonPosition().y());

    if (m_skin->titleBarQuitButtonAnchor() == Qt::AnchorLeft)
        m_quitButton->move(m_skin->titleBarQuitButtonPosition().x(), m_skin->titleBarQuitButtonPosition().y());
    else if (m_skin->titleBarQuitButtonAnchor() == Qt::AnchorRight)
        m_quitButton->move(width() - m_skin->titleBarQuitButtonPosition().x(), m_skin->titleBarQuitButtonPosition().y());
}

void TitleBar::resizeEvent(QResizeEvent* event)
{
    moveButtons();

    updateMask();

    QWidget::resizeEvent(event);
}

void TitleBar::paintEvent(QPaintEvent*)
{
    QPainter painter(this);
    painter.setPen(m_skin->titleBarTextColor());

    const QPixmap& backgroundImage = m_skin->titleBarBackgroundImage();
    const QPixmap& leftCornerImage = m_skin->titleBarLeftCornerImage();
    const QPixmap& rightCornerImage = m_skin->titleBarRightCornerImage();

    painter.drawTiledPixmap(leftCornerImage.width(), 0,
                            width() - leftCornerImage.width() - rightCornerImage.width(), height(),
                            backgroundImage);

    painter.drawPixmap(0, 0, leftCornerImage);
    painter.drawPixmap(width() - rightCornerImage.width(), 0, rightCornerImage);

    QFont font = QFontDatabase::systemFont(QFontDatabase::TitleFont);
    font.setBold(m_skin->titleBarTextBold());
    painter.setFont(font);

    const QString title = this->title();
    if (m_skin->titleBarTextCentered() && width() > m_skin->titleBarTextPosition().x() + painter.fontMetrics().width(title) + m_focusButton->width() + m_quitButton->width() + m_menuButton->width())
        painter.drawText(0, 0, width(), height(), Qt::AlignCenter, title);
    else
        painter.drawText(m_skin->titleBarTextPosition(), title);

    painter.end();
}

void TitleBar::updateMask()
{
    const QPixmap& leftCornerImage = m_skin->titleBarLeftCornerImage();
    const QPixmap& rightCornerImage = m_skin->titleBarRightCornerImage();

    QRegion leftCornerRegion = leftCornerImage.hasAlpha() ? QRegion(leftCornerImage.mask()) : QRegion(leftCornerImage.rect());
    QRegion rightCornerRegion = rightCornerImage.hasAlpha() ? QRegion(rightCornerImage.mask()) : QRegion(rightCornerImage.rect());

    QRegion mask = leftCornerRegion;

    mask += QRegion(QRect(0, 0, width() - leftCornerImage.width() - rightCornerImage.width(),
        height())).translated(leftCornerImage.width(), 0);

    mask += rightCornerRegion.translated(width() - rightCornerImage.width(), 0);

    setMask(mask);
}

void TitleBar::setFocusButtonState(bool checked)
{
    m_focusButton->setChecked(checked);
}

QString TitleBar::title()
{
    if (!m_skin->titleBarText().isEmpty() && !m_title.isEmpty())
        return m_title + QStringLiteral(" - ") + m_skin->titleBarText();
    else if (!m_skin->titleBarText().isEmpty() && m_title.isEmpty())
        return m_skin->titleBarText();
    else
        return m_title;
}

void TitleBar::setTitle(const QString& title)
{
    m_title = title;

    repaint();
}
