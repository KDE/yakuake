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


#include "tabbed_widget.h"
#include "tabbed_widget.moc"

#include <qcursor.h>
#include <qwhatsthis.h>

#include <kaction.h>
#include <kpopupmenu.h>
#include <kmainwindow.h>
#include <kiconloader.h>


TabbedWidget::TabbedWidget(QWidget* parent, const char* name) : QWidget(parent, name)
{
    current_position = -1;
    pressed = false;
    pressed_position = -1;
    edited_position = -1;

    context_menu = 0;

    inline_edit = new QLineEdit(this);
    inline_edit->hide();

    root_pixmap = new KRootPixmap(this, "Transparent background");
    root_pixmap->setCustomPainting(true);
    root_pixmap->start();

    connect(root_pixmap, SIGNAL(backgroundUpdated(const QPixmap &)), this, SLOT(slotUpdateBuffer(const QPixmap &)));
    connect(inline_edit, SIGNAL(returnPressed()), this, SLOT(slotRenameSelected()));
    connect(inline_edit, SIGNAL(lostFocus()), inline_edit, SLOT(hide()));
    connect(inline_edit, SIGNAL(lostFocus()), this, SLOT(slotResetEditedPosition()));

    selected_font = font();
    unselected_font = font();
    selected_font.setBold(true);

    refreshBuffer();
}

TabbedWidget::~TabbedWidget()
{
    delete context_menu;
    delete root_pixmap;
}

int TabbedWidget::pressedPosition()
{
    return pressed_position;
}

void TabbedWidget::createContextMenu()
{
    context_menu = new KPopupMenu();

    static_cast<KMainWindow*>(parent()->parent())->actionCollection()->
        action("split_horizontally")->plug(context_menu);
    static_cast<KMainWindow*>(parent()->parent())->actionCollection()->
        action("split_vertically")->plug(context_menu);
    static_cast<KMainWindow*>(parent()->parent())->actionCollection()->
        action("remove_terminal")->plug(context_menu);

    context_menu->insertSeparator();

    static_cast<KMainWindow*>(parent()->parent())->actionCollection()->
        action("move_tab_left")->plug(context_menu);
    static_cast<KMainWindow*>(parent()->parent())->actionCollection()->
        action("move_tab_right")->plug(context_menu);

    context_menu->insertSeparator();

    static_cast<KMainWindow*>(parent()->parent())->actionCollection()->
        action("edit_name")->plug(context_menu);

    static_cast<KMainWindow*>(parent()->parent())->actionCollection()->
        action("remove_tab")->plug(context_menu);

}

void TabbedWidget::addItem(int session_id)
{
    items.append(session_id);
    areas.append(0);
    captions.append(lowestAvailableCaption());

    refreshBuffer();
}

QString TabbedWidget::defaultTabCaption(int id)
{
    return i18n("Shell", "Shell No. %n", id+1);
}

QString TabbedWidget::lowestAvailableCaption()
{
    QString newTitle = defaultTabCaption(0);

    bool nameOk;
    int count = 0;

    do
    {
        nameOk = true;

        QValueList<QString>::iterator it;

        for (it = captions.begin(); it != captions.end(); ++it)
        {
            if (newTitle == (*it))
            {
                nameOk = false;
                break;
            }
        }

        if (!nameOk)
        {
            count++;
            newTitle = newTitle = defaultTabCaption(count);
        }
    }
    while (!nameOk);

    return newTitle;
}

int TabbedWidget::removeItem(int session_id)
{
    uint position = items.findIndex(session_id);

    items.remove(items.at(position));
    areas.remove(areas.at(position));
    captions.remove(captions.at(position));

    if (position != items.count())
        current_position = position;
    else if (position != 0)
        current_position = position - 1;
    else
        current_position = -1;

    refreshBuffer();

    if (current_position != -1)
        emit itemSelected(items[current_position]);

    return current_position;
}

const QString TabbedWidget::itemName(int session_id)
{
    int position = items.findIndex(session_id);

    if (position == -1) return 0;

    return captions[position];
}

void TabbedWidget::renameItem(int session_id, const QString& namep)
{
    int position = items.findIndex(session_id);

    if (position == -1) return;

    QString name = namep.stripWhiteSpace();
    captions[position] = !name.isEmpty() ? name : captions[position];

    refreshBuffer();
}

void TabbedWidget::interactiveRename()
{
    if (pressed_position != -1)
    {
        interactiveRename(pressed_position);
        pressed_position = -1;
    }
    else
    {
        interactiveRename(current_position);
    }
}

void TabbedWidget::interactiveRename(int position)
{
    if (position >= 0 && position < int(items.count()) && !items.isEmpty())
    {
        edited_position = position;

        int index;
        int width;

        for (index = 0, width = 0; index < position; index++)
            width += areas[index];

        inline_edit->setText(captions[position]);
        inline_edit->setGeometry(width, 0, areas[position], height());
        inline_edit->setAlignment(Qt::AlignHCenter);
        inline_edit->setFrame(false);
        inline_edit->selectAll();
        inline_edit->setFocus();
        inline_edit->show();
    }
}

void TabbedWidget::slotRenameSelected()
{
    if (edited_position >= 0 && edited_position < int(items.count()))
    {
        QString text = inline_edit->text().stripWhiteSpace();
        captions[edited_position] = !text.isEmpty() ? text : captions[edited_position];

        inline_edit->hide();

        edited_position = -1;

        refreshBuffer();
    }
}

void TabbedWidget::slotResetEditedPosition()
{
    edited_position = -1;
}

int TabbedWidget::tabPositionForSessionId(int session_id)
{
    return items.findIndex(session_id);
}

int TabbedWidget::sessionIdForTabPosition(int position)
{
    if (position < int(items.count()) && !items.isEmpty())
        return items[position];
    else
        return -1;
}

void TabbedWidget::selectItem(int session_id)
{
    int new_position = items.findIndex(session_id);

    if (new_position != -1)
    {
        current_position = new_position;
        refreshBuffer();
    }
}

void TabbedWidget::selectPosition(int position)
{
    if (position < int(items.count()) && !items.isEmpty())
    {
        current_position = position;

        refreshBuffer();

        emit itemSelected(items[current_position]);
    }
}

void TabbedWidget::selectNextItem()
{
    if (current_position != int(items.count()) - 1)
        current_position++;
    else
        current_position = 0;

    refreshBuffer();

    emit itemSelected(items[current_position]);
}

void TabbedWidget::selectPreviousItem()
{
    if (current_position != 0)
        current_position--;
    else
        current_position = items.count() - 1;

    refreshBuffer();

    emit itemSelected(items[current_position]);
}

void TabbedWidget::moveItemLeft()
{
    if (pressed_position != -1)
    {
        moveItemLeft(pressed_position);
        pressed_position = -1;
    }
    else
        moveItemLeft(current_position);
}


void TabbedWidget::moveItemLeft(int position)
{
    if (position > 0)
    {
        int session_id = items[position];
        QString caption = captions[position];

        int neighbor_session_id = items[position-1];
        QString neighbor_caption = captions[position-1];

        items[position-1] = session_id;
        captions[position-1] = caption;

        items[position] = neighbor_session_id;
        captions[position] = neighbor_caption;

        if (position == current_position)
            current_position--;
        else if (position == current_position + 1)
            current_position++;

        refreshBuffer();
    }
}

void TabbedWidget::moveItemRight()
{
    if (pressed_position != -1)
    {
        moveItemRight(pressed_position);
        pressed_position = -1;
    }
    else
        moveItemRight(current_position);
}

void TabbedWidget::moveItemRight(int position)
{
    if (position < int(items.count()) - 1)
    {
        int session_id = items[position];
        QString caption = captions[position];

        int neighbor_session_id = items[position+1];
        QString neighbor_caption = captions[position+1];

        items[position+1] = session_id;
        captions[position+1] = caption;

        items[position] = neighbor_session_id;
        captions[position] = neighbor_caption;

        if (position == current_position)
            current_position++;
        else if (position == current_position - 1)
            current_position--;

        refreshBuffer();
    }
}

void TabbedWidget::setFontColor(const QColor & color)
{
    font_color = color;
}

void TabbedWidget::setBackgroundPixmap(const QString & path)
{
    background_image.load(path);
    resize(width(), background_image.height());

    repaint();
}

void TabbedWidget::setSeparatorPixmap(const QString & path)
{
    separator_image.load(path);
}

void TabbedWidget::setUnselectedPixmap(const QString & path)
{
    unselected_image.load(path);
}

void TabbedWidget::setSelectedPixmap(const QString & path)
{
    selected_image.load(path);
}

void TabbedWidget::setSelectedLeftPixmap(const QString & path)
{
    selected_left_image.load(path);
}

void TabbedWidget::setSelectedRightPixmap(const QString & path)
{
    selected_right_image.load(path);
}

void TabbedWidget::keyPressEvent(QKeyEvent* e)
{
    if (e->key() == Key_Escape && inline_edit->isShown())
    {
        inline_edit->hide();
        edited_position = -1;
    }

    QWidget::keyPressEvent(e);
}

void TabbedWidget::wheelEvent(QWheelEvent* e)
{
    if (e->delta() < 0)
        selectNextItem();
    else
        selectPreviousItem();
}

void TabbedWidget::mousePressEvent(QMouseEvent* e)
{
    if (QWhatsThis::inWhatsThisMode()) return;

    int position;
    int width;

    if (e->x() < 0)
        return ;

    for (position = 0, width = 0; (position < int(areas.count())) && (e->x() >= width); position++)
        width += areas[position];

    if ((e->x() <= width) && (e->button() == Qt::LeftButton) && !(current_position == position - 1))
    {
        pressed = true;
        pressed_position = position - 1;
    }
    else if ((e->x() <= width) && (e->button() == Qt::RightButton))
    {
        pressed_position = position - 1;

        if (!context_menu) createContextMenu();

        static_cast<KMainWindow*>(parent()->parent())->actionCollection()->
            action("move_tab_left")->setEnabled((position - 1 > 0));
        static_cast<KMainWindow*>(parent()->parent())->actionCollection()->
            action("move_tab_right")->setEnabled((position - 1 < int(items.count()) - 1));

        context_menu->exec(QCursor::pos());

        static_cast<KMainWindow*>(parent()->parent())->actionCollection()->
            action("move_tab_left")->setEnabled(true);
        static_cast<KMainWindow*>(parent()->parent())->actionCollection()->
            action("move_tab_right")->setEnabled(true);
    }
}

void TabbedWidget::mouseReleaseEvent(QMouseEvent* e)
{
    if (QWhatsThis::inWhatsThisMode()) return;

    int position;
    int width;

    if (e->x() < 0)
        return ;

    for (position = 0, width = 0; (position < int(areas.count())) && (e->x() >= width); position++)
        width += areas[position];

    if ((e->x() <= width) && (e->button() == Qt::LeftButton) && !(current_position == position - 1))
    {
        if (pressed && pressed_position == (position - 1))
        {
            current_position = position - 1;

            refreshBuffer();

            emit itemSelected(items[current_position]);
        }
    }

    pressed = false;
}

void TabbedWidget::mouseDoubleClickEvent(QMouseEvent* e)
{
    if (QWhatsThis::inWhatsThisMode()) return;

    int position;
    int width;

    inline_edit->hide();

    if (e->x() < 0)
        return ;

    for (position = 0, width = 0; (position < int(areas.count())) && (e->x() >= width); position++)
        width += areas[position];

    if ((e->x() <= width) && (e->button() == Qt::LeftButton) && current_position == position - 1)
    {
        interactiveRename(position - 1);
    }
    else if ((e->x() > width) && (e->button() == Qt::LeftButton))
        emit addItem();
}

void TabbedWidget::leaveEvent(QEvent* e)
{
    pressed = false;

    QWidget::leaveEvent(e);
}

void TabbedWidget::paintEvent(QPaintEvent*)
{
    bitBlt(this, 0, 0, &buffer_image);
}

void TabbedWidget::refreshBuffer()
{
    /* Refreshes the back buffer. */

    int x = 0;
    QPainter painter;

    buffer_image.resize(size());
    painter.begin(&buffer_image);

    painter.drawTiledPixmap(0, 0, width(), height(), desktop_image);

    for (uint i = 0; i < items.count(); i++)
        x = drawButton(i, painter);

    painter.drawTiledPixmap(x, 0, width() - x, height(), background_image);

    painter.end();

    repaint();
}

const int TabbedWidget::drawButton(int position, QPainter& painter)
{
    /* Draws the tab buttons. */

    static int x = 0;
    QPixmap tmp_pixmap;

    areas[position] = 0;
    x = (!position) ? 0 : x;

    // Initialize the painter.
    painter.setPen(font_color);
    painter.setFont((position == current_position) ? selected_font : unselected_font);

    // Draw the left border.
    if (position == current_position)
        tmp_pixmap = selected_left_image;
    else if (position != current_position + 1)
        tmp_pixmap = separator_image;

    painter.drawPixmap(x, 0, tmp_pixmap);
    areas[position] += tmp_pixmap.width();
    x += tmp_pixmap.width();

    // Draw the main contents.
    int width;
    QFontMetrics metrics(painter.font());

    width = metrics.width(captions[position]) + 10;

    tmp_pixmap = (position == current_position) ? selected_image : unselected_image;
    painter.drawTiledPixmap(x, 0, width, height(), tmp_pixmap);

    painter.drawText(x, 0, width + 1, height() + 1,
                     Qt::AlignHCenter | Qt::AlignVCenter,  captions[position]);

    areas[position] += width;
    x += width;

    // Draw the right border if needed.
    if (position == current_position)
    {
        painter.drawPixmap(x, 0, selected_right_image);
        areas[position] += selected_right_image.width();
        x += selected_right_image.width();
    }
    else if (position == (int) items.count() - 1)
    {
        painter.drawPixmap(x, 0, separator_image);
        areas[position] += separator_image.width();
        x += separator_image.width();
    }

    return x;
}

void TabbedWidget::slotUpdateBuffer(const QPixmap& pixmap)
{
    desktop_image = pixmap;

    refreshBuffer();
}

void TabbedWidget::slotUpdateBackground()
{
    if (root_pixmap) root_pixmap->repaint(true);
}
