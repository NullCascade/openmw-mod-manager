#-------------------------------------------------
#
# Project created by QtCreator 2017-02-14T15:20:26
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = OpenMW-MM
TEMPLATE = app


SOURCES += main.cpp\
        WinMain.cpp \
    TreeModModel.cpp \
    TreeModItem.cpp \
    SettingsInterface.cpp \
    OpenMWConfigInterface.cpp

HEADERS  += WinMain.h \
    TreeModModel.h \
    TreeModItem.h \
    SettingsInterface.h \
    OpenMWConfigInterface.h

FORMS    += WinMain.ui
