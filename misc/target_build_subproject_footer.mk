#*****************************************************************************
# @file    target_build_subproject_footer.mk
# @author  Artem Pisarenko, PMR dept. software team, ONIIP, PJSC
# @date    26.08.2015
# @brief   Подвальный файл для для сборки субпроекта makepp-проекта
#
# Данный файл инклудится в конце make-файла субпроекта.
# Связан с target_build_subproject_header.mk.
#
#*****************************************************************************

include misc/target_build_footer.mk

$(OBJECTS_system): IDIR += $(INCDIR_system)
$(OBJECTS): IDIR += $(INCDIR)
