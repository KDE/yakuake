/*****************************************************************************
 *                                                                           *
 *   Copyright (C) 2005 by Chazal Francois             <neptune3k@free.fr>   *
 *   website : http://workspace.free.fr                                      *
 *                                                                           *
 *                     =========  GPL License  =========                     *
 *    This program is free software; you can redistribute it and/or modify   *
 *   it under the terms of the  GNU General Public License as published by   *
 *   the  Free  Software  Foundation ; either version 2 of the License, or   *
 *   (at your option) any later version.                                     *
 *                                                                           *
 *****************************************************************************/

#ifndef IMAGE_BUTTON_H
# define IMAGE_BUTTON_H

//== INCLUDE REQUIREMENTS ===================================================//

/*
** Qt libraries */
#include <qurl.h>
#include <qcolor.h>
#include <qpoint.h>
#include <qbitmap.h>
#include <qpixmap.h>
#include <qwidget.h>
#include <qpainter.h>
#include <qpopupmenu.h>

/*
** KDE libraries */
#include <krootpixmap.h>


//== DEFINE CLASS & DATATYPES ===============================================//

/*
** Class 'ImageButton' defines a title bar object
**************************************************/

class ImageButton : public QWidget
{
    Q_OBJECT

private:

    //-- PRIVATE ATTRIBUTES ---------------------------------------------//

    /*
    ** Widget's state */
    int     state;
    bool    toggle;
    bool    pressed;


    /*
    ** Widget's mask */
    QRegion     mask;


    /*
    ** Widget's tip */
    QString     tooltip;


    /*
    ** Widget's pixmaps */
    QPixmap     up_pixmap;
    QPixmap     over_pixmap;
    QPixmap     down_pixmap;


    /*
    ** Widget's popup menu */
    QPopupMenu *    popup_menu;


    /*
    ** Widget's rootPixmap */
    KRootPixmap *   root_pixmap;



protected:

    //-- PROTECTED METHODS ----------------------------------------------//

    virtual void    enterEvent(QEvent *);
    virtual void    leaveEvent(QEvent *);

    virtual void    paintEvent(QPaintEvent *);

    virtual void    mousePressEvent(QMouseEvent *);
    virtual void    mouseReleaseEvent(QMouseEvent *);



public:

    //-- CONSTRUCTORS AND DESTRUCTORS -----------------------------------//

    ImageButton(QWidget * parent = 0, const char * name = 0);
    ~ImageButton();


    //-- PUBLIC METHODS -------------------------------------------------//

    /*
    ** Creates a translucent button */
    void    setTranslucent(bool value);


    /*
    ** Creates a toggle button */
    void    setToggleButton(bool toggled);


    /*
    ** Sets the configuration menu */
    void    setPopupMenu(QPopupMenu * menu);


    /*
    ** Sets the widget's pixmaps */
    void    setUpPixmap(const QString & path);
    void    setOverPixmap(const QString & path);
    void    setDownPixmap(const QString & path);


signals:

    //-- SIGNALS DEFINITION ---------------------------------------------//

    void    clicked();
    void    toggled(bool toggled);
};

#endif /* IMAGE_BUTTON_H */
