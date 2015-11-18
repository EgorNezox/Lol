#******************************************************************************
# @file    firmware.pro
# @author  Artem Pisarenko, PMR dept. software team, ONIIP, PJSC
# @date    18.11.2015
# @brief   qmake-файл сборки статической библиотеки firmware
#
#******************************************************************************

TEMPLATE = lib
CONFIG += staticlib

include(../Qm/Qm.pri)
include(../system/3rdparty/libsigc++/sigc++.pri)

SOURCES += firmware_main.cpp
