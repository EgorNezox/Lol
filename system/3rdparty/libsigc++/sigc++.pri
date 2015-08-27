#******************************************************************************
# @file    sigc++.pri
# @author  Artem Pisarenko, PMR dept. software team, ONIIP, PJSC
# @date    06.09.2015
# @brief   qmake-файл сборки libsigc++
#
#******************************************************************************

CONFIG += no_keywords
QMAKE_CXXFLAGS += -std=gnu++11

INCLUDEPATH += $$PWD

SOURCES += \
    $$PWD/sigc++/functors/slot.cc \
    $$PWD/sigc++/functors/slot_base.cc \
    $$PWD/sigc++/connection.cc \
    $$PWD/sigc++/signal.cc \
    $$PWD/sigc++/signal_base.cc \
    $$PWD/sigc++/trackable.cc
