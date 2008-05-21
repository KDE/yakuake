/*
  Copyright (C) 2008 by Eike Hein <hein@kde.org>

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

#include <QHash>
#include <QStackedWidget>


class Session;


class SessionStack : public QStackedWidget
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.kde.yakuake")

    public:
        explicit SessionStack(QWidget* parent = 0);
        ~SessionStack();

        void closeActiveTerminal(int sessionId = -1);

        void editProfile(int sessionId = -1);

        void splitLeftRight(int sessionId = -1);
        void splitTopBottom(int sessionId = -1);

        void emitTitles();


    public slots:
        Q_SCRIPTABLE void addSession(Session::SessionType type = Session::Single);
        Q_SCRIPTABLE void addSessionTwoHorizontal();
        Q_SCRIPTABLE void addSessionTwoVertical();
        Q_SCRIPTABLE void addSessionQuad();

        Q_SCRIPTABLE void raiseSession(int sessionId);

        Q_SCRIPTABLE void removeSession(int sessionId);
        Q_SCRIPTABLE void removeTerminal(int terminalId);

        Q_SCRIPTABLE int activeSessionId() { return m_activeSessionId; }
        Q_SCRIPTABLE int activeTerminalId();
        Q_SCRIPTABLE const QString sessionIdList();
        Q_SCRIPTABLE const QString terminalIdList();

        Q_SCRIPTABLE void runCommand(const QString& command);
        Q_SCRIPTABLE void runCommandInTerminal(int terminalId, const QString& command);


    signals:
        void sessionAdded(int sessionId, const QString& title = 0);
        void sessionRaised(int sessionId);
        void sessionRemoved(int sessionId);

        void activeTitleChanged(const QString& title);
        void titleChanged(int sessionId, const QString& title);

        void closeTerminal();

        void previousTerminal();
        void nextTerminal();

        void manageProfiles();


    private slots:
        void cleanup(int sessionId);


    private:
        int m_activeSessionId;

        QHash<int, Session*> m_sessions;
};

#endif
