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


#include "sessionstack.h"

#include <QtDBus/QtDBus>


SessionStack::SessionStack(QWidget* parent) : QStackedWidget(parent)
{
    QDBusConnection::sessionBus().registerObject("/yakuake/sessions", this, QDBusConnection::ExportScriptableSlots);

    m_activeSessionId = -1;
}

SessionStack::~SessionStack()
{
}

void SessionStack::addSession(Session::SessionType type)
{
    Session* session = new Session(type, this);
    connect(session, SIGNAL(destroyed(int)), this, SLOT(cleanup(int)));
    connect(session, SIGNAL(titleChanged(int, const QString&)), 
        this, SIGNAL(titleChanged(int, const QString&)));

    addWidget(session->widget());

    m_sessions.insert(session->id(), session);

    emit sessionAdded(session->id());
}

void SessionStack::addSessionTwoHorizontal()
{
addSession(Session::TwoHorizontal);
}

void SessionStack::addSessionTwoVertical()
{
    addSession(Session::TwoVertical);
}

void SessionStack::addSessionQuad()
{
    addSession(Session::Quad);
}

void SessionStack::raiseSession(int sessionId)
{
    if (sessionId == -1 || !m_sessions.contains(sessionId)) return;

    if (m_activeSessionId != -1 && m_sessions.contains(m_activeSessionId))
    {
        disconnect(m_sessions[m_activeSessionId], SLOT(closeTerminal()));
        disconnect(m_sessions[m_activeSessionId], SLOT(focusPreviousTerminal()));
        disconnect(m_sessions[m_activeSessionId], SLOT(focusNextTerminal()));
        disconnect(m_sessions[m_activeSessionId], SLOT(splitLeftRight()));
        disconnect(m_sessions[m_activeSessionId], SLOT(splitTopBottom()));
        disconnect(m_sessions[m_activeSessionId], SLOT(manageProfiles()));
        disconnect(m_sessions[m_activeSessionId], SIGNAL(titleChanged(const QString&)), 
            this, SIGNAL(activeTitleChanged(const QString&)));
    }

    m_activeSessionId = sessionId;

    setCurrentWidget(m_sessions[sessionId]->widget());

    if (m_sessions[sessionId]->widget()->focusWidget())
        m_sessions[sessionId]->widget()->focusWidget()->setFocus();

    connect(this, SIGNAL(closeTerminal()), m_sessions[sessionId], SLOT(closeTerminal()));
    connect(this, SIGNAL(previousTerminal()), m_sessions[sessionId], SLOT(focusPreviousTerminal()));
    connect(this, SIGNAL(nextTerminal()), m_sessions[sessionId], SLOT(focusNextTerminal()));
    connect(this, SIGNAL(splitLeftRight()), m_sessions[sessionId], SLOT(splitLeftRight()));
    connect(this, SIGNAL(splitTopBottom()), m_sessions[sessionId], SLOT(splitTopBottom()));
    connect(this, SIGNAL(manageProfiles()), m_sessions[sessionId], SLOT(manageProfiles()));
    connect(m_sessions[sessionId], SIGNAL(titleChanged(const QString&)), 
        this, SIGNAL(activeTitleChanged(const QString&)));

    emit sessionRaised(sessionId);
    emit activeTitleChanged(m_sessions[sessionId]->title());
}

void SessionStack::removeSession(int sessionId)
{
    if (sessionId == -1) sessionId = m_activeSessionId;
    if (sessionId == -1) return;
    if (!m_sessions.contains(sessionId)) return;

    m_sessions[sessionId]->deleteLater();
}

void SessionStack::removeTerminal(int terminalId)
{
    if (terminalId == -1)
    {
        if (m_activeSessionId == -1) return;
        if (!m_sessions.contains(m_activeSessionId)) return;
    
        m_sessions[m_activeSessionId]->closeTerminal();
    }
    else
    {
        QHashIterator<int, Session*> it(m_sessions);

        while (it.hasNext()) 
        {
            it.next();

            it.value()->closeTerminal(terminalId);
        }
    }
}

void SessionStack::cleanup(int sessionId)
{
    if (sessionId == m_activeSessionId) m_activeSessionId = -1;

    m_sessions.remove(sessionId);

    emit sessionRemoved(sessionId);
}

int SessionStack::activeTerminalId()
{
    if (!m_sessions.contains(m_activeSessionId)) return -1;

    return m_sessions[m_activeSessionId]->activeTerminalId();
}

const QString SessionStack::sessionIdList()
{
    QList<int> keyList = m_sessions.uniqueKeys();
    QStringList idList;

    for (int i = 0; i < keyList.size(); ++i) 
        idList << QString::number(keyList.at(i));
    
    return idList.join(",");
}

const QString SessionStack::terminalIdList()
{
    QStringList idList;

    QHashIterator<int, Session*> it(m_sessions);

    while (it.hasNext()) 
    {
        it.next();

        idList << it.value()->terminalIdList();
    }

    return idList.join(",");
}

void SessionStack::runCommand(const QString& command)
{
    if (m_activeSessionId == -1) return;
    if (!m_sessions.contains(m_activeSessionId)) return;

    m_sessions[m_activeSessionId]->runCommand(command);
}

void SessionStack::runCommandInTerminal(int terminalId, const QString& command)
{
    QHashIterator<int, Session*> it(m_sessions);

    while (it.hasNext()) 
    {
        it.next();
        it.value()->runCommand(command, terminalId);
    }
}

void SessionStack::editProfile(int sessionId)
{
    if (sessionId == -1) sessionId = m_activeSessionId;
    if (sessionId == -1) return;
    if (!m_sessions.contains(sessionId)) return;

    m_sessions[sessionId]->editProfile();
}
