/*
    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.
*/

/*
  Copyright (C) 2007 Eike Hein <hein@kde.org>
*/


#include "terminal_splitter.h"
#include "terminal_splitter.moc"

#include <qobjectlist.h>



TerminalSplitter::TerminalSplitter(Orientation o, QWidget* parent, const char* name)
 : QSplitter(o, parent, name)
{
    is_shutting_down = false;
}

TerminalSplitter::~TerminalSplitter()
{
}

void TerminalSplitter::setPrepareShutdown(bool shutdown)
{
    is_shutting_down = shutdown;
}

void TerminalSplitter::focusNext()
{
    focusNextPrevChild(true);
}

void TerminalSplitter::focusPrevious()
{
    focusNextPrevChild(false);
}

void TerminalSplitter::focusLast()
{
    if (is_shutting_down) return;

    if (terminalCount(true) == 1)
    {
        QWidget* w = static_cast<QWidget*>(child(0, QCString("TEWidget"), true));
        if (w) w->setFocus();
    }
    else
    {
        focusPrevious();
        focusNext();
    }
}

int TerminalSplitter::count()
{
    return (terminalCount() + splitterCount());
}

int TerminalSplitter::terminalCount(bool recursive)
{
    return queryList(QCString("TEWidget"), 0, false, recursive)->count();
}

int TerminalSplitter::splitterCount(bool recursive)
{
    return queryList(QCString("TerminalSplitter"), 0, false, recursive)->count();
}

bool TerminalSplitter::isFirst(QWidget* w)
{
    /* Return whether the widget w is the first in the splitter. */

    if (idAfter(w) != 0)
        return true;
    else
        return false;
}

void TerminalSplitter::recursiveCleanup()
{
    /* Clean away empty splitters after a terminal was removed. */

    QObjectList* list = queryList("TerminalSplitter", 0, false, false);
    QObjectListIt it(*list);
    QObject *obj;

    while ((obj = it.current()) != 0)
    {
        ++it;
        TerminalSplitter* splitter = static_cast<TerminalSplitter*>(obj);
        splitter->recursiveCleanup();
    }

    if (count() == 0) deleteLater();
}
