/*
  SPDX-FileCopyrightText: 2026 Jakub

  SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#include "sessionstore.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QRegularExpression>
#include <QSaveFile>
#include <QStandardPaths>

#include <KLocalizedString>

QString SessionStore::sessionsDir()
{
    return QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation) + QStringLiteral("/yakuake/sessions");
}

QString SessionStore::sanitizeName(const QString &name)
{
    QString clean = name.trimmed();

    // The name becomes a file name; strip path separators and other
    // characters that are not portable, and never allow hidden files or
    // relative paths such as "..".
    static const QRegularExpression invalidChars(QStringLiteral("[/\\\\:*?\"<>|]"));
    clean.replace(invalidChars, QStringLiteral("_"));

    while (clean.startsWith(QLatin1Char('.')))
        clean.remove(0, 1);

    if (clean.length() > 100)
        clean.truncate(100);

    return clean.trimmed();
}

QString SessionStore::sessionPath(const QString &name)
{
    const QString cleanName = sanitizeName(name);
    if (cleanName.isEmpty())
        return {};

    return sessionsDir() + QLatin1Char('/') + cleanName + QStringLiteral(".json");
}

QJsonObject SessionStore::read(const QString &filePath, QString *errorMessage)
{
    if (filePath.isEmpty()) {
        if (errorMessage)
            *errorMessage = i18nc("@info", "The session file name is not valid.");
        return {};
    }

    QFile file(filePath);

    if (!file.open(QIODevice::ReadOnly)) {
        if (errorMessage)
            *errorMessage = i18nc("@info", "Could not open the session file %1: %2", filePath, file.errorString());
        return {};
    }

    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(file.readAll(), &parseError);

    if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
        if (errorMessage)
            *errorMessage = i18nc("@info", "The session file %1 is corrupted: %2", filePath, parseError.errorString());
        return {};
    }

    return document.object();
}

bool SessionStore::write(const QString &filePath, const QJsonObject &session, QString *errorMessage)
{
    if (filePath.isEmpty()) {
        if (errorMessage)
            *errorMessage = i18nc("@info", "The session file name is not valid.");
        return false;
    }

    const QString dirPath = QFileInfo(filePath).absolutePath();

    if (!QDir().mkpath(dirPath)) {
        if (errorMessage)
            *errorMessage = i18nc("@info", "Could not create the directory %1.", dirPath);
        return false;
    }

    QSaveFile file(filePath);

    if (!file.open(QIODevice::WriteOnly)) {
        if (errorMessage)
            *errorMessage = i18nc("@info", "Could not write the session file %1: %2", filePath, file.errorString());
        return false;
    }

    file.write(QJsonDocument(session).toJson(QJsonDocument::Indented));

    if (!file.commit()) {
        if (errorMessage)
            *errorMessage = i18nc("@info", "Could not save the session file %1: %2", filePath, file.errorString());
        return false;
    }

    return true;
}

QString SessionStore::firstWorkingDirectory(const QJsonObject &layout)
{
    if (layout.value(QStringLiteral("type")).toString() == QLatin1String("terminal"))
        return layout.value(QStringLiteral("cwd")).toString();

    const QJsonArray children = layout.value(QStringLiteral("children")).toArray();

    for (const QJsonValue &child : children) {
        const QString workingDirectory = firstWorkingDirectory(child.toObject());
        if (!workingDirectory.isEmpty())
            return workingDirectory;
    }

    return {};
}
