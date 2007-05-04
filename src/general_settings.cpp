/*
    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.
*/

/*
  Copyright (C) 2007 Eike Hein <hein@kde.org>
*/


#include "general_settings.h"
#include "general_settings.moc"

#include <qapplication.h>
#include <qslider.h>
#include <qspinbox.h>
#include <qcombobox.h>
#include <qlabel.h>

#include <knuminput.h>
#include <klocale.h>


GeneralSettings::GeneralSettings(QWidget* parent, const char* name)
 : GeneralSettingsUI(parent, name)
{
    for (int i = 2; i <= QApplication::desktop()->numScreens(); i++)
        kcfg_screen->insertItem(i18n("Screen %1").arg(QString::number(i)));

    if (QApplication::desktop()->numScreens() > 1)
    {
        screen_label->setEnabled(true);
        kcfg_screen->setEnabled(true);
    }

    connect(kcfg_width, SIGNAL(valueChanged(int)), this, SLOT(updateWidthSlider(int)));
    connect(width_slider, SIGNAL(valueChanged(int)), this, SLOT(updateWidthSpinbox(int)));

    connect(kcfg_height, SIGNAL(valueChanged(int)), this, SLOT(updateHeightSlider(int)));
    connect(height_slider, SIGNAL(valueChanged(int)), this, SLOT(updateHeightSpinbox(int)));

    connect(kcfg_steps, SIGNAL(valueChanged(int)), this, SLOT(updateStepsSpinbox(int)));
    connect(steps_spinbox, SIGNAL(valueChanged(int)), this, SLOT(updateStepsSlider(int)));

    connect(kcfg_location, SIGNAL(valueChanged(int)), this, SLOT(updateLocation(int)));
}

GeneralSettings::~GeneralSettings()
{
}

void GeneralSettings::updateWidthSlider(int width)
{
    width_slider->setValue(width/10);

    emit updateSize(width, kcfg_height->value(), kcfg_location->value());
}

void GeneralSettings::updateWidthSpinbox(int width)
{
    kcfg_width->setValue(width*10);
}

void GeneralSettings::updateHeightSlider(int height)
{
    height_slider->setValue(height/10);

    emit updateSize(kcfg_width->value(), height, kcfg_location->value());
}

void GeneralSettings::updateHeightSpinbox(int height)
{
    kcfg_height->setValue(height*10);
}

void GeneralSettings::updateStepsSlider(int speed)
{
    kcfg_steps->setValue(speed/10);
}

void GeneralSettings::updateStepsSpinbox(int speed)
{
    steps_spinbox->setValue(speed*10);
}

void GeneralSettings::updateLocation(int location)
{
    emit updateSize(kcfg_width->value(), kcfg_height->value(), location);
}
