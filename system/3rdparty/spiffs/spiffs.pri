#******************************************************************************
# @file    spiffs.pri
# @author  Artem Pisarenko, PMR dept. software team, ONIIP, PJSC
# @date    16.02.2016
# @brief   qmake-файл сборки SPIFFS
#
#******************************************************************************

INCLUDEPATH += $$PWD/src
SOURCES += $$wildcardSources(src, *.c)
