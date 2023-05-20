/*
  SPDX-FileCopyrightText: 2008 Eike Hein <hein@kde.org>

  SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#ifndef APPEARANCESETTINGS_H
#define APPEARANCESETTINGS_H

#include "ui_appearancesettings.h"

#include <QTemporaryFile>

#include <KIO/Job>

class SkinListDelegate;

class QStandardItem;
class QStandardItemModel;

class AppearanceSettings : public QWidget, private Ui::AppearanceSettings
{
    Q_OBJECT

public:
    explicit AppearanceSettings(QWidget *parent = nullptr);
    ~AppearanceSettings();

    enum DataRole {
        SkinId = Qt::UserRole + 1,
        SkinDir = Qt::UserRole + 2,
        SkinName = Qt::UserRole + 3,
        SkinAuthor = Qt::UserRole + 4,
        SkinIcon = Qt::UserRole + 5,
        SkinInstalledWithKns = Qt::UserRole + 6,
    };

public Q_SLOTS:
    void resetSelection();

Q_SIGNALS:
    void settingsChanged();

protected:
    void showEvent(QShowEvent *event) Q_DECL_OVERRIDE;

private Q_SLOTS:
    void populateSkinList();

    void updateSkinSetting();

    void installSkin();
    void installSkinArchive();

    /**
     * Validates the given skin.
     * This method checks if the fileList of the skin
     * contains all required files.
     *
     * @param skinId The SkinID of the skin which will be validated.
     * @param kns The skin has been installed from kns.
     *
     * @return True if the skin is valid, otherwise false.
     */
    bool validateSkin(const QString &skinId, bool kns);

    /**
     * Extracts the skin IDs from the given fileList.
     * There can be multiple skins, but only one skin per directory.
     * The files need to be located in the m_knsSkinDir.
     *
     * @param fileList All files which were installed.
     *
     * @return The list of skin IDs which were extracted from the
     *         given fileList.
     */
    QSet<QString> extractKnsSkinIds(const QStringList &fileList);

    void knsDialogFinished(const QList<KNSCore::Entry> &changedEntries);

    void updateRemoveSkinButton();
    void removeSelectedSkin();

private:
    QStandardItem *createSkinItem(const QString &skinDir);

    void populateSkinList(const QString &installationDirectory);
    void checkForExistingSkin();
    void removeSkin(const QString &skinDir, std::function<void()> successCallback = nullptr);
    void installSkin(const QUrl &skinUrl);
    void failInstall(const QString &error);
    void cleanupAfterInstall();

    QString m_selectedSkinId;

    QStandardItemModel *m_skins;
    SkinListDelegate *m_skinListDelegate;

    QString m_localSkinsDir;
    QString m_knsSkinDir;
    QString m_installSkinId;
    QTemporaryFile m_installSkinFile;
    QStringList m_installSkinFileList;

    QString m_knsConfigFileName;
};

#endif
