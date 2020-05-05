//******************************************************************************
// Copyright 2019-2020 Lawrence Livermore National Security, LLC and other
// LIST Project Developers. See the LICENSE file for details.
// SPDX-License-Identifier: MIT
//
// LIvermore Sem image Tools (LIST)
// QT Project created by QtCreator
//*****************************************************************************/


QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = LIST
TEMPLATE = app
CONFIG += app_bundle
CONFIG += c++11
#CONFIG += static
#CONFIG += staticlib
#static static-runtime


SOURCES += main.cpp\
		mainwindow.cpp\
		mainview.cpp\
		configwindow.cpp\
		histwindow.cpp\
		histview.cpp\
		semproc.cpp\
		semutil.cpp\
		textdetect.cpp \
		segmenter.cpp

HEADERS += mainwindow.h\
		mainview.h\
		configwindow.h\
		histwindow.h\
		histview.h\
		semproc.h\
		semutil.h\
		textdetect.h\
		segmenter.h\
		datatype.h


# OpenCV
DEFINES		+= __LIB_OPENCV
INCLUDEPATH += /usr/local/include/opencv4
DEPENDPATH  += /usr/local/include/opencv4
LIBS        += -L/usr/local/lib
LIBS        += -lopencv_core
LIBS        += -lopencv_imgproc
LIBS		+= -lopencv_imgcodecs
LIBS        += -lopencv_dnn

# Tesseract
INCLUDEPATH += /usr/local/include
DEPENDPATH  += /usr/local/include
LIBS        += -L/usr/local/lib
LIBS        += -ltesseract
#LIBS		+= -ljpeg


