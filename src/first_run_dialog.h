/*
    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.
*/

/*
  Copyright (C) 2007 Eike Hein <hein@kde.org>
*/


#ifndef FIRST_RUN_DIALOG_H
#define FIRST_RUN_DIALOG_H


#include "first_run_dialog_ui.h"


class KShortcut;

class FirstRunDialog : public FirstRunDialogUI
{
    Q_OBJECT

    public:
        explicit FirstRunDialog(QWidget* parent, const char* name=NULL);
        ~FirstRunDialog();

        KShortcut shortcut();


    public slots:
        void setShortcut(const KShortcut& shortcut);


    private slots:
        void validateShortcut(const KShortcut& shortcut);
};


#endif /* FIRST_RUN_DIALOG_H */
