/*
  Copyright (C) 2008 by Eike Hein <hein@kde.org>

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License as
  published by the Free Software Foundation; either version 2 of
  the License or (at your option) version 3 or any later version
  accepted by the membership of KDE e.V. (or its successor appro-
  ved by the membership of KDE e.V.), which shall act as a proxy
  defined in Section 14 of version 3 of the license.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program. If not, see http://www.gnu.org/licenses/.
*/


#include "skinlistdelegate.h"
#include "appearancesettings.h"

#include <QApplication>
#include <QModelIndex>
#include <QPainter>


#define MARGIN 3
#define ICON 32
#define KNS_ICON_SIZE (ICON / 2)


SkinListDelegate::SkinListDelegate(QObject* parent) : QAbstractItemDelegate(parent)
{
}

SkinListDelegate::~SkinListDelegate()
{
}

void SkinListDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex &index) const
{
    painter->save();

    paintBackground(painter, option);

    paintIcon(painter, option, index);

    paintText(painter, option, index);

    painter->restore();
}

void SkinListDelegate::paintBackground(QPainter* painter, const QStyleOptionViewItem& option) const
{
    QStyleOptionViewItemV4 opt = option;
    QStyle* style = opt.widget ? opt.widget->style() : QApplication::style();
    style->drawPrimitive(QStyle::PE_PanelItemViewItem, &opt, painter, opt.widget);
}

void SkinListDelegate::paintIcon(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex &index) const
{
    QVariant value;

    value = index.data(AppearanceSettings::SkinIcon);

    if (value.isValid() && value.type() == QVariant::Icon)
    {
        int x = option.rect.x() + MARGIN;
        int y = option.rect.y() + (option.rect.height() / 2) - (ICON / 2);

        if (option.direction == Qt::RightToLeft)
            x = option.rect.right() - ICON - MARGIN;

        qvariant_cast<QIcon>(value).paint(painter, x, y, ICON, ICON);
    }
}

void SkinListDelegate::paintText(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex &index) const
{
    QFont font = option.font;
    int initialY = option.rect.y();
    int x = option.rect.x() + ICON + (3 * MARGIN);
    int y = initialY;
    int textWidth = 0;
    int width = option.rect.width() - ICON - (3 * MARGIN);

    if (option.state & QStyle::State_Selected)
        painter->setPen(option.palette.color(QPalette::HighlightedText));

    QVariant value;

    value = index.data(AppearanceSettings::SkinName);

    if (value.isValid())
    {
        font.setBold(true);
        painter->setFont(font);
        QFontMetrics fontMetrics(font);

        QRect textRect = fontMetrics.boundingRect(value.toString());
        textWidth = textRect.width();

        if (option.direction == Qt::RightToLeft)
        {
            if (width < textWidth)
                x = option.rect.x() + MARGIN;
            else
                x = option.rect.right() - ICON - (3 * MARGIN) - textWidth;
        }

        y += textRect.height();

        painter->drawText(x, y, fontMetrics.elidedText(value.toString(), option.textElideMode, width));

        value = index.data(AppearanceSettings::SkinInstalledWithKns);

        if (value.isValid() && value.toBool())
        {
            KIcon ghnsIcon("get-hot-new-stuff");
            int knsIconX = x;
            int iconSize = qMin(textRect.height(), KNS_ICON_SIZE);

            if (option.direction == Qt::RightToLeft)
                // In RTL mode we have to correct our position
                // so there's room for the icon and enough space
                // between the text and the icon.
                knsIconX -= (iconSize + MARGIN);
            else
                // Otherwise we just have to make sure that we
                // start painting after the text and add some margin.
                knsIconX += textWidth + MARGIN;

            ghnsIcon.paint(painter, knsIconX, initialY + MARGIN, iconSize, iconSize);
        }
    }

    value = index.data(AppearanceSettings::SkinAuthor);

    if (value.isValid())
    {
        QString skinAuthor = i18nc("@item:intext", "by %1", value.toString());

        font.setBold(false);
        painter->setFont(font);
        QFontMetrics fontMetrics(font);

        QRect textRect = fontMetrics.boundingRect(skinAuthor);

        if (option.direction == Qt::RightToLeft)
        {
            if (width < textRect.width())
                x = option.rect.x() + MARGIN;
            else
                x = option.rect.right() - ICON - (3 * MARGIN) - textRect.width();
        }

        y += textRect.height();

        painter->drawText(x, y, fontMetrics.elidedText(skinAuthor, option.textElideMode, width));
    }
}

QSize SkinListDelegate::sizeHint(const QStyleOptionViewItem&option, const QModelIndex& index) const
{
    QFont font = option.font;
    QRect name, author;
    int width, height;

    QVariant value;

    value = index.data(AppearanceSettings::SkinName);

    if (value.isValid())
    {
        font.setBold(true);
        QFontMetrics fontMetrics(font);
        name = fontMetrics.boundingRect(value.toString());
    }

    value = index.data(Qt::UserRole + 1);

    if (value.isValid())
    {
        QString skinAuthor = i18nc("@item:intext", "by %1", value.toString());

        font.setBold(false);
        QFontMetrics fontMetrics(font);
        author = fontMetrics.boundingRect(skinAuthor);
    }

    width = qMax(name.width(), author.width());
    QRect textRect(0, 0, width, name.height() + author.height());

    width = ICON + (3 * MARGIN) + textRect.width() + MARGIN;;
    height = qMax(ICON + (2 * MARGIN), textRect.height() + (2 * MARGIN));

    return QSize(width, height);
}
