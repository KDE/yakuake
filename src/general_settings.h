/*
    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.
*/

/*
  Copyright (C) 2007 Eike Hein <hein@kde.org>
*/


#ifndef GENERAL_SETTINGS_H
#define GENERAL_SETTINGS_H


#include "general_settings_ui.h"


class GeneralSettings : public GeneralSettingsUI
{
    Q_OBJECT

    public:
        explicit GeneralSettings(QWidget* parent, const char* name=NULL);
        ~GeneralSettings();


    signals:
        void updateSize(int width, int height, int location);


    private slots:
        void updateWidthSlider(int width);
        void updateWidthSpinbox(int width);

        void updateHeightSlider(int height);
        void updateHeightSpinbox(int height);

        void updateStepsSlider(int height);
        void updateStepsSpinbox(int height);

        void updateLocation(int location);
};


#endif /* GENERAL_SETTINGS_H */
