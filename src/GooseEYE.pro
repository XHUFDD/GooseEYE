
QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET    = GooseEYE
TEMPLATE  = app

DEFINES  += QT_DEPRECATED_WARNINGS

SOURCES  += gui_main.cpp\
            gui_mainwindow.cpp

HEADERS  += gui_mainwindow.h

FORMS    += gui_mainwindow.ui
