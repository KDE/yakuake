/*
  Copyright (C) 2008-2009 by Eike Hein <hein@kde.org>

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

#include <KMenu>
#include <KPushButton>
#include <KGlobalSettings>
#include <KLocalizedString>

#include <QBitmap>
#include <QPainter>


TitleBar::TitleBar(MainWindow* mainWindow) : QWidget(mainWindow)
{
    setWhatsThis(i18nc("@info:whatsthis",
                       "<title>Title Bar</title>"
                       "<para>The title bar displays the session title if available.</para>"));

    setAttribute(Qt::WA_OpaquePaintEvent);

    m_mainWindow = mainWindow;
    m_skin = mainWindow->skin();

    m_focusButton = new KPushButton(this);
    m_focusButton->setFocusPolicy(Qt::NoFocus);
    m_focusButton->setCheckable(true);
    m_focusButton->setToolTip(i18nc("@info:tooltip", "Keep window open when it loses focus"));
    m_focusButton->setWhatsThis(i18nc("@info:whatsthis", "If this is checked, the window will stay open when it loses focus."));
    connect(m_focusButton, SIGNAL(toggled(bool)), mainWindow, SLOT(setKeepOpen(bool)));

    m_menuButton = new KPushButton(this);
    m_menuButton->setFocusPolicy(Qt::NoFocus);
    m_menuButton->setMenu(mainWindow->menu());
    m_menuButton->setToolTip(i18nc("@info:tooltip", "Open Menu"));
    m_menuButton->setWhatsThis(i18nc("@info:whatsthis", "Opens the main menu."));

    m_quitButton = new KPushButton(this);
    m_quitButton->setFocusPolicy(Qt::NoFocus);
    m_quitButton->setToolTip(i18nc("@info:tooltip Quits the application", "Quit"));
    m_quitButton->setWhatsThis(i18nc("@info:whatsthis", "Quits the application."));
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

    m_focusButton->move(width() - m_skin->titleBarFocusButtonPosition().x(), m_skin->titleBarFocusButtonPosition().y());
    m_menuButton->move(width() - m_skin->titleBarMenuButtonPosition().x(), m_skin->titleBarMenuButtonPosition().y());
    m_quitButton->move(width() - m_skin->titleBarQuitButtonPosition().x(), m_skin->titleBarQuitButtonPosition().y());

    repaint();

    updateMask();
}

void TitleBar::resizeEvent(QResizeEvent* event)
{
    m_focusButton->move(width() - m_skin->titleBarFocusButtonPosition().x(), m_skin->titleBarFocusButtonPosition().y());
    m_menuButton->move(width() - m_skin->titleBarMenuButtonPosition().x(), m_skin->titleBarMenuButtonPosition().y());
    m_quitButton->move(width() - m_skin->titleBarQuitButtonPosition().x(), m_skin->titleBarQuitButtonPosition().y());

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

    QFont font = KGlobalSettings::windowTitleFont();
    font.setBold(m_skin->titleBarTextBold());
    painter.setFont(font);

    painter.drawText(m_skin->titleBarTextPosition(), title());

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
        return m_title + " - " + m_skin->titleBarText();
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
