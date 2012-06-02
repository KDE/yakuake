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


#include "firstrundialog.h"
#include "mainwindow.h"
#include "ui_firstrundialog.h"

#include <KGlobalAccel>
#include <KActionCollection>


FirstRunDialog::FirstRunDialog(MainWindow* mainWindow) : KDialog(mainWindow)
{
    m_mainWindow = mainWindow;

    setCaption(i18nc("@title:window", "First Run"));
    setButtons(KDialog::Ok | KDialog::Cancel);

    QWidget* widget = new QWidget(this);

    m_ui = new Ui::FirstRunDialog();
    m_ui->setupUi(widget);
    m_ui->titleWidget->setPixmap(KIcon("yakuake").pixmap(22, 22));

    widget->layout()->setSpacing(spacingHint());
    widget->setMinimumSize(widget->sizeHint());
    setMainWidget(widget);

    initKeyButton();

    connect(m_ui->keyButton, SIGNAL(keySequenceChanged(QKeySequence)),
        this, SLOT(validateKeySequence(QKeySequence)));
}

FirstRunDialog::~FirstRunDialog()
{
}

void FirstRunDialog::initKeyButton()
{
    m_ui->keyButton->setMultiKeyShortcutsAllowed(false);

    m_ui->keyButton->blockSignals(true);

    KAction* action = static_cast<KAction*>(m_mainWindow->actionCollection()->action("toggle-window-state"));

    m_keySequence = action->globalShortcut().primary();

    m_ui->keyButton->setKeySequence(m_keySequence);

    m_ui->keyButton->blockSignals(false);
}

void FirstRunDialog::validateKeySequence(const QKeySequence& keySequence)
{
    if (!KGlobalAccel::isGlobalShortcutAvailable(keySequence))
    {
        bool steal = KGlobalAccel::promptStealShortcutSystemwide(this,
            KGlobalAccel::getGlobalShortcutsByKey(keySequence), keySequence);

        if (!steal)
            initKeyButton();
        else
        {
            KGlobalAccel::stealShortcutSystemwide(keySequence);
            m_keySequence = m_ui->keyButton->keySequence();
        }
    }
    else
        m_keySequence = m_ui->keyButton->keySequence();
}
