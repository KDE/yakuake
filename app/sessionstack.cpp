/*
  SPDX-FileCopyrightText: 2008-2014 Eike Hein <hein@kde.org>
  SPDX-FileCopyrightText: 2009 Juan Carlos Torres <carlosdgtorres@gmail.com>

  SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#include "sessionstack.h"
#include "settings.h"
#include "terminal.h"
#include "visualeventoverlay.h"

#include <KLocalizedString>
#include <KMessageBox>
#include <KNotification>

#include <QDBusConnection>

#include <algorithm>

static bool show_disallow_certain_dbus_methods_message = true;

SessionStack::SessionStack(QWidget *parent)
    : QStackedWidget(parent)
{
    QDBusConnection::sessionBus().registerObject(QStringLiteral("/yakuake/sessions"), this, QDBusConnection::ExportScriptableSlots);

    m_activeSessionId = -1;

    m_visualEventOverlay = new VisualEventOverlay(this);
    connect(this, SIGNAL(removeTerminalHighlight()), m_visualEventOverlay, SLOT(removeTerminalHighlight()));
}

SessionStack::~SessionStack()
{
}

int SessionStack::addSessionImpl(Session::SessionType type)
{
    Session *currentSession = m_sessions.value(activeSessionId());
    Terminal *currentTerminal = currentSession ? currentSession->getTerminal(currentSession->activeTerminalId()) : nullptr;
    QString workingDir = currentTerminal ? currentTerminal->currentWorkingDirectory() : QString();

    Session *session = new Session(workingDir, type, this);
    // clang-format off
    connect(session, SIGNAL(titleChanged(int,QString)), this, SIGNAL(titleChanged(int,QString)));
    connect(session, SIGNAL(terminalManuallyActivated(Terminal*)), this, SLOT(handleManualTerminalActivation(Terminal*)));
    connect(session, SIGNAL(keyboardInputBlocked(Terminal*)), m_visualEventOverlay, SLOT(indicateKeyboardInputBlocked(Terminal*)));
    connect(session, SIGNAL(activityDetected(Terminal*)), parentWidget(), SLOT(handleTerminalActivity(Terminal*)));
    connect(session, SIGNAL(silenceDetected(Terminal*)), parentWidget(), SLOT(handleTerminalSilence(Terminal*)));
    connect(parentWidget(), SIGNAL(windowClosed()), session, SLOT(reconnectMonitorActivitySignals()));
    // clang-format on
    connect(session, SIGNAL(destroyed(int)), this, SLOT(cleanup(int)));
    connect(session, &Session::wantsBlurChanged, this, &SessionStack::wantsBlurChanged);

    addWidget(session->widget());

    m_sessions.insert(session->id(), session);

    Q_EMIT wantsBlurChanged();

    if (Settings::dynamicTabTitles())
        Q_EMIT sessionAdded(session->id(), session->title());
    else
        Q_EMIT sessionAdded(session->id(), QString());

    return session->id();
}

int SessionStack::addSession()
{
    return addSessionImpl(Session::Single);
}

int SessionStack::addSessionTwoHorizontal()
{
    return addSessionImpl(Session::TwoHorizontal);
}

int SessionStack::addSessionTwoVertical()
{
    return addSessionImpl(Session::TwoVertical);
}

int SessionStack::addSessionQuad()
{
    return addSessionImpl(Session::Quad);
}

void SessionStack::raiseSession(int sessionId)
{
    if (sessionId == -1 || !m_sessions.contains(sessionId))
        return;
    Session *session = m_sessions.value(sessionId);

    if (!m_visualEventOverlay->isHidden())
        m_visualEventOverlay->hide();

    if (m_activeSessionId != -1 && m_sessions.contains(m_activeSessionId)) {
        Session *oldActiveSession = m_sessions.value(m_activeSessionId);

        disconnect(oldActiveSession, SLOT(closeTerminal()));
        disconnect(oldActiveSession, SLOT(focusPreviousTerminal()));
        disconnect(oldActiveSession, SLOT(focusNextTerminal()));
        disconnect(oldActiveSession, SLOT(manageProfiles()));
        disconnect(oldActiveSession, SIGNAL(titleChanged(QString)), this, SIGNAL(activeTitleChanged(QString)));

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

    Q_EMIT sessionRaised(sessionId);

    Q_EMIT activeTitleChanged(session->title());
}

void SessionStack::removeSession(int sessionId)
{
    if (sessionId == -1)
        sessionId = m_activeSessionId;
    if (sessionId == -1)
        return;
    if (!m_sessions.contains(sessionId))
        return;

    if (queryClose(sessionId, QueryCloseSession))
        m_sessions.value(sessionId)->deleteLater();
}

void SessionStack::removeTerminal(int terminalId)
{
    int sessionId = sessionIdForTerminalId(terminalId);

    if (terminalId == -1) {
        if (m_activeSessionId == -1)
            return;
        if (!m_sessions.contains(m_activeSessionId))
            return;

        if (m_sessions.value(m_activeSessionId)->closable())
            m_sessions.value(m_activeSessionId)->closeTerminal();
    } else {
        if (m_sessions.value(sessionId)->closable())
            m_sessions.value(sessionId)->closeTerminal(terminalId);
    }
}

void SessionStack::closeActiveTerminal(int sessionId)
{
    if (sessionId == -1)
        sessionId = m_activeSessionId;
    if (sessionId == -1)
        return;
    if (!m_sessions.contains(sessionId))
        return;

    if (queryClose(sessionId, QueryCloseTerminal))
        m_sessions.value(sessionId)->closeTerminal();
}

void SessionStack::cleanup(int sessionId)
{
    if (sessionId == m_activeSessionId)
        m_activeSessionId = -1;

    m_sessions.remove(sessionId);

    Q_EMIT wantsBlurChanged();
    Q_EMIT sessionRemoved(sessionId);
}

int SessionStack::activeTerminalId()
{
    if (!m_sessions.contains(m_activeSessionId))
        return -1;

    return m_sessions.value(m_activeSessionId)->activeTerminalId();
}

const QString SessionStack::sessionIdList()
{
    QList<int> keyList = m_sessions.keys();
    QStringList idList;

    QListIterator<int> i(keyList);

    while (i.hasNext())
        idList << QString::number(i.next());

    return idList.join(QLatin1Char(','));
}

const QString SessionStack::terminalIdList()
{
    QStringList idList;

    QHashIterator<int, Session *> it(m_sessions);

    while (it.hasNext()) {
        it.next();

        idList << it.value()->terminalIdList();
    }

    return idList.join(QLatin1Char(','));
}

const QString SessionStack::terminalIdsForSessionId(int sessionId)
{
    if (!m_sessions.contains(sessionId))
        return QString::number(-1);

    return m_sessions.value(sessionId)->terminalIdList();
}

int SessionStack::sessionIdForTerminalId(int terminalId)
{
    int sessionId = -1;

    QHashIterator<int, Session *> it(m_sessions);

    while (it.hasNext()) {
        it.next();

        if (it.value()->hasTerminal(terminalId)) {
            sessionId = it.key();

            break;
        }
    }

    return sessionId;
}

static void warnAboutDBus()
{
#if !defined(REMOVE_SENDTEXT_RUNCOMMAND_DBUS_METHODS)
    if (show_disallow_certain_dbus_methods_message) {
        KNotification::event(
            KNotification::Warning,
            QStringLiteral("Yakuake D-Bus Warning"),
            i18n("The D-Bus method runCommand was just used.  There are security concerns about allowing these methods to be public.  If desired, these "
                 "methods can be changed to internal use only by re-compiling Yakuake. <p>This warning will only show once for this Yakuake instance.</p>"));
        show_disallow_certain_dbus_methods_message = false;
    }
#endif
}

void SessionStack::runCommand(const QString &command)
{
    warnAboutDBus();

    if (m_activeSessionId == -1)
        return;
    if (!m_sessions.contains(m_activeSessionId))
        return;

    m_sessions.value(m_activeSessionId)->runCommand(command);
}

void SessionStack::runCommandInTerminal(int terminalId, const QString &command)
{
    warnAboutDBus();

    QHashIterator<int, Session *> it(m_sessions);

    while (it.hasNext()) {
        it.next();

        it.value()->runCommand(command, terminalId);
    }
}

bool SessionStack::isSessionClosable(int sessionId)
{
    if (sessionId == -1)
        sessionId = m_activeSessionId;
    if (sessionId == -1)
        return false;
    if (!m_sessions.contains(sessionId))
        return false;

    return m_sessions.value(sessionId)->closable();
}

void SessionStack::setSessionClosable(int sessionId, bool closable)
{
    if (sessionId == -1)
        sessionId = m_activeSessionId;
    if (sessionId == -1)
        return;
    if (!m_sessions.contains(sessionId))
        return;

    m_sessions.value(sessionId)->setClosable(closable);
}

bool SessionStack::hasUnclosableSessions() const
{
    QHashIterator<int, Session *> it(m_sessions);

    while (it.hasNext()) {
        it.next();

        if (!it.value()->closable())
            return true;
    }

    return false;
}

bool SessionStack::isSessionKeyboardInputEnabled(int sessionId)
{
    if (sessionId == -1)
        sessionId = m_activeSessionId;
    if (sessionId == -1)
        return false;
    if (!m_sessions.contains(sessionId))
        return false;

    return m_sessions.value(sessionId)->keyboardInputEnabled();
}

void SessionStack::setSessionKeyboardInputEnabled(int sessionId, bool enabled)
{
    if (sessionId == -1)
        sessionId = m_activeSessionId;
    if (sessionId == -1)
        return;
    if (!m_sessions.contains(sessionId))
        return;

    m_sessions.value(sessionId)->setKeyboardInputEnabled(enabled);

    if (sessionId == m_activeSessionId) {
        if (enabled)
            m_visualEventOverlay->hide();
        else
            m_visualEventOverlay->show();
    }
}

bool SessionStack::isTerminalKeyboardInputEnabled(int terminalId)
{
    int sessionId = sessionIdForTerminalId(terminalId);
    if (sessionId == -1)
        return false;
    if (!m_sessions.contains(sessionId))
        return false;

    return m_sessions.value(sessionId)->keyboardInputEnabled(terminalId);
}

void SessionStack::setTerminalKeyboardInputEnabled(int terminalId, bool enabled)
{
    int sessionId = sessionIdForTerminalId(terminalId);
    if (sessionId == -1)
        return;
    if (!m_sessions.contains(sessionId))
        return;

    m_sessions.value(sessionId)->setKeyboardInputEnabled(terminalId, enabled);

    if (sessionId == m_activeSessionId) {
        if (enabled)
            m_visualEventOverlay->hide();
        else
            m_visualEventOverlay->show();
    }
}

bool SessionStack::hasTerminalsWithKeyboardInputEnabled(int sessionId)
{
    if (sessionId == -1)
        sessionId = m_activeSessionId;
    if (sessionId == -1)
        return false;
    if (!m_sessions.contains(sessionId))
        return false;

    return m_sessions.value(sessionId)->hasTerminalsWithKeyboardInputEnabled();
}

bool SessionStack::hasTerminalsWithKeyboardInputDisabled(int sessionId)
{
    if (sessionId == -1)
        sessionId = m_activeSessionId;
    if (sessionId == -1)
        return false;
    if (!m_sessions.contains(sessionId))
        return false;

    return m_sessions.value(sessionId)->hasTerminalsWithKeyboardInputDisabled();
}

bool SessionStack::isSessionMonitorActivityEnabled(int sessionId)
{
    if (sessionId == -1)
        sessionId = m_activeSessionId;
    if (sessionId == -1)
        return false;
    if (!m_sessions.contains(sessionId))
        return false;

    return m_sessions.value(sessionId)->monitorActivityEnabled();
}

void SessionStack::setSessionMonitorActivityEnabled(int sessionId, bool enabled)
{
    if (sessionId == -1)
        sessionId = m_activeSessionId;
    if (sessionId == -1)
        return;
    if (!m_sessions.contains(sessionId))
        return;

    m_sessions.value(sessionId)->setMonitorActivityEnabled(enabled);
}

bool SessionStack::isTerminalMonitorActivityEnabled(int terminalId)
{
    int sessionId = sessionIdForTerminalId(terminalId);
    if (sessionId == -1)
        return false;
    if (!m_sessions.contains(sessionId))
        return false;

    return m_sessions.value(sessionId)->monitorActivityEnabled(terminalId);
}

void SessionStack::setTerminalMonitorActivityEnabled(int terminalId, bool enabled)
{
    int sessionId = sessionIdForTerminalId(terminalId);
    if (sessionId == -1)
        return;
    if (!m_sessions.contains(sessionId))
        return;

    m_sessions.value(sessionId)->setMonitorActivityEnabled(terminalId, enabled);
}

bool SessionStack::hasTerminalsWithMonitorActivityEnabled(int sessionId)
{
    if (sessionId == -1)
        sessionId = m_activeSessionId;
    if (sessionId == -1)
        return false;
    if (!m_sessions.contains(sessionId))
        return false;

    return m_sessions.value(sessionId)->hasTerminalsWithMonitorActivityEnabled();
}

bool SessionStack::hasTerminalsWithMonitorActivityDisabled(int sessionId)
{
    if (sessionId == -1)
        sessionId = m_activeSessionId;
    if (sessionId == -1)
        return false;
    if (!m_sessions.contains(sessionId))
        return false;

    return m_sessions.value(sessionId)->hasTerminalsWithMonitorActivityDisabled();
}

bool SessionStack::isSessionMonitorSilenceEnabled(int sessionId)
{
    if (sessionId == -1)
        sessionId = m_activeSessionId;
    if (sessionId == -1)
        return false;
    if (!m_sessions.contains(sessionId))
        return false;

    return m_sessions.value(sessionId)->monitorSilenceEnabled();
}

void SessionStack::setSessionMonitorSilenceEnabled(int sessionId, bool enabled)
{
    if (sessionId == -1)
        sessionId = m_activeSessionId;
    if (sessionId == -1)
        return;
    if (!m_sessions.contains(sessionId))
        return;

    m_sessions.value(sessionId)->setMonitorSilenceEnabled(enabled);
}

bool SessionStack::isTerminalMonitorSilenceEnabled(int terminalId)
{
    int sessionId = sessionIdForTerminalId(terminalId);
    if (sessionId == -1)
        return false;
    if (!m_sessions.contains(sessionId))
        return false;

    return m_sessions.value(sessionId)->monitorSilenceEnabled(terminalId);
}

void SessionStack::setTerminalMonitorSilenceEnabled(int terminalId, bool enabled)
{
    int sessionId = sessionIdForTerminalId(terminalId);
    if (sessionId == -1)
        return;
    if (!m_sessions.contains(sessionId))
        return;

    m_sessions.value(sessionId)->setMonitorSilenceEnabled(terminalId, enabled);
}

bool SessionStack::hasTerminalsWithMonitorSilenceEnabled(int sessionId)
{
    if (sessionId == -1)
        sessionId = m_activeSessionId;
    if (sessionId == -1)
        return false;
    if (!m_sessions.contains(sessionId))
        return false;

    return m_sessions.value(sessionId)->hasTerminalsWithMonitorSilenceEnabled();
}

bool SessionStack::hasTerminalsWithMonitorSilenceDisabled(int sessionId)
{
    if (sessionId == -1)
        sessionId = m_activeSessionId;
    if (sessionId == -1)
        return false;
    if (!m_sessions.contains(sessionId))
        return false;

    return m_sessions.value(sessionId)->hasTerminalsWithMonitorSilenceDisabled();
}

void SessionStack::editProfile(int sessionId)
{
    if (sessionId == -1)
        sessionId = m_activeSessionId;
    if (sessionId == -1)
        return;
    if (!m_sessions.contains(sessionId))
        return;

    m_sessions.value(sessionId)->editProfile();
}

int SessionStack::splitSessionLeftRight(int sessionId)
{
    if (sessionId == -1)
        return -1;
    if (!m_sessions.contains(sessionId))
        return -1;

    return m_sessions.value(sessionId)->splitLeftRight();
}

int SessionStack::splitSessionTopBottom(int sessionId)
{
    if (sessionId == -1)
        return -1;
    if (!m_sessions.contains(sessionId))
        return -1;

    return m_sessions.value(sessionId)->splitTopBottom();
}

int SessionStack::splitTerminalLeftRight(int terminalId)
{
    int sessionId = sessionIdForTerminalId(terminalId);

    if (sessionId == -1)
        return -1;

    return m_sessions.value(sessionId)->splitLeftRight(terminalId);
}

int SessionStack::splitTerminalTopBottom(int terminalId)
{
    int sessionId = sessionIdForTerminalId(terminalId);

    if (sessionId == -1)
        return -1;

    return m_sessions.value(sessionId)->splitTopBottom(terminalId);
}

int SessionStack::tryGrowTerminalRight(int terminalId, uint pixels)
{
    int sessionId = sessionIdForTerminalId(terminalId);

    if (sessionId == -1)
        return -1;

    return m_sessions.value(sessionId)->tryGrowTerminal(terminalId, Session::Right, pixels);
}

int SessionStack::tryGrowTerminalLeft(int terminalId, uint pixels)
{
    int sessionId = sessionIdForTerminalId(terminalId);

    if (sessionId == -1)
        return -1;

    return m_sessions.value(sessionId)->tryGrowTerminal(terminalId, Session::Left, pixels);
}

int SessionStack::tryGrowTerminalTop(int terminalId, uint pixels)
{
    int sessionId = sessionIdForTerminalId(terminalId);

    if (sessionId == -1)
        return -1;

    return m_sessions.value(sessionId)->tryGrowTerminal(terminalId, Session::Up, pixels);
}

int SessionStack::tryGrowTerminalBottom(int terminalId, uint pixels)
{
    int sessionId = sessionIdForTerminalId(terminalId);

    if (sessionId == -1)
        return -1;

    return m_sessions.value(sessionId)->tryGrowTerminal(terminalId, Session::Down, pixels);
}

void SessionStack::emitTitles()
{
    QString title;

    QHashIterator<int, Session *> it(m_sessions);

    while (it.hasNext()) {
        it.next();

        title = it.value()->title();

        if (!title.isEmpty())
            Q_EMIT titleChanged(it.value()->id(), title);
    }
}

bool SessionStack::requiresVisualEventOverlay()
{
    if (m_activeSessionId == -1)
        return false;
    if (!m_sessions.contains(m_activeSessionId))
        return false;

    return m_sessions.value(m_activeSessionId)->hasTerminalsWithKeyboardInputDisabled();
}

void SessionStack::handleTerminalHighlightRequest(int terminalId)
{
    Terminal *terminal = nullptr;

    QHashIterator<int, Session *> it(m_sessions);

    while (it.hasNext()) {
        it.next();

        terminal = it.value()->getTerminal(terminalId);

        if (terminal && it.value()->id() == m_activeSessionId) {
            m_visualEventOverlay->highlightTerminal(terminal, true);

            break;
        }
    }
}

void SessionStack::showEvent(QShowEvent *event)
{
    Q_UNUSED(event)

    if (m_activeSessionId == -1)
        return;
    if (!m_sessions.contains(m_activeSessionId))
        return;

    Terminal *terminal = m_sessions.value(m_activeSessionId)->getTerminal(activeTerminalId());

    if (terminal) {
        QWidget *terminalWidget = terminal->terminalWidget();
        if (terminalWidget)
            terminalWidget->setFocus();
    }
}

void SessionStack::handleManualTerminalActivation(Terminal *terminal)
{
    if (!Settings::terminalHighlightOnManualActivation())
        return;

    Session *session = qobject_cast<Session *>(QObject::sender());

    if (session->terminalCount() > 1)
        m_visualEventOverlay->highlightTerminal(terminal, false);
}

bool SessionStack::queryClose(int sessionId, QueryCloseType type)
{
    if (!m_sessions.contains(sessionId))
        return false;

    if (!m_sessions.value(sessionId)->closable()) {
        QString closeQuestionIntro = xi18nc("@info", "<warning>You have locked this session to prevent accidental closing of terminals.</warning>");
        QString closeQuestion;

        if (type == QueryCloseSession)
            closeQuestion = xi18nc("@info", "Are you sure you want to close this session?");
        else if (type == QueryCloseTerminal)
            closeQuestion = xi18nc("@info", "Are you sure you want to close this terminal?");

        int result = KMessageBox::warningContinueCancel(this,
                                                        closeQuestionIntro + QStringLiteral("<br/><br/>") + closeQuestion,
                                                        xi18nc("@title:window", "Really Close?"),
                                                        KStandardGuiItem::close(),
                                                        KStandardGuiItem::cancel());

        if (result != KMessageBox::Continue)
            return false;
    }

    return true;
}

QList<KActionCollection *> SessionStack::getPartActionCollections()
{
    QList<KActionCollection *> actionCollections;

    const auto sessions = m_sessions.values();
    for (auto *session : sessions) {
        const auto terminalIds = session->terminalIdList().split(QStringLiteral(","));

        for (const auto &terminalID : terminalIds) {
            auto *terminal = session->getTerminal(terminalID.toInt());
            if (terminal) {
                auto *collection = terminal->actionCollection();
                if (collection) {
                    actionCollections.append(collection);
                }
            }
        }
    }

    return actionCollections;
}

bool SessionStack::wantsBlur() const
{
    return std::any_of(m_sessions.cbegin(), m_sessions.cend(), [](Session *session) {
        return session->wantsBlur();
    });
}

#include "moc_sessionstack.cpp"
