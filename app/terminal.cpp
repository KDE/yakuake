/*
  Copyright (C) 2008 by Eike Hein <hein@kde.org>

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


#include <terminal.h>

#include <KActionCollection>
#include <KApplication>
#include <kde_terminal_interface.h>
#include <KLibLoader>
#include <KLocalizedString>
#include <KMessageBox>
#include <KUser>

#include <QAction>
#include <QWidget>
#include <QKeyEvent>


int Terminal::m_availableTerminalId = 0;

Terminal::Terminal(QWidget* parent) : QObject(parent)
{
    m_terminalId = m_availableTerminalId;
    m_availableTerminalId++;

    m_part = NULL;
    m_terminalInterface = NULL;
    m_partWidget = NULL;
    m_terminalWidget = NULL;
    m_parentSplitter = parent;

    KPluginFactory* factory = KPluginLoader("libkonsolepart").factory();
    m_part = factory ? (factory->create<KParts::Part>(parent)) : 0;

    if (m_part)
    {
        connect(m_part, SIGNAL(setWindowCaption(const QString&)), this, SLOT(setTitle(const QString&)));
        connect(m_part, SIGNAL(overrideShortcut(QKeyEvent*, bool&)), this, SLOT(overrideShortcut(QKeyEvent*, bool&)));
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
    {
        KMessageBox::error(KApplication::activeWindow(), 
            i18nc("@info", "<application>Yakuake</application> was unable to load the <application>Konsole</application> component. "
                           "A <application>Konsole</application> installation is required to run Yakuake.<nl/><nl/>"
                           "The application will now quit."));

        QMetaObject::invokeMethod(kapp, "quit", Qt::QueuedConnection);
    }
}

Terminal::~Terminal()
{
    emit destroyed(m_terminalId);
}

void Terminal::deletePart()
{
    if (m_part) m_part->deleteLater();
}

bool Terminal::eventFilter(QObject* /* watched */, QEvent* event)
{
    if (event->type() == QEvent::FocusIn) emit activated (m_terminalId);

    return false;
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
