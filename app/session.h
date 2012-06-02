/*
  Copyright (C) 2008-2009 by Eike Hein <hein@kde.org>
  Copyright (C) 2009 by Juan Carlos Torres <carlosdgtorres@gmail.com>

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License as
  published by the Free Software Foundation; either version 2 of
  the License or (at your option) version 3 or any later version
  accepted by the membership of KDE e.V. (or its successor appro-
  ved by the membership of KDE e.V.), which shall act as a proxy
  defined in Section 14 of version 3 of the license.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program. If not, see http://www.gnu.org/licenses/.
*/


#ifndef SESSION_H
#define SESSION_H


#include "splitter.h"

#include <QMap>
#include <QObject>


class Terminal;


class Session : public QObject
{
    Q_OBJECT

    public:
        enum SessionType { Single, TwoHorizontal, TwoVertical, Quad };
        enum GrowthDirection { Up, Right, Down, Left };

        explicit Session(SessionType type = Single, QWidget* parent = 0);
         ~Session();

        int id() { return m_sessionId; }
        const QString title() { return m_title; }
        QWidget* widget() { return m_baseSplitter; }

        int activeTerminalId() { return m_activeTerminalId; }
        const QString terminalIdList();
        int terminalCount() { return m_terminals.count(); }
        bool hasTerminal(int terminalId);
        Terminal* getTerminal(int terminalId);

        bool closable() { return m_closable; }
        void setClosable(bool closable) { m_closable = closable; }

        bool keyboardInputEnabled();
        void setKeyboardInputEnabled(bool enabled);
        bool keyboardInputEnabled(int terminalId);
        void setKeyboardInputEnabled(int terminalId, bool enabled);
        bool hasTerminalsWithKeyboardInputEnabled();
        bool hasTerminalsWithKeyboardInputDisabled();

        bool monitorActivityEnabled();
        void setMonitorActivityEnabled(bool enabled);
        bool monitorActivityEnabled(int terminalId);
        void setMonitorActivityEnabled(int terminalId, bool enabled);
        bool hasTerminalsWithMonitorActivityEnabled();
        bool hasTerminalsWithMonitorActivityDisabled();

        bool monitorSilenceEnabled();
        void setMonitorSilenceEnabled(bool enabled);
        bool monitorSilenceEnabled(int terminalId);
        void setMonitorSilenceEnabled(int terminalId, bool enabled);
        bool hasTerminalsWithMonitorSilenceEnabled();
        bool hasTerminalsWithMonitorSilenceDisabled();


    public slots:
        void closeTerminal(int terminalId = -1);

        void focusNextTerminal();
        void focusPreviousTerminal();

        int splitLeftRight(int terminalId = -1);
        int splitTopBottom(int terminalId = -1);

        int tryGrowTerminal(int terminalId, GrowthDirection direction, uint pixels);

        void runCommand(const QString& command, int terminalId = -1);

        void manageProfiles();
        void editProfile();

        void reconnectMonitorActivitySignals();


    signals:
        void titleChanged(const QString& title);
        void titleChanged(int sessionId, const QString& title);
        void terminalManuallyActivated(Terminal* terminal);
        void keyboardInputBlocked(Terminal* terminal);
        void activityDetected(Terminal* terminal);
        void silenceDetected(Terminal* terminal);
        void destroyed(int sessionId);


    private slots:
        void setActiveTerminal(int terminalId);
        void setTitle(int terminalId, const QString& title);

        void cleanup(int terminalId);
        void cleanup();
        void prepareShutdown();


    private:
        void setupSession(SessionType type);

        Terminal* addTerminal(QWidget* parent);
        int split(Terminal* terminal, Qt::Orientation orientation);

        static int m_availableSessionId;
        int m_sessionId;

        Splitter* m_baseSplitter;

        int m_activeTerminalId;
        QMap<int, Terminal*> m_terminals;

        QString m_title;

        bool m_closable;
};

#endif
