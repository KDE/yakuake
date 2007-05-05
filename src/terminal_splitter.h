/*
    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.
*/

/*
  Copyright (C) 2007 Eike Hein <hein@kde.org>
*/


#ifndef TERMINAL_SPLITTER_H
#define TERMINAL_SPLITTER_H


#include <qsplitter.h>


class TerminalSplitter : public QSplitter
{
    Q_OBJECT

    public:
        explicit TerminalSplitter(Orientation o, QWidget* parent=0, const char* name=0);
        ~TerminalSplitter();

        void focusNext();
        void focusPrevious();
        void focusLast();

        int count();
        int terminalCount(bool recursive = false);
        int splitterCount(bool recursive = false);

        bool isFirst(QWidget * w);

        void recursiveCleanup();

        void setPrepareShutdown(bool shutdown);


    private:
        bool is_shutting_down;

};


#endif /* TERMINAL_SPLITTER_H */
