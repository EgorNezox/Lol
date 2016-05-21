#******************************************************************************
# @file    sigc++.pri
# @author  Artem Pisarenko, PMR dept. software team, ONIIP, PJSC
# @date    06.09.2015
# @brief   qmake-файл сборки/подключения libsigc++
#
# Библиотека компилируется при установленной переменной SIGCPP_BUILD=1
#
#******************************************************************************

CONFIG += c++11

INCLUDEPATH += $$PWD

defined(SIGCPP_BUILD, var) {
    SOURCES += \
        $$PWD/sigc++/functors/slot.cc \
        $$PWD/sigc++/functors/slot_base.cc \
        $$PWD/sigc++/connection.cc \
        $$PWD/sigc++/signal.cc \
        $$PWD/sigc++/signal_base.cc \
        $$PWD/sigc++/trackable.cc
}
