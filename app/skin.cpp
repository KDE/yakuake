/*
  Copyright (C) 2008 by Eike Hein <hein@kde.org>

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


#include "skin.h"

#include <KConfig>
#include <KConfigGroup>
#include <KStandardDirs>

#include <QFileInfo>
#include <QUrl>


Skin::Skin() 
{
}

Skin::~Skin()
{
}

bool Skin::load(const QString& name)
{
    QString titlePath = KStandardDirs::locate("appdata", "skins/" + name + "/title.skin");
    QString tabPath = KStandardDirs::locate("appdata", "skins/" + name + "/tabs.skin");

    if (!QFile::exists(titlePath) || !QFile::exists(tabPath))
        return false;

    QString titleDir(QFileInfo(titlePath).absolutePath());
    QString tabDir(QFileInfo(tabPath).absolutePath());

    KConfig titleConfig(titlePath, KConfig::SimpleConfig);
    KConfig tabConfig(tabPath, KConfig::SimpleConfig);


    KConfigGroup border = titleConfig.group("Border");

    m_borderColor = QColor(border.readEntry("red", 0), 
                           border.readEntry("green", 0), 
                           border.readEntry("blue", 0));

    m_borderWidth = border.readEntry("width", 1);


    KConfigGroup titleBarBackground = titleConfig.group("Background");

    m_titleBarBackgroundImage.load(titleDir + titleBarBackground.readEntry("back_image", ""));
    m_titleBarLeftCornerImage.load(titleDir + titleBarBackground.readEntry("left_corner", ""));
    m_titleBarRightCornerImage.load(titleDir + titleBarBackground.readEntry("right_corner", ""));


    KConfigGroup titleBarFocusButton = titleConfig.group("FocusButton");

    m_titleBarFocusButtonPosition.setX(titleBarFocusButton.readEntry("x", 0));
    m_titleBarFocusButtonPosition.setY(titleBarFocusButton.readEntry("y", 0));  

    m_titleBarFocusButtonStyleSheet = buttonStyleSheet(titleDir + titleBarFocusButton.readEntry("up_image", ""),
                                                       titleDir + titleBarFocusButton.readEntry("over_image", ""),
                                                       titleDir + titleBarFocusButton.readEntry("down_image", ""));


    KConfigGroup titleBarMenuButton = titleConfig.group("ConfigButton");

    m_titleBarMenuButtonPosition.setX(titleBarMenuButton.readEntry("x", 0));
    m_titleBarMenuButtonPosition.setY(titleBarMenuButton.readEntry("y", 0));  

    m_titleBarMenuButtonStyleSheet = buttonStyleSheet(titleDir + titleBarMenuButton.readEntry("up_image", ""),
                                                        titleDir + titleBarMenuButton.readEntry("over_image", ""),
                                                        titleDir + titleBarMenuButton.readEntry("down_image", ""));


    KConfigGroup titleBarQuitButton = titleConfig.group("QuitButton");

    m_titleBarQuitButtonPosition.setX(titleBarQuitButton.readEntry("x", 0));
    m_titleBarQuitButtonPosition.setY(titleBarQuitButton.readEntry("y", 0));  

    m_titleBarQuitButtonStyleSheet = buttonStyleSheet(titleDir + titleBarQuitButton.readEntry("up_image", ""),
                                                      titleDir + titleBarQuitButton.readEntry("over_image", ""),
                                                      titleDir + titleBarQuitButton.readEntry("down_image", ""));


    KConfigGroup titleBarText = titleConfig.group("Text");

    m_titleBarText = titleBarText.readEntry("text", "");

    m_titleBarTextPosition.setX(titleBarText.readEntry("x", 0));
    m_titleBarTextPosition.setY(titleBarText.readEntry("y", 0));  

    m_titleBarTextColor = QColor(titleBarText.readEntry("red", 0), 
                                 titleBarText.readEntry("green", 0), 
                                 titleBarText.readEntry("blue", 0));



    KConfigGroup tabBar = tabConfig.group("Tabs");

    m_tabBarPosition.setX(tabBar.readEntry("x", 0));
    m_tabBarPosition.setY(tabBar.readEntry("y", 0)); 

    m_tabBarTextColor = QColor(tabBar.readEntry("red", 0), 
                               tabBar.readEntry("green", 0), 
                               tabBar.readEntry("blue", 0));

    m_tabBarSeparatorImage.load(tabDir + tabBar.readEntry("separator_image", ""));
    m_tabBarUnselectedBackgroundImage.load(tabDir + tabBar.readEntry("unselected_background", ""));
    m_tabBarSelectedBackgroundImage.load(tabDir + tabBar.readEntry("selected_background", ""));
    m_tabBarSelectedLeftCornerImage.load(tabDir + tabBar.readEntry("selected_left_corner", ""));
    m_tabBarSelectedRightCornerImage.load(tabDir + tabBar.readEntry("selected_right_corner", ""));


    KConfigGroup tabBarBackground = tabConfig.group("Background");

    m_tabBarBackgroundImage.load(tabDir + tabBarBackground.readEntry("back_image", ""));
    m_tabBarLeftCornerImage.load(tabDir + tabBarBackground.readEntry("left_corner", ""));
    m_tabBarRightCornerImage.load(tabDir + tabBarBackground.readEntry("right_corner", ""));


    KConfigGroup tabBarNewTabButton = tabConfig.group("PlusButton");

    m_tabBarNewTabButtonPosition.setX(tabBarNewTabButton.readEntry("x", 0));
    m_tabBarNewTabButtonPosition.setY(tabBarNewTabButton.readEntry("y", 0));  

    m_tabBarNewTabButtonStyleSheet = buttonStyleSheet(tabDir + tabBarNewTabButton.readEntry("up_image", ""),
                                                      tabDir + tabBarNewTabButton.readEntry("over_image", ""),
                                                      tabDir + tabBarNewTabButton.readEntry("down_image", ""));


    KConfigGroup tabBarCloseTabButton = tabConfig.group("MinusButton");

    m_tabBarCloseTabButtonPosition.setX(tabBarCloseTabButton.readEntry("x", 0));
    m_tabBarCloseTabButtonPosition.setY(tabBarCloseTabButton.readEntry("y", 0));  

    m_tabBarCloseTabButtonStyleSheet = buttonStyleSheet(tabDir + tabBarCloseTabButton.readEntry("up_image", ""),
                                                        tabDir + tabBarCloseTabButton.readEntry("over_image", ""),
                                                        tabDir + tabBarCloseTabButton.readEntry("down_image", ""));


    return true;
}

const QString Skin::buttonStyleSheet(const QString& up, const QString& over, const QString& down)
{
    QString styleSheet;

    QString borderBit("border: none;");

    QPixmap buttonImage(up);
    QString w(QString::number(buttonImage.width()));
    QString h(QString::number(buttonImage.height()));

    QString sizeBit("min-width:" + w + "; min-height:" + h + "; max-width:" + w + "; max-height:" + h + ';');

    styleSheet.append("KPushButton {" + borderBit + "image:url(" + up + ");" + sizeBit + '}');
    styleSheet.append("KPushButton::hover {" + borderBit + "image:url(" + over + ");" + sizeBit + '}');
    styleSheet.append("KPushButton::pressed {" + borderBit + "image:url(" + down + ");" + sizeBit + '}');
    styleSheet.append("KPushButton::checked {" + borderBit + "image:url(" + down + ");" + sizeBit + '}');
    styleSheet.append("KPushButton::open {" + borderBit + "image:url(" + down + ");" + sizeBit + '}');
    styleSheet.append("KPushButton::menu-indicator { left: " + w + " }");

    styleSheet.append("QToolButton {" + borderBit + "image:url(" + up + ");" + sizeBit + '}');
    styleSheet.append("QToolButton::hover {" + borderBit + "image:url(" + over + ");" + sizeBit + '}');
    styleSheet.append("QToolButton::pressed {" + borderBit + "image:url(" + down + ");" + sizeBit + '}');
    styleSheet.append("QToolButton::checked {" + borderBit + "image:url(" + down + ");" + sizeBit + '}');
    styleSheet.append("QToolButton::open {" + borderBit + "image:url(" + down + ");" + sizeBit + '}');
    styleSheet.append("QToolButton::menu-indicator { left: " + w + " }");

    return styleSheet;
}
