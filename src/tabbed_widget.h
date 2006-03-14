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

#ifndef TABBED_WIDGET_H
# define TABBED_WIDGET_H

//== INCLUDE REQUIREMENTS ===================================================//

/*
** Qt libraries */
#include <qfont.h>
#include <qcolor.h>
#include <qpoint.h>
#include <qpixmap.h>
#include <qwidget.h>
#include <qpainter.h>
#include <qlineedit.h>
#include <qvaluelist.h>
#include <qfontmetrics.h>

/*
** KDE libraries */
#include <klocale.h>
#include <krootpixmap.h>
#include <kinputdialog.h>


//== DEFINE CLASS & DATATYPES ===============================================//

/*
** Class 'TabbedWidget' defines a tabbed widget
*************************************************/

class TabbedWidget : public QWidget
{
    Q_OBJECT

private:

    //-- PRIVATE ATTRIBUTES ---------------------------------------------//

    /*
    ** Tabs properties */
    QColor      font_color;
    int         selected_id;
    QFont       selected_font;
    QFont       unselected_font;


    /*
    ** Inline renaming */
    QLineEdit * inline_edit;


    /*
    ** Widget's pixmaps */
    QPixmap     background_image;

    QPixmap     separator_image;
    QPixmap     unselected_image;

    QPixmap     selected_image;
    QPixmap     selected_left_image;
    QPixmap     selected_right_image;


    /*
    ** Widget's appearance */
    QPixmap     buffer_image;
    QPixmap     desktop_image;


    /*
    ** Tabs value lists */
    QValueList<int>     items;
    QValueList<int>     areas;
    QValueList<QString> captions;

    /*
    ** Widget's rootPixmap */
    KRootPixmap *   root_pixmap;


    //-- PRIVATE METHODS ------------------------------------------------//

    void        refreshBuffer();
    const int   drawButton(int id, QPainter & painter);



private slots:

    //-- PRIVATE SLOTS --------------------------------------------------//

    void    slotRenameSelected();
    void    slotUpdateBuffer(const QPixmap & pixmap);

    void    slotLostFocus() { inline_edit->hide(); };



protected:

    //-- PROTECTED METHODS ----------------------------------------------//

    virtual void    paintEvent(QPaintEvent *);

    virtual void    wheelEvent(QWheelEvent *);

    virtual void    mouseReleaseEvent(QMouseEvent *);



public:

    //-- CONSTRUCTORS AND DESTRUCTORS -----------------------------------//

    TabbedWidget(QWidget * parent = 0, const char * name = 0);
    ~TabbedWidget();


    //-- PUBLIC METHODS -------------------------------------------------//

    void    addItem(int id);
    void    selectItem(int id);
    int     removeItem(int id);

    void    selectNextItem();
    void    selectPreviousItem();

    void    renameItem(int id, const QString & name);

    void    setFontColor(const QColor & color);
    void    setBackgroundPixmap(const QString & path);
    void    setSeparatorPixmap(const QString & path);

    void    setUnselectedPixmap(const QString & path);

    void    setSelectedPixmap(const QString & path);
    void    setSelectedLeftPixmap(const QString & path);
    void    setSelectedRightPixmap(const QString & path);



signals:

    //-- SIGNALS DEFINITION ---------------------------------------------//

    void    itemSelected(int id);
};

#endif /* TABBED_WIDGET_H */
