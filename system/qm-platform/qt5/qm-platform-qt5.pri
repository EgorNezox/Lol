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
    $$PWD/port_ramtex_s6d0129_cfg_seps525/ccfg0129 \
    $$PWD/port_ramtex_s6d0129_cfg_seps525/cfgio \
    $$PWD/port_ramtex_s6d0129_cfg_seps525/qt
HEADERS += \
    $$PWD/port_hardwareio/iopininterface.h \
    $$PWD/port_hardwareio/iopincheckbox.h \
    $$PWD/port_hardwareio/uartinterface.h \
    $$PWD/port_hardwareio/uartconsolewidget.h \
    $$PWD/port_hardwareio/i2cbus.h \
    $$PWD/port_hardwareio/i2cdeviceinterface.h \
    $$PWD/port_hardwareio/spibus.h \
    $$PWD/port_hardwareio/spideviceinterface.h \
    $$PWD/port_ramtex_s6d0129_cfg_seps525/qt/ramtexdisplaywidget.h \
    $$PWD/devices/flashm25pdevice.h
SOURCES += \
    $$PWD/hardware_resources.cpp \
    $$wildcardSources(port_hardwareio, *.cpp) \
    $$wildcardSources(port_ramtex_s6d0129_cfg_seps525, *.c) \
    $$PWD/port_ramtex_s6d0129_cfg_seps525/ccfg0129/ghwinit.c \
    $$PWD/port_ramtex_s6d0129_cfg_seps525/cfgio/ghwioini.c \
    $$PWD/port_ramtex_s6d0129_cfg_seps525/qt/ramtexdisplaywidget.cpp \
    $$PWD/devices/flashm25pdevice.cpp
FORMS += \
    $$PWD/port_hardwareio/uartconsolewidget.ui
