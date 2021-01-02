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


#include "windowsettings.h"
#include "settings.h"

#include <KLocalizedString>
#include <KMessageBox>

#include <QDesktopWidget>

WindowSettings::WindowSettings(QWidget* parent) : QWidget(parent)
{
    setupUi(this);

    for (int i = 2; i <= QGuiApplication::screens().count(); i++)
        kcfg_Screen->insertItem(i, xi18nc("@item:inlistbox", "Screen %1", i));

    if (QGuiApplication::screens().count() > 1)
    {
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
    widthSlider->setValue(width/10);

    emit updateWindowGeometry(width, kcfg_Height->value(), kcfg_Position->value());
}

void WindowSettings::updateWidthSpinBox(int width)
{
    kcfg_Width->setValue(width*10);
}

void WindowSettings::updateHeightSlider(int height)
{
    heightSlider->setValue(height/10);

    emit updateWindowGeometry(kcfg_Width->value(), height, kcfg_Position->value());
}

void WindowSettings::updateHeightSpinBox(int height)
{
    kcfg_Height->setValue(height*10);
}

void WindowSettings::updateFramesSlider(int speed)
{
    kcfg_Frames->setValue(speed/10);
}

void WindowSettings::updateFramesSpinBox(int speed)
{
    framesSpinBox->setValue(speed*10);
}

void WindowSettings::updatePosition(int position)
{
    emit updateWindowGeometry(kcfg_Width->value(), kcfg_Height->value(), position);
}

void WindowSettings::interceptHideTitleBar(int state)
{
    if (state == 0) {

        if (Settings::showTitleBar()) { // If the title bar is hidden don't ask if toggling is ok

            const char* message = "You are about to hide the title bar. This will keep you "
                                  "from accessing the settings menu via the mouse. To show "
                                  "the title bar again press the keyboard shortcut (default "
                                  "Ctrl+Shift+m) or access the settings menu via keyborad "
                                  "shortcut (defult: Ctrl+Shift+,).";

            const int result = KMessageBox::warningContinueCancel(
                this,
                xi18nc("@info", message),
                xi18nc("@title:window", "Hiding Title Bar"),
                KStandardGuiItem::cont(),
                KStandardGuiItem::cancel(),
                QStringLiteral("hinding_title_bar")
            );

            if (result == KMessageBox::ButtonCode::Cancel) {
                kcfg_ShowTitleBar->setCheckState(Qt::CheckState::Checked);
            }
        }

    }

}
