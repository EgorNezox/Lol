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
		SmartHSState_SMART_PREPARING_PLAY,
		SmartHSState_SMART_PLAYING,
		SmartHSState_SMART_PREPARING_RECORD,
		SmartHSState_SMART_RECORDING,
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
	void processReceivedCmd(uint8_t cmd, uint8_t* data, int data_len);
	void processReceivedStatus(uint8_t* data, int data_len);
	void processReceivedStatusAsync(uint8_t* data, int data_len);
	void processHSUartPolling();
	void processCmdResponceTimeout();
	void processInitSmartHS();
	void updateState(State new_state);
	void updateStatus(Status new_status);
	void resetState();
	bool verifyHSChannels(uint8_t* data, int data_len);
	void synchronizeHSState();

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

	const Multiradio::voice_channels_table_t* ch_table;
	uint16_t ch_number;
	Multiradio::voice_channel_t ch_type;
	bool indication_enable;
	bool squelch_enable;
};

} /* namespace Headset */

#endif /* FIRMWARE_APP_HEADSET_CONTROLLER_H_ */
