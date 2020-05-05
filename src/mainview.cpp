//******************************************************************************
// Copyright 2019-2020 Lawrence Livermore National Security, LLC and other
// LIST Project Developers. See the LICENSE file for details.
// SPDX-License-Identifier: MIT
//
// LIvermore Sem image Tools (LIST)
// Main view class (center image view) (.h, .cpp)
//*****************************************************************************/

#include <QMouseEvent>
#include <QPaintEvent>
#include <QResizeEvent>
#include <QPainter>
#include <QImage>
#include <QPoint>
#include <QLineEdit>
#include <QCheckBox>
#include <QRadioButton>
#include <QTableWidget>
#include <QHeaderView>

#include <stdio.h>
#include <stdlib.h>
#include <iostream>

#include "mainwindow.h"
#include "mainview.h"
#include "semproc.h"



MainView::MainView(QWidget *parent) : QWidget(parent)
{
    m_MainWindow = (MainWindow*)parent;
    m_CtrlKeyPressed = false;
    this->setMouseTracking(true);
    this->setFocusPolicy(Qt::StrongFocus);
}

MainView::~MainView()
{
}

QSize MainView::minimumSizeHint() const
{
    return QSize(200, 200);
}

QSize MainView::sizeHint() const
{
    return QSize(1400, 800);
}

void MainView::setImage()
{
    CImage* image;
    if (m_MainWindow->m_ImageRadio1->isChecked())
        image = m_MainWindow->m_SEMShape->getImage();
    else if (m_MainWindow->m_ImageRadio2->isChecked())
        image = m_MainWindow->m_SEMShape->getAdjImage();
    else if (m_MainWindow->m_ImageRadio3->isChecked())
        image = m_MainWindow->m_SEMShape->getBinImage();
    else if (m_MainWindow->m_ImageRadio4->isChecked())
        image = m_MainWindow->m_SEMShape->getDstImage();
    else
        image = m_MainWindow->m_SEMShape->getOutImage();
    if (image->getWidth() == 0 || image->getPixels() == NULL)
        return;

    QImage qimage(image->getWidth(), image->getHeight(), QImage::Format_ARGB32);
    for (int y = 0; y < image->getHeight(); ++y) {
        QRgb *destrow = (QRgb*)qimage.scanLine(image->getHeight() - 1 - y);
        for (int x = 0; x < image->getWidth(); ++x) {
            float* psrc = image->getPixel(x, y);
            unsigned int pdest = (unsigned int)(*psrc * 255);
            destrow[x] = qRgba(pdest, pdest, pdest, 255);
        }
    }
    //qimage.loadFromData(image->getPixels(), image->getWidth() * image->getHeight());
    m_Image = qimage.scaled(this->width(), this->height(), Qt::KeepAspectRatio);

    this->update();
}

void MainView::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Control)
        m_CtrlKeyPressed = true;
}

void MainView::keyReleaseEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Control)
        m_CtrlKeyPressed = false;
}

void MainView::mousePressEvent(QMouseEvent *event)
{
    m_MousePressed = event->pos();

    if (m_MainWindow->m_SEMShape->getImage()->getWidth() == 0)
        return;

    if (m_MainWindow->m_ScaleBarMode == 1) {
        m_MainWindow->m_ScaleBarMode = 2;
        m_MainWindow->m_ScaleBarBox.setLeft(event->pos().x());
        m_MainWindow->m_ScaleBarBox.setTop(event->pos().y());
        m_MainWindow->m_ScaleBarBox.setRight(event->pos().x());
        m_MainWindow->m_ScaleBarBox.setBottom(event->pos().y());
    }
    else if (m_MainWindow->m_SelectMode == 0) { // && m_CtrlKeyPressed) {
        m_MainWindow->m_SelectMode = 2;
        m_MainWindow->m_SelectBox.setLeft(event->pos().x());
        m_MainWindow->m_SelectBox.setTop(event->pos().y());
        m_MainWindow->m_SelectBox.setRight(event->pos().x());
        m_MainWindow->m_SelectBox.setBottom(event->pos().y());
    }
}

void MainView::mouseMoveEvent(QMouseEvent *event)
{
    int dx = m_MousePressed.x() - event->x();
    int dy = -(m_MousePressed.y() - event->y());

    if (m_MainWindow->m_SEMShape->getImage()->getWidth() == 0)
        return;

    if (m_MainWindow->m_ScaleBarMode == 2) {
        m_MainWindow->m_ScaleBarBox.setRight(event->pos().x());
        m_MainWindow->m_ScaleBarBox.setBottom(event->pos().y());
        this->update();
    }
    else if (m_MainWindow->m_SelectMode == 2) {
        m_MainWindow->m_SelectBox.setRight(event->pos().x());
        m_MainWindow->m_SelectBox.setBottom(event->pos().y());
        this->update();
    }
}

void MainView::mouseReleaseEvent(QMouseEvent *event)
{
    if (m_MainWindow->m_SEMShape->getImage()->getWidth() == 0)
        return;

    float ratio = (float)m_MainWindow->m_SEMScaleBar->getImage()->getWidth() / m_Image.width();
    if (m_MainWindow->m_ScaleBarMode == 2) {
        m_MainWindow->m_ScaleBarMode = 0;
        int xmin = min(m_MainWindow->m_ScaleBarBox.left(), m_MainWindow->m_ScaleBarBox.right()) * ratio;
        int xmax = max(m_MainWindow->m_ScaleBarBox.left(), m_MainWindow->m_ScaleBarBox.right()) * ratio;
        int ymin = min(m_MainWindow->m_ScaleBarBox.top(), m_MainWindow->m_ScaleBarBox.bottom()) * ratio;
        int ymax = max(m_MainWindow->m_ScaleBarBox.top(), m_MainWindow->m_ScaleBarBox.bottom()) * ratio;
        int length = xmax - xmin;
        m_MainWindow->m_SEMScaleBar->manualSelect(xmin, ymin, xmax, ymax, 0, 0);
        m_MainWindow->m_ScalebarLengthEdit->setText(QString::number(length));
        m_MainWindow->m_ScalebarVisible->setCheckState(Qt::Checked);

        this->update();
    }
    else if (m_MainWindow->m_SelectMode == 2) {
        m_MainWindow->m_SelectMode = 0;
        int xmin = min(m_MainWindow->m_SelectBox.left(), m_MainWindow->m_SelectBox.right()) * ratio;
        int xmax = max(m_MainWindow->m_SelectBox.left(), m_MainWindow->m_SelectBox.right()) * ratio;
        int ymin = min(m_MainWindow->m_SelectBox.top(), m_MainWindow->m_SelectBox.bottom()) * ratio;
        int ymax = max(m_MainWindow->m_SelectBox.top(), m_MainWindow->m_SelectBox.bottom()) * ratio;

        if (m_MainWindow->m_SEMShape->selectByBox(xmin, ymin, xmax, ymax) > 0)
            m_MainWindow->selectShapeTable();

        this->update();
    }
}

void MainView::paintEvent(QPaintEvent *event)
{
    QPainter painter(this);

    // draw image
    if (!m_Image.isNull())
        painter.drawImage(QPoint(0, 0), m_Image);
    float ratio = (float)m_Image.width() / m_MainWindow->m_SEMScaleBar->getImage()->getWidth();

    // draw shape centers, contour, radius if available
    std::vector<TShapeInfo>* shape_list = m_MainWindow->m_SEMShape->getShapeList();
    if (shape_list->size() > 0) {

        painter.setBrush(Qt::NoBrush);
        if (m_MainWindow->m_CenterVisible->checkState() == Qt::Checked) {
            for (size_t n = 0; n < shape_list->size(); n++) {
                TShapeInfo sinfo = (*shape_list)[n];
                int x = sinfo.Center.x * ratio;
                int y = sinfo.Center.y * ratio;
                if (sinfo.Selected)
                    painter.setPen(QPen(Qt::red, 2));
                else
                    painter.setPen(QPen(Qt::yellow, 2));
                painter.drawEllipse(x-3, y-3, 6, 6);
            }
        }

        if (m_MainWindow->m_SizeVisible->checkState() == Qt::Checked) {
            for (size_t n = 0; n < shape_list->size(); n++) {
                TShapeInfo* shape_info = &(*shape_list)[n];

                std::vector<INT2> line_S = shape_info->CoreSizeSPoints;
                std::vector<INT2> line_L = shape_info->CoreSizeLPoints;
                if (line_S.size() && line_L.size()) {
                    painter.setPen(QPen(Qt::cyan, 1));
                    QLine line(
                        line_S[0].x * ratio, line_S[0].y * ratio,
                        line_S[1].x * ratio, line_S[1].y * ratio);
                    painter.drawLine(line);
                    line.setLine(
                        line_L[0].x * ratio, line_L[0].y * ratio,
                        line_L[1].x * ratio, line_L[1].y * ratio);
                    painter.drawLine(line);
                }

                line_S = shape_info->ShellSizeSPoints;
                line_L = shape_info->ShellSizeLPoints;
                if (line_S.size() && line_L.size()) {
                    painter.setPen(QPen(Qt::blue, 1));
                    QLine line(
                        line_S[0].x * ratio, line_S[0].y * ratio,
                        line_S[1].x * ratio, line_S[1].y * ratio);
                    painter.drawLine(line);
                    line.setLine(
                        line_L[0].x * ratio, line_L[0].y * ratio,
                        line_L[1].x * ratio, line_L[1].y * ratio);
                    painter.drawLine(line);
                }

                /*std::vector<INT2> core_contour = shape_info->CoreContour;
                std::vector<QPoint> plist(core_contour.size());
                for (size_t p = 0; p < core_contour.size(); p++) {
                    plist[p].setX(core_contour[p].x * ratio);
                    plist[p].setY(core_contour[p].y * ratio);
                }
                painter.drawPoints(&plist[0], plist.size());*/
            }
        }
    }

    // scale bar visible if available
    if (m_MainWindow->m_ScalebarVisible->checkState() == Qt::Checked) {
        std::vector<INT4>* scalebar_list = m_MainWindow->m_SEMScaleBar->getScaleBarList();
        std::vector<INT3>* scale_list = m_MainWindow->m_SEMScaleBar->getScaleInfoList();

        // draw detected scalebar
        painter.setBrush(Qt::NoBrush);
        if (scale_list->size() > 0) {
            INT3 scale_info = (*scale_list)[0];
            INT4 scalebar_box = (*scalebar_list)[scale_info.x];
            int xmin = scalebar_box.x * ratio;
            int ymin = scalebar_box.y * ratio;
            int xmax = scalebar_box.z * ratio;
            int ymax = scalebar_box.w * ratio;
            painter.setPen(Qt::green);
            painter.drawRect(xmin, ymin, xmax-xmin, ymax-ymin);
            painter.setPen(Qt::red);
            painter.drawRect(xmin-3, ymin-3, xmax-xmin+6, ymax-ymin+6);
        }
        // draw all candidate scalebar
        //painter.setPen(Qt::green);
        //for (size_t n = 0; n < (*scalebar_list).size(); n++) {
        //    INT4 bbox = (*scalebar_list)[n];
        //    int xmin = bbox.x * ratio;
        //    int ymin = bbox.y * ratio;
        //    int xmax = bbox.z * ratio;
        //    int ymax = bbox.w * ratio;
        //    painter.drawRect(xmin, ymin, xmax - xmin, ymax - ymin);
        //}
    }

    // scale bar manual mode
    if (m_MainWindow->m_ScaleBarMode == 1 || m_MainWindow->m_ScaleBarMode == 2) {
        int px = this->width()/2;
        int py = m_Image.height()/2;

        painter.setPen(Qt::black);
        painter.setBrush(Qt::yellow);
        painter.drawRect(px-250, py+0, 500, 32);
        painter.drawText(px-150, py+10, 350, 24, 0, tr("  Drag mouse to select scale bar region !"));

        if (m_MainWindow->m_ScaleBarMode == 2) {
            painter.setPen(Qt::green);
            painter.setBrush(Qt::NoBrush);
            int xmin = min(m_MainWindow->m_ScaleBarBox.left(), m_MainWindow->m_ScaleBarBox.right());
            int xmax = max(m_MainWindow->m_ScaleBarBox.left(), m_MainWindow->m_ScaleBarBox.right());
            int ymin = min(m_MainWindow->m_ScaleBarBox.top(), m_MainWindow->m_ScaleBarBox.bottom());
            int ymax = max(m_MainWindow->m_ScaleBarBox.top(), m_MainWindow->m_ScaleBarBox.bottom());
            painter.drawRect(xmin, ymin, xmax-xmin, ymax-ymin);
        }
    }

    // select mode
    if (m_MainWindow->m_SelectMode == 2) {
        painter.setPen(Qt::green);
        painter.setBrush(Qt::NoBrush);
        int xmin = min(m_MainWindow->m_SelectBox.left(), m_MainWindow->m_SelectBox.right());
        int xmax = max(m_MainWindow->m_SelectBox.left(), m_MainWindow->m_SelectBox.right());
        int ymin = min(m_MainWindow->m_SelectBox.top(), m_MainWindow->m_SelectBox.bottom());
        int ymax = max(m_MainWindow->m_SelectBox.top(), m_MainWindow->m_SelectBox.bottom());
        painter.drawRect(xmin, ymin, xmax-xmin, ymax-ymin);
    }
}

void MainView::resizeEvent(QResizeEvent *event)
{
    this->setImage();
    QWidget::resizeEvent(event);
}
