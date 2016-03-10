/*
 * texts.cpp
 *
 *  Created on: 14 дек. 2015 г.
 *      Author: user
 */


#include "texts.h"

char * mode_txt = (char *)"Речь";
char *disabled_ch_txt = (char *)"--";
char ch_open_letter='О';
char ch_zero_letter='0';
char ch_closed_letter='З';
char ch_invalid_letter='!';


char * ok_texts[LANG_COUNT]={(char *)"Ок"};
char * missing_ch_table_txt[LANG_COUNT]={(char *)"Отсутствует таблица речевых каналов"};
char * missing_open_ch_txt[LANG_COUNT]={(char *)"Отсутствуют открытые речевые каналы"};
char * ch_table_mismatch_txt[LANG_COUNT]={(char *)"Несоответствие таблицы речевых каналов списку каналов гарнитуры"};


char *mainMenu[] = {(char*)"\tВызов", (char*)"\tПрием", (char*)"\tДанные", (char*)"\tНастройки"};

char *callSubMenu[] = {(char*)"ТЛФ", (char*)"Условные команды", (char*)"SMS", (char*)"Голосовая почта", (char*)"Группа усл.команд"};
    char *commandsSubMenu[] = {(char*)"Одност.связь", (char*)"Двухст.связь"};
        char *smplSubMenu[] = {(char*)"Индивидуяльный", (char*)"Групповой вызов"};
        char *twSubMenu[] = {(char*)"Без ретранслятора", (char*)"С ретранслятором"};
    char *smsSubMenu[] = {(char*)"Без ретранслятора", (char*)"С ретранслятором"};
    char *groupCommandsSubMenu[] = {(char*)"Одност.связь", (char*)"Двухст.связь"};

char *reciveSubMenu[] = {(char*)"Дежурный прием", (char*)"Сканирование", (char*)"ТЛФ", (char*)"Условные команды", (char*)"SMS", (char*)"Голосовая почта", (char*)"Группа усл.команд"};

char *dataSubMenu[] = {(char*)"Принятые", (char*)"Отправленные", (char*)"Сохраненые"};
    char *recvSubMenu[] = {(char*)"Условные команды", (char*)"СМС", (char*)"Головая почта", (char*)"Группа усл.команд"};
    char *sendSubMenu[] = {(char*)"Условные команды", (char*)"СМС", (char*)"Головая почта", (char*)"Группа усл.команд"};
    char *saveSubMenu[] = {(char*)"Условные команды", (char*)"СМС", (char*)"Головая почта", (char*)"Группа усл.команд"};

char *settingsSubMenu[] = {(char*)"Дата/время", (char*)"Режим связи"};
    char *dateAndTimeSubMenu[] = {(char*)"Установить дату", (char*)"Установить время"};
    char *setTimeSubMenu[] = {(char*)"GPS синх-ция", (char*)"Ручная установка"};
        char * modeSubMenu[] = {(char*)"Ручной", (char*)"Автоматический"};
