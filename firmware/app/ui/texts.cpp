/*
 * texts.cpp
 *
 *  Created on: 14 дек. 2015 г.
 *      Author: user
 */


#include "texts.h"

char * mode_txt[] = {(char *)"Речь", (char*)"АУС", (char*)"ГрупУК"};
char *disabled_ch_txt = (char *)"--";
char ch_open_letter='О';
char ch_zero_letter='0';
char ch_closed_letter='З';
char ch_invalid_letter='!';

const char* freq_khz = "кГц\0";
const char* freq_hz = "Гц\0";

const char* trans = "Отправить";
const char* callTitle[2] = {"команда\0", "адрес\0"};

char * ok_texts[LANG_COUNT]={(char *)"Ок"};
char * missing_ch_table_txt[LANG_COUNT]={(char *)"Отсутствует таблица речевых каналов"};
char * missing_open_ch_txt[LANG_COUNT]={(char *)"Отсутствуют открытые речевые каналы"};
char * ch_table_mismatch_txt[LANG_COUNT]={(char *)"Несоответствие таблицы речевых каналов списку каналов гарнитуры"};


const char *mainMenu[] = {"ГЛАВНОЕ МЕНЮ\0", "\tВызов\0", "\tПрием\0", "\tДанные\0", "\tНастройки\0"};

const char *callSubMenu[] = {"Условные команды\0", "SMS\0", "Голосовая почта\0", "Группа усл.команд\0"};
    const char *commandsSubMenu[] = {"Одност.связь\0", "Двухст.связь\0"};
        const char *smplSubMenu[] = {"Групповой вызов\0", "Индивидуальный\0"};
        const char *twSubMenu[] = {"Без ретранслятора\0", "С ретранслятором\0"};
    const char *smsSubMenu[] = {"Без ретранслятора\0", "С ретранслятором\0"};
    const char *groupCommandsSubMenu[] = {"Одност.связь\0", "Двухст.связь\0"};
        const char *groupCommandsSimplSubMenu[] = {"Групповой вызов\0", "Индивидуальный\0"};

const char *reciveSubMenu[] = {"Речь", "Сообщение(АУС)", "Группа усл.команд"};

const char *dataSubMenu[] = {"Принятые\0", "Отправленные\0", "Сохраненые\0"};
    const char *dataSubSubMenu[] = {"Условные команды\0", "СМС\0", "Головая почта\0", "Группа усл.команд\0"};

const char *settingsSubMenu[] = {"Дата/время\0", "Параметры связи\0", "Сканирование\0", "АРУ/АРМ\0", "Шумоподавитель\0"};
    const char *dateAndTimeSubMenu[] = {"GPS синх-ция\0", "Ручная установка\0"};
        const char *setDateOrTime[] = {"Установить дату\0", "Установить время\0"};
    const char *setConnParam[] = {"Частота\0", "Скорость\0"};
    const char *useScanMenu[] = {"ВКЛ\0", "ВЫКЛ\0"};
