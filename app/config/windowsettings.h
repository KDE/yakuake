/*
  SPDX-FileCopyrightText: 2008 Eike Hein <hein@kde.org>

  SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#ifndef WINDOWSETTINGS_H
#define WINDOWSETTINGS_H

#include "ui_windowsettings.h"

class WindowSettings : public QWidget, private Ui::WindowSettings
{
    Q_OBJECT

public:
    explicit WindowSettings(QWidget *parent = nullptr);
    ~WindowSettings();

private:
Q_SIGNALS:
    void updateWindowGeometry(int width, int height, int position);

private Q_SLOTS:
    void updateWidthSlider(int width);
    void updateWidthSpinBox(int width);

    void updateHeightSlider(int height);
    void updateHeightSpinBox(int height);

    void updateFramesSlider(int height);
    void updateFramesSpinBox(int height);

    void updatePosition(int position);

    void interceptHideTitleBar(int state);
};

#endif
