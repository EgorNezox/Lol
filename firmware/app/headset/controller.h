/**
 ******************************************************************************
 * @file    Controller.h
 * @author  Artem Pisarenko, PMR dept. software team, ONIIP, PJSC
 * @author  Petr Dmitriev
 * @date    29.10.2015
 *
 ******************************************************************************
 */

#ifndef FIRMWARE_APP_HEADSET_CONTROLLER_H_
#define FIRMWARE_APP_HEADSET_CONTROLLER_H_

#include <vector>
#include "qmobject.h"
#include "multiradio.h"
#include <qmtimer.h>

class QmPushButtonKey;
class QmTimer;

namespace Headset {

class SmartTransport;

/*!
 * \brief Класс, реализующий логику работы с гарнитурой (полевая, СКЗИ).
 * \detailed Детектирует статус подключения гарнитуры, тип подключенной гарнитуры. Выполняет инициализацию
 * гарнитуры СКЗИ при включении.
 */
class Controller :public QmObject {
public:
	/*!< Статус подключения гарнитуры */
	enum Status {
		StatusNone,
		StatusSmartOk,
		StatusSmartMalfunction,
		StatusAnalog
	};
	/*!< Статус гарнитуры СКЗИ */
	struct SmartStatusDescription {
		bool channels_mismatch;
	};
	/*!< Состояние функции отложенный речи рагнитуры СКЗИ */
	enum SmartHSState {
		SmartHSState_SMART_EMPTY_MESSAGE,
		SmartHSState_SMART_NOT_CONNECTED,
		SmartHSState_SMART_ERROR,
		SmartHSState_SMART_BAD_CHANNEL,
		SmartHSState_SMART_PREPARING_PLAY_SETTING_CHANNEL,
		SmartHSState_SMART_PREPARING_PLAY_SETTING_MODE,
		SmartHSState_SMART_RECORD_DOWNLOADING,
		SmartHSState_SMART_PLAYING,
		SmartHSState_SMART_PREPARING_RECORD_SETTING_CHANNEL,
		SmartHSState_SMART_PREPARING_RECORD_SETTING_MODE,
		SmartHSState_SMART_RECORDING,
		SmartHSState_SMART_RECORD_UPLOADING,
		SmartHSState_SMART_RECORD_TIMEOUT,
		SmartHSState_SMART_READY
	};

	/*!
	 * \param rs232_uart_resource идентификатор аппаратного ресурса UART порта подключения гарнитуры
	 * \param ptt_iopin_resource идентификатор аппаратного ресурса тангенты гарнитуры
	 */
	Controller(int rs232_uart_resource, int ptt_iopin_resource);
	~Controller();
	/*!
	 * \brief Включает обработку событий
	 * \detailed До вызова \a startServicing() обработка событий отключена
	 */
	void startServicing(const Multiradio::voice_channels_table_t &local_channels_table);
	/*!
	 * \brief Возвращает статус подключения гарнитуры
	 * \return статус подключения гарнитуры
	 */
	Status getStatus();
	/*!
	 * \brief Возвращает статус гарнитуры СКЗИ
	 * \param description статус гарнитуры СКЗИ
	 * \return статус операции
	 */
	bool getSmartStatus(SmartStatusDescription &description);
	/*!
	 * \brief Возвращает статус полевой гарнитуры
	 * \param open_channels_missing статус полевой гарнитуры
	 * \return статус операции
	 */
	bool getAnalogStatus(bool &open_channels_missing);
	/*!
	 * \brief Возвращает состояние тангенты гарнитуры
	 * \param state состояние тангенты гарнитуры
	 * \return статус операции
	 */
	bool getPTTState(bool &state);
	/*!
	 * \brief Возвращает параметры текущего канала гарнитуры СКЗИ
	 * \param number номер канала
	 * \param type тип канала
	 * \return статус операции
	 */
	bool getSmartCurrentChannel(int &number, Multiradio::voice_channel_t &type);
	/*!
	 * \brief РЈСЃС‚Р°РЅР°РІР»РёРІР°РµС‚ СЃРєРѕСЂРѕСЃС‚СЊ С‚РµРєСѓС‰РµРіРѕ РєР°РЅР°Р»Р° РіР°СЂРЅРёС‚СѓСЂС‹ РЎРљР—Р
	 * \param speed СЃРєРѕСЂРѕСЃС‚СЊ
	 * \return СЃС‚Р°С‚СѓСЃ РѕРїРµСЂР°С†РёРё
	 */
	bool setSmartCurrentChannelSpeed(Multiradio::voice_channel_speed_t speed);
	/*!
	 * \brief Инициирует воспроизведение сообщения на гарнитуре СКЗИ
	 * \detailed Воспроизводимое сообщение должно быть предварительно установлено вызовом \a setSmartMessageToPlay()
	 * \param channel номер канала воспроизведения
	 */
	void startSmartPlay(uint8_t channel);
	/*!
	 * \brief Останавливает воспроизведение сообщения на гарнитуре СКЗИ
	 */
	void stopSmartPlay();
	/*!
	 * \brief Инициирует запись сообщения на гарнитуре СКЗИ
	 * \param channel номер канала воспроизведения
	 */
	void startSmartRecord(uint8_t channel);
	/*!
	 * \brief Останавливает запись сообщения на гарнитуре СКЗИ
	 */
	void stopSmartRecord();
	/*!
	 * \brief Возвращает состояние функции отложенный речи рагнитуры СКЗИ
	 * \return состояние функции отложенный речи рагнитуры СКЗИ
	 */
	SmartHSState getSmartHSState();
	/*!
	 * \brief Возвращает записанное сообщение
	 * \return записанное сообщение
	 */
	Multiradio::voice_message_t getRecordedSmartMessage();
	/*!
	 * \brief Устанавливает сообщение для последующего воспроизведения на гарнитуре СКЗИ
	 * \param data сообщение для воспроизведения
	 */
	void setSmartMessageToPlay(Multiradio::voice_message_t data);


	/*!
	 * \brief Сигнал изменения статуса подключения гарнитуры
	 * \param new_status статус подключения гарнитуры
	 */
	sigc::signal<void, Status/*new_status*/> statusChanged;
	/*!
	 * \brief Сигнал изменения состояния тангенты гарнитуры
	 * \detailed Поддерживает только одно соединение (возвращает значение)
	 * \param new_state состояние тангенты гарнитуры
	 * \return статус подтверждения обработки сигнала
	 */
	sigc::signal<bool/*accepted*/, bool/*new_state*/> pttStateChanged;
	/*!
	 * \brief Сигнал изменения активного канала гарнитуры СКЗИ
	 * \param new_channel_number номер канала
	 * \param new_channel_type тип канала
	 */
	sigc::signal<void, int/*new_channel_number*/, Multiradio::voice_channel_t/*new_channel_type*/> smartCurrentChannelChanged;
	/*!
	 * \brief Сигнал изменения состояния функции отложенный речи рагнитуры СКЗИ
	 * \param new_state состояние функции отложенный речи рагнитуры СКЗИ
	 */
	sigc::signal<void, SmartHSState/*new_state*/> smartHSStateChanged;

	void GarnitureStart();

private:
	/*!< Состояние подключения гарнитуры */
	enum State {
		StateNone,
		StateAnalog,
		StateSmartInitChList,
		StateSmartInitHSModeSetting,
		StateSmartOk,
		StateSmartMalfunction
	};

	/*!
	 * \brief Обработчик изменения состояния тангенты гарнитуры
	 */
	void processPttStateChanged();
	/*!
	 * \brief Обработчик таймаута дребезга тангенты гарнитуры
	 */
	void processPttDebounceTimeout();
	/*!
	 * \brief Обработчик таймаута ответа гарнитуры
	 * \detailed Актуально для всех типов гарнитур. Гарнитура СКЗИ ответит корректным кадром.
	 * Полевая гарнитура замыкает TX на RX, т.о. ответ придет автоматически.
	 */
	void processPttResponseTimeout();
	/*!
	 * \brief Отправляет команду (ожидается ответ)
	 * \param cmd команда
	 * \param data данные
	 * \param data_len размер данных
	 */
	void transmitCmd(uint8_t cmd, uint8_t *data, int data_len);
	/*!
	 * \brief Отправляет ответ на команду
	 * \param cmd команда-ответ
	 * \param data данные
	 * \param data_len размер данных
	 */
	void transmitResponceCmd(uint8_t cmd, uint8_t *data, int data_len);
	/*!
	 * \brief Обработчик таймаута ожидания ответа на команду
	 */
	void processCmdResponceTimeout();
	/*!
	 * \brief Повторная передача последней переданной команды
	 */
	void repeatLastCmd();
	/*!
	 * \brief Периодический опрос UART порта для детектирования статуса подключения гарнитуры
	 */
	void processHSUartPolling();
	/*!
	 * \brief Обработчик полученной команды
	 * \param cmd команда
	 * \param data данные
	 * \param data_len размер данных
	 */
	void processReceivedCmd(uint8_t cmd, uint8_t* data, int data_len);
	/*!
	 * \brief Обработчик полученной команды статуса в ответ на отправленную
	 * \param data данные
	 * \param data_len размер данных
	 */
	void processReceivedStatus(uint8_t* data, int data_len);
	/*!
	 * \brief Обработчик полученной команды статуса инициативно от гарнитуры
	 * \param data данные
	 * \param data_len размер данных
	 */
	void processReceivedStatusAsync(uint8_t* data, int data_len);
	/*!
	 * \brief Устанавливает состояние подключения гарнитуры
	 * \param new_state состояние подключения гарнитуры
	 */
	void updateState(State new_state);
	/*!
	 * \brief Устанавливает статус подключения гарнитуры
	 * \param new_state статус подключения гарнитуры
	 */
	void updateStatus(Status new_status);
	/*!
	 * \brief Сброс всех состояний и статусов
	 */
	void resetState();
	/*!
	 * \brief Проверяет соответствие локального и гарнитурного (СКЗИ) списка каналов
	 * \param data данные
	 * \param data_len размер данных
	 * \return статус соответствия
	 */
	bool verifyHSChannels(uint8_t* data, int data_len);
	/*!
	 * \brief Отправляет команду уставки статуса гарнитуры в соответствии с актуальным состоянием радиостанции
	 */
	void synchronizeHSState();
	/*!
	 * \brief Устанавливает состояние функции отложенный речи рагнитуры СКЗИ
	 * \param состояние функции отложенный речи рагнитуры СКЗИ
	 */
	void setSmartHSState(SmartHSState state);
	/*!
	 * \brief Устанавливает режим воспроизведения сообщения на рагнитуре СКЗИ
	 */
	void startMessagePlay();
	/*!
	 * \brief Отправляет пакет данных сообщения
	 */
	void sendHSMessageData();
	/*!
	 * \brief Разбивает сообщение для воспроизведения на пакеты
	 * \detailed Сообщение должно быть предварительно установлено вызовом \a setSmartMessageToPlay()
	 */
	void messageToPlayDataPack();
	/*!
	 * \brief Устанавливает режим записи сообщения на рагнитуре СКЗИ
	 */
	void startMessageRecord();
	/*!
	 * \brief Обработчик принятого пакета записанного сообщения
	 * \param data данные
	 * \param data_len размер данных
	 */
	void messagePacketReceived(uint8_t* data, int data_len);
	/*!
	 * \brief Отправляет ответ на полученный пакет записанного сообщения
	 * \param packet_number номер пакета
	 */
	void messagePacketResponce(int packet_number);
	/*!
	 * \brief Вычисляет контрольную сумму CRC16 данных
	 * \param data данные
	 * \param data_len размер данных
	 * \return CRC16 данных
	 */
	uint16_t calcPacketCrc(uint8_t* data, int data_len);

	bool isSmartHSStateChange = false;
	void checkUpdateSmartHSState();

	State state;
	Status status;
	QmPushButtonKey* ptt_key;
	bool ptt_pressed;
	bool updated_ptt_pressed;
	QmTimer* ptt_debounce_timer;
	QmTimer* ptt_resp_timer;
	SmartTransport* transport;
	QmTimer* poll_timer;
	QmTimer* cmd_resp_timer;
	int cmd_repeats_counter;

	SmartStatusDescription smart_status_description;
	const Multiradio::voice_channels_table_t* ch_table;
	uint16_t ch_number;
	Multiradio::voice_channel_t ch_type;
	Multiradio::voice_channel_speed_t ch_speed;
	bool indication_enable;
	bool squelch_enable;

	SmartHSState hs_state;
	Multiradio::voice_message_t message_to_play_data;
	Multiradio::voice_message_t message_record_data;
	bool message_record_data_ready;

	std::vector<Multiradio::voice_message_t> message_to_play_data_packets;
	uint16_t message_to_play_data_packets_sent;
	uint8_t message_to_play_last_packet_data_size;

	bool minimal_activity_mode;
};

} /* namespace Headset */

#endif /* FIRMWARE_APP_HEADSET_CONTROLLER_H_ */
