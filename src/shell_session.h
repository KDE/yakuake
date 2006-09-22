/*****************************************************************************
 *                                                                           *
 *   Copyright (C) 2005 by Chazal Francois             <neptune3k@free.fr>   *
 *   website : http://workspace.free.fr                                      *
 *                                                                           *
 *                     =========  GPL License  =========                     *
 *    This program is free software; you can redistribute it and/or modify   *
 *   it under the terms of the  GNU General Public License as published by   *
 *   the  Free  Software  Foundation ; either version 2 of the License, or   *
 *   (at your option) any later version.                                     *
 *                                                                           *
 *****************************************************************************/

#ifndef SHELL_SESSION_H
# define SHELL_SESSION_H

//== INCLUDE REQUIREMENTS ===================================================//

/*
** C libraries */
#include <stdlib.h>

/*
** Qt libraries */
#include <qobject.h>
#include <qwidget.h>

/*
** KDE libraries */
#include <klibloader.h>
#include <kparts/part.h>
#include <kde_terminal_interface.h>


//== DEFINE CLASS & DATATYPES ===============================================//

/*
** Class 'ShellSession' creates a shell session from konsole kpart
********************************************************************/

class ShellSession : public QObject
{
    Q_OBJECT

public:

    //-- PRIVATE ATTRIBUTES ---------------------------------------------//

    KParts::Part *      session_part;
    QString             session_title;
    QWidget *           session_widget;
    TerminalInterface * session_terminal;



public:

    //-- CONSTRUCTORS AND DESTRUCTORS -----------------------------------//

    ShellSession(QWidget * parent = 0, const char * name = 0);
    virtual ~ShellSession();


    //-- PUBLIC ATTRIBUTES ----------------------------------------------//



    //-- PUBLIC METHODS -------------------------------------------------//



public slots:

    //-- PUBLIC SLOTS ---------------------------------------------------//

    void    slotDestroySession();
    void    slotUpdateSessionTitle(const QString &);



signals:

    //-- SIGNALS DEFINITION ---------------------------------------------//

    void    titleUpdated();
};

#endif /* SHELL_SESSION_H */
