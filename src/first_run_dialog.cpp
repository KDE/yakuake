/*
    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.
*/

/*
  Copyright (C) 2007 Eike Hein <hein@kde.org>
*/


#include "first_run_dialog.h"
#include "first_run_dialog.moc"

#include <kkeybutton.h>
#include <kshortcut.h>
#include <kkeydialog.h>


FirstRunDialog::FirstRunDialog(QWidget* parent, const char* name)
 : FirstRunDialogUI(parent, name)
{
    connect(key_button, SIGNAL(capturedShortcut(const KShortcut&)),
        this, SLOT(validateShortcut(const KShortcut&)));
}

FirstRunDialog::~FirstRunDialog()
{
}

KShortcut FirstRunDialog::shortcut()
{
    return key_button->shortcut();
}

void FirstRunDialog::setShortcut(const KShortcut& shortcut)
{
    key_button->setShortcut(shortcut, false);
}

void FirstRunDialog::validateShortcut(const KShortcut& shortcut)
{
    if (!KKeyChooser::checkGlobalShortcutsConflict(shortcut, true, this)
        && !KKeyChooser::KKeyChooser::checkStandardShortcutsConflict(shortcut, true, this))
    {
        key_button->setShortcut(shortcut, false);
    }
}
