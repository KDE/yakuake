/*
  SPDX-FileCopyrightText: 2008-2014 Eike Hein <hein@kde.org>
  SPDX-FileCopyrightText: 2020 Ryan McCoskrie <work@ryanmccoskrie.me>

  SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#ifndef TITLEBAR_H
#define TITLEBAR_H

#include <QMouseEvent>
#include <QWidget>

class MainWindow;
class Skin;

class QPushButton;

class TitleBar : public QWidget
{
    Q_OBJECT

public:
    explicit TitleBar(MainWindow *mainWindow);
    ~TitleBar() override;

    void setVisible(bool visible) override;
    void applySkin();
    void updateMask();
    void updateMenu();

    QString title();

    void setFocusButtonState(bool checked);

public Q_SLOTS:
    void setTitle(const QString &title);

protected:
    void resizeEvent(QResizeEvent *) override;
    void paintEvent(QPaintEvent *) override;
    void mouseMoveEvent(QMouseEvent *) override;

private:
    void moveButtons();

    MainWindow *m_mainWindow = nullptr;
    Skin *m_skin = nullptr;
    bool m_visible = false;

    QPushButton *m_focusButton = nullptr;
    QPushButton *m_menuButton = nullptr;
    QPushButton *m_quitButton = nullptr;

    QString m_title;
};

#endif
