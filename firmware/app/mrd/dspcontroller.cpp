/**
 ******************************************************************************
 * @file    dspcontroller.cpp
 * @author  Artem Pisarenko, PMR dept. software team, ONIIP, PJSC
 * @date    22.12.2015
 *
 * TODO: добавить сигналы о завершении операций
 ******************************************************************************
 */

#include "qm.h"
#define QMDEBUGDOMAIN	mrd
#include "qmdebug.h"
#include "qmendian.h"
#include "qmtimer.h"
#include "qmthread.h"
#include "qmiopin.h"

#include "dspcontroller.h"
#include "dsptransport.h"

#define DEFAULT_PACKET_HEADER_LEN	2 // индикатор кадра + код параметра ("адрес" на самом деле не входит сюда, это "адрес назначения" из канального уровня)

namespace Multiradio {

struct DspCommand {
	bool in_progress;
	bool sync_next;
	DspController::Module module;
	int code;
	DspController::ParameterValue value;
};

DspController::DspController(int uart_resource, int reset_iopin_resource, QmObject *parent) :
	QmObject(parent),
	is_ready(false)
{
	pending_command = new DspCommand;
	startup_timer = new QmTimer(true, this);
	startup_timer->timeout.connect(sigc::mem_fun(this, &DspController::processStartupTimeout));
	command_timer = new QmTimer(true, this);
	command_timer->setInterval(100);
	command_timer->timeout.connect(sigc::mem_fun(this, &DspController::processCommandTimeout));
	reset_iopin = new QmIopin(reset_iopin_resource, this);
	// max_tx_queue_size: 1 команда радиотракта + 1 запас
	transport = new DspTransport(uart_resource, 2, this);
	transport->receivedFrame.connect(sigc::mem_fun(this, &DspController::processReceivedFrame));
	initResetState();
}

DspController::~DspController()
{
	delete pending_command;
}

void DspController::reset() {
	is_ready = false;
	initResetState();
	reset_iopin->writeOutput(QmIopin::Level_Low);
	transport->flush();
	QmThread::msleep(10);
	reset_iopin->writeOutput(QmIopin::Level_High);
	startup_timer->start(5000);
}

void DspController::setRadioParameters(RadioMode mode, uint32_t frequency) {
	bool processing_required = true;
	QM_ASSERT(is_ready);
	switch (radio_state) {
	case radiostateSync: {
		switch (current_radio_direction) {
		case RadioDirectionInvalid:
			radio_state = radiostateCmdRxFreq;
			break;
		case RadioDirectionRx:
			radio_state = radiostateCmdModeOffRx;
			break;
		case RadioDirectionTx:
			radio_state = radiostateCmdModeOffTx;
			break;
		}
		break;
	}
	case radiostateCmdRxFreq:
	case radiostateCmdTxFreq: {
		radio_state = radiostateCmdRxFreq;
		break;
	}
	case radiostateCmdRxMode: {
		radio_state = radiostateCmdModeOffRx;
		break;
	}
	case radiostateCmdTxMode: {
		radio_state = radiostateCmdModeOffTx;
		break;
	}
	default:
		processing_required = false;
		break;
	}
	current_radio_mode = mode;
	current_radio_frequency = frequency;
	if (processing_required)
		processRadioState();
}

void DspController::setRadioRx() {
	QM_ASSERT(is_ready);
	if (current_radio_direction == RadioDirectionRx)
		return;
	bool processing_required = true;
	QM_ASSERT(is_ready);
	switch (radio_state) {
	case radiostateSync:
	case radiostateCmdTxMode:
		if (current_radio_direction == RadioDirectionInvalid)
			radio_state = radiostateCmdRxMode;
		else
			radio_state = radiostateCmdTxOff;
		break;
	default:
		processing_required = false;
		break;
	}
	current_radio_direction = RadioDirectionRx;
	if (processing_required)
		processRadioState();
}

void DspController::setRadioTx() {
	QM_ASSERT(is_ready);
	if (current_radio_direction == RadioDirectionTx)
		return;
	bool processing_required = true;
	QM_ASSERT(is_ready);
	switch (radio_state) {
	case radiostateSync:
	case radiostateCmdRxMode:
		if (current_radio_direction == RadioDirectionInvalid)
			radio_state = radiostateCmdTxMode;
		else
			radio_state = radiostateCmdRxOff;
		break;
	default:
		processing_required = false;
		break;
	}
	current_radio_direction = RadioDirectionTx;
	if (processing_required)
		processRadioState();
}

void DspController::initResetState() {
	radio_state = radiostateSync;
	current_radio_direction = RadioDirectionInvalid;
	current_radio_mode = RadioModeOff;
	current_radio_frequency = 0;
	pending_command->in_progress = false;
}

void DspController::processStartup(uint16_t id, uint16_t major_version, uint16_t minor_version) {
	if (!is_ready) {
		qmDebugMessage(QmDebug::Info, "DSP started (id=0x%02X, version=%u.%u)", id, major_version, minor_version);
		startup_timer->stop();
		is_ready = true;
	} else {
		qmDebugMessage(QmDebug::Warning, "DSP restart detected");
		initResetState();
	}
	started();
}

void DspController::processStartupTimeout() {
	qmDebugMessage(QmDebug::Warning, "DSP startup timeout");
	is_ready = true;
	started();
}

void DspController::processCommandTimeout() {
	QM_ASSERT(pending_command->in_progress);
	qmDebugMessage(QmDebug::Warning, "dsp response timed out");
	syncPendingCommand();
}

void DspController::processCommandResponse(bool success, Module module, int code, ParameterValue value) {
	QM_UNUSED(value);
	if(!pending_command->in_progress) {
		qmDebugMessage(QmDebug::Warning, "dsp response, but no command was sent");
		return;
	}
	if ((module == pending_command->module) && (code == pending_command->code)) {
		command_timer->stop();
		if (!success)
			qmDebugMessage(QmDebug::Info, "dsp command failed (module=0x%02X, code=0x%02X)", module, code);
		syncPendingCommand();
	} else {
		qmDebugMessage(QmDebug::Warning, "dsp command response was unexpected (module=0x%02X, code=0x%02X)", module, code);
	}
}

void DspController::syncPendingCommand() {
	pending_command->in_progress = false;
	switch (pending_command->module) {
	case RxRadiopath:
	case TxRadiopath:
		if (pending_command->sync_next)
			syncNextRadioState();
		processRadioState();
		break;
	}
}

void DspController::processRadioState() {
	if (pending_command->in_progress) {
		pending_command->sync_next = false;
		return;
	}
	ParameterValue command_value;
	switch (radio_state) {
	case radiostateSync:
		break;
	case radiostateCmdRxFreq: {
		command_value.frequency = current_radio_frequency;
		sendCommand(RxRadiopath, RxFrequency, command_value);
		break;
	}
	case radiostateCmdTxFreq: {
		command_value.frequency = current_radio_frequency;
		sendCommand(TxRadiopath, TxFrequency, command_value);
		break;
	}
	case radiostateCmdModeOffRx:
	case radiostateCmdRxOff: {
		command_value.radio_mode = RadioModeOff;
		sendCommand(RxRadiopath, RxRadioMode, command_value);
		break;
	}
	case radiostateCmdModeOffTx:
	case radiostateCmdTxOff: {
		command_value.radio_mode = RadioModeOff;
		sendCommand(TxRadiopath, TxRadioMode, command_value);
		break;
	}
	case radiostateCmdRxMode: {
		command_value.radio_mode = current_radio_mode;
		sendCommand(RxRadiopath, RxRadioMode, command_value);
		break;
	}
	case radiostateCmdTxMode: {
		command_value.radio_mode = current_radio_mode;
		sendCommand(TxRadiopath, TxRadioMode, command_value);
		break;
	}
	}
}

void DspController::syncNextRadioState() {
	switch (radio_state) {
	case radiostateSync:
		QM_ASSERT(0);
		break;
	case radiostateCmdModeOffRx:
	case radiostateCmdModeOffTx: {
		radio_state = radiostateCmdRxFreq;
		break;
	}
	case radiostateCmdRxFreq: {
		radio_state = radiostateCmdTxFreq;
		break;
	}
	case radiostateCmdTxFreq:
	case radiostateCmdRxOff:
	case radiostateCmdTxOff: {
		switch (current_radio_direction) {
		case RadioDirectionInvalid:
			radio_state = radiostateSync;
			break;
		case RadioDirectionRx:
			radio_state = radiostateCmdRxMode;
			break;
		case RadioDirectionTx:
			radio_state = radiostateCmdTxMode;
			break;
		}
		break;
	}
	case radiostateCmdRxMode:
	case radiostateCmdTxMode: {
		radio_state = radiostateSync;
		break;
	}
	}
}

void DspController::sendCommand(Module module, int code, ParameterValue value) {
	uint8_t tx_address;
	uint8_t tx_data[DspTransport::MAX_FRAME_DATA_LEN];
	int tx_data_len = DEFAULT_PACKET_HEADER_LEN;
	qmToBigEndian((uint8_t)2, tx_data+0); // индикатор: "команда (установка)"
	qmToBigEndian((uint8_t)code, tx_data+1); // код параметра
	switch (module) {
	case RxRadiopath:
	case TxRadiopath: {
		if (module == RxRadiopath)
			tx_address = 0x50;
		else
			tx_address = 0x80;
		switch (code) {
		case 1:
			qmToBigEndian(value.frequency, tx_data+tx_data_len);
			tx_data_len += 4;
			break;
		case 2:
			qmToBigEndian((uint8_t)value.radio_mode, tx_data+tx_data_len);
			tx_data_len += 1;
			break;
		default: QM_ASSERT(0);
		}
		break;
	}
	default: QM_ASSERT(0);
	}
	QM_ASSERT(pending_command->in_progress == false);
	pending_command->in_progress = true;
	pending_command->sync_next = true;
	pending_command->module = module;
	pending_command->code = code;
	pending_command->value = value;
	transport->transmitFrame(tx_address, tx_data, tx_data_len);
	command_timer->start();
}

void DspController::processReceivedFrame(uint8_t address, uint8_t* data, int data_len) {
	if (data_len < DEFAULT_PACKET_HEADER_LEN)
		return;
	uint8_t indicator = qmFromBigEndian<uint8_t>(data+0);
	uint8_t code = qmFromBigEndian<uint8_t>(data+1);
	uint8_t *value_ptr = data + 2;
	int value_len = data_len - 2;
	switch (address) {
	case 0x11: {
		if ((indicator == 5) && (code == 2) && (value_len == 6)) // инициативное сообщение с цифровой информацией о прошивке ?
			processStartup(qmFromBigEndian<uint16_t>(value_ptr+0), qmFromBigEndian<uint16_t>(value_ptr+2), qmFromBigEndian<uint16_t>(value_ptr+4));
		break;
	}
	case 0x51:
	case 0x81: {
		if ((indicator == 3) || (indicator == 4)) { // "команда выполнена", "команда не выполнена" ?
			ParameterValue value;
			if ((code == 1) && (value_len == 4)) {
				value.frequency = qmFromBigEndian<uint32_t>(value_ptr+0);
			} else if ((code == 2) && (value_len == 1)) {
				value.radio_mode = (RadioMode)qmFromBigEndian<uint8_t>(value_ptr+0);
			} else {
				break;
			}
			Module module;
			if (address == 0x51)
				module = RxRadiopath;
			else
				module = TxRadiopath;
			processCommandResponse((indicator == 3), module, code, value);
		}
		break;
	}
	default: break;
	}
}

} /* namespace Multiradio */
