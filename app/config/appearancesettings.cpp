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


#include "appearancesettings.h"
#include "settings.h"
#include "skinlistdelegate.h"

#include <KIO/CopyJob>
#include <KIO/DeleteJob>
#include <KJobUiDelegate>
#include <KLocalizedString>
#include <KMessageBox>
#include <KTar>

#include <KNS3/DownloadDialog>
#include <KNS3/DownloadManager>

#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileDialog>
#include <QPointer>
#include <QStandardItemModel>

#include <unistd.h>

AppearanceSettings::AppearanceSettings(QWidget* parent) : QWidget(parent)
{
    setupUi(this);

    kcfg_Skin->hide();
    kcfg_SkinInstalledWithKns->hide();

    m_skins = new QStandardItemModel(this);

    m_skinListDelegate = new SkinListDelegate(this);

    skinList->setModel(m_skins);
    skinList->setItemDelegate(m_skinListDelegate);

    connect(skinList->selectionModel(), SIGNAL(currentChanged(QModelIndex,QModelIndex)),
        this, SLOT(updateSkinSetting()));
    connect(skinList->selectionModel(), SIGNAL(currentChanged(QModelIndex,QModelIndex)),
        this, SLOT(updateRemoveSkinButton()));
    connect(installButton, SIGNAL(clicked()), this, SLOT(installSkin()));
    connect(removeButton, SIGNAL(clicked()), this, SLOT(removeSelectedSkin()));

    installButton->setIcon(QIcon(QStringLiteral("folder")));
    removeButton->setIcon(QIcon(QStringLiteral("edit-delete")));
    ghnsButton->setIcon(QIcon(QStringLiteral("get-hot-new-stuff")));

    m_knsConfigFileName = QLatin1String("yakuake.knsrc");
    //m_knsDownloadManager = new KNS3::DownloadManager(m_knsConfigFileName);

    connect(ghnsButton, &QPushButton::clicked, this, &AppearanceSettings::getNewSkins);

    m_selectedSkinId = Settings::skin();

    // Get all local skin directories.
    // One for manually installed skins, one for skins installed
    // through KNS3.
    m_localSkinsDir = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + QStringLiteral("/yakuake/skins/");
    m_knsSkinDir = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + QStringLiteral("/yakuake/kns_skins/");

    populateSkinList();
}

AppearanceSettings::~AppearanceSettings()
{
}

void AppearanceSettings::showEvent(QShowEvent* event)
{
    populateSkinList();

    if (skinList->currentIndex().isValid())
        skinList->scrollTo(skinList->currentIndex());

    QWidget::showEvent(event);
}

void AppearanceSettings::populateSkinList()
{
    m_skins->clear();

    QStringList allSkinLocations;
    allSkinLocations << QStandardPaths::locateAll(QStandardPaths::GenericDataLocation, QStringLiteral("/yakuake/skins/"), QStandardPaths::LocateDirectory);
    allSkinLocations << QStandardPaths::locateAll(QStandardPaths::GenericDataLocation, QStringLiteral("/yakuake/kns_skins/"), QStandardPaths::LocateDirectory);

    Q_FOREACH (const QString& skinLocation, allSkinLocations)
    {
        populateSkinList(skinLocation);
    }

    m_skins->sort(0);

    updateRemoveSkinButton();
}

void AppearanceSettings::populateSkinList(const QString& installLocation)
{
    QDirIterator it(installLocation, QDir::Dirs | QDir::NoDotAndDotDot);

    while (it.hasNext())
    {
        const QDir& skinDir(it.next());

        if (skinDir.exists(QStringLiteral("title.skin")) && skinDir.exists(QStringLiteral("tabs.skin")))
        {
            QStandardItem* skin = createSkinItem(skinDir.absolutePath());

            if (!skin)
                continue;

            m_skins->appendRow(skin);

            if (skin->data(SkinId).toString() == m_selectedSkinId)
                skinList->setCurrentIndex(skin->index());
        }
    }
}

QStandardItem* AppearanceSettings::createSkinItem(const QString& skinDir)
{
    QString skinId = skinDir.section(QStringLiteral("/"), -1, -1);
    QString titleName, tabName, skinName;
    QString titleAuthor, tabAuthor, skinAuthor;
    QString titleIcon, tabIcon;
    QIcon skinIcon;

    // Check if the skin dir starts with the path where all
    // KNS3 skins are found in.
    bool isKnsSkin = skinDir.startsWith(m_knsSkinDir);

    KConfig titleConfig(skinDir + QStringLiteral("/title.skin"), KConfig::SimpleConfig);
    KConfigGroup titleDescription = titleConfig.group("Description");

    KConfig tabConfig(skinDir + QStringLiteral("/tabs.skin"), KConfig::SimpleConfig);
    KConfigGroup tabDescription = tabConfig.group("Description");

    titleName = titleDescription.readEntry("Skin", "");
    titleAuthor = titleDescription.readEntry("Author", "");
    titleIcon = skinDir + titleDescription.readEntry("Icon", "");

    tabName = tabDescription.readEntry("Skin", "");
    tabAuthor = tabDescription.readEntry("Author", "");
    tabIcon = skinDir + tabDescription.readEntry("Icon", "");

    skinName = titleName.isEmpty() ? tabName : titleName;
    skinAuthor = titleAuthor.isEmpty() ? tabAuthor : titleAuthor;
    titleIcon.isEmpty() ? skinIcon.addPixmap(tabIcon) : skinIcon.addPixmap(titleIcon);

    if (skinName.isEmpty() || skinAuthor.isEmpty())
        skinName = skinId;

    if (skinAuthor.isEmpty())
        skinAuthor = xi18nc("@item:inlistbox Unknown skin author", "Unknown");

    QStandardItem* skin = new QStandardItem(skinName);

    skin->setData(skinId, SkinId);
    skin->setData(skinDir, SkinDir);
    skin->setData(skinName, SkinName);
    skin->setData(skinAuthor, SkinAuthor);
    skin->setData(skinIcon, SkinIcon);
    skin->setData(isKnsSkin, SkinInstalledWithKns);

    return skin;
}

void AppearanceSettings::updateSkinSetting()
{
    QString skinId = skinList->currentIndex().data(SkinId).toString();

    if (!skinId.isEmpty())
    {
        m_selectedSkinId = skinId;
        kcfg_Skin->setText(skinId);
        kcfg_SkinInstalledWithKns->setChecked(skinList->currentIndex().data(SkinInstalledWithKns).toBool());
    }
}

void AppearanceSettings::resetSelection()
{
    m_selectedSkinId = Settings::skin();

    QModelIndexList skins = m_skins->match(m_skins->index(0, 0), SkinId,
        Settings::skin(), 1, Qt::MatchExactly | Qt::MatchWrap);

    if (skins.count() > 0) skinList->setCurrentIndex(skins.at(0));
}

void AppearanceSettings::installSkin()
{
    QStringList mimeTypes;
    mimeTypes << QStringLiteral("application/x-compressed-tar");
    mimeTypes << QStringLiteral("application/x-xz-compressed-tar");
    mimeTypes << QStringLiteral("application/x-bzip-compressed-tar");
    mimeTypes << QStringLiteral("application/zip");
    mimeTypes << QStringLiteral("application/x-tar");

    QFileDialog fileDialog(parentWidget());
    fileDialog.setWindowTitle(i18nc("@title:window", "Select the skin archive to install"));
    fileDialog.setMimeTypeFilters(mimeTypes);
    fileDialog.setFileMode(QFileDialog::ExistingFile);

    QUrl skinUrl;
    if (fileDialog.exec() && !fileDialog.selectedUrls().isEmpty())
        skinUrl = fileDialog.selectedUrls().at(0);
    else
        return;

    m_installSkinFile.open();

    KIO::CopyJob *job = KIO::copy(skinUrl, QUrl::fromLocalFile(m_installSkinFile.fileName()), KIO::JobFlag::HideProgressInfo | KIO::JobFlag::Overwrite);
    connect(job, &KIO::CopyJob::result, [=](KJob *job) {
        if (job->error())
        {
            job->uiDelegate()->showErrorMessage();
            job->kill();

            cleanupAfterInstall();

            return;
        }

        installSkin(static_cast<KIO::CopyJob*>(job)->destUrl());
    });
}

void AppearanceSettings::installSkin(const QUrl& skinUrl)
{
    QUrl skinArchiveUrl = QUrl(skinUrl);
    skinArchiveUrl.setScheme(QStringLiteral("tar"));

    KIO::ListJob* job = KIO::listRecursive(skinArchiveUrl, KIO::HideProgressInfo, false);
    connect(job, &KIO::ListJob::entries, [=](KIO::Job* /* job */, const KIO::UDSEntryList& list) {
        if (list.count() == 0) return;

        QListIterator<KIO::UDSEntry> i(list);
        while (i.hasNext())
            m_installSkinFileList.append(i.next().stringValue(KIO::UDSEntry::UDS_NAME));
    });

    connect(job, &KIO::ListJob::result, [=](KJob *job) {
        if (!job->error())
        {
            m_installSkinId = m_installSkinFileList.at(0);

            if (validateSkin(m_installSkinId, m_installSkinFileList))
                checkForExistingSkin();
            else
                failInstall(xi18nc("@info", "Unable to locate required files in the skin archive.<nl/><nl/>The archive appears to be invalid."));
        }
        else
            failInstall(xi18nc("@info", "Unable to list the skin archive contents.") + QStringLiteral("\n\n") + job->errorString());
    });
}

bool AppearanceSettings::validateSkin(const QString &skinId, const QStringList& fileList)
{
    bool titleFileFound = false;
    bool tabsFileFound = false;
    QString titleFileName = skinId + QStringLiteral("/title.skin");
    QString tabsFileName = skinId + QStringLiteral("/tabs.skin");

    foreach (const QString& fileName, fileList)
    {
        if (fileName.endsWith(titleFileName))
        {
            titleFileFound = true;
        }
        else if (fileName.endsWith(tabsFileName))
        {
            tabsFileFound = true;
        }
    }

    return titleFileFound && tabsFileFound;
}

void AppearanceSettings::checkForExistingSkin()
{
    QModelIndexList skins = m_skins->match(m_skins->index(0, 0), SkinId,
        m_installSkinId, 1, Qt::MatchExactly | Qt::MatchWrap);

    int exists = skins.count();

    foreach (const QModelIndex& skin, skins)
    {
        if (m_skins->item(skin.row())->data(SkinInstalledWithKns).toBool())
            --exists;
    }

    if (exists > 0)
    {
        QString skinDir = skins.at(0).data(SkinDir).toString();
        QFile skin(skinDir + QStringLiteral("titles.skin"));

        if (!skin.open(QIODevice::ReadWrite))
        {
            failInstall(xi18nc("@info", "This skin appears to be already installed and you lack the required permissions to overwrite it."));
        }
        else
        {
            skin.close();

            int remove = KMessageBox::warningContinueCancel(parentWidget(),
                xi18nc("@info", "This skin appears to be already installed. Do you want to overwrite it?"),
                xi18nc("@title:window", "Skin Already Exists"),
                KGuiItem(xi18nc("@action:button", "Reinstall Skin")));

            if (remove == KMessageBox::Continue)
            {
                KIO::DeleteJob* job = KIO::del(QUrl::fromLocalFile(skinDir), KIO::HideProgressInfo);
                connect(job, SIGNAL(result(KJob*)), this, SLOT(installSkinArchive(KJob*)));
            }
            else
                cleanupAfterInstall();
        }
    }
    else
        installSkinArchive();
}

void AppearanceSettings::installSkinArchive(KJob* deleteJob)
{
    if (deleteJob && deleteJob->error())
    {
        KMessageBox::error(parentWidget(), deleteJob->errorString(), xi18nc("@title:Window", "Could Not Delete Skin"));
        return;
    }

    KTar skinArchive(m_installSkinFile.fileName());

    if (skinArchive.open(QIODevice::ReadOnly))
    {
        const KArchiveDirectory* skinDir = skinArchive.directory();
        skinDir->copyTo(m_localSkinsDir);
        skinArchive.close();

        populateSkinList();

        if (Settings::skin() == m_installSkinId)
            emit settingsChanged();

        cleanupAfterInstall();
    }
    else
        failInstall(xi18nc("@info", "The skin archive file could not be opened."));
}

void AppearanceSettings::failInstall(const QString& error)
{
    KMessageBox::error(parentWidget(), error, xi18nc("@title:window", "Cannot Install Skin"));

    cleanupAfterInstall();
}

void AppearanceSettings::cleanupAfterInstall()
{
    m_installSkinId.clear();
    m_installSkinFileList.clear();

    if (m_installSkinFile.exists())
    {
        m_installSkinFile.close();
        m_installSkinFile.remove();
    }
}

void AppearanceSettings::updateRemoveSkinButton()
{
    if (m_skins->rowCount() <= 1)
    {
        removeButton->setEnabled(false);
        return;
    }

    QString skinDir;

    QVariant value = skinList->currentIndex().data(SkinDir);
    if (value.isValid())
        skinDir = value.toString();

    value = skinList->currentIndex().data(SkinInstalledWithKns);
    bool isKnsSkin = value.toBool();

    // We don't allow the user to remove the default skin
    // or any skin which was installed through KNS3.
    if (skinDir.isEmpty() || isKnsSkin)
    {
        removeButton->setEnabled(false);
        return;
    }

    QFile titleSkin(skinDir + QStringLiteral("/title.skin"));

    if (!titleSkin.open(QIODevice::ReadWrite))
        removeButton->setEnabled(false);
    else
        removeButton->setEnabled(true);

    titleSkin.close();
}

void AppearanceSettings::removeSelectedSkin()
{
    if (m_skins->rowCount() <= 1) return;

    QString skinId = skinList->currentIndex().data(SkinId).toString();
    QString skinDir = skinList->currentIndex().data(SkinDir).toString();
    QString skinName = skinList->currentIndex().data(SkinName).toString();
    QString skinAuthor = skinList->currentIndex().data(SkinAuthor).toString();

    if (skinDir.isEmpty()) return;

    int remove = KMessageBox::warningContinueCancel(parentWidget(),
            xi18nc("@info", "Do you want to remove \"%1\" by %2?", skinName, skinAuthor),
            xi18nc("@title:window", "Remove Skin"),
            KStandardGuiItem::del());

    if (remove == KMessageBox::Continue)
    {
        KIO::DeleteJob* job = KIO::del(QUrl::fromLocalFile(skinDir), KIO::HideProgressInfo);
        connect(job, &KIO::DeleteJob::result, [=](KJob* deleteJob) {
            if (deleteJob->error())
            {
                KMessageBox::error(parentWidget(), deleteJob->errorString(), xi18nc("@title:Window", "Could Not Delete Skin"));
            } else {
                if (skinId == Settings::skin())
                {
                    Settings::setSkin(QStringLiteral("default"));
                    Settings::setSkinInstalledWithKns(false);
                    Settings::self()->save();
                    emit settingsChanged();
                }

                resetSelection();
                populateSkinList();
            }
        });
    }
}

QSet<QString> AppearanceSettings::extractKnsSkinIds(const QStringList& fileList)
{
    QSet<QString> skinIdList;

    foreach (const QString& file, fileList)
    {
        // We only care about files/directories which are subdirectories of our KNS skins dir.
        if (file.startsWith(m_knsSkinDir, Qt::CaseInsensitive))
        {
            // Get the relative filename (this removes the KNS install dir from the filename).
            QString relativeName = QString(file).remove(m_knsSkinDir, Qt::CaseInsensitive);

            // Get everything before the first slash - that should be our skins ID.
            QString skinId = relativeName.section(QStringLiteral("/"), 0, QString::SectionSkipEmpty);

            // Skip all other entries in the file list if we found what we were searching for.
            if (!skinId.isEmpty())
            {
                // First remove all remaining slashes (as there could be leading or trailing ones).
                skinId = skinId.replace(QStringLiteral("/"), QString());

                skinIdList.insert(skinId);
            }
        }
    }

    return skinIdList;
}

void AppearanceSettings::getNewSkins()
{
    QPointer<KNS3::DownloadDialog> dialog = new KNS3::DownloadDialog(m_knsConfigFileName, this);

    // Show the KNS dialog. NOTE: We are NOT supposed to check the dialog's result,
    // because the actions are asynchronous (items are installed or uninstalled,
    // regardless whether the dialog's result is Accepted or Rejected)!
    dialog->exec();

    if (dialog.isNull())
    {
        return;
    }

    if (!dialog->installedEntries().empty())
    {
        quint32 invalidEntryCount = 0;
        QString invalidSkinText;

        foreach (const KNS3::Entry &entry, dialog->installedEntries())
        {
            bool isValid = true;
            const QSet<QString>& skinIdList = extractKnsSkinIds(entry.installedFiles());

            // Validate all skin IDs as each archive can contain multiple skins.
            foreach (const QString& skinId, skinIdList)
            {
                // Validate the current skin.
                if (!validateSkin(skinId, entry.installedFiles()))
                {
                    isValid = false;
                }
            }

            // We'll add an error message for the whole KNS entry if
            // the current skin is marked as invalid.
            // We should not do this per skin as the user does not know that
            // there are more skins inside one archive.
            if (!isValid)
            {
                invalidEntryCount++;

                // The user needs to know the name of the skin which
                // was removed.
                invalidSkinText += QString(QStringLiteral("<li>%1</li>")).arg(entry.name());

                // Then remove the skin.
                m_knsDownloadManager->uninstallEntry(entry);
            }
        }

        // Are there any invalid entries?
        if (invalidEntryCount > 0)
        {
            failInstall(xi18ncp("@info",
                               "The following skin is missing required files. Thus it was removed:<ul>%2</ul>",
                               "The following skins are missing required files. Thus they were removed:<ul>%2</ul>",
                               invalidEntryCount,
                               invalidSkinText));
        }
    }

    if (!dialog->changedEntries().isEmpty())
    {
        // Reset the selection in case the currently selected
        // skin was removed.
        resetSelection();

        // Re-populate the list of skins if the user changed something.
        populateSkinList();
    }

    delete dialog;
}
