/*
  SPDX-FileCopyrightText: 2026 Jakub

  SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#ifndef SESSIONSTORE_H
#define SESSIONSTORE_H

#include <QJsonObject>
#include <QList>
#include <QString>
#include <QStringList>

// Reads and writes session files. Named sessions (as used over D-Bus) live in
// ~/.config/yakuake/sessions/, the dialogs can use any path.
class SessionStore
{
public:
    static QString sessionsDir();

    // Turns a user supplied name into a plain file name; returns an empty
    // string when nothing usable is left.
    static QString sanitizeName(const QString &name);

    // Path of the named session file in sessionsDir(); empty when the name is
    // not valid.
    static QString sessionPath(const QString &name);

    static QJsonObject read(const QString &filePath, QString *errorMessage = nullptr);
    static bool write(const QString &filePath, const QJsonObject &session, QString *errorMessage = nullptr);

    // Working directory of the first terminal of a serialized layout, i.e. the
    // one the session must be created with.
    static QString firstWorkingDirectory(const QJsonObject &layout);

    // Programs that were running in the terminals of a serialized layout,
    // one command line per terminal.
    static QList<QStringList> commands(const QJsonObject &layout);

    // Command line stored in a single terminal node; empty when there is none.
    static QStringList command(const QJsonObject &terminalNode);

    // Shell-quoted form of a command line, as it is shown to the user and
    // typed into the terminal.
    static QString commandToString(const QStringList &command);
};

#endif
