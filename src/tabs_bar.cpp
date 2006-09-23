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

//== INCLUDE REQUIREMENTS =====================================================

/*
** Local libraries */
#include "tabs_bar.h"
#include "tabs_bar.moc"


//== CONSTRUCTORS AND DESTRUCTORS =============================================


TabsBar::TabsBar(QWidget * parent, const char * name, const QString & skin) : QWidget(parent, name)
{
    loadSkin(skin);

    // Connects slots to signals --------------------------

    connect(plus_button, SIGNAL(clicked()), this, SLOT(slotAddItem()));
    connect(minus_button, SIGNAL(clicked()), this, SLOT(slotRemoveItem()));
    connect(tabs_widget, SIGNAL(itemSelected(int)), this, SLOT(slotItemSelected(int)));
}

TabsBar::~TabsBar()
{
    delete root_pixmap;
    delete tabs_widget;
    delete plus_button;
    delete minus_button;
}



//== PUBLIC METHODS ===========================================================


/******************************************************************************
** Adds an item to the tabs
*****************************/

void    TabsBar::addItem(int id)
{
    tabs_widget->addItem(id);
    tabs_widget->selectItem(id);
}


/******************************************************************************
** Selects an item in the tabs
********************************/

void    TabsBar::selectItem(int id)
{
    tabs_widget->selectItem(id);
}


/******************************************************************************
** Removes an item from the tabs
**********************************/

int     TabsBar::removeItem(int id)
{
    return tabs_widget->removeItem(id);
}


/******************************************************************************
** Renames an item given its id
*********************************/

void    TabsBar::renameItem(int id, const QString & name)
{
    tabs_widget->renameItem(id, name);
}


/******************************************************************************
** Open inline edit for the current item
******************************************/

void    TabsBar::interactiveRename()
{
    tabs_widget->interactiveRename();
}


//== PRIVATE SLOTS ============================================================


/******************************************************************************
** Selects the new item in the list
*************************************/

void    TabsBar::slotSelectNextItem()
{
    tabs_widget->selectNextItem();
}


/******************************************************************************
** Selects the previous item in the list
******************************************/

void    TabsBar::slotSelectPreviousItem()
{
    tabs_widget->selectPreviousItem();
}



//== PROTECTED METHODS ========================================================


/******************************************************************************
** Repaints the widget when asked
***********************************/

void    TabsBar::paintEvent(QPaintEvent *)
{
    QPainter    painter(this);

    painter.drawPixmap(0, 0, left_corner);
    painter.drawPixmap(width() - right_corner.width(), 0, right_corner);

    painter.drawTiledPixmap(left_corner.width(), 0, width() -
            left_corner.width() -right_corner.width(), height(), back_image);

    painter.end();
}


/******************************************************************************
** Adjusts the widget's components when resized
*************************************************/

void    TabsBar::resizeEvent(QResizeEvent *)
{
    plus_button->move(plus_position.x(), plus_position.y());
    minus_button->move(width() - minus_position.x(), minus_position.y());

    tabs_widget->move(tabs_position.x(), tabs_position.y());
    tabs_widget->resize(width() - 2 * tabs_position.x(), tabs_widget->height());

}



//== PRIVATE METHODS ==========================================================


/******************************************************************************
** Initializes the skin objects
*********************************/

void    TabsBar::loadSkin(const QString & skin)
{
    QUrl    url(locate("appdata", skin + "/tabs.skin"));
    KConfig config(url.path());

    // Initializes the background pixmap ------------------

    root_pixmap = new KRootPixmap(this, "Transparent background");
    root_pixmap->start();

    // Loads the text information -------------------------

    config.setGroup("Tabs");

    tabs_position.setX(config.readNumEntry("x", 20));
    tabs_position.setY(config.readNumEntry("y", 0));

    tabs_widget = new TabbedWidget(this, "Tabbed widget");

    tabs_widget->setFontColor(QColor(config.readNumEntry("red", 255),
                                config.readNumEntry("green", 255),
                                config.readNumEntry("blue", 255)));

    tabs_widget->setSeparatorPixmap(url.dirPath() + config.readEntry("separator_image", ""));
    tabs_widget->setSelectedPixmap(url.dirPath() + config.readEntry("selected_background", ""));
    tabs_widget->setSelectedLeftPixmap(url.dirPath() + config.readEntry("selected_left_corner", ""));
    tabs_widget->setSelectedRightPixmap(url.dirPath() + config.readEntry("selected_right_corner", ""));
    tabs_widget->setUnselectedPixmap(url.dirPath() + config.readEntry("unselected_background", ""));

    // Loads the background pixmaps -----------------------

    config.setGroup("Background");

    tabs_widget->setBackgroundPixmap(url.dirPath() + config.readEntry("back_image", ""));

    back_image.load(url.dirPath() + config.readEntry("back_image", ""));
    left_corner.load(url.dirPath() + config.readEntry("left_corner", ""));
    right_corner.load(url.dirPath() + config.readEntry("right_corner", ""));

    // Loads the plus button ------------------------------

    config.setGroup("PlusButton");

    plus_button = new ImageButton(this, "Plus button");

    plus_position.setX(config.readNumEntry("x", 2));
    plus_position.setY(config.readNumEntry("y", 2));

    plus_button->setUpPixmap(url.dirPath() + config.readEntry("up_image", ""));
    plus_button->setOverPixmap(url.dirPath() + config.readEntry("over_image", ""));
    plus_button->setDownPixmap(url.dirPath() + config.readEntry("down_image", ""));

    plus_button->setTranslucent(true);

    // Loads the minus button ------------------------------

    config.setGroup("MinusButton");

    minus_button = new ImageButton(this, "Minus button");

    minus_position.setX(config.readNumEntry("x", 18));
    minus_position.setY(config.readNumEntry("y", 2));

    minus_button->setUpPixmap(url.dirPath() + config.readEntry("up_image", ""));
    minus_button->setOverPixmap(url.dirPath() + config.readEntry("over_image", ""));
    minus_button->setDownPixmap(url.dirPath() + config.readEntry("down_image", ""));

    minus_button->setTranslucent(true);

    // Resizes the widgets --------------------------------

    resize(width(), back_image.height());
}



//== PRIVATE SLOTS ============================================================


/******************************************************************************
** Receives the activation of the 'PLUS' button
*************************************************/

void    TabsBar::slotAddItem()
{
    emit addItem();
}


/******************************************************************************
** Receives the selection of an item in the list
**************************************************/

void    TabsBar::slotItemSelected(int id)
{
    emit itemSelected(id);
}


/******************************************************************************
** Receives the activation of the 'MINUS' button
**************************************************/

void    TabsBar::slotRemoveItem()
{
    emit    removeItem();
}
