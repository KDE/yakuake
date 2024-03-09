/*
  SPDX-FileCopyrightText: 2008-2014 Eike Hein <hein@kde.org>
  SPDX-FileCopyrightText: 2009 Juan Carlos Torres <carlosdgtorres@gmail.com>

  SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#include "session.h"
#include "terminal.h"

#include <algorithm>

int Session::m_availableSessionId = 0;

Session::Session(const QString &workingDir, SessionType type, QWidget *parent)
    : QObject(parent)
{
    m_workingDir = workingDir;
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
    if (m_baseSplitter)
        delete m_baseSplitter;

    Q_EMIT destroyed(m_sessionId);
}

void Session::setupSession(SessionType type)
{
    switch (type) {
    case Single: {
        Terminal *terminal = addTerminal(m_baseSplitter);
        setActiveTerminal(terminal->id());

        break;
    }

    case TwoHorizontal: {
        int splitterWidth = m_baseSplitter->width();

        Terminal *terminal = addTerminal(m_baseSplitter);
        addTerminal(m_baseSplitter);

        QList<int> newSplitterSizes;
        newSplitterSizes << (splitterWidth / 2) << (splitterWidth / 2);
        m_baseSplitter->setSizes(newSplitterSizes);

        QWidget *terminalWidget = terminal->terminalWidget();

        if (terminalWidget) {
            terminalWidget->setFocus();
            setActiveTerminal(terminal->id());
        }

        break;
    }

    case TwoVertical: {
        m_baseSplitter->setOrientation(Qt::Vertical);

        int splitterHeight = m_baseSplitter->height();

        Terminal *terminal = addTerminal(m_baseSplitter);
        addTerminal(m_baseSplitter);

        QList<int> newSplitterSizes;
        newSplitterSizes << (splitterHeight / 2) << (splitterHeight / 2);
        m_baseSplitter->setSizes(newSplitterSizes);

        QWidget *terminalWidget = terminal->terminalWidget();

        if (terminalWidget) {
            terminalWidget->setFocus();
            setActiveTerminal(terminal->id());
        }

        break;
    }

    case Quad: {
        int splitterWidth = m_baseSplitter->width();
        int splitterHeight = m_baseSplitter->height();

        m_baseSplitter->setOrientation(Qt::Vertical);

        Splitter *upperSplitter = new Splitter(Qt::Horizontal, m_baseSplitter);
        connect(upperSplitter, SIGNAL(destroyed()), this, SLOT(cleanup()));

        Splitter *lowerSplitter = new Splitter(Qt::Horizontal, m_baseSplitter);
        connect(lowerSplitter, SIGNAL(destroyed()), this, SLOT(cleanup()));

        Terminal *terminal = addTerminal(upperSplitter);
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

        QWidget *terminalWidget = terminal->terminalWidget();

        if (terminalWidget) {
            terminalWidget->setFocus();
            setActiveTerminal(terminal->id());
        }

        break;
    }

    default: {
        addTerminal(m_baseSplitter);

        break;
    }
    }
}

Terminal *Session::addTerminal(QSplitter *parent, QString workingDir)
{
    if (workingDir.isEmpty()) {
        // fallback to session's default working dir
        workingDir = m_workingDir;
    }

    std::unique_ptr<Terminal> terminal = std::make_unique<Terminal>(workingDir, parent);
    connect(terminal.get(), SIGNAL(activated(int)), this, SLOT(setActiveTerminal(int)));
    connect(terminal.get(), SIGNAL(manuallyActivated(Terminal *)), this, SIGNAL(terminalManuallyActivated(Terminal *)));
    connect(terminal.get(), SIGNAL(titleChanged(int, QString)), this, SLOT(setTitle(int, QString)));
    connect(terminal.get(), SIGNAL(keyboardInputBlocked(Terminal *)), this, SIGNAL(keyboardInputBlocked(Terminal *)));
    connect(terminal.get(), SIGNAL(silenceDetected(Terminal *)), this, SIGNAL(silenceDetected(Terminal *)));
    connect(terminal.get(), &Terminal::closeRequested, this, QOverload<int>::of(&Session::cleanup));

    Terminal *term = terminal.get();

    m_terminals[terminal->id()] = std::move(terminal);

    Q_EMIT wantsBlurChanged();

    parent->addWidget(term->partWidget());
    QWidget *terminalWidget = term->terminalWidget();
    if (terminalWidget)
        terminalWidget->setFocus();

    return term;
}

void Session::closeTerminal(int terminalId)
{
    if (terminalId == -1)
        terminalId = m_activeTerminalId;
    if (terminalId == -1)
        return;
    if (!m_terminals.contains(terminalId))
        return;

    cleanup(terminalId);
}

void Session::focusPreviousTerminal()
{
    if (m_activeTerminalId == -1)
        return;
    if (!m_terminals.contains(m_activeTerminalId))
        return;

    std::map<int, std::unique_ptr<Terminal>>::iterator currentTerminal = m_terminals.find(m_activeTerminalId);

    std::map<int, std::unique_ptr<Terminal>>::iterator previousTerminal;

    if (currentTerminal == m_terminals.begin()) {
        previousTerminal = std::prev(m_terminals.end());
    } else {
        previousTerminal = std::prev(currentTerminal);
    }

    QWidget *terminalWidget = previousTerminal->second->terminalWidget();
    if (terminalWidget) {
        terminalWidget->setFocus();
    }
}

void Session::focusNextTerminal()
{
    if (m_activeTerminalId == -1)
        return;
    if (!m_terminals.contains(m_activeTerminalId))
        return;

    std::map<int, std::unique_ptr<Terminal>>::iterator currentTerminal = m_terminals.find(m_activeTerminalId);

    std::map<int, std::unique_ptr<Terminal>>::iterator nextTerminal = std::next(currentTerminal);

    if (nextTerminal == m_terminals.end()) {
        nextTerminal = m_terminals.begin();
    }

    QWidget *terminalWidget = nextTerminal->second->terminalWidget();
    if (terminalWidget) {
        terminalWidget->setFocus();
    }
}

int Session::splitLeftRight(int terminalId)
{
    if (terminalId == -1)
        terminalId = m_activeTerminalId;
    if (terminalId == -1)
        return -1;
    if (!m_terminals.contains(terminalId))
        return -1;

    Terminal *terminal = m_terminals[terminalId].get();

    if (terminal)
        return split(terminal, Qt::Horizontal);
    else
        return -1;
}

int Session::splitTopBottom(int terminalId)
{
    if (terminalId == -1)
        terminalId = m_activeTerminalId;
    if (terminalId == -1)
        return -1;
    if (!m_terminals.contains(terminalId))
        return -1;

    Terminal *terminal = m_terminals[terminalId].get();

    if (terminal)
        return split(terminal, Qt::Vertical);
    else
        return -1;
}

int Session::split(Terminal *terminal, Qt::Orientation orientation)
{
    Splitter *splitter = static_cast<Splitter *>(terminal->splitter());

    if (splitter->count() == 1) {
        int splitterWidth = splitter->width();

        if (splitter->orientation() != orientation)
            splitter->setOrientation(orientation);

        terminal = addTerminal(splitter, terminal->currentWorkingDirectory());

        QList<int> newSplitterSizes;
        newSplitterSizes << (splitterWidth / 2) << (splitterWidth / 2);
        splitter->setSizes(newSplitterSizes);

        QWidget *partWidget = terminal->partWidget();
        if (partWidget)
            partWidget->show();

        m_activeTerminalId = terminal->id();
    } else {
        QList<int> splitterSizes = splitter->sizes();

        Splitter *newSplitter = new Splitter(orientation, splitter);
        connect(newSplitter, SIGNAL(destroyed()), this, SLOT(cleanup()));

        if (splitter->indexOf(terminal->partWidget()) == 0)
            splitter->insertWidget(0, newSplitter);

        QWidget *partWidget = terminal->partWidget();
        if (partWidget)
            partWidget->setParent(newSplitter);

        terminal->setSplitter(newSplitter);

        terminal = addTerminal(newSplitter, terminal->currentWorkingDirectory());

        splitter->setSizes(splitterSizes);
        QList<int> newSplitterSizes;
        newSplitterSizes << (splitterSizes[1] / 2) << (splitterSizes[1] / 2);
        newSplitter->setSizes(newSplitterSizes);

        newSplitter->show();

        partWidget = terminal->partWidget();
        if (partWidget)
            partWidget->show();

        m_activeTerminalId = terminal->id();
    }

    return m_activeTerminalId;
}

int Session::tryGrowTerminal(int terminalId, GrowthDirection direction, uint pixels)
{
    Terminal *terminal = getTerminal(terminalId);
    Splitter *splitter = static_cast<Splitter *>(terminal->splitter());
    QWidget *child = terminal->partWidget();

    while (splitter) {
        bool isHorizontal = (direction == Right || direction == Left);
        bool isForward = (direction == Down || direction == Right);

        // Detecting correct orientation.
        if ((splitter->orientation() == Qt::Horizontal && isHorizontal) || (splitter->orientation() == Qt::Vertical && !isHorizontal)) {
            int currentPos = splitter->indexOf(child);

            if (currentPos != -1 // Next/Prev movable element detection.
                && (currentPos != 0 || isForward) && (currentPos != splitter->count() - 1 || !isForward)) {
                QList<int> currentSizes = splitter->sizes();
                int oldSize = currentSizes[currentPos];

                int affected = isForward ? currentPos + 1 : currentPos - 1;
                currentSizes[currentPos] += pixels;
                currentSizes[affected] -= pixels;
                splitter->setSizes(currentSizes);

                return splitter->sizes().at(currentPos) - oldSize;
            }
        }
        // Try with a higher level.
        child = splitter;
        splitter = static_cast<Splitter *>(splitter->parentWidget());
    }

    return -1;
}

void Session::setActiveTerminal(int terminalId)
{
    m_activeTerminalId = terminalId;

    setTitle(m_activeTerminalId, m_terminals[m_activeTerminalId]->title());
}

void Session::setTitle(int terminalId, const QString &title)
{
    if (terminalId == m_activeTerminalId) {
        m_title = title;

        Q_EMIT titleChanged(m_title);
        Q_EMIT titleChanged(m_sessionId, m_title);
    }
}

void Session::cleanup(int terminalId)
{
    if (m_activeTerminalId == terminalId && m_terminals.size() > 1)
        focusPreviousTerminal();

    m_terminals.erase(terminalId);
    Q_EMIT wantsBlurChanged();

    cleanup();
}

void Session::cleanup()
{
    if (!m_baseSplitter)
        return;

    m_baseSplitter->recursiveCleanup();

    if (m_terminals.empty())
        m_baseSplitter->deleteLater();
}

void Session::prepareShutdown()
{
    m_baseSplitter = nullptr;

    deleteLater();
}

const QString Session::terminalIdList()
{
    QStringList idList;
    for (auto &[id, terminal] : m_terminals) {
        idList << QString::number(id);
    }

    return idList.join(QLatin1Char(','));
}

bool Session::hasTerminal(int terminalId)
{
    return m_terminals.contains(terminalId);
}

Terminal *Session::getTerminal(int terminalId)
{
    if (!m_terminals.contains(terminalId))
        return nullptr;

    return m_terminals[terminalId].get();
}

void Session::runCommand(const QString &command, int terminalId)
{
    if (terminalId == -1)
        terminalId = m_activeTerminalId;
    if (terminalId == -1)
        return;
    if (!m_terminals.contains(terminalId))
        return;

    m_terminals[terminalId]->runCommand(command);
}

void Session::manageProfiles()
{
    if (m_activeTerminalId == -1)
        return;
    if (!m_terminals.contains(m_activeTerminalId))
        return;

    m_terminals[m_activeTerminalId]->manageProfiles();
}

void Session::editProfile()
{
    if (m_activeTerminalId == -1)
        return;
    if (!m_terminals.contains(m_activeTerminalId))
        return;

    m_terminals[m_activeTerminalId]->editProfile();
}

bool Session::keyboardInputEnabled()
{
    return std::all_of(m_terminals.cbegin(), m_terminals.cend(), [](auto &it) {
        auto &[id, terminal] = it;
        return terminal->keyboardInputEnabled();
    });
}

void Session::setKeyboardInputEnabled(bool enabled)
{
    for (auto &[id, terminal] : m_terminals) {
        terminal->setKeyboardInputEnabled(enabled);
    }
}

bool Session::keyboardInputEnabled(int terminalId)
{
    if (!m_terminals.contains(terminalId))
        return false;

    return m_terminals[terminalId]->keyboardInputEnabled();
}

void Session::setKeyboardInputEnabled(int terminalId, bool enabled)
{
    if (!m_terminals.contains(terminalId))
        return;

    m_terminals[terminalId]->setKeyboardInputEnabled(enabled);
}

bool Session::hasTerminalsWithKeyboardInputEnabled()
{
    return std::any_of(m_terminals.cbegin(), m_terminals.cend(), [](auto &it) {
        auto &[id, terminal] = it;
        return terminal->keyboardInputEnabled();
    });
}

bool Session::hasTerminalsWithKeyboardInputDisabled()
{
    return std::any_of(m_terminals.cbegin(), m_terminals.cend(), [](auto &it) {
        auto &[id, terminal] = it;
        return !terminal->keyboardInputEnabled();
    });
}

bool Session::monitorActivityEnabled()
{
    return std::all_of(m_terminals.cbegin(), m_terminals.cend(), [](auto &it) {
        auto &[id, terminal] = it;
        return terminal->monitorActivityEnabled();
    });
}

void Session::setMonitorActivityEnabled(bool enabled)
{
    for (auto &[id, terminal] : m_terminals) {
        setMonitorActivityEnabled(id, enabled);
    }
}

bool Session::monitorActivityEnabled(int terminalId)
{
    if (!m_terminals.contains(terminalId))
        return false;

    return m_terminals[terminalId]->monitorActivityEnabled();
}

void Session::setMonitorActivityEnabled(int terminalId, bool enabled)
{
    if (!m_terminals.contains(terminalId))
        return;

    Terminal *terminal = m_terminals[terminalId].get();

    connect(terminal, SIGNAL(activityDetected(Terminal *)), this, SIGNAL(activityDetected(Terminal *)), Qt::UniqueConnection);

    terminal->setMonitorActivityEnabled(enabled);
}

bool Session::hasTerminalsWithMonitorActivityEnabled()
{
    return std::any_of(m_terminals.cbegin(), m_terminals.cend(), [](auto &it) {
        auto &[id, terminal] = it;
        return terminal->monitorActivityEnabled();
    });
}

bool Session::hasTerminalsWithMonitorActivityDisabled()
{
    return std::any_of(m_terminals.cbegin(), m_terminals.cend(), [](auto &it) {
        auto &[id, terminal] = it;
        return !terminal->monitorActivityEnabled();
    });
}

void Session::reconnectMonitorActivitySignals()
{
    for (auto &[id, terminal] : m_terminals) {
        // clang-format off
        connect(terminal.get(), SIGNAL(activityDetected(Terminal*)), this, SIGNAL(activityDetected(Terminal*)), Qt::UniqueConnection);
        // clang-format on
    }
}

bool Session::monitorSilenceEnabled()
{
    return std::all_of(m_terminals.cbegin(), m_terminals.cend(), [](auto &it) {
        auto &[id, terminal] = it;
        return terminal->monitorSilenceEnabled();
    });
}

void Session::setMonitorSilenceEnabled(bool enabled)
{
    for (auto &[id, terminal] : m_terminals) {
        terminal->setMonitorSilenceEnabled(enabled);
    }
}

bool Session::monitorSilenceEnabled(int terminalId)
{
    if (!m_terminals.contains(terminalId))
        return false;

    return m_terminals[terminalId]->monitorSilenceEnabled();
}

void Session::setMonitorSilenceEnabled(int terminalId, bool enabled)
{
    if (!m_terminals.contains(terminalId))
        return;

    m_terminals[terminalId]->setMonitorSilenceEnabled(enabled);
}

bool Session::hasTerminalsWithMonitorSilenceDisabled()
{
    return std::any_of(m_terminals.cbegin(), m_terminals.cend(), [](auto &it) {
        auto &[id, terminal] = it;
        return !terminal->monitorSilenceEnabled();
    });
}

bool Session::hasTerminalsWithMonitorSilenceEnabled()
{
    return std::any_of(m_terminals.cbegin(), m_terminals.cend(), [](auto &it) {
        auto &[id, terminal] = it;
        return terminal->monitorSilenceEnabled();
    });
}

bool Session::wantsBlur() const
{
    return std::any_of(m_terminals.cbegin(), m_terminals.cend(), [](auto &it) {
        auto &[id, terminal] = it;
        return terminal->wantsBlur();
    });
}

#include "moc_session.cpp"
