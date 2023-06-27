/*
  SPDX-FileCopyrightText: 2008-2009 Eike Hein <hein@kde.org>
  SPDX-FileCopyrightText: 2009 Juan Carlos Torres <carlosdgtorres@gmail.com>

  SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#ifndef SESSION_H
#define SESSION_H

#include "splitter.h"

#include <QObject>

class Terminal;

class Session : public QObject
{
    Q_OBJECT

public:
    enum SessionType {
        Single,
        TwoHorizontal,
        TwoVertical,
        Quad,
    };
    enum GrowthDirection {
        Up,
        Right,
        Down,
        Left,
    };

    explicit Session(const QString &workingDir, SessionType type = Single, QWidget *parent = nullptr);
    ~Session();

    int id()
    {
        return m_sessionId;
    }
    const QString title()
    {
        return m_title;
    }
    QWidget *widget()
    {
        return m_baseSplitter;
    }

    int activeTerminalId()
    {
        return m_activeTerminalId;
    }
    const QString terminalIdList();
    int terminalCount()
    {
        return m_terminals.size();
    }
    bool hasTerminal(int terminalId);
    Terminal *getTerminal(int terminalId);

    bool closable()
    {
        return m_closable;
    }
    void setClosable(bool closable)
    {
        m_closable = closable;
    }

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

    bool wantsBlur() const;

public Q_SLOTS:
    void closeTerminal(int terminalId = -1);

    void focusNextTerminal();
    void focusPreviousTerminal();

    int splitLeftRight(int terminalId = -1);
    int splitTopBottom(int terminalId = -1);

    int tryGrowTerminal(int terminalId, GrowthDirection direction, uint pixels);

    void runCommand(const QString &command, int terminalId = -1);

    void manageProfiles();
    void editProfile();

    void reconnectMonitorActivitySignals();

Q_SIGNALS:
    void titleChanged(const QString &title);
    void titleChanged(int sessionId, const QString &title);
    void terminalManuallyActivated(Terminal *terminal);
    void keyboardInputBlocked(Terminal *terminal);
    void activityDetected(Terminal *terminal);
    void silenceDetected(Terminal *terminal);
    void destroyed(int sessionId);
    void wantsBlurChanged();

private Q_SLOTS:
    void setActiveTerminal(int terminalId);
    void setTitle(int terminalId, const QString &title);

    void cleanup(int terminalId);
    void cleanup();
    void prepareShutdown();

private:
    void setupSession(SessionType type);

    Terminal *addTerminal(QSplitter *parent, QString workingDir = QString());
    int split(Terminal *terminal, Qt::Orientation orientation);

    QString m_workingDir;
    static int m_availableSessionId;
    int m_sessionId;

    Splitter *m_baseSplitter;

    int m_activeTerminalId;
    std::map<int, std::unique_ptr<Terminal>> m_terminals;

    QString m_title;

    bool m_closable;
};

#endif
