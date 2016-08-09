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
 * \brief �����, ����������� ������ ������ � ���������� (�������, ����).
 * \detailed ����������� ������ ����������� ���������, ��� ������������ ���������. ��������� �������������
 * ��������� ���� ��� ���������.
 */
class Controller :public QmObject {
public:
	/*!< ������ ����������� ��������� */
	enum Status {
		StatusNone,
		StatusSmartOk,
		StatusSmartMalfunction,
		StatusAnalog
	};
	/*!< ������ ��������� ���� */
	struct SmartStatusDescription {
		bool channels_mismatch;
	};
	/*!< ��������� ������� ���������� ���� ��������� ���� */
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
	 * \param rs232_uart_resource ������������� ����������� ������� UART ����� ����������� ���������
	 * \param ptt_iopin_resource ������������� ����������� ������� �������� ���������
	 */
	Controller(int rs232_uart_resource, int ptt_iopin_resource);
	~Controller();
	/*!
	 * \brief �������� ��������� �������
	 * \detailed �� ������ \a startServicing() ��������� ������� ���������
	 */
	void startServicing(const Multiradio::voice_channels_table_t &local_channels_table);
	/*!
	 * \brief ���������� ������ ����������� ���������
	 * \return ������ ����������� ���������
	 */
	Status getStatus();
	/*!
	 * \brief ���������� ������ ��������� ����
	 * \param description ������ ��������� ����
	 * \return ������ ��������
	 */
	bool getSmartStatus(SmartStatusDescription &description);
	/*!
	 * \brief ���������� ������ ������� ���������
	 * \param open_channels_missing ������ ������� ���������
	 * \return ������ ��������
	 */
	bool getAnalogStatus(bool &open_channels_missing);
	/*!
	 * \brief ���������� ��������� �������� ���������
	 * \param state ��������� �������� ���������
	 * \return ������ ��������
	 */
	bool getPTTState(bool &state);
	/*!
	 * \brief ���������� ��������� �������� ������ ��������� ����
	 * \param number ����� ������
	 * \param type ��� ������
	 * \return ������ ��������
	 */
	bool getSmartCurrentChannel(int &number, Multiradio::voice_channel_t &type);
	/*!
	 * \brief Устанавливает скорость текущего канала гарнитуры СКЗИ
	 * \param speed скорость
	 * \return статус операции
	 */
	bool setSmartCurrentChannelSpeed(Multiradio::voice_channel_speed_t speed);
	/*!
	 * \brief ���������� ��������������� ��������� �� ��������� ����
	 * \detailed ��������������� ��������� ������ ���� �������������� ����������� ������� \a setSmartMessageToPlay()
	 * \param channel ����� ������ ���������������
	 */
	void startSmartPlay(uint8_t channel);
	/*!
	 * \brief ������������� ��������������� ��������� �� ��������� ����
	 */
	void stopSmartPlay();
	/*!
	 * \brief ���������� ������ ��������� �� ��������� ����
	 * \param channel ����� ������ ���������������
	 */
	void startSmartRecord(uint8_t channel);
	/*!
	 * \brief ������������� ������ ��������� �� ��������� ����
	 */
	void stopSmartRecord();
	/*!
	 * \brief ���������� ��������� ������� ���������� ���� ��������� ����
	 * \return ��������� ������� ���������� ���� ��������� ����
	 */
	SmartHSState getSmartHSState();
	/*!
	 * \brief ���������� ���������� ���������
	 * \return ���������� ���������
	 */
	Multiradio::voice_message_t getRecordedSmartMessage();
	/*!
	 * \brief ������������� ��������� ��� ������������ ��������������� �� ��������� ����
	 * \param data ��������� ��� ���������������
	 */
	void setSmartMessageToPlay(Multiradio::voice_message_t data);

	bool smartChannelType();


	/*!
	 * \brief ������ ��������� ������� ����������� ���������
	 * \param new_status ������ ����������� ���������
	 */
	sigc::signal<void, Status/*new_status*/> statusChanged;
	/*!
	 * \brief ������ ��������� ��������� �������� ���������
	 * \detailed ������������ ������ ���� ���������� (���������� ��������)
	 * \param new_state ��������� �������� ���������
	 * \return ������ ������������� ��������� �������
	 */
	sigc::signal<bool/*accepted*/, bool/*new_state*/> pttStateChanged;
	/*!
	 * \brief ������ ��������� ��������� ������ ��������� ����
	 * \param new_channel_number ����� ������
	 * \param new_channel_type ��� ������
	 */
	sigc::signal<void, int/*new_channel_number*/, Multiradio::voice_channel_t/*new_channel_type*/> smartCurrentChannelChanged;
	/*!
	 * \brief ������ ��������� ��������� ������� ���������� ���� ��������� ����
	 * \param new_state ��������� ������� ���������� ���� ��������� ����
	 */
	sigc::signal<void, SmartHSState/*new_state*/> smartHSStateChanged;

    bool getMainLabelStatus(int);
    bool statusMainLabel = false;

private:
	/*!< ��������� ����������� ��������� */
	enum State {
		StateNone,
		StateAnalog,
		StateSmartInitChList,
		StateSmartInitHSModeSetting,
		StateSmartOk,
		StateSmartMalfunction
	};

	/*!
	 * \brief ���������� ��������� ��������� �������� ���������
	 */
	void processPttStateChanged();
	/*!
	 * \brief ���������� �������� �������� �������� ���������
	 */
	void processPttDebounceTimeout();
	/*!
	 * \brief ���������� �������� ������ ���������
	 * \detailed ��������� ��� ���� ����� ��������. ��������� ���� ������� ���������� ������.
	 * ������� ��������� �������� TX �� RX, �.�. ����� ������ �������������.
	 */
	void processPttResponseTimeout();
	/*!
	 * \brief ���������� ������� (��������� �����)
	 * \param cmd �������
	 * \param data ������
	 * \param data_len ������ ������
	 */
	void transmitCmd(uint8_t cmd, uint8_t *data, int data_len);
	/*!
	 * \brief ���������� ����� �� �������
	 * \param cmd �������-�����
	 * \param data ������
	 * \param data_len ������ ������
	 */
	void transmitResponceCmd(uint8_t cmd, uint8_t *data, int data_len);
	/*!
	 * \brief ���������� �������� �������� ������ �� �������
	 */
	void processCmdResponceTimeout();
	/*!
	 * \brief ��������� �������� ��������� ���������� �������
	 */
	void repeatLastCmd();
	/*!
	 * \brief ������������� ����� UART ����� ��� �������������� ������� ����������� ���������
	 */
	void processHSUartPolling();
	/*!
	 * \brief ���������� ���������� �������
	 * \param cmd �������
	 * \param data ������
	 * \param data_len ������ ������
	 */
	void processReceivedCmd(uint8_t cmd, uint8_t* data, int data_len);
	/*!
	 * \brief ���������� ���������� ������� ������� � ����� �� ������������
	 * \param data ������
	 * \param data_len ������ ������
	 */
	void processReceivedStatus(uint8_t* data, int data_len);
	/*!
	 * \brief ���������� ���������� ������� ������� ����������� �� ���������
	 * \param data ������
	 * \param data_len ������ ������
	 */
	void processReceivedStatusAsync(uint8_t* data, int data_len);
	/*!
	 * \brief ������������� ��������� ����������� ���������
	 * \param new_state ��������� ����������� ���������
	 */
	void updateState(State new_state);
	/*!
	 * \brief ������������� ������ ����������� ���������
	 * \param new_state ������ ����������� ���������
	 */
	void updateStatus(Status new_status);
	/*!
	 * \brief ����� ���� ��������� � ��������
	 */
	void resetState();
	/*!
	 * \brief ��������� ������������ ���������� � ������������ (����) ������ �������
	 * \param data ������
	 * \param data_len ������ ������
	 * \return ������ ������������
	 */
	bool verifyHSChannels(uint8_t* data, int data_len);
	/*!
	 * \brief ���������� ������� ������� ������� ��������� � ������������ � ���������� ���������� ������������
	 */
	void synchronizeHSState();
	/*!
	 * \brief ������������� ��������� ������� ���������� ���� ��������� ����
	 * \param ��������� ������� ���������� ���� ��������� ����
	 */
	void setSmartHSState(SmartHSState state);
	/*!
	 * \brief ������������� ����� ��������������� ��������� �� ��������� ����
	 */
	void startMessagePlay();
	/*!
	 * \brief ���������� ����� ������ ���������
	 */
	void sendHSMessageData();
	/*!
	 * \brief ��������� ��������� ��� ��������������� �� ������
	 * \detailed ��������� ������ ���� �������������� ����������� ������� \a setSmartMessageToPlay()
	 */
	void messageToPlayDataPack();
	/*!
	 * \brief ������������� ����� ������ ��������� �� ��������� ����
	 */
	void startMessageRecord();
	/*!
	 * \brief ���������� ��������� ������ ����������� ���������
	 * \param data ������
	 * \param data_len ������ ������
	 */
	void messagePacketReceived(uint8_t* data, int data_len);
	/*!
	 * \brief ���������� ����� �� ���������� ����� ����������� ���������
	 * \param packet_number ����� ������
	 */
	void messagePacketResponce(int packet_number);
	/*!
	 * \brief ��������� ����������� ����� CRC16 ������
	 * \param data ������
	 * \param data_len ������ ������
	 * \return CRC16 ������
	 */
	uint16_t calcPacketCrc(uint8_t* data, int data_len);

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
};

} /* namespace Headset */

#endif /* FIRMWARE_APP_HEADSET_CONTROLLER_H_ */
