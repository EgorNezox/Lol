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
    $$PWD/dsp/dspdevice.h \
    $$PWD/dsp/dsptransport.h \
    $$PWD/atu/atudevice.h \
    $$PWD/mainwidget.h
SOURCES += \
    $$wildcardSources(dsp, *.cpp) \
    $$wildcardSources(atu, *.cpp) \
    $$PWD/hardware_emulation.cpp \
    $$PWD/mainwidget.cpp
FORMS += \
    $$PWD/dsp/dspdevice.ui \
    $$PWD/atu/atudevice.ui \
    $$PWD/mainwidget.ui
