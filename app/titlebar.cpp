/*
  SPDX-FileCopyrightText: 2008-2014 Eike Hein <hein@kde.org>
  SPDX-FileCopyrightText: 2020 Ryan McCoskrie <work@ryanmccoskrie.me>

  SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#include "titlebar.h"
#include "mainwindow.h"
#include "skin.h"

#include <KLocalizedString>
#include <QFontDatabase>
#include <QPushButton>

#include <QBitmap>
#include <QPainter>

TitleBar::TitleBar(MainWindow *mainWindow)
    : QWidget(mainWindow)
{
    setWhatsThis(xi18nc("@info:whatsthis",
                        "<title>Title Bar</title>"
                        "<para>The title bar displays the session title if available.</para>"));

    setAttribute(Qt::WA_OpaquePaintEvent);

    m_mainWindow = mainWindow;
    m_skin = mainWindow->skin();

    setCursor(Qt::SizeVerCursor);

    m_focusButton = new QPushButton(this);
    m_focusButton->setFocusPolicy(Qt::NoFocus);
    m_focusButton->setCheckable(true);
    m_focusButton->setToolTip(xi18nc("@info:tooltip", "Keep window open when it loses focus"));
    m_focusButton->setWhatsThis(xi18nc("@info:whatsthis", "If this is checked, the window will stay open when it loses focus."));
    m_focusButton->setCursor(Qt::ArrowCursor);
    connect(m_focusButton, SIGNAL(toggled(bool)), mainWindow, SLOT(setKeepOpen(bool)));

    m_menuButton = new QPushButton(this);
    m_menuButton->setFocusPolicy(Qt::NoFocus);
    m_menuButton->setMenu(mainWindow->menu());
    m_menuButton->setToolTip(xi18nc("@info:tooltip", "Open Menu"));
    m_menuButton->setWhatsThis(xi18nc("@info:whatsthis", "Opens the main menu."));
    m_menuButton->setCursor(Qt::ArrowCursor);

    m_quitButton = new QPushButton(this);
    m_quitButton->setFocusPolicy(Qt::NoFocus);
    m_quitButton->setToolTip(xi18nc("@info:tooltip Quits the application", "Quit"));
    m_quitButton->setWhatsThis(xi18nc("@info:whatsthis", "Quits the application."));
    m_quitButton->setCursor(Qt::ArrowCursor);
    connect(m_quitButton, SIGNAL(clicked()), mainWindow, SLOT(close()));
}

TitleBar::~TitleBar()
{
}

void TitleBar::setVisible(bool visible)
{
    m_visible = visible;
    if (m_visible) {
        resize(width(), m_skin->titleBarBackgroundImage().height());
    } else {
        resize(width(), 0);
    }

    QWidget::setVisible(m_visible);
}

void TitleBar::applySkin()
{
    resize(width(), m_visible ? m_skin->titleBarBackgroundImage().height() : 0);

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

void TitleBar::resizeEvent(QResizeEvent *event)
{
    moveButtons();

    updateMask();

    QWidget::resizeEvent(event);
}

void TitleBar::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.setPen(m_skin->titleBarTextColor());

    const QPixmap &backgroundImage = m_skin->titleBarBackgroundImage();
    const QPixmap &leftCornerImage = m_skin->titleBarLeftCornerImage();
    const QPixmap &rightCornerImage = m_skin->titleBarRightCornerImage();

    painter.drawTiledPixmap(leftCornerImage.width(), 0, width() - leftCornerImage.width() - rightCornerImage.width(), height(), backgroundImage);

    painter.drawPixmap(0, 0, leftCornerImage);
    painter.drawPixmap(width() - rightCornerImage.width(), 0, rightCornerImage);

    QFont font = QFontDatabase::systemFont(QFontDatabase::TitleFont);
    font.setBold(m_skin->titleBarTextBold());
    painter.setFont(font);

    const QString titleAsString = this->title();
    if (m_skin->titleBarTextCentered()
        && width() > m_skin->titleBarTextPosition().x() + painter.fontMetrics().horizontalAdvance(titleAsString) + m_focusButton->width()
                + m_quitButton->width() + m_menuButton->width())
        painter.drawText(0, 0, width(), height(), Qt::AlignCenter, titleAsString);
    else
        painter.drawText(m_skin->titleBarTextPosition(), titleAsString);

    painter.end();
}

void TitleBar::mouseMoveEvent(QMouseEvent *event)
{
    if (event->buttons() == Qt::LeftButton) {
        // Dynamic cast needed to use getDesktopGeometry()
        MainWindow *window = dynamic_cast<MainWindow *>(parent());

        int maxHeight = window->getDesktopGeometry().height();
        int newHeight = event->globalY() / (maxHeight / 100);

        // Correct newHeight if mouse is dragged too far
        if (newHeight > 100) {
            newHeight = 100;
        } else if (newHeight < 10) {
            newHeight = 10;
        }

        window->setWindowHeight(newHeight);
    } else {
        QWidget::mouseMoveEvent(event);
    }
}

void TitleBar::updateMask()
{
    const QPixmap &leftCornerImage = m_skin->titleBarLeftCornerImage();
    const QPixmap &rightCornerImage = m_skin->titleBarRightCornerImage();

    QRegion leftCornerRegion = leftCornerImage.hasAlpha() ? QRegion(leftCornerImage.mask()) : QRegion(leftCornerImage.rect());
    QRegion rightCornerRegion = rightCornerImage.hasAlpha() ? QRegion(rightCornerImage.mask()) : QRegion(rightCornerImage.rect());

    QRegion mask = leftCornerRegion;

    mask += QRegion(QRect(0, 0, width() - leftCornerImage.width() - rightCornerImage.width(), height())).translated(leftCornerImage.width(), 0);

    mask += rightCornerRegion.translated(width() - rightCornerImage.width(), 0);

    setMask(mask);
}

void TitleBar::updateMenu()
{
    m_menuButton->setMenu(m_mainWindow->menu());
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

void TitleBar::setTitle(const QString &title)
{
    m_title = title;

    repaint();
}

#include "moc_titlebar.cpp"
