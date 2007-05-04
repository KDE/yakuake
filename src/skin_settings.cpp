/*
    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.
*/

/*
  Copyright (C) 2007 Eike Hein <hein@kde.org>
*/


#include "skin_settings.h"
#include "skin_settings.moc"
#include "skin_list_item.h"
#include "settings.h"

#include <qurl.h>
#include <qheader.h>
#include <qpixmap.h>
#include <qpushbutton.h>
#include <qfile.h>

#include <kapplication.h>
#include <kglobal.h>
#include <kstandarddirs.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <kurl.h>
#include <kio/global.h>
#include <kio/job.h>
#include <kio/netaccess.h>
#include <kfiledialog.h>
#include <ktar.h>
#include <klineedit.h>

#include <unistd.h> // unlink()


SkinSettings::SkinSettings(QWidget* parent, const char* name)
 : SkinSettingsUI(parent, name)
{
    kcfg_skin->hide();

    skins_list->header()->hide();
    skins_list->setSelectionModeExt(KListView::Single);
    skins_list->setResizeMode(KListView::LastColumn);
    skins_list->setRootIsDecorated(false);
    skins_list->setDragEnabled(false);
    skins_list->setAcceptDrops(false);
    skins_list->setDropVisualizer(false);
    skins_list->setSortColumn(0);

    connect(skins_list, SIGNAL(selectionChanged()), this, SLOT(slotUpdateRemoveButton()));
    connect(skins_list, SIGNAL(selectionChanged()), this, SLOT(slotUpdateSkinSetting()));
    connect(kcfg_skin, SIGNAL(textChanged(const QString&)), this, SLOT(slotUpdateSelection(const QString&)));
    connect(install_button, SIGNAL(clicked()), this, SLOT(slotInstallSkin()));
    connect(remove_button, SIGNAL(clicked()), this, SLOT(slotRemoveSkin()));

    skins_dir = locateLocal("data", "yakuake/");

    selected = Settings::skin();

    slotPopulate();
}

SkinSettings::~SkinSettings()
{
}

void SkinSettings::showEvent(QShowEvent* e)
{
    slotPopulate();
    SkinSettingsUI::showEvent(e);
}

void SkinSettings::slotPopulate()
{
    QStringList skins_dirs;
    QStringList titles_dirs = KGlobal::dirs()->findAllResources("data","yakuake/*/title.skin");
    QStringList tabs_dirs = KGlobal::dirs()->findAllResources("data","yakuake/*/tabs.skin");

    for (QStringList::Iterator it = titles_dirs.begin(); it != titles_dirs.end(); ++it)
    {
        if (tabs_dirs.contains((*it).section('/', 0, -2) + "/tabs.skin"))
            skins_dirs << (*it).section('/', 0, -2);
    }

    if (skins_dirs.count() > 0)
    {
        skins_list->clear();

        for (QStringList::Iterator it = skins_dirs.begin(); it != skins_dirs.end(); ++it)
        {
            QUrl titles_url = locate("appdata", (*it) + "/title.skin");
            KConfig titles_config(titles_url.path());
            titles_config.setGroup("Description");

            QString titles_name(titles_config.readEntry("Skin", ""));
            QString titles_author(titles_config.readEntry("Author", ""));
            QString titles_icon_name(titles_config.readEntry("Icon", ""));

            QUrl tabs_url = locate("appdata", (*it) + "/tabs.skin");
            KConfig tabs_config(tabs_url.path());
            tabs_config.setGroup("Description");

            QString tabs_name(tabs_config.readEntry("Skin", ""));
            QString tabs_author(tabs_config.readEntry("Author", ""));
            QString tabs_icon_name(tabs_config.readEntry("Icon", ""));

            QString skin_name = (*it).section('/', -1, -1);
            QString skin_fancy_name = i18n("Unnamed");
            QString skin_author = i18n("Unknown");
            QString skin_icon_name;
            QUrl skin_icon_url;
            QPixmap skin_icon;

            if (!titles_name.isEmpty())
                skin_fancy_name = titles_name;
            else if (!tabs_name.isEmpty())
                skin_fancy_name = tabs_name;

            if (!titles_author.isEmpty())
                skin_author = titles_author;
            else if (!tabs_author.isEmpty())
                skin_author = tabs_author;

            if (!titles_icon_name.isEmpty())
                skin_icon_name = titles_icon_name;
            else if (!tabs_icon_name.isEmpty())
                skin_icon_name = tabs_icon_name;

            skin_icon_url = locate("appdata", (*it) + skin_icon_name);

            if (skin_icon_url.isValid())
                skin_icon.load(skin_icon_url.path());

            SkinListItem* skin = new SkinListItem(skins_list, skin_fancy_name, skin_author, skin_icon, skin_name, (*it));

            if (skin_name == selected) skins_list->setSelected(skin, true);
        }
    }

    slotUpdateRemoveButton();
}

void SkinSettings::slotInstallSkin()
{
    KURL skin_url = KFileDialog::getOpenURL(QString(),
        i18n("*.tar.gz *.tar.bz2 *.tar *.zip|Yakuake Skins"),
        NULL, i18n("Select Skin Archive"));

    if (skin_url.isEmpty()) return;

    if (!KIO::NetAccess::download(skin_url, install_skin_file, NULL))
    {
        KMessageBox::error(0L,
            KIO::NetAccess::lastErrorString(),
            i18n("Failed to Download Skin"),
            KMessageBox::Notify
            );

        return;
    }

    QDir skin_dir(install_skin_file);

    if (!skin_dir.exists())
    {
        KIO::ListJob* job = KIO::listRecursive("tar:" + install_skin_file, false, false);

        connect(job, SIGNAL(entries(KIO::Job*, const KIO::UDSEntryList&)),
            this, SLOT(slotListSkinArchive(KIO::Job*, const KIO::UDSEntryList&)));

        connect(job, SIGNAL(result(KIO::Job*)),
            this, SLOT(slotValidateSkinArchive(KIO::Job*)));
    }
    else
        failInstall(i18n("The installer was given a directory, not a file."));
}

void SkinSettings::slotListSkinArchive(KIO::Job* /* job */, const KIO::UDSEntryList& list)
{
    KIO::UDSEntryList::const_iterator it = list.begin();

    for(; it != list.end(); ++it)
    {
        KIO::UDSEntry::const_iterator itUSDEntry = (*it).begin();

        for (; itUSDEntry != (*it).end(); ++itUSDEntry )
        {
            if((*itUSDEntry).m_uds == KIO::UDS_NAME)
            {
                install_skin_file_list.append((*itUSDEntry).m_str);
            }
        }
    }
}

void SkinSettings::slotValidateSkinArchive(KIO::Job* job)
{
    if (!job->error())
    {
        install_skin_name = install_skin_file_list.first();

        if (install_skin_file_list.contains(QString(install_skin_name + "/title.skin"))
            && install_skin_file_list.contains(QString(install_skin_name + "/tabs.skin")))
        {
            checkForExistingSkin();
        }
        else
            failInstall(i18n("Unable to locate required files in the skin archive.\n\n The archive appears to be invalid."));
    }
    else
        failInstall(i18n("Unable to list the skin archive contents.") + QString("\n\n %1").arg(job->errorString()));
}

void SkinSettings::checkForExistingSkin()
{
    bool exists = false;
    SkinListItem* item = 0;

    QListViewItemIterator it(skins_list);

    while (it.current())
    {
        item = static_cast<SkinListItem*>(it.current());
        if (item && item->name() == install_skin_name) exists = true;
        ++it;
    }

    if (exists)
    {
        QFile skin(item->dir() + "/titles.skin");

        if (!skin.open(IO_ReadOnly | IO_WriteOnly))
        {
            failInstall(i18n("This skin appears to be already installed and you lack the required permissions to overwrite it."));
        }
        else
        {
            skin.close();

            int remove = KMessageBox::warningContinueCancel(0L,
            i18n("This skin appears to be already installed. Do you want to overwrite it?"),
            i18n("Skin Already Exists"),
            i18n("Reinstall Skin"));

            if (remove == KMessageBox::Continue)
            {
                unlink(QFile::encodeName(item->dir()));
                KIO::DeleteJob* job = KIO::del(KURL(item->dir()), false, false);
                connect(job, SIGNAL(result(KIO::Job*)), this, SLOT(slotInstallSkinArchive(KIO::Job*)));
            }
            else
                cleanupAfterInstall();
        }
    }
    else
        slotInstallSkinArchive();
}

void SkinSettings::slotInstallSkinArchive(KIO::Job* delete_job)
{
    if (delete_job && delete_job->error())
    {
        KMessageBox::error(0L,
            delete_job->errorString(),
            i18n("Could Not Delete Skin"),
            KMessageBox::Notify
            );

        return;
    }

    KTar skin_archive(install_skin_file);

    if (skin_archive.open(IO_ReadOnly))
    {
        const KArchiveDirectory* skin_dir = skin_archive.directory();
        skin_dir->copyTo(skins_dir);
        skin_archive.close();

        slotPopulate();

        if (Settings::skin() == install_skin_name)
            emit settingsChanged();

        cleanupAfterInstall();
    }
    else
        failInstall(i18n("The skin archive file could not be opened."));
}

void SkinSettings::failInstall(const QString& error)
{
    KMessageBox::error(0L, error,
        i18n("Cannot Install Skin"),
        KMessageBox::Notify
        );

    cleanupAfterInstall();
}

void SkinSettings::cleanupAfterInstall()
{
    KIO::NetAccess::removeTempFile(install_skin_file);
    install_skin_file = QString();
    install_skin_name = QString();
    install_skin_file_list.clear();
}

void SkinSettings::slotRemoveSkin()
{
    if (skins_list->childCount() <= 1)
        return;

    SkinListItem* selected_item = static_cast<SkinListItem*>(skins_list->selectedItem());

    if (!selected_item) return;

    int remove = KMessageBox::warningContinueCancel(0L,
        i18n("Do you want to remove \"%1\" by %2?").arg(selected_item->text(0)).arg(selected_item->author()),
        i18n("Remove Skin"),
        KStdGuiItem::del());

    if (remove == KMessageBox::Continue)
    {
        unlink(QFile::encodeName(selected_item->dir()));
        KIO::DeleteJob* job = KIO::del(KURL(selected_item->dir()), false, false);
        connect(job, SIGNAL(result(KIO::Job *)), this, SLOT(slotPopulate()));

        if (selected_item->name() == Settings::skin())
        {
            Settings::setSkin("default");
            Settings::writeConfig();
            emit settingsChanged();
        }

        slotResetSelection();
    }
}

void SkinSettings::slotUpdateRemoveButton()
{
    if (skins_list->childCount() <= 1)
    {
        remove_button->setEnabled(false);
        return;
    }

    SkinListItem* selected_item = static_cast<SkinListItem*>(skins_list->selectedItem());

    if (!selected_item) return;

    if (selected_item->name() == "default")
    {
        remove_button->setEnabled(false);
        return;
    }

    QFile skin(selected_item->dir() + "/title.skin");

    if (!skin.open(IO_ReadOnly | IO_WriteOnly))
        remove_button->setEnabled(false);
    else
        remove_button->setEnabled(true);

    skin.close();
}

void SkinSettings::slotUpdateSkinSetting()
{
    SkinListItem* selected_item = static_cast<SkinListItem*>(skins_list->selectedItem());

    if (selected_item)
    {
        selected = selected_item->name();
        kcfg_skin->setText(selected_item->name());
    }
}

void SkinSettings::slotResetSelection()
{
    selected = Settings::skin();
}

void SkinSettings::slotUpdateSelection(const QString& selection)
{
    selected = selection;
    SkinListItem* skin = 0;

    QListViewItemIterator it(skins_list);

    while (it.current())
    {
        skin = static_cast<SkinListItem*>(it.current());

        if (skin && skin->name() == selected)
            skins_list->setSelected(skin, true);

        ++it;
    }
}
