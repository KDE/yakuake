/*
  SPDX-FileCopyrightText: 2026 Jakub

  SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#ifndef SESSIONSTORE_H
#define SESSIONSTORE_H

#include <QJsonObject>
#include <QString>

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
};

#endif
