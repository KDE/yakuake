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


#ifndef TABBED_WIDGET_H
#define TABBED_WIDGET_H


#include <qfont.h>
#include <qcolor.h>
#include <qpoint.h>
#include <qpixmap.h>
#include <qwidget.h>
#include <qpainter.h>
#include <qlineedit.h>
#include <qvaluelist.h>
#include <qfontmetrics.h>

#include <klocale.h>
#include <krootpixmap.h>
#include <kinputdialog.h>


class KPopupMenu;

class TabbedWidget : public QWidget
{
    Q_OBJECT

    public:
        explicit TabbedWidget(QWidget* parent = 0, const char* name = 0);
        ~TabbedWidget();

        int pressedPosition();
        void resetPressedPosition() { pressed_position = -1; }

        void addItem(int session_id);
        int removeItem(int session_id);

        int tabPositionForSessionId(int session_id);
        int sessionIdForTabPosition(int position);

        void selectItem(int session_id);
        void selectPosition(int position);

        void selectNextItem();
        void selectPreviousItem();

        void moveItemLeft();
        void moveItemLeft(int position);

        void moveItemRight();
        void moveItemRight(int position);

        void renameItem(int session_id, const QString& name);

        void interactiveRename();
        void interactiveRename(int position);

        void setFontColor(const QColor& color);
        void setBackgroundPixmap(const QString& path);
        void setSeparatorPixmap(const QString& path);

        void setUnselectedPixmap(const QString& path);

        void setSelectedPixmap(const QString& path);
        void setSelectedLeftPixmap(const QString& path);
        void setSelectedRightPixmap(const QString& path);

        void refreshBuffer();


    public slots:
        void slotUpdateBackground();


    signals:
        void addItem();
        void itemSelected(int session_id);


    protected:
        virtual void keyPressEvent(QKeyEvent*);

        virtual void wheelEvent(QWheelEvent*);
        virtual void mousePressEvent(QMouseEvent*);
        virtual void mouseReleaseEvent(QMouseEvent*);
        virtual void mouseDoubleClickEvent(QMouseEvent*);

        virtual void leaveEvent(QEvent*);

        virtual void paintEvent(QPaintEvent*);


    private:
        void createContextMenu();
        const int drawButton(int position, QPainter& painter);
        QString defaultTabCaption(int session_id);

        int current_position;
        bool pressed;
        int pressed_position;
        int edited_position;

        /* Tabs properties */
        QColor font_color;
        QFont selected_font;
        QFont unselected_font;

        /* Inline renaming */
        QLineEdit* inline_edit;

        /* Widget's pixmaps */
        QPixmap background_image;

        QPixmap separator_image;
        QPixmap unselected_image;

        QPixmap selected_image;
        QPixmap selected_left_image;
        QPixmap selected_right_image;

        /* Widget's appearance */
        QPixmap buffer_image;
        QPixmap desktop_image;

        /* Tabs value lists */
        QValueList<int> items;
        QValueList<int> areas;
        QValueList<QString> captions;

        /* Widget's rootPixmap */
        KRootPixmap* root_pixmap;

        KPopupMenu* context_menu;


    private slots:
        void slotRenameSelected();
        void slotUpdateBuffer(const QPixmap& pixmap);
        void slotResetEditedPosition();
};

#endif /* TABBED_WIDGET_H */
