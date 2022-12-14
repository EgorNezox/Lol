#******************************************************************************
# @file    Qm.pri
# @author  Artem Pisarenko, PMR dept. software team, ONIIP, PJSC
# @date    06.09.2015
# @brief   qmake-файл сборки/подключения Qm
#
# Модули фреймворка компилируются при установке соответствующих значений в переменной QM_BUILD_MODULES.
# (Например, модулю QmCore соответствует значение "core".)
#
#******************************************************************************

INCLUDEPATH += \
	$$PWD/qmcore/include \
	$$PWD/qmhardwareio/include \
	$$PWD/qmconsole/include \
	$$PWD/qmstorage/include \
        $$PWD/qmkeysinput/include \
        $$PWD/qmrtc/include \
        $$PWD/qmusb/include

DEFINES += QM_PLATFORM_QT

contains(QM_BUILD_MODULES,core) {
    SOURCES += \
        $$PWD/qmcore/src/qm_core_qt.cpp \
        $$PWD/qmcore/src/qmdebug.cpp \
        $$PWD/qmcore/src/qmobject.cpp \
        $$PWD/qmcore/src/qmobject_qt.cpp \
        $$PWD/qmcore/src/qmthread_qt.cpp \
        $$PWD/qmcore/src/qmmutex_qt.cpp \
        $$PWD/qmcore/src/qmmutexlocker.cpp \
        $$PWD/qmcore/src/qmtimer.cpp \
        $$PWD/qmcore/src/qmtimer_qt.cpp \
        $$PWD/qmcore/src/qmtimestamp_qt.cpp \
        $$PWD/qmcore/src/qmabstimer.cpp \
        $$PWD/qmcore/src/qmabstimer_qt.cpp \
        $$PWD/qmcore/src/qmelapsedtimer_qt.cpp \
        $$PWD/qmcore/src/qmapplication.cpp \
        $$PWD/qmcore/src/qmapplication_qt.cpp
}

contains(QM_BUILD_MODULES,hardwareio) {
    HEADERS += \
        $$PWD/qmhardwareio/src/qmiopin_p.h \
        $$PWD/qmhardwareio/src/qmuart_p.h \
        $$PWD/qmhardwareio/src/qmi2cdevice_p.h \
        $$PWD/qmhardwareio/src/qmsmbushost_p.h \
        $$PWD/qmhardwareio/src/qmspidevice_p.h
    SOURCES += \
        $$PWD/qmhardwareio/src/qmiopin.cpp \
        $$PWD/qmhardwareio/src/qmiopin_qt.cpp \
        $$PWD/qmhardwareio/src/qmuart.cpp \
        $$PWD/qmhardwareio/src/qmuart_qt.cpp \
        $$PWD/qmhardwareio/src/qmi2cdevice.cpp \
        $$PWD/qmhardwareio/src/qmi2cdevice_qt.cpp \
        $$PWD/qmhardwareio/src/qmsmbushost.cpp \
        $$PWD/qmhardwareio/src/qmsmbushost_qt.cpp \
        $$PWD/qmhardwareio/src/qmspibus_qt.cpp \
        $$PWD/qmhardwareio/src/qmspidevice.cpp \
        $$PWD/qmhardwareio/src/qmspidevice_qt.cpp
}

contains(QM_BUILD_MODULES,storage) {
    SOURCES += \
        $$PWD/qmstorage/src/qmfile.cpp \
        $$PWD/qmstorage/src/qmspiffs.cpp \
        $$PWD/qmstorage/src/qmm25pdevice.cpp
}

contains(QM_BUILD_MODULES,keysinput) {
    HEADERS += \
        $$PWD/qmkeysinput/src/qmpushbuttonkey_p.h \
        $$PWD/qmkeysinput/src/qmmatrixkeyboard_p.h
    SOURCES += \
        $$PWD/qmkeysinput/src/qmpushbuttonkey.cpp \
        $$PWD/qmkeysinput/src/qmpushbuttonkey_qt.cpp \
        $$PWD/qmkeysinput/src/qmmatrixkeyboard.cpp \
        $$PWD/qmkeysinput/src/qmmatrixkeyboard_qt.cpp
}

contains(QM_BUILD_MODULES,console) {
    SOURCES += \
        $$PWD/qmconsole/src/qmconsolescreen.cpp
}

contains(QM_BUILD_MODULES,rtc) {
    HEADERS += \
        $$PWD/qmrtc/src/qmrtc_p.h
    SOURCES += \
        $$PWD/qmrtc/src/qmrtc.cpp \
        $$PWD/qmrtc/src/qmrtc_qt.cpp
}

contains(QM_BUILD_MODULES,usb) {
    HEADERS += \
        $$PWD/qmusb/src/qmusb_p.h
    SOURCES += \
        $$PWD/qmusb/src/qmusb.cpp \
        $$PWD/qmusb/src/qmusb_stm32f2xx.cpp
}
