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


#include "session.h"
#include "terminal.h"


int Session::m_availableSessionId = 0;

Session::Session(SessionType type, QWidget* parent) : QObject(parent)
{
    m_sessionId = m_availableSessionId;
    m_availableSessionId++;

    m_activeTerminalId = -1;

    m_closable = true;

    m_baseSplitter = new Splitter(Qt::Horizontal, parent);
    connect(m_baseSplitter, SIGNAL(destroyed()), this, SLOT(prepareShutdown()));

    setupSession(type);
}

Session::~Session()
{
    if (m_baseSplitter) delete m_baseSplitter;

    emit destroyed(m_sessionId);
}

void Session::setupSession(SessionType type)
{
    switch (type)
    {
        case Single:
        {
            Terminal* terminal = addTerminal(m_baseSplitter);
            setActiveTerminal(terminal->id());

            break;
        }

        case TwoHorizontal:
        {
            int splitterWidth = m_baseSplitter->width();

            Terminal* terminal = addTerminal(m_baseSplitter);
            addTerminal(m_baseSplitter);

            QList<int> newSplitterSizes;
            newSplitterSizes << (splitterWidth / 2) << (splitterWidth / 2);
            m_baseSplitter->setSizes(newSplitterSizes);

            QWidget* terminalWidget = terminal->terminalWidget();

            if (terminalWidget)
            {
                terminalWidget->setFocus();
                setActiveTerminal(terminal->id());
            }

            break;
        }

        case TwoVertical:
        {
            m_baseSplitter->setOrientation(Qt::Vertical);

            int splitterHeight = m_baseSplitter->height();

            Terminal* terminal = addTerminal(m_baseSplitter);
            addTerminal(m_baseSplitter);

            QList<int> newSplitterSizes;
            newSplitterSizes << (splitterHeight / 2) << (splitterHeight / 2);
            m_baseSplitter->setSizes(newSplitterSizes);

            QWidget* terminalWidget = terminal->terminalWidget();

            if (terminalWidget)
            {
                terminalWidget->setFocus();
                setActiveTerminal(terminal->id());
            }

            break;
        }

        case Quad:
        {
            int splitterWidth = m_baseSplitter->width();
            int splitterHeight = m_baseSplitter->height();

            m_baseSplitter->setOrientation(Qt::Vertical);

            Splitter* upperSplitter = new Splitter(Qt::Horizontal, m_baseSplitter);
            connect(upperSplitter, SIGNAL(destroyed()), this, SLOT(cleanup()));

            Splitter* lowerSplitter = new Splitter(Qt::Horizontal, m_baseSplitter);
            connect(lowerSplitter, SIGNAL(destroyed()), this, SLOT(cleanup()));

            Terminal* terminal = addTerminal(upperSplitter);
            addTerminal(upperSplitter);

            addTerminal(lowerSplitter);
            addTerminal(lowerSplitter);

            QList<int> newSplitterSizes;
            newSplitterSizes << (splitterHeight / 2) << (splitterHeight / 2);
            m_baseSplitter->setSizes(newSplitterSizes);

            newSplitterSizes.clear();
            newSplitterSizes << (splitterWidth / 2) << (splitterWidth / 2);
            upperSplitter->setSizes(newSplitterSizes);
            lowerSplitter->setSizes(newSplitterSizes);

            QWidget* terminalWidget = terminal->terminalWidget();

            if (terminalWidget)
            {
                terminalWidget->setFocus();
                setActiveTerminal(terminal->id());
            }

            break;
        }

        default:
        {
            addTerminal(m_baseSplitter);

            break;
        }
    }
}

Terminal* Session::addTerminal(QWidget* parent)
{
    Terminal* terminal = new Terminal(parent);
    connect(terminal, SIGNAL(activated(int)), this, SLOT(setActiveTerminal(int)));
    connect(terminal, SIGNAL(manuallyActivated(Terminal*)), this, SIGNAL(terminalManuallyActivated(Terminal*)));
    connect(terminal, SIGNAL(titleChanged(int,QString)), this, SLOT(setTitle(int,QString)));
    connect(terminal, SIGNAL(keyboardInputBlocked(Terminal*)), this, SIGNAL(keyboardInputBlocked(Terminal*)));
    connect(terminal, SIGNAL(silenceDetected(Terminal*)), this, SIGNAL(silenceDetected(Terminal*)));
    connect(terminal, SIGNAL(destroyed(int)), this, SLOT(cleanup(int)));

    m_terminals.insert(terminal->id(), terminal);

    QWidget* terminalWidget = terminal->terminalWidget();
    if (terminalWidget) terminalWidget->setFocus();

    return terminal;
}

void Session::closeTerminal(int terminalId)
{
    if (terminalId == -1) terminalId = m_activeTerminalId;
    if (terminalId == -1) return;
    if (!m_terminals.contains(terminalId)) return;

    m_terminals.value(terminalId)->deletePart();
}

void Session::focusPreviousTerminal()
{
    if (m_activeTerminalId == -1) return;
    if (!m_terminals.contains(m_activeTerminalId)) return;

    QMapIterator<int, Terminal*> it(m_terminals);

    it.toBack();

    while (it.hasPrevious())
    {
        it.previous();

        if (it.key() == m_activeTerminalId)
        {
            if (it.hasPrevious())
            {
                QWidget* terminalWidget = it.peekPrevious().value()->terminalWidget();
                if (terminalWidget) terminalWidget->setFocus();
            }
            else
            {
                it.toBack();

                QWidget* terminalWidget = it.peekPrevious().value()->terminalWidget();
                if (terminalWidget) terminalWidget->setFocus();
            }

            break;
        }
    }
}

void Session::focusNextTerminal()
{
    if (m_activeTerminalId == -1) return;
    if (!m_terminals.contains(m_activeTerminalId)) return;

    QMapIterator<int, Terminal*> it(m_terminals);

    while (it.hasNext())
    {
        it.next();

        if (it.key() == m_activeTerminalId)
        {
            if (it.hasNext())
            {
                QWidget* terminalWidget = it.peekNext().value()->terminalWidget();
                if (terminalWidget) terminalWidget->setFocus();
            }
            else
            {
                it.toFront();

                QWidget* terminalWidget = it.peekNext().value()->terminalWidget();
                if (terminalWidget) terminalWidget->setFocus();
            }

            break;
        }
    }
}

int Session::splitLeftRight(int terminalId)
{
    if (terminalId == -1) terminalId = m_activeTerminalId;
    if (terminalId == -1) return -1;
    if (!m_terminals.contains(terminalId)) return -1;

    Terminal* terminal = m_terminals.value(terminalId);

    if (terminal)
        return split(terminal, Qt::Horizontal);
    else
        return -1;
}

int Session::splitTopBottom(int terminalId)
{
    if (terminalId == -1) terminalId = m_activeTerminalId;
    if (terminalId == -1) return -1;
    if (!m_terminals.contains(terminalId)) return -1;

    Terminal* terminal = m_terminals.value(terminalId);

    if (terminal)
        return split(terminal, Qt::Vertical);
    else
        return -1;
}

int Session::split(Terminal* terminal, Qt::Orientation orientation)
{
    Splitter* splitter = static_cast<Splitter*>(terminal->splitter());

    if (splitter->count() == 1)
    {
        int splitterWidth = splitter->width();

        if (splitter->orientation() != orientation)
            splitter->setOrientation(orientation);

        terminal = addTerminal(splitter);

        QList<int> newSplitterSizes;
        newSplitterSizes << (splitterWidth / 2) << (splitterWidth / 2);
        splitter->setSizes(newSplitterSizes);

        QWidget* partWidget = terminal->partWidget();
        if (partWidget) partWidget->show();

        m_activeTerminalId = terminal->id();
    }
    else
    {
        QList<int> splitterSizes = splitter->sizes();

        Splitter* newSplitter = new Splitter(orientation, splitter);
        connect(newSplitter, SIGNAL(destroyed()), this, SLOT(cleanup()));

        if (splitter->indexOf(terminal->partWidget()) == 0)
            splitter->insertWidget(0, newSplitter);

        QWidget* partWidget = terminal->partWidget();
        if (partWidget) partWidget->setParent(newSplitter);

        terminal->setSplitter(newSplitter);

        terminal = addTerminal(newSplitter);

        splitter->setSizes(splitterSizes);
        QList<int> newSplitterSizes;
        newSplitterSizes << (splitterSizes[1] / 2) << (splitterSizes[1] / 2);
        newSplitter->setSizes(newSplitterSizes);

        newSplitter->show();

        partWidget = terminal->partWidget();
        if (partWidget) partWidget->show();

        m_activeTerminalId = terminal->id();
    }

    return m_activeTerminalId;
}

int Session::tryGrowTerminal(int terminalId, GrowthDirection direction, uint pixels)
{
    Terminal* terminal = getTerminal(terminalId);
    Splitter* splitter = static_cast<Splitter*>(terminal->splitter());
    QWidget* child = terminal->partWidget();

    while (splitter)
    {
        bool isHorizontal = (direction == Right || direction == Left);
        bool isForward = (direction == Down || direction == Right);

        // Detecting correct orientation.
        if ((splitter->orientation() == Qt::Horizontal && isHorizontal)
            || (splitter->orientation() == Qt::Vertical && !isHorizontal))
        {

            int currentPos = splitter->indexOf(child);

            if (currentPos != -1 // Next/Prev movable element detection.
                && (currentPos != 0 || isForward)
                && (currentPos != splitter->count() - 1 || !isForward))
            {
                QList<int> currentSizes = splitter->sizes();
                int oldSize = currentSizes[currentPos];

                int affected = isForward ? currentPos + 1: currentPos -1;
                currentSizes[currentPos] += pixels;
                currentSizes[affected] -= pixels;
                splitter->setSizes(currentSizes);

                return splitter->sizes()[currentPos] - oldSize;
            }
        }
        // Try with a higher level.
        child = splitter;
        splitter = static_cast<Splitter*>(splitter->parentWidget());
    }

    return -1;
}

void Session::setActiveTerminal(int terminalId)
{
    m_activeTerminalId = terminalId;

    setTitle(m_activeTerminalId, m_terminals.value(m_activeTerminalId)->title());
}

void Session::setTitle(int terminalId, const QString& title)
{
    if (terminalId == m_activeTerminalId)
    {
        m_title = title;

        emit titleChanged(m_title);
        emit titleChanged(m_sessionId, m_title);
    }
}

void Session::cleanup(int terminalId)
{
    if (m_activeTerminalId == terminalId && m_terminals.count() > 1)
        focusPreviousTerminal();

    m_terminals.remove(terminalId);

    cleanup();
}

void Session::cleanup()
{
    if (!m_baseSplitter) return;

    m_baseSplitter->recursiveCleanup();

    if (m_terminals.count() == 0)
        m_baseSplitter->deleteLater();
}

void Session::prepareShutdown()
{
    m_baseSplitter = NULL;

    deleteLater();
}

const QString Session::terminalIdList()
{
    QList<int> keyList = m_terminals.uniqueKeys();
    QStringList idList;

    QListIterator<int> i(keyList);

    while (i.hasNext())
        idList << QString::number(i.next());

    return idList.join(",");
}

bool Session::hasTerminal(int terminalId)
{
    return m_terminals.contains(terminalId);
}

Terminal* Session::getTerminal(int terminalId)
{
    if (!m_terminals.contains(terminalId)) return 0;

    return m_terminals.value(terminalId);
}

void Session::runCommand(const QString& command, int terminalId)
{
    if (terminalId == -1) terminalId = m_activeTerminalId;
    if (terminalId == -1) return;
    if (!m_terminals.contains(terminalId)) return;

    m_terminals.value(terminalId)->runCommand(command);
}

void Session::manageProfiles()
{
    if ( m_activeTerminalId == -1) return;
    if (!m_terminals.contains(m_activeTerminalId)) return;

    m_terminals.value(m_activeTerminalId)->manageProfiles();
}

void Session::editProfile()
{
    if ( m_activeTerminalId == -1) return;
    if (!m_terminals.contains(m_activeTerminalId)) return;

    m_terminals.value(m_activeTerminalId)->editProfile();
}

bool Session::keyboardInputEnabled()
{
    int keyboardInputEnabledCount = 0;

    QMapIterator<int, Terminal*> i(m_terminals);

    while (i.hasNext())
        if (i.next().value()->keyboardInputEnabled())
            ++keyboardInputEnabledCount;

    return m_terminals.count() == keyboardInputEnabledCount;
}

void Session::setKeyboardInputEnabled(bool enabled)
{
    QMapIterator<int, Terminal*> i(m_terminals);

    while (i.hasNext())
        setKeyboardInputEnabled(i.next().key(), enabled);
}

bool Session::keyboardInputEnabled(int terminalId)
{
    if (!m_terminals.contains(terminalId)) return false;

    return m_terminals.value(terminalId)->keyboardInputEnabled();
}

void Session::setKeyboardInputEnabled(int terminalId, bool enabled)
{
    if (!m_terminals.contains(terminalId)) return;

    m_terminals.value(terminalId)->setKeyboardInputEnabled(enabled);
}

bool Session::hasTerminalsWithKeyboardInputEnabled()
{
    QMapIterator<int, Terminal*> i(m_terminals);

    while (i.hasNext())
        if (i.next().value()->keyboardInputEnabled())
            return true;

    return false;
}

bool Session::hasTerminalsWithKeyboardInputDisabled()
{
    QMapIterator<int, Terminal*> i(m_terminals);

    while (i.hasNext())
        if (!i.next().value()->keyboardInputEnabled())
            return true;

    return false;
}

bool Session::monitorActivityEnabled()
{
    int monitorActivityEnabledCount = 0;

    QMapIterator<int, Terminal*> i(m_terminals);

    while (i.hasNext())
        if (i.next().value()->monitorActivityEnabled())
            ++monitorActivityEnabledCount;

    return m_terminals.count() == monitorActivityEnabledCount;
}

void Session::setMonitorActivityEnabled(bool enabled)
{
    QMapIterator<int, Terminal*> i(m_terminals);

    while (i.hasNext())
        setMonitorActivityEnabled(i.next().key(), enabled);
}

bool Session::monitorActivityEnabled(int terminalId)
{
    if (!m_terminals.contains(terminalId)) return false;

    return m_terminals.value(terminalId)->monitorActivityEnabled();
}

void Session::setMonitorActivityEnabled(int terminalId, bool enabled)
{
    if (!m_terminals.contains(terminalId)) return;

    Terminal* terminal = m_terminals.value(terminalId);

    connect(terminal, SIGNAL(activityDetected(Terminal*)), this, SIGNAL(activityDetected(Terminal*)),
        Qt::UniqueConnection);

    terminal->setMonitorActivityEnabled(enabled);
}

bool Session::hasTerminalsWithMonitorActivityEnabled()
{
    QMapIterator<int, Terminal*> i(m_terminals);

    while (i.hasNext())
        if (i.next().value()->monitorActivityEnabled())
            return true;

    return false;
}

bool Session::hasTerminalsWithMonitorActivityDisabled()
{
    QMapIterator<int, Terminal*> i(m_terminals);

    while (i.hasNext())
        if (!i.next().value()->monitorActivityEnabled())
            return true;

    return false;
}

void Session::reconnectMonitorActivitySignals()
{
    QMapIterator<int, Terminal*> i(m_terminals);

    while (i.hasNext())
    {
        i.next();

        connect(i.value(), SIGNAL(activityDetected(Terminal*)), this, SIGNAL(activityDetected(Terminal*)),
            Qt::UniqueConnection);
    }
}

bool Session::monitorSilenceEnabled()
{
    int monitorSilenceEnabledCount = 0;

    QMapIterator<int, Terminal*> i(m_terminals);

    while (i.hasNext())
        if (i.next().value()->monitorSilenceEnabled())
            ++monitorSilenceEnabledCount;

    return m_terminals.count() == monitorSilenceEnabledCount;
}

void Session::setMonitorSilenceEnabled(bool enabled)
{
    QMapIterator<int, Terminal*> i(m_terminals);

    while (i.hasNext())
        setMonitorSilenceEnabled(i.next().key(), enabled);
}

bool Session::monitorSilenceEnabled(int terminalId)
{
    if (!m_terminals.contains(terminalId)) return false;

    return m_terminals.value(terminalId)->monitorSilenceEnabled();
}

void Session::setMonitorSilenceEnabled(int terminalId, bool enabled)
{
    if (!m_terminals.contains(terminalId)) return;

    m_terminals.value(terminalId)->setMonitorSilenceEnabled(enabled);
}

bool Session::hasTerminalsWithMonitorSilenceDisabled()
{
    QMapIterator<int, Terminal*> i(m_terminals);

    while (i.hasNext())
        if (!i.next().value()->monitorSilenceEnabled())
            return true;

    return false;
}

bool Session::hasTerminalsWithMonitorSilenceEnabled()
{
    QMapIterator<int, Terminal*> i(m_terminals);

    while (i.hasNext())
        if (i.next().value()->monitorSilenceEnabled())
            return true;

    return false;
}
