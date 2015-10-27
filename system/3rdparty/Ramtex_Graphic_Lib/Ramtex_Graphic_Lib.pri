#******************************************************************************
# @file    Ramtex_Graphic_Lib.pri
# @author  Artem Pisarenko, PMR dept. software team, ONIIP, PJSC
# @date    26.10.2015
# @brief   qmake-файл сборки Ramtex Graphic Library
#
#******************************************************************************

INCLUDEPATH += \
	$$PWD/inc \
	$$PWD/fonts
SOURCES += $$wildcardSources(common, *.c)
