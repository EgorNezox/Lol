#******************************************************************************
# @file    qm-platform-qt5.pri
# @author  Artem Pisarenko, PMR dept. software team, ONIIP, PJSC
# @date    26.10.2015
# @brief   qmake-файл сборки Qm-платформы под фреймворк Qt5
#
#******************************************************************************

QT += core gui widgets
CONFIG += no_keywords # because of conflict with libsigc++

INCLUDEPATH += \
    $$PWD \
    $$PWD/../config \
    $$PWD/port_spiffs \
    $$PWD/port_ramtex_sdd0323_cfg_ssd1327/cfg0323 \
    $$PWD/port_ramtex_sdd0323_cfg_ssd1327/cfgio \
    $$PWD/port_ramtex_sdd0323_cfg_ssd1327/qt
HEADERS += \
    $$PWD/port_hardwareio/iopininterface.h \
    $$PWD/port_hardwareio/iopincheckbox.h \
    $$PWD/port_hardwareio/uartinterface.h \
    $$PWD/port_hardwareio/uartconsolewidget.h \
    $$PWD/port_hardwareio/i2cbus.h \
    $$PWD/port_hardwareio/i2cdeviceinterface.h \
    $$PWD/port_hardwareio/spibus.h \
    $$PWD/port_hardwareio/spideviceinterface.h \
    $$PWD/port_keysinput/pushbuttonkey.h \
    $$PWD/port_keysinput/pushbuttonkeyinterface.h \
    $$PWD/port_keysinput/matrixkeyboardwidget.h \
    $$PWD/port_keysinput/matrixkeyboardinterface.h \
    $$PWD/port_ramtex_sdd0323_cfg_ssd1327/qt/ramtexdisplaywidget.h \
    $$PWD/devices/flashm25pdevice.h \
    $$PWD/port_rtc/rtc.h \
    $$PWD/port_rtc/rtcinterface.h
SOURCES += \
    $$PWD/hardware_resources.cpp \
    $$wildcardSources(port_hardwareio, *.cpp) \
    $$wildcardSources(port_keysinput, *.cpp) \
    $$wildcardSources(port_ramtex_sdd0323_cfg_ssd1327, *.c) \
    $$PWD/port_ramtex_sdd0323_cfg_ssd1327/cfgio/ghwioini.c \
    $$PWD/port_ramtex_sdd0323_cfg_ssd1327/qt/ramtexdisplaywidget.cpp \
    $$PWD/devices/flashm25pdevice.cpp \
    $$PWD/port_rtc/rtc.cpp \
    $$PWD/port_rtc/rtcinterface.cpp
FORMS += \
    $$PWD/port_hardwareio/uartconsolewidget.ui \
    $$PWD/port_keysinput/matrixkeyboardwidget.ui
