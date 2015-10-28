#******************************************************************************
# @file    Qm.pri
# @author  Artem Pisarenko, PMR dept. software team, ONIIP, PJSC
# @date    06.09.2015
# @brief   qmake-файл сборки Qm
#
#******************************************************************************

DEFINES += QMCORE_PLATFORM_QT QM_HARDWARE_QT
INCLUDEPATH += $$PWD/qmcore/include
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
    $$PWD/qmcore/src/qmapplication.cpp \
    $$PWD/qmcore/src/qmapplication_qt.cpp

contains(QM_MODULES,hardwareio) {
    INCLUDEPATH += $$PWD/qmhardwareio/include
    SOURCES += \
        $$PWD/qmhardwareio/src/qmuart.cpp \
        $$PWD/qmhardwareio/src/qmuart_qt.cpp \
        $$PWD/qmhardwareio/src/qmiopin.cpp \
        $$PWD/qmhardwareio/src/qmiopin_qt.cpp
}

contains(QM_MODULES,keysinput) {
    INCLUDEPATH += $$PWD/qmkeysinput/include
    SOURCES += \
        $$PWD/qmkeysinput/src/qmpushbuttonkey.cpp \
        $$PWD/qmkeysinput/src/qmpushbuttonkey_qt.cpp \
        $$PWD/qmkeysinput/src/qmmatrixkeyboard.cpp \
        $$PWD/qmkeysinput/src/qmmatrixkeyboard_qt.cpp
}
