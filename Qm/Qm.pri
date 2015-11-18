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
	$$PWD/qmhardwareio/include

contains(QM_BUILD_MODULES,core) {
    DEFINES += QMCORE_PLATFORM_QT
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
}

contains(QM_BUILD_MODULES,hardwareio) {
    DEFINES += QMHARDWAREIO_PLATFORM_QT
    HEADERS += \
        $$PWD/qmhardwareio/src/qmiopin_p.h \
        $$PWD/qmhardwareio/src/qmuart_p.h
    SOURCES += \
        $$PWD/qmhardwareio/src/qmiopin.cpp \
        $$PWD/qmhardwareio/src/qmiopin_qt.cpp \
        $$PWD/qmhardwareio/src/qmuart.cpp \
        $$PWD/qmhardwareio/src/qmuart_qt.cpp
}
