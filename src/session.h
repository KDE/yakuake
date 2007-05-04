/*
    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.
*/

/*
  Copyright (C) 2007 Eike Hein <hein@kde.org>
*/


#ifndef SESSION_H
#define SESSION_H

#include "terminal_splitter.h"

#include <qobject.h>
#include <qwidget.h>


class Terminal;
class TerminalInterface;
class TerminalFocusWatcher;

class Session : public QObject
{
    Q_OBJECT

    public:
        enum SessionType { Single, TwoHorizontal, TwoVertical, Quad };

        explicit Session(QWidget* parent = 0, SessionType type = Single, const char* name = 0);
        virtual ~Session();

        int id() { return session_id; }

        int activeTerminalId();

        QWidget* widget() { return base_widget; }

        const QString title();
        const QString title(int terminal_id);
        void setTitle(const QString& title);
        void setTitle(int terminal_id, const QString& title);

        void pasteClipboard();
        void pasteClipboard(int terminal_id);
        void pasteSelection();
        void pasteSelection(int terminal_id);

        void runCommand(const QString& command);
        void runCommand(int terminal_id, const QString& command);

        void removeTerminal();
        void removeTerminal(int terminal_id);


    public slots:
        void splitHorizontally();
        void splitHorizontally(int terminal_id);

        void splitVertically();
        void splitVertically(int terminal_id);

        void focusNextSplit();
        void focusPreviousSplit();

        void slotTitleChange(QWidget* w, const QString& title);


    signals:
        void destroyed(int id);
        void titleChanged(const QString&);


    private:
        void createInitialSplits(SessionType);
        void split(QWidget* active_terminal, Orientation o);
        Terminal* addTerminal(QWidget* parent);
        bool checkFocusWidget();

        static int available_session_id;
        int session_id;
        QString session_title;

        QWidget* active_terminal;

        TerminalSplitter* base_widget;
        TerminalFocusWatcher* focus_watcher;

        QMap<int, Terminal*> terminals;
        QMap<QWidget*, int> terminal_ids;
        QMap<int, QWidget*> terminal_widgets;
        QMap<int, TerminalInterface*> terminal_parts;


    private slots: 
        void slotFocusChanged();
        void slotLastTerminalClosed();
        void cleanup(int terminal_id);
        void cleanup();
};

#endif /* SESSION_H */
