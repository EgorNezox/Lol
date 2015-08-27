#*****************************************************************************
# @file    target_build_footer.mk
# @author  Artem Pisarenko, PMR dept. software team, ONIIP, PJSC
# @date    26.08.2015
# @brief   Подвальный файл для для сборки частей makepp-проекта
#
# Данный файл инклудится в конце каждого make-файла, описывающего сборку проекта.
# Связан с target_build_header.mk.
#
#*****************************************************************************

INCDIR_system += $(abspath $(INCLUDEPATH_system))
INCDIR += $(abspath $(INCLUDEPATH))

LDDEPS += $(abspath $(DEPENDENCIES))

OBJECTS_system += $(sources_to_objects $(SOURCES_system))
OBJECTS += $(sources_to_objects $(SOURCES))
