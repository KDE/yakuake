/*
    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.
*/

/*
  Copyright (C) 2007 Eike Hein <hein@kde.org>
*/


#include "terminal_focus_watcher.h"
#include "terminal_focus_watcher.moc"


TerminalFocusWatcher::TerminalFocusWatcher(QObject* parent, const char* name)
    : QObject(parent, name)
{
}

TerminalFocusWatcher::~TerminalFocusWatcher()
{
}

bool TerminalFocusWatcher::eventFilter (QObject* /* watched */, QEvent* e)
{
    if (e->type() == QEvent::FocusIn)
        emit focusChanged();

    return false;
}
