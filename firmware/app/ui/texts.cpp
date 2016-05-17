/*
 * texts.cpp
 *
 *  Created on: 14 дек. 2015 г.
 *      Author: user
 */


#include "texts.h"

const char *test_Pass = {"Тестирование пройдено"};

char *mode_txt[] = {(char *)"Речь", (char*)"АУС", (char*)"ГрупУК"};
char *disabled_ch_txt = (char *)"--";
char ch_open_letter='О';
char ch_zero_letter='0';
char ch_closed_letter='З';
char ch_invalid_letter='!';

const char* freq_khz = "кГц\0";
const char* freq_hz = "Гц\0";

const char* speed_bit = "бит/с";

const char proc = '%';

const char* trans = "Отправить";
const char* callTitle[2] = {"Команда\0", "Адрес\0"};

char * ok_texts[LANG_COUNT]={(char *)"Ок"};
char * missing_ch_table_txt[LANG_COUNT]={(char *)"Отсутствует таблица речевых каналов"};
char * missing_open_ch_txt[LANG_COUNT]={(char *)"Отсутствуют открытые речевые каналы"};
char * ch_table_mismatch_txt[LANG_COUNT]={(char *)"Несоответствие таблицы речевых каналов списку каналов гарнитуры"};


const char *mainMenu[] = {"ГЛАВНОЕ МЕНЮ\0", "\tВызов\0", "\tПрием\0", "\tДанные\0", "\tНастройки\0"};

const char *callSubMenu[] = {" Условные команды\0", " SMS\0", " Голосовая почта\0", " Группа усл.команд\0"};
    const char *commandsSubMenu[] = {" Одност.связь\0", " Двухст.связь\0"};
        const char *smplSubMenu[] = {" Групповой вызов\0", " Индивидуальный\0"};
        const char *twSubMenu[] = {" Без ретранслятора\0", " С ретранслятором\0"};
    const char *smsSubMenu[] = {" Без ретранслятора\0", " С ретранслятором\0"};
    const char *groupCommandsSubMenu[] = {" Одност.связь\0", " Двухст.связь\0"};
        const char *groupCommandsSimplSubMenu[] = {" Групповой вызов\0", " Индивидуальный\0"};

const char *reciveSubMenu[] = {" Речь\0", " СМС\0", " Сообщение(АУС)\0", " УК\0", " Группа усл.команд\0"};

const char *dataSubMenu[] = {" Принятые\0", " Отправленные\0", " Сохраненые\0", " GPS координаты\0"};
    const char *dataSubSubMenu[] = {" Условные команды\0", " СМС\0", " Головая почта\0", " Группа усл.команд\0"};

const char *settingsSubMenu[] = {" Дата/время\0", " Параметры связи\0", " Сканирование\0", " АРУ / АРМ / АУC\0", " Громкость\0", " Шумоподавитель\0"};
    const char *dateAndTimeSubMenu[] = {" GPS синх-ция\0", " Ручная установка\0"};
        const char *setDateOrTime[] = {" Установить дату\0", " Установить время\0"};
    const char *setConnParam[] = {" Частота\0", " Скорость\0"};

const char *useScanMenu[] = {"ВКЛ\0", "ВЫКЛ\0"};
const char *aruStr = "АРУ\0";
const char *armStr = "АРМ\0";
const char *ausStr = "АУC\0";
const char *error_SWF = "Ошибка АФУ\0";
const char *true_SWF = "Нет ошибок\0";

const char *smsText[] = {"Адрес: \0", "Сообщение:\n\0"};

const char *sms_quit_fail1 = "Ошибка приема SMS\0";
const char *sms_quit_fail2 = "Квитанция потеряна\0";
const char *sms_crc_fail = "Пакет доставлен с ошибками\0";
const char *sms_sucsess = "SMS принята\0";

const char ch_key0[2] = { '0', '\ '};
const char ch_key1[] = { '.', ',', '!', '?', '\"', ':' };
const char ch_key2[] = { 'А', 'Б', 'В', 'Г' };
const char ch_key3[] = { 'Д', 'Е', 'Ж', 'З' };
const char ch_key4[] = { 'И', 'Й', 'К', 'Л' };
const char ch_key5[] = { 'М', 'Н', 'О', 'П' };
const char ch_key6[] = { 'Р', 'С', 'Т', 'У' };
const char ch_key7[] = { 'Ф', 'Х', 'Ц', 'Ч' };
const char ch_key8[] = { 'Ш', 'Щ', 'Ъ', 'Ы' };
const char ch_key9[] = { 'Ь', 'Э', 'Ю', 'Я' };

const char *recvCondCommand = "Принята условная команда\0";
