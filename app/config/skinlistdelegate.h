/*
  SPDX-FileCopyrightText: 2008 Eike Hein <hein@kde.org>

  SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#ifndef SKINLISTDELEGATE_H
#define SKINLISTDELEGATE_H

#include <QAbstractItemDelegate>

class SkinListDelegate : public QAbstractItemDelegate
{
public:
    explicit SkinListDelegate(QObject *parent = nullptr);
    ~SkinListDelegate() override;

    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const override;

private:
    void paintBackground(QPainter *painter, const QStyleOptionViewItem &option) const;
    void paintIcon(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const;
    void paintText(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const;
};

#endif
