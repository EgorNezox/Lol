#******************************************************************************
# @file    PCSimulatorBuild.pro
# @author  Artem Pisarenko, PMR dept. software team, ONIIP, PJSC
# @date    18.11.2015
# @brief   Корневой файл сборки проекта симулятора HOST
#
#******************************************************************************

TEMPLATE = subdirs
SUBDIRS = PCSimulator firmware

PCSimulator.file = PCSimulator.pro
PCSimulator.depends = firmware

firmware.file = firmware/firmware.pro
