#******************************************************************************
# @file    target_build_subproject_header.mk
# @author  Artem Pisarenko, PMR dept. software team, ONIIP, PJSC
# @date    26.08.2015
# @brief   Заголовочный файл для для сборки субпроекта makepp-проекта
#
# Данный файл инклудится в конце make-файла субпроекта.
# Для корректной работы правил во вложенных директориях перед инклудом должна быть установлена опция
#  makepp_percent_subdirs=1
# Связан с target_build_subproject_footer.mk.
#
# В субпроекте осуществляется линковка всех объектников и получение финального результата (цели сборки).
# Make-файлы должен явно загружать составные части проекта (выражением load_makefile).
# При этом, если составная часть проекта находится вне иерархии директории субпроекта,
#  то необходимо задавать спец. переменную с относительным путем: load_makefile BUILD_OBJ_REL_DIR="$(SUBPROJECT_PATH)/.." <make-файлы>.
#
# Субпроект должен определить значения переменных:
# - TARGET_NAME (имя субпроекта, используется в названии выходного файла сборки);
# - LDSCRIPTS (linker-скрипты).
#
#******************************************************************************

global SUBPROJECT_PATH := $(abspath .)
include misc/target_build_header.mk

ifeq ($(BUILD_MODE),release)
  DEFINES += NDEBUG
  DEFINES_system += NDEBUG
endif

# Полная сборка суб-проекта (build all)
$(phony all): build hex dfu
# сборка программы (генерация выходного файла .elf)
$(phony build): $(BUILD_ROOT_DIR)/$(TARGET_NAME).elf
# генерация файла программы .hex
$(phony hex): $(BUILD_ROOT_DIR)/$(TARGET_NAME).hex
# генерация файла программы .dfu
$(phony dfu): $(BUILD_ROOT_DIR)/$(TARGET_NAME).dfu
# прошивка программы в целевое устройство по DFU
$(phony flash_dfu): $(BUILD_ROOT_DIR)/$(TARGET_NAME)._flash_dfu
.PHONY: $(BUILD_ROOT_DIR)/$(TARGET_NAME)._flash_dfu
# генерация листинга программы
$(phony list): $(BUILD_ROOT_DIR)/$(TARGET_NAME).lst
# вывод размера программы
$(phony size): $(BUILD_ROOT_DIR)/$(TARGET_NAME)._size
.PHONY: $(BUILD_ROOT_DIR)/$(TARGET_NAME)._size
# Очистка проекта (clean) (удаляет все содержимое директории сборки !)
$(phony clean):
    &rm -fm $(wildcard $(relative_to $(BUILD_ROOT_DIR), .)/**/*)

# Линковка программы (выходного файла elf) из скомпилированных частей
$(BUILD_ROOT_DIR)/$(TARGET_NAME).elf $(BUILD_ROOT_DIR)/$(TARGET_NAME).map: $(LDSCRIPTS) $(LDDEPS) $(OBJECTS_system) $(OBJECTS)
	@&echo "*** Link elf target: $(TARGET_NAME).elf ***"
	@&mkdir -p $(BUILD_ROOT_DIR)
	$(CXX) $(filter_out $( $(LDSCRIPTS) $(LDDEPS) ), $(inputs)) $(LDFLAGS) $(LDFLAGS_system) -T$(LDSCRIPTS) -Wl,-Map,$(BUILD_ROOT_DIR)/$(TARGET_NAME).map -o $(output)
