/*
  Copyright (C) 2008-2014 by Eike Hein <hein@kde.org>
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


#ifndef SESSIONSTACK_H
#define SESSIONSTACK_H


#include "session.h"

#include "config-yakuake.h"

#include <QHash>
#include <QStackedWidget>


class Session;
class VisualEventOverlay;

class SessionStack : public QStackedWidget
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.kde.yakuake")

    public:
        explicit SessionStack(QWidget* parent = 0);
        ~SessionStack();

        void closeActiveTerminal(int sessionId = -1);

        void editProfile(int sessionId = -1);

        void emitTitles();

        bool requiresVisualEventOverlay();


    public Q_SLOTS:
        Q_SCRIPTABLE int addSession(Session::SessionType type = Session::Single);
        Q_SCRIPTABLE int addSessionTwoHorizontal();
        Q_SCRIPTABLE int addSessionTwoVertical();
        Q_SCRIPTABLE int addSessionQuad();

        Q_SCRIPTABLE void raiseSession(int sessionId);

        Q_SCRIPTABLE void removeSession(int sessionId);
        Q_SCRIPTABLE void removeTerminal(int terminalId);

        Q_SCRIPTABLE int splitSessionLeftRight(int sessionId);
        Q_SCRIPTABLE int splitSessionTopBottom(int sessionId);
        Q_SCRIPTABLE int splitTerminalLeftRight(int terminalId);
        Q_SCRIPTABLE int splitTerminalTopBottom(int terminalId);

        Q_SCRIPTABLE int tryGrowTerminalRight(int terminalId, uint pixels = 10);
        Q_SCRIPTABLE int tryGrowTerminalLeft(int terminalId, uint pixels = 10);
        Q_SCRIPTABLE int tryGrowTerminalTop(int terminalId, uint pixels = 10);
        Q_SCRIPTABLE int tryGrowTerminalBottom(int terminalId, uint pixels = 10);

        Q_SCRIPTABLE int activeSessionId() { return m_activeSessionId; }
        Q_SCRIPTABLE int activeTerminalId();

        Q_SCRIPTABLE const QString sessionIdList();
        Q_SCRIPTABLE const QString terminalIdList();
        Q_SCRIPTABLE const QString terminalIdsForSessionId(int sessionId);
        Q_SCRIPTABLE int sessionIdForTerminalId(int terminalId);

#if defined(REMOVE_SENDTEXT_RUNCOMMAND_DBUS_METHODS)
        void runCommand(const QString& command);
        void runCommandInTerminal(int terminalId, const QString& command);
#else
        Q_SCRIPTABLE void runCommand(const QString& command);
        Q_SCRIPTABLE void runCommandInTerminal(int terminalId, const QString& command);
#endif

        Q_SCRIPTABLE bool isSessionClosable(int sessionId);
        Q_SCRIPTABLE void setSessionClosable(int sessionId, bool closable);
        Q_SCRIPTABLE bool hasUnclosableSessions() const;

        Q_SCRIPTABLE bool isSessionKeyboardInputEnabled(int sessionId);
        Q_SCRIPTABLE void setSessionKeyboardInputEnabled(int sessionId, bool enabled);
        Q_SCRIPTABLE bool isTerminalKeyboardInputEnabled(int terminalId);
        Q_SCRIPTABLE void setTerminalKeyboardInputEnabled(int terminalId, bool enabled);
        Q_SCRIPTABLE bool hasTerminalsWithKeyboardInputEnabled(int sessionId);
        Q_SCRIPTABLE bool hasTerminalsWithKeyboardInputDisabled(int sessionId);

        Q_SCRIPTABLE bool isSessionMonitorActivityEnabled(int sessionId);
        Q_SCRIPTABLE void setSessionMonitorActivityEnabled(int sessionId, bool enabled);
        Q_SCRIPTABLE bool isTerminalMonitorActivityEnabled(int terminalId);
        Q_SCRIPTABLE void setTerminalMonitorActivityEnabled(int terminalId, bool enabled);
        Q_SCRIPTABLE bool hasTerminalsWithMonitorActivityEnabled(int sessionId);
        Q_SCRIPTABLE bool hasTerminalsWithMonitorActivityDisabled(int sessionId);

        Q_SCRIPTABLE bool isSessionMonitorSilenceEnabled(int sessionId);
        Q_SCRIPTABLE void setSessionMonitorSilenceEnabled(int sessionId, bool enabled);
        Q_SCRIPTABLE bool isTerminalMonitorSilenceEnabled(int terminalId);
        Q_SCRIPTABLE void setTerminalMonitorSilenceEnabled(int terminalId, bool enabled);
        Q_SCRIPTABLE bool hasTerminalsWithMonitorSilenceEnabled(int sessionId);
        Q_SCRIPTABLE bool hasTerminalsWithMonitorSilenceDisabled(int sessionId);

        void handleTerminalHighlightRequest(int terminalId);


    Q_SIGNALS:
        void sessionAdded(int sessionId, const QString& title);
        void sessionRaised(int sessionId);
        void sessionRemoved(int sessionId);

        void activeTitleChanged(const QString& title);
        void titleChanged(int sessionId, const QString& title);

        void closeTerminal();

        void previousTerminal();
        void nextTerminal();

        void manageProfiles();

        void removeTerminalHighlight();


    protected:
        virtual void showEvent(QShowEvent *event) override;


    private Q_SLOTS:
        void handleManualTerminalActivation(Terminal*);

        void cleanup(int sessionId);


    private:
        enum QueryCloseType { QueryCloseSession, QueryCloseTerminal };
        bool queryClose(int sessionId, QueryCloseType type);

        const QString currentWorkingDirectory();

        VisualEventOverlay* m_visualEventOverlay;

        int m_activeSessionId;

        QHash<int, Session*> m_sessions;
};

#endif
