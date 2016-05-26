#*****************************************************************************
# @file    target_build_header.mk
# @author  Artem Pisarenko, PMR dept. software team, ONIIP, PJSC
# @date    26.08.2015
# @brief   Заголовочный файл для для сборки частей makepp-проекта
#
# Данный файл инклудится в начале каждого make-файла, описывающего сборку проекта или его части.
# Для корректной работы правил во вложенных директориях перед инклудом должна быть установлена опция
#  makepp_percent_subdirs=1
# Связан с target_build_footer.mk.
#
# Проект делится на две части:
# - прикладная;
# - системная.
# Сборка описывается следующими переменными:
# - C_EXTRA_FLAGS (доп. опции компиляции исходников C всего субпроекта, размещаются перед общими опциями)
# - CXX_EXTRA_FLAGS (доп. опции компиляции исходников C++ всего субпроекта, размещаются перед общими опциями)
# - INCLUDEPATH (include-пути C/C++ для исходников прикладной части);
# - INCLUDEPATH_system (include-пути C/C++ для исходников системной части);
# - DEPENDENCIES (нестандартные файлы-зависимости выходного файла, т.е. отсутствующие в правилах сборки и используемые косвенно);
# - SOURCES (исходники прикладной части);
# - SOURCES_system (исходники системной части);
# - DEFINES (глобальные макросы для исходников прикладной части);
# - DEFINES_system (глобальные макросы для исходников системной части);
# - LDFLAGS_system (глобальные флаги линковщика выходного файла).
#
#*****************************************************************************

perl { use Text::ParseWords }

# Build configuration
ifdef PREFIX_CC_CXX
register-parser $(PREFIX_CC_CXX) skip-word
endif
BUILD_DIR_NAME = BUILD_$(BUILD_PORT)_$(BUILD_MODE) # название директории сборки
BUILD_ROOT_DIR = $(SUBPROJECT_PATH)/$(BUILD_DIR_NAME) # путь к корню директории сборки суб-проекта
BUILD_OBJ_REL_DIR ?= $(SUBPROJECT_PATH) # см. выше, переменная может быть переопределена для makefile'ов директорий, располагающихся вне дерева суб-проекта, чтобы их объектные файлы отражались в дерево сборки корректно
BUILD_OBJ_REL_PATH = $(BUILD_ROOT_DIR)/$(relative_to ., $(BUILD_OBJ_REL_DIR)) # спец. трюк для указания объектных файлов в директории сборки независимо от текущей директории и уровня вложенности makefile-ов, используется при определении правил
no_implicit_load * # в связи с нестандартным подходом к сборке (out-of-tree), все makefile'ы подгружаются явно

global C_EXTRA_FLAGS CXX_EXTRA_FLAGS
global DEFINES INCDIR OBJECTS LDDEPS
global DEFINES_system INCDIR_system OBJECTS_system LDFLAGS_system

# Файл конфигурации сборки проекта (в корне исходников проекта)
# В нем должны быть определены следующие локальные переменные:
# - PROJECT_NAME (имя проекта, м.б. использоваться в субпроектах как название выходного файла сборки)
# - BUILD_PORT (название порта сборки, используется в названии директории сборки и пользовательских Makepp-файлах, м.б. определено значение по умолчанию, а текущее значение определяться в аргументах команды makepp)
# - BUILD_MODE (вариант сборки, стандартные значения debug/release, используется в названии директории сборки и пользовательских Makepp-файлах, м.б. определено значение по умолчанию, а текущее значение определяться в аргументах команды makepp)
# - CFLAGS (общие флаги компилятора для всех исходников на C)
# - CXXFLAGS (общие флаги компилятора для всех исходников на C++)
# - ASFLAGS (общие флаги компилятора для всех исходников на Assembler)
# - LDFLAGS (флаги линковщика для выходного/исполняемого файла)
# Также опционально можно переопределить стандартные переменные Makepp (например, CC)
include ../target_build_options.mk

# функция возвращает "откорректированный" список путей (директории, файлы), переданный в аргументе
# (решение проблем с парсером Eclipse)
sub f_nicepath {
  my $argpath = &arg;
  if ($^O eq 'msys') {
    $argpath =~ s/^\/([A-Za-z])\//$1:\//;
  }
  return $argpath; 
}

# функция конвертирует пути к исходным файлам в пути к соответствующим объектным файлам
sub f_sources_to_objects {
  my $files_string = &arg;
  my $extensions = 'c|cpp|cc|s'; # должно соответствовать всем шаблонным правилам для %.o !
  my $rel_path = $_[1]->expand_variable('BUILD_OBJ_REL_PATH');
  my @files_list = shellwords($files_string);
  foreach (@files_list) {
   s/(.*)\.(?:$extensions)$/\"$rel_path\/$1\.o\"/;
  }
  $files_string = join(' ', @files_list);
  return $files_string;
}


###################################################################################################
# Шаблонные правила
# (чтобы '%' учитывал вложенные директории, нужно перед инклудом файла задать makepp_percent_subdirs=1)
# Внимание! Расширения исходников (*_SOURCE) имеют приоритет в сборке согласно порядку перечисления правил (например, если в исходниках указан файл <name>.c, а рядом с ним имеется файл <name>.cpp, то в сборку войдет последний).

define COMPILE_C_SOURCE
	@&echo "*** Compile C source: $(relative_to $(input), $(SUBPROJECT_PATH)) ***"
	@&mkdir -p $(dir_noslash $(output))
	$(CC) -c $(C_EXTRA_FLAGS) $(CFLAGS) $(foreach define,$(DDEF),-D$(define)) $(foreach dir,$(IDIR),-I$(nicepath $(dir))) $(nicepath $(abspath $(input))) -o $(output)
endef
define COMPILE_CPP_SOURCE
	@&echo "*** Compile C++ source: $(relative_to $(input), $(SUBPROJECT_PATH)) ***"
	@&mkdir -p $(dir_noslash $(output))
	$(CXX) -c $(CXX_EXTRA_FLAGS) $(CXXFLAGS) $(foreach define,$(DDEF),-D$(define)) $(foreach dir,$(IDIR),-I$(nicepath $(dir))) $(nicepath $(abspath $(input))) -o $(output)
endef
define COMPILE_ASSEMBLER_SOURCE
	@&echo "*** Compile assembler source: $(relative_to $(input), $(SUBPROJECT_PATH)) ***"
	@&mkdir -p $(dir_noslash $(output))
	$(AS) -c $(ASFLAGS) $(foreach define,$(DDEF),-D$(define)) $(nicepath $(abspath $(input))) -o $(output)
endef

$(BUILD_OBJ_REL_PATH)/%.o : %.c
	:smartscan
	$(COMPILE_C_SOURCE)

$(BUILD_OBJ_REL_PATH)/%.o : %.cpp
	:smartscan
	$(COMPILE_CPP_SOURCE)

$(BUILD_OBJ_REL_PATH)/%.o : %.cc
	:smartscan
	$(COMPILE_CPP_SOURCE)

$(BUILD_OBJ_REL_PATH)/%.o : %.s
	$(COMPILE_ASSEMBLER_SOURCE)

$(BUILD_OBJ_REL_PATH)/%.o : %.asm
	$(COMPILE_ASSEMBLER_SOURCE)

$(BUILD_ROOT_DIR)/%.hex : $(BUILD_ROOT_DIR)/%.elf
	@&echo "*** Generate flash image: $(notdir $(output)) ***"
	@&mkdir -p $(dir_noslash $(output))
	$(OBJCOPY) -O ihex -j .text -j .data -j .fastcode $(foreach section,$(SECTIONS),-j $(section)) $(input) $(output)

$(BUILD_ROOT_DIR)/%.lst : $(BUILD_ROOT_DIR)/%.elf
	@&echo "*** Generate listing: $(notdir $(output)) ***"
	@&mkdir -p $(dir_noslash $(output))
	$(OBJDUMP) -h -S $(input) > $(output)

$(BUILD_ROOT_DIR)/%._size : $(BUILD_ROOT_DIR)/%.elf
	@&echo "*** Print size of $(notdir $(input)) ***"
	@&mkdir -p $(dir_noslash $(output))
	$(SIZE) $(input)

###################################################################################################
