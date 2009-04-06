/*
  Copyright (C) 2009 by Eike Hein <hein@kde.org>

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

#ifndef VISUALEVENTOVERLAY_H
#define VISUALEVENTOVERLAY_H


#include <QRect>
#include <QTime>
#include <QWidget>


class SessionStack;
class Terminal;

class QTimer;


class EventRect : public QRect
{
    public:
        enum EventType { TerminalHighlight, KeyboardInputBlocked };

        enum EventFlag
        {
            NoFlags    = 0x00000000,
            Singleton  = 0x00000001,
            Exclusive  = 0x00000002,
            Persistent = 0x00000004
        };
        Q_DECLARE_FLAGS(EventFlags, EventFlag)

        EventRect(const QPoint& topLeft, const QPoint& bottomRight, EventType type,
            EventFlags flags = EventRect::NoFlags);
        ~EventRect();

        EventType eventType() const { return m_eventType; }
        const QTime& timeStamp() const { return m_timeStamp; }

        EventFlags eventFlags() const { return m_eventFlags; }
        void setEventFlags(EventFlags flags) { m_eventFlags = flags; }
        inline bool testFlag(EventFlag flag) const { return m_eventFlags & flag; }

        bool operator==(const EventRect& eventRect) const;
        bool operator<(const EventRect& eventRect) const;


    private:
        EventType m_eventType;
        EventFlags m_eventFlags;

        QTime m_timeStamp;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(EventRect::EventFlags)


class VisualEventOverlay : public QWidget
{
    Q_OBJECT

    public:
        explicit VisualEventOverlay(SessionStack* parent = 0);
         ~VisualEventOverlay();


    public slots:
        void highlightTerminal(Terminal* terminal, bool persistent = false);
        void removeTerminalHighlight();

        void indicateKeyboardInputBlocked(Terminal* terminal);

        void terminalEvent(Terminal* terminal, EventRect::EventType type,
            EventRect::EventFlags flags = EventRect::NoFlags);


    protected:
        virtual void showEvent(QShowEvent*);
        virtual void hideEvent(QHideEvent*);
        virtual void paintEvent(QPaintEvent*);


    private slots:
        void cleanupOverlay();


    private:
        void scheduleCleanup(int in);

        QList<EventRect> m_eventRects;

        QTimer* m_cleanupTimer;
        QTime m_cleanupTimerStarted;
        int m_cleanupTimerCeiling;

        QTime m_time;

        SessionStack* m_sessionStack;
};

#endif
