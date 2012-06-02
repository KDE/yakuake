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


#include "visualeventoverlay.h"
#include "sessionstack.h"
#include "settings.h"
#include "terminal.h"

#include <KColorScheme>

#include <QPainter>
#include <QTimer>


EventRect::EventRect(const QPoint& topLeft, const QPoint& bottomRight, EventType type,
    EventFlags flags) : QRect(topLeft, bottomRight)
{
    m_eventType = type;

    m_eventFlags = flags;

    m_timeStamp.start();
}

EventRect::~EventRect()
{
}

bool EventRect::operator==(const EventRect& eventRect) const
{
    if (m_eventType == eventRect.eventType() && eventRect.testFlag(EventRect::Singleton))
        return true;

    if (m_eventType != eventRect.eventType())
        return false;

    return ::operator==(*this, eventRect);
}

bool EventRect::operator<(const EventRect& eventRect) const
{
    if (!testFlag(EventRect::Exclusive) && eventRect.testFlag(EventRect::Exclusive))
        return false;
    if (m_eventType < eventRect.eventType())
        return true;
    else if (m_timeStamp < eventRect.timeStamp())
        return true;

    return false;
}

VisualEventOverlay::VisualEventOverlay(SessionStack* parent) : QWidget(parent)
{
    m_sessionStack = parent;

    setAutoFillBackground(false);

    setAttribute(Qt::WA_TranslucentBackground, true);

    setFocusPolicy(Qt::NoFocus);
    setAttribute(Qt::WA_TransparentForMouseEvents, true);

    m_cleanupTimer = new QTimer(this);
    m_cleanupTimer->setSingleShot(true);
    connect(m_cleanupTimer, SIGNAL(timeout()), this, SLOT(cleanupOverlay()));

    m_cleanupTimerCeiling = 0;

    hide();
}

VisualEventOverlay::~VisualEventOverlay()
{
}

void VisualEventOverlay::highlightTerminal(Terminal* terminal, bool persistent)
{
    if (!persistent && Settings::terminalHighlightDuration() == 0)
        return;

    if (isHidden()) show();

    EventRect::EventFlags flags = EventRect::Singleton | EventRect::Exclusive;
    if (persistent) flags |= EventRect::Persistent;

    terminalEvent(terminal, EventRect::TerminalHighlight, flags);

    if (!persistent)
        scheduleCleanup(Settings::terminalHighlightDuration());
}

void VisualEventOverlay::removeTerminalHighlight()
{
    if (!m_eventRects.count()) return;

    QMutableListIterator<EventRect> i(m_eventRects);

    while (i.hasNext())
    {
        if (i.next().eventType() == EventRect::TerminalHighlight)
            i.remove();
    }

    if (m_sessionStack->requiresVisualEventOverlay())
        update();
    else
        hide();
}

void VisualEventOverlay::indicateKeyboardInputBlocked(Terminal* terminal)
{
    if (Settings::keyboardInputBlockIndicatorDuration() == 0)
        return;

    terminalEvent(terminal, EventRect::KeyboardInputBlocked);

    scheduleCleanup(Settings::keyboardInputBlockIndicatorDuration());
}

void VisualEventOverlay::terminalEvent(Terminal* terminal, EventRect::EventType type, EventRect::EventFlags flags)
{
    QRect partRect(terminal->partWidget()->rect());
    const QWidget* partWidget = terminal->partWidget();

    QPoint topLeft(partWidget->mapTo(parentWidget(), partRect.topLeft()));
    QPoint bottomRight(partWidget->mapTo(parentWidget(), partRect.bottomRight()));

    EventRect eventRect(topLeft, bottomRight, type, flags);

    m_eventRects.removeAll(eventRect);
    m_eventRects.append(eventRect);

    qSort(m_eventRects);

    update();
}

void VisualEventOverlay::paintEvent(QPaintEvent*)
{
    if (!m_eventRects.count()) return;

    QPainter painter(this);

    m_time.start();
    bool painted = false;

    QListIterator<EventRect> i(m_eventRects);

    while (i.hasNext())
    {
        const EventRect& eventRect = i.next();

        painted = false;

        if (eventRect.eventType() == EventRect::TerminalHighlight
            && (eventRect.timeStamp().msecsTo(m_time) <= Settings::terminalHighlightDuration()
            || eventRect.testFlag(EventRect::Persistent)))
        {
            KStatefulBrush terminalHighlightBrush(KColorScheme::View, KColorScheme::HoverColor);

            painter.setOpacity(Settings::terminalHighlightOpacity());

            painter.fillRect(eventRect, terminalHighlightBrush.brush(this));

            painted = true;
        }
        else if (eventRect.eventType() == EventRect::KeyboardInputBlocked
            && eventRect.timeStamp().msecsTo(m_time) <= Settings::keyboardInputBlockIndicatorDuration())
        {
            painter.setOpacity(Settings::keyboardInputBlockIndicatorOpacity());

            painter.fillRect(eventRect, Settings::keyboardInputBlockIndicatorColor());

            painted = true;
        }

        if (painted && i.hasNext() && eventRect.testFlag(EventRect::Exclusive))
        {
            if (!painter.hasClipping())
                painter.setClipRect(rect());

            painter.setClipRegion(painter.clipRegion().subtracted(eventRect));
        }
    }
}

void VisualEventOverlay::showEvent(QShowEvent*)
{
    resize(parentWidget()->rect().size());

    raise();
}

void VisualEventOverlay::hideEvent(QHideEvent*)
{
    m_cleanupTimer->stop();

    m_eventRects.clear();
}

void VisualEventOverlay::scheduleCleanup(int in)
{
    int left = m_cleanupTimerCeiling - m_cleanupTimerStarted.elapsed();

    if (in > left)
    {
        m_cleanupTimerCeiling = in;
        m_cleanupTimerStarted.start();
        m_cleanupTimer->start(in);
    }
}

void VisualEventOverlay::cleanupOverlay()
{
    if (m_eventRects.count())
    {
        m_time.start();

        QMutableListIterator<EventRect> i(m_eventRects);

        while (i.hasNext())
        {
            const EventRect& eventRect = i.next();

            if (eventRect.eventType() == EventRect::TerminalHighlight
                && eventRect.timeStamp().msecsTo(m_time) >= Settings::terminalHighlightDuration()
                && !eventRect.testFlag(EventRect::Persistent))
            {
                i.remove();
            }
            else if (eventRect.eventType() == EventRect::KeyboardInputBlocked
                && eventRect.timeStamp().msecsTo(m_time) >= Settings::keyboardInputBlockIndicatorDuration())
            {
                i.remove();
            }
        }
    }

    if (m_sessionStack->requiresVisualEventOverlay())
        update();
    else
        hide();
}
