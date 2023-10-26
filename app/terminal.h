/*
  SPDX-FileCopyrightText: 2008-2014 Eike Hein <hein@kde.org>

  SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#ifndef TERMINAL_H
#define TERMINAL_H

#include <KParts/Part>

#include <QPointer>

class QKeyEvent;

// Requires V2 to access profileProperty().
class TerminalInterface;

class Terminal : public QObject
{
    Q_OBJECT

public:
    explicit Terminal(const QString &workingDir, QWidget *parent = nullptr);
    ~Terminal();

    bool eventFilter(QObject *watched, QEvent *event) override;

    int id()
    {
        return m_terminalId;
    }
    const QString title()
    {
        return m_title;
    }

    QWidget *partWidget()
    {
        return m_partWidget;
    }
    QWidget *terminalWidget()
    {
        return m_terminalWidget;
    }

    QWidget *splitter()
    {
        return m_parentSplitter;
    }
    void setSplitter(QWidget *splitter)
    {
        m_parentSplitter = splitter;
    }

    void runCommand(const QString &command);

    void manageProfiles();
    void editProfile();

    bool keyboardInputEnabled()
    {
        return m_keyboardInputEnabled;
    }
    void setKeyboardInputEnabled(bool enabled)
    {
        m_keyboardInputEnabled = enabled;
    }

    bool monitorActivityEnabled()
    {
        return m_monitorActivityEnabled;
    }
    void setMonitorActivityEnabled(bool enabled);

    bool monitorSilenceEnabled()
    {
        return m_monitorSilenceEnabled;
    }
    void setMonitorSilenceEnabled(bool enabled);

    QString currentWorkingDirectory() const;

    void deletePart();

    KActionCollection *actionCollection();

    bool wantsBlur() const
    {
        return m_wantsBlur;
    }

Q_SIGNALS:
    void titleChanged(int terminalId, const QString &title);
    void activated(int terminalId);
    void manuallyActivated(Terminal *terminal);
    void keyboardInputBlocked(Terminal *terminal);
    void activityDetected(Terminal *terminal);
    void silenceDetected(Terminal *terminal);
    void destroyed(int terminalId);
    void closeRequested(int terminalId);

private Q_SLOTS:
    void setTitle(const QString &title);
    void overrideShortcut(QKeyEvent *event, bool &override);
    void silenceDetected();
    void activityDetected();

private:
    void disableOffendingPartActions();

    void displayKPartLoadError();

    static int m_availableTerminalId;
    int m_terminalId;

    KParts::Part *m_part = nullptr;
    TerminalInterface *m_terminalInterface = nullptr;
    QWidget *m_partWidget = nullptr;
    QPointer<QWidget> m_terminalWidget = nullptr;
    QWidget *m_parentSplitter;

    QString m_title;

    bool m_keyboardInputEnabled = true;

    bool m_monitorActivityEnabled = false;
    bool m_monitorSilenceEnabled = false;
    bool m_wantsBlur = false;

    bool m_destroying = false;
};

#endif
