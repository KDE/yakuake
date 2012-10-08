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


#include "sessionstack.h"
#include "settings.h"
#include "visualeventoverlay.h"

#include <KMessageBox>
#include <KLocalizedString>

#include <QtDBus/QtDBus>


SessionStack::SessionStack(QWidget* parent) : QStackedWidget(parent)
{
    QDBusConnection::sessionBus().registerObject("/yakuake/sessions", this, QDBusConnection::ExportScriptableSlots);

    m_activeSessionId = -1;

    m_visualEventOverlay = new VisualEventOverlay(this);
    connect(this, SIGNAL(removeTerminalHighlight()), m_visualEventOverlay, SLOT(removeTerminalHighlight()));
}

SessionStack::~SessionStack()
{
}

int SessionStack::addSession(Session::SessionType type)
{
    Session* session = new Session(type, this);
    connect(session, SIGNAL(titleChanged(int,QString)), this, SIGNAL(titleChanged(int,QString)));
    connect(session, SIGNAL(terminalManuallyActivated(Terminal*)), this, SLOT(handleManualTerminalActivation(Terminal*)));
    connect(session, SIGNAL(keyboardInputBlocked(Terminal*)), m_visualEventOverlay, SLOT(indicateKeyboardInputBlocked(Terminal*)));
    connect(session, SIGNAL(activityDetected(Terminal*)), parentWidget(), SLOT(handleTerminalActivity(Terminal*)));
    connect(session, SIGNAL(silenceDetected(Terminal*)), parentWidget(), SLOT(handleTerminalSilence(Terminal*)));
    connect(parentWidget(), SIGNAL(windowClosed()), session, SLOT(reconnectMonitorActivitySignals()));
    connect(session, SIGNAL(destroyed(int)), this, SLOT(cleanup(int)));

    addWidget(session->widget());

    m_sessions.insert(session->id(), session);

    if (Settings::dynamicTabTitles())
        emit sessionAdded(session->id(), session->title());
    else
        emit sessionAdded(session->id());

    return session->id();
}

int SessionStack::addSessionTwoHorizontal()
{
    return addSession(Session::TwoHorizontal);
}

int SessionStack::addSessionTwoVertical()
{
    return addSession(Session::TwoVertical);
}

int SessionStack::addSessionQuad()
{
    return addSession(Session::Quad);
}

void SessionStack::raiseSession(int sessionId)
{
    if (sessionId == -1 || !m_sessions.contains(sessionId)) return;
    Session* session = m_sessions.value(sessionId);

    if (!m_visualEventOverlay->isHidden())
        m_visualEventOverlay->hide();

    if (m_activeSessionId != -1 && m_sessions.contains(m_activeSessionId))
    {
        Session* oldActiveSession = m_sessions.value(m_activeSessionId);

        disconnect(oldActiveSession, SLOT(closeTerminal()));
        disconnect(oldActiveSession, SLOT(focusPreviousTerminal()));
        disconnect(oldActiveSession, SLOT(focusNextTerminal()));
        disconnect(oldActiveSession, SLOT(manageProfiles()));
        disconnect(oldActiveSession, SIGNAL(titleChanged(QString)),
            this, SIGNAL(activeTitleChanged(QString)));

        oldActiveSession->reconnectMonitorActivitySignals();
    }

    m_activeSessionId = sessionId;

    setCurrentWidget(session->widget());

    if (session->widget()->focusWidget())
        session->widget()->focusWidget()->setFocus();

    if (session->hasTerminalsWithKeyboardInputDisabled())
        m_visualEventOverlay->show();

    connect(this, SIGNAL(closeTerminal()), session, SLOT(closeTerminal()));
    connect(this, SIGNAL(previousTerminal()), session, SLOT(focusPreviousTerminal()));
    connect(this, SIGNAL(nextTerminal()), session, SLOT(focusNextTerminal()));
    connect(this, SIGNAL(manageProfiles()), session, SLOT(manageProfiles()));
    connect(session, SIGNAL(titleChanged(QString)), this, SIGNAL(activeTitleChanged(QString)));

    emit sessionRaised(sessionId);

    emit activeTitleChanged(session->title());
}

void SessionStack::removeSession(int sessionId)
{
    if (sessionId == -1) sessionId = m_activeSessionId;
    if (sessionId == -1) return;
    if (!m_sessions.contains(sessionId)) return;

    if (queryClose(sessionId, QueryCloseSession))
        m_sessions.value(sessionId)->deleteLater();
}

void SessionStack::removeTerminal(int terminalId)
{
    int sessionId = sessionIdForTerminalId(terminalId);

    if (terminalId == -1)
    {
        if (m_activeSessionId == -1) return;
        if (!m_sessions.contains(m_activeSessionId)) return;

        if (m_sessions.value(m_activeSessionId)->closable())
            m_sessions.value(m_activeSessionId)->closeTerminal();
    }
    else
    {
        if (m_sessions.value(sessionId)->closable())
            m_sessions.value(sessionId)->closeTerminal(terminalId);
    }
}

void SessionStack::closeActiveTerminal(int sessionId)
{
    if (sessionId == -1) sessionId = m_activeSessionId;
    if (sessionId == -1) return;
    if (!m_sessions.contains(sessionId)) return;

    if (queryClose(sessionId, QueryCloseTerminal))
        m_sessions.value(sessionId)->closeTerminal();
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

    return m_sessions.value(m_activeSessionId)->activeTerminalId();
}

const QString SessionStack::sessionIdList()
{
    QList<int> keyList = m_sessions.uniqueKeys();
    QStringList idList;

    QListIterator<int> i(keyList);

    while (i.hasNext())
        idList << QString::number(i.next());

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

const QString SessionStack::terminalIdsForSessionId(int sessionId)
{
    if (!m_sessions.contains(sessionId)) return QString::number(-1);

    return m_sessions.value(sessionId)->terminalIdList();
}

int SessionStack::sessionIdForTerminalId(int terminalId)
{
    int sessionId = -1;

    QHashIterator<int, Session*> it(m_sessions);

    while (it.hasNext())
    {
        it.next();

        if (it.value()->hasTerminal(terminalId))
        {
            sessionId = it.key();

            break;
        }
    }

    return sessionId;
}

void SessionStack::runCommand(const QString& command)
{
    if (m_activeSessionId == -1) return;
    if (!m_sessions.contains(m_activeSessionId)) return;

    m_sessions.value(m_activeSessionId)->runCommand(command);
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

bool SessionStack::isSessionClosable(int sessionId)
{
    if (sessionId == -1) sessionId = m_activeSessionId;
    if (sessionId == -1) return false;
    if (!m_sessions.contains(sessionId)) return false;

    return m_sessions.value(sessionId)->closable();
}

void SessionStack::setSessionClosable(int sessionId, bool closable)
{
    if (sessionId == -1) sessionId = m_activeSessionId;
    if (sessionId == -1) return;
    if (!m_sessions.contains(sessionId)) return;

    m_sessions.value(sessionId)->setClosable(closable);
}

bool SessionStack::hasUnclosableSessions() const
{
    QHashIterator<int, Session*> it(m_sessions);

    while (it.hasNext())
    {
        it.next();

        if (!it.value()->closable())
            return true;
    }

    return false;
}

bool SessionStack::isSessionKeyboardInputEnabled(int sessionId)
{
    if (sessionId == -1) sessionId = m_activeSessionId;
    if (sessionId == -1) return false;
    if (!m_sessions.contains(sessionId)) return false;

    return m_sessions.value(sessionId)->keyboardInputEnabled();
}

void SessionStack::setSessionKeyboardInputEnabled(int sessionId, bool enabled)
{
    if (sessionId == -1) sessionId = m_activeSessionId;
    if (sessionId == -1) return;
    if (!m_sessions.contains(sessionId)) return;

    m_sessions.value(sessionId)->setKeyboardInputEnabled(enabled);

    if (sessionId == m_activeSessionId)
    {
        if (enabled)
            m_visualEventOverlay->hide();
        else
            m_visualEventOverlay->show();
    }
}

bool SessionStack::isTerminalKeyboardInputEnabled(int terminalId)
{
    int sessionId = sessionIdForTerminalId(terminalId);
    if (sessionId == -1) return false;
    if (!m_sessions.contains(sessionId)) return false;

    return m_sessions.value(sessionId)->keyboardInputEnabled(terminalId);
}

void SessionStack::setTerminalKeyboardInputEnabled(int terminalId, bool enabled)
{
    int sessionId = sessionIdForTerminalId(terminalId);
    if (sessionId == -1) return;
    if (!m_sessions.contains(sessionId)) return;

    m_sessions.value(sessionId)->setKeyboardInputEnabled(terminalId, enabled);

    if (sessionId == m_activeSessionId)
    {
        if (enabled)
            m_visualEventOverlay->hide();
        else
            m_visualEventOverlay->show();
    }
}

bool SessionStack::hasTerminalsWithKeyboardInputEnabled(int sessionId)
{
    if (sessionId == -1) sessionId = m_activeSessionId;
    if (sessionId == -1) return false;
    if (!m_sessions.contains(sessionId)) return false;

    return m_sessions.value(sessionId)->hasTerminalsWithKeyboardInputEnabled();
}

bool SessionStack::hasTerminalsWithKeyboardInputDisabled(int sessionId)
{
    if (sessionId == -1) sessionId = m_activeSessionId;
    if (sessionId == -1) return false;
    if (!m_sessions.contains(sessionId)) return false;

    return m_sessions.value(sessionId)->hasTerminalsWithKeyboardInputDisabled();
}

bool SessionStack::isSessionMonitorActivityEnabled(int sessionId)
{
    if (sessionId == -1) sessionId = m_activeSessionId;
    if (sessionId == -1) return false;
    if (!m_sessions.contains(sessionId)) return false;

    return m_sessions.value(sessionId)->monitorActivityEnabled();
}

void SessionStack::setSessionMonitorActivityEnabled(int sessionId, bool enabled)
{
    if (sessionId == -1) sessionId = m_activeSessionId;
    if (sessionId == -1) return;
    if (!m_sessions.contains(sessionId)) return;

    m_sessions.value(sessionId)->setMonitorActivityEnabled(enabled);
}

bool SessionStack::isTerminalMonitorActivityEnabled(int terminalId)
{
    int sessionId = sessionIdForTerminalId(terminalId);
    if (sessionId == -1) return false;
    if (!m_sessions.contains(sessionId)) return false;

    return m_sessions.value(sessionId)->monitorActivityEnabled(terminalId);
}

void SessionStack::setTerminalMonitorActivityEnabled(int terminalId, bool enabled)
{
    int sessionId = sessionIdForTerminalId(terminalId);
    if (sessionId == -1) return;
    if (!m_sessions.contains(sessionId)) return;

    m_sessions.value(sessionId)->setMonitorActivityEnabled(terminalId, enabled);
}

bool SessionStack::hasTerminalsWithMonitorActivityEnabled(int sessionId)
{
    if (sessionId == -1) sessionId = m_activeSessionId;
    if (sessionId == -1) return false;
    if (!m_sessions.contains(sessionId)) return false;

    return m_sessions.value(sessionId)->hasTerminalsWithMonitorActivityEnabled();
}

bool SessionStack::hasTerminalsWithMonitorActivityDisabled(int sessionId)
{
    if (sessionId == -1) sessionId = m_activeSessionId;
    if (sessionId == -1) return false;
    if (!m_sessions.contains(sessionId)) return false;

    return m_sessions.value(sessionId)->hasTerminalsWithMonitorActivityDisabled();
}

bool SessionStack::isSessionMonitorSilenceEnabled(int sessionId)
{
    if (sessionId == -1) sessionId = m_activeSessionId;
    if (sessionId == -1) return false;
    if (!m_sessions.contains(sessionId)) return false;

    return m_sessions.value(sessionId)->monitorSilenceEnabled();
}

void SessionStack::setSessionMonitorSilenceEnabled(int sessionId, bool enabled)
{
    if (sessionId == -1) sessionId = m_activeSessionId;
    if (sessionId == -1) return;
    if (!m_sessions.contains(sessionId)) return;

    m_sessions.value(sessionId)->setMonitorSilenceEnabled(enabled);
}

bool SessionStack::isTerminalMonitorSilenceEnabled(int terminalId)
{
    int sessionId = sessionIdForTerminalId(terminalId);
    if (sessionId == -1) return false;
    if (!m_sessions.contains(sessionId)) return false;

    return m_sessions.value(sessionId)->monitorSilenceEnabled(terminalId);
}

void SessionStack::setTerminalMonitorSilenceEnabled(int terminalId, bool enabled)
{
    int sessionId = sessionIdForTerminalId(terminalId);
    if (sessionId == -1) return;
    if (!m_sessions.contains(sessionId)) return;

    m_sessions.value(sessionId)->setMonitorSilenceEnabled(terminalId, enabled);
}

bool SessionStack::hasTerminalsWithMonitorSilenceEnabled(int sessionId)
{
    if (sessionId == -1) sessionId = m_activeSessionId;
    if (sessionId == -1) return false;
    if (!m_sessions.contains(sessionId)) return false;

    return m_sessions.value(sessionId)->hasTerminalsWithMonitorSilenceEnabled();
}

bool SessionStack::hasTerminalsWithMonitorSilenceDisabled(int sessionId)
{
    if (sessionId == -1) sessionId = m_activeSessionId;
    if (sessionId == -1) return false;
    if (!m_sessions.contains(sessionId)) return false;

    return m_sessions.value(sessionId)->hasTerminalsWithMonitorSilenceDisabled();
}

void SessionStack::editProfile(int sessionId)
{
    if (sessionId == -1) sessionId = m_activeSessionId;
    if (sessionId == -1) return;
    if (!m_sessions.contains(sessionId)) return;

    m_sessions.value(sessionId)->editProfile();
}

int SessionStack::splitSessionLeftRight(int sessionId)
{
    if (sessionId == -1) return -1;
    if (!m_sessions.contains(sessionId)) return -1;

    return m_sessions.value(sessionId)->splitLeftRight();
}

int SessionStack::splitSessionTopBottom(int sessionId)
{
    if (sessionId == -1) return -1;
    if (!m_sessions.contains(sessionId)) return -1;

    return m_sessions.value(sessionId)->splitTopBottom();
}

int SessionStack::splitTerminalLeftRight(int terminalId)
{
    int sessionId = sessionIdForTerminalId(terminalId);

    if (sessionId == -1) return -1;

    return m_sessions.value(sessionId)->splitLeftRight(terminalId);
}

int SessionStack::splitTerminalTopBottom(int terminalId)
{
    int sessionId = sessionIdForTerminalId(terminalId);

    if (sessionId == -1) return -1;

    return m_sessions.value(sessionId)->splitTopBottom(terminalId);
}

int SessionStack::tryGrowTerminalRight(int terminalId, uint pixels)
{
    int sessionId = sessionIdForTerminalId(terminalId);

    if (sessionId == -1) return -1;

    return m_sessions.value(sessionId)->tryGrowTerminal(terminalId, Session::Right, pixels);
}

int SessionStack::tryGrowTerminalLeft(int terminalId, uint pixels)
{
    int sessionId = sessionIdForTerminalId(terminalId);

    if (sessionId == -1) return -1;

    return m_sessions.value(sessionId)->tryGrowTerminal(terminalId, Session::Left, pixels);
}

int SessionStack::tryGrowTerminalTop(int terminalId, uint pixels)
{
    int sessionId = sessionIdForTerminalId(terminalId);

    if (sessionId == -1) return -1;

    return m_sessions.value(sessionId)->tryGrowTerminal(terminalId, Session::Up, pixels);
}

int SessionStack::tryGrowTerminalBottom(int terminalId, uint pixels)
{
    int sessionId = sessionIdForTerminalId(terminalId);

    if (sessionId == -1) return -1;

    return m_sessions.value(sessionId)->tryGrowTerminal(terminalId, Session::Down, pixels);
}

void SessionStack::emitTitles()
{
    QString title;

    QHashIterator<int, Session*> it(m_sessions);

    while (it.hasNext())
    {
        it.next();

        title = it.value()->title();

        if (!title.isEmpty())
            emit titleChanged(it.value()->id(), title);
    }
}

bool SessionStack::requiresVisualEventOverlay()
{
    if (m_activeSessionId == -1) return false;
    if (!m_sessions.contains(m_activeSessionId)) return false;

    return m_sessions.value(m_activeSessionId)->hasTerminalsWithKeyboardInputDisabled();
}

void SessionStack::handleTerminalHighlightRequest(int terminalId)
{
    Terminal* terminal = 0;

    QHashIterator<int, Session*> it(m_sessions);

    while (it.hasNext())
    {
        it.next();

        terminal = it.value()->getTerminal(terminalId);

        if (terminal && it.value()->id() == m_activeSessionId)
        {
            m_visualEventOverlay->highlightTerminal(terminal, true);

            break;
        }
    }
}

void SessionStack::handleManualTerminalActivation(Terminal* terminal)
{
    if (!Settings::terminalHighlightOnManualActivation())
        return;

    Session* session = qobject_cast<Session*>(QObject::sender());

    if (session->terminalCount() > 1)
        m_visualEventOverlay->highlightTerminal(terminal, false);
}

bool SessionStack::queryClose(int sessionId, QueryCloseType type)
{
    if (!m_sessions.contains(sessionId)) return false;

    if (!m_sessions.value(sessionId)->closable())
    {
        QString closeQuestionIntro = i18nc("@info", "<warning>You have locked this session to prevent accidental closing of terminals.</warning>");
        QString closeQuestion;

        if (type == QueryCloseSession)
            closeQuestion = i18nc("@info", "Are you sure you want to close this session?");
        else if (type == QueryCloseTerminal)
            closeQuestion = i18nc("@info", "Are you sure you want to close this terminal?");

        int result = KMessageBox::warningContinueCancel(this,
            closeQuestionIntro + "<br/><br/>" + closeQuestion,
            i18nc("@title:window", "Really Close?"), KStandardGuiItem::close(), KStandardGuiItem::cancel());

        if (result != KMessageBox::Continue)
            return false;
    }

    return true;
}
