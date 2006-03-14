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

#ifndef TABS_BAR_H
# define TABS_BAR_H

//== INCLUDE REQUIREMENTS ===================================================//

/*
** Qt libraries */
#include <qurl.h>
#include <qcolor.h>
#include <qpoint.h>
#include <qpixmap.h>
#include <qwidget.h>
#include <qpainter.h>

/*
** KDE libraries */
#include <kconfig.h>
#include <krootpixmap.h>
#include <kstandarddirs.h>

/*
** Local libraries */
#include "image_button.h"
#include "tabbed_widget.h"


//== DEFINE CLASS & DATATYPES ===============================================//

/*
** Class 'TabsBar' defines a tabs bar object
**********************************************/

class TabsBar : public QWidget
{
    Q_OBJECT

public:

    //-- PRIVATE ATTRIBUTES ---------------------------------------------//

    /*
    ** Text properties */
    QColor      text_color;


    /*
    ** Widget's pixmaps */
    QPixmap     back_image;
    QPixmap     left_corner;
    QPixmap     right_corner;


    /*
    ** Plus button */
    QPoint          plus_position;
    ImageButton *   plus_button;


    /*
    ** Minus button */
    QPoint          minus_position;
    ImageButton *   minus_button;


    /*
    ** Tabbed widget */
    QPoint          tabs_position;
    TabbedWidget *  tabs_widget;


    /*
    ** Widget's rootPixmap */
    KRootPixmap *   root_pixmap;


    //-- PRIVATE METHODS ------------------------------------------------//

    void    loadSkin(const QString & skin);



private slots:

    //-- PRIVATE SLOTS --------------------------------------------------//

    void    slotAddItem();
    void    slotRemoveItem();
    void    slotItemSelected(int id);




protected:

    //-- PROTECTED METHODS ----------------------------------------------//

    virtual void    paintEvent(QPaintEvent *);
    virtual void    resizeEvent(QResizeEvent *);



public:

    //-- CONSTRUCTORS AND DESTRUCTORS -----------------------------------//

    TabsBar(QWidget * parent = 0, const char * name = 0, const QString & skin = "default");
    ~TabsBar();


    //-- PUBLIC METHODS -------------------------------------------------//

    void    addItem(int id);
    void    selectItem(int id);
    int     removeItem(int id);

    void    renameItem(int id, const QString & name);



public slots:

    //-- PUBLIC SLOTS ---------------------------------------------------//

    void    slotSelectNextItem();
    void    slotSelectPreviousItem();



signals:

    //-- SIGNALS DEFINITION ---------------------------------------------//

    void    addItem();
    void    removeItem();
    void    itemSelected(int id);
};

#endif /* TABS_BAR_H */
