//******************************************************************************
// Copyright 2019-2020 Lawrence Livermore National Security, LLC and other
// LIST Project Developers. See the LICENSE file for details.
// SPDX-License-Identifier: MIT
//
// LIvermore Sem image Tools (LIST)
// Main application (main.cpp)
//*****************************************************************************/

#include <QApplication>
#include <QMessageBox>
#include <QDir>
#include "mainwindow.h"



int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    QString ini_dir = app.applicationDirPath();
    //printf("current dir: %s\n", ini_dir.toStdString().c_str());

    MainWindow win(NULL, &app, ini_dir);
    win.resize(1024, 768);
    win.show();

    return app.exec();
}
