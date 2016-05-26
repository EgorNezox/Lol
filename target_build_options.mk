#*****************************************************************************
# @file    target_build_options.mk
# @author  Artem Pisarenko, PMR dept. software team, ONIIP, PJSC
# @date    27.08.2015
# @brief   Опции сборки makepp проекта
#
#*****************************************************************************

PROJECT_NAME = SazhenN_HOST_$(BUILD_PORT)

CC := $(PREFIX_CC_CXX) arm-none-eabi-gcc
CXX := $(PREFIX_CC_CXX) arm-none-eabi-g++
AS := arm-none-eabi-gcc -x assembler-with-cpp
OBJCOPY := arm-none-eabi-objcopy
OBJDUMP := arm-none-eabi-objdump
SIZE := arm-none-eabi-size
BUILD_PORT ?= target-device-rev1 # по умолчанию целевое устройство последней ревизии
BUILD_MODE ?= release # по умолчанию релизная сборка (боевая)

# Architecture-specific options
ARCH_FLAGS = -mthumb -mcpu=cortex-m3
# Debugging options
DEBUG_FLAGS = -g3
# Common compiler optimization options
C_OPT_FLAGS = -ffunction-sections -fdata-sections

# общие опции компиляции для всех исходников на языке C
CFLAGS = $(ARCH_FLAGS) $(DEBUG_FLAGS) $(C_OPT_FLAGS) -c -fmessage-length=0 -Wall -std=gnu99
# общие опции компиляции для всех исходников на языке C++
CXXFLAGS = $(ARCH_FLAGS) $(DEBUG_FLAGS) $(C_OPT_FLAGS) -c -fmessage-length=0 -Wall -std=gnu++11 -fno-exceptions -fno-rtti
# общие опции компиляции для всех исходников на языке Assembler
ASFLAGS = $(ARCH_FLAGS) $(DEBUG_FLAGS) -c -fmessage-length=0 -Wall
# опции линковщика (используется LTO, но результат зависит от того, включен ли LTO в опциях компиляции индивидуально для субпроекта)
LDFLAGS = $(ARCH_FLAGS) $(DEBUG_FLAGS) -flto -fuse-linker-plugin -Xlinker --gc-sections -Xlinker --fatal-warnings -Xlinker --warn-common
