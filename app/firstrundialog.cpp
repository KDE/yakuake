/*
  SPDX-FileCopyrightText: 2008-2009 Eike Hein <hein@kde.org>

  SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
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
    m_ui->titleWidget->setIcon(QIcon::fromTheme(QStringLiteral("yakuake")));

    widget->setMinimumSize(widget->sizeHint());

    initKeyButton();

    connect(m_ui->keyButton, SIGNAL(keySequenceChanged(QKeySequence)), this, SLOT(validateKeySequence(QKeySequence)));
}

FirstRunDialog::~FirstRunDialog()
{
    delete m_ui;
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
        bool steal = KGlobalAccel::promptStealShortcutSystemwide(this, KGlobalAccel::globalShortcutsByKey(keySequence), keySequence);

        if (!steal)
            initKeyButton();
        else {
            KGlobalAccel::stealShortcutSystemwide(keySequence);
            m_keySequence = m_ui->keyButton->keySequence();
        }
    } else
        m_keySequence = m_ui->keyButton->keySequence();
}

#include "moc_firstrundialog.cpp"
