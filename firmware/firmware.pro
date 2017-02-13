#******************************************************************************
# @file    firmware.pro
# @author  Artem Pisarenko, PMR dept. software team, ONIIP, PJSC
# @date    18.11.2015
# @brief   qmake-файл сборки статической библиотеки firmware
#
#******************************************************************************

TEMPLATE = lib
CONFIG += staticlib
CONFIG -= qt
CONFIG += c++11

include(../misc/qt_build_utils.pri)

include(../Qm/Qm.pri)
include(../system/3rdparty/libsigc++/sigc++.pri)

SOURCES += \
    firmware_main.cpp \
    $$wildcardSources(app/atu, *.cpp) \
    $$wildcardSources(app/datastorage, *.cpp) \
    $$wildcardSources(app/dsp, *.cpp) \
    $$wildcardSources(app/headset, *.cpp) \
    $$wildcardSources(app/mrd, *.cpp) \
    $$wildcardSources(app/navigation, *.cpp) \
    $$wildcardSources(app/power, *.cpp) \
    $$wildcardSources(app/ui, *.cpp) \
    app/ui/menu.cpp \
    app/ui/gui_tree.cpp \
    app/dsp/sheldurer.cpp \
    app/mrd/ale_data_transport.cpp \
    app/mrd/ale_main.cpp \
    app/mrd/ale_com.cpp \
    app/mrd/continious_timer.cpp \
    app/mrd/aleservice.cpp \
    app/mrd/ale_fxn.cpp

#!!! Временный хак для реализации прямого доступа к Ramtex из модуля UI
#!!! Убрать, когда модуль будет переработан на использование средств Qm
INCLUDEPATH += \
    app \
    ../system/3rdparty/Ramtex_Graphic_Lib/inc \
    ../system/3rdparty/Ramtex_Graphic_Lib/fonts \
    ../system/3rdparty/Ramtex_Graphic_Lib/icons \
    ../system/qm-platform/qt5/port_ramtex_s6d0129_cfg_seps525/ccfg0129 \
    ../system/qm-platform/qt5/port_ramtex_s6d0129_cfg_seps525/cfgio \
    ../system/qm-platform/qt5/port_ramtex_s6d0129_cfg_seps525/qt

DEFINES += PORT__PCSIMULATOR


HEADERS += \
    app/ui/menu.h \
    app/ui/elements.h \
    app/ui/service.h \
    app/ui/gui_tree.h \
    app/mrd/dispatcher.h \
    app/mrd/voiceserviceinterface.h \
    app/atu/atucontroller.h \
    app/datastorage/fs.h \
    app/dsp/dspcontroller.h \
    app/dsp/dsptransport.h \
    app/headset/controller.h \
    app/power/battery.h \
    app/ui/dialogs.h \
    app/ui/element_templates.h \
    app/ui/gui_elements_common.h \
    app/ui/gui_obj.h \
    app/ui/keyboard.h \
    app/ui/texts.h \
    app/ui/ui_keys.h \
    app/multiradio.h \
    app/headset/smarttransport.h \
    app/navigation/navigator.h \
    app/messages/rs_tms.h \
    app/dsp/packagemanager.h \
    app/dsp/sheldurer.h \
    app/mrd/ale_data_transport.h \
    app/mrd/ale_com.h \
    app/mrd/ale_main.h \
    app/mrd/ale_const.h \
    app/mrd/continious_timer.h \
    app/mrd/aleservice.h \
    app/mrd/ale_fxn.h

CONFIG(debug, release|debug):DEFINES += _DEBUG_
