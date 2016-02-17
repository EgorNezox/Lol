#******************************************************************************
# @file    PCSimulator.pro
# @author  Artem Pisarenko, PMR dept. software team, ONIIP, PJSC
# @date    18.11.2015
# @brief   qmake-файл сборки Qt-приложения симулятора HOST
#
#******************************************************************************

TARGET = SazhenN_HOST_PCSimulator
TEMPLATE = app

include(misc/qt_build_utils.pri)

include(system/qm-platform/qt5/qm-platform-qt5.pri)
include(system/port__pc-simulator/pc-simulator.pri)

!win32:CONFIG(debug, debug|release): error("Non-debug build isn't supported")
LIBS += -L$$OUT_PWD/firmware/debug/ -lfirmware
win32-g++: PRE_TARGETDEPS += $$OUT_PWD/firmware/debug/libfirmware.a
else:win32:!win32-g++: PRE_TARGETDEPS += $$OUT_PWD/firmware/debug/firmware.lib
else:unix: PRE_TARGETDEPS += $$OUT_PWD/firmware/libfirmware.a

QM_BUILD_MODULES = core hardwareio keysinput
include(Qm/Qm.pri)

SIGCPP_BUILD = 1
include(system/3rdparty/libsigc++/sigc++.pri)

include(system/3rdparty/Ramtex_Graphic_Lib/Ramtex_Graphic_Lib.pri)
