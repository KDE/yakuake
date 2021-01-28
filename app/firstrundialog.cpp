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
  along with this program. If not, see https://www.gnu.org/licenses/.
*/

#include "firstrundialog.h"
#include "mainwindow.h"
#include "ui_firstrundialog.h"

#include <KActionCollection>
#include <KGlobalAccel>
#include <KLocalizedString>
#include <QDialogButtonBox>
#include <QPushButton>

FirstRunDialog::FirstRunDialog(MainWindow *mainWindow)
    : QDialog(mainWindow)
{
    m_mainWindow = mainWindow;

    setWindowTitle(xi18nc("@title:window", "First Run"));
    QVBoxLayout *mainLayout = new QVBoxLayout;
    setLayout(mainLayout);
    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    QPushButton *okButton = buttonBox->button(QDialogButtonBox::Ok);
    okButton->setDefault(true);
    okButton->setShortcut(Qt::CTRL | Qt::Key_Return);
    connect(buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
    connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject()));

    QWidget *widget = new QWidget(this);
    mainLayout->addWidget(widget);
    mainLayout->addWidget(buttonBox);

    m_ui = new Ui::FirstRunDialog();
    m_ui->setupUi(widget);
    m_ui->titleWidget->setPixmap(QIcon::fromTheme(QStringLiteral("yakuake")).pixmap(22, 22));

    widget->setMinimumSize(widget->sizeHint());

    initKeyButton();

    connect(m_ui->keyButton, SIGNAL(keySequenceChanged(QKeySequence)), this, SLOT(validateKeySequence(QKeySequence)));
}

FirstRunDialog::~FirstRunDialog()
{
}

void FirstRunDialog::initKeyButton()
{
    m_ui->keyButton->setMultiKeyShortcutsAllowed(false);

    m_ui->keyButton->blockSignals(true);

    QAction *action = static_cast<QAction *>(m_mainWindow->actionCollection()->action(QStringLiteral("toggle-window-state")));

    m_keySequence = KGlobalAccel::self()->shortcut(action).first();

    m_ui->keyButton->setKeySequence(m_keySequence);

    m_ui->keyButton->blockSignals(false);
}

void FirstRunDialog::validateKeySequence(const QKeySequence &keySequence)
{
    if (!KGlobalAccel::isGlobalShortcutAvailable(keySequence)) {
        bool steal = KGlobalAccel::promptStealShortcutSystemwide(this, KGlobalAccel::getGlobalShortcutsByKey(keySequence), keySequence);

        if (!steal)
            initKeyButton();
        else {
            KGlobalAccel::stealShortcutSystemwide(keySequence);
            m_keySequence = m_ui->keyButton->keySequence();
        }
    } else
        m_keySequence = m_ui->keyButton->keySequence();
}
