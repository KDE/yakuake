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


#include "tab_bar.h"
#include "tab_bar.moc"
#include "settings.h"

#include <qtooltip.h>
#include <qwhatsthis.h>

#include <klocale.h>
#include <kpopupmenu.h>


TabBar::TabBar(QWidget* parent, const char* name, bool translucency, const QString & skin)
    : TranslucentWidget(parent, name, translucency)
{
    loadSkin(skin);

    connect(plus_button, SIGNAL(clicked()), this, SIGNAL(addItem()));
    connect(minus_button, SIGNAL(clicked()), this, SIGNAL(removeItem()));
    connect(tabs_widget, SIGNAL(itemSelected(int)), this, SIGNAL(itemSelected(int)));
    connect(tabs_widget, SIGNAL(addItem()), this, SIGNAL(addItem()));
    connect(this, SIGNAL(updateBackground()), this, SLOT(slotUpdateBackground()));
}

TabBar::~TabBar()
{
    delete tabs_widget;
    delete plus_button;
    delete minus_button;
}

int TabBar::pressedPosition()
{
    return tabs_widget->pressedPosition();
}


void TabBar::resetPressedPosition()
{
    tabs_widget->resetPressedPosition();
}

void TabBar::addItem(int session_id)
{
    tabs_widget->addItem(session_id);
    tabs_widget->selectItem(session_id);
}

int TabBar::removeItem(int session_id)
{
    return tabs_widget->removeItem(session_id);
}

const QString TabBar::itemName(int session_id)
{
    return tabs_widget->itemName(session_id);
}

void TabBar::renameItem(int session_id, const QString& name)
{
    tabs_widget->renameItem(session_id, name);
}

void TabBar::interactiveRename()
{
    tabs_widget->interactiveRename();
}

int TabBar::tabPositionForSessionId(int session_id)
{
    return tabs_widget->tabPositionForSessionId(session_id);
}

int TabBar::sessionIdForTabPosition(int position)
{
    return tabs_widget->sessionIdForTabPosition(position);
}

void TabBar::selectItem(int session_id)
{
    tabs_widget->selectItem(session_id);
}

void TabBar::selectPosition(int position)
{
    tabs_widget->selectPosition(position);
}

void TabBar::slotSelectNextItem()
{
    tabs_widget->selectNextItem();
}

void TabBar::slotSelectPreviousItem()
{
    tabs_widget->selectPreviousItem();
}

void TabBar::slotMoveItemLeft()
{
    tabs_widget->moveItemLeft();
}

void TabBar::slotMoveItemRight()
{
    tabs_widget->moveItemRight();
}

void TabBar::paintEvent(QPaintEvent*)
{
    QPainter painter(this);

    erase();

    if (!useTranslucency())
        painter.fillRect(0, 0, width(), height(), Settings::skinbgcolor());

    painter.drawPixmap(0, 0, left_corner);
    painter.drawPixmap(width() - right_corner.width(), 0, right_corner);

    painter.drawTiledPixmap(left_corner.width(), 0, width() -
            left_corner.width() -right_corner.width(), height(), back_image);

    painter.end();
}

void TabBar::resizeEvent(QResizeEvent*)
{
    plus_button->move(plus_position.x(), plus_position.y());
    minus_button->move(width() - minus_position.x(), minus_position.y());

    tabs_widget->move(tabs_position.x(), tabs_position.y());
    tabs_widget->resize(width() - 2 * tabs_position.x(), tabs_widget->height());
    tabs_widget->refreshBuffer();
}

void TabBar::loadSkin(const QString& skin)
{
    tabs_widget = new TabbedWidget(this, "Tabbed widget", useTranslucency());
    QWhatsThis::add(tabs_widget, i18n("The tab bar allows you to switch between sessions."));
    connect(this, SIGNAL(updateBackground()), tabs_widget, SLOT(slotUpdateBackground()));

    plus_button = new ImageButton(this, "Plus button", useTranslucency());
    plus_button->setDelayedPopup(true);
    QToolTip::add(plus_button, i18n("New Session"));
    QWhatsThis::add(plus_button, i18n("Adds a new session. Press and hold to select session type from menu."));
    connect(this, SIGNAL(updateBackground()), plus_button, SLOT(slotUpdateBackground()));

    minus_button = new ImageButton(this, "Minus button", useTranslucency());
    QToolTip::add(minus_button, i18n("Close Session"));
    QWhatsThis::add(minus_button, i18n("Closes the active session."));
    connect(this, SIGNAL(updateBackground()), minus_button, SLOT(slotUpdateBackground()));

    setPixmaps(skin);

    resize(width(), back_image.height());
}

void TabBar::reloadSkin(const QString& skin)
{
    setPixmaps(skin);

    resize(width(), back_image.height());

    plus_button->move(plus_position.x(), plus_position.y());
    minus_button->move(width() - minus_position.x(), minus_position.y());

    tabs_widget->move(tabs_position.x(), tabs_position.y());
    tabs_widget->resize(width() - 2 * tabs_position.x(), tabs_widget->height());

    minus_button->repaint();
    plus_button->repaint();
    tabs_widget->refreshBuffer();
    repaint();
}

void TabBar::setPixmaps(const QString& skin)
{
    QUrl url(locate("appdata", skin + "/tabs.skin"));
    KConfig config(url.path());

    config.setGroup("Tabs");

    tabs_position.setX(config.readNumEntry("x", 20));
    tabs_position.setY(config.readNumEntry("y", 0));

    tabs_widget->setFontColor(QColor(config.readNumEntry("red", 255),
                                config.readNumEntry("green", 255),
                                config.readNumEntry("blue", 255)));

    tabs_widget->setSeparatorPixmap(url.dirPath() + config.readEntry("separator_image", ""));
    tabs_widget->setSelectedPixmap(url.dirPath() + config.readEntry("selected_background", ""));
    tabs_widget->setSelectedLeftPixmap(url.dirPath() + config.readEntry("selected_left_corner", ""));
    tabs_widget->setSelectedRightPixmap(url.dirPath() + config.readEntry("selected_right_corner", ""));
    tabs_widget->setUnselectedPixmap(url.dirPath() + config.readEntry("unselected_background", ""));

    // Load the background pixmaps.
    config.setGroup("Background");

    tabs_widget->setBackgroundPixmap(url.dirPath() + config.readEntry("back_image", ""));

    back_image.load(url.dirPath() + config.readEntry("back_image", ""));
    left_corner.load(url.dirPath() + config.readEntry("left_corner", ""));
    right_corner.load(url.dirPath() + config.readEntry("right_corner", ""));

    // Load the plus button.
    config.setGroup("PlusButton");

    plus_position.setX(config.readNumEntry("x", 2));
    plus_position.setY(config.readNumEntry("y", 2));

    plus_button->setUpPixmap(url.dirPath() + config.readEntry("up_image", ""));
    plus_button->setOverPixmap(url.dirPath() + config.readEntry("over_image", ""));
    plus_button->setDownPixmap(url.dirPath() + config.readEntry("down_image", ""));

    // Load the minus button.
    config.setGroup("MinusButton");

    minus_position.setX(config.readNumEntry("x", 18));
    minus_position.setY(config.readNumEntry("y", 2));

    minus_button->setUpPixmap(url.dirPath() + config.readEntry("up_image", ""));
    minus_button->setOverPixmap(url.dirPath() + config.readEntry("over_image", ""));
    minus_button->setDownPixmap(url.dirPath() + config.readEntry("down_image", ""));
}

void TabBar::setSessionMenu(KPopupMenu* menu)
{
    plus_button->setPopupMenu(menu);
}
