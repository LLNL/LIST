//******************************************************************************
// Copyright 2019-2020 Lawrence Livermore National Security, LLC and other
// LIST Project Developers. See the LICENSE file for details.
// SPDX-License-Identifier: MIT
//
// LIvermore Sem image Tools (LIST)
// Main window class (.h, .cpp)
//*****************************************************************************/

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QWidget>

#define APP_CAPTION         "Livermore SEM-TEM Image Tools - LIST"


class MainView;
class HistWindow;
class ConfigWindow;
class QApplication;
class QToolBar;
class QLabel;
class QPushButton;
class QRadioButton;
class QLineEdit;
class QComboBox;
class QListWidget;
class QSpinBox;
class QCheckBox;
class QTableWidget;
class QAction;
class QActionGroup;

class SEMShape;
class SEMScaleBar;

QT_BEGIN_NAMESPACE
class QSlider;
QT_END_NAMESPACE



struct AppConfig
{
    AppConfig()
    {
        DataDir = ".";
        OutDir_UseRelative = true;
        OutDir = ".";
        Scale_UseDefault = false;
        Scale_DefaultNumber = 100;
        Scale_DefaultUnit = 1;
        Scale_EASTDetectorPath = "../Resources/frozen_east_text_detection.pb";
        Scale_TesseractDataPath = "../Resources";
        Outlier_AutoRemoval = true;
        Outlier_StdevThreshold = 2;
    }

    QString DataDir;

    bool OutDir_UseRelative;
    QString OutDir;

    bool Scale_UseDefault;
    int Scale_DefaultNumber;
    int Scale_DefaultUnit;
    QString Scale_EASTDetectorPath;
    QString Scale_TesseractDataPath;

    bool Outlier_AutoRemoval;
    float Outlier_StdevThreshold;

};



class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = NULL, QApplication* app = NULL, QString init_dir="");
    ~MainWindow();

    void createUI();

protected:
    void keyPressEvent(QKeyEvent *event);

private slots:
    void onSetDir();
    void onRunDir();
    void onConfig();
    void onOpenFile();
    void onRunFile();
    void onScalebarMeasure();
    void onAutoMeasureCheck();
    void onSizeMeasure();

    void onImageVisible();
    void onScalebarVisible();
    void onShapeCenterVisible();
    void onShapeSizeVisible();
    void onShapeTableSelected(int row, int col);

    void onClearSelected();
    void onRemoveSelected();
    void onFilterOutlier();
    void onShowPlot();

public:
    void setWorkDir();
    void setShapeTable();
    void selectShapeTable();
    void saveOutput();
    void loadIni();
    void saveIni();

public:
    QApplication*   m_MainApp;
    MainView*       m_MainView;
    HistWindow*     m_HistWindow;
    ConfigWindow*   m_ConfigDialog;

    QLineEdit*      m_DirectoryEdit;
    QPushButton*    m_DirectorySetButton;
    QPushButton*    m_DirectoryRunButton;
    QPushButton*    m_ConfigButton;

    QListWidget*    m_FileList;
    QPushButton*    m_OpenButton;
    QLabel*         m_FileNameLabel;
    QLabel*         m_FileInfoLabel;
    QPushButton*    m_RunButton;

    QPushButton*    m_ScalebarMeasureutton;
    QLineEdit*      m_ScalebarLengthEdit;
    QLineEdit*      m_ScalebarNumberEdit;
    QComboBox*      m_ScalebarUnitCombo;

    QPushButton*    m_MeasureButton;
    QCheckBox*      m_AutoMeasureCheck;
    QComboBox*      m_MorphCombo;
    QComboBox*      m_BinInvertCombo;
    QLineEdit*      m_BinThresholdEdit;
    QLineEdit*      m_RGThresholdEdit;

    QRadioButton*   m_ImageRadio1;
    QRadioButton*   m_ImageRadio2;
    QRadioButton*   m_ImageRadio3;
    QRadioButton*   m_ImageRadio4;
    QRadioButton*   m_ImageRadio5;
    QCheckBox*      m_ScalebarVisible;
    QCheckBox*      m_CenterVisible;
    QCheckBox*      m_SizeVisible;

    QPushButton*    m_ClearButton;
    QPushButton*    m_RemoveButton;
    QPushButton*    m_OutlierButton;
    QPushButton*    m_PlotButton;
    QTableWidget*   m_ShapeTable;

    AppConfig       m_Config;
    QString         m_IniDir;

    SEMShape*       m_SEMShape;
    SEMScaleBar*    m_SEMScaleBar;
    int             m_ScaleBarMode; // for manual mode
    QRect           m_ScaleBarBox;
    int             m_SelectMode; // for manual select mode
    QRect           m_SelectBox;

};

#endif // MAINWINDOW_H
