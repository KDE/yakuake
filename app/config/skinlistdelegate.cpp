/*
  SPDX-FileCopyrightText: 2008 Eike Hein <hein@kde.org>

  SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#include "skinlistdelegate.h"
#include "appearancesettings.h"

#include <KLocalizedString>

#include <QApplication>
#include <QModelIndex>
#include <QPainter>

#define MARGIN 3
#define ICON 32
#define KNS_ICON_SIZE (ICON / 2)

SkinListDelegate::SkinListDelegate(QObject *parent)
    : QAbstractItemDelegate(parent)
{
}

SkinListDelegate::~SkinListDelegate()
{
}

void SkinListDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    painter->save();

    paintBackground(painter, option);

    paintIcon(painter, option, index);

    paintText(painter, option, index);

    painter->restore();
}

void SkinListDelegate::paintBackground(QPainter *painter, const QStyleOptionViewItem &option) const
{
    QStyleOptionViewItem opt = option;
    QStyle *style = opt.widget ? opt.widget->style() : QApplication::style();
    style->drawPrimitive(QStyle::PE_PanelItemViewItem, &opt, painter, opt.widget);
}

void SkinListDelegate::paintIcon(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QVariant value;

    value = index.data(AppearanceSettings::SkinIcon);

    if (value.isValid() && value.type() == QVariant::Icon) {
        int x = option.rect.x() + MARGIN;
        int y = option.rect.y() + (option.rect.height() / 2) - (ICON / 2);

        if (option.direction == Qt::RightToLeft)
            x = option.rect.right() - ICON - MARGIN;

        qvariant_cast<QIcon>(value).paint(painter, x, y, ICON, ICON);
    }
}

void SkinListDelegate::paintText(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QFont font = option.font;
    int initialY = option.rect.y();
    int x = option.rect.x() + ICON + (3 * MARGIN);
    int y = initialY;
    int width = option.rect.width() - ICON - (3 * MARGIN);

    if (option.state & QStyle::State_Selected)
        painter->setPen(option.palette.color(QPalette::HighlightedText));

    QVariant value;

    value = index.data(AppearanceSettings::SkinName);

    if (value.isValid()) {
        font.setBold(true);
        painter->setFont(font);
        QFontMetrics fontMetrics(font);

        QRect textRect = fontMetrics.boundingRect(value.toString());
        int textWidth = textRect.width();

        if (option.direction == Qt::RightToLeft) {
            if (width < textWidth)
                x = option.rect.x() + MARGIN;
            else
                x = option.rect.right() - ICON - (3 * MARGIN) - textWidth;
        }

        y += textRect.height();

        painter->drawText(x, y, fontMetrics.elidedText(value.toString(), option.textElideMode, width));

        value = index.data(AppearanceSettings::SkinInstalledWithKns);

        if (value.isValid() && value.toBool()) {
            QIcon ghnsIcon(QStringLiteral("get-hot-new-stuff"));
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

    if (value.isValid()) {
        QString skinAuthor = xi18nc("@item:intext", "by %1", value.toString());

        font.setBold(false);
        painter->setFont(font);
        QFontMetrics fontMetrics(font);

        QRect textRect = fontMetrics.boundingRect(skinAuthor);

        if (option.direction == Qt::RightToLeft) {
            if (width < textRect.width())
                x = option.rect.x() + MARGIN;
            else
                x = option.rect.right() - ICON - (3 * MARGIN) - textRect.width();
        }

        y += textRect.height();

        painter->drawText(x, y, fontMetrics.elidedText(skinAuthor, option.textElideMode, width));
    }
}

QSize SkinListDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QFont font = option.font;
    QRect name, author;
    int width, height;

    QVariant value;

    value = index.data(AppearanceSettings::SkinName);

    if (value.isValid()) {
        font.setBold(true);
        QFontMetrics fontMetrics(font);
        name = fontMetrics.boundingRect(value.toString());
    }

    value = index.data(Qt::UserRole + 1);

    if (value.isValid()) {
        QString skinAuthor = xi18nc("@item:intext", "by %1", value.toString());

        font.setBold(false);
        QFontMetrics fontMetrics(font);
        author = fontMetrics.boundingRect(skinAuthor);
    }

    width = qMax(name.width(), author.width());
    QRect textRect(0, 0, width, name.height() + author.height());

    width = ICON + (3 * MARGIN) + textRect.width() + MARGIN;
    height = qMax(ICON + (2 * MARGIN), textRect.height() + (2 * MARGIN));

    return QSize(width, height);
}
