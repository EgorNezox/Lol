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

class Controller :public QmObject {
public:
	enum Status {
		StatusNone,
		StatusSmartOk,
		StatusSmartMalfunction,
		StatusAnalog
	};
	struct SmartStatusDescription {
		bool channels_mismatch;
	};
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

	Controller(int rs232_uart_resource, int ptt_iopin_resource);
	~Controller();
	void startServicing(const Multiradio::voice_channels_table_t &local_channels_table);
	Status getStatus();
	bool getSmartStatus(SmartStatusDescription &description);
	bool getAnalogStatus(bool &open_channels_missing);
	bool getPTTState(bool &state);
	bool getSmartCurrentChannel(int &number, Multiradio::voice_channel_t &type);

	void startSmartPlay(uint8_t channel);
	void stopSmartPlay();
	void startSmartRecord(uint8_t channel);
	void stopSmartRecord();
	SmartHSState getSmartHSState();
	Multiradio::voice_message_t getRecordedSmartMessage();
	void setSmartMessageToPlay(Multiradio::voice_message_t data);

	bool smartChannelType();


	sigc::signal<void, Status/*new_status*/> statusChanged;
	sigc::signal<bool/*accepted*/, bool/*new_state*/> pttStateChanged; // single connection (returns value)
	sigc::signal<void, int/*new_channel_number*/, Multiradio::voice_channel_t/*new_channel_type*/> smartCurrentChannelChanged;
	sigc::signal<void, SmartHSState/*new_state*/> smartHSStateChanged;

private:
	enum State {
		StateNone,
		StateAnalog,
		StateSmartInitChList,
		StateSmartInitHSModeSetting,
		StateSmartOk,
		StateSmartMalfunction
	};

	void processPttStateChanged();
	void processPttDebounceTimeout();
	void processPttResponseTimeout();
	void transmitCmd(uint8_t cmd, uint8_t *data, int data_len);
	void transmitResponceCmd(uint8_t cmd, uint8_t *data, int data_len);
	void processCmdResponceTimeout();
	void repeatLastCmd();
	void processHSUartPolling();
	void processReceivedCmd(uint8_t cmd, uint8_t* data, int data_len);
	void processReceivedStatus(uint8_t* data, int data_len);
	void processReceivedStatusAsync(uint8_t* data, int data_len);
	void updateState(State new_state);
	void updateStatus(Status new_status);
	void resetState();
	bool verifyHSChannels(uint8_t* data, int data_len);
	void synchronizeHSState();
	void setSmartHSState(SmartHSState state);
	void startMessagePlay();
	void sendHSMessageData();
	void messageToPlayDataPack();
	void startMessageRecord();
	void messagePacketReceived(uint8_t* data, int data_len);
	void messagePacketResponce(int packet_number);
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

	const Multiradio::voice_channels_table_t* ch_table;
	uint16_t ch_number;
	Multiradio::voice_channel_t ch_type;
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
