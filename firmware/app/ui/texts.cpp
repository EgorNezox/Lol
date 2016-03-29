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

const char* freq_khz = "кГц";
const char* freq_hz = "Гц";

char * ok_texts[LANG_COUNT]={(char *)"Ок"};
char * missing_ch_table_txt[LANG_COUNT]={(char *)"Отсутствует таблица речевых каналов"};
char * missing_open_ch_txt[LANG_COUNT]={(char *)"Отсутствуют открытые речевые каналы"};
char * ch_table_mismatch_txt[LANG_COUNT]={(char *)"Несоответствие таблицы речевых каналов списку каналов гарнитуры"};


const char *mainMenu[] = {"ГЛАВНОЕ МЕНЮ", "\tВызов", "\tПрием", "\tДанные", "\tНастройки"};

const char *callSubMenu[] = {"Условные команды", "SMS", "Голосовая почта", "Группа усл.команд"};
    const char *commandsSubMenu[] = {"Одност.связь", "Двухст.связь"};
        const char *smplSubMenu[] = {"Групповой вызов", "Индивидуальный"};
        const char *twSubMenu[] = {"Без ретранслятора", "С ретранслятором"};
    const char *smsSubMenu[] = {"Без ретранслятора", "С ретранслятором"};
    const char *groupCommandsSubMenu[] = {"Одност.связь", "Двухст.связь"};
        const char *groupCommandsSimplSubMenu[] = {"Групповой вызов", "Индивидуальный"};

const char *reciveSubMenu[] = {"Речь", "Сообщение(АУС)", "Группа усл.команд"};

const char *dataSubMenu[] = {"Принятые", "Отправленные", "Сохраненые"};
    const char *dataSubSubMenu[] = {"Условные команды", "СМС", "Головая почта", "Группа усл.команд"};

const char *settingsSubMenu[] = {"Дата/время", "Параметры связи", "Сканирование","Громкость"};
    const char *dateAndTimeSubMenu[] = {"GPS синх-ция", "Ручная установка"};
        const char *setDateOrTime[] = {"Установить дату", "Установить время"};
    const char *setConnParam[] = {"Частота", "Скорость"};
    const char *useScanMenu[] = {"ВКЛ", "ВЫКЛ"};
