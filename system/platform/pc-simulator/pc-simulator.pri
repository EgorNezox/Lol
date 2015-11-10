#******************************************************************************
# @file    pc-simulator.pri
# @author  Artem Pisarenko, PMR dept. software team, ONIIP, PJSC
# @date    26.10.2015
# @brief   qmake-файл сборки порта под PCSimulator
#
#******************************************************************************

DEFINES += PORT__PCSIMULATOR
INCLUDEPATH += \
    $$PWD \
    $$PWD/port_ramtex_s6d0129_cfg_qt5seps525widget/ccfg0129 \
    $$PWD/port_ramtex_s6d0129_cfg_qt5seps525widget/cfgio \
    $$PWD/port_ramtex_s6d0129_cfg_qt5seps525widget/qt
HEADERS += \
    $$PWD/mainwidget.h \
    $$PWD/port_hardwareio/iopininterface.h \
    $$PWD/port_hardwareio/iopincheckbox.h \
    $$PWD/port_ramtex_s6d0129_cfg_qt5seps525widget/qt/ramtexdisplaywidget.h
SOURCES += \
    $$PWD/hardware_emulation.cpp \
    $$PWD/mainwidget.cpp \
    $$wildcardSources(port_hardwareio, *.cpp) \
    $$wildcardSources(port_ramtex_s6d0129_cfg_qt5seps525widget, *.c) \
    $$PWD/port_ramtex_s6d0129_cfg_qt5seps525widget/ccfg0129/ghwinit.c \
    $$PWD/port_ramtex_s6d0129_cfg_qt5seps525widget/cfgio/ghwioini.c \
    $$PWD/port_ramtex_s6d0129_cfg_qt5seps525widget/qt/ramtexdisplaywidget.cpp
FORMS += \
    $$PWD/mainwidget.ui