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


#include "title_bar.h"
#include "title_bar.moc"

#include <qtooltip.h>
#include <qwhatsthis.h>

#include <klocale.h>


TitleBar::TitleBar(QWidget * parent, const char * name, const QString & skin) : QWidget(parent, name)
{
    QWhatsThis::add(this, i18n("The title bar displays the session title if available."));

    loadSkin(skin);

    connect(focus_button, SIGNAL(toggled(bool)), parent, SLOT(slotSetFocusPolicy(bool)));
    connect(quit_button, SIGNAL(clicked()), parent, SLOT(close()));
}

TitleBar::~TitleBar()
{
    delete config_button;
    delete focus_button;
    delete quit_button;
}

QRegion& TitleBar::getWidgetMask()
{
    return mask;
}

void TitleBar::setTitleText(const QString& title)
{
    if (title == title_text)
        return;

    title_text = title;
    repaint();
}

void TitleBar::setFocusButtonEnabled(bool enable)
{
    focus_button->setToggled(enable);
}

void TitleBar::setConfigurationMenu(KPopupMenu* menu)
{
    config_button->setPopupMenu(menu);
}

void TitleBar::paintEvent(QPaintEvent*)
{
    QPainter painter(this);
    QFont font(painter.font());

    painter.drawTiledPixmap(0, 0, width(), height(), back_image);

    painter.drawPixmap(0, 0, left_corner);
    painter.drawPixmap(width() - right_corner.width(), 0, right_corner);

    // Add " - " divider between title and skin text.
    QString title_postfix(skin_text);
    if (!title_postfix.isEmpty() && !title_text.isEmpty())
        title_postfix.prepend(" - ");

    font.setBold(true);
    painter.setFont(font);
    painter.setPen(text_color);
    painter.drawText(text_position, title_text + title_postfix);

    painter.end();
}

void TitleBar::resizeEvent(QResizeEvent *)
{
    updateWidgetMask();

    config_button->move(width() - config_position.x(), config_position.y());
    focus_button->move(width() - focus_position.x(), focus_position.y());
    quit_button->move(width() - quit_position.x(), quit_position.y());
}

void TitleBar::loadSkin(const QString& skin)
{
    focus_button = new ImageButton(this, "Focus button");
    focus_button->setToggleButton(true);
    QToolTip::add(focus_button, i18n("Keep open when focus is lost"));

    config_button = new ImageButton(this, "Configuration button");
    QToolTip::add(config_button, i18n("Open Menu"));

    quit_button = new ImageButton(this, "Quit button");
    QToolTip::add(quit_button, i18n("Quit"));

    setPixmaps(skin);

    resize(width(), back_image.height());
}

void TitleBar::reloadSkin(const QString& skin)
{
    setPixmaps(skin);

    resize(width(), back_image.height());

    config_button->move(width() - config_position.x(), config_position.y());
    focus_button->move(width() - focus_position.x(), focus_position.y());
    quit_button->move(width() - quit_position.x(), quit_position.y());

    updateWidgetMask();

    repaint();
}

void TitleBar::setPixmaps(const QString& skin)
{
    /* Initialize the skin objects. */

    QUrl url(locate("appdata", skin + "/title.skin"));
    KConfig config(url.path());

    // Load the text information.
    config.setGroup("Text");

    skin_text = config.readEntry("text", 0);

    text_color.setRgb(config.readNumEntry("red", 0),
                      config.readNumEntry("green", 0),
                      config.readNumEntry("blue", 0));

    text_position.setX(config.readNumEntry("x", 0));
    text_position.setY(config.readNumEntry("y", 0));

    // Load the background pixmaps.
    config.setGroup("Background");

    back_image.load(url.dirPath() + config.readEntry("back_image", ""));
    left_corner.load(url.dirPath() + config.readEntry("left_corner", ""));
    right_corner.load(url.dirPath() + config.readEntry("right_corner", ""));

    // Load the stay button.
    config.setGroup("FocusButton");

    focus_position.setX(config.readNumEntry("x", 0));
    focus_position.setY(config.readNumEntry("y", 0));
    focus_button->setUpPixmap(url.dirPath() + config.readEntry("up_image", ""));
    focus_button->setOverPixmap(url.dirPath() + config.readEntry("over_image", ""));
    focus_button->setDownPixmap(url.dirPath() + config.readEntry("down_image", ""));

    // Load the configuration button.
    config.setGroup("ConfigButton");

    config_position.setX(config.readNumEntry("x", 0));
    config_position.setY(config.readNumEntry("y", 0));
    config_button->setUpPixmap(url.dirPath() + config.readEntry("up_image", ""));
    config_button->setOverPixmap(url.dirPath() + config.readEntry("over_image", ""));
    config_button->setDownPixmap(url.dirPath() + config.readEntry("down_image", ""));

    // Load the quit button.
    config.setGroup("QuitButton");

    quit_position.setX(config.readNumEntry("x", 0));
    quit_position.setY(config.readNumEntry("y", 0));
    quit_button->setUpPixmap(url.dirPath() + config.readEntry("up_image", ""));
    quit_button->setOverPixmap(url.dirPath() + config.readEntry("over_image", ""));
    quit_button->setDownPixmap(url.dirPath() + config.readEntry("down_image", ""));
}

void TitleBar::updateWidgetMask()
{
    QRegion temp_mask;

    mask = QRegion(*left_corner.mask());
    mask += QRegion(QRect(QPoint(left_corner.width(), 0), QPoint(width() - right_corner.width() - 1, height() - 1)));

    temp_mask = QRegion(*right_corner.mask());
    temp_mask.translate(width() - right_corner.width(), 0);
    mask += temp_mask;
}
