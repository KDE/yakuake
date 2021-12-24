/*
  SPDX-FileCopyrightText: 2008 Eike Hein <hein@kde.org>

  SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#ifndef SPLITTER_H
#define SPLITTER_H

#include <QSplitter>

class Splitter : public QSplitter
{
    Q_OBJECT

public:
    explicit Splitter(Qt::Orientation orientation, QWidget *parent = nullptr);
    ~Splitter();

    void recursiveCleanup();
};

#endif
