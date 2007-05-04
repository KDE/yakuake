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


#ifndef IMAGE_BUTTON_H
#define IMAGE_BUTTON_H


#include <qurl.h>
#include <qcolor.h>
#include <qpoint.h>
#include <qbitmap.h>
#include <qpixmap.h>
#include <qwidget.h>
#include <qpainter.h>
#include <qpopupmenu.h>

#include <krootpixmap.h>


class ImageButton : public QWidget
{
    Q_OBJECT

    public:
        explicit ImageButton(QWidget* parent = 0, const char * name = 0);
        ~ImageButton();

        /* Creates a translucent button */
        void setTranslucent(bool value);

        /* Creates a toggle button */
        void setToggleButton(bool toggled);
        void setToggled(bool enable);

        /* Sets the configuration menu */
        void setPopupMenu(QPopupMenu* menu);
        void setDelayedPopup(bool delay) { delay_popup = delay; }

        /* Sets the widget's pixmaps */
        void setUpPixmap(const QString& path);
        void setOverPixmap(const QString& path);
        void setDownPixmap(const QString& path);


    public slots:
        void slotUpdateBackground();


    signals:
        void clicked();
        void toggled(bool toggled);


    protected:
        virtual void enterEvent(QEvent*);
        virtual void leaveEvent(QEvent*);

        virtual void paintEvent(QPaintEvent*);

        virtual void mousePressEvent(QMouseEvent*);
        virtual void mouseReleaseEvent(QMouseEvent*);


    private:
        int state;
        bool toggle;
        bool pressed;
        bool delay_popup;

        QTimer* popup_timer;

        /* Widget's mask */
        QRegion mask;

        /* Widget's tip */
        QString tooltip;

        /* Widget's pixmaps */
        QPixmap up_pixmap;
        QPixmap over_pixmap;
        QPixmap down_pixmap;

        /* Widget's popup menu */
        QPopupMenu* popup_menu;

        /* Widget's rootPixmap */
        KRootPixmap* root_pixmap;

    private slots:
        void showPopupMenu();
};

#endif /* IMAGE_BUTTON_H */
