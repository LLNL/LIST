//******************************************************************************
// Copyright 2019-2020 Lawrence Livermore National Security, LLC and other
// LIST Project Developers. See the LICENSE file for details.
// SPDX-License-Identifier: MIT
//
// LIvermore Sem image Tools (LIST)
// Histogram window class (contains multiple histograms) (.h, .cpp)
//*****************************************************************************/

#include <QMouseEvent>
#include <QPaintEvent>
#include <QResizeEvent>
#include <QPainter>
#include <QImage>
#include <QPoint>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGridLayout>
#include <QPushButton>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QComboBox>

#include <stdio.h>
#include <stdlib.h>
#include <iostream>

#include "mainwindow.h"
#include "histwindow.h"
#include "histview.h"
#include "semproc.h"



HistWindow::HistWindow(QWidget *parent, QWidget* main_window) : QDialog(parent)
{
    m_MainWindow = (MainWindow*)main_window;
    this->setMouseTracking(true);
    this->setFocusPolicy(Qt::StrongFocus);
    this->createUI();
}

HistWindow::~HistWindow()
{
    delete m_CoreHistViewS;
    delete m_CoreHistViewL;
    delete m_CoreHistViewA;
    delete m_ShellHistViewS;
    delete m_ShellHistViewL;
    delete m_ShellHistViewA;
}

QSize HistWindow::minimumSizeHint() const
{
    return QSize(1024, 512);
}

QSize HistWindow::sizeHint() const
{
    return QSize(1024, 512);
}

void HistWindow::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
}

void HistWindow::createUI()
{
    m_CoreHistViewS = new HistView(this,  m_MainWindow, 0, 0);
    m_CoreHistViewL = new HistView(this,  m_MainWindow, 0, 1);
    m_CoreHistViewA = new HistView(this,  m_MainWindow, 0, 2);
    m_ShellHistViewS = new HistView(this, m_MainWindow, 1, 0);
    m_ShellHistViewL = new HistView(this, m_MainWindow, 1, 1);
    m_ShellHistViewA = new HistView(this, m_MainWindow, 1, 2);

    m_CoreLabelS = new QLabel("dS = 0");
    m_CoreLabelL = new QLabel("dL = 0");
    m_CoreLabelA = new QLabel("d = 0");
    m_ShellLabelS = new QLabel("dS = 0");
    m_ShellLabelL = new QLabel("dL = 0");
    m_ShellLabelA = new QLabel("d = 0");

    QLabel* label1 = new QLabel(tr("Core"));
    QLabel* label2 = new QLabel(tr("Shell"));
    QGroupBox* gbox11 = new QGroupBox(tr(""));
    QVBoxLayout *gbox_layout11 = new QVBoxLayout;
    gbox_layout11->addWidget(m_CoreLabelS);
    gbox11->setLayout(gbox_layout11);
    QGroupBox* gbox12 = new QGroupBox(tr(""));
    QVBoxLayout *gbox_layout12 = new QVBoxLayout;
    gbox_layout12->addWidget(m_CoreLabelL);
    gbox12->setLayout(gbox_layout12);
    QGroupBox* gbox13 = new QGroupBox(tr(""));
    QVBoxLayout *gbox_layout13 = new QVBoxLayout;
    gbox_layout13->addWidget(m_CoreLabelA);
    gbox13->setLayout(gbox_layout13);
    QGroupBox* gbox21 = new QGroupBox(tr(""));
    QVBoxLayout *gbox_layout21 = new QVBoxLayout;
    gbox_layout21->addWidget(m_ShellLabelS);
    gbox21->setLayout(gbox_layout21);
    QGroupBox* gbox22 = new QGroupBox(tr(""));
    QVBoxLayout *gbox_layout22 = new QVBoxLayout;
    gbox_layout22->addWidget(m_ShellLabelL);
    gbox22->setLayout(gbox_layout22);
    QGroupBox* gbox23 = new QGroupBox(tr(""));
    QVBoxLayout *gbox_layout23 = new QVBoxLayout;
    gbox_layout23->addWidget(m_ShellLabelA);
    gbox23->setLayout(gbox_layout23);
    QPushButton* close_button = new QPushButton(tr("Close"));
    connect(close_button, SIGNAL(clicked()), this, SLOT(close()));

    QGridLayout* mainlayout = new QGridLayout;
    mainlayout->addWidget(label1, 0, 0);
    mainlayout->addWidget(m_CoreHistViewS, 0, 1);
    mainlayout->addWidget(m_CoreHistViewL, 0, 2);
    mainlayout->addWidget(m_CoreHistViewA, 0, 3);
    mainlayout->addWidget(gbox11, 1, 1);
    mainlayout->addWidget(gbox12, 1, 2);
    mainlayout->addWidget(gbox13, 1, 3);
    mainlayout->addWidget(label2, 2, 0);
    mainlayout->addWidget(m_ShellHistViewS, 2, 1);
    mainlayout->addWidget(m_ShellHistViewL, 2, 2);
    mainlayout->addWidget(m_ShellHistViewA, 2, 3);
    mainlayout->addWidget(gbox21, 3, 1);
    mainlayout->addWidget(gbox22, 3, 2);
    mainlayout->addWidget(gbox23, 3, 3);
    mainlayout->addWidget(close_button, 4, 3);

    this->setLayout(mainlayout);
}

void HistWindow::setHistogramAll()
{
    std::vector<float> csize_list_S;
    std::vector<float> csize_list_L;
    std::vector<float> csize_list_A;
    std::vector<float> ssize_list_S;
    std::vector<float> ssize_list_L;
    std::vector<float> ssize_list_A;
    int bin_size = 10;

    int slength = m_MainWindow->m_ScalebarLengthEdit->text().toInt();
    int snumber = m_MainWindow->m_ScalebarNumberEdit->text().toInt();
    int sunit = m_MainWindow->m_ScalebarUnitCombo->currentIndex();
    QString unit_str = m_MainWindow->m_ScalebarUnitCombo->currentText();

    std::vector<TShapeInfo>* shape_list = m_MainWindow->m_SEMShape->getShapeList();
    if (shape_list->size() == 0)
        return;
    for (size_t n = 0; n < shape_list->size(); n++) {
        TShapeInfo sinfo = (*shape_list)[n];

        float csize_S = SEMScaleBar::convert(sinfo.CoreSizeS, slength, snumber, sunit);
        float csize_L = SEMScaleBar::convert(sinfo.CoreSizeL, slength, snumber, sunit);
        float ssize_S = SEMScaleBar::convert(sinfo.ShellSizeS, slength, snumber, sunit);
        float ssize_L = SEMScaleBar::convert(sinfo.ShellSizeL, slength, snumber, sunit);

        csize_list_S.push_back(csize_S);
        csize_list_L.push_back(csize_L);
        csize_list_A.push_back(csize_S);
        csize_list_A.push_back(csize_L);

        ssize_list_S.push_back(ssize_S);
        ssize_list_L.push_back(ssize_L);
        ssize_list_A.push_back(ssize_S);
        ssize_list_A.push_back(ssize_L);
    }

    // get min, max
    int csize_min = 0;
    int csize_max = 0;
    if (csize_list_A.size() >= 0) {
        std::sort(csize_list_A.begin(), csize_list_A.end());
        float csize_min_f = csize_list_A[0];
        float csize_max_f = csize_list_A[csize_list_A.size()-1];
        //printf("csize_min_f=%f, csize_max_f=%f\n", csize_min_f, csize_max_f);
        if (csize_max_f > 1e-4) { // only if min, max are valid numbers
            csize_min = (int)floor(csize_min_f);
            csize_max = (int)ceil(csize_max_f + 1e-4); // in case max is an integer
        }
    }

    int ssize_min = 0;
    int ssize_max = 0;
    if (ssize_list_A.size() >= 0) {
        std::sort(ssize_list_A.begin(), ssize_list_A.end());
        float ssize_min_f = ssize_list_A[0];
        float ssize_max_f = ssize_list_A[ssize_list_A.size()-1];
        if (ssize_max_f > 1e-4) { // only if min, max are valid numbers
            ssize_min = (int)floor(ssize_min_f);
            ssize_max = (int)ceil(ssize_max_f + 1e-4); // in case max is an integer
        }
    }

    int hist_max = 0;
    m_CoreHistViewA->setHistogram(csize_list_A, csize_min, csize_max, bin_size, hist_max);
    m_CoreHistViewS->setHistogram(csize_list_S, csize_min, csize_max, bin_size, hist_max);
    m_CoreHistViewL->setHistogram(csize_list_L, csize_min, csize_max, bin_size, hist_max);
    hist_max = 0;
    m_ShellHistViewA->setHistogram(ssize_list_A, ssize_min, ssize_max, bin_size, hist_max);
    m_ShellHistViewS->setHistogram(ssize_list_S, ssize_min, ssize_max, bin_size, hist_max);
    m_ShellHistViewL->setHistogram(ssize_list_L, ssize_min, ssize_max, bin_size, hist_max);

    m_CoreLabelA->setText("d = " + m_CoreHistViewA->m_Histogram.getText() + " " + unit_str);
    m_CoreLabelS->setText("dS = " + m_CoreHistViewS->m_Histogram.getText() + " " + unit_str);
    m_CoreLabelL->setText("dL = " + m_CoreHistViewL->m_Histogram.getText() + " " + unit_str);
    m_ShellLabelA->setText("d = " + m_ShellHistViewA->m_Histogram.getText() + " " + unit_str);
    m_ShellLabelS->setText("dS = " + m_ShellHistViewS->m_Histogram.getText() + " " + unit_str);
    m_ShellLabelL->setText("dL = " + m_ShellHistViewL->m_Histogram.getText() + " " + unit_str);

    printf("csize_min=%d, csize_max=%d, ssize_min=%d, ssize_max=%d\n", csize_min, csize_max, ssize_min, ssize_max);
}

void HistWindow::clearHistogramAll()
{
    m_CoreHistViewA->clearHistogram();
    m_CoreHistViewS->clearHistogram();
    m_CoreHistViewL->clearHistogram();
    m_ShellHistViewA->clearHistogram();
    m_ShellHistViewS->clearHistogram();
    m_ShellHistViewL->clearHistogram();

    m_CoreLabelA->setText("d = 0");
    m_CoreLabelS->setText("dS = 0");
    m_CoreLabelL->setText("dL = 0");
    m_ShellLabelA->setText("d = 0");
    m_ShellLabelS->setText("dS = 0");
    m_ShellLabelL->setText("dL = 0");

}

void HistWindow::unselectHistogramAll()
{
    m_CoreHistViewA->m_Histogram.unselectAll();
    m_CoreHistViewS->m_Histogram.unselectAll();
    m_CoreHistViewL->m_Histogram.unselectAll();
    m_ShellHistViewA->m_Histogram.unselectAll();
    m_ShellHistViewS->m_Histogram.unselectAll();
    m_ShellHistViewL->m_Histogram.unselectAll();

    m_CoreHistViewA->setImage();
    m_CoreHistViewS->setImage();
    m_CoreHistViewL->setImage();
    m_ShellHistViewA->setImage();
    m_ShellHistViewS->setImage();
    m_ShellHistViewL->setImage();
}

void HistWindow::saveHistogramAll(QString outDir, QString outPrefix)
{
    QString outPath_prefix = outDir + __DIR_DELIMITER + outPrefix;

    m_CoreHistViewS->saveImage(outPath_prefix + "_histogram_core_dS.png");
    m_CoreHistViewL->saveImage(outPath_prefix + "_histogram_core_dL.png");
    m_CoreHistViewA->saveImage(outPath_prefix + "_histogram_core_d.png");

    m_ShellHistViewS->saveImage(outPath_prefix + "_histogram_shell_dS.png");
    m_ShellHistViewL->saveImage(outPath_prefix + "_histogram_shell_dL.png");
    m_ShellHistViewA->saveImage(outPath_prefix + "_histogram_shell_d.png");
}

void HistWindow::selectOutliers(float stdevThreshold)
{
    m_CoreHistViewS->selectOutliers(stdevThreshold);
    m_CoreHistViewL->selectOutliers(stdevThreshold);
    m_ShellHistViewS->selectOutliers(stdevThreshold);
    m_ShellHistViewL->selectOutliers(stdevThreshold);
}
