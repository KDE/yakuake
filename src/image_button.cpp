/*
    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.
*/

/*
  Copyright (C) 2005 Francois Chazal <neptune3k@free.fr>
  Copyright (C) 2006-2007 Eike Hein <hein@kde.org>
*/


#include "image_button.h"
#include "image_button.moc"


#include <qwhatsthis.h>
#include <qtimer.h>

#include <kglobalsettings.h>


ImageButton::ImageButton(QWidget * parent, const char * name) : QWidget(parent, name)
{
    state = 0;
    toggle = false;
    pressed = false;
    delay_popup = false;

    popup_menu = NULL;
    popup_timer = NULL;

    root_pixmap = NULL;
}

ImageButton::~ImageButton()
{
    if (root_pixmap) delete root_pixmap;
}

void ImageButton::setTranslucent(bool value)
{
    /* Creates a translucent button. */

    if (root_pixmap == NULL)
        root_pixmap = new KRootPixmap(this, "Transparent background");

    if (value)
    {
        root_pixmap->start();
        setMask(QRegion(up_pixmap.rect()));
    }
    else
    {
        root_pixmap->stop();
        setMask(*up_pixmap.mask());
    }
}

void ImageButton::setToggleButton(bool toggled)
{
    /* Sets the toggling ability. */

    pressed = false;
    toggle = toggled;
}

void ImageButton::setToggled(bool enable)
{
    if (toggle)
    {
        state = 0;

        if (enable)
            pressed = true;
        else
            pressed = false;

        repaint();
    }
}

void ImageButton::setPopupMenu(QPopupMenu* menu)
{
    popup_menu = menu;
    popup_timer = new QTimer(this);
    connect(popup_timer, SIGNAL(timeout()), this, SLOT(showPopupMenu()));
}

void ImageButton::showPopupMenu()
{
    popup_menu->exec(mapToGlobal(QPoint(0, height())));
}

void ImageButton::setUpPixmap(const QString& path)
{
    up_pixmap.load(path);
    resize(up_pixmap.size());

    if (up_pixmap.hasAlphaChannel()) setMask(*up_pixmap.mask());
}

void ImageButton::setOverPixmap(const QString& path)
{
    over_pixmap.load(path);
}

void ImageButton::setDownPixmap(const QString& path)
{
    down_pixmap.load(path);
}

void ImageButton::enterEvent(QEvent*)
{
    state = pressed ? 2 : 1;

    repaint();
}

void ImageButton::leaveEvent(QEvent*)
{
    state = 0;

    if (popup_timer) popup_timer->stop();

    repaint();
}

void ImageButton::mousePressEvent(QMouseEvent*)
{
    if (QWhatsThis::inWhatsThisMode()) return;

    state = 2;

    if (popup_timer) popup_timer->stop();

    repaint();

    if (popup_menu)
    {
        if (delay_popup)
            popup_timer->start(800, true);
        else
            popup_menu->exec(mapToGlobal(QPoint(0, height())));
    }
}

void ImageButton::mouseReleaseEvent(QMouseEvent*)
{
    if (QWhatsThis::inWhatsThisMode()) return;

    // Don't process event if press and release didn't
    // occur within the button.
    if (!state > 0) return;

    state = toggle ? 0 : 1;
    pressed = toggle ? !pressed : false;

    if (popup_timer) popup_timer->stop();

    repaint();

    if (toggle)
        emit toggled(pressed);
    else
        emit clicked();
}

void ImageButton::paintEvent(QPaintEvent*)
{
    QPainter painter(this);

    erase();

    switch (state)
    {
        case 0:
            if (pressed)
                painter.drawPixmap(0, 0, down_pixmap);
            else
                painter.drawPixmap(0, 0, up_pixmap);
            break;

        case 1:
            painter.drawPixmap(0, 0, over_pixmap);
            break;

        case 2:
            painter.drawPixmap(0, 0, down_pixmap);
            break;
    }

    painter.end();
}

void ImageButton::slotUpdateBackground()
{
    if (root_pixmap) root_pixmap->repaint(true);
}
