//******************************************************************************
// Copyright 2019-2020 Lawrence Livermore National Security, LLC and other
// LIST Project Developers. See the LICENSE file for details.
// SPDX-License-Identifier: MIT
//
// LIvermore Sem image Tools (LIST)
// Histogram view class (single histogram) (.h, .cpp)
//*****************************************************************************/

#ifndef HISTVIEW_H
#define HISTVIEW_H

#include <QWidget>
#include <QDialog>
#include <QString>
#include "datatype.h"

class MainWindow;


struct HistBin
{
    HistBin()
    {
        ValueMin = 0; ValueMax = 0; ValueCount = 0;
        XMin = 0; XMax = 0; YMin = 0; YMax = 0; Selected = 0;
    }
    float   ValueMin;
    float   ValueMax;
    int     ValueCount;
    int     XMin;
    int     XMax;
    int     YMin;
    int     YMax;
    int     Selected;
};


struct Histogram
{
    Histogram()
    {
        this->init();
    }
    void init()
    {
        DataReady = false;
        DataMin = 0; DataMax = 0;
        BinSize = 0; BinWidth = 0; HistMax = 0;
        Mean = 0; Stdev = 0;
        ImageWidth = 600; ImageHeight = 400;
        AxisXStart = 50; AxisXEnd = ImageWidth - 30;
        AxisYStart = 30; AxisYEnd = ImageHeight - 50;
        PlotXStart = AxisXStart + 10; PlotXEnd = AxisXEnd - 10;
        PlotWidth = PlotXEnd - PlotXStart; PlotHeight = AxisYEnd - AxisYStart - 10;
        Bins.clear();
    }
    bool setup(int dataMin, int dataMax, int binSize, std::vector<float>& dataList, int& histMax)
    {
        DataReady = false;
        DataMin = dataMin;
        DataMax = dataMax;
        BinSize = binSize;
        BinWidth = (int)((float)PlotWidth / BinSize);
        Bins.resize(BinSize);
        if (DataMin == 0 && DataMax == 0)
            return false;
        if (dataList.size() <= 1)
            return false;

        // initialize bins
        for (int n = 0; n < BinSize; n++) {
            Bins[n].ValueMin = ((float)(n+0) / BinSize) * (DataMax-DataMin) + DataMin;
            Bins[n].ValueMax = ((float)(n+1) / BinSize) * (DataMax-DataMin) + DataMin;
            Bins[n].ValueCount = 0;
            Bins[n].Selected = 0;
        }
        // assign each value to histogram bin, also compute mean and stdev
        Mean = 0;
        Stdev = 0;
        for (size_t n = 0; n < dataList.size(); n++) {
            float size = (float)(dataList[n]-DataMin) / (DataMax-DataMin); // normalized size
            int bin_ind = size * BinSize; // do not use (BinSize-1) because data_max is always greater than the actual maxium
            if (bin_ind < 0 || bin_ind >= BinSize) {
                printf("out of bound histogram\n");
            }
            Bins[bin_ind].ValueCount++;
            Mean += dataList[n];
        }
        Mean /= dataList.size();
        for (size_t n = 0; n < dataList.size(); n++)
            Stdev += ((dataList[n] - Mean) * (dataList[n] - Mean));
        Stdev = sqrt(Stdev / (dataList.size() - 1));

        // get maximum count (frequency) across the bins
        if (histMax == 0) {
            for (int n = 0; n < BinSize; n++)
                histMax = __MAX(histMax, Bins[n].ValueCount);
        }
        HistMax = histMax;
        // setup histogram bar
        for (int n = 0; n < BinSize; n++) {
            int hist_height = (int)((float)Bins[n].ValueCount / HistMax * PlotHeight);
            Bins[n].XMin = PlotXStart + BinWidth*(n+0);
            Bins[n].XMax = PlotXStart + BinWidth*(n+1);
            Bins[n].YMin = AxisYEnd - hist_height;
            Bins[n].YMax = AxisYEnd;
        }
        DataReady = true;
        return true;
    }
    void unselectAll()
    {
        if (!DataReady)
            return;
        for (int n = 0; n < BinSize; n++)
            Bins[n].Selected = 0;
    }
    void getXAxis(QLine& xAxis, std::vector<QPoint>& xTicks, std::vector<QString>& xLabels)
    {
        xAxis.setLine(AxisXStart, AxisYEnd, AxisXEnd, AxisYEnd);
        for (int n = 0; n < BinSize; n++) {
            int x = PlotXStart + BinWidth * n + BinWidth/2;
            float size = (Bins[n].ValueMin + Bins[n].ValueMax) / 2;
            xTicks.push_back(QPoint(x, AxisYEnd));
            xLabels.push_back(QString::number((int)size));
        }
    }   
    void getYAxis(int TickCount, QLine& yAxis, std::vector<QPoint>& yTicks, std::vector<QString>& yLabels)
    {
        yAxis.setLine(AxisXStart, AxisYStart, AxisXStart, AxisYEnd);
        for (int n = 1; n < TickCount; n++) {
            int hist_count = ((float)n / TickCount * HistMax);
            int y = AxisYEnd - ((float)n / TickCount * PlotHeight);
            yTicks.push_back(QPoint(AxisXStart, y));
            yLabels.push_back(QString::number(hist_count));
        }
    }
    QString getText()
    {
        QString text = QString::number(Mean, 'f', 1) + QString(" ± ") + QString::number(Stdev, 'f', 1);
        //printf(text.toStdString().c_str());
        return text;
    }

    bool    DataReady;
    int     DataMin;
    int     DataMax;
    int     BinSize;
    int     BinWidth;
    int     HistMax;
    float   Mean;
    float   Stdev;

    int     ImageWidth;
    int     ImageHeight;
    int     AxisXStart;
    int     AxisXEnd;
    int     AxisYStart;
    int     AxisYEnd;
    int     PlotXStart;
    int     PlotXEnd;
    int     PlotWidth;
    int     PlotHeight;

    std::vector<HistBin>    Bins;
};



class HistView : public QWidget
{
    Q_OBJECT

public:
    HistView(QWidget *parent, QWidget *main_window, int scaleMode, int sizeMode);
    ~HistView();

    QSize minimumSizeHint() const;
    QSize sizeHint() const;

    void setHistogram(std::vector<float>& size_list, int size_min, int size_max, int bin_size, int& hist_max);
    void clearHistogram();
    void setImage();
    void saveImage(QString outPath);
    void selectOutliers(float stdevThreshold);

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void paintEvent(QPaintEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private:
    QPoint              m_MousePressed;
    QImage              m_HistImage; // original one
    QImage              m_Image; // resized one to screen

public:
    MainWindow*         m_MainWindow;
    Histogram           m_Histogram;

    int                 m_ScaleMode; // 0: Core, 1: Shell
    int                 m_SizeMode;  // 0: Short, 1: Long, 2: Both
    int                 m_SelectMode; // for manual select mode
    QRect               m_SelectBox;

};



#endif // MAINVIEW_H
