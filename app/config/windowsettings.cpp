/*
  SPDX-FileCopyrightText: 2008-2009 Eike Hein <hein@kde.org>

  SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#include "windowsettings.h"
#include "settings.h"

#include <KLocalizedString>
#include <KMessageBox>

WindowSettings::WindowSettings(QWidget *parent)
    : QWidget(parent)
{
    setupUi(this);

    for (int i = 2; i <= QGuiApplication::screens().count(); i++)
        kcfg_Screen->insertItem(i, xi18nc("@item:inlistbox", "Screen %1", i));

    if (QGuiApplication::screens().count() > 1) {
        screenLabel->setEnabled(true);
        kcfg_Screen->setEnabled(true);
    }

    connect(kcfg_ShowTitleBar, SIGNAL(stateChanged(int)), this, SLOT(interceptHideTitleBar(int)));

    connect(kcfg_Width, SIGNAL(valueChanged(int)), this, SLOT(updateWidthSlider(int)));
    connect(widthSlider, SIGNAL(valueChanged(int)), this, SLOT(updateWidthSpinBox(int)));

    connect(kcfg_Height, SIGNAL(valueChanged(int)), this, SLOT(updateHeightSlider(int)));
    connect(heightSlider, SIGNAL(valueChanged(int)), this, SLOT(updateHeightSpinBox(int)));

    connect(kcfg_Frames, SIGNAL(valueChanged(int)), this, SLOT(updateFramesSpinBox(int)));
    connect(framesSpinBox, SIGNAL(valueChanged(int)), this, SLOT(updateFramesSlider(int)));

    connect(kcfg_Position, SIGNAL(valueChanged(int)), this, SLOT(updatePosition(int)));

    updateFramesSpinBox(Settings::frames() * 10);
}

WindowSettings::~WindowSettings()
{
}

void WindowSettings::updateWidthSlider(int width)
{
    widthSlider->setValue(width / 10);

    Q_EMIT updateWindowGeometry(width, kcfg_Height->value(), kcfg_Position->value());
}

void WindowSettings::updateWidthSpinBox(int width)
{
    kcfg_Width->setValue(width * 10);
}

void WindowSettings::updateHeightSlider(int height)
{
    heightSlider->setValue(height / 10);

    Q_EMIT updateWindowGeometry(kcfg_Width->value(), height, kcfg_Position->value());
}

void WindowSettings::updateHeightSpinBox(int height)
{
    kcfg_Height->setValue(height * 10);
}

void WindowSettings::updateFramesSlider(int speed)
{
    kcfg_Frames->setValue(speed / 10);
}

void WindowSettings::updateFramesSpinBox(int speed)
{
    framesSpinBox->setValue(speed * 10);
}

void WindowSettings::updatePosition(int position)
{
    Q_EMIT updateWindowGeometry(kcfg_Width->value(), kcfg_Height->value(), position);
}

void WindowSettings::interceptHideTitleBar(int state)
{
    if (state == 0) {
        if (Settings::showTitleBar()) { // If the title bar is hidden don't ask if toggling is ok

            const char *message =
                "You are about to hide the title bar. This will keep you "
                "from accessing the settings menu via the mouse. To show "
                "the title bar again press the keyboard shortcut (default "
                "Ctrl+Shift+m) or access the settings menu via keyboard "
                "shortcut (default: Ctrl+Shift+,).";

            const int result = KMessageBox::warningContinueCancel(this,
                                                                  xi18nc("@info", message),
                                                                  xi18nc("@title:window", "Hiding Title Bar"),
                                                                  KStandardGuiItem::cont(),
                                                                  KStandardGuiItem::cancel(),
                                                                  QStringLiteral("hinding_title_bar"));

            if (result == KMessageBox::ButtonCode::Cancel) {
                kcfg_ShowTitleBar->setCheckState(Qt::CheckState::Checked);
            }
        }
    }
}

#include "moc_windowsettings.cpp"
