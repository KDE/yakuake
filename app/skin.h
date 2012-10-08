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

        bool load(const QString& name, bool kns = false);


        const QColor& borderColor() { return m_borderColor; }
        int borderWidth() { return m_borderWidth; }


        const QPixmap& titleBarBackgroundImage() { return m_titleBarBackgroundImage; }
        const QPixmap& titleBarLeftCornerImage() { return m_titleBarLeftCornerImage; }
        const QPixmap& titleBarRightCornerImage() { return m_titleBarRightCornerImage; }

        const QPoint& titleBarFocusButtonPosition() { return m_titleBarFocusButtonPosition; }
        const QString titleBarFocusButtonStyleSheet() { return m_titleBarFocusButtonStyleSheet; }

        const QPoint& titleBarMenuButtonPosition() { return m_titleBarMenuButtonPosition; }
        const QString titleBarMenuButtonStyleSheet() { return m_titleBarMenuButtonStyleSheet; }

        const QPoint& titleBarQuitButtonPosition() { return m_titleBarQuitButtonPosition; }
        const QString titleBarQuitButtonStyleSheet() { return m_titleBarQuitButtonStyleSheet; }

        const QString titleBarText() { return m_titleBarText; }
        const QPoint& titleBarTextPosition() { return m_titleBarTextPosition; }
        const QColor& titleBarTextColor() { return m_titleBarTextColor; }
              bool    titleBarTextBold() { return m_titleBarTextBold; }


        const QPoint& tabBarPosition() { return m_tabBarPosition; }
        const QColor& tabBarTextColor() { return m_tabBarTextColor; }

        const QPixmap& tabBarSeparatorImage() { return m_tabBarSeparatorImage; }
        const QPixmap& tabBarUnselectedBackgroundImage() { return m_tabBarUnselectedBackgroundImage; }
        const QPixmap& tabBarSelectedBackgroundImage() { return m_tabBarSelectedBackgroundImage; }
        const QPixmap& tabBarSelectedLeftCornerImage() { return m_tabBarSelectedLeftCornerImage; }
        const QPixmap& tabBarSelectedRightCornerImage() { return m_tabBarSelectedRightCornerImage; }

        const QPixmap tabBarPreventClosingImage();
        const QPoint& tabBarPreventClosingImagePosition() { return m_tabBarPreventClosingImagePosition; }

        const QPixmap& tabBarBackgroundImage() { return m_tabBarBackgroundImage; }
        const QPixmap& tabBarLeftCornerImage() { return m_tabBarLeftCornerImage; }
        const QPixmap& tabBarRightCornerImage() { return m_tabBarRightCornerImage; }

        const QPoint& tabBarNewTabButtonPosition() { return m_tabBarNewTabButtonPosition; }
        const QString tabBarNewTabButtonStyleSheet() { return m_tabBarNewTabButtonStyleSheet; }

        const QPoint& tabBarCloseTabButtonPosition() { return m_tabBarCloseTabButtonPosition; }
        const QString tabBarCloseTabButtonStyleSheet() { return m_tabBarCloseTabButtonStyleSheet; }


    signals:
        void iconChanged();


    private slots:
        void systemIconsChanged(int group);


    private:
        const QString buttonStyleSheet(const QString& up, const QString& over, const QString& down);

        void updateTabBarPreventClosingImageCache();

        QColor m_borderColor;
        int m_borderWidth;


        QPixmap m_titleBarBackgroundImage;
        QPixmap m_titleBarLeftCornerImage;
        QPixmap m_titleBarRightCornerImage;

        QPoint m_titleBarFocusButtonPosition;
        QString m_titleBarFocusButtonStyleSheet;

        QPoint m_titleBarMenuButtonPosition;
        QString m_titleBarMenuButtonStyleSheet;

        QPoint m_titleBarQuitButtonPosition;
        QString m_titleBarQuitButtonStyleSheet;

        QString m_titleBarText;
        QPoint m_titleBarTextPosition;
        QColor m_titleBarTextColor;
        bool m_titleBarTextBold;

        QPoint m_tabBarPosition;
        QColor m_tabBarTextColor;

        QPixmap m_tabBarSeparatorImage;
        QPixmap m_tabBarUnselectedBackgroundImage;
        QPixmap m_tabBarSelectedBackgroundImage;
        QPixmap m_tabBarSelectedLeftCornerImage;
        QPixmap m_tabBarSelectedRightCornerImage;

        QPixmap m_tabBarPreventClosingImage;
        QPixmap m_tabBarPreventClosingImageCached;
        QPoint m_tabBarPreventClosingImagePosition;

        QPixmap m_tabBarBackgroundImage;
        QPixmap m_tabBarLeftCornerImage;
        QPixmap m_tabBarRightCornerImage;

        QPoint m_tabBarNewTabButtonPosition;
        QString m_tabBarNewTabButtonStyleSheet;

        QPoint m_tabBarCloseTabButtonPosition;
        QString m_tabBarCloseTabButtonStyleSheet;
};

#endif
