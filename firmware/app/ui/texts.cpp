#include "texts.h"

const char *test_Pass = {"Тестирование\0"};
const char* exitStr = "Выход\0";
char *mode_txt[] = {(char *)"Речь\0", (char*)"АУС\0", (char*)"ГрупУК\0"};
char *disabled_ch_txt = (char *)"--";
char ch_open_letter='О';
char ch_zero_letter='0';
char ch_closed_letter='З';
char ch_invalid_letter='!';

const char* freq_khz = "кГц\0";
const char* freq_hz = "Гц\0";

const char* speed_bit = "бит/с\0";

const char proc = '%';

const char* trans = "Отправить\0";
const char* callTitle[2] = {"Команда\0", "Адрес\0"};

char * ok_texts[LANG_COUNT]={(char *)"Ок"};
char * missing_ch_table_txt[LANG_COUNT]={(char *)"Отсутствует таблица речевых каналов\0"};
char * missing_open_ch_txt[LANG_COUNT]={(char *)"Отсутствуют открытые речевые каналы\0"};
char * ch_table_mismatch_txt[LANG_COUNT]={(char *)"Несоответствие таблицы речевых каналов списку каналов гарнитуры\0"};

const char* receiveStr = "Принять\0";
const char* continueStr = "Продолжить\0";
const char* receiveStatusStr[] = {"Начать прием\0", "Идет прием\0"};

const char *mainMenu[] = {"ГЛАВНОЕ МЕНЮ\0", " Передача\0", " Прием\0", " Данные\0", " Настройки\0"};

const char *callSubMenu[] = {" УК\0", " СМС\0", " ГП\0", " ГУК\0"};
    const char *commandsSubMenu[] = {" Одност.связь\0", " Двухст.связь\0"};
        const char *smplSubMenu[] = {" Груп.\0", " Индивид.\0", "С квит.\0"};
        const char *twSubMenu[] = {" Без ретранслятора\0", " С ретранслятором\0"};
    const char *smsSubMenu[] = {" Без ретранслятора\0", " С ретранслятором\0"};
    const char *groupCommandsSubMenu[] = {" Одност.связь\0", " Двухст.связь\0"};
        const char *groupCommandsSimplSubMenu[] = {" Групповой вызов\0", " Индивидуальный\0"};

const char *reciveSubMenu[] = {" Речь\0", " СМС\0", " ГП\0", " УК\0", " ГУК\0", " Радиомолч\0"};

const char *dataSubMenu[] = {" Принятые\0", " Отправленные\0", " Сохраненые\0", " GPS координаты\0"};
    const char *dataSubSubMenu[] = {" Условные команды\0", " СМС\0", " Головая почта\0", " Группа усл.команд\0"};

const char *settingsSubMenu[] = {"Дата/время\0", " Пар связи\0", " Скан\0", " АРУ/АРМ/АУC\0", " Громкость\0", " подавитель\0"," Ожид ГУК\0", " КР"}; //TODO:
    const char *dateAndTimeSubMenu[] = {" GPS синх-ция\0", " Ручная установка\0"};
        const char *setDateOrTime[] = {" Установить дату\0", " Установить время\0"};
    const char *setConnParam[] = {" Частота\0", " Скорость\0"};

const char *coordinateStr = "Координаты\0";
const char *useScanMenu[] = {"ВКЛ\0", "ВЫКЛ\0"};
const char *aruStr = "АРУ\0";
const char *armStr = "АРМ\0";
const char *ausStr = "АУC\0";
const char *error_SWF = "ТЕСТ ПРОЙДЕН\0";
const char *true_SWF = "ТЕСТ ПРОЙДЕН\0";

const char *smsText[] = {"Адрес: \0", "Сообщение:\n\0"};

const char *sms_quit_fail1 = "Ошибка приема SMS\0";
const char *sms_quit_fail2 = "Квитанция потеряна\0";
const char *sms_crc_fail = "Пакет доставлен с ошибками\0";

const char ch_key0[] = { ' ', '0' };
const char ch_key1[] = { '.', ',', '!', '?', '\"', ':', '1' };
const char ch_key2[] = { 'А', 'Б', 'В', 'Г', '2' };
const char ch_key3[] = { 'Д', 'Е', 'Ж', 'З', '3' };
const char ch_key4[] = { 'И', 'Й', 'К', 'Л', '4' };
const char ch_key5[] = { 'М', 'Н', 'О', 'П', '5' };
const char ch_key6[] = { 'Р', 'С', 'Т', 'У', '6' };
const char ch_key7[] = { 'Ф', 'Х', 'Ц', 'Ч', '7' };
const char ch_key8[] = { 'Ш', 'Щ', 'Ъ', 'Ы', '8' };
const char ch_key9[] = { 'Ь', 'Э', 'Ю', 'Я', '9' };

const char* recvCondCommand = "Принята условная команда\0";

const char* groupCondCommFreqStr = "Частота \0";
const char* dataAndTime[] = {"Дата\0", "Время\0"};

const char* putOffVoiceStageOneStr[] = {"Нажмите ввод для записи\0", "Ожидание записи\0"};
const char* putOffVoiceStageTwoStr[] = {"Нажмите ввод для завершения\0", "Запись речи\0"};

const char* startAleTxVoiceMailStr = "Нажмите Ввод для\nзапуска передачи\0";

const char* smatrHSStateStr[] = {
    "Воспроизвести\nневозможно\nПовторите\n\0",
    "Воспроизвести\nневозможно\nНет\nгарнитуры\n0",
    "Ошибка\nгарнитуры\n\0",
    "Ошибка\nканала\nВыберите\nдругой канал\0",
    "Подготовка\nсообщения\nЖдите\0",
    "Проигрывание\nсообщения\nЖдите\0",
    "Отправка записи\nсообщения\nЖдите\0",
    "Проигрывание\ncообщения\nОжидайте\0",
    "Подготовка\nзаписи\nЖдите\0",
    "Подготовка\nзаписи\nЖдите\0",
    "Удерживайте\nтангенту\nдля записи\n\0",
    "Получение\nзаписи\nЖдите\0",
    "Истекло\nвремя\nожидания\n\0",
    "Готово\n\0",
    "14","15","16","17","18","19","20"
};

const char* aleStateStr[] = {
    "...\0",
    "Нет\nсинхронизации\n\0",
    "Аппаратный\nсбой\0",
    "Нет частот\nАУС\nпо умолчанию\n\0",
    "Не задан\nадрес\nстанции.\n\0",
    "Ошибка\nнеобходима\nсинхронизация\0",
    "Ожидание\nвызова...\0",
    "Принят\nвызов\nнастройка\nобмена...\0",
    "Входящий\nпрерван\nПовторите\0",
    "Прием ГП\0", //"<AleVmProgress>%\0",
    "Исходящий\nвызов\0",
    "Нет связи\nПовторите\0",
    "Вызов принят\nсогласование\nпараметров\nобмена\0",
    "Передача ГП\0", //"<AleVmProgress>%\0",
    "Нет связи\nПовторите\0",
    "Сообщение\nпотеряно\nПовторите\0",
    "Передача\nуспешно\nзавершена\n\0",
    "Прием\nсообщения\nпрерван\nПовторите\0",
    "Нет связи\nПовторите\0",
    "ГП принята\nс ошибками\nВвод\nдля прод.\0",
    "Принята ГП\n Ввод\nдля прод.\0",
};

const char *smsDataInformTx[] = {
    "Вызов\nабонента\0",
    "Ожидание\nответа\0",
    "Ответ\nпринят\0",
    "Передача\nданных\0",
    "Данные\nдоставлены\0",
    "Сеанс\nпрерван\nПовторите\0",
    "Данные\nпотеряны\nПовторите\nпередачу\0",
};

const char *smsDataInformRx[] = {
    "Ожидание\nприема\0",
    "Получен\nвходящий\nвызов\0",
    "Ответ\nвызываюшему\0",
    "Прием\nданных\0",
    "Сеанс\nпрерван\nПовторите\0",
    "Данные\nпотеряны\nПовторите\0",
};

const char *txSmsResultStatus[] = {
    "Вызов станции\0",
    "Отправка ответа\0",
    "Отправка данных\0",
    "Квитанция\0"
};

const char *rxSmsResultStatus[] = {
    "Ожидание вызова\0",
    "Получение ответа\0",
    "Прием данных\0",
    "Ожидание квитанции\0"
};

const char* voiceRxStr[] = {"Нажмите Ввод для\nзапуска приема\0", "От: \0", "Нажмите ввод\0"};
const char* voiceRxTxLabelStr[] = {"Канал записи\0", "Канал воспр.\0", "Запись речи\0", "Воспр. речи\0", "Получатель\0", "Получена гол.поч.\0"};

const char* condCommStr[] = {"Адрес получателя\0", "Адрес ретранслятора\0", "Команда\0", "Ретранслятор\0", "Сообщение\0"};
const char* condCommSendStr = "Нажмите Ввод для \nзапуска передачи\0";

const char* ticketStr[] = {"Квитирование\0", "Ретрансляция\0"};
const char* voicePostTitleStr[] = {"Передача гол. почты\0", "Прием гол. почты\0"};

const char* gucQuitTextFail = "Квитанция не принята\0";
const char* gucQuitTextOk   = "Принята квитанция\0";
const char* errorCrcGuc = "Данные не достоверны\0";
const char* pressEnter = "Нажмите ввод\nдля продолжения\0";

