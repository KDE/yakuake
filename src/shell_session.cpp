/*****************************************************************************
 *                                                                           *
 *   Copyright (C) 2005 by Chazal Francois             <neptune3k@free.fr>   *
 *   website : http://workspace.free.fr                                      *
 *                                                                           *
 *                     =========  GPL Licence  =========                     *
 *    This program is free software; you can redistribute it and/or modify   *
 *   it under the terms of the  GNU General Public License as published by   *
 *   the  Free  Software  Foundation ; either version 2 of the License, or   *
 *   (at your option) any later version.                                     *
 *                                                                           *
 *****************************************************************************/

//== INCLUDE REQUIREMENTS =====================================================

/*
** Local libraries */
#include "shell_session.h"
#include "shell_session.moc"



//== CONSTRUCTORS AND DESTRUCTORS =============================================

ShellSession::ShellSession(QWidget * parent, const char * name) : QObject(parent, name)
{
    KLibFactory *   factory = NULL;

    session_part = NULL;
    session_title = "";
    session_widget = NULL;
    session_terminal = NULL;


    if ((factory = KLibLoader::self()->factory("libkonsolepart")) != NULL)
        session_part = (KParts::Part *) (factory->create(parent));

    if (session_part != NULL)
    {
        QStrList            args;
        const char *        shell;

        session_widget = session_part->widget();
        session_widget->setFocusPolicy(QWidget::WheelFocus);

        // Initializes the terminal -----------------------

        session_terminal = (TerminalInterface *) (session_part->qt_cast("TerminalInterface"));

/* We don't actually want a login shell_session

        args.append("-l");
        shell = getenv("SHELL");
        if (shell == NULL || *shell == '\0')
            shell = "/bin/sh";

        session_terminal->startProgram(shell, args);
*/

        // Connects signals to slots ----------------------

        connect(session_part, SIGNAL(destroyed()), this, SLOT(slotDestroySession()));
        connect(session_part, SIGNAL(setWindowCaption(const QString &)), this, SLOT(slotUpdateSessionTitle(const QString &)));
    }
}

ShellSession::~ShellSession()
{}



//== PUBLIC SLOTS =============================================================

void    ShellSession::slotDestroySession()
{
    delete this;
}

void    ShellSession::slotUpdateSessionTitle(const QString & title)
{
    session_title = title;
    emit(titleUpdated());
}

