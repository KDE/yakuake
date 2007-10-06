/*
    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.
*/

/*
  Copyright (C) 2007 Eike Hein <hein@kde.org>
*/


#ifndef SKIN_SETTINGS_H
#define SKIN_SETTINGS_H


#include "skin_settings_ui.h"

#include <kio/job.h>


class SkinSettings : public SkinSettingsUI
{
    Q_OBJECT

    public:
        explicit SkinSettings(QWidget* parent, const char* name=NULL, bool translucency = false);
        ~SkinSettings();


    public slots:
        void slotResetSelection();


    signals:
        void settingsChanged();


    protected:
        void showEvent(QShowEvent* e);


    private:
        void checkForExistingSkin();
        void failInstall(const QString& error);
        void cleanupAfterInstall();

        QString selected;

        QString skins_dir;
        QString install_skin_file;
        QString install_skin_name;
        QStringList install_skin_file_list;


    private slots:
        void slotPopulate();

        void slotInstallSkin();
        void slotListSkinArchive(KIO::Job* job, const KIO::UDSEntryList& list);
        void slotValidateSkinArchive(KIO::Job* job);
        void slotInstallSkinArchive(KIO::Job* delete_job = 0);

        void slotRemoveSkin();

        void slotUpdateRemoveButton();
        void slotUpdateSkinSetting();
        void slotUpdateSelection(const QString&);
};


#endif /* SKIN_SETTINGS_H */
