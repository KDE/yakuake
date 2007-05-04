/*
    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.
*/

/*
  Copyright (C) 2007 Eike Hein <hein@kde.org>
*/


#ifndef SKIN_LIST_ITEM_H
#define SKIN_LIST_ITEM_H


#include "klistview.h"


#define MARGIN 3


class QSimpleRichText;

class SkinListItem : public KListViewItem
{
    public:
        explicit SkinListItem(KListView* parent, const QString& fancy_name,
            const QString& author, const QPixmap& icon, const QString& name, const QString& dir);
        ~SkinListItem();

        void setAuthor(const QString& author);
        QString author();

        void setName(const QString& name);
        QString name();

        void setDir(const QString& dir);
        QString dir();

        void setup();
        void paintCell(QPainter* p, const QColorGroup& cg, int column, int width, int align);


    private:
        QSimpleRichText* item_text;
        QString skin_name;
        QString skin_author;
        QString skin_dir;
};


#endif /* SKIN_LIST_ITEM_H */
