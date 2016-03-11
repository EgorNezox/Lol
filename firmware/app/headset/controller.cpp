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

#include "controller.h"
#include "smarttransport.h"

namespace Headset {

#define HS_CMD_STATUS				0xB0
#define HS_CMD_SET_MODE				0xB1
#define HS_CMD_CH_LIST				0xB3
#define HS_CMD_PTT_STATE			0x4A

#define HS_CMD_STATUS_RESP_LEN		32
#define HS_CMD_CH_LIST_RESP_LEN		25
#define HS_CMD_PTT_STATE_RESP_LEN	0

#define HS_CHANNELS_COUNT			98

#define PTT_DEBOUNCE_TIMEOUT		50
#define HS_UART_POLLING_INTERVAL	1000
#define HS_RESPONCE_TIMEOUT			30

Controller::Controller(int rs232_uart_resource, int ptt_iopin_resource) :
	state(StateNone), status(StatusNone), ptt_pressed(false),
	ch_number(1), ch_type(Multiradio::channelInvalid),
	indication_enable(true), squelch_enable(false)
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
		transport->transmitCmd(HS_CMD_PTT_STATE, &data, 1);
		ptt_resp_timer->start();
	}
}

void Controller::processPttResponseTimeout() {
	qmDebugMessage(QmDebug::Info, "ptt response timeout");
	resetState();
}

void Controller::transmitCmd(uint8_t cmd, uint8_t *data, int data_len) {
	transport->transmitCmd(cmd, data, data_len);
	cmd_resp_timer->start();
	qmDebugMessage(QmDebug::Dump, "cmd_resp_timer started");
}

void Controller::processReceivedCmd(uint8_t cmd, uint8_t* data, int data_len) {
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
	default:
		qmDebugMessage(QmDebug::Dump, "receved unknown cmd 0x%02X", cmd);
	}
}

void Controller::processReceivedStatus(uint8_t* data, int data_len) {
	switch (state) {
	case StateNone: {
		poll_timer->stop();
		updateState(StateSmartInitChList);
		transmitCmd(HS_CMD_CH_LIST, NULL, 0);
		break;
	}
	case StateSmartInitHSModeSetting: {
//		poll_timer->start();
//		break;
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
		case 5: break;
		default: qmDebugMessage(QmDebug::Dump, "headset channel speed is not match local");
		}
		if (ch_number != chan_number || ch_type != chan_type) {
			ch_number = chan_number;
			ch_type = chan_type;
			updateState(StateSmartOk);
			smartCurrentChannelChanged(ch_number, ch_type);
			//TODO: transmit 0xB1
		} else {
			updateState(StateSmartOk);
		}
		indication_enable = (mode_mask_add & 0x01) == 0;
		squelch_enable = ((mode_mask_add >> 1) & 0x01) == 0;
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
		case 5: break;
		default: qmDebugMessage(QmDebug::Dump, "headset channel speed is not match local");
		}
		if (ch_number != chan_number || ch_type != chan_type) {
			ch_number = chan_number;
			ch_type = chan_type;
			updateState(StateSmartOk);
			smartCurrentChannelChanged(ch_number, ch_type);
			//TODO: transmit 0xB1
			synchronizeHSState();
		} else {
			updateState(StateSmartOk);
		}
		indication_enable = (mode_mask_add & 0x01) == 0;
		squelch_enable = ((mode_mask_add >> 1) & 0x01) == 0;
		break;
	}
	default:
		break;
	}
}

void Controller::processHSUartPolling() {
	transmitCmd(HS_CMD_STATUS, NULL, 0);
}

void Controller::processCmdResponceTimeout() {
	qmDebugMessage(QmDebug::Warning, "cmd response timeout");
	resetState();
}

void Controller::updateState(State new_state) {
	if (state != new_state) {
		qmDebugMessage(QmDebug::Dump, "state changed: %d", new_state);
		state = new_state;
		switch (state) {
		case StateNone: updateStatus(StatusNone); break;
		case StateAnalog: updateStatus(StatusAnalog); break;
		case StateSmartInitChList: break;
		case StateSmartInitHSModeSetting: break;
		case StateSmartOk: updateStatus(StatusSmartOk); break;
		case StateSmartMalfunction: updateStatus(StatusSmartMalfunction); break;
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
	qmDebugMessage(QmDebug::Dump, "synchronizeHSState()");
	qmDebugMessage(QmDebug::Dump, "channel number: %d", ch_number);
	uint8_t data[16];
	data[0] = 0xAB;
	data[1] = 0xBA;
	switch (ch_table->at(ch_number - 1).speed) {
	case Multiradio::voicespeed600: data[2] = 0x02; break;
	case Multiradio::voicespeed1200: data[2] = 0x06; break;
	case Multiradio::voicespeed2400: data[2] = 0x08; break;
	case Multiradio::voicespeed4800: data[2] = 0x0B; break;
	default: data[2] = 0x02; break;
	}
	data[2] |= (uint8_t)((squelch_enable ? 0 : 1) << 5); // mode mask
	data[3] = ch_number; // channel number
	data[4] = 0xFF; // reserved
	data[5] = 0x01 | (uint8_t)((indication_enable ? 0 : 1) << 2);
	for (int i = 6; i < 16; ++i)
		data[i] = 0;
	transmitCmd(HS_CMD_SET_MODE, data, 16);
}

} /* namespace Headset */

#include "qmdebug_domains_start.h"
QMDEBUG_DEFINE_DOMAIN(hscontroller, LevelVerbose)
#include "qmdebug_domains_end.h"
