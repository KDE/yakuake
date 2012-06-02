/*
  Copyright (C) 2008-2009 by Eike Hein <hein@kde.org>

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


#include "terminal.h"
#include "settings.h"

#include <KActionCollection>
#include <KApplication>
#include <KColorScheme>
#include <kde_terminal_interface.h>
#include <KLocalizedString>
#include <KIcon>
#include <KPluginFactory>
#include <KPluginLoader>
#include <KService>
#include <KUser>

#include <QAction>
#include <QHBoxLayout>
#include <QLabel>
#include <QWidget>

#include <QKeyEvent>


int Terminal::m_availableTerminalId = 0;

Terminal::Terminal(QWidget* parent) : QObject(parent)
{
    m_terminalId = m_availableTerminalId;
    m_availableTerminalId++;

    m_keyboardInputEnabled = true;

    m_monitorActivityEnabled = false;
    m_monitorSilenceEnabled = false;

    m_part = NULL;
    m_terminalInterface = NULL;
    m_partWidget = NULL;
    m_terminalWidget = NULL;
    m_parentSplitter = parent;

    KPluginFactory* factory = 0;
    KService::Ptr service = KService::serviceByDesktopName("konsolepart");
    if( service )
    {
        factory = KPluginLoader(service->library()).factory();
    }

    m_part = factory ? (factory->create<KParts::Part>(parent)) : 0;

    if (m_part)
    {
        connect(m_part, SIGNAL(setWindowCaption(QString)), this, SLOT(setTitle(QString)));
        connect(m_part, SIGNAL(overrideShortcut(QKeyEvent*,bool&)), this, SLOT(overrideShortcut(QKeyEvent*,bool&)));
        connect(m_part, SIGNAL(destroyed()), this, SLOT(deleteLater()));

        m_partWidget = m_part->widget();

        m_terminalWidget = m_part->widget()->focusWidget();

        if (m_terminalWidget)
        {
            m_terminalWidget->setFocusPolicy(Qt::WheelFocus);
            m_terminalWidget->installEventFilter(this);
        }

        disableOffendingPartActions();

        m_terminalInterface = qobject_cast<TerminalInterface*>(m_part);
        if (m_terminalInterface) m_terminalInterface->showShellInDir(KUser().homeDir());
    }
    else
        displayKPartLoadError();
}

Terminal::~Terminal()
{
    emit destroyed(m_terminalId);
}

void Terminal::deletePart()
{
    if (m_part)
        m_part->deleteLater();
    else
        deleteLater();
}

bool Terminal::eventFilter(QObject* /* watched */, QEvent* event)
{
    if (event->type() == QEvent::FocusIn)
    {
        emit activated(m_terminalId);

        QFocusEvent* focusEvent = static_cast<QFocusEvent*>(event);

        if (focusEvent->reason() == Qt::MouseFocusReason || focusEvent->reason() == Qt::OtherFocusReason
                || focusEvent->reason() == Qt::BacktabFocusReason)
            emit manuallyActivated(this);
    }
    else if (event->type() == QEvent::MouseMove)
    {
        if (Settings::focusFollowsMouse() && m_terminalWidget && !m_terminalWidget->hasFocus())
            m_terminalWidget->setFocus();
    }

    if (!m_keyboardInputEnabled)
    {
        if (event->type() == QEvent::KeyPress)
        {
            QKeyEvent* keyEvent = static_cast<QKeyEvent*>(event);

            if (keyEvent->modifiers() == Qt::NoModifier)
                emit keyboardInputBlocked(this);

            return true;
        }
        else if (event->type() == QEvent::KeyRelease)
            return true;
    }

    return false;
}

void Terminal::displayKPartLoadError()
{
    KColorScheme colorScheme(QPalette::Active);
    QColor warningColor = colorScheme.background(KColorScheme::NeutralBackground).color();
    QColor warningColorLight = KColorScheme::shade(warningColor, KColorScheme::LightShade, 0.1);
    QString gradient = "qlineargradient(x1:0, y1:0, x2:0, y2:1,"
                       "stop: 0 %1, stop: 0.6 %1 ,stop: 1.0 %2)";
    gradient = gradient.arg(warningColor.name()).arg(warningColorLight.name());
    QString styleSheet = "QLabel { background: %1; }";

    QWidget* widget = new QWidget(m_parentSplitter);
    widget->setStyleSheet(styleSheet.arg(gradient));
    m_partWidget = widget;
    m_terminalWidget = widget;
    m_terminalWidget->setFocusPolicy(Qt::WheelFocus);
    m_terminalWidget->installEventFilter(this);

    QLabel* label = new QLabel(widget);
    label->setMargin(10);
    label->setWordWrap(false);
    label->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    label->setTextInteractionFlags(Qt::TextSelectableByMouse);
    label->setText(i18nc("@info", "<application>Yakuake</application> was unable to load "
                                 "the <application>Konsole</application> component.<nl/> "
                                 "A <application>Konsole</application> installation is "
                                 "required to use Yakuake."));

    QLabel* icon = new QLabel(widget);
    icon->setMargin(10);
    icon->setPixmap(KIcon("dialog-warning").pixmap(QSize(48, 48)));
    icon->setAlignment(Qt::AlignRight | Qt::AlignVCenter);

    QHBoxLayout* layout = new QHBoxLayout(widget);
    layout->addWidget(icon);
    layout->addWidget(label);
    layout->setSpacing(0);
    layout->setMargin(0);
    layout->setStretchFactor(icon, 1);
    layout->setStretchFactor(label,5);
}

void Terminal::disableOffendingPartActions()
{
    // This is an unwelcome stop-gap that will be removed once we can
    // count on a Konsole version that doesn't pollute a KPart user's
    // shortcut "namespace".

    if (!m_part) return;

    KActionCollection* actionCollection = m_part->actionCollection();

    if (actionCollection)
    {
        QAction* action = 0;

        action = actionCollection->action("next-view");
        if (action) action->setEnabled(false);

        action = actionCollection->action("previous-view");
        if (action) action->setEnabled(false);

        action = actionCollection->action("close-active-view");
        if (action) action->setEnabled(false);

        action = actionCollection->action("split-view-left-right");
        if (action) action->setEnabled(false);

        action = actionCollection->action("split-view-top-bottom");
        if (action) action->setEnabled(false);

        action = actionCollection->action("rename-session");
        if (action) action->setEnabled(false);

        action = actionCollection->action("enlarge-font");
        if (action) action->setEnabled(false);

        action = actionCollection->action("shrink-font");
        if (action) action->setEnabled(false);
    }
}

void Terminal::setTitle(const QString& title)
{
    m_title = title;

    emit titleChanged(m_terminalId, m_title);
}

void Terminal::runCommand(const QString& command)
{
    m_terminalInterface->sendInput(command + '\n');
}

void Terminal::manageProfiles()
{
    QMetaObject::invokeMethod(m_part, "showManageProfilesDialog",
        Qt::QueuedConnection, Q_ARG(QWidget*, KApplication::activeWindow()));
}

void Terminal::editProfile()
{
    QMetaObject::invokeMethod(m_part, "showEditCurrentProfileDialog",
        Qt::QueuedConnection, Q_ARG(QWidget*, KApplication::activeWindow()));
}

void Terminal::overrideShortcut(QKeyEvent* /* event */, bool& override)
{
    override = false;
}

void Terminal::setMonitorActivityEnabled(bool enabled)
{
    m_monitorActivityEnabled = enabled;

    if (enabled)
    {
        connect(m_part, SIGNAL(activityDetected()), this, SLOT(activityDetected()),
            Qt::UniqueConnection);

        QMetaObject::invokeMethod(m_part, "setMonitorActivityEnabled",
            Qt::QueuedConnection, Q_ARG(bool, true));
    }
    else
    {
        disconnect(m_part, SIGNAL(activityDetected()), this, SLOT(activityDetected()));

        QMetaObject::invokeMethod(m_part, "setMonitorActivityEnabled",
            Qt::QueuedConnection, Q_ARG(bool, false));
    }
}

void Terminal::setMonitorSilenceEnabled(bool enabled)
{
    m_monitorSilenceEnabled = enabled;

    if (enabled)
    {
        connect(m_part, SIGNAL(silenceDetected()), this, SLOT(silenceDetected()),
            Qt::UniqueConnection);

        QMetaObject::invokeMethod(m_part, "setMonitorSilenceEnabled",
            Qt::QueuedConnection, Q_ARG(bool, true));
    }
    else
    {
        disconnect(m_part, SIGNAL(silenceDetected()), this, SLOT(silenceDetected()));

        QMetaObject::invokeMethod(m_part, "setMonitorSilenceEnabled",
            Qt::QueuedConnection, Q_ARG(bool, false));
    }
}

void Terminal::activityDetected()
{
    emit activityDetected(this);
}

void Terminal::silenceDetected()
{
    emit silenceDetected(this);
}
