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


#ifndef APPEARANCESETTINGS_H
#define APPEARANCESETTINGS_H


#include "ui_appearancesettings.h"

#include <KIO/Job>


class SkinListDelegate;

class QStandardItem;
class QStandardItemModel;

namespace KNS3
{
    class DownloadManager;
}


class AppearanceSettings : public QWidget, private Ui::AppearanceSettings
{
    Q_OBJECT

    public:
        explicit AppearanceSettings(QWidget* parent = 0);
         ~AppearanceSettings();

        enum DataRole
        {
            SkinId = Qt::UserRole + 1,
            SkinDir = Qt::UserRole + 2,
            SkinName = Qt::UserRole + 3,
            SkinAuthor = Qt::UserRole + 4,
            SkinIcon = Qt::UserRole + 5,
            SkinInstalledWithKns = Qt::UserRole + 6
        };


    public slots:
        void resetSelection();


    signals:
        void settingsChanged();


    protected:
        virtual void showEvent(QShowEvent* event);


    private slots:
        void populateSkinList();

        void updateSkinSetting();

        void installSkin();
        void listSkinArchive(KIO::Job* job, const KIO::UDSEntryList& list);
        void validateSkinArchive(KJob* job);
        void installSkinArchive(KJob* deleteJob = 0);

        /**
         * Validates the given skin.
         * This method checks if the fileList of the skin
         * contains all required files.
         *
         * @param skinId The SkinID of the skin which will be validated.
         * @param fileList The filelist of the skin.
         *
         * @return True if the skin is valid, otherwise false.
         */
        bool validateSkin(const QString &skinId, const QStringList& fileList);

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
        QSet<QString> extractKnsSkinIds(const QStringList& fileList);

        /**
         * Shows the KNS3 dialog where the user can download new skins.
         * @note Only works from KDE platform 4.7.0 onwards.
         *       The signature needs to be compiled always, because
         *       otherwise the SLOT cannot be found during runtime.
         *       The code for this method is disabled inside the method
         *       body.
         */
        void getNewSkins();

        void updateRemoveSkinButton();
        void removeSelectedSkin();


    private:
        QStandardItem* createSkinItem(const QString& skinDir);

        void checkForExistingSkin();
        void failInstall(const QString& error);
        void cleanupAfterInstall();

        QString m_selectedSkinId;

        QStandardItemModel* m_skins;
        SkinListDelegate* m_skinListDelegate;

        QString m_localSkinsDir;
        QString m_knsSkinDir;
        QString m_installSkinId;
        QString m_installSkinFile;
        QStringList m_installSkinFileList;

        QString m_knsConfigFileName;
        KNS3::DownloadManager* m_knsDownloadManager;
};

#endif
