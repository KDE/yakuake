/*
    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.
*/

/*
  Copyright (C) 2005 Francois Chazal <neptune3k@free.fr>
  Copyright (C) 2006-2007 Eike Hein <hein@kde.org>
*/


#ifndef TAB_BAR_H
#define TAB_BAR_H


#include "image_button.h"
#include "tabbed_widget.h"
#include "translucent_widget.h"

#include <qurl.h>
#include <qcolor.h>
#include <qpoint.h>
#include <qpixmap.h>
#include <qpainter.h>

#include <kconfig.h>
#include <krootpixmap.h>
#include <kstandarddirs.h>


class TabBar : public TranslucentWidget
{
    Q_OBJECT

    public:
        explicit TabBar(QWidget* parent = 0, const char* name = 0,
                        bool translucency = false, const QString& skin = "default");
        ~TabBar();

        void setSessionMenu(KPopupMenu* menu);

        int pressedPosition();
        void resetPressedPosition();

        void addItem(int session_id);
        int removeItem(int session_id);

        const QString itemName(int session_id);
        void renameItem(int session_id, const QString& name);
        void interactiveRename();

        int tabPositionForSessionId(int session_id);
        int sessionIdForTabPosition(int position);

        void selectItem(int session_id);
        void selectPosition(int position);

        void reloadSkin(const QString& skin);


    public slots:
        void slotSelectNextItem();
        void slotSelectPreviousItem();

        void slotMoveItemLeft();
        void slotMoveItemRight();


    signals:
        void addItem();
        void removeItem();
        void itemSelected(int session_id);
        void updateBackground();


    protected:
        virtual void paintEvent(QPaintEvent*);
        virtual void resizeEvent(QResizeEvent*);


    private:
        void setPixmaps(const QString& skin);
        void loadSkin(const QString& skin);

        /* Text properties */
        QColor text_color;

        /* Widget's pixmaps */
        QPixmap back_image;
        QPixmap left_corner;
        QPixmap right_corner;

        /* Plus button */
        QPoint plus_position;
        ImageButton* plus_button;

        /* Minus button */
        QPoint minus_position;
        ImageButton* minus_button;

        /* Tabbed widget */
        QPoint tabs_position;
        TabbedWidget* tabs_widget;
};

#endif /* TAB_BAR_H */
