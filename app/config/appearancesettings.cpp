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


#include "appearancesettings.h"
#include "settings.h"
#include "skinlistdelegate.h"

#include <KApplication>
#include <KFileDialog>
#include <KIO/DeleteJob>
#include <KIO/NetAccess>
#include <KMessageBox>
#include <KStandardDirs>
#include <KTar>
#include <KUrl>

#include <QFile>
#include <QStandardItemModel>

#include <unistd.h>

AppearanceSettings::AppearanceSettings(QWidget* parent) : QWidget(parent)
{
    setupUi(this);

    kcfg_Skin->hide();

    m_skins = new QStandardItemModel(this);

    m_skinListDelegate = new SkinListDelegate(this);

    skinList->setModel(m_skins);
    skinList->setItemDelegate(m_skinListDelegate);

    connect(skinList->selectionModel(), SIGNAL(currentChanged(const QModelIndex&, const QModelIndex&)),
        this, SLOT(updateSkinSetting()));
    connect(skinList->selectionModel(), SIGNAL(currentChanged(const QModelIndex&, const QModelIndex&)),
        this, SLOT(updateRemoveSkinButton()));
    connect(installButton, SIGNAL(clicked()), this, SLOT(installSkin()));
    connect(removeButton, SIGNAL(clicked()), this, SLOT(removeSelectedSkin()));

    m_selectedSkinId = Settings::skin();

    m_localSkinsDir = KStandardDirs::locateLocal("data", "yakuake/skins/");

    populateSkinList();
}

AppearanceSettings::~AppearanceSettings()
{
}

void AppearanceSettings::showEvent(QShowEvent* event)
{
    populateSkinList();

    QWidget::showEvent(event);
}

void AppearanceSettings::populateSkinList()
{
    QStringList skinDirs;
    QStringList titleDirs = KGlobal::dirs()->findAllResources("data", "yakuake/skins/*/title.skin");
    QStringList tabDirs = KGlobal::dirs()->findAllResources("data", "yakuake/skins/*/tabs.skin");

    for (int index = 0; index < titleDirs.size(); ++index)
    {
        if (tabDirs.contains(titleDirs.at(index).section('/', 0, -2) + "/tabs.skin"))
            skinDirs << titleDirs.at(index).section('/', 0, -2);
    }

    if (skinDirs.size() > 0)
    {
        m_skins->clear();

        for (int index = 0; index < skinDirs.size(); ++index)
        {
            QString skinId = skinDirs.at(index).section('/', -1, -1);

            int exists = m_skins->match(m_skins->index(0, 0), SkinId, skinId, 
                1, Qt::MatchExactly | Qt::MatchWrap).size();

            if (exists == 0) 
            {
                QStandardItem* skin = createSkinItem(skinDirs.at(index));

                if (!skin) continue;

                m_skins->appendRow(skin);

                if (skin->data(SkinId).toString() == m_selectedSkinId)
                    skinList->setCurrentIndex(skin->index());
            }
        }

        m_skins->sort(0);
    }

    updateRemoveSkinButton();
}

QStandardItem* AppearanceSettings::createSkinItem(const QString& skinDir)
{
    QString skinId = skinDir.section('/', -1, -1);
    QString titleName, tabName, skinName;
    QString titleAuthor, tabAuthor, skinAuthor;
    QString titleIcon, tabIcon;
    QIcon skinIcon;

    KConfig titleConfig(skinDir + "/title.skin", KConfig::SimpleConfig);
    KConfigGroup titleDescription = titleConfig.group("Description");

    KConfig tabConfig(skinDir + "/tabs.skin", KConfig::SimpleConfig);
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
        skinAuthor = i18nc("@item:inlistbox Unknown skin author", "Unknown");

    QStandardItem* skin = new QStandardItem(skinName);

    skin->setData(skinId, SkinId);
    skin->setData(skinDir, SkinDir);
    skin->setData(skinName, SkinName);
    skin->setData(skinAuthor, SkinAuthor);
    skin->setData(skinIcon, SkinIcon);

    return skin;
}

void AppearanceSettings::updateSkinSetting()
{
    QString skinId = skinList->currentIndex().data(SkinId).toString();

    if (!skinId.isEmpty()) 
    {
        m_selectedSkinId = skinId;
        kcfg_Skin->setText(skinId);
    }
}

void AppearanceSettings::resetSelection()
{
    m_selectedSkinId = Settings::skin();

    QModelIndexList skins = m_skins->match(m_skins->index(0, 0), SkinId, 
        Settings::skin(), 1, Qt::MatchExactly | Qt::MatchWrap);

    if (skins.size() > 0) skinList->setCurrentIndex(skins.at(0));
}

void AppearanceSettings::installSkin()
{
    QString mimeFilter = "application/x-tar application/x-compressed-tar "
                         "application/x-bzip-compressed-tar application/zip";

    KUrl skinUrl = KFileDialog::getOpenUrl(KUrl(), mimeFilter, parentWidget());

    if (skinUrl.isEmpty()) return;

    if (!KIO::NetAccess::download(skinUrl, m_installSkinFile, KApplication::activeWindow()))
    {
        KMessageBox::error(parentWidget(), KIO::NetAccess::lastErrorString(),
            i18nc("@title:window", "Failed to Download Skin"));

        return;
    }

    QDir skinDir(m_installSkinFile);

    if (!skinDir.exists())
    {
        KIO::ListJob* job = KIO::listRecursive("tar:" + m_installSkinFile, KIO::HideProgressInfo, false);

        connect(job, SIGNAL(entries(KIO::Job*, const KIO::UDSEntryList&)),
            this, SLOT(listSkinArchive(KIO::Job*, const KIO::UDSEntryList&)));

        connect(job, SIGNAL(result(KJob*)), this, SLOT(validateSkinArchive(KJob*)));
    }
    else
        failInstall(i18nc("@info", "The installer was given a directory, not a file."));
}

void AppearanceSettings::listSkinArchive(KIO::Job* /* job */, const KIO::UDSEntryList& list)
{
    if (list.size() == 0) return;

    for (int entry = 0; entry < list.size(); ++entry) 
    {  
        m_installSkinFileList.append(list.at(entry).stringValue(KIO::UDSEntry::UDS_NAME));
    }
}

void AppearanceSettings::validateSkinArchive(KJob* job)
{
    if (!job->error())
    {
        m_installSkinId = m_installSkinFileList.at(0);

        if (m_installSkinFileList.contains(QString(m_installSkinId + "/title.skin"))
            && m_installSkinFileList.contains(QString(m_installSkinId + "/tabs.skin")))
        {
            checkForExistingSkin();
        }
        else
            failInstall(i18nc("@info", "Unable to locate required files in the skin archive.<nl/><nl/>The archive appears to be invalid."));
    }
    else
        failInstall(i18nc("@info", "Unable to list the skin archive contents.") + "\n\n" + job->errorString());
}

void AppearanceSettings::checkForExistingSkin()
{

    QModelIndexList skins = m_skins->match(m_skins->index(0, 0), SkinId, 
        m_installSkinId, 1, Qt::MatchExactly | Qt::MatchWrap);

    int exists = skins.size();

    if (exists > 0)
    {
        QString skinDir = skins.at(0).data(SkinDir).toString();
        QFile skin(skinDir + "/titles.skin");

        if (!skin.open(QIODevice::ReadWrite))
        {
            failInstall(i18nc("@info", "This skin appears to be already installed and you lack the required permissions to overwrite it."));
        }
        else
        {
            skin.close();

            int remove = KMessageBox::warningContinueCancel(parentWidget(),
                i18nc("@info", "This skin appears to be already installed. Do you want to overwrite it?"),
                i18nc("@title:window", "Skin Already Exists"),
                KGuiItem(i18nc("@action:button", "Reinstall Skin")));

            if (remove == KMessageBox::Continue)
            {
                unlink(QFile::encodeName(skinDir));
                KIO::DeleteJob* job = KIO::del(KUrl(skinDir), KIO::HideProgressInfo);
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
        KMessageBox::error(parentWidget(), deleteJob->errorString(), i18nc("@title:Window", "Could Not Delete Skin"));

        return;
    }

    KTar skinArchive(m_installSkinFile);

    if (skinArchive.open(QIODevice::ReadOnly))
    {
        const KArchiveDirectory* skinDir = skinArchive.directory();
        skinDir->copyTo(m_localSkinsDir);
        skinArchive.close();

        populateSkinList();

        if (Settings::skin() == m_installSkinId) emit settingsChanged();

        cleanupAfterInstall();
    }
    else
        failInstall(i18nc("@info", "The skin archive file could not be opened."));
}

void AppearanceSettings::failInstall(const QString& error)
{
    KMessageBox::error(parentWidget(), error, i18nc("@title:window", "Cannot Install Skin"));

    cleanupAfterInstall();
}

void AppearanceSettings::cleanupAfterInstall()
{
    KIO::NetAccess::removeTempFile(m_installSkinFile);
    m_installSkinId.clear();
    m_installSkinFile.clear();
    m_installSkinFileList.clear();
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
    if (value.isValid()) skinDir = value.toString();

    if (skinDir.isEmpty())
    {
        removeButton->setEnabled(false);
        return;
    }

    QFile titleSkin(skinDir + "/title.skin");

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
        i18nc("@info", "Do you want to remove \"%1\" by %2?", skinName, skinAuthor),
        i18nc("@title:window", "Remove Skin"),
        KStandardGuiItem::del());

    if (remove == KMessageBox::Continue)
    {
        unlink(QFile::encodeName(skinDir));

        bool deleted = KIO::NetAccess::del(KUrl(skinDir), KApplication::activeWindow());

        if (deleted)
        {
            if (skinId == Settings::skin())
            {
                Settings::setSkin("default");
                Settings::self()->writeConfig();
                emit settingsChanged();
            }

            resetSelection();
            populateSkinList();
        }
        else
            KMessageBox::error(parentWidget(), i18nc("@info", "Could not remove skin \"%1\".", skinName));
    }
}
