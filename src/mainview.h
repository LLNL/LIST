//******************************************************************************
// Copyright 2019-2020 Lawrence Livermore National Security, LLC and other
// LIST Project Developers. See the LICENSE file for details.
// SPDX-License-Identifier: MIT
//
// LIvermore Sem image Tools (LIST)
// Main view class (center image view) (.h, .cpp)
//*****************************************************************************/

#ifndef MAINVIEW_H
#define MAINVIEW_H

#include <QWidget>

class MainWindow;


class MainView : public QWidget
{
    Q_OBJECT

public:
    MainView(QWidget *parent = 0);
    ~MainView();

    QSize minimumSizeHint() const;
    QSize sizeHint() const;

    void setImage();

protected:
    void keyPressEvent(QKeyEvent *event) override;
    void keyReleaseEvent(QKeyEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void paintEvent(QPaintEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private:
    bool                m_CtrlKeyPressed;
    QPoint              m_MousePressed;
    QImage              m_Image;

public:
    MainWindow*         m_MainWindow;


};



#endif // MAINVIEW_H
