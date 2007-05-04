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


#ifndef TITLE_BAR_H
# define TITLE_BAR_H

#include "image_button.h"

#include <qurl.h>
#include <qcolor.h>
#include <qpoint.h>
#include <qbitmap.h>
#include <qpixmap.h>
#include <qwidget.h>
#include <qpainter.h>
#include <qpushbutton.h>


#include <kconfig.h>
#include <kpopupmenu.h>
#include <kapplication.h>
#include <kstandarddirs.h>


class TitleBar : public QWidget
{
    Q_OBJECT

    public:
        explicit TitleBar(QWidget * parent = 0, const char * name = 0, const QString & skin = "default");
        ~TitleBar();

        QRegion& getWidgetMask();

        void setTitleText(const QString& title);

        void setFocusButtonEnabled(bool enable);

        void setConfigurationMenu(KPopupMenu* menu);

        void reloadSkin(const QString& skin);


    protected:
        virtual void paintEvent(QPaintEvent*);
        virtual void resizeEvent(QResizeEvent*);


    private:
        void setPixmaps(const QString& skin);
        void loadSkin(const QString& skin);
        void updateWidgetMask();

        /* Widget's mask */
        QRegion mask;

        /* Text properties */
        QString title_text;
        QString skin_text;
        QColor text_color;
        QPoint text_position;

        /* Widget's pixmaps */
        QPixmap back_image;
        QPixmap left_corner;
        QPixmap right_corner;

        /* Quit button */
        QPoint quit_position;
        ImageButton* quit_button;

        /* Focus button */
        QPoint focus_position;
        ImageButton* focus_button;

        /* Configure button */
        QPoint config_position;
        ImageButton* config_button;
};

#endif /* TITLE_BAR_H */
