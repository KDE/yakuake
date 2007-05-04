/*
    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.
*/

/*
  Copyright (C) 2007 Eike Hein <hein@kde.org>
*/


#include "skin_list_item.h"

#include <qsimplerichtext.h>
#include <qrect.h>
#include <qpainter.h>
#include <qpixmap.h>
#include <qpalette.h>

#include <kglobalsettings.h>
#include <klocale.h>


SkinListItem::SkinListItem(KListView* parent, const QString& fancy_name,
    const QString& author, const QPixmap& icon, const QString& name, const QString& dir)
    : KListViewItem(parent, fancy_name)
{
    setName(name);
    setAuthor(author);
    setDir(dir);

    QString fancy_author = i18n("by %1").arg(author);
    QString text = QString("<qt><b>%1</b><br>%2</qt>").arg(fancy_name).arg(fancy_author);

    item_text = new QSimpleRichText(text, listView()->font());
    item_text->adjustSize();

    setPixmap(0, icon);

}

SkinListItem::~SkinListItem()
{
}

void SkinListItem::setName(const QString& name)
{
    skin_name = name;
}

QString SkinListItem::name()
{
    return skin_name;
}

void SkinListItem::setAuthor(const QString& author)
{
    skin_author = author;
}

QString SkinListItem::author()
{
    return skin_author;
}

void SkinListItem::setDir(const QString& dir)
{
    skin_dir = dir;
}

QString SkinListItem::dir()
{
    return skin_dir;
}

void SkinListItem::setup()
{
    widthChanged();

    item_text->setDefaultFont(listView()->font());
    item_text->setWidth(listView()->columnWidth(0));
    int text_height = item_text->height()+(MARGIN*2);

    if (text_height < 32)
        setHeight(32+(MARGIN*2));
    else
        setHeight(text_height);
}

void SkinListItem::paintCell(QPainter* p, const QColorGroup& /* cg */, int /* column */, int width, int /* align */)
{
    if (width <= 0) return;

    QColor textColor = isSelected() ? KGlobalSettings::highlightedTextColor() : KGlobalSettings::textColor();
    QColor background = isSelected() ? KGlobalSettings::highlightColor() : listView()->paletteBackgroundColor();

    QColorGroup colors;
    colors.setColor(QColorGroup::Foreground, textColor);
    colors.setColor(QColorGroup::Text, textColor);
    colors.setColor(QColorGroup::Background, background);
    colors.setColor(QColorGroup::Base, background);

    p->fillRect(0, 0, width, height(), background);


    if (pixmap(0))
    {
        int y = (height() - 32) / 2;
        p->drawPixmap(MARGIN, y, *pixmap(0));
    }

    item_text->setWidth(width);
    item_text->draw(p, MARGIN+32+MARGIN+MARGIN, MARGIN, QRect(0, 0, width-MARGIN-32-MARGIN, height()), colors);
}
