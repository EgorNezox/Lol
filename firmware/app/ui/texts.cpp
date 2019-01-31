#include "texts.h"

const char *errorStr = "Ошибка\0";
char *mode_txt[] = {" Авто\0", "Ручн.\0"};
char *disabled_ch_txt = (char *)"--";
char ch_open_letter='О';
char ch_zero_letter='0';
char ch_closed_letter='З';
char ch_invalid_letter='x';

const char* ch_em_type_str[] = {(char *)"F3E\0", (char*)"J3E\0",(char*)"В_П\0",(char*)"Ч_П\0"};

const char* exitStr = "Выход\0";

const char* freq_khz = "кГц\0";
const char* freq_hz = "Гц\0";

const char* speed_bit = "бит/с\0";

const char proc = '%';

const char* trans = "Отправить\0";
const char* callTitle[2] = {"Команда\0", "Адрес\0"};
const char* yesNo[] = {"\t\tНЕТ\0", "\t\tДА\0"};

char * ok_texts[LANG_COUNT]={(char *)"Ок"};
char * missing_ch_table_txt[LANG_COUNT]={(char *)"Отсутствует таблица\nречевых каналов\0"};
char * missing_open_ch_txt[LANG_COUNT]={(char *)"Отсутствуют открытые\nречевые каналы\0"};
char * ch_table_mismatch_txt[LANG_COUNT]={(char *)"Несоответствие\nтаблицы речевых\nканалов списку\nканалов гарнитуры\0"};

const char* receiveStr = "Принять\0";
const char* continueStr = "Продолжить\0";
const char* receiveStatusStr[] = {"  Начать\n  прием\0", "   Идет\n  прием\0"};

const char *mainMenuFull[] = {"ГЛАВНОЕ МЕНЮ\0", " Передача\0", " Прием\0", " Данные\0", " Настройки\0"};
const char *mainMenu[]     = {"ГЛАВНОЕ МЕНЮ\0", " Перед.\0", " Прием\0", " Данные\0", " Настр.\0"};

const char *callSubMenu[] = {" УК\0", " СМС\0", " ГП\0", " ГУК\0"};
const char *commandsSubMenu[] = {" Одност.связь\0", " Двухст.связь\0"};
const char *smplSubMenu[] = {"Групповой\0", "Индивид.\0", "  Индивид.\n   с квит.\0"};
const char *twSubMenu[] = {" Без ретранслятора\0", " С ретранслятором\0"};
const char *smsSubMenu[] = {" Без ретранслятора\0", " С ретранслятором\0"};
const char *groupCommandsSubMenu[] = {" Одност.связь\0", " Двухст.связь\0"};
const char *groupCommandsSimplSubMenu[] = {" Групповой вызов\0", " Индивидуальный\0"};

const char *reciveSubMenu[] = {" Речь\0", " СМС\0", " ГП\0", " УК\0", " ГУК\0"};
//const char *tmpParsing[] = {" Речь\0", " УК\0", " СМС\0", " ГП\0", " ГУК\0"};
const char *tmpParsing[] = {" Речь\0", " УК\0", " ГУК\0", " СМС\0", " ГП\0"};
const char *fileRxTx[] = {" ПРМ\0", " ПРД\0"};

const char *dataSubMenu[] = {" Принятые\0", " Отправленные\0", " Сохраненые\0", " GPS координаты\0"};
const char *dataSubSubMenu[] = {" Условные команды\0", " СМС\0", " Головая почта\0", " Группа усл.команд\0"};

const char *settingsSubMenu[] = {" Дат.Вр.\0", " Связь\0", " Скан.\0", " Режим\0", " Звук\0",
                                 " Шум\0"," Время ГУК\0", " Кл. сети"," Конфиг."," Коорд",
                                 " Дисплей\0", " Технол.\0", " Канал\0" }; //TODO:

const char *settingsSubMenuIn[] = {" Дата Время\0", " Связь\0", " Скан.\0", " Режим\0", " Звук\0",
                                 " Шумоподавитель\0"," Время ГУК\0", " Ключ сети"," Конфигуратор"," Координаты",
                                 " Дисплей\0", " Технол.меню\0", " Запомнить\n канал:\0"  };

const char *technoSubMenu[]   = {" Генер.\0", " Адрес\0", " Версия\0", " ГукВвод\0", "Формат.\0"};
const char *technoSubMenuIn[] = {" Настр.генератора\0", " Адрес станции\0", " Версия ПО\0", "Размер команд ГУК\0", "Флэш\0"};

const char *dateAndTimeSubMenu[] = {" Синхро\0", " Ручной\0"};
const char *setDateOrTime[] = {" Дата\0", " Время\0"};
//const char *setDateOrTime[] = {" Ввод даты\0", " Ввод время\0"};
const char *setConnParam[] = {" Частота\0", " Скор.\0", " Излуч.\0", " Речь\0"};
const char *files[] = {" Файлы\0"};

const char *coordinateStr = "Координаты\0";
const char *coordNotExistStr = "Координаты\n отсутствуют\0";
//const char *coordNotExistStr = "Координат\n нет\0";
const char *notExistStr = "отсутств.\0";
const char *useScanMenu[] = {"ВЫКЛ\0", "ВКЛ\0", "МАЛ\0","СРЕД\0","ВЫС\0"};
const char *aruStr = "АРУ\0";
const char *armStr = "АРМ\0";
const char *ausStr = "АУC\0";
const char *error_SWF = "ТЕСТ ПРОЙДЕН\0";
const char *true_SWF = "ТЕСТ ПРОЙДЕН\0";

const char *smsText[] = {"Адрес: \0", "Сообщение:\n\0"};

const char *sms_quit_fail1 = "Нет ответа\0";
const char *sms_quit_fail2 = "Квитанция потеряна\0";
//const char *sms_crc_fail = "Пакет доставлен с ошибками\0";
const char *sms_crc_fail = "Пакет дост. с ошиб.\0";

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

//const char* recvCondCommand = "Принята условная команда\0";
const char* recvCondCommand = "Принята усл. ком.\0";

const char* groupCondCommFreqStr = "Частота \0";
const char* schedulerFreqStr = "Частота Гц\0";
const char* dataAndTime[] = {"Дата\0", "Время\0"};

//const char* putOffVoiceStageOneStr[] = {"Нажмите ввод для записи\0", "Ожидание записи\0"};
//const char* putOffVoiceStageTwoStr[] = {"Нажмите ввод для завершения\0", "Запись речи\0"};
const char* putOffVoiceStageOneStr[] = {"Записать\0", "Ожид. зап.\0"};
const char* putOffVoiceStageTwoStr[] = {"Завершить\0", "Запись речи\0"};

//const char* startAleTxVoiceMailStr = "Нажмите\nВвод для\nзапуска\0";
//const char* startAleTxVoiceMailStr = "Нажмите Ввод для запуска\n передачи.\0";
const char* startAleTxVoiceMailStr = "Запустить\n передачу\0";

//const char* smatrHSStateStr[] = {
//    "Воспроизвести\nневозможно.\nПовторите.\0",
//    "Воспроизвести\nневозможно.\nНет гарнитуры\0",
//    " Ошибка\n гарнитуры.\0",
//    "Ошибка канала\n Выберите\n другой канал.\0",
//    " Подготовка\n сообщения.\n Ожидайте.\0",
//    "Проигрывание\nсообщения.\nОжидайте.\0",
//    " Отправка\n записи сообщ.\n Ожидайте.\0",
//    "Проигрывание\n cообщения.\n Ожидайте.\0",
//    " Подготовка\n записи.\n Ожидайте.\0",
//    " Подготовка\n записи.\n Ожидайте.\0",
//    " Удерживайте\n тангенту\n для записи\0",
//    " Получение\n записи.\n Ожидайте.\0",
//    " Истекло\n время\n ожидания.\0",
//    " Готово.\0",
//    "14","15","16","17","18","19","20"
//};

const char* smatrHSStateStr[] = {
    "Воспроизв.\nневозм.\nПовторите.\0",
    "Воспроизв.\nневозм. Нет\n гарнитуры\0",
    " Ошибка\n гарнит.\0",
    "Ошиб. кан.\n Выберите\n др. канал\0",
    " Подгот.\n сообщ.\n Ожидайте.\0",
    "Проигрыв.\nсообщения.\nОжидайте.\0",
    " Отправка\n зап. сообщ.\n Ожидайте\0",
    "Проигрыв.\n cообщ.\n Ожидайте\0",
    " Подготов.\n записи.\n Ожидайте\0",
    " Подготов.\n записи.\n Ожидайте\0",
    " Удержив.\n тангенту\n для записи\0",
    " Получение\n записи.\n Ожидайте\0",
    " Истекло\n время\n ожидания\0",
    " Готово.\0",
    "14","15","16","17","18","19","20"
};

//const char* openChannelPlayErrorStr = "Проиграть\nневозможно.\nОткрытый канал.\0";
//const char* noHeadsetPlayErrorStr   = "Проиграть\nневозможно.\nНет гарнитуры.\0";
const char* openChannelPlayErrorStr = "Проигр.\nневозмож.\nОткр. кан.\0";
const char* noHeadsetPlayErrorStr   = "Проигр.\nневозмож.\nНет гарн.\0";

const char* aleStateStr[] = {
/* 0 */			"\0",
/* 1 */		    "Нет\nсинхрониз.\0",
/* 2 */		    "Аппаратный\n сбой\0",
/* 3 */		    "Нет частот\n АУС\n по умолчанию\0",
/* 4 */		    "Не задан\n адрес\n станции\0",
/* 5 */		    "Ошибка.\n Необх.\nсинхрониз.\0",
/* 6 */		    "Ожидание\n вызова...\0",
/* 7 */		    "Прин. выз.\n Настройка\0",
/* 8 */		    "Вход.\n сигнал\n прерван.\n Повторите.\0",
/* 9 */		    "Прием ГП\0",                            //"<AleVmProgress>%\0",
/* 10 */	    "Исходящий\n вызов\0",
/* 11 */	    "Нет связи.\n Повторите.\0",
/* 12 */	    "Выз. прин.\nСогласов.\n обмена.\0",
/* 13 */	    "Передача ГП.\0",                         //"<AleVmProgress>%\0",
/* 14 */	    "Нет связи.\n Повторите.\0",
/* 15 */	    "Сообщение\nпотеряно.\nПовторите.\0",
/* 16 */	    "Передача\n успешно\n завершена.\0",
/* 17 */	    "Прием сооб.\n прерван.\n Повторите.\0",
/* 18 */	    "Нет связи.\n Повторите.\0",
/* 19 */	    "ГП принята\nс ошибками.\0",
/* 20 */	    "Принята ГП.\n Ввод для\n продолжения.\0",
/* 21 */		"Передача\n зондов.\0",
/* 22 */		"Прием\n оценки\n зондов.\0",
/* 23 */		"Прием\n зондов.\0",
/* 24 */		"Передача\n оценки\n зондов.\0",
/* 25 */		"Сбой\n логики\n ГП.\0",
/* 26 */		"Нет данных\n для передачи\n ГП.\0",
};

const char *smsDataInformTx[] = {
    "Вызов\nабонента\0",
    "Ожидание\nответа\0",
    "Ответ\nпринят\0",
    "Передача\nданных\0",
    "Данные\nдоставл.\0",
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

const char *smsDataInformDx[] = {
    "Неизвестное сообщение\0",
    "Начат вызов\0",
    "Ответ от станции\0",
    "Отправка данных\0",
    "Данные переданы (отправка квитанции)\0",
};
const char *rxSmsResultStatus[] = {
    "Ожидание вызова\0",
    "Получение ответа\0",
    "Прием данных\0",
    " Ожидание\n квитанции\0"
};

const char* utcStr[] = {" Пояс\0" , "Часовой пояс\0"};

const char* voiceRxStr[] = {"Запустить\n прием\0", "От:\0", "Нажм. ввод\0"};
//const char* voiceRxStr[] = {"Нажмите Ввод\n для запуска\n приема\0", "От:\0", "Нажмите ввод\0"};
const char* voiceRxTxLabelStr[] = {"Канал зап.\0", "Канал\nвос.\0", "Зап. речи\0", "Вос. речи\0", "Получат.\0", "Получ. ГП\0"};
//const char* voiceRxTxLabelStr[] = {"Канал записи\0", "Канал\nвоспроизвед.\0", "Запись речи.\0", "Воспр. речи.\0", "Получатель\0", "Получена ГП\0"};
const char* condCommStr[] = {"Адрес получателя\0", "       Адрес \n  ретранслятора\0", "Команда\0", "Ретранслятор\0", "Сообщение\0"};
const char* condCommSendStr = "Запустить \nпередачу\0";
//const char* condCommSendStr = "Нажмите Ввод для \nзапуска передачи\0";

const char* ticketStr[] = {"Квитирование\0", "Ретрансляция\0"};
//const char* voicePostTitleStr[] = {"Передача ГП\0", "Прием ГП\0"};
const char* voicePostTitleStr[] = {"Перед.\0", "Прием\0"};

const char* gucQuitTextFail = "Квитанция не принята\0";
const char* gucQuitTextOk   = "Принята квитанция\0";
const char* errorCrcGuc = "Данные не достоверны\0";
//const char* pressEnter = "Нажмите ввод\nдля продолжения\0";
const char* pressEnter = "Продолжить\0";

const char* recievedGUC_CMD = "Приняты команды\0";
const char* titleGuc        = "Сообщение\0";
const char* titleCoord      = "Получены\nкоординаты\0";

const char* startStr           = "Старт\0";

const char* typeCondCmd         = "Выберите тип\0";
const char* NoYesGucCoord[]     = {"Нет\0", "Есть\0"};
const char* StartGucTx          = "Старт\0";
const char* GucIndividGroup[]   = {"Одному\0", "Всем\0"};
const char* Sheldure            = "12 000000 Гц\n12:34 ГУК\0";
const char* Sheldure_label      = "  Расписание";
const char* editSheldure_label  = "  Редактирование";
const char* delSheldure_lable   = "  Удаление";
const char* newSheldure_label   = "  Новый сеанс";
const char* addSheldure         = " Добавить\n сеанс\0";
const char* editSheldure        = " Изменить\n сеанс\0";
const char* delSheldure         = " Удалить\n сеанс\0";
const char* askDelSheldure      = "\t\t Удалить\n\t сеанс?\0";
const char* atumalfunction_title_str = "Сбой АСУ";
const char* atumalfunction_text_str = "Перед. на тек. част. ограничена";

const char* dsphardwarefailure_7_5_title_str = "Авария АФУ";
const char* dsphardwarefailure_7_5_text_str = "0";
const char* dsphardwarefailure_unknown_title_str = "Аппаратный сбой DSP";
const char* dsphardwarefailure_unknown_text_str = "Устр. %d, код %d";

const char *StartCmd = "Передача...\0";
const char *EndCmd   = "Прием\0";
const char *EndSms   = "Сообщение доставлено\0";
const char *EndSms2  = "Сообщение не доставлено\0";

const char *coodGucStr  = "Координ.\n\0";

const char *STARTS = "Старт";

const char *recPacket = "Принята команда\0";
//const char* cmdRec   = "Принята квитанция\0";
const char* cmdRec   = "Квитанция\0";
const char *notReiableRecPacket = "Принята недостоверная команда\0";

const char *errorReadFile = "Ошибка чтения файла\0";

//const char *displayBrightnessStr[] = {"Минимальная\0","Средняя\0","Максимальная\0"};
const char *displayBrightnessStr[] = {"Минимум\0","Средняя\0","Максимум\0"};
const char *displayBrightnessTitleStr = "Яркость\0";

const char *schedulePromptStr = " минут\n до начала\n сеанса связи\n режима\0";

const char *rxtxFilesStr[] = {"Принятые\0", "Переданные\0"};

const char *rxtxFiledSmsStr[] = {"Принятый пакет\0", "Ошибка СМС\0"};

const char *rxCondErrorStr[] = {"Принятый пакет:\n\tОшибка\t\0", "Принятый пакет:\n\tНеизвестная\n\tошибка\t\0" };
const char *rnKey = "Ключ радиосети\0";

const char *waitingStr = "Ожидайте.\0";
const char *flashProcessingStr = "Идет работа\n с памятью\0";

const char *radioStationStr = "Радиостанция\0";
//const char *sazhenNameStr = "САЖЕНЬ-Н\0";

const char *fromStr = "от\0";

const char *syncWaitingStr = "Синхро...\0";

const char *txQwit  = "Отпр.\n квит.\0";

const char *err  = " Ошибка\0";
const char *errCrc  = " Ошибка CRC\0";

const char *mainScrMode[] = {"УК\0", "ГУК\0", "СМС\0", "ГП\0"};
