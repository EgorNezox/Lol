#******************************************************************************
# @file    PCSimulator.pro
# @author  Artem Pisarenko, PMR dept. software team, ONIIP, PJSC
# @date    28.08.2015
# @brief   Корневой файл сборки программы firmware (Qt-приложение)
#
#******************************************************************************

QT += core gui widgets

TARGET = SazhenN_HOST_PCSimulator
TEMPLATE = app

include(../misc/qt_build_utils.pri)

include(../Qm/Qm.pri)
include(../system/3rdparty/libsigc++/sigc++.pri)

SOURCES += firmware_main.cpp
