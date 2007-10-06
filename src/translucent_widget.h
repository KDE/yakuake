/*
    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.
*/

/*
  Copyright (C) 2007 Eike Hein <hein@kde.org>
*/


#ifndef TRANSLUCENT_WIDGET_H
#define TRANSLUCENT_WIDGET_H


#include <qwidget.h>


class KRootPixmap;


class TranslucentWidget : public QWidget
{
    Q_OBJECT

    public:
        explicit TranslucentWidget(QWidget* parent = 0, const char* name = 0, bool translucency = false);
        virtual ~TranslucentWidget();


    public slots:
        void slotUpdateBackground();


    protected:
        bool useTranslucency() { return use_translucency; }
        void setUseTranslucency(bool translucency) { use_translucency = translucency; }

        KRootPixmap* root_pixmap;
        bool use_translucency;
};


#endif /* TRANSLUCENT_WIDGET_H */
