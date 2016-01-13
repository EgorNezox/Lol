#******************************************************************************
# @file    firmware.pro
# @author  Artem Pisarenko, PMR dept. software team, ONIIP, PJSC
# @date    18.11.2015
# @brief   qmake-файл сборки статической библиотеки firmware
#
#******************************************************************************

TEMPLATE = lib
CONFIG += staticlib

include(../misc/qt_build_utils.pri)

include(../Qm/Qm.pri)
include(../system/3rdparty/libsigc++/sigc++.pri)

SOURCES += \
    firmware_main.cpp \
    $$wildcardSources(app/datastorage, *.cpp) \
    $$wildcardSources(app/dsp, *.cpp) \
    $$wildcardSources(app/headset, *.cpp) \
    $$wildcardSources(app/mrd, *.cpp) \
    $$wildcardSources(app/power, *.cpp) \
    $$wildcardSources(app/ui, *.cpp)

#!!! Временный хак для реализации прямого доступа к Ramtex из модуля UI
#!!! Убрать, когда модуль будет переработан на использование средств Qm
INCLUDEPATH += \
    app \
    ../system/3rdparty/Ramtex_Graphic_Lib/inc \
    ../system/3rdparty/Ramtex_Graphic_Lib/fonts \
    ../system/3rdparty/Ramtex_Graphic_Lib/icons \
    ../system/platform/pc-simulator/port_ramtex_s6d0129_cfg_qt5seps525widget/ccfg0129 \
    ../system/platform/pc-simulator/port_ramtex_s6d0129_cfg_qt5seps525widget/cfgio \
    ../system/platform/pc-simulator/port_ramtex_s6d0129_cfg_qt5seps525widget/qt

DEFINES += PORT__PCSIMULATOR
