/*****************************************************************************
 *                                                                           *
 *   Copyright (C) 2005 by Chazal Francois             <neptune3k@free.fr>   *
 *   website : http://workspace.free.fr                                      *
 *                                                                           *
 *                     =========  GPL Licence  =========                     *
 *    This program is free software; you can redistribute it and/or modify   *
 *   it under the terms of the  GNU General Public License as published by   *
 *   the  Free  Software  Foundation ; either version 2 of the License, or   *
 *   (at your option) any later version.                                     *
 *                                                                           *
 *****************************************************************************/

//== INCLUDE REQUIREMENTS =====================================================

/*
** Local libraries */
#include "image_button.h"
#include "image_button.moc"



//== CONSTRUCTORS AND DESTRUCTORS =============================================


ImageButton::ImageButton(QWidget * parent, const char * name) : QWidget(parent, name)
{
    state = 0;
    toggle = false;
    pressed = false;

    popup_menu = NULL;
    root_pixmap = NULL;
}

ImageButton::~ImageButton()
{
    if (root_pixmap)
        delete root_pixmap;
}



//== PUBLIC METHODS ===========================================================


/******************************************************************************
** Creates a translucent button
*********************************/

void    ImageButton::setTranslucent(bool value)
{
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


/******************************************************************************
** Sets the toggling ability
******************************/

void    ImageButton::setToggleButton(bool toggled)
{
    pressed = false;
    toggle = toggled;
}


/******************************************************************************
** Sets the configuration menu
********************************/

void    ImageButton::setPopupMenu(QPopupMenu * menu)
{
    popup_menu = menu;
}


/******************************************************************************
** Loads the 'UP' pixmap
**************************/

void    ImageButton::setUpPixmap(const QString & path)
{
    up_pixmap.load(path);
    resize(up_pixmap.size());
    setMask(*up_pixmap.mask());
}


/******************************************************************************
** Loads the 'OVER' pixmap
****************************/

void    ImageButton::setOverPixmap(const QString & path)
{
    over_pixmap.load(path);
}


/******************************************************************************
** Loads the 'DOWN' pixmap
****************************/

void    ImageButton::setDownPixmap(const QString & path)
{
    down_pixmap.load(path);
}



//== PROTECTED METHODS ========================================================


/******************************************************************************
** Modifies button's state (mouse over)
*****************************************/

void    ImageButton::enterEvent(QEvent *)
{
    state = (!pressed) ? 1 : 2;

    repaint();
}


/******************************************************************************
** Modifies button's state (mouse out)
****************************************/

void    ImageButton::leaveEvent(QEvent *)
{
    state = 0;

    repaint();
}


/******************************************************************************
** Modifies button's state (mouse down)
****************************************/

void    ImageButton::mousePressEvent(QMouseEvent *)
{
    state = 2;

    repaint();

    if (popup_menu != NULL)
        popup_menu->exec(mapToGlobal(QPoint(0, height())));
}


/******************************************************************************
** Modifies button's state (mouse up)
***************************************/

void    ImageButton::mouseReleaseEvent(QMouseEvent *)
{
    state = (toggle) ? 0 : 1;
    pressed = (toggle) ? !pressed : false;

    repaint();

    if (toggle)
        emit toggled(pressed);
    else
        emit clicked();
}


/******************************************************************************
** Repaints the widget when asked
***********************************/

void    ImageButton::paintEvent(QPaintEvent *)
{
    QPainter    painter(this);

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
