/*
  SPDX-FileCopyrightText: 2008 Eike Hein <hein@kde.org>

  SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#ifndef SKIN_H
#define SKIN_H

#include <QObject>
#include <QPixmap>
#include <QString>

class Skin : public QObject
{
    Q_OBJECT

public:
    explicit Skin();
    ~Skin();

    bool load(const QString &name, bool kns = false);

    const QColor &borderColor()
    {
        return m_borderColor;
    }
    int borderWidth()
    {
        return m_borderWidth;
    }

    const QPixmap &titleBarBackgroundImage()
    {
        return m_titleBarBackgroundImage;
    }
    const QPixmap &titleBarLeftCornerImage()
    {
        return m_titleBarLeftCornerImage;
    }
    const QPixmap &titleBarRightCornerImage()
    {
        return m_titleBarRightCornerImage;
    }

    const QPoint &titleBarFocusButtonPosition()
    {
        return m_titleBarFocusButtonPosition;
    }
    const Qt::AnchorPoint &titleBarFocusButtonAnchor()
    {
        return m_titleBarFocusButtonAnchor;
    }
    const QString titleBarFocusButtonStyleSheet()
    {
        return m_titleBarFocusButtonStyleSheet;
    }

    const QPoint &titleBarMenuButtonPosition()
    {
        return m_titleBarMenuButtonPosition;
    }
    const Qt::AnchorPoint &titleBarMenuButtonAnchor()
    {
        return m_titleBarMenuButtonAnchor;
    }
    const QString titleBarMenuButtonStyleSheet()
    {
        return m_titleBarMenuButtonStyleSheet;
    }

    const QPoint &titleBarQuitButtonPosition()
    {
        return m_titleBarQuitButtonPosition;
    }
    const Qt::AnchorPoint &titleBarQuitButtonAnchor()
    {
        return m_titleBarQuitButtonAnchor;
    }
    const QString titleBarQuitButtonStyleSheet()
    {
        return m_titleBarQuitButtonStyleSheet;
    }

    const QString titleBarText()
    {
        return m_titleBarText;
    }
    const QPoint &titleBarTextPosition()
    {
        return m_titleBarTextPosition;
    }
    const QColor &titleBarTextColor()
    {
        return m_titleBarTextColor;
    }
    bool titleBarTextBold()
    {
        return m_titleBarTextBold;
    }
    bool titleBarTextCentered()
    {
        return m_titleBarTextCentered;
    }

    const QPoint &tabBarPosition()
    {
        return m_tabBarPosition;
    }
    const QColor &tabBarTextColor()
    {
        return m_tabBarTextColor;
    }

    const QPixmap &tabBarSeparatorImage()
    {
        return m_tabBarSeparatorImage;
    }
    const QPixmap &tabBarUnselectedBackgroundImage()
    {
        return m_tabBarUnselectedBackgroundImage;
    }
    const QPixmap &tabBarSelectedBackgroundImage()
    {
        return m_tabBarSelectedBackgroundImage;
    }
    const QPixmap &tabBarUnselectedLeftCornerImage()
    {
        return m_tabBarUnselectedLeftCornerImage;
    }
    const QPixmap &tabBarUnselectedRightCornerImage()
    {
        return m_tabBarUnselectedRightCornerImage;
    }
    const QPixmap &tabBarSelectedLeftCornerImage()
    {
        return m_tabBarSelectedLeftCornerImage;
    }
    const QPixmap &tabBarSelectedRightCornerImage()
    {
        return m_tabBarSelectedRightCornerImage;
    }
    bool tabBarSelectedTextBold()
    {
        return m_tabBarSelectedTextBold;
    }

    bool tabBarCompact()
    {
        return m_tabBarCompact;
    }
    int tabBarLeft()
    {
        return m_tabBarLeft;
    }
    int tabBarRight()
    {
        return m_tabBarRight;
    }

    const QPixmap tabBarPreventClosingImage();
    const QPoint &tabBarPreventClosingImagePosition()
    {
        return m_tabBarPreventClosingImagePosition;
    }

    const QPixmap &tabBarBackgroundImage()
    {
        return m_tabBarBackgroundImage;
    }
    const QPixmap &tabBarLeftCornerImage()
    {
        return m_tabBarLeftCornerImage;
    }
    const QPixmap &tabBarRightCornerImage()
    {
        return m_tabBarRightCornerImage;
    }

    const QPoint &tabBarNewTabButtonPosition()
    {
        return m_tabBarNewTabButtonPosition;
    }
    const QString tabBarNewTabButtonStyleSheet()
    {
        return m_tabBarNewTabButtonStyleSheet;
    }
    bool tabBarNewTabButtonIsAtEndOfTabs()
    {
        return m_tabBarNewTabButtonIsAtEndOfTabs;
    }

    const QPoint &tabBarCloseTabButtonPosition()
    {
        return m_tabBarCloseTabButtonPosition;
    }
    const QString tabBarCloseTabButtonStyleSheet()
    {
        return m_tabBarCloseTabButtonStyleSheet;
    }

Q_SIGNALS:
    void iconChanged();

private Q_SLOTS:
    void systemIconsChanged(int group);

private:
    const QString buttonStyleSheet(const QString &up, const QString &over, const QString &down);

    void updateTabBarPreventClosingImageCache();

    QColor m_borderColor;
    int m_borderWidth;

    QPixmap m_titleBarBackgroundImage;
    QPixmap m_titleBarLeftCornerImage;
    QPixmap m_titleBarRightCornerImage;

    QPoint m_titleBarFocusButtonPosition;
    Qt::AnchorPoint m_titleBarFocusButtonAnchor;
    QString m_titleBarFocusButtonStyleSheet;

    QPoint m_titleBarMenuButtonPosition;
    Qt::AnchorPoint m_titleBarMenuButtonAnchor;
    QString m_titleBarMenuButtonStyleSheet;

    QPoint m_titleBarQuitButtonPosition;
    Qt::AnchorPoint m_titleBarQuitButtonAnchor;
    QString m_titleBarQuitButtonStyleSheet;

    QString m_titleBarText;
    QPoint m_titleBarTextPosition;
    QColor m_titleBarTextColor;
    bool m_titleBarTextBold;
    bool m_titleBarTextCentered;

    QPoint m_tabBarPosition;
    QColor m_tabBarTextColor;

    QPixmap m_tabBarSeparatorImage;
    QPixmap m_tabBarUnselectedBackgroundImage;
    QPixmap m_tabBarSelectedBackgroundImage;
    QPixmap m_tabBarUnselectedLeftCornerImage;
    QPixmap m_tabBarUnselectedRightCornerImage;
    QPixmap m_tabBarSelectedLeftCornerImage;
    QPixmap m_tabBarSelectedRightCornerImage;
    bool m_tabBarSelectedTextBold;

    QPixmap m_tabBarPreventClosingImage;
    QPixmap m_tabBarPreventClosingImageCached;
    QPoint m_tabBarPreventClosingImagePosition;

    QPixmap m_tabBarBackgroundImage;
    QPixmap m_tabBarLeftCornerImage;
    QPixmap m_tabBarRightCornerImage;

    QPoint m_tabBarNewTabButtonPosition;
    QString m_tabBarNewTabButtonStyleSheet;
    bool m_tabBarNewTabButtonIsAtEndOfTabs;

    bool m_tabBarCompact;
    int m_tabBarLeft;
    int m_tabBarRight;

    QPoint m_tabBarCloseTabButtonPosition;
    QString m_tabBarCloseTabButtonStyleSheet;
};

#endif
