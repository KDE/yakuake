/*
    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.
*/

/*
  Copyright (C) 2005 Francois Chazal <neptune3k@free.fr>
  Copyright (C) 2006-2007 Eike Hein <hein@kde.org>
*/


#ifndef TERMINAL_H
#define TERMINAL_H


#include <stdlib.h>

#include <qobject.h>
#include <qwidget.h>

#include <klibloader.h>
#include <kparts/part.h>
#include <kde_terminal_interface.h>


class Terminal : public QObject
{
    Q_OBJECT

    public:
        explicit Terminal(QWidget* parent = 0, const char* name = 0);
        virtual ~Terminal();

        int id() { return terminal_id; }

        QWidget* widget();
        TerminalInterface* terminal();

        const QString title();
        void setTitle(const QString& title);



    signals:
        void destroyed(int);
        void titleChanged(QWidget*, const QString&);


    private:
        static int available_terminal_id;
        int terminal_id;

        KParts::Part* terminal_part;
        QString  terminal_title;
        QWidget* terminal_widget;
        TerminalInterface* terminal_interface;


    private slots:
        void slotUpdateSessionTitle(const QString &);
};

#endif /* TERMINAL_H */
