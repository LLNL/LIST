//******************************************************************************
// Copyright 2019-2020 Lawrence Livermore National Security, LLC and other
// LIST Project Developers. See the LICENSE file for details.
// SPDX-License-Identifier: MIT
//
// LIvermore Sem image Tools (LIST)
// Histogram view class (single histogram) (.h, .cpp)
//*****************************************************************************/

#include <QMouseEvent>
#include <QPaintEvent>
#include <QResizeEvent>
#include <QPainter>
#include <QImage>
#include <QPixmap>
#include <QPoint>
#include <QTableWidget>
#include <QHeaderView>
#include <QLineEdit>
#include <QComboBox>

#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <algorithm>

#include "mainwindow.h"
#include "mainview.h"
#include "histview.h"
#include "semproc.h"



HistView::HistView(QWidget *parent, QWidget* main_window, int scaleMode, int sizeMode) : QWidget(parent), m_ScaleMode(scaleMode), m_SizeMode(sizeMode)
{
    m_MainWindow = (MainWindow*)main_window;
    m_SelectMode = 0;

    this->setMouseTracking(true);
    this->setFocusPolicy(Qt::StrongFocus);
}

HistView::~HistView()
{
}

QSize HistView::minimumSizeHint() const
{
    return QSize(300, 200);
}

QSize HistView::sizeHint() const
{
    return QSize(420, 300);
}

void HistView::setHistogram(std::vector<float>& size_list, int size_min, int size_max, int bin_size, int& hist_max)
{
    // initialize histogram
    if (!m_Histogram.setup(size_min, size_max, bin_size, size_list, hist_max))
        return;
    m_Histogram.unselectAll();

    this->setImage();
}

void HistView::clearHistogram()
{
    m_Histogram.init();
    m_HistImage = QImage(m_Histogram.ImageWidth, m_Histogram.ImageHeight, QImage::Format_ARGB32);
    QPainter painter(&m_HistImage);
    painter.setPen(Qt::white);
    painter.setBrush(Qt::white);
    painter.drawRect(0, 0, m_HistImage.width(), m_HistImage.height());

    m_Image = m_HistImage.scaled(this->width(), this->height(), Qt::KeepAspectRatio);
    this->update();
}

void HistView::setImage()
{
    if (!m_Histogram.DataReady)
        return;

    // setup image
    m_HistImage = QImage(m_Histogram.ImageWidth, m_Histogram.ImageHeight, QImage::Format_ARGB32);
    QPainter painter(&m_HistImage);

    // reset
    painter.setPen(Qt::white);
    painter.setBrush(Qt::white);
    painter.drawRect(0, 0, m_HistImage.width(), m_HistImage.height());

    // get x-axis and y-axis info (including ticks, labels)
    QLine x_axis;
    QLine y_axis;
    std::vector<QPoint> x_ticks;
    std::vector<QPoint> y_ticks;
    std::vector<QString> x_labels;
    std::vector<QString> y_labels;
    m_Histogram.getXAxis(x_axis, x_ticks, x_labels);
    m_Histogram.getYAxis(5, y_axis, y_ticks, y_labels);

    // draw x-axis and y-axis
    painter.setPen(QPen(Qt::black, 3));
    painter.drawLine(x_axis);
    painter.drawLine(y_axis);

    // draw axis ticks and labels
    painter.setPen(QPen(Qt::black, 2));
    QFont font = painter.font();
    font.setPointSize(20);
    painter.setFont(font);
    for (size_t n = 0; n < x_ticks.size(); n++) {
        painter.drawLine(x_ticks[n].x(), x_ticks[n].y(), x_ticks[n].x(), x_ticks[n].y()+7);
        painter.drawText(x_ticks[n].x(), x_ticks[n].y()+30, x_labels[n]);
    }
    for (size_t n = 0; n < y_ticks.size(); n++) {
        painter.drawLine(y_ticks[n].x()-7, y_ticks[n].y(), y_ticks[n].x(), y_ticks[n].y());
        painter.drawText(y_ticks[n].x()-40, y_ticks[n].y()+10, y_labels[n]);
    }

    // draw histogram
    for (int n = 0; n < m_Histogram.BinSize; n++) {
        if (m_Histogram.Bins[n].Selected)
            painter.setBrush(QBrush(Qt::red));
        else
            painter.setBrush(QBrush(Qt::blue));

        painter.drawRect(m_Histogram.Bins[n].XMin, m_Histogram.Bins[n].YMin, \
                         m_Histogram.Bins[n].XMax - m_Histogram.Bins[n].XMin, \
                         m_Histogram.Bins[n].YMax - m_Histogram.Bins[n].YMin);
    }

    // draw mean and stdev
    float mean_n = (float)(m_Histogram.Mean-m_Histogram.DataMin) / (m_Histogram.DataMax-m_Histogram.DataMin);
    float std_n1 = (float)(m_Histogram.Mean-m_Histogram.Stdev-m_Histogram.DataMin) / (m_Histogram.DataMax-m_Histogram.DataMin);
    float std_n2 = (float)(m_Histogram.Mean+m_Histogram.Stdev-m_Histogram.DataMin) / (m_Histogram.DataMax-m_Histogram.DataMin);
    int mean_pos = (int)(m_Histogram.PlotXStart + mean_n * m_Histogram.PlotWidth);
    int stdev_pos1 = (int)(m_Histogram.PlotXStart + std_n1 * m_Histogram.PlotWidth);
    int stdev_pos2 = (int)(m_Histogram.PlotXStart + std_n2 * m_Histogram.PlotWidth);
    painter.setPen(QPen(Qt::red, 3));
    painter.drawLine(mean_pos, m_Histogram.AxisYEnd - m_Histogram.PlotHeight, mean_pos, m_Histogram.AxisYEnd);
    painter.setPen(QPen(Qt::red, 3, Qt::DotLine));
    painter.drawLine(stdev_pos1, m_Histogram.AxisYEnd - m_Histogram.PlotHeight, stdev_pos1, m_Histogram.AxisYEnd);
    painter.drawLine(stdev_pos2, m_Histogram.AxisYEnd - m_Histogram.PlotHeight, stdev_pos2, m_Histogram.AxisYEnd);

    m_Image = m_HistImage.scaled(this->width(), this->height(), Qt::KeepAspectRatio);
    this->update();
}

void HistView::saveImage(QString outPath)
{
    if (!m_Histogram.DataReady)
        return;

    m_HistImage.save(outPath);
}

void HistView::selectOutliers(float stdevThreshold)
{
    if (!m_Histogram.DataReady)
        return;

    // get scalebar info for conversion
    int slength = m_MainWindow->m_ScalebarLengthEdit->text().toInt();
    int snumber = m_MainWindow->m_ScalebarNumberEdit->text().toInt();
    int sunit = m_MainWindow->m_ScalebarUnitCombo->currentIndex();

    float lower_bound = m_Histogram.Mean - m_Histogram.Stdev * stdevThreshold;
    float upper_bound = m_Histogram.Mean + m_Histogram.Stdev * stdevThreshold;

    int pvalue_min = SEMScaleBar::inverse(m_Histogram.DataMin, slength, snumber, sunit);
    int pvalue_max = SEMScaleBar::inverse(lower_bound, slength, snumber, sunit);
    m_MainWindow->m_SEMShape->selectByRange(m_ScaleMode, m_SizeMode, pvalue_min, pvalue_max);

    pvalue_min = SEMScaleBar::inverse(upper_bound, slength, snumber, sunit);
    pvalue_max = SEMScaleBar::inverse(m_Histogram.DataMax, slength, snumber, sunit);
    m_MainWindow->m_SEMShape->selectByRange(m_ScaleMode, m_SizeMode, pvalue_min, pvalue_max);
}

void HistView::mousePressEvent(QMouseEvent *event)
{
    m_MousePressed = event->pos();

    if (m_MainWindow->m_SEMShape->getImage()->getWidth() == 0)
        return;

    if (m_SelectMode == 0) {
        m_SelectMode = 2;
        m_SelectBox.setLeft(event->pos().x());
        m_SelectBox.setTop(event->pos().y());
        m_SelectBox.setRight(event->pos().x());
        m_SelectBox.setBottom(event->pos().y());
    }
}

void HistView::mouseMoveEvent(QMouseEvent *event)
{
    int dx = m_MousePressed.x() - event->x();
    int dy = -(m_MousePressed.y() - event->y());

    if (m_MainWindow->m_SEMShape->getImage()->getWidth() == 0)
        return;

    if (m_SelectMode == 2) {
        m_SelectBox.setRight(event->pos().x());
        m_SelectBox.setBottom(event->pos().y());
        this->update();
    }
}

void HistView::mouseReleaseEvent(QMouseEvent *event)
{
    if (m_MainWindow->m_SEMShape->getImage()->getWidth() == 0)
        return;

    float ratio = (float)m_HistImage.width() / m_Image.width();
    if (m_SelectMode == 2) {
        m_SelectMode = 0;
        if (!m_Histogram.DataReady)
            return;

        int xmin = min(m_SelectBox.left(), m_SelectBox.right()) * ratio;
        int xmax = max(m_SelectBox.left(), m_SelectBox.right()) * ratio;
        int ymin = min(m_SelectBox.top(), m_SelectBox.bottom()) * ratio;
        int ymax = max(m_SelectBox.top(), m_SelectBox.bottom()) * ratio;

        // get scalebar info for conversion
        int slength = m_MainWindow->m_ScalebarLengthEdit->text().toInt();
        int snumber = m_MainWindow->m_ScalebarNumberEdit->text().toInt();
        int sunit = m_MainWindow->m_ScalebarUnitCombo->currentIndex();

        for (int n = 0; n < m_Histogram.BinSize; n++) {
            if (xmin <= m_Histogram.Bins[n].XMin && ymin <= m_Histogram.Bins[n].YMin && \
                xmax >= m_Histogram.Bins[n].XMax && ymax >= m_Histogram.Bins[n].YMax) {
                m_Histogram.Bins[n].Selected = 1;

                int pvalue_min = SEMScaleBar::inverse(m_Histogram.Bins[n].ValueMin, slength, snumber, sunit);
                int pvalue_max = SEMScaleBar::inverse(m_Histogram.Bins[n].ValueMax, slength, snumber, sunit);
                m_MainWindow->m_SEMShape->selectByRange(m_ScaleMode, m_SizeMode, pvalue_min, pvalue_max);
            }
        }
        this->setImage();
        this->update();
        m_MainWindow->selectShapeTable();
        m_MainWindow->m_MainView->repaint();
    }
}

void HistView::paintEvent(QPaintEvent *event)
{
    QPainter painter(this);

    painter.setBrush(Qt::white);
    painter.drawRect(0, 0, this->width(), this->height());

    // draw histogram
    if (!m_Image.isNull())
        painter.drawImage(QPoint(0, 0), m_Image);

    // draw selected rectangle
    if (m_SelectMode == 2) {
        painter.setPen(Qt::green);
        painter.setBrush(Qt::NoBrush);
        int xmin = min(m_SelectBox.left(), m_SelectBox.right());
        int xmax = max(m_SelectBox.left(), m_SelectBox.right());
        int ymin = min(m_SelectBox.top(), m_SelectBox.bottom());
        int ymax = max(m_SelectBox.top(), m_SelectBox.bottom());
        painter.drawRect(xmin, ymin, xmax-xmin, ymax-ymin);
    }

}

void HistView::resizeEvent(QResizeEvent *event)
{
    this->setImage();
    QWidget::resizeEvent(event);
}


