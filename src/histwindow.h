//******************************************************************************
// Copyright 2019-2020 Lawrence Livermore National Security, LLC and other
// LIST Project Developers. See the LICENSE file for details.
// SPDX-License-Identifier: MIT
//
// LIvermore Sem image Tools (LIST)
// Histogram window class (contains multiple histograms) (.h, .cpp)
//*****************************************************************************/

#ifndef HISTWINDOW_H
#define HISTWINDOW_H

#include <QWidget>
#include <QDialog>

class QLabel;
class MainWindow;
class HistView;


class HistWindow : public QDialog
{
    Q_OBJECT

public:
    HistWindow(QWidget *parent, QWidget *main_window);
    ~HistWindow();

    QSize minimumSizeHint() const;
    QSize sizeHint() const;

    void createUI();
    void setHistogramAll();
    void clearHistogramAll();
    void unselectHistogramAll();
    void saveHistogramAll(QString outDir, QString outPrefix);
    void selectOutliers(float stdevThreshold);

protected:
    void resizeEvent(QResizeEvent *event) override;

public:
    MainWindow*         m_MainWindow;
    HistView*           m_CoreHistViewS;
    HistView*           m_CoreHistViewL;
    HistView*           m_CoreHistViewA;
    HistView*           m_ShellHistViewS;
    HistView*           m_ShellHistViewL;
    HistView*           m_ShellHistViewA;
    QLabel*             m_CoreLabelS;
    QLabel*             m_CoreLabelL;
    QLabel*             m_CoreLabelA;
    QLabel*             m_ShellLabelS;
    QLabel*             m_ShellLabelL;
    QLabel*             m_ShellLabelA;

};



#endif // MAINVIEW_H
