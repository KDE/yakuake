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


#include "terminal.h"
#include "terminal.moc"


int Terminal::available_terminal_id = 0;

Terminal::Terminal(QWidget* parent, const char* name) : QObject(parent, name)
{
    terminal_id = available_terminal_id;
    available_terminal_id++;

    setenv("DCOP_YAKUAKE_TERMINAL", QString::number(terminal_id).ascii(), 1);
    putenv((char*)"COLORTERM="); // Trigger mc's color detection.

    KLibFactory* factory = NULL;

    terminal_part = NULL;
    terminal_title = "";
    terminal_widget = NULL;
    terminal_interface = NULL;

    if ((factory = KLibLoader::self()->factory("libkonsolepart")) != NULL)
        terminal_part = (KParts::Part *) (factory->create(parent));

    if (terminal_part != NULL)
    {
        terminal_widget = terminal_part->widget();
        terminal_widget->setFocusPolicy(QWidget::WheelFocus);
        terminal_interface = (TerminalInterface *) (terminal_part->qt_cast("TerminalInterface"));

        connect(terminal_part, SIGNAL(destroyed()), this, SLOT(deleteLater()));
        connect(terminal_part, SIGNAL(setWindowCaption(const QString &)), this, SLOT(slotUpdateSessionTitle(const QString &)));
    }
}

Terminal::~Terminal()
{
    emit destroyed(terminal_id);
}

QWidget* Terminal::widget()
{
    return terminal_widget;
}

TerminalInterface* Terminal::terminal()
{
    return terminal_interface;
}

const QString Terminal::title()
{
    return terminal_title;
}

void Terminal::setTitle(const QString& title)
{
    slotUpdateSessionTitle(title);
}

void Terminal::slotUpdateSessionTitle(const QString& title)
{
    terminal_title = title;

    // Remove trailing " -" from the caption. If needed this is
    // handled later in TitleBar.
    if (terminal_title.endsWith(" - "))
        terminal_title.truncate(terminal_title.length() - 3);

    emit titleChanged(terminal_widget, terminal_title);
}
