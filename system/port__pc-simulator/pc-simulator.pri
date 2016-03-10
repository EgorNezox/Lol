#******************************************************************************
# @file    pc-simulator.pri
# @author  Artem Pisarenko, PMR dept. software team, ONIIP, PJSC
# @date    17.02.2016
# @brief   qmake-файл сборки порта под PCSimulator
#
#******************************************************************************

QT += core gui widgets

DEFINES += PORT__PCSIMULATOR
HEADERS += \
    $$PWD/mainwidget.h
SOURCES += \
    $$PWD/hardware_emulation.cpp \
    $$PWD/mainwidget.cpp
FORMS += \
    $$PWD/mainwidget.ui
