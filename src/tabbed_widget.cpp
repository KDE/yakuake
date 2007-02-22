/*****************************************************************************
 *                                                                           *
 *   Copyright (C) 2005 by Chazal Francois             <neptune3k@free.fr>   *
 *   website : http://workspace.free.fr                                      *
 *                                                                           *
 *                     =========  GPL License  =========                     *
 *    This program is free software; you can redistribute it and/or modify   *
 *   it under the terms of the  GNU General Public License as published by   *
 *   the  Free  Software  Foundation ; either version 2 of the License, or   *
 *   (at your option) any later version.                                     *
 *                                                                           *
 *****************************************************************************/

//== INCLUDE REQUIREMENTS =====================================================

/*
** Local libraries */
#include "tabbed_widget.h"
#include "tabbed_widget.moc"
#include <kdebug.h>

//== CONSTRUCTORS AND DESTRUCTORS =============================================


TabbedWidget::TabbedWidget(QWidget * parent, const char * name) : QWidget(parent, name)
{
    inline_edit = new QLineEdit(this);
    inline_edit->hide();

    // Initializes the background pixmap ------------------

    root_pixmap = new KRootPixmap(this, "Transparent background");
    root_pixmap->setCustomPainting(true);
    root_pixmap->start();

    connect(root_pixmap, SIGNAL(backgroundUpdated(const QPixmap &)), this, SLOT(slotUpdateBuffer(const QPixmap &)));
    connect(inline_edit, SIGNAL(returnPressed()), this, SLOT(slotRenameSelected()));
    connect(inline_edit, SIGNAL(lostFocus()), this, SLOT(slotLostFocus()));

    selected_font = font();
    unselected_font = font();
    selected_font.setBold(true);

    refreshBuffer();
}

TabbedWidget::~TabbedWidget()
{
    delete root_pixmap;
}



//== PUBLIC METHODS ===========================================================


/******************************************************************************
** Adds an item to the tabs
*****************************/

void    TabbedWidget::addItem(int id)
{
    items.append(id);
    areas.append(0);
    captions.append(defaultTabCaption(id));

    refreshBuffer();
}


/******************************************************************************
** Selects an item in the tabs
********************************/

void    TabbedWidget::selectItem(int id)
{
    selected_id = items.findIndex(id);
    refreshBuffer();
}


/******************************************************************************
** Removes an item from the tabs
**********************************/

int     TabbedWidget::removeItem(int id)
{
    int index = items.findIndex(id);

    items.remove(items.at(index));
    areas.remove(areas.at(index));
    captions.remove(captions.at(index));

    if (index != items.size())
        selected_id = index;
    else if (index != 0)
        selected_id = index - 1;
    else
        selected_id = -1;

    refreshBuffer();

    if (selected_id != -1)
        emit itemSelected(items[selected_id]);

    return selected_id;
}


/******************************************************************************
** Selects the new item in the list
*************************************/

void    TabbedWidget::selectNextItem()
{
    if (selected_id != items.size() - 1)
        selected_id++;
    else
        selected_id = 0;

    refreshBuffer();

    emit itemSelected(items[selected_id]);
}


/******************************************************************************
** Selects the previous item in the list
******************************************/

void    TabbedWidget::selectPreviousItem()
{
    if (selected_id != 0)
        selected_id--;
    else
        selected_id = items.size() - 1;

    refreshBuffer();

    emit itemSelected(items[selected_id]);
}


/******************************************************************************
** Renames an item given its id
*********************************/

void    TabbedWidget::renameItem(int id, const QString & namep)
{
    int index = items.findIndex(id);

    if (index == -1) return;

    QString name = namep.stripWhiteSpace();
    captions[index] = !name.isEmpty() ? name : captions[index];

    refreshBuffer();
}


/******************************************************************************
** Open inline edit for the current item
******************************************/

void    TabbedWidget::interactiveRename()
{
    kdDebug() << "Hit" << endl;

    uint    id;
    int     width;

    int index = items.findIndex(selected_id);

    for (id = 0, width = 0; id < selected_id; id++)
        width += areas[id];

    inline_edit->setText(captions[index]);
    inline_edit->setGeometry(width, 0, areas[index], height());
    inline_edit->setAlignment(Qt::AlignHCenter);
    inline_edit->setFrame(false);
    inline_edit->selectAll();
    inline_edit->setFocus();
    inline_edit->show();
}


/******************************************************************************
** Sets the font color
************************/

void    TabbedWidget::setFontColor(const QColor & color)
{
    font_color = color;
}


/******************************************************************************
** Loads the background pixmap
********************************/

void    TabbedWidget::setBackgroundPixmap(const QString & path)
{
    background_image.load(path);
    resize(width(), background_image.height());

    repaint();
}


/******************************************************************************
** Loads the separator pixmap
*******************************/

void    TabbedWidget::setSeparatorPixmap(const QString & path)
{
    separator_image.load(path);
}


/******************************************************************************
** Loads the unselected background pixmap
*******************************************/

void    TabbedWidget::setUnselectedPixmap(const QString & path)
{
    unselected_image.load(path);
}


/******************************************************************************
** Loads the selected background pixmap
*****************************************/

void    TabbedWidget::setSelectedPixmap(const QString & path)
{
    selected_image.load(path);
}


/******************************************************************************
** Loads the selected left corner pixmap
******************************************/

void    TabbedWidget::setSelectedLeftPixmap(const QString & path)
{
    selected_left_image.load(path);
}


/******************************************************************************
** Loads the selected right corner pixmap
*******************************************/

void    TabbedWidget::setSelectedRightPixmap(const QString & path)
{
    selected_right_image.load(path);
}



//== PROTECTED METHODS ========================================================


/******************************************************************************
** Repaints the widget when asked
***********************************/

void    TabbedWidget::paintEvent(QPaintEvent *)
{
    bitBlt(this, 0, 0, &buffer_image);
}


/******************************************************************************
** Changes focused tab on mouse scroll
****************************************/

void    TabbedWidget::wheelEvent(QWheelEvent *e)
{
    if (e->delta() < 0)
        selectNextItem();
    else
        selectPreviousItem();
}


/******************************************************************************
** Modifies button's state (mouse up)
***************************************/

void    TabbedWidget::mouseReleaseEvent(QMouseEvent *e)
{
    uint    id;
    int     width;

    inline_edit->hide();

    if (e->x() < 0)
        return ;

    for (id = 0, width = 0; (id < areas.size()) && (e->x() >= width); id++)
        width += areas[id];

    if ((e->x() <= width) && (e->button() == Qt::LeftButton))
    {
        if (selected_id == id - 1)
        {
            for (id = 0, width = 0; id < selected_id; id++)
                width += areas[id];

            inline_edit->setText(captions[id]);
            inline_edit->setGeometry(width, 0, areas[selected_id], height());
            inline_edit->setAlignment(Qt::AlignHCenter);
            inline_edit->setFrame(false);
            inline_edit->selectAll();
            inline_edit->setFocus();
            inline_edit->show();
        }
        else
        {
            selected_id = id - 1;

            refreshBuffer();
            emit itemSelected(items[selected_id]);
        }
    }
}

QString TabbedWidget::defaultTabCaption(int id)
{
    return i18n("Shell", "Shell No. %n", id+1);
}


//== PRIVATE METHODS ==========================================================


/******************************************************************************
** Refreshes the back buffer
******************************/

void   TabbedWidget::refreshBuffer()
{
    int         x = 0;
    QPainter    painter;

    buffer_image.resize(size());
    painter.begin(&buffer_image);

    painter.drawTiledPixmap(0, 0, width(), height(), desktop_image);

    for (uint i = 0; i < items.size(); i++)
        x = drawButton(i, painter);

    painter.drawTiledPixmap(x, 0, width() - x, height(), background_image);

    painter.end();

    repaint();
}


/******************************************************************************
** Draws the tabs button
**************************/

const int   TabbedWidget::drawButton(int id, QPainter & painter)
{
    static int  x = 0;
    QPixmap     tmp_pixmap;

    areas[id] = 0;
    x = (!id) ? 0 : x;

    // Initializes the painter ----------------------------

    painter.setPen(font_color);
    painter.setFont((id == selected_id) ? selected_font : unselected_font);

    // draws the left border ------------------------------

    if (id == selected_id)
        tmp_pixmap = selected_left_image;
    else if (id != selected_id + 1)
        tmp_pixmap = separator_image;

    painter.drawPixmap(x, 0, tmp_pixmap);
    areas[id] += tmp_pixmap.width();
    x += tmp_pixmap.width();

    // draws the main contents ----------------------------

    int             width;
    QFontMetrics    metrics(painter.font());

    width = metrics.width(captions[id]) + 10;

    tmp_pixmap = (id == selected_id) ? selected_image : unselected_image;
    painter.drawTiledPixmap(x, 0, width, height(), tmp_pixmap);

    painter.drawText(x, 0, width + 1, height() + 1,
                     Qt::AlignHCenter | Qt::AlignVCenter,  captions[id]);

    areas[id] += width;
    x += width;

    // draws the right border if needed -------------------

    if (id == selected_id)
    {
        painter.drawPixmap(x, 0, selected_right_image);
        areas[id] += selected_right_image.width();
        x += selected_right_image.width();
    }
    else if (id == (int) items.size() - 1)
    {
        painter.drawPixmap(x, 0, separator_image);
        areas[id] += separator_image.width();
        x += separator_image.width();
    }

    return x;
}



//== PRIVATE SLOTS ============================================================


void    TabbedWidget::slotRenameSelected()
{
    inline_edit->hide();

    QString text = inline_edit->text().stripWhiteSpace();
    captions[selected_id] = !text.isEmpty() ? text : captions[selected_id];

    refreshBuffer();
}


void    TabbedWidget::slotUpdateBuffer(const QPixmap & pixmap)
{
    desktop_image = pixmap;

    refreshBuffer();
}
