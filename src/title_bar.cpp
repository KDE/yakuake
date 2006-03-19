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
#include "title_bar.h"
#include "title_bar.moc"



//== CONSTRUCTORS AND DESTRUCTORS =============================================


TitleBar::TitleBar(QWidget * parent, const char * name, const QString & skin) : QWidget(parent, name)
{
    loadSkin(skin);

    // Connects slots to signals --------------------------

    connect(focus_button, SIGNAL(toggled(bool)), parent, SLOT(slotSetFocusPolicy(bool)));
    connect(quit_button, SIGNAL(clicked()), parent, SLOT(close()));
}

TitleBar::~TitleBar()
{
    delete config_button;
    delete focus_button;
    delete quit_button;
}



//== PUBLIC METHODS ===========================================================


/******************************************************************************
** Gets the title bar's mask
******************************/

QRegion &   TitleBar::getWidgetMask()
{
    return mask;
}


/******************************************************************************
** Sets the title bar's text
******************************/

void        TitleBar::setTitleText(const QString & text)
{
    text_value = text + text_title;
    repaint();
}


/******************************************************************************
** Sets the configuration menu
********************************/

void    TitleBar::setConfigurationMenu(KPopupMenu * menu)
{
    config_button->setPopupMenu(menu);
}



//== PROTECTED METHODS ========================================================


/******************************************************************************
** Repaints the widget when asked
***********************************/

void    TitleBar::paintEvent(QPaintEvent *)
{
    QPainter    painter(this);
    QFont       font(painter.font());

    painter.drawTiledPixmap(0, 0, width(), height(), back_image);

    painter.drawPixmap(0, 0, left_corner);
    painter.drawPixmap(width() - right_corner.width(), 0, right_corner);

    font.setBold(true);
    painter.setFont(font);
    painter.setPen(text_color);
    painter.drawText(text_position, text_value);

    painter.end();
}


/******************************************************************************
** Adjusts the widget's components when resized
*************************************************/

void    TitleBar::resizeEvent(QResizeEvent *)
{
    updateWidgetMask();

    config_button->move(width() - config_position.x(), config_position.y());
    focus_button->move(width() - focus_position.x(), focus_position.y());
    quit_button->move(width() - quit_position.x(), quit_position.y());
}



//== PRIVATE METHODS ==========================================================


/******************************************************************************
** Initializes the skin objects
*********************************/

void    TitleBar::loadSkin(const QString & skin)
{
    QUrl    url(locate("appdata", skin + "/title.skin"));
    KConfig config(url.path());

    // Loads the text informations ------------------------

    config.setGroup("Text");

    text_title = text_value = config.readEntry("text", 0);

    text_color.setRgb(config.readNumEntry("red", 0),
                      config.readNumEntry("green", 0),
                      config.readNumEntry("blue", 0));

    text_position.setX(config.readNumEntry("x", 0));
    text_position.setY(config.readNumEntry("y", 0));

    // Loads the background pixmaps -----------------------

    config.setGroup("Background");

    back_image.load(url.dirPath() + config.readEntry("back_image", ""));
    left_corner.load(url.dirPath() + config.readEntry("left_corner", ""));
    right_corner.load(url.dirPath() + config.readEntry("right_corner", ""));

    // Loads the stay button ------------------------------

    config.setGroup("FocusButton");

    focus_button = new ImageButton(this, "Focus button");
    focus_button->setToggleButton(true);

    focus_position.setX(config.readNumEntry("x", 0));
    focus_position.setY(config.readNumEntry("y", 0));
    focus_button->setUpPixmap(url.dirPath() + config.readEntry("up_image", ""));
    focus_button->setOverPixmap(url.dirPath() + config.readEntry("over_image", ""));
    focus_button->setDownPixmap(url.dirPath() + config.readEntry("down_image", ""));

    // Loads the configuration button ---------------------

    config.setGroup("ConfigButton");

    config_button = new ImageButton(this, "Configuration button");

    config_position.setX(config.readNumEntry("x", 0));
    config_position.setY(config.readNumEntry("y", 0));
    config_button->setUpPixmap(url.dirPath() + config.readEntry("up_image", ""));
    config_button->setOverPixmap(url.dirPath() + config.readEntry("over_image", ""));
    config_button->setDownPixmap(url.dirPath() + config.readEntry("down_image", ""));

    // Loads the quit button ------------------------------

    config.setGroup("QuitButton");

    quit_button = new ImageButton(this, "Quit button");

    quit_position.setX(config.readNumEntry("x", 0));
    quit_position.setY(config.readNumEntry("y", 0));
    quit_button->setUpPixmap(url.dirPath() + config.readEntry("up_image", ""));
    quit_button->setOverPixmap(url.dirPath() + config.readEntry("over_image", ""));
    quit_button->setDownPixmap(url.dirPath() + config.readEntry("down_image", ""));

    // Resizes the widgets --------------------------------

    resize(width(), back_image.height());
}


/******************************************************************************
** Updates the widget's mask
******************************/

void    TitleBar::updateWidgetMask()
{
    QRegion     temp_mask;

    mask = QRegion(*left_corner.mask());
    mask += QRegion(QRect(QPoint(left_corner.width(), 0), QPoint(width() - right_corner.width() - 1, height() - 1)));

    temp_mask = QRegion(*right_corner.mask());
    temp_mask.translate(width() - right_corner.width(), 0);
    mask += temp_mask;
}
