/*
  SPDX-FileCopyrightText: 2008-2009 Eike Hein <hein@kde.org>

  SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#include "splitter.h"

Splitter::Splitter(Qt::Orientation orientation, QWidget *parent)
    : QSplitter(orientation, parent)
{
    setAutoFillBackground(true);
    setOpaqueResize(false);
}

Splitter::~Splitter()
{
}

void Splitter::recursiveCleanup()
{
    if (count() == 0)
        deleteLater();
    else {
        QList<Splitter *> list = findChildren<Splitter *>();

        QListIterator<Splitter *> i(list);

        while (i.hasNext()) {
            Splitter *splitter = i.next();

            if (splitter->parent() == this)
                splitter->recursiveCleanup();
        }
    }
}

#include "moc_splitter.cpp"
