//******************************************************************************
// Copyright 2019-2020 Lawrence Livermore National Security, LLC and other
// LIST Project Developers. See the LICENSE file for details.
// SPDX-License-Identifier: MIT
//
// LIvermore Sem image Tools (LIST)
// Configuration window class (.h, .cpp)
//*****************************************************************************/

#ifndef CONFIGWINDOW_H
#define CONFIGWINDOW_H

#include <QWidget>
#include <QDialog>

class MainWindow;
struct AppConfig;

class QLineEdit;
class QPushButton;
class QCheckBox;
class QComboBox;



class ConfigWindow : public QDialog
{
    Q_OBJECT

public:
    ConfigWindow(QWidget *parent, QWidget *main_window);
    ~ConfigWindow();

    QSize minimumSizeHint() const;
    QSize sizeHint() const;

    void createUI();
    void setUI(AppConfig* config);
    void getUI(AppConfig* config);

private slots:
    void onOutdirButton();
    void onScaleEastButton();
    void onScaleTesseractButton();

public:
    MainWindow*     m_MainWindow;

    QCheckBox*      m_OutdirRelativeCheck;
    QLineEdit*      m_OutdirEdit;
    QPushButton*    m_OutdirButton;

    QCheckBox*      m_ScaleUseCheck;
    QLineEdit*      m_ScaleNumberEdit;
    QComboBox*      m_ScaleUnitCombo;
    QLineEdit*      m_ScaleEastEdit;
    QPushButton*    m_ScaleEastButton;
    QLineEdit*      m_ScaleTesseractEdit;
    QPushButton*    m_ScaleTesseractButton;

    QCheckBox*      m_OutlierRemovalCheck;
    QLineEdit*      m_OutlierThresEdit;

};



#endif // CONFIGWINDOW_H
