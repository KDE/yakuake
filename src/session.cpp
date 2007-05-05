/*
    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.
*/

/*
  Copyright (C) 2007 Eike Hein <hein@kde.org>
*/


#include "session.h"
#include "session.moc"
#include "terminal.h"
#include "terminal_splitter.h"
#include "terminal_focus_watcher.h"

#include <qobjectlist.h>
#include <qclipboard.h>
#include <qtimer.h>


int Session::available_session_id = 0;

Session::Session(QWidget* parent, SessionType type, const char* name) : QObject(parent, name)
{
    session_id = available_session_id;
    available_session_id++;

    active_terminal = NULL;

    focus_watcher = new TerminalFocusWatcher(this);
    connect(focus_watcher, SIGNAL(focusChanged()), this, SLOT(slotFocusChanged()));

    base_widget = new TerminalSplitter(TerminalSplitter::Horizontal, parent, "base");
    connect(base_widget, SIGNAL(destroyed()), this, SLOT(slotLastTerminalClosed()));

    setenv("DCOP_YAKUAKE_SESSION", QString::number(session_id).ascii(), 1);

    createInitialSplits(type);
}

Session::~Session()
{
    if (base_widget)
    {
        base_widget->setPrepareShutdown(true);
        delete base_widget;
    }

    emit destroyed(session_id);
}

void Session::slotFocusChanged()
{
    if (checkFocusWidget())
    {
        base_widget->setFocusProxy(base_widget->focusWidget());
        active_terminal = base_widget->focusWidget();
        int terminal_id = terminal_ids[base_widget->focusWidget()];
        slotTitleChange(base_widget->focusWidget(),
            terminals[terminal_id]->title());
    }
}

int Session::activeTerminalId()
{
    if (checkFocusWidget())
        return terminal_ids[base_widget->focusWidget()];
    else
        return -1;
}

const QString Session::title()
{
    return session_title;
}

const QString Session::title(int terminal_id)
{
    if (terminals[terminal_id])
        return terminals[terminal_id]->title();
    else
        return 0;
}

void Session::setTitle(const QString& title)
{
    if (active_terminal)
    {
        int terminal_id = terminal_ids[active_terminal];
        setTitle(terminal_id, title);
    }
}

void Session::setTitle(int terminal_id, const QString& title)
{
    if (terminals[terminal_id]) terminals[terminal_id]->setTitle(title);
}

void Session::pasteClipboard()
{
    if (active_terminal)
    {
        int terminal_id = terminal_ids[active_terminal];
        pasteClipboard(terminal_id);
    }
}

void Session::pasteClipboard(int terminal_id)
{
    if (terminal_parts[terminal_id])
        terminal_parts[terminal_id]->sendInput(QApplication::clipboard()->text(QClipboard::Clipboard));
}

void Session::pasteSelection()
{
    if (active_terminal)
    {
        int terminal_id = terminal_ids[active_terminal];
        pasteSelection(terminal_id);
    }
}

void Session::pasteSelection(int terminal_id)
{
    if (terminal_parts[terminal_id])
        terminal_parts[terminal_id]->sendInput(QApplication::clipboard()->text(QClipboard::Selection));
}

void Session::runCommand(const QString& command)
{
    if (active_terminal)
    {
        int terminal_id = terminal_ids[active_terminal];
        runCommand(terminal_id, command);
    }
}

void Session::runCommand(int terminal_id, const QString& command)
{
    if (terminal_parts[terminal_id])
        terminal_parts[terminal_id]->sendInput(command + '\n');
}


void Session::splitHorizontally()
{
    if (active_terminal)
    {
        int terminal_id = terminal_ids[active_terminal];
        splitHorizontally(terminal_id);
    }
}

void Session::splitHorizontally(int terminal_id)
{
    if (terminal_widgets[terminal_id])
        split(terminal_widgets[terminal_id], TerminalSplitter::Horizontal);
}

void Session::splitVertically()
{
    if (active_terminal)
    {
        int terminal_id = terminal_ids[active_terminal];
        splitVertically(terminal_id);
    }
}

void Session::splitVertically(int terminal_id)
{
    if (terminal_widgets[terminal_id])
        split(terminal_widgets[terminal_id], TerminalSplitter::Vertical);
}

void Session::removeTerminal()
{
    if (active_terminal)
    {
        delete active_terminal;
        active_terminal = NULL;
    }
}

void Session::removeTerminal(int terminal_id)
{
    if (terminal_widgets[terminal_id])
        delete terminal_widgets[terminal_id];
}

void Session::focusNextSplit()
{
    base_widget->focusNext();
}

void Session::focusPreviousSplit()
{
    base_widget->focusPrevious();
}

void Session::createInitialSplits(SessionType type)
{
    switch (type)
    {
        case Single:
        {
            addTerminal(base_widget);

            break;
        }

        case TwoHorizontal:
        {
            int splitter_width = base_widget->width();

            Terminal* terminal = addTerminal(base_widget);
            addTerminal(base_widget);

            QValueList<int> new_splitter_sizes;
            new_splitter_sizes << (splitter_width / 2) << (splitter_width / 2);
            base_widget->setSizes(new_splitter_sizes);

            terminal->widget()->setFocus();

            break;
        }

        case TwoVertical:
        {
            base_widget->setOrientation(TerminalSplitter::Vertical);

            int splitter_height = base_widget->height();

            Terminal* terminal = addTerminal(base_widget);
            addTerminal(base_widget);

            QValueList<int> new_splitter_sizes;
            new_splitter_sizes << (splitter_height / 2) << (splitter_height / 2);
            base_widget->setSizes(new_splitter_sizes);

            terminal->widget()->setFocus();

            break;
        }

        case Quad:
        {
            int splitter_width = base_widget->width();
            int splitter_height = base_widget->height();


            base_widget->setOrientation(TerminalSplitter::Vertical);

            TerminalSplitter* upper_splitter = new TerminalSplitter(TerminalSplitter::Horizontal, base_widget);
            connect(upper_splitter, SIGNAL(destroyed()), this, SLOT(cleanup()));

            TerminalSplitter* lower_splitter = new TerminalSplitter(TerminalSplitter::Horizontal, base_widget);
            connect(lower_splitter, SIGNAL(destroyed()), this, SLOT(cleanup()));

            Terminal* terminal = addTerminal(upper_splitter);
            addTerminal(upper_splitter);

            addTerminal(lower_splitter);
            addTerminal(lower_splitter);

            QValueList<int> new_splitter_sizes;
            new_splitter_sizes << (splitter_height / 2) << (splitter_height / 2);
            base_widget->setSizes(new_splitter_sizes);

            new_splitter_sizes.clear();
            new_splitter_sizes << (splitter_width / 2) << (splitter_width / 2);
            upper_splitter->setSizes(new_splitter_sizes);
            lower_splitter->setSizes(new_splitter_sizes);

            terminal->widget()->setFocus();

            break;
        }

        default:
        {
            addTerminal(base_widget);

            break;
        }
    }
}

void Session::split(QWidget* active_terminal, Orientation o)
{
    TerminalSplitter* splitter = static_cast<TerminalSplitter*>(active_terminal->parentWidget());
    if (!splitter) return;

    // If the parent splitter of this terminal has only this one child,
    // add the new terminal to the same splitter, after resetting the
    // splitter orientation as needed.
    if (splitter->count() == 1)
    {
        int splitter_width = splitter->width();

        if (splitter->orientation() != o)
            splitter->setOrientation(o);

        Terminal* terminal = addTerminal(splitter);

        QValueList<int> new_splitter_sizes;
        new_splitter_sizes << (splitter_width / 2) << (splitter_width / 2);
        splitter->setSizes(new_splitter_sizes);

        terminal->widget()->show();
    }
    // If the parent splitter of this terminal already has two children,
    // add a new splitter to it and reparent the terminal to the new
    // splitter.
    else
    {
        // Store the old splitter sizes to re-apply them later after the
        // add-and-remove action is done screwing with the splitter.
        QValueList<int> splitter_sizes = splitter->sizes();

        TerminalSplitter* new_splitter = new TerminalSplitter(o, splitter);
        connect(new_splitter, SIGNAL(destroyed()), this, SLOT(cleanup()));

        if (splitter->isFirst(active_terminal)) splitter->moveToFirst(new_splitter);

        active_terminal->reparent(new_splitter, 0, QPoint(), true);

        Terminal* terminal = addTerminal(new_splitter);

        splitter->setSizes(splitter_sizes);
        QValueList<int> new_splitter_sizes;
        new_splitter_sizes << (splitter_sizes[1] / 2) << (splitter_sizes[1] / 2);
        new_splitter->setSizes(new_splitter_sizes);

        new_splitter->show();
        terminal->widget()->show();
    }
}

Terminal* Session::addTerminal(QWidget* parent)
{
    Terminal* terminal = new Terminal(parent);

    terminals.insert(terminal->id(), terminal);
    terminal_ids.insert(terminal->widget(), terminal->id());
    terminal_widgets.insert(terminal->id(), terminal->widget());
    terminal_parts.insert(terminal->id(), terminal->terminal());

    terminal->widget()->installEventFilter(focus_watcher);

    connect(terminal, SIGNAL(destroyed(int)), this, SLOT(cleanup(int)));
    connect(terminal, SIGNAL(titleChanged(QWidget*, const QString&)),
        this, SLOT(slotTitleChange(QWidget*, const QString&)));

    active_terminal = terminal->widget();
    base_widget->setFocusProxy(terminal->widget());

    return terminal;
}

void Session::cleanup(int terminal_id)
{
    /* Clean up the id <-> terminals */

    terminals.erase(terminal_id);
    terminal_ids.erase(terminal_widgets[terminal_id]);
    terminal_widgets.erase(terminal_id);
    terminal_parts.erase(terminal_id);

    cleanup();
}

void Session::cleanup()
{
    /* Clean away empty splitters after a terminal was removed */

    if (!base_widget) return;

    base_widget->focusLast();

    base_widget->recursiveCleanup();
}

void Session::slotLastTerminalClosed()
{
    base_widget = NULL;
    deleteLater();
}

void Session::slotTitleChange(QWidget* w, const QString& title)
{
    if (w == base_widget->focusWidget())
    {
        session_title = title;
        emit titleChanged( session_title);
    }
}

bool Session::checkFocusWidget()
{
    if (base_widget->focusWidget()
        && base_widget->focusWidget()->isA(QCString("TEWidget")))
    {
        return true;
    }
    else
        return false;
}
