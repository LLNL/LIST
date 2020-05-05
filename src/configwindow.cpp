//******************************************************************************
// Copyright 2019-2020 Lawrence Livermore National Security, LLC and other
// LIST Project Developers. See the LICENSE file for details.
// SPDX-License-Identifier: MIT
//
// LIvermore Sem image Tools (LIST)
// Configuration window class (.h, .cpp)
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
#include <QCheckBox>
#include <QDialogButtonBox>
#include <QMessageBox>
#include <QFileDialog>

#include <stdio.h>
#include <stdlib.h>
#include <iostream>

#include "mainwindow.h"
#include "configwindow.h"



ConfigWindow::ConfigWindow(QWidget *parent, QWidget* main_window) : QDialog(parent)
{
    m_MainWindow = (MainWindow*)main_window;
    this->setMouseTracking(true);
    this->setFocusPolicy(Qt::StrongFocus);
    this->createUI();
}

ConfigWindow::~ConfigWindow()
{
}

QSize ConfigWindow::minimumSizeHint() const
{
    return QSize(500, 320);
}

QSize ConfigWindow::sizeHint() const
{
    return QSize(500, 320);
}

void ConfigWindow::createUI()
{
    m_OutdirRelativeCheck = new QCheckBox(tr("Use Relative Path (DataDir_Out)"));
    m_OutdirEdit = new QLineEdit(tr(""));
    m_OutdirButton = new QPushButton(tr("..."));
    connect(m_OutdirButton, SIGNAL(clicked()), this, SLOT(onOutdirButton()));

    m_ScaleUseCheck = new QCheckBox(tr("Use Default"));
    m_ScaleNumberEdit = new QLineEdit(tr(""));
    m_ScaleUnitCombo = new QComboBox();
    m_ScaleUnitCombo->addItem(tr("µm"));
    m_ScaleUnitCombo->addItem(tr("nm"));
    m_ScaleUnitCombo->setCurrentIndex(0);
    m_ScaleEastEdit = new QLineEdit(tr(""));
    m_ScaleEastButton = new QPushButton(tr("..."));
    connect(m_ScaleEastButton, SIGNAL(clicked()), this, SLOT(onScaleEastButton()));
    m_ScaleTesseractEdit = new QLineEdit(tr(""));
    m_ScaleTesseractButton = new QPushButton(tr("..."));
    connect(m_ScaleTesseractButton, SIGNAL(clicked()), this, SLOT(onScaleTesseractButton()));

    m_OutlierRemovalCheck = new QCheckBox(tr("Outlier Removal"));
    m_OutlierThresEdit = new QLineEdit(tr(""));

    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    connect(buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
    connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject()));

    QGridLayout *mainLayout = new QGridLayout(this);
    mainLayout->addWidget(m_OutdirRelativeCheck, 0, 0);
    mainLayout->addWidget(new QLabel(tr("Output Dir:")), 1, 0);
    mainLayout->addWidget(m_OutdirEdit, 1, 1, 1, 3);
    mainLayout->addWidget(m_OutdirButton, 1, 4);
    mainLayout->addWidget(m_ScaleUseCheck, 2, 0);
    mainLayout->addWidget(new QLabel(tr("Default Scale:")), 3, 0);
    mainLayout->addWidget(m_ScaleNumberEdit, 3, 1, 1, 3);
    mainLayout->addWidget(new QLabel(tr("Default Scale Unit:")), 4, 0);
    mainLayout->addWidget(m_ScaleUnitCombo, 4, 1, 1, 3);
    mainLayout->addWidget(new QLabel(tr("EAST Detector:")), 5, 0);
    mainLayout->addWidget(m_ScaleEastEdit, 5, 1, 1, 3);
    mainLayout->addWidget(m_ScaleEastButton, 5, 4);
    mainLayout->addWidget(new QLabel(tr("Tesseract DataPath:")), 6, 0);
    mainLayout->addWidget(m_ScaleTesseractEdit, 6, 1, 1, 3);
    mainLayout->addWidget(m_ScaleTesseractButton, 6, 4);
    mainLayout->addWidget(m_OutlierRemovalCheck, 7, 0);
    mainLayout->addWidget(new QLabel(tr("Stdev Threshold:")), 8, 0);
    mainLayout->addWidget(m_OutlierThresEdit, 8, 1, 1, 3);
    mainLayout->addWidget(buttonBox, 9, 2);
}

void ConfigWindow::setUI(AppConfig* config)
{
    m_OutdirRelativeCheck->setChecked(config->OutDir_UseRelative);
    m_OutdirEdit->setText(config->OutDir);
    m_ScaleUseCheck->setChecked(config->Scale_UseDefault);
    m_ScaleNumberEdit->setText(QString::number(config->Scale_DefaultNumber));
    m_ScaleUnitCombo->setCurrentIndex(config->Scale_DefaultUnit);
    m_ScaleEastEdit->setText(config->Scale_EASTDetectorPath);
    m_ScaleTesseractEdit->setText(config->Scale_TesseractDataPath);
    m_OutlierRemovalCheck->setChecked(config->Outlier_AutoRemoval);
    m_OutlierThresEdit->setText(QString::number(config->Outlier_StdevThreshold));
}

void ConfigWindow::getUI(AppConfig* config)
{
    config->OutDir_UseRelative = m_OutdirRelativeCheck->isChecked();
    config->OutDir = m_OutdirEdit->text();
    config->Scale_UseDefault = m_ScaleUseCheck->isChecked();
    config->Scale_DefaultNumber = atoi(m_ScaleNumberEdit->text().toStdString().c_str());
    config->Scale_DefaultUnit = m_ScaleUnitCombo->currentIndex();
    config->Scale_EASTDetectorPath = m_ScaleEastEdit->text();
    config->Scale_TesseractDataPath = m_ScaleTesseractEdit->text();
    config->Outlier_AutoRemoval = m_OutlierRemovalCheck->isChecked();
    config->Outlier_StdevThreshold = atof(m_OutlierThresEdit->text().toStdString().c_str());
}

void ConfigWindow::onOutdirButton()
{
    QString selectedDir = QFileDialog::getExistingDirectory( \
                this, tr("Select output directory"), m_OutdirEdit->text(), \
                QFileDialog::DontUseNativeDialog | QFileDialog::ReadOnly | QFileDialog::ShowDirsOnly);
    if (selectedDir.isEmpty())
        return;

    m_OutdirEdit->setText(selectedDir);
}

void ConfigWindow::onScaleEastButton()
{
    QString selectedFile = QFileDialog::getOpenFileName( \
                this, tr("Select EAST detector pb file"), m_ScaleEastEdit->text(), \
                tr("TensorFlow saved model files (*.pb)"));
    if (selectedFile.isEmpty())
        return;

    m_ScaleEastEdit->setText(selectedFile);
}

void ConfigWindow::onScaleTesseractButton()
{
    QString selectedFile = QFileDialog::getOpenFileName( \
                this, tr("Select Tesseract trained data path"), m_ScaleTesseractEdit->text(), \
                tr("Tesseract trained data files (*.traineddata)"));
    if (selectedFile.isEmpty())
        return;

    m_ScaleTesseractEdit->setText(selectedFile);
}


