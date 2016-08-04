/**
 ******************************************************************************
 * @file    Controller.cpp
 * @author  Artem Pisarenko, PMR dept. software team, ONIIP, PJSC
 * @author  Petr Dmitriev
 * @date    29.10.2015
 *
 ******************************************************************************
 */

#define QMDEBUGDOMAIN	hscontroller

#include "qm.h"
#include "qmdebug.h"
#include "qmendian.h"
#include "qmpushbuttonkey.h"
#include "qmtimer.h"

#include "headsetcrc.h"
#include "controller.h"
#include "smarttransport.h"

namespace Headset {

#define HS_CMD_STATUS				0xB0
#define HS_CMD_SET_MODE				0xB1
#define HS_CMD_CH_LIST				0xB3
#define HS_CMD_PTT_STATE			0x4A
#define HS_CMD_MESSAGE_UPLOAD		0x2A
#define HS_CMD_MESSAGE_DOWNLOAD		0xC2

#define HS_CMD_STATUS_RESP_LEN		32
#define HS_CMD_CH_LIST_RESP_LEN		25
#define HS_CMD_PTT_STATE_RESP_LEN	0
#define HS_CMD_MESSAGE_LEN			208

#define HS_MAX_CMD_REPEATS			10

#define HS_CHANNELS_COUNT			98

#define PTT_DEBOUNCE_TIMEOUT		50
#define HS_UART_POLLING_INTERVAL	1000
#define HS_RESPONCE_TIMEOUT			100

Controller::Controller(int rs232_uart_resource, int ptt_iopin_resource) :
	state(StateNone), status(StatusNone), ptt_pressed(false),
	cmd_repeats_counter(0),
	ch_number(1), ch_type(Multiradio::channelInvalid),
	indication_enable(true), squelch_enable(false),
	hs_state(SmartHSState_SMART_NOT_CONNECTED),
	message_record_data_ready(false),
	message_to_play_data_packets_sent(0),
	message_to_play_last_packet_data_size(0)
{
	ptt_key = new QmPushButtonKey(ptt_iopin_resource);
	ptt_debounce_timer = new QmTimer(true, this);
	ptt_debounce_timer->timeout.connect(sigc::mem_fun(this, &Controller::processPttDebounceTimeout));
	ptt_debounce_timer->setInterval(PTT_DEBOUNCE_TIMEOUT);
	ptt_resp_timer = new QmTimer(true, this);
	ptt_resp_timer->timeout.connect(sigc::mem_fun(this, &Controller::processPttResponseTimeout));
	ptt_resp_timer->setInterval(HS_RESPONCE_TIMEOUT);
	transport = new SmartTransport(rs232_uart_resource, 1, this);
	transport->receivedCmd.connect(sigc::mem_fun(this, &Controller::processReceivedCmd));
	poll_timer = new QmTimer(false, this);
	poll_timer->timeout.connect(sigc::mem_fun(this, &Controller::processHSUartPolling));
	poll_timer->setInterval(HS_UART_POLLING_INTERVAL);
	cmd_resp_timer = new QmTimer(true, this);
	cmd_resp_timer->timeout.connect(sigc::mem_fun(this, &Controller::processCmdResponceTimeout));
	cmd_resp_timer->setInterval(HS_RESPONCE_TIMEOUT);
}

Controller::~Controller() {
	//...
}

void Controller::startServicing(const Multiradio::voice_channels_table_t& local_channels_table) {
	QM_ASSERT(local_channels_table.size() > 0);
	qmDebugMessage(QmDebug::Info, "start servicing...");
	ch_table = &local_channels_table;
	ch_number = 1;
	ch_type = ch_table->at(0).type;
	transport->enable();
	poll_timer->start();
	ptt_pressed = ptt_key->isPressed();
	ptt_key->stateChanged.connect(sigc::mem_fun(this, &Controller::processPttStateChanged));
}

Controller::Status Controller::getStatus() {
	return status;
}

bool Controller::getSmartStatus(SmartStatusDescription& description) {
	description.channels_mismatch = false; //TODO:
	return state == StateSmartOk;
}

bool Controller::getAnalogStatus(bool& open_channels_missing) {
	open_channels_missing = false;
	return state == StateAnalog;
}

bool Controller::getPTTState(bool& state) {
	state = ptt_pressed;
	return this->state == StateAnalog || this->state == StateSmartOk;
}

bool Controller::getSmartCurrentChannel(int& number, Multiradio::voice_channel_t &type) {
	number = ch_number;
	type = ch_type;
	return state == StateSmartOk;
}

void Controller::processPttStateChanged() {
	ptt_debounce_timer->start();
}

void Controller::processPttDebounceTimeout() {
	if (ptt_pressed != ptt_key->isPressed()) {
		ptt_pressed = !ptt_pressed;
		qmDebugMessage(QmDebug::Dump, "ptt state changed: %d", ptt_pressed);
		uint8_t data = ptt_pressed ? 0xFF : 0;
		qmDebugMessage(QmDebug::Dump, "transmit HS_CMD_PTT_STATE: 0x%02X", data);
		++cmd_repeats_counter;
		transport->transmitCmd(HS_CMD_PTT_STATE, &data, 1);
		ptt_resp_timer->start();
	}
}

void Controller::processPttResponseTimeout() {
	qmDebugMessage(QmDebug::Info, "ptt response timeout, repeat %d", cmd_repeats_counter);
	if (cmd_repeats_counter < HS_MAX_CMD_REPEATS) {
		repeatLastCmd();
	} else {
		resetState();
	}
}

void Controller::transmitCmd(uint8_t cmd, uint8_t *data, int data_len) {
	QM_ASSERT(data_len <= SmartTransport::MAX_FRAME_DATA_SIZE);
	++cmd_repeats_counter;
	transport->transmitCmd(cmd, data, data_len);
	cmd_resp_timer->start();
	qmDebugMessage(QmDebug::Dump, "cmd_resp_timer started");
}

void Controller::transmitResponceCmd(uint8_t cmd, uint8_t *data, int data_len) {
	transport->transmitCmd(cmd, data, data_len);
}

void Controller::processCmdResponceTimeout() {
	qmDebugMessage(QmDebug::Info, "cmd response timeout, repeat %d", cmd_repeats_counter);
	if (cmd_repeats_counter < HS_MAX_CMD_REPEATS) {
		repeatLastCmd();
	} else {
		resetState();
	}
}

void Controller::repeatLastCmd() {
	++cmd_repeats_counter;
	transport->repeatLastCmd();
	cmd_resp_timer->start();
}

void Controller::processHSUartPolling() {
	transmitCmd(HS_CMD_STATUS, NULL, 0);
}

void Controller::processReceivedCmd(uint8_t cmd, uint8_t* data, int data_len) {
	qmDebugMessage(QmDebug::Dump, "processReceivedCmd() cmd = 0x%2X, data_len = %d", cmd, data_len);
	cmd_repeats_counter = 0;
	switch (cmd) {
	case HS_CMD_STATUS: {
		qmDebugMessage(QmDebug::Dump, "cmd_resp_timer(%p): %d", cmd_resp_timer, cmd_resp_timer->isActive());
		if (state != StateSmartOk && !cmd_resp_timer->isActive()) {
			qmDebugMessage(QmDebug::Dump, "unexpected cmd HS_CMD_STATUS");
			break;
		}
		if (state == StateSmartOk && !cmd_resp_timer->isActive()) {
			qmDebugMessage(QmDebug::Dump, "asynchronous cmd HS_CMD_STATUS");
			processReceivedStatusAsync(data, data_len);
			break;
		}
		cmd_resp_timer->stop();
		if (data_len == 0) { //идентичный отправленному пакет
			cmd_resp_timer->stop();
			if (state == StateNone) {
				updateState(StateAnalog);
			} else if (state == StateSmartOk) {
				updateState(StateNone);
			}
		} else if (data_len == HS_CMD_STATUS_RESP_LEN) {
			processReceivedStatus(data, data_len);
		} else {
			cmd_resp_timer->stop();
			qmDebugMessage(QmDebug::Dump, "wrong data length of responce to cmd HS_CMD_STATUS");
			resetState();
		}
		break;
	}
	case HS_CMD_CH_LIST: {
		cmd_resp_timer->stop();
		if (state != StateSmartInitChList) {
			qmDebugMessage(QmDebug::Dump, "unexpected cmd HS_CMD_CH_LIST");
			resetState();
			break;
		} else if (data_len != HS_CMD_CH_LIST_RESP_LEN) {
			qmDebugMessage(QmDebug::Dump, "wrong data length of responce to cmd HS_CMD_CH_LIST");
			resetState();
			updateState(StateSmartMalfunction);
			break;
		} else if (!verifyHSChannels(data, data_len)) {
			qmDebugMessage(QmDebug::Warning, "Headset channels is not match to local channels table");
			resetState();
			updateState(StateSmartMalfunction);
			break;
		}
		updateState(StateSmartInitHSModeSetting);
		synchronizeHSState();
		break;
	}
	case HS_CMD_PTT_STATE: {
		ptt_resp_timer->stop();
		qmDebugMessage(QmDebug::Dump, "ptt response received");
		if (ptt_pressed == ptt_key->isPressed()) {
			bool accepted = pttStateChanged(ptt_pressed);
			qmDebugMessage(QmDebug::Dump, "ptt state accepted: %d", accepted);
		}
		break;
	}
	case HS_CMD_MESSAGE_DOWNLOAD: {
		cmd_resp_timer->stop();
		switch (hs_state) {
		case SmartHSState_SMART_RECORD_DOWNLOADING:
			if (message_to_play_data_packets_sent == message_to_play_data_packets.size()) {
				setSmartHSState(SmartHSState_SMART_PLAYING);
			} else {
				sendHSMessageData();
			}
			break;
		default: break;
		}
		break;
	}
	case HS_CMD_MESSAGE_UPLOAD: {
		cmd_resp_timer->stop();
		if (hs_state == SmartHSState_SMART_RECORDING) {
			setSmartHSState(SmartHSState_SMART_RECORD_UPLOADING);
		} else if (hs_state != SmartHSState_SMART_RECORD_UPLOADING) {
			break;
		}
		qmDebugMessage(QmDebug::Dump, "message packet received");
		if (data_len != HS_CMD_MESSAGE_LEN) {
			qmDebugMessage(QmDebug::Dump, "wrong data length of cmd HS_CMD_MESSAGE_RECORD");
			break;
		}
		messagePacketReceived(data, data_len);
		break;
	}
	default:
		qmDebugMessage(QmDebug::Dump, "receved unknown cmd 0x%02X", cmd);
	}
}

void Controller::processReceivedStatus(uint8_t* data, int data_len) {
	qmDebugMessage(QmDebug::Dump, "processReceivedStatus()");
	switch (state) {
	case StateNone: {
		poll_timer->stop();
		updateState(StateSmartInitChList);
		transmitCmd(HS_CMD_CH_LIST, NULL, 0);
		break;
	}
	case StateSmartInitHSModeSetting: {
//		poll_timer->start();
//		no break;
	}
	case StateSmartOk: {
		uint16_t chan_number = qmFromLittleEndian<uint16_t>(data + 4);
		uint8_t ch_mask = data[8];
		uint8_t mode_mask = data[9];
		uint8_t mode_mask_add = data[10];
		uint8_t error_status = data[16] & 0x1;
		qmDebugMessage(QmDebug::Dump, "chan_number: %d", chan_number);
		qmDebugMessage(QmDebug::Dump, "ch_mask: 0x%X", ch_mask);
		qmDebugMessage(QmDebug::Dump, "mode_mask: 0x%X", mode_mask);
		qmDebugMessage(QmDebug::Dump, "mode_mask_add: 0x%X", mode_mask_add);
		if (error_status) {
			qmDebugMessage(QmDebug::Dump, "smart headset error");
			updateState(StateSmartMalfunction);
			break;
		}
		Multiradio::voice_channel_t chan_type = Multiradio::channelInvalid;
		switch (ch_mask & 0x03) {
		case 0: chan_type = Multiradio::channelOpen; break;
		case 1: chan_type = Multiradio::channelClose; break;
		}
		switch ((ch_mask & 0x1C) >> 2) {
		case 1:
		case 3:
		case 4:
		case 5:
			break;
		default: qmDebugMessage(QmDebug::Dump, "headset channel speed is not match local, %d", (ch_mask & 0x1C) >> 2);
		}
		qmDebugMessage(QmDebug::Dump, "headset channel speed = %d", ((ch_mask & 0x1C) >> 2));
		if (ch_number != chan_number || ch_type != chan_type) {
			ch_number = chan_number;
			ch_type = chan_type;
			updateState(StateSmartOk);
			smartCurrentChannelChanged(ch_number, ch_type);
		} else {
			updateState(StateSmartOk);
		}
		indication_enable = (mode_mask_add & 0x01) == 0;
		squelch_enable = ((mode_mask_add >> 1) & 0x01) == 0;

		switch (hs_state) {
		case SmartHSState_SMART_PREPARING_RECORD_SETTING_CHANNEL: {
			startMessageRecord();
			break;
		}
		case SmartHSState_SMART_PREPARING_RECORD_SETTING_MODE: {
			cmd_resp_timer->setInterval(HS_RESPONCE_TIMEOUT); //TODO: fix it
			if (mode_mask & 0x10) {
				setSmartHSState(SmartHSState_SMART_RECORDING);
			} else {
				setSmartHSState(SmartHSState_SMART_ERROR);
			}
			break;
		}
		case SmartHSState_SMART_PREPARING_PLAY_SETTING_CHANNEL: {
			startMessagePlay();
			break;
		}
		case SmartHSState_SMART_PREPARING_PLAY_SETTING_MODE: {
			if (mode_mask & 0x40) {
				setSmartHSState(SmartHSState_SMART_RECORD_DOWNLOADING);
				sendHSMessageData();
			} else {
				setSmartHSState(SmartHSState_SMART_ERROR);
				resetState();
			}
			break;
		}
		default: break;
		}
		break;
	}
	case StateSmartInitChList: {
		qmDebugMessage(QmDebug::Dump, "unexpected cmd HS_CMD_STATUS");
		resetState();
		break;
	}
	default:
		QM_ASSERT(0);
	}
}

void Controller::processReceivedStatusAsync(uint8_t* data, int data_len) {
	qmDebugMessage(QmDebug::Dump, "processReceivedStatusAsync()");
	switch (state) {
	case StateSmartOk: {
		uint16_t chan_number = qmFromLittleEndian<uint16_t>(data + 4);
		uint8_t ch_mask = data[8];
		uint8_t mode_mask = data[9];
		uint8_t mode_mask_add = data[10];
		uint8_t error_status = data[16] & 0x1;
		qmDebugMessage(QmDebug::Dump, "chan_number: %d", chan_number);
		qmDebugMessage(QmDebug::Dump, "ch_mask: 0x%X", ch_mask);
		qmDebugMessage(QmDebug::Dump, "mode_mask: 0x%X", mode_mask);
		qmDebugMessage(QmDebug::Dump, "mode_mask_add: 0x%X", mode_mask_add);
		if (error_status) {
			qmDebugMessage(QmDebug::Dump, "smart headset error");
			updateState(StateSmartMalfunction);
			break;
		}
		Multiradio::voice_channel_t chan_type = Multiradio::channelInvalid;
		switch (ch_mask & 0x03) {
		case 0: chan_type = Multiradio::channelOpen; break;
		case 1: chan_type = Multiradio::channelClose; break;
		}
		switch ((ch_mask & 0x1C) >> 2) {
		case 1:
		case 3:
		case 4:
		case 5:
			break;
		default: qmDebugMessage(QmDebug::Dump, "headset channel speed is not match local, %d", (ch_mask & 0x1C) >> 2);
		}
		qmDebugMessage(QmDebug::Dump, "headset channel speed = %d", ((ch_mask & 0x1C) >> 2));
		if (ch_number != chan_number || ch_type != chan_type) {
			ch_number = chan_number;
			ch_type = chan_type;
			updateState(StateSmartOk);
			smartCurrentChannelChanged(ch_number, ch_type);
			synchronizeHSState();
		} else {
			updateState(StateSmartOk);
		}
		indication_enable = (mode_mask_add & 0x01) == 0;
		squelch_enable = ((mode_mask_add >> 1) & 0x01) == 0;
		switch (hs_state) {
		case SmartHSState_SMART_PLAYING: {
			if (!(mode_mask & 0x40)) {
				setSmartHSState(SmartHSState_SMART_READY);
			}
			break;
		}
		default: break;
		}
		break;
	}
	default:
		break;
	}
}

void Controller::updateState(State new_state) {
	if (state != new_state) {
		qmDebugMessage(QmDebug::Dump, "state changed: %d", new_state);
		state = new_state;
		switch (state) {
		case StateNone:
			updateStatus(StatusNone);
			setSmartHSState(SmartHSState_SMART_NOT_CONNECTED);
			break;
		case StateAnalog: updateStatus(StatusAnalog); break;
		case StateSmartInitChList: break;
		case StateSmartInitHSModeSetting: break;
		case StateSmartOk:
			updateStatus(StatusSmartOk);
			setSmartHSState(SmartHSState_SMART_READY);
			break;
		case StateSmartMalfunction:
			updateStatus(StatusSmartMalfunction);
			setSmartHSState(SmartHSState_SMART_ERROR);
			break;
		default: QM_ASSERT(0);
		}
	}
}

void Controller::updateStatus(Status new_status) {
	if (status != new_status) {
		qmDebugMessage(QmDebug::Dump, "status changed: %d", new_status);
		status = new_status;
		statusChanged(new_status);
	}
}

void Controller::resetState() {
	updateState(StateNone);
	cmd_repeats_counter = 0;
	message_to_play_data_packets_sent = 0;
	if (!poll_timer->isActive())
		poll_timer->start();
}

bool Controller::verifyHSChannels(uint8_t* data, int data_len) {
	if (ch_table->size() != HS_CHANNELS_COUNT)
		return false;
//	qmDebugMessage(QmDebug::Dump, "Channels types:");
//	int type_[4] = {0, 0, 0, 0};
//	for (int i = 0; i < 25; ++i) {
//		type_[3] = (data[i] >> 6) & 0x03;
//		type_[2] = (data[i] >> 4) & 0x03;
//		type_[1] = (data[i] >> 2) & 0x03;
//		type_[0] = data[i] & 0x03;
//		for (int j = 0; j < 4; ++j) {
//			qmDebugMessage(QmDebug::Dump, "%d) %d", i * 4 + j, type_[j]);
//		}
//	}
	int type[4];
	for (int byte_ind = 0; byte_ind < 25; ++byte_ind) {
		type[0] = data[byte_ind] & 0x03;
		type[1] = (data[byte_ind] >> 2) & 0x03;
		type[2] = (data[byte_ind] >> 4) & 0x03;
		type[3] = (data[byte_ind] >> 6) & 0x03;
		for (int bit_ind = 0; bit_ind < 4; ++bit_ind) {
			if ((byte_ind == 0 && bit_ind == 0) || (byte_ind == 24 && bit_ind == 3))
				continue;
			switch (type[bit_ind]) {
			case 0:
				if (ch_table->at(byte_ind * 4 + bit_ind - 1).type != Multiradio::channelOpen)
					return false;
				break;
			case 1:
				if (ch_table->at(byte_ind * 4 + bit_ind - 1).type != Multiradio::channelClose)
					return false;
				break;
			case 2:
				if (ch_table->at(byte_ind * 4 + bit_ind - 1).type != Multiradio::channelInvalid)
					return false;
				break;
			default:
				return false;
			}
		}
	}
	return true;
}

void Controller::synchronizeHSState() {
	qmDebugMessage(QmDebug::Dump, "synchronizeHSState() channel number: %d", ch_number);
	const int data_size = 16;
	uint8_t data[data_size];
	data[0] = 0xAB;
	data[1] = 0xBA;
	data[2] = 0x01;
	switch (ch_table->at(ch_number - 1).speed) {
	case Multiradio::voicespeed600: data[2] |= 0x02; break;
	case Multiradio::voicespeed1200: data[2] |= 0x06; break;
	case Multiradio::voicespeed2400: data[2] |= 0x08; break;
	case Multiradio::voicespeed4800: data[2] |= 0x0B; break;
	default: data[2] |= 0x02; break;
	}
	data[2] |= (uint8_t)((squelch_enable ? 0 : 1) << 5); // mode mask
	data[3] = ch_number; // channel number
	data[4] = 0xFF; // reserved
	data[5] = 0x01 | (uint8_t)((indication_enable ? 0 : 1) << 2);
	for (int i = 6; i < data_size; ++i)
		data[i] = 0;
	transmitCmd(HS_CMD_SET_MODE, data, data_size);
}

void Controller::startSmartPlay(uint8_t channel) {
	QM_ASSERT(channel > 0 || channel < 99);
	if (ch_table->at(channel - 1).type != Multiradio::channelClose) {
		qmDebugMessage(QmDebug::Warning, "startSmartPlay() channel %d is not close", channel);
		setSmartHSState(SmartHSState_SMART_BAD_CHANNEL);
		return;
	}
	qmDebugMessage(QmDebug::Dump, "startSmartPlay() channel %d", channel);
	if (message_to_play_data.size() == 0) {
		qmDebugMessage(QmDebug::Warning, "startSmartPlay() message is empty");
		setSmartHSState(SmartHSState_SMART_EMPTY_MESSAGE);
		return;
	}
	if (hs_state != SmartHSState_SMART_READY) {
		qmDebugMessage(QmDebug::Dump, "startSmartPlay() unexpected state");
		return;
	}
	setSmartHSState(SmartHSState_SMART_PREPARING_PLAY_SETTING_CHANNEL);
	messageToPlayDataPack();
	ch_number = channel;
	synchronizeHSState();
}

void Controller::startMessagePlay() {
	qmDebugMessage(QmDebug::Warning, "startMessagePlay()");
	setSmartHSState(SmartHSState_SMART_PREPARING_PLAY_SETTING_MODE);
	message_to_play_data_packets_sent = 0;
	const int data_size = 16;
	uint8_t data[data_size];
	data[0] = 0xAB;
	data[1] = 0xBA;
	data[2] = 0x01;
	data[2] |= 0x40;
	data[2] |= (uint8_t)((squelch_enable ? 0 : 1) << 5); // mode mask
	data[3] = 0; // channel number
	data[4] = 0xFF; // reserved
	data[5] = 0x01 | (uint8_t)((indication_enable ? 0 : 1) << 2);
	for (int i = 6; i < data_size; ++i)
		data[i] = 0;
	cmd_resp_timer->setInterval(3000);
	transmitCmd(HS_CMD_SET_MODE, data, data_size);
}

void Controller::stopSmartPlay() {
	qmDebugMessage(QmDebug::Dump, "stopSmartPlay()");
	setSmartHSState(SmartHSState_SMART_READY);
	synchronizeHSState();
}

void Controller::startSmartRecord(uint8_t channel) {
	QM_ASSERT(channel > 0 || channel < 99);
	if (ch_table->at(channel - 1).type != Multiradio::channelClose) {
		qmDebugMessage(QmDebug::Warning, "startSmartRecord() channel %d is not close", channel);
		setSmartHSState(SmartHSState_SMART_BAD_CHANNEL);
		return;
	}
	if (hs_state != SmartHSState_SMART_READY) {
		qmDebugMessage(QmDebug::Dump, "startSmartRecord() unexpected state");
		return;
	}
	qmDebugMessage(QmDebug::Dump, "startSmartRecord() channel %d", channel);
	message_record_data_ready = false;
	message_record_data.clear();
	setSmartHSState(SmartHSState_SMART_PREPARING_RECORD_SETTING_CHANNEL);
	ch_number = channel;
	synchronizeHSState();
}

void Controller::startMessageRecord() {
	qmDebugMessage(QmDebug::Dump, "startMessageRecord()");
	cmd_resp_timer->setInterval(3000); //TODO: fix it
	setSmartHSState(SmartHSState_SMART_PREPARING_RECORD_SETTING_MODE);
	const int data_size = 16;
	uint8_t data[data_size];
	data[0] = 0xAB;
	data[1] = 0xBA;
	data[2] = 0x01;
	data[2] |= 0x10;
	data[2] |= (uint8_t)((squelch_enable ? 0 : 1) << 5); // mode mask
	data[3] = 0x00; // channel number
	data[4] = 0xFF; // reserved
	data[5] = 0x01 | (uint8_t)((indication_enable ? 0 : 1) << 2);
	for (int i = 6; i < data_size; ++i)
		data[i] = 0;
	transmitCmd(HS_CMD_SET_MODE, data, data_size);
}

void Controller::stopSmartRecord() {
	qmDebugMessage(QmDebug::Dump, "stopSmartRecord()");
	setSmartHSState(SmartHSState_SMART_READY);
	synchronizeHSState();
}

Controller::SmartHSState Controller::getSmartHSState() {
    return hs_state;
}

Multiradio::voice_message_t Controller::getRecordedSmartMessage() {
	if (message_record_data_ready) {
		return message_record_data;
	}
	return Multiradio::voice_message_t();
}

void Controller::setSmartMessageToPlay(Multiradio::voice_message_t data) {
	message_to_play_data = data;
}

void Controller::setSmartHSState(SmartHSState state) {
	qmDebugMessage(QmDebug::Dump, "setSmartHSState() state = %d", state);
	hs_state = state;
	smartHSStateChanged(hs_state);
}

void Controller::sendHSMessageData() {
	QM_ASSERT(message_to_play_data.size() != 0);
	qmDebugMessage(QmDebug::Dump, "sendHSMessageData()");
	const int data_size = 208;
	uint8_t data[data_size];
	data[0] = 0xAA;
	data[1] = 0xCD;
	qmToLittleEndian(message_to_play_data_packets_sent, &data[2]);
	if (message_to_play_data_packets_sent == message_to_play_data_packets.size() - 1) {
		qmToLittleEndian((uint16_t)0, &data[4]);
	} else if (message_to_play_data_packets_sent == message_to_play_data_packets.size() - 2) {
		qmToLittleEndian((uint16_t)message_to_play_last_packet_data_size, &data[4]);
	} else {
		qmToLittleEndian((uint16_t)message_to_play_data_packets.at(message_to_play_data_packets_sent).size(), &data[4]);
	}
	memcpy(&data[6], message_to_play_data_packets.at(message_to_play_data_packets_sent).data(),
			message_to_play_data_packets.at(message_to_play_data_packets_sent).size());
	qmToLittleEndian(calcPacketCrc(&data[2], 204), &data[206]);
//	for (int i = 0; i < data_size; ++i) {
//		qmDebugMessage(QmDebug::Dump, "data[%d] = %2X", i, data[i]);
//	}
	transmitCmd(HS_CMD_MESSAGE_DOWNLOAD, data, data_size);
	++message_to_play_data_packets_sent;
}

void Controller::messageToPlayDataPack() {
	QM_ASSERT(message_to_play_data.size() != 0);
	message_to_play_data_packets.clear();
	message_to_play_last_packet_data_size = message_to_play_data.size() % 200;
	if (message_to_play_data.size() <= 200) {
		message_to_play_data_packets.push_back(message_to_play_data);
		for (int i = message_to_play_data.size(); i < 200; ++i) {
			message_to_play_data_packets.at(0).push_back(0);
		}
	} else {
		const size_t data_size = message_to_play_data.size();
		for (size_t i = 0; i < data_size; ++i) {
			if (i % 200 == 0) {
				message_to_play_data_packets.push_back(Multiradio::voice_message_t());
			}
			message_to_play_data_packets.at(i / 200).push_back(message_to_play_data.at(i));
		}
		if (message_to_play_data_packets.at(message_to_play_data_packets.size() - 1).size() != 200) {
			const int last = message_to_play_data_packets.size() - 1;
			for (int i = message_to_play_data_packets.at(message_to_play_data_packets.size() - 1).size(); i < 200; ++i) {
				message_to_play_data_packets.at(last).push_back(0);
			}
		}
	}
	message_to_play_data_packets.push_back(Multiradio::voice_message_t());
	const int last = message_to_play_data_packets.size() - 1;
	for (int i = 0; i < 200; ++i) {
		message_to_play_data_packets.at(last).push_back(0);
	}
	qmDebugMessage(QmDebug::Dump, "messageToPlayDataPack() packets number = %d", message_to_play_data_packets.size());
}

void Controller::messagePacketReceived(uint8_t* data, int data_len) {
	qmDebugMessage(QmDebug::Dump, "messagePacketReceived() data_len = %d", data_len);
//	for (int i = 0; i < data_len; ++i) {
//		qmDebugMessage(QmDebug::Dump, "data[%d] = %2X", i, data[i]);
//	}
	if (data[0] != 0xAA && data[1] != 0xCD) {
		qmDebugMessage(QmDebug::Dump, "messagePacketReceived() data header is not valid");
		return;
	}
	uint16_t data_crc = calcPacketCrc(&data[2], 204);
	uint16_t crc = qmFromLittleEndian<uint16_t>(data + 206);
	if (data_crc != crc) {
		qmDebugMessage(QmDebug::Dump, "messagePacketReceived() crc is not valid");
		return;
	}
	uint16_t packet_number = qmFromLittleEndian<uint16_t>(data + 2);
	uint16_t data_size = qmFromLittleEndian<uint16_t>(data + 4);
	if (data_size > 200) {
		qmDebugMessage(QmDebug::Dump, "messagePacketReceived() data size %d is not valid", data_size);
		return;
	} else if (data_size == 0) {
		qmDebugMessage(QmDebug::Dump, "total message data size = %d", message_record_data.size());
		message_record_data_ready = true;
		setSmartHSState(SmartHSState_SMART_READY);
	}
	for (int i = 6; i < data_size + 6; ++i) {
		message_record_data.push_back(data[i]);
	}
	messagePacketResponce(packet_number);
}

void Controller::messagePacketResponce(int packet_number) {
	qmDebugMessage(QmDebug::Dump, "messagePacketResponce() packet_number %d", packet_number);
	const int data_size = 4;
	uint8_t data[data_size];
	for (int i = 0; i < data_size; ++i) {
		data[i] = 0;
	}
	data[2] = (uint8_t)packet_number;
	transmitResponceCmd(HS_CMD_MESSAGE_UPLOAD, data, data_size);
}

uint16_t Controller::calcPacketCrc(uint8_t* data, int data_len) {
	HeadsetCRC crc;
	crc.update(data, (uint16_t)data_len);
	uint16_t result = ~crc.result();
	uint16_t least = (result & 0x00FF);
	result = (result >> 8) & 0x00FF;
	result |= (least << 8) & 0xFF00;
	return result;
}

bool Controller::smartChannelType(){
	bool res = false;
	res = (ch_type == Multiradio::channelOpen)  ?  true : false;
    return res;
}

bool Controller::getMainLabelStatus(int value)
{
    statusMainLabel = ((value == 1) ? true : false); // 0 -active, 1- disable
    return statusMainLabel;
}

} /* namespace Headset */

#include "qmdebug_domains_start.h"
QMDEBUG_DEFINE_DOMAIN(hscontroller, LevelDefault)
#include "qmdebug_domains_end.h"
