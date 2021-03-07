/*
  SPDX-FileCopyrightText: 2008 Eike Hein <hein@kde.org>

  SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#ifndef FIRSTRUNDIALOG_H
#define FIRSTRUNDIALOG_H

#include <QDialog>

class MainWindow;
class Ui_FirstRunDialog;

class FirstRunDialog : public QDialog
{
    Q_OBJECT

public:
    explicit FirstRunDialog(MainWindow *mainWindow);
    ~FirstRunDialog();

    QKeySequence keySequence()
    {
        return m_keySequence;
    }

private Q_SLOTS:
    void validateKeySequence(const QKeySequence &keySequence);

private:
    void initKeyButton();

    Ui_FirstRunDialog *m_ui;
    MainWindow *m_mainWindow;

    QKeySequence m_keySequence;
};

#endif
