/*
    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.
*/

/*
  Copyright (C) 2007 Eike Hein <hein@kde.org>
*/


#include "translucent_widget.h"
#include "translucent_widget.moc"

#include <krootpixmap.h>


TranslucentWidget::TranslucentWidget(QWidget* parent, const char* name, bool translucency) : QWidget(parent, name)
{
    use_translucency = translucency;

    root_pixmap = NULL;

    if (use_translucency)
    {
        root_pixmap = new KRootPixmap(this);
        root_pixmap->start();
    }
}

TranslucentWidget::~TranslucentWidget()
{
    if (root_pixmap) delete root_pixmap;
}

void TranslucentWidget::slotUpdateBackground()
{

    // This is wired up to KApplication::backgroundChanged and needed
    // to kick KRootPixmap into updating the background again, which
    // it likes to forget after having been moved off-screen.
    if (root_pixmap) root_pixmap->repaint(true);
}
