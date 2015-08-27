#******************************************************************************
# @file    qt_build_utils.pri
# @author  Artem Pisarenko, PMR dept. software team, ONIIP, PJSC
# @date    28.08.2015
# @brief   Инструментальный модуль сборки qmake
#
# Определяет вспомогательные пользовательские функции.
#
# Функция wildcardSources(directory, pattern)
#  возвращает список файлов
#  в директории directory (относительно текущей папки вызывающего pro/pri-файла)
#  удовлетворяющих шаблону pattern (символ * означает любую последовательность символов).
#
#******************************************************************************

defineReplace(wildcardSources) {
    directory = $$1
    pattern = $$2
    system_listing =
    result =
    exists($$directory) {
        win32 {
            system_listing += $$system(dir /B /A:-D \"$$directory\\$$pattern\")
            for(file, system_listing) result += $$PWD/$$directory/$$file
        } else:unix {
            system_listing += $$system(ls \"$$directory/$$pattern\")
            for(file, system_listing) result += $$PWD/$$file
        } else {
            error("wildcardSources(...): unsupported platform")
        }
        isEmpty(result): error("wildcardSources(...): no files matching $$pattern found in directory \"$$directory\"")
    } else {
        error("wildcardSources(...): no such directory \"$$directory\"")
    }
    return($$result)
}
