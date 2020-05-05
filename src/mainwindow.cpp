//******************************************************************************
// Copyright 2019-2020 Lawrence Livermore National Security, LLC and other
// LIST Project Developers. See the LICENSE file for details.
//
// SPDX-License-Identifier: MIT
//
// LIvermore Sem image Tools (LIST)
// Main window class (.h, .cpp)
//*****************************************************************************/

#include <QApplication>
#include <QDir>
#include <QFileInfo>
#include <QAction>
#include <QKeyEvent>
#include <QTime>
#include <QFile>
#include <QSettings>
#include <QTextStream>

#include <QToolBar>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QButtonGroup>
#include <QRadioButton>
#include <QPushButton>
#include <QSlider>
#include <QComboBox>
#include <QCheckBox>
#include <QSpinBox>
#include <QListWidget>
#include <QTableWidget>
#include <QHeaderView>

#include <QMessageBox>
#include <QFileDialog>
#include <QInputDialog>

#include "mainwindow.h"
#include "mainview.h"
#include "histwindow.h"
#include "histview.h"
#include "configwindow.h"

#include "semproc.h"


#define MESSAGE_BOX_ERROR(msg)      (QMessageBox::critical(this, APP_CAPTION, msg, QMessageBox::Ok))
#define MESSAGE_BOX_INFO(msg)       (QMessageBox::information(this, APP_CAPTION, msg, QMessageBox::Ok))

inline QString getFullPath(QString path)
{
    return (path.length() > 0 && path[0] == '.') ? \
        (QApplication::applicationDirPath() + __DIR_DELIMITER + path) : (path);
}



MainWindow::MainWindow(QWidget *parent, QApplication* app, QString init_dir)
    : QMainWindow(parent), m_MainApp(app), m_IniDir(init_dir)
{
    m_SEMShape = new SEMShape();
    m_SEMScaleBar = new SEMScaleBar();
    m_ScaleBarMode = 0;
    m_SelectMode = 0;

    this->createUI();
    this->loadIni();
    this->setWindowTitle(tr(APP_CAPTION));
}

MainWindow::~MainWindow()
{
    this->saveIni();

    if (m_SEMShape)
        delete m_SEMShape;
    if (m_SEMScaleBar)
        delete m_SEMScaleBar;
    if (m_MainView)
        delete m_MainView;
    if (m_HistWindow)
        delete m_HistWindow;
    if (m_ConfigDialog)
        delete m_ConfigDialog;
}

void MainWindow::createUI()
{
    m_MainView = new MainView(this);
    m_HistWindow = new HistWindow(this, this);
    m_ConfigDialog = new ConfigWindow(this, this);

    // set up toolbar
    QLabel* directory_label = new QLabel(tr("Working Dir"));
    m_DirectoryEdit = new QLineEdit();
    m_DirectoryEdit->setReadOnly(true);
    m_DirectoryEdit->setText(m_Config.DataDir);
    m_DirectorySetButton = new QPushButton(tr("Set Dir"));
    connect(m_DirectorySetButton, SIGNAL(clicked()), this, SLOT(onSetDir()));
    m_DirectoryRunButton = new QPushButton(tr("Run Dir"));
    connect(m_DirectoryRunButton, SIGNAL(clicked()), this, SLOT(onRunDir()));
    m_ConfigButton = new QPushButton(tr("Preference"));
    connect(m_ConfigButton, SIGNAL(clicked()), this, SLOT(onConfig()));
    QToolBar* toolbar = new QToolBar();
    toolbar->addWidget(directory_label);
    toolbar->addWidget(m_DirectoryEdit);
    toolbar->addWidget(m_DirectorySetButton);
    toolbar->addWidget(m_DirectoryRunButton);
    toolbar->addWidget(m_ConfigButton);
    this->addToolBar(toolbar);

    // setup toolbox
    // file list
    m_FileList = new QListWidget();
    m_FileList->setMinimumSize(240, 120);
    m_FileList->setMaximumSize(240, 200);
    this->setWorkDir();

    // file buttons, info
    QGroupBox* file_gbox = new QGroupBox(tr("Image File"));
    QFormLayout* file_layout = new QFormLayout();
    m_OpenButton = new QPushButton(tr("Open"));
    connect(m_OpenButton, SIGNAL(clicked()), this, SLOT(onOpenFile()));
    m_RunButton = new QPushButton(tr("Run"));
    connect(m_RunButton, SIGNAL(clicked()), this, SLOT(onRunFile()));
    QLabel* f_label0 = new QLabel(tr(""));
    QLabel* f_label1 = new QLabel(tr("File Name: "));
    QLabel* f_label2 = new QLabel(tr("Image Dim: "));
    m_FileNameLabel = new QLabel(tr(""));
    m_FileInfoLabel = new QLabel(tr(""));
    file_layout->setSpacing(5);
    file_layout->setMargin(7);
    //file_layout->addRow(m_OpenButton, m_RunButton);
    file_layout->addRow(m_OpenButton, f_label0);
    file_layout->addRow(f_label1, m_FileNameLabel);
    file_layout->addRow(f_label2, m_FileInfoLabel);
    file_gbox->setLayout(file_layout);


    // scalebar detection
    QGroupBox* scalebar_gbox = new QGroupBox(tr("Scale Bar"));
    //scalebar_gbox->setStyleSheet(QStringLiteral("QGroupBox{border:2px solid gray;border-radius:5px;margin-top: 1ex;} QGroupBox::title{subcontrol-origin: margin;subcontrol-position:top center;padding:0 3px;}"));
    QFormLayout* scalebar_layout = new QFormLayout();
    QLabel* sb_label1 = new QLabel(tr(""));
    QLabel* sb_label2 = new QLabel(tr("Pixel Length"));
    QLabel* sb_label3 = new QLabel(tr("Number"));
    QLabel* sb_label4 = new QLabel(tr("Unit"));
    m_ScalebarMeasureutton = new QPushButton(tr("Select"));
    m_ScalebarMeasureutton->setMinimumWidth(110);
    m_ScalebarMeasureutton->setMaximumWidth(110);
    connect(m_ScalebarMeasureutton, SIGNAL(clicked()), this, SLOT(onScalebarMeasure()));
    m_ScalebarLengthEdit = new QLineEdit(tr("0"));
    m_ScalebarLengthEdit->setMinimumWidth(95);
    m_ScalebarLengthEdit->setMaximumWidth(95);
    m_ScalebarLengthEdit->setAlignment(Qt::AlignRight);
    m_ScalebarNumberEdit = new QLineEdit(tr("0"));
    m_ScalebarNumberEdit->setMinimumWidth(95);
    m_ScalebarNumberEdit->setMaximumWidth(95);
    m_ScalebarNumberEdit->setAlignment(Qt::AlignRight);
    m_ScalebarUnitCombo = new QComboBox();
    m_ScalebarUnitCombo->addItem(tr("µm"));
    m_ScalebarUnitCombo->addItem(tr("nm"));
    m_ScalebarUnitCombo->setCurrentIndex(0);
    m_ScalebarUnitCombo->setMinimumWidth(100);
    m_ScalebarUnitCombo->setMaximumWidth(100);
    scalebar_layout->setSpacing(5);
    scalebar_layout->setMargin(7);
    scalebar_layout->addRow(sb_label1, m_ScalebarMeasureutton);
    scalebar_layout->addRow(sb_label2, m_ScalebarLengthEdit);
    scalebar_layout->addRow(sb_label3, m_ScalebarNumberEdit);
    scalebar_layout->addRow(sb_label4, m_ScalebarUnitCombo);
    scalebar_gbox->setLayout(scalebar_layout);


    // size measurement (with center detection)
    QGroupBox* size_gbox = new QGroupBox(tr("Size Measurement"));
    QFormLayout *size_layout = new QFormLayout();
    QLabel* c_label1 = new QLabel(tr(""));
    //QLabel* c_label2 = new QLabel(tr(""));
    QLabel* c_label3 = new QLabel(tr(""));
    QLabel* c_label4 = new QLabel(tr("Morphology: "));
    QLabel* c_label5 = new QLabel(tr("Bin invert:"));
    QLabel* c_label6 = new QLabel(tr("Bin threshold:"));
    QLabel* c_label7 = new QLabel(tr("RG threshold:"));
    m_MeasureButton = new QPushButton(tr("Measure"));
    m_MeasureButton->setMinimumWidth(110);
    m_MeasureButton->setMaximumWidth(110);
    connect(m_MeasureButton, SIGNAL(clicked()), this, SLOT(onSizeMeasure()));
    m_AutoMeasureCheck = new QCheckBox(tr("Auto-Measure"));
    m_AutoMeasureCheck->setCheckState(Qt::Checked);
    connect(m_AutoMeasureCheck, SIGNAL(clicked()), this, SLOT(onAutoMeasureCheck()));
    m_MorphCombo = new QComboBox();
    m_MorphCombo->addItem(tr("Core-Only"));
    m_MorphCombo->addItem(tr("Core-Shell"));
    m_MorphCombo->setCurrentIndex(0);
    m_MorphCombo->setMinimumWidth(100);
    m_MorphCombo->setMaximumWidth(100);
    m_MorphCombo->setEnabled(false);
    m_BinInvertCombo = new QComboBox();
    m_BinInvertCombo->addItem(tr("No invert"));
    m_BinInvertCombo->addItem(tr("Invert"));
    m_BinInvertCombo->setCurrentIndex(0);
    m_BinInvertCombo->setMinimumWidth(100);
    m_BinInvertCombo->setMaximumWidth(100);
    m_BinInvertCombo->setEnabled(false);
    m_BinThresholdEdit = new QLineEdit(tr("-1"));
    m_BinThresholdEdit->setMinimumWidth(95);
    m_BinThresholdEdit->setMaximumWidth(95);
    m_BinThresholdEdit->setAlignment(Qt::AlignRight);
    m_BinThresholdEdit->setEnabled(false);
    m_RGThresholdEdit = new QLineEdit(tr("50"));
    m_RGThresholdEdit->setMinimumWidth(95);
    m_RGThresholdEdit->setMaximumWidth(95);
    m_RGThresholdEdit->setAlignment(Qt::AlignRight);
    m_RGThresholdEdit->setEnabled(false);
    size_layout->setSpacing(5);
    size_layout->setMargin(7);
    size_layout->addRow(c_label1, m_MeasureButton);
    size_layout->addRow(c_label3, m_AutoMeasureCheck);
    size_layout->addRow(c_label4, m_MorphCombo);
    size_layout->addRow(c_label5, m_BinInvertCombo);
    size_layout->addRow(c_label6, m_BinThresholdEdit);
    size_layout->addRow(c_label7, m_RGThresholdEdit);
    size_gbox->setLayout(size_layout);


    // visible image group box
    QGroupBox *vis_gbox = new QGroupBox(tr("Visualization"));
    QVBoxLayout *vis_layout = new QVBoxLayout;
    m_ImageRadio1 = new QRadioButton(tr("Input Image"));
    m_ImageRadio1->setChecked(true);
    connect(m_ImageRadio1, SIGNAL(clicked()), this, SLOT(onImageVisible()));
    m_ImageRadio2 = new QRadioButton(tr("Histogram-adj Image"));
    connect(m_ImageRadio2, SIGNAL(clicked()), this, SLOT(onImageVisible()));
    m_ImageRadio3 = new QRadioButton(tr("Binary Image"));
    connect(m_ImageRadio3, SIGNAL(clicked()), this, SLOT(onImageVisible()));
    m_ImageRadio4 = new QRadioButton(tr("Dist-transform Image"));
    connect(m_ImageRadio4, SIGNAL(clicked()), this, SLOT(onImageVisible()));
    m_ImageRadio5 = new QRadioButton(tr("Output Image"));
    connect(m_ImageRadio5, SIGNAL(clicked()), this, SLOT(onImageVisible()));
    m_ScalebarVisible = new QCheckBox(tr("Scale Bar"));
    m_ScalebarVisible->setCheckState(Qt::Unchecked);
    connect(m_ScalebarVisible, SIGNAL(clicked()), this, SLOT(onScalebarVisible()));
    m_CenterVisible = new QCheckBox(tr("Shape Center"));
    m_CenterVisible->setCheckState(Qt::Unchecked);
    connect(m_CenterVisible, SIGNAL(clicked()), this, SLOT(onShapeCenterVisible()));
    m_SizeVisible = new QCheckBox(tr("Shape Size"));
    m_SizeVisible->setCheckState(Qt::Unchecked);
    connect(m_SizeVisible, SIGNAL(clicked()), this, SLOT(onShapeSizeVisible()));
    vis_layout->addWidget(m_ImageRadio1);
    vis_layout->addWidget(m_ImageRadio2);
    vis_layout->addWidget(m_ImageRadio3);
    vis_layout->addWidget(m_ImageRadio4);
    vis_layout->addStretch(1);
    vis_layout->addWidget(m_ScalebarVisible);
    vis_layout->addWidget(m_CenterVisible);
    vis_layout->addWidget(m_SizeVisible);
    vis_gbox->setLayout(vis_layout);

    QWidget* toolwidget = new QWidget();
    QVBoxLayout* toollayout = new QVBoxLayout();
    toollayout->setSpacing(5);
    toollayout->setMargin(7);
    toollayout->addWidget(m_FileList);
    toollayout->addWidget(file_gbox);
    toollayout->addWidget(scalebar_gbox);
    toollayout->addWidget(size_gbox);
    toollayout->addWidget(vis_gbox);
    toolwidget->setLayout(toollayout);


    // bottom post-processing widget
    QWidget *post_widget = new QWidget();
    QHBoxLayout *post_layout = new QHBoxLayout;
    m_ClearButton = new QPushButton(tr("Clear"));
    m_ClearButton->setMinimumWidth(150);
    m_ClearButton->setMaximumWidth(150);
    connect(m_ClearButton, SIGNAL(clicked()), this, SLOT(onClearSelected()));
    m_RemoveButton = new QPushButton(tr("Remove"));
    m_RemoveButton->setMinimumWidth(150);
    m_RemoveButton->setMaximumWidth(150);
    connect(m_RemoveButton, SIGNAL(clicked()), this, SLOT(onRemoveSelected()));
    m_OutlierButton = new QPushButton(tr("Filter..."));
    m_OutlierButton->setMinimumWidth(150);
    m_OutlierButton->setMaximumWidth(150);
    connect(m_OutlierButton, SIGNAL(clicked()), this, SLOT(onFilterOutlier()));
    m_PlotButton = new QPushButton(tr("Histogram..."));
    m_PlotButton->setMinimumWidth(150);
    m_PlotButton->setMaximumWidth(150);
    connect(m_PlotButton, SIGNAL(clicked()), this, SLOT(onShowPlot()));
    post_layout->setSpacing(0);
    post_layout->setMargin(0);
    post_layout->addWidget(m_ClearButton);
    post_layout->addWidget(m_RemoveButton);
    post_layout->addWidget(m_OutlierButton);
    post_layout->addWidget(m_PlotButton);
    post_widget->setLayout(post_layout);


    // bottom shape list table
    m_ShapeTable = new QTableWidget();
    m_ShapeTable->setMinimumHeight(150);
    m_ShapeTable->setColumnCount(9);
    m_ShapeTable->setRowCount(1);
    m_ShapeTable->setColumnWidth(0, 40);
    m_ShapeTable->setColumnWidth(1, 50);
    m_ShapeTable->setColumnWidth(2, 50);
    m_ShapeTable->setColumnWidth(3, 80);
    m_ShapeTable->setColumnWidth(6, 80);
    QStringList table_header;
    table_header << "#" << "X" << "Y" << "Core Type" << "Core SizeS" << "Core SizeL" << "Shell Type" << "Shell SizeS" << "Shell SizeL";
    m_ShapeTable->setHorizontalHeaderLabels(table_header);
    m_ShapeTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_ShapeTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_ShapeTable->setSelectionMode(QAbstractItemView::MultiSelection); // MultiSelection or SingleSelection
    m_ShapeTable->verticalHeader()->setVisible(false);
    connect(m_ShapeTable, SIGNAL(cellClicked(int, int)), this, SLOT(onShapeTableSelected(int, int)));

    QWidget* bottomwidget = new QWidget();
    QVBoxLayout* bottomlayout = new QVBoxLayout();
    bottomlayout->setSpacing(0);
    bottomlayout->setMargin(0);
    bottomlayout->addWidget(post_widget);
    bottomlayout->addWidget(m_ShapeTable);
    bottomwidget->setLayout(bottomlayout);


    // setup center layout (main view and table)
    QWidget* centerwidget = new QWidget();
    QVBoxLayout* centerlayout = new QVBoxLayout;
    centerlayout->addWidget(m_MainView);
    centerlayout->addWidget(bottomwidget);
    centerwidget->setLayout(centerlayout);


    // setup main layout (entire)
    QWidget* mainwidget = new QWidget();
    QHBoxLayout* mainlayout = new QHBoxLayout;
    mainlayout->addWidget(toolwidget);
    mainlayout->addWidget(centerwidget);
    mainwidget->setLayout(mainlayout);

    this->setCentralWidget(mainwidget);
}

void MainWindow::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Escape)
        close();
    else
        QWidget::keyPressEvent(event);
}

void MainWindow::onSetDir()
{
    QString dataDir = getFullPath(m_Config.DataDir);
    QString selectedDir = QFileDialog::getExistingDirectory( \
                this, tr("Select working directory"), dataDir, \
                QFileDialog::DontUseNativeDialog | QFileDialog::ReadOnly | QFileDialog::ShowDirsOnly);
    if (selectedDir.isEmpty() || selectedDir.isNull())
        return;

    //printf("selectedDir= %s\n", selectedDir.toStdString().c_str());

    m_Config.DataDir = selectedDir;
    setWorkDir();
}

void MainWindow::onRunDir()
{
    if (m_FileList->count() == 0) {
        QMessageBox msgBox;
        msgBox.setText("No file exists in this directory");
        msgBox.exec();
        return;
    }

    for (int n = 0; n < m_FileList->count(); n++) {
        m_FileList->setCurrentRow(n);
        m_MainApp->processEvents();

        // open image
        std::string file_path = (getFullPath(m_Config.DataDir) + __DIR_DELIMITER + m_FileList->currentItem()->text()).toStdString();
        if (!m_SEMShape->openImage(file_path.c_str()) || !m_SEMScaleBar->openImage(file_path.c_str()))
            continue;

        // get file info and update UI
        m_FileNameLabel->setText(m_FileList->currentItem()->text());
        std::string image_dim_str = intToString(m_SEMShape->getImage()->getWidth()) + "x" + intToString(m_SEMShape->getImage()->getHeight());
        m_FileInfoLabel->setText(QString::fromStdString(image_dim_str));
        m_MainApp->processEvents();

        // detect scale bar and text
        if (!m_SEMScaleBar->detectScaleBar())
            continue;
        if (m_SEMScaleBar->detectScaleText())
            continue;

        // get detected scale info and update UI
        int slength, snumber, sunit;
        if (!m_SEMScaleBar->getDetectedScale(slength, snumber, sunit))
            continue;
        m_ScalebarLengthEdit->setText(QString::number(slength));
        m_ScalebarNumberEdit->setText(QString::number(snumber));
        m_ScalebarUnitCombo->setCurrentIndex(sunit);
        m_MainApp->processEvents();

        // detect shape
        if (!m_SEMShape->detectShape(true, 0))
            continue;

        // optional outlier removal
        if (m_Config.Outlier_AutoRemoval) {
            m_HistWindow->setHistogramAll();
            m_HistWindow->selectOutliers(m_Config.Outlier_StdevThreshold);
            m_SEMShape->removeSelected();
        }
        // update histogram again and update UI
        m_HistWindow->setHistogramAll();
        m_MainView->setImage();
        m_MainView->repaint();
        m_MainApp->processEvents();

        // save output
        this->saveOutput();
    }
}

void MainWindow::onConfig()
{
    m_ConfigDialog->setUI(&m_Config);
    if (m_ConfigDialog->exec() == QDialog::Accepted) {
        m_ConfigDialog->getUI(&m_Config);
    }
}

void MainWindow::onOpenFile()
{
    if (m_FileList->currentRow() == -1) {
        QMessageBox msgBox;
        msgBox.setText("No file is selected");
        msgBox.exec();
        return;
    }

    // open image file
    std::string file_path = (getFullPath(m_Config.DataDir) + __DIR_DELIMITER + m_FileList->currentItem()->text()).toStdString();
    if (!m_SEMShape->openImage(file_path.c_str()) || !m_SEMScaleBar->openImage(file_path.c_str())) {
        QMessageBox msgBox;
        msgBox.setText("Image file error");
        msgBox.exec();
        return;
    }

    // initialize UI
    m_FileNameLabel->setText(m_FileList->currentItem()->text());
    std::string image_dim_str = intToString(m_SEMShape->getImage()->getWidth()) + "x" + intToString(m_SEMShape->getImage()->getHeight());
    m_FileInfoLabel->setText(QString::fromStdString(image_dim_str));

    m_ScalebarLengthEdit->setText(QString::number(0));
    m_ScalebarNumberEdit->setText(QString::number(0));
    m_ScalebarUnitCombo->setCurrentIndex(0);

    this->setShapeTable();
    m_HistWindow->clearHistogramAll();
    m_CenterVisible->setChecked(false);

    // initialize variable and update screen
    m_ScaleBarMode = 0;
    m_ScaleBarBox = QRect(0, 0, 0, 0);
    m_SelectMode = 0;
    m_SelectBox = QRect(0, 0, 0, 0);

    // detect scalebar
    if (m_SEMScaleBar->detectScaleBar()) {
        if (m_SEMScaleBar->detectScaleText()) {
            // update UI for scalebar
            int slength, snumber, sunit;
            if (m_SEMScaleBar->getDetectedScale(slength, snumber, sunit)) {
                m_ScalebarLengthEdit->setText(QString::number(slength));
                m_ScalebarNumberEdit->setText(QString::number(snumber));
                m_ScalebarUnitCombo->setCurrentIndex(sunit);
                m_ScalebarVisible->setChecked(true);
            }
        }
        else {
            QMessageBox msgBox;
            msgBox.setText("No scale info (number, unit) is detected");
            msgBox.exec();
        }
    }
    else {
        QMessageBox msgBox;
        msgBox.setText("No scale bar segment is detected");
        msgBox.exec();
    }

    // update UI
    m_MainView->setImage();
    m_MainView->repaint();
}

void MainWindow::onRunFile()
{
    // not implemented, not used
}

void MainWindow::onScalebarMeasure()
{
    if (m_SEMScaleBar->getImage()->getWidth() == 0) {
        QMessageBox msgBox;
        msgBox.setText("No image file is opened");
        msgBox.exec();
        return;
    }
    if (m_SelectMode != 0) {
        QMessageBox msgBox;
        msgBox.setText("Center removal mode. Stop removal first");
        msgBox.exec();
        return;
    }

    if (m_ScaleBarMode == 1) {
        m_ScaleBarMode = 0;
    }
    else {
        m_ScaleBarMode = 1;
    }
    m_ScaleBarBox = QRect(0, 0, 0, 0);
    m_MainView->repaint();
}

void MainWindow::onAutoMeasureCheck()
{
    if (m_AutoMeasureCheck->checkState() == Qt::Checked) {
        m_MorphCombo->setEnabled(false);
        m_BinInvertCombo->setEnabled(false);
        m_BinThresholdEdit->setEnabled(false);
        m_RGThresholdEdit->setEnabled(false);
        m_MorphCombo->setCurrentIndex(0);
        m_BinInvertCombo->setCurrentIndex(0);
        m_BinThresholdEdit->setText(tr("-1"));
        m_RGThresholdEdit->setText(tr("50"));
    }
    else {
        m_MorphCombo->setEnabled(true);
        m_BinInvertCombo->setEnabled(true);
        m_BinThresholdEdit->setEnabled(true);
        m_RGThresholdEdit->setEnabled(true);
    }
}

void MainWindow::onSizeMeasure()
{
    if (m_SEMShape->getImage()->getWidth() == 0) {
        QMessageBox msgBox;
        msgBox.setText("No image file is opened");
        msgBox.exec();
        return;
    }

    TShapeSegmenter_Param* param = m_SEMShape->getParam();
    param->bin_inv = m_BinInvertCombo->currentIndex();
    param->bin_threshold = atoi(m_BinThresholdEdit->text().toStdString().c_str());
    param->rg_threshold = atoi(m_RGThresholdEdit->text().toStdString().c_str());

    bool auto_detect = (m_AutoMeasureCheck->checkState() == Qt::Checked) ? true : false;
    int shapetype = m_MorphCombo->currentIndex() + 1;
    if (!m_SEMShape->detectShape(auto_detect, shapetype)) {
        QMessageBox msgBox;
        msgBox.setText("No center is detected");
        msgBox.exec();
        return;
    }

    // optional outlier removal
    if (m_Config.Outlier_AutoRemoval) {
        m_HistWindow->setHistogramAll();
        m_HistWindow->selectOutliers(m_Config.Outlier_StdevThreshold);
        m_SEMShape->removeSelected();
    }

    // update UI
    this->setShapeTable();
    m_CenterVisible->setChecked(true);
    m_MorphCombo->setCurrentIndex(m_SEMShape->getShapeType()-1);
    m_BinInvertCombo->setCurrentIndex(m_SEMShape->getParam()->bin_inv);

    // update view
    m_SelectMode = 0;
    m_MainView->setImage();
    m_MainView->repaint();

    // update histogram again
    m_HistWindow->setHistogramAll();

    // save output
    this->saveOutput();
}

void MainWindow::onImageVisible()
{
    m_MainView->setImage();
    m_MainView->repaint();
}

void MainWindow::onScalebarVisible()
{
    m_MainView->repaint();
}

void MainWindow::onShapeCenterVisible()
{
    m_MainView->repaint();
}

void MainWindow::onShapeSizeVisible()
{
    m_MainView->repaint();
}

void MainWindow::onShapeTableSelected(int row, int col)
{
    if (m_SEMShape->getImage()->getWidth() == 0) {
        QMessageBox msgBox;
        msgBox.setText("No image file is opened");
        msgBox.exec();
        return;
    }
    if (m_SEMShape->getShapeList()->size() == 0)
        return;

    std::vector<TShapeInfo>* shape_list = m_SEMShape->getShapeList();
    if (row < 0 || row >= (int)(*shape_list).size())
        return;

    QTableWidgetItem* item = m_ShapeTable->item(row, col);
    if (item->isSelected())
        (*shape_list)[row].Selected = 1;
    else
        (*shape_list)[row].Selected = 0;

    m_MainView->repaint();
}

void MainWindow::onClearSelected()
{
    if (m_SEMShape->getImage()->getWidth() == 0) {
        QMessageBox msgBox;
        msgBox.setText("No image file is opened");
        msgBox.exec();
        return;
    }
    if (m_SEMShape->getShapeList()->size() == 0)
        return;

    m_SEMShape->clearSelected();
    m_ShapeTable->clearSelection();
    m_HistWindow->unselectHistogramAll();
    m_MainView->repaint();
}

void MainWindow::onRemoveSelected()
{
    if (m_SEMShape->getImage()->getWidth() == 0) {
        QMessageBox msgBox;
        msgBox.setText("No image file is opened");
        msgBox.exec();
        return;
    }
    if (m_SEMShape->getShapeList()->size() == 0)
        return;

    m_SEMShape->removeSelected();

    // update table
    this->setShapeTable();

    // update view
    m_MainView->repaint();

    // update histogram
    m_HistWindow->setHistogramAll();

    // save output
    this->saveOutput();
}

void MainWindow::onFilterOutlier()
{
    QMessageBox msgBox;
    msgBox.setText("This function is not available in this version");
    msgBox.exec();
    return;

    if (m_SEMShape->getImage()->getWidth() == 0) {
        QMessageBox msgBox;
        msgBox.setText("No image file is opened");
        msgBox.exec();
        return;
    }
    if (m_SEMShape->getShapeList()->size() == 0)
        return;

    // update table
    this->setShapeTable();

    // update view
    m_MainView->repaint();

    // update histogram
    m_HistWindow->setHistogramAll();

    // save output
    this->saveOutput();
}

void MainWindow::onShowPlot()
{
    if (m_SEMShape->getImage()->getWidth() == 0) {
        QMessageBox msgBox;
        msgBox.setText("No image file is opened");
        msgBox.exec();
        return;
    }

    Qt::WindowFlags flags = m_HistWindow->windowFlags();
    flags |= Qt::WindowStaysOnTopHint;
    m_HistWindow->setWindowFlags(flags);
    m_HistWindow->show(); // -> modeless
    //m_HistWindow->exec(); // -> modal
}

void MainWindow::setWorkDir()
{
    m_DirectoryEdit->setText(m_Config.DataDir);

    QString fullDir = getFullPath(m_Config.DataDir);
    QDir directory(fullDir);
    QStringList filelist = directory.entryList(QStringList() << "*.png" << "*.PNG" << "*.jpg" << "*.JPG" << "*.jpeg" << "*.JPEG" << "*.tif" << "*.TIF" << "*.tiff" << "*.TIFF", QDir::Files);
    m_FileList->clear();
    for (int n = 0; n < filelist.count(); n++)
        m_FileList->addItem(filelist[n]);
}

void MainWindow::setShapeTable()
{
    // get scale info for conversion
    int slength = m_ScalebarLengthEdit->text().toInt();
    int snumber = m_ScalebarNumberEdit->text().toInt();
    int sunit = m_ScalebarUnitCombo->currentIndex();

    // setup shape table widget
    std::vector<TShapeInfo>* shape_list = m_SEMShape->getShapeList();
    m_ShapeTable->setRowCount(shape_list->size());

    for (size_t n = 0; n < shape_list->size(); n++) {
        TShapeInfo sinfo = (*shape_list)[n];
        float csize_S = SEMScaleBar::convert(sinfo.CoreSizeS, slength, snumber, sunit);
        float csize_L = SEMScaleBar::convert(sinfo.CoreSizeL, slength, snumber, sunit);
        float ssize_S = SEMScaleBar::convert(sinfo.ShellSizeS, slength, snumber, sunit);
        float ssize_L = SEMScaleBar::convert(sinfo.ShellSizeL, slength, snumber, sunit);

        QString item_list[9];
        item_list[0] = QString::number(n);
        item_list[1] = QString::number(sinfo.Center.x);
        item_list[2] = QString::number(sinfo.Center.y);
        item_list[3] = tr("");
        if (csize_S == 0)
            item_list[4] = QString::number(sinfo.CoreSizeS);
        else
            item_list[4] = QString::number(sinfo.CoreSizeS) + tr(" (") + QString::number(csize_S, 'f', 1) + tr(")");
        if (csize_L == 0)
            item_list[5] = QString::number(sinfo.CoreSizeL);
        else
            item_list[5] = QString::number(sinfo.CoreSizeL) + tr(" (") + QString::number(csize_L, 'f', 1) + tr(")");
        item_list[6] = tr("");
        if (ssize_S == 0)
            item_list[7] = QString::number(sinfo.ShellSizeS);
        else
            item_list[7] = QString::number(sinfo.ShellSizeS) + tr(" (") + QString::number(ssize_S, 'f', 1) + tr(")");
        if (ssize_L == 0)
            item_list[8] = QString::number(sinfo.ShellSizeL);
        else
            item_list[8] = QString::number(sinfo.ShellSizeL) + tr(" (") + QString::number(ssize_L, 'f', 1) + tr(")");

        for (int m = 0; m < 9; m++) {
            QTableWidgetItem *cell = m_ShapeTable->item(n, m);
            if (!cell) {
                cell = new QTableWidgetItem;
                m_ShapeTable->setItem(n, m, cell);
            }
            cell->setText(item_list[m]);
        }
    }
    m_ShapeTable->clearSelection();
}

void MainWindow::selectShapeTable()
{
    if (m_SEMShape->getShapeList()->size() == 0)
        return;

    std::vector<TShapeInfo>* shape_list = m_SEMShape->getShapeList();

    //m_ShapeTable->setCurrentCell(n, 0);
    for (size_t n = 0; n < shape_list->size(); n++) {
        QTableWidgetItem* item = m_ShapeTable->item(n, 0);
        if ((*shape_list)[n].Selected == 1) {
            if (!item->isSelected()) // this is important because selectRow acts as toggle!
                m_ShapeTable->selectRow(n);
        }
    }
    m_ShapeTable->setFocus();
}

void MainWindow::saveOutput()
{
    if (m_SEMShape->getShapeList()->size() == 0)
        return;

    // get file name prefix
    std::string file_prefix = m_FileNameLabel->text().toStdString();
    file_prefix = file_prefix.substr(0, file_prefix.length() - 4);
    //printf("file_prefix = %s\n", file_prefix.c_str());

    // get scale info for conversion
    int slength = m_ScalebarLengthEdit->text().toInt();
    int snumber = m_ScalebarNumberEdit->text().toInt();
    int sunit = m_ScalebarUnitCombo->currentIndex();

    // save histogram images
    QString out_dir = getFullPath(m_Config.OutDir);
    m_HistWindow->saveHistogramAll(out_dir, QString::fromStdString(file_prefix));

    // save shape size info into csv/text files
    QString text_path = out_dir + __DIR_DELIMITER + QString::fromStdString(file_prefix) + "_size_list.csv";
    QFile text_file(text_path);
    if (text_file.open(QIODevice::WriteOnly)) {
        std::vector<TShapeInfo>* shape_list = m_SEMShape->getShapeList();
        for (size_t n = 0; n < shape_list->size(); n++) {
            TShapeInfo sinfo = (*shape_list)[n];
            float csize_S = SEMScaleBar::convert(sinfo.CoreSizeS, slength, snumber, sunit);
            float csize_L = SEMScaleBar::convert(sinfo.CoreSizeL, slength, snumber, sunit);
            float ssize_S = SEMScaleBar::convert(sinfo.ShellSizeS, slength, snumber, sunit);
            float ssize_L = SEMScaleBar::convert(sinfo.ShellSizeL, slength, snumber, sunit);

            char line[1024];
            sprintf(line, "%4d %5d %5d %5d %5d %5d %5d   %.2f   %.2f   %.2f   %.2f", (int)n+1, sinfo.Center.x, sinfo.Center.y, \
                    sinfo.CoreSizeS, sinfo.CoreSizeL, sinfo.ShellSizeS, sinfo.ShellSizeL, \
                    csize_S, csize_L, ssize_S, ssize_L);
            QTextStream stream(&text_file);
            stream << line << endl;
        }
    }

    QString text_path2 = out_dir + __DIR_DELIMITER + QString::fromStdString(file_prefix) + "_size_summary.txt";
    QFile text_file2(text_path2);
    if (text_file2.open(QIODevice::WriteOnly)) {
        QTextStream stream(&text_file2);

        char line[1024];
        sprintf(line, "%s\t %s\t %s", m_HistWindow->m_CoreLabelS->text().toStdString().c_str(), \
                m_HistWindow->m_CoreLabelL->text().toStdString().c_str(), \
                m_HistWindow->m_CoreLabelA->text().toStdString().c_str());
        stream << line << endl;
        sprintf(line, "%s\t %s\t %s", m_HistWindow->m_ShellLabelS->text().toStdString().c_str(), \
                m_HistWindow->m_ShellLabelL->text().toStdString().c_str(), \
                m_HistWindow->m_ShellLabelA->text().toStdString().c_str());
        stream << line << endl;
    }
}

void MainWindow::loadIni()
{
    QString iniPath = m_IniDir + __DIR_DELIMITER + "LIST.ini";
    if (!QFile::exists(iniPath))
        return;

    QSettings iniSetting(iniPath, QSettings::IniFormat);

    iniSetting.beginGroup("/Main");
    m_Config.DataDir = iniSetting.value("/DataDir", m_Config.DataDir).toString();
    m_Config.OutDir_UseRelative = iniSetting.value("/UseRelativeOutDir", m_Config.OutDir_UseRelative).toBool();
    m_Config.OutDir = iniSetting.value("/OutDir", m_Config.OutDir).toString();
    iniSetting.endGroup();

    iniSetting.beginGroup("/Scale");
    m_Config.Scale_UseDefault = iniSetting.value("/UseDefault", m_Config.Scale_UseDefault).toBool();
    m_Config.Scale_DefaultNumber = iniSetting.value("/DefaultNumber", m_Config.Scale_DefaultNumber).toInt();
    m_Config.Scale_DefaultUnit = iniSetting.value("/DefaultUnit", m_Config.Scale_DefaultUnit).toInt();
    m_Config.Scale_EASTDetectorPath = iniSetting.value("/EASTDetectorPath", m_Config.Scale_EASTDetectorPath).toString();
    m_Config.Scale_TesseractDataPath = iniSetting.value("/TesseractDataPath", m_Config.Scale_TesseractDataPath).toString();
    iniSetting.endGroup();

    iniSetting.beginGroup("/Outlier");
    m_Config.Outlier_AutoRemoval = iniSetting.value("/AutoRemoval", m_Config.Outlier_AutoRemoval).toBool();
    m_Config.Outlier_StdevThreshold = iniSetting.value("/StdevThreshold", m_Config.Outlier_StdevThreshold).toFloat();
    iniSetting.endGroup();

    // create output directory
    if (m_Config.OutDir_UseRelative)
        m_Config.OutDir = m_Config.DataDir + "_out";
    QString out_dir = getFullPath(m_Config.OutDir);
    QDir().mkdir(out_dir);

    // set data directory to load image files
    this->setWorkDir();

    // load TEXT detector
    std::string east_path = getFullPath(m_Config.Scale_EASTDetectorPath).toStdString();
    if (!m_SEMScaleBar->initTextDetector(east_path.c_str())) {
        QMessageBox msgBox;
        msgBox.setText("EAST text detector file error. Default text search will be used.");
        msgBox.exec();
    }

    // load text recognizer
    std::string tess_path = getFullPath(m_Config.Scale_TesseractDataPath).toStdString();
    if (!m_SEMScaleBar->initTextRecognizer(tess_path.c_str())) {
        QMessageBox msgBox;
        msgBox.setText("Tesseract text recognizer initialization error.");
        msgBox.exec();
    }

}

void MainWindow::saveIni()
{
    QString iniPath = m_IniDir + __DIR_DELIMITER + "LIST.ini";
    QSettings iniSetting(iniPath, QSettings::IniFormat);

    iniSetting.beginGroup("/Main");
    iniSetting.setValue("/DataDir", m_Config.DataDir);
    iniSetting.setValue("/UseRelativeOutDir", m_Config.OutDir_UseRelative);
    iniSetting.setValue("/OutDir", m_Config.OutDir);
    iniSetting.endGroup();

    iniSetting.beginGroup("/Scale");
    iniSetting.setValue("/UseDefault", m_Config.Scale_UseDefault);
    iniSetting.setValue("/DefaultNumber", m_Config.Scale_DefaultNumber);
    iniSetting.setValue("/DefaultUnit", m_Config.Scale_DefaultUnit);
    iniSetting.setValue("/EASTDetectorPath", m_Config.Scale_EASTDetectorPath);
    iniSetting.setValue("/TesseractDataPath", m_Config.Scale_TesseractDataPath);
    iniSetting.endGroup();

    iniSetting.beginGroup("/Outlier");
    iniSetting.setValue("/AutoRemoval", m_Config.Outlier_AutoRemoval);
    iniSetting.setValue("/StdevThreshold", m_Config.Outlier_StdevThreshold);
    iniSetting.endGroup();

    iniSetting.sync();
}

