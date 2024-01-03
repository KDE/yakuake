/*
  SPDX-FileCopyrightText: 2008-2009 Eike Hein <hein@kde.org>

  SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#include "appearancesettings.h"
#include "settings.h"
#include "skinlistdelegate.h"

#include <KIO/CopyJob>
#include <KIO/DeleteJob>
#include <KIO/ListJob>
#include <KJobUiDelegate>
#include <KLocalizedString>
#include <KMessageBox>
#include <KTar>

#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileDialog>
#include <QPointer>
#include <QStandardItemModel>

#include <unistd.h>

AppearanceSettings::AppearanceSettings(QWidget *parent)
    : QWidget(parent)
{
    setupUi(this);

    kcfg_Skin->hide();
    kcfg_SkinInstalledWithKns->hide();

    m_skins = new QStandardItemModel(this);

    m_skinListDelegate = new SkinListDelegate(this);

    skinList->setModel(m_skins);
    skinList->setItemDelegate(m_skinListDelegate);

    connect(skinList->selectionModel(), &QItemSelectionModel::currentChanged, this, &AppearanceSettings::updateSkinSetting);
    connect(skinList->selectionModel(), &QItemSelectionModel::currentChanged, this, &AppearanceSettings::updateRemoveSkinButton);
    connect(installButton, SIGNAL(clicked()), this, SLOT(installSkin()));
    connect(removeButton, &QAbstractButton::clicked, this, &AppearanceSettings::removeSelectedSkin);

    installButton->setIcon(QIcon::fromTheme(QStringLiteral("folder")));
    removeButton->setIcon(QIcon::fromTheme(QStringLiteral("edit-delete")));
    ghnsButton->setConfigFile(QStringLiteral("yakuake.knsrc"));

    connect(ghnsButton, &KNSWidgets::Button::dialogFinished, this, &AppearanceSettings::knsDialogFinished);

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

void AppearanceSettings::showEvent(QShowEvent *event)
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

    for (const QString &skinLocation : std::as_const(allSkinLocations)) {
        populateSkinList(skinLocation);
    }

    m_skins->sort(0);

    updateRemoveSkinButton();
}

void AppearanceSettings::populateSkinList(const QString &installLocation)
{
    QDirIterator it(installLocation, QDir::Dirs | QDir::NoDotAndDotDot);

    while (it.hasNext()) {
        const QDir &skinDir(it.next());

        if (skinDir.exists(QStringLiteral("title.skin")) && skinDir.exists(QStringLiteral("tabs.skin"))) {
            QStandardItem *skin = createSkinItem(skinDir.absolutePath());

            if (!skin)
                continue;

            m_skins->appendRow(skin);

            if (skin->data(SkinId).toString() == m_selectedSkinId)
                skinList->setCurrentIndex(skin->index());
        }
    }
}

QStandardItem *AppearanceSettings::createSkinItem(const QString &skinDir)
{
    QString skinId = skinDir.section(QLatin1Char('/'), -1, -1);
    QString titleName, tabName, skinName;
    QString titleAuthor, tabAuthor, skinAuthor;
    QString titleIcon, tabIcon;
    QIcon skinIcon;

    // Check if the skin dir starts with the path where all
    // KNS3 skins are found in.
    bool isKnsSkin = skinDir.startsWith(m_knsSkinDir);

    KConfig titleConfig(skinDir + QStringLiteral("/title.skin"), KConfig::SimpleConfig);
    KConfigGroup titleDescription = titleConfig.group(QStringLiteral("Description"));

    KConfig tabConfig(skinDir + QStringLiteral("/tabs.skin"), KConfig::SimpleConfig);
    KConfigGroup tabDescription = tabConfig.group(QStringLiteral("Description"));

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

    QStandardItem *skin = new QStandardItem(skinName);

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

    if (!skinId.isEmpty()) {
        m_selectedSkinId = skinId;
        kcfg_Skin->setText(skinId);
        kcfg_SkinInstalledWithKns->setChecked(skinList->currentIndex().data(SkinInstalledWithKns).toBool());
    }
}

void AppearanceSettings::resetSelection()
{
    m_selectedSkinId = Settings::skin();

    QModelIndexList skins = m_skins->match(m_skins->index(0, 0), SkinId, Settings::skin(), 1, Qt::MatchExactly | Qt::MatchWrap);

    if (skins.count() > 0)
        skinList->setCurrentIndex(skins.at(0));
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
    connect(job, &KIO::CopyJob::result, [=, this](KJob *job) {
        if (job->error()) {
            job->uiDelegate()->showErrorMessage();
            job->kill();

            cleanupAfterInstall();

            return;
        }

        installSkin(static_cast<KIO::CopyJob *>(job)->destUrl());
    });
}

void AppearanceSettings::installSkin(const QUrl &skinUrl)
{
    QUrl skinArchiveUrl = QUrl(skinUrl);
    skinArchiveUrl.setScheme(QStringLiteral("tar"));

    KIO::ListJob *job = KIO::listRecursive(skinArchiveUrl, KIO::HideProgressInfo, KIO::ListJob::ListFlags{});
    connect(job, &KIO::ListJob::entries, [=, this](KIO::Job * /* job */, const KIO::UDSEntryList &list) {
        if (list.isEmpty())
            return;

        QListIterator<KIO::UDSEntry> i(list);
        while (i.hasNext())
            m_installSkinFileList.append(i.next().stringValue(KIO::UDSEntry::UDS_NAME));
    });

    connect(job, &KIO::ListJob::result, this, [=, this](KJob *job) {
        if (!job->error())
            checkForExistingSkin();
        else
            failInstall(xi18nc("@info", "Unable to list the skin archive contents.") + QStringLiteral("\n\n") + job->errorString());
    });
}

bool AppearanceSettings::validateSkin(const QString &skinId, bool kns)
{
    QString dir = kns ? QStringLiteral("kns_skins/") : QStringLiteral("skins/");

    QString titlePath = QStandardPaths::locate(QStandardPaths::AppDataLocation, dir + skinId + QStringLiteral("/title.skin"));
    QString tabsPath = QStandardPaths::locate(QStandardPaths::AppDataLocation, dir + skinId + QStringLiteral("/tabs.skin"));

    return !titlePath.isEmpty() && !tabsPath.isEmpty();
}

void AppearanceSettings::checkForExistingSkin()
{
    m_installSkinId = m_installSkinFileList.at(0);

    const QModelIndexList skins = m_skins->match(m_skins->index(0, 0), SkinId, m_installSkinId, 1, Qt::MatchExactly | Qt::MatchWrap);

    int exists = skins.count();

    for (const QModelIndex &skin : skins) {
        if (m_skins->item(skin.row())->data(SkinInstalledWithKns).toBool())
            --exists;
    }

    if (exists > 0) {
        QString skinDir = skins.at(0).data(SkinDir).toString();
        QFile skin(skinDir + QStringLiteral("titles.skin"));

        if (!skin.open(QIODevice::ReadWrite)) {
            failInstall(xi18nc("@info", "This skin appears to be already installed and you lack the required permissions to overwrite it."));
        } else {
            skin.close();

            int remove = KMessageBox::warningContinueCancel(parentWidget(),
                                                            xi18nc("@info", "This skin appears to be already installed. Do you want to overwrite it?"),
                                                            xi18nc("@title:window", "Skin Already Exists"),
                                                            KGuiItem(xi18nc("@action:button", "Reinstall Skin")));

            if (remove == KMessageBox::Continue)
                removeSkin(skinDir, [this]() {
                    installSkinArchive();
                });
            else
                cleanupAfterInstall();
        }
    } else
        installSkinArchive();
}

void AppearanceSettings::removeSkin(const QString &skinDir, std::function<void()> successCallback)
{
    KIO::DeleteJob *job = KIO::del(QUrl::fromLocalFile(skinDir), KIO::HideProgressInfo);
    connect(job, &KIO::DeleteJob::result, this, [=, this](KJob *deleteJob) {
        if (deleteJob->error()) {
            KMessageBox::error(parentWidget(), deleteJob->errorString(), xi18nc("@title:Window", "Could Not Delete Skin"));
        } else if (successCallback) {
            successCallback();
        }
    });
}

void AppearanceSettings::installSkinArchive()
{
    KTar skinArchive(m_installSkinFile.fileName());

    if (skinArchive.open(QIODevice::ReadOnly)) {
        const KArchiveDirectory *skinDir = skinArchive.directory();
        skinDir->copyTo(m_localSkinsDir);
        skinArchive.close();

        if (validateSkin(m_installSkinId, false)) {
            populateSkinList();

            if (Settings::skin() == m_installSkinId)
                Q_EMIT settingsChanged();

            cleanupAfterInstall();
        } else {
            removeSkin(m_localSkinsDir + m_installSkinId);
            failInstall(xi18nc("@info", "Unable to locate required files in the skin archive.<nl/><nl/>The archive appears to be invalid."));
        }
    } else
        failInstall(xi18nc("@info", "The skin archive file could not be opened."));
}

void AppearanceSettings::failInstall(const QString &error)
{
    KMessageBox::error(parentWidget(), error, xi18nc("@title:window", "Cannot Install Skin"));

    cleanupAfterInstall();
}

void AppearanceSettings::cleanupAfterInstall()
{
    m_installSkinId.clear();
    m_installSkinFileList.clear();

    if (m_installSkinFile.exists()) {
        m_installSkinFile.close();
        m_installSkinFile.remove();
    }
}

void AppearanceSettings::updateRemoveSkinButton()
{
    if (m_skins->rowCount() <= 1) {
        removeButton->setEnabled(false);
        return;
    }

    const QString skinDir = skinList->currentIndex().data(SkinDir).toString();
    bool enabled = false;
    if (!skinDir.isEmpty()) {
        enabled = QFileInfo(skinDir + QStringLiteral("/title.skin")).isWritable();
    }
    removeButton->setEnabled(enabled);
}

void AppearanceSettings::removeSelectedSkin()
{
    if (m_skins->rowCount() <= 1)
        return;

    QString skinDir = skinList->currentIndex().data(SkinDir).toString();
    QString skinName = skinList->currentIndex().data(SkinName).toString();
    QString skinAuthor = skinList->currentIndex().data(SkinAuthor).toString();

    if (skinDir.isEmpty())
        return;

    int remove = KMessageBox::warningContinueCancel(parentWidget(),
                                                    xi18nc("@info", "Do you want to remove \"%1\" by %2?", skinName, skinAuthor),
                                                    xi18nc("@title:window", "Remove Skin"),
                                                    KStandardGuiItem::del());

    if (remove == KMessageBox::Continue)
        removeSkin(skinDir, [=, this]() {
            QString skinId = skinList->currentIndex().data(SkinId).toString();
            if (skinId == Settings::skin()) {
                Settings::setSkin(QStringLiteral("default"));
                Settings::setSkinInstalledWithKns(false);
                Settings::self()->save();
                Q_EMIT settingsChanged();
            }

            resetSelection();
            populateSkinList();
        });
}

QSet<QString> AppearanceSettings::extractKnsSkinIds(const QStringList &fileList)
{
    QSet<QString> skinIdList;

    for (const QString &file : fileList) {
        // We only care about files/directories which are subdirectories of our KNS skins dir.
        if (file.startsWith(m_knsSkinDir, Qt::CaseInsensitive)) {
            // Get the relative filename (this removes the KNS install dir from the filename).
            QString relativeName = QString(file).remove(m_knsSkinDir, Qt::CaseInsensitive);

            // Get everything before the first slash - that should be our skins ID.
            QString skinId = relativeName.section(QLatin1Char('/'), 0, 0, QString::SectionSkipEmpty);

            // Skip all other entries in the file list if we found what we were searching for.
            if (!skinId.isEmpty()) {
                // First remove all remaining slashes (as there could be leading or trailing ones).
                skinId.remove(QStringLiteral("/"));

                skinIdList.insert(skinId);
            }
        }
    }

    return skinIdList;
}

void AppearanceSettings::knsDialogFinished(const QList<KNSCore::Entry> &changedEntries)
{
    quint32 invalidEntryCount = 0;
    QString invalidSkinText;
    for (const auto &entry : changedEntries) {
        auto Installed = KNSCore::Entry::Installed;
        if (entry.status() != Installed) {
            continue;
        }
        bool isValid = true;
        const QSet<QString> &skinIdList = extractKnsSkinIds(entry.installedFiles());

        // Validate all skin IDs as each archive can contain multiple skins.
        for (const QString &skinId : skinIdList) {
            // Validate the current skin.
            if (!validateSkin(skinId, true)) {
                isValid = false;
            }
        }

        // We'll add an error message for the whole KNS entry if
        // the current skin is marked as invalid.
        // We should not do this per skin as the user does not know that
        // there are more skins inside one archive.
        if (!isValid) {
            invalidEntryCount++;

            // The user needs to know the name of the skin which
            // was removed.
            invalidSkinText += QString(QStringLiteral("<li>%1</li>")).arg(entry.name());

            // Then remove the skin.
            const QStringList files = entry.installedFiles();
            for (const QString &file : files) {
                QFileInfo info(QString(file).remove(QStringLiteral("/*")));
                if (!info.exists()) {
                    continue;
                }
                if (info.isDir()) {
                    QDir(info.absoluteFilePath()).removeRecursively();
                } else {
                    QFile::remove(info.absoluteFilePath());
                }
            }
        }
    }

    // Are there any invalid entries?
    if (invalidEntryCount > 0) {
        failInstall(xi18ncp("@info",
                            "The following skin is missing required files. Thus it was removed:<ul>%2</ul>",
                            "The following skins are missing required files. Thus they were removed:<ul>%2</ul>",
                            invalidEntryCount,
                            invalidSkinText));
    }

    if (!changedEntries.isEmpty()) {
        // Reset the selection in case the currently selected
        // skin was removed.
        resetSelection();

        // Re-populate the list of skins if the user changed something.
        populateSkinList();
    }
}

#include "moc_appearancesettings.cpp"
