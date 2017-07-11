/*
  Copyright (C) 2008-2014 by Eike Hein <hein@kde.org>

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


#ifndef TERMINAL_H
#define TERMINAL_H

#include <KParts/Part>

#include <QPointer>


class QKeyEvent;

class TerminalInterface;


class Terminal : public QObject
{
    Q_OBJECT

    public:
        explicit Terminal(const QString& workingDir, QWidget* parent = 0);
         ~Terminal();

        bool eventFilter(QObject* watched, QEvent* event) Q_DECL_OVERRIDE;

        int id() { return m_terminalId; }
        const QString title() { return m_title; }

        QString currentDir(bool *ok = NULL) const;

        QWidget* partWidget() { return m_partWidget; }
        QWidget* terminalWidget() { return m_terminalWidget; }

        QWidget* splitter() { return m_parentSplitter; }
        void setSplitter(QWidget* splitter) { m_parentSplitter = splitter; }

        void runCommand(const QString& command);

        void manageProfiles();
        void editProfile();

        bool keyboardInputEnabled() { return m_keyboardInputEnabled; }
        void setKeyboardInputEnabled(bool enabled) { m_keyboardInputEnabled = enabled; }

        bool monitorActivityEnabled() { return m_monitorActivityEnabled; }
        void setMonitorActivityEnabled(bool enabled);

        bool monitorSilenceEnabled() { return m_monitorSilenceEnabled; }
        void setMonitorSilenceEnabled(bool enabled);

        void deletePart();


    Q_SIGNALS:
        void titleChanged(int terminalId, const QString& title);
        void activated(int terminalId);
        void manuallyActivated(Terminal* terminal);
        void keyboardInputBlocked(Terminal* terminal);
        void activityDetected(Terminal* terminal);
        void silenceDetected(Terminal* terminal);
        void destroyed(int terminalId);


    private Q_SLOTS:
        void setTitle(const QString& title);
        void overrideShortcut(QKeyEvent* event, bool& override);
        void silenceDetected();
        void activityDetected();


    private:
        void disableOffendingPartActions();

        void displayKPartLoadError();

        static int m_availableTerminalId;
        int m_terminalId;

        KParts::Part* m_part;
        TerminalInterface* m_terminalInterface;
        QWidget* m_partWidget;
        QPointer<QWidget> m_terminalWidget;
        QWidget* m_parentSplitter;

        QString m_title;

        bool m_keyboardInputEnabled;

        bool m_monitorActivityEnabled;
        bool m_monitorSilenceEnabled;
};

#endif
