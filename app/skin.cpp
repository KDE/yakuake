/*
  SPDX-FileCopyrightText: 2008-2014 Eike Hein <hein@kde.org>

  SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#include "skin.h"

#include <KConfig>
#include <KConfigGroup>
#include <KIconLoader>

#include <QFileInfo>
#include <QIcon>

Skin::Skin()
{
    m_borderWidth = 0;
}

Skin::~Skin()
{
}

bool Skin::load(const QString &name, bool kns)
{
    QString dir = kns ? QStringLiteral("kns_skins/") : QStringLiteral("skins/");

    QString titlePath = QStandardPaths::locate(QStandardPaths::AppDataLocation, dir + name + QStringLiteral("/title.skin"));
    QString tabPath = QStandardPaths::locate(QStandardPaths::AppDataLocation, dir + name + QStringLiteral("/tabs.skin"));

    if (!QFile::exists(titlePath) || !QFile::exists(tabPath))
        return false;

    connect(KIconLoader::global(), SIGNAL(iconChanged(int)), this, SLOT(systemIconsChanged(int)), Qt::UniqueConnection);

    QString titleDir(QFileInfo(titlePath).absolutePath());
    QString tabDir(QFileInfo(tabPath).absolutePath());

    KConfig titleConfig(titlePath, KConfig::SimpleConfig);
    KConfig tabConfig(tabPath, KConfig::SimpleConfig);

    KConfigGroup border = titleConfig.group(QStringLiteral("Border"));

    m_borderColor = QColor(border.readEntry("red", 0), border.readEntry("green", 0), border.readEntry("blue", 0));

    m_borderWidth = border.readEntry("width", 1);

    KConfigGroup titleBarBackground = titleConfig.group(QStringLiteral("Background"));

    m_titleBarBackgroundImage.load(titleDir + titleBarBackground.readEntry("back_image", ""));
    m_titleBarLeftCornerImage.load(titleDir + titleBarBackground.readEntry("left_corner", ""));
    m_titleBarRightCornerImage.load(titleDir + titleBarBackground.readEntry("right_corner", ""));

    KConfigGroup titleBarFocusButton = titleConfig.group(QStringLiteral("FocusButton"));

    m_titleBarFocusButtonPosition.setX(titleBarFocusButton.readEntry("x", 0));
    m_titleBarFocusButtonPosition.setY(titleBarFocusButton.readEntry("y", 0));

    m_titleBarFocusButtonStyleSheet = buttonStyleSheet(titleDir + titleBarFocusButton.readEntry("up_image", ""),
                                                       titleDir + titleBarFocusButton.readEntry("over_image", ""),
                                                       titleDir + titleBarFocusButton.readEntry("down_image", ""));

    m_titleBarFocusButtonAnchor = titleBarFocusButton.readEntry("anchor", "") == QLatin1String("left") ? Qt::AnchorLeft : Qt::AnchorRight;

    KConfigGroup titleBarMenuButton = titleConfig.group(QStringLiteral("ConfigButton"));

    m_titleBarMenuButtonPosition.setX(titleBarMenuButton.readEntry("x", 0));
    m_titleBarMenuButtonPosition.setY(titleBarMenuButton.readEntry("y", 0));

    m_titleBarMenuButtonStyleSheet = buttonStyleSheet(titleDir + titleBarMenuButton.readEntry("up_image", ""),
                                                      titleDir + titleBarMenuButton.readEntry("over_image", ""),
                                                      titleDir + titleBarMenuButton.readEntry("down_image", ""));

    m_titleBarMenuButtonAnchor = titleBarMenuButton.readEntry("anchor", "") == QLatin1String("left") ? Qt::AnchorLeft : Qt::AnchorRight;

    KConfigGroup titleBarQuitButton = titleConfig.group(QStringLiteral("QuitButton"));

    m_titleBarQuitButtonPosition.setX(titleBarQuitButton.readEntry("x", 0));
    m_titleBarQuitButtonPosition.setY(titleBarQuitButton.readEntry("y", 0));

    m_titleBarQuitButtonStyleSheet = buttonStyleSheet(titleDir + titleBarQuitButton.readEntry("up_image", ""),
                                                      titleDir + titleBarQuitButton.readEntry("over_image", ""),
                                                      titleDir + titleBarQuitButton.readEntry("down_image", ""));

    m_titleBarQuitButtonAnchor = titleBarQuitButton.readEntry("anchor", "") == QLatin1String("left") ? Qt::AnchorLeft : Qt::AnchorRight;

    KConfigGroup titleBarText = titleConfig.group(QStringLiteral("Text"));

    m_titleBarText = titleBarText.readEntry("text", "");

    m_titleBarTextPosition.setX(titleBarText.readEntry("x", 0));
    m_titleBarTextPosition.setY(titleBarText.readEntry("y", 0));

    m_titleBarTextColor = QColor(titleBarText.readEntry("red", 0), titleBarText.readEntry("green", 0), titleBarText.readEntry("blue", 0));

    m_titleBarTextBold = titleBarText.readEntry("bold", true);
    m_titleBarTextCentered = titleBarText.readEntry("centered", false);

    KConfigGroup tabBar = tabConfig.group(QStringLiteral("Tabs"));

    m_tabBarPosition.setX(tabBar.readEntry("x", 0));
    m_tabBarPosition.setY(tabBar.readEntry("y", 0));

    m_tabBarTextColor = QColor(tabBar.readEntry("red", 0), tabBar.readEntry("green", 0), tabBar.readEntry("blue", 0));

    m_tabBarSeparatorImage.load(tabDir + tabBar.readEntry("separator_image", ""));
    m_tabBarUnselectedBackgroundImage.load(tabDir + tabBar.readEntry("unselected_background", ""));
    m_tabBarSelectedBackgroundImage.load(tabDir + tabBar.readEntry("selected_background", ""));
    m_tabBarUnselectedLeftCornerImage.load(tabDir + tabBar.readEntry("unselected_left_corner", ""));
    m_tabBarUnselectedRightCornerImage.load(tabDir + tabBar.readEntry("unselected_right_corner", ""));
    m_tabBarSelectedLeftCornerImage.load(tabDir + tabBar.readEntry("selected_left_corner", ""));
    m_tabBarSelectedRightCornerImage.load(tabDir + tabBar.readEntry("selected_right_corner", ""));
    m_tabBarSelectedTextBold = tabBar.readEntry("selected_text_bold", true);

    m_tabBarPreventClosingImage.load(tabDir + tabBar.readEntry("prevent_closing_image", ""));
    m_tabBarPreventClosingImagePosition.setX(tabBar.readEntry("prevent_closing_image_x", 0));
    m_tabBarPreventClosingImagePosition.setY(tabBar.readEntry("prevent_closing_image_y", 0));

    m_tabBarCompact = tabBar.readEntry("compact", false);

    KConfigGroup tabBarBackground = tabConfig.group(QStringLiteral("Background"));

    m_tabBarBackgroundImage.load(tabDir + tabBarBackground.readEntry("back_image", ""));
    m_tabBarLeftCornerImage.load(tabDir + tabBarBackground.readEntry("left_corner", ""));
    m_tabBarRightCornerImage.load(tabDir + tabBarBackground.readEntry("right_corner", ""));

    KConfigGroup tabBarNewTabButton = tabConfig.group(QStringLiteral("PlusButton"));

    m_tabBarNewTabButtonPosition.setX(tabBarNewTabButton.readEntry("x", 0));
    m_tabBarNewTabButtonPosition.setY(tabBarNewTabButton.readEntry("y", 0));

    m_tabBarNewTabButtonStyleSheet = buttonStyleSheet(tabDir + tabBarNewTabButton.readEntry("up_image", ""),
                                                      tabDir + tabBarNewTabButton.readEntry("over_image", ""),
                                                      tabDir + tabBarNewTabButton.readEntry("down_image", ""));

    m_tabBarNewTabButtonIsAtEndOfTabs = tabBarNewTabButton.readEntry("at_end_of_tabs", false);

    KConfigGroup tabBarCloseTabButton = tabConfig.group(QStringLiteral("MinusButton"));

    m_tabBarCloseTabButtonPosition.setX(tabBarCloseTabButton.readEntry("x", 0));
    m_tabBarCloseTabButtonPosition.setY(tabBarCloseTabButton.readEntry("y", 0));

    m_tabBarCloseTabButtonStyleSheet = buttonStyleSheet(tabDir + tabBarCloseTabButton.readEntry("up_image", ""),
                                                        tabDir + tabBarCloseTabButton.readEntry("over_image", ""),
                                                        tabDir + tabBarCloseTabButton.readEntry("down_image", ""));

    if (m_tabBarCompact) {
        if (m_tabBarNewTabButtonIsAtEndOfTabs) {
            m_tabBarLeft = m_tabBarPosition.x();
            m_tabBarPosition.setX(0);
        } else {
            if (m_tabBarNewTabButtonPosition.x() < m_tabBarPosition.x())
                m_tabBarLeft = m_tabBarNewTabButtonPosition.x();
            else
                m_tabBarLeft = m_tabBarPosition.x();

            m_tabBarPosition.setX(m_tabBarPosition.x() - m_tabBarLeft);
            m_tabBarNewTabButtonPosition.setX(m_tabBarNewTabButtonPosition.x() - m_tabBarLeft);
        }

        int closeButtonWidth = QPixmap(tabDir + tabBarCloseTabButton.readEntry("up_image", "")).width();
        m_tabBarRight = m_tabBarCloseTabButtonPosition.x() - closeButtonWidth;
        m_tabBarCloseTabButtonPosition.setX(closeButtonWidth);
    }

    if (m_tabBarPreventClosingImage.isNull())
        updateTabBarPreventClosingImageCache();

    return true;
}

const QString Skin::buttonStyleSheet(const QString &up, const QString &over, const QString &down)
{
    QString styleSheet;

    QString borderBit(QStringLiteral("border: none;"));

    QPixmap buttonImage(up);
    QString w(QString::number(buttonImage.width()));
    QString h(QString::number(buttonImage.height()));

    QString sizeBit(QStringLiteral("min-width:") + w + QStringLiteral("; min-height:") + h + QStringLiteral("; max-width:") + w
                    + QStringLiteral("; max-height:") + h + QStringLiteral(";"));

    styleSheet.append(QStringLiteral("QPushButton {") + borderBit + QStringLiteral("image:url(") + up + QStringLiteral(");") + sizeBit + QStringLiteral("}"));
    styleSheet.append(QStringLiteral("QPushButton::hover {") + borderBit + QStringLiteral("image:url(") + over + QStringLiteral(");") + sizeBit
                      + QStringLiteral("}"));
    styleSheet.append(QStringLiteral("QPushButton::pressed {") + borderBit + QStringLiteral("image:url(") + down + QStringLiteral(");") + sizeBit
                      + QStringLiteral("}"));
    styleSheet.append(QStringLiteral("QPushButton::checked {") + borderBit + QStringLiteral("image:url(") + down + QStringLiteral(");") + sizeBit
                      + QStringLiteral("}"));
    styleSheet.append(QStringLiteral("QPushButton::open {") + borderBit + QStringLiteral("image:url(") + down + QStringLiteral(");") + sizeBit
                      + QStringLiteral("}"));
    styleSheet.append(QStringLiteral("QPushButton::menu-indicator { left: ") + w + QStringLiteral(" }"));

    styleSheet.append(QStringLiteral("QToolButton {") + borderBit + QStringLiteral("image:url(") + up + QStringLiteral(");") + sizeBit + QStringLiteral("}"));
    styleSheet.append(QStringLiteral("QToolButton::hover {") + borderBit + QStringLiteral("image:url(") + over + QStringLiteral(");") + sizeBit
                      + QStringLiteral("}"));
    styleSheet.append(QStringLiteral("QToolButton::pressed {") + borderBit + QStringLiteral("image:url(") + down + QStringLiteral(");") + sizeBit
                      + QStringLiteral("}"));
    styleSheet.append(QStringLiteral("QToolButton::checked {") + borderBit + QStringLiteral("image:url(") + down + QStringLiteral(");") + sizeBit
                      + QStringLiteral("}"));
    styleSheet.append(QStringLiteral("QToolButton::open {") + borderBit + QStringLiteral("image:url(") + down + QStringLiteral(");") + sizeBit
                      + QStringLiteral("}"));
    styleSheet.append(QStringLiteral("QToolButton::menu-indicator { left: ") + w + QStringLiteral(" }"));

    return styleSheet;
}

const QPixmap Skin::tabBarPreventClosingImage()
{
    if (m_tabBarPreventClosingImage.isNull())
        return m_tabBarPreventClosingImageCached;

    return m_tabBarPreventClosingImage;
}

void Skin::updateTabBarPreventClosingImageCache()
{
    // Get the target image size from the tabBar height, acquired from
    // background image, minus (2 * y position) of the lock icon.
    int m_IconSize = m_tabBarBackgroundImage.height() - (2 * m_tabBarPreventClosingImagePosition.y());

    // Get the system lock icon in a generous size.
    m_tabBarPreventClosingImageCached = QIcon::fromTheme(QStringLiteral("object-locked.png")).pixmap(48, 48);

    // Resize the image if it's too tall.
    if (m_IconSize < m_tabBarPreventClosingImageCached.height()) {
        m_tabBarPreventClosingImageCached = m_tabBarPreventClosingImageCached.scaled(m_IconSize, m_IconSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    }
}

void Skin::systemIconsChanged(int group)
{
    Q_UNUSED(group);

    if (m_tabBarPreventClosingImage.isNull()) {
        updateTabBarPreventClosingImageCache();

        Q_EMIT iconChanged();
    }
}

#include "moc_skin.cpp"
