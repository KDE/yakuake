/*
    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.
*/

/*
  Copyright (C) 2007 Eike Hein <hein@kde.org>
*/


#ifndef TERMINAL_FOCUS_WATCHER_H
#define TERMINAL_FOCUS_WATCHER_H


#include <qobject.h>


class TerminalFocusWatcher : public QObject
{
    Q_OBJECT

    public:
        explicit TerminalFocusWatcher(QObject* parent = 0, const char* name = 0);
        virtual ~TerminalFocusWatcher();

        virtual bool eventFilter (QObject* watched, QEvent* e);


    signals:
        void focusChanged();
};

#endif /* TERMINAL_FOCUS_WATCHER_H */
