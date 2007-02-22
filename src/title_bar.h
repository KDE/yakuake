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

#ifndef TITLE_BAR_H
# define TITLE_BAR_H

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
#include <qpushbutton.h>

/*
** KDE libraries */
#include <kconfig.h>
#include <kpopupmenu.h>
#include <kapplication.h>
#include <kstandarddirs.h>

/*
** Local libraries */
#include "image_button.h"


//== DEFINE CLASS & DATATYPES ===============================================//

/*
** Class 'TitleBar' defines a title bar object
***********************************************/

class TitleBar : public QWidget
{
    Q_OBJECT

private:

    //-- PRIVATE ATTRIBUTES ---------------------------------------------//

    /*
    ** Widget's mask */
    QRegion     mask;


    /*
    ** Text properties */
    QString     text_value;
    QString     text_title;
    QColor      text_color;
    QPoint      text_position;


    /*
    ** Widget's pixmaps */
    QPixmap     back_image;
    QPixmap     left_corner;
    QPixmap     right_corner;


    /*
    ** Quit button */
    QPoint          quit_position;
    ImageButton *   quit_button;


    /*
    ** Focus button */
    QPoint          focus_position;
    ImageButton *   focus_button;


    /*
    ** Configure button */
    QPoint          config_position;
    ImageButton *   config_button;


    //-- PRIVATE METHODS ------------------------------------------------//

    void    loadSkin(const QString & skin);
    void    updateWidgetMask();



protected:

    //-- PROTECTED METHODS ----------------------------------------------//

    virtual void    paintEvent(QPaintEvent *);
    virtual void    resizeEvent(QResizeEvent *);



public:

    //-- CONSTRUCTORS AND DESTRUCTORS -----------------------------------//

    explicit TitleBar(QWidget * parent = 0, const char * name = 0, const QString & skin = "default");
    ~TitleBar();


    //-- PUBLIC METHODS -------------------------------------------------//

    /*
    ** Gets the title bar's mask */
    QRegion &   getWidgetMask();


    /*
    ** Sets the title text */
    void        setTitleText(const QString & text);


    /*
    ** Sets the configuration menu */
    void        setConfigurationMenu(KPopupMenu * menu);
};

#endif /* TITLE_BAR_H */
