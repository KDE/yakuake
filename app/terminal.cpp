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

#include <KApplication>
#include <kde_terminal_interface.h>
#include <KLibLoader>
#include <KUser>

#include <QWidget>


int Terminal::m_availableTerminalId = 0;

Terminal::Terminal(QWidget* parent) : QObject(parent)
{
    m_terminalId = m_availableTerminalId;
    m_availableTerminalId++;

    m_part = NULL;
    m_parentSplitter = parent;

    KPluginFactory* factory = KPluginLoader("libkonsolepart").factory();
    m_part = factory ? (factory->create<KParts::Part>(parent)) : 0;

    if (m_part)
    {
        connect(m_part, SIGNAL(setWindowCaption(const QString&)), this, SLOT(setTitle(const QString&)));
        connect(m_part, SIGNAL(destroyed()), this, SLOT(deleteLater()));

        m_partWidget = m_part->widget();

        m_terminalWidget = m_part->widget()->focusWidget();
        m_terminalWidget->setFocusPolicy(Qt::WheelFocus);
        m_terminalWidget->installEventFilter(this);

        m_terminalInterface = qobject_cast<TerminalInterface*>(m_part);
        m_terminalInterface->showShellInDir(KUser().homeDir());
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
