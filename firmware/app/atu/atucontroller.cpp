/**
 ******************************************************************************
 * @file    atucontroller.cpp
 * @author  Artem Pisarenko, PMR dept. software team, ONIIP, PJSC
 * @author  неизвестные
 * @date    11.01.2016
 *
 ******************************************************************************
 */

#include <string.h>
#define QMDEBUGDOMAIN	atucontroller
#include "qmdebug.h"
#include "qmthread.h"
#include "qmtimer.h"
#include "qmuart.h"
#include "qmiopin.h"

#include "atucontroller.h"

#define MAX_FRAME_DATA_SIZE		5
#define FRAME_SYMBOL_EOT		0x03

namespace Multiradio {

AtuController::AtuController(int uart_resource, int iopin_resource, QmObject *parent) :
	QmObject(parent),
	mode(modeNone), force_next_tunetx_full(false), antenna(1),
	minimal_activity_mode(false)
{
	command.id = commandInactive;
	command.data_buf = new uint8_t[MAX_FRAME_DATA_SIZE];
	uart_rx_state = uartrxNone;
	uart_rx_frame.data_buf = new uint8_t[MAX_FRAME_DATA_SIZE];
	deferred_enterbypass_active = false;
	deferred_tunetx_active = false;
	QmUart::ConfigStruct uart_config;
	uart_config.baud_rate = 115200;
	uart_config.stop_bits = QmUart::StopBits_1;
	uart_config.parity = QmUart::Parity_Even;
	uart_config.flow_control = QmUart::FlowControl_None;
	uart_config.rx_buffer_size = 512;
	uart_config.tx_buffer_size = 512;
	uart_config.io_pending_interval = 1;
	uart = new QmUart(uart_resource, &uart_config, this);
	uart->dataReceived.connect(sigc::mem_fun(this, &AtuController::processUartReceivedData));
	scan_timer = new QmTimer(false, this);
	scan_timer->setInterval(1000);
	scan_timer->timeout.connect(sigc::mem_fun(this, &AtuController::scan));
	command_timeout_timer = new QmTimer(true, this);
	command_timeout_timer->timeout.connect(sigc::mem_fun(this, &AtuController::tryRepeatCommand));
	tx_tune_timer = new QmTimer(true, this);
	tx_tune_timer->setInterval(100);
	tx_tune_timer->timeout.connect(sigc::mem_fun(this, &AtuController::processTxTuneTimeout));
	poff_iopin = new QmIopin(iopin_resource, this);
}

AtuController::~AtuController()
{
	uart->close();
	delete[] uart_rx_frame.data_buf;
	delete[] command.data_buf;
}

void AtuController::startServicing() {
	qmDebugMessage(QmDebug::Info, "start servicing...");
	setMode(modeNone);
	command.id = commandInactive;
	uart->open();
	scan();
	scan_timer->start();
}

bool AtuController::isDeviceConnected() {
	return (mode != modeNone);
}

AtuController::Mode AtuController::getMode() {
	return mode;
}

bool AtuController::enterBypassMode(uint32_t frequency) {
	setAntenna(frequency);
	if (!((mode == modeActiveTx) || (mode == modeBypass)) || (antenna == 0))
		return false;
	if (command.id != commandInactive) {
		deferred_enterbypass_active = true;
		return true;
	}
	executeEnterBypassMode();
	return true;
}

bool AtuController::tuneTxMode(uint32_t frequency) {
	setAntenna(frequency);
	if (antenna == 0)
		return false;
	if (!((mode == modeBypass) || (mode == modeActiveTx)
			|| ((mode == modeMalfunction) && force_next_tunetx_full)))
		return false;
	tunetx_frequency = frequency;
	if (command.id != commandInactive) {
		deferred_tunetx_active = true;
		return true;
	}
	executeTuneTxMode();
	return true;
}

void AtuController::setNextTuningParams(bool force_full) {
	force_next_tunetx_full = force_full;
}

void AtuController::acknowledgeTxRequest() {
	uint8_t frameid = tx_tuning_state?frameid_U:frameid_D;
	sendFrame(frameid, 0, 0);
	tx_tuning_state = !tx_tuning_state;
	tx_tune_timer->start();
}

void AtuController::setRadioPowerOff(bool enable) {
	poff_iopin->writeOutput(enable ? QmIopin::Level_High : QmIopin::Level_Low);
	QmThread::msleep(2);
}

void AtuController::setMinimalActivityMode(bool enabled) {
	minimal_activity_mode = enabled;
	if (enabled) {
		scan_timer->stop();
		command_timeout_timer->stop();
		uart->close();
		mode = modeNone;
	} else {
		uart->open();
		scan_timer->start();
	}
}

void AtuController::setMode(Mode mode) {
	switch (mode) {
	case modeNone:
		qmDebugMessage(QmDebug::Info, "no ATU mode");
		scan_timer->start();
		break;
	case modeMalfunction:
		qmDebugMessage(QmDebug::Info, "malfunction mode");
		tx_tune_timer->stop();
		scan_timer->start();
		break;
	case modeStartingBypass:
		qmDebugMessage(QmDebug::Info, "starting bypass mode...");
		scan_timer->stop();
		break;
	case modeBypass:
		qmDebugMessage(QmDebug::Info, "bypass mode");
		scan_timer->start();
		break;
	case modeTestTuning:
		qmDebugMessage(QmDebug::Info, "test TWF for tuning tx...");
		scan_timer->stop();
		break;
	case modeStartTuning:
		qmDebugMessage(QmDebug::Info, "start tuning tx...");
		break;
	case modeTuning:
		qmDebugMessage(QmDebug::Info, "tuning tx...");
		break;
	case modeActiveTx:
		qmDebugMessage(QmDebug::Info, "active tx mode");
		scan_timer->start();
		break;
	}
	if (this->mode != mode) {
		this->mode = mode;
		modeChanged(mode);
	}
}

void AtuController::scan() {
	if (command.id != commandInactive)
		return;
	startCommand(commandRequestState, 0, 0, 0);
}

void AtuController::startCommand(CommandId id, const uint8_t* data, int data_len, int repeat_count, int timeout) {
	QM_ASSERT(data_len <= MAX_FRAME_DATA_SIZE);
	command.id = id;
	memcpy(command.data_buf, data, data_len);
	command.data_len = data_len;
	command.repeat_count = repeat_count;
	sendFrame(id, data, data_len);
	if (timeout > 0)
		command_timeout_timer->start(timeout);
}

void AtuController::finishCommand() {
	command_timeout_timer->stop();
	command.id = commandInactive;
}

void AtuController::tryRepeatCommand() {
	if (command.repeat_count > 0) {
		sendFrame(command.id, command.data_buf, command.data_len);
		command_timeout_timer->start();
		command.repeat_count--;
		return;
	}
	qmDebugMessage(QmDebug::Info, "command failed (no response)");
	command.id = commandInactive;
	deferred_enterbypass_active = false;
	deferred_tunetx_active = false;
	if (mode == modeNone) {
		if (!scan_timer->isActive())
			scan_timer->start();
		return;
	}
	if (!((mode == modeStartingBypass) || (mode == modeMalfunction)) && (antenna != 0))
		startCommand(commandEnterBypassMode, &antenna, 1, 2, 10);
	switch (mode) {
	case modeStartingBypass:
	case modeTestTuning:
	case modeStartTuning:
	case modeTuning:
		setMode(modeMalfunction);
		break;
	default:
		setMode(modeNone);
		break;
	}
}

void AtuController::processReceivedTuningFrame(uint8_t id, uint8_t *data, int data_len) {
	if ((id == frameid_A) && (data_len >= 1))
		qmDebugMessage(QmDebug::Info, "received unexpected state message with error_code = %u", data[0]);
	if (mode == modeStartTuning) {
		switch (id) {
		case frameid_A:
			finishCommand();
			setMode(modeMalfunction);
			break;
		case frameid_U:
		case frameid_D:
			finishCommand();
			setMode(modeTuning);
			tx_tuning_state = (id == frameid_U)?true:false;
//			requestTx(tx_tuning_state);
			setRadioPowerOff(!tx_tuning_state);
			acknowledgeTxRequest();
			break;
		default:
			processReceivedUnexpectedFrame(id);
			break;
		}
	} else {
		switch (id) {
		case frameid_A:
			setMode(modeMalfunction);
			break;
		case frameid_f:
		case frameid_F:
			tx_tune_timer->stop();
			setMode(modeActiveTx);
			break;
		case frameid_U:
			if (!tx_tuning_state)
				break;
			tx_tune_timer->stop();
			setRadioPowerOff(false);
			acknowledgeTxRequest();
			break;
		case frameid_D:
			if (tx_tuning_state)
				break;
			tx_tune_timer->stop();
			setRadioPowerOff(true);
			acknowledgeTxRequest();
			break;
		default:
			processReceivedUnexpectedFrame(id);
			break;
		}
	}
}

void AtuController::processReceivedTuneTestingFrame(uint8_t id, uint8_t *data, int data_len) {
	if ((id == frameid_A) && (data_len >= 1))
		qmDebugMessage(QmDebug::Info, "received unexpected state message with error_code = %u", data[0]);
	switch (id) {
	case frameid_A:
		setMode(modeMalfunction);
		break;
	case frameid_U:
	case frameid_D:
		tx_tuning_state = (id == frameid_U)?true:false;
		setRadioPowerOff(!tx_tuning_state);
		sendFrame(id, 0, 0);
		break;
	case frameid_K:
		processReceivedTWFMessage(data, data_len);
		break;
	default:
		processReceivedUnexpectedFrame(id);
		break;
	}
}

void AtuController::processTxTuneTimeout() {
	QM_ASSERT(mode == modeTuning);
	qmDebugMessage(QmDebug::Info, "tx tuning timeout");
	setMode(modeMalfunction);
}

void AtuController::processReceivedStateMessage(uint8_t *data, int data_len) {
	if (!((data_len >= 1) && (command.id == commandRequestState))) {
		sendNak();
		return;
	}
	finishCommand();
	uint8_t error_code = data[0];
	if ((mode == modeNone) && (error_code == 0)) {
		startCommand(commandEnterBypassMode, &antenna, 1, 2);
		setMode(modeStartingBypass);
	} else if ((mode != modeMalfunction) && (error_code != 0)) {
		qmDebugMessage(QmDebug::Info, "received state message with non-zero error_code = %u", error_code);
		setMode(modeMalfunction);
	}
}

void AtuController::processReceivedBypassModeMessage() {
	if (!(command.id == commandEnterBypassMode)) {
		sendNak();
		return;
	}
	finishCommand();
	if (mode != modeStartingBypass)
		return;
	setMode(modeBypass);
}

void AtuController::processReceivedTWFMessage(uint8_t *data, int data_len) {
	if (!((data_len >= 1) && (command.id == commandRequestTWF))) {
		sendNak();
		return;
	}
	finishCommand();
	uint8_t value = data[0];
	qmDebugMessage(QmDebug::Info, "received TWF = %u%%", value);
	if (mode == modeTestTuning) {
		if (force_next_tunetx_full || (value < 30)) {
			setRadioPowerOff(true);
			setMode(modeStartTuning);
			tx_tuning_state = false;
			uint32_t encoded_freq = tunetx_frequency/10;
			uint8_t command_data[4];
			command_data[0] = (encoded_freq >> 16) & 0xFF;
			command_data[1] = (encoded_freq >> 8) & 0xFF;
			command_data[2] = (encoded_freq) & 0xFF;
			command_data[3] = antenna;
			startCommand(commandEnterTuningMode, command_data, sizeof(command_data), 2);
		} else {
			setMode(modeActiveTx);
		}
	}
}

void AtuController::processReceivedUnexpectedFrame(uint8_t id) {
	if (id != frameid_NAK)
		sendNak();
	if (command.id != commandInactive)
		tryRepeatCommand();
}

void AtuController::processReceivedFrame(uint8_t id, uint8_t *data, int data_len) {
	switch (mode) {
	case modeTestTuning:
		processReceivedTuneTestingFrame(id, data, data_len);
		break;
	case modeStartTuning:
	case modeTuning: {
		processReceivedTuningFrame(id, data, data_len);
		break;
	}
	default: {
		switch (id) {
		case frameid_A:
			processReceivedStateMessage(data, data_len);
			break;
		case frameid_Y:
			processReceivedBypassModeMessage();
			break;
		case frameid_K:
			processReceivedTWFMessage(data, data_len);
			break;
		default:
			processReceivedUnexpectedFrame(id);
			break;
		}
		if (command.id == commandInactive)
			processDeferred();
		break;
	}
	}
}

void AtuController::sendFrame(uint8_t id, const uint8_t *data, int data_len) {
	uint8_t eot = FRAME_SYMBOL_EOT;
	qmDebugMessage(QmDebug::Info, "transmitting frame (id=0x%02X, data_len=%d)", id, data_len);
	if (qmDebugIsVerbose() && (data_len > 0)) {
		QM_ASSERT(data != 0);
		for (int i = 0; i < data_len; i++)
			qmDebugMessage(QmDebug::Dump, "frame data: 0x%02X", data[i]);
	}
	int64_t written = 0;
	written += uart->writeData(&id, 1);
	written += uart->writeData(data, data_len);
	written += uart->writeData(&eot, 1);
	QM_ASSERT(written == (2 + data_len));
}

void AtuController::sendNak() {
	const uint8_t frame[] = {frameid_NAK, FRAME_SYMBOL_EOT};
	if (uart->getTxSpaceAvailable() < 128) { // АСУ флудит ?
		qmDebugMessage(QmDebug::Warning, "uart tx queue overflowed, no NAK will be sent");
		return; // отражение DoS-атаки :)
	}
	qmDebugMessage(QmDebug::Info, "transmitting NAK frame");
	uart->writeData(frame, sizeof(frame));
}

void AtuController::processUartReceivedData() {
	uint8_t byte;
	qmDebugMessage(QmDebug::Dump, "uart rx data...");
	while (uart->readData(&byte, 1)) {
		switch (uart_rx_state) {
		case uartrxNone:
			if (byte == FRAME_SYMBOL_EOT) {
				qmDebugMessage(QmDebug::Dump, "uart rx: - ignoring EOT");
				break;
			}
			qmDebugMessage(QmDebug::Dump, "uart rx: - frame id 0x%02X", byte);
			uart_rx_frame.id = byte;
			uart_rx_frame.data_len = 0;
			uart_rx_frame.truncated = false;
			uart_rx_state = uartrxFrame;
			break;
		case uartrxFrame:
			if (byte != FRAME_SYMBOL_EOT) {
				qmDebugMessage(QmDebug::Dump, "uart rx: - frame data 0x%02X", byte);
				if (uart_rx_frame.data_len < MAX_FRAME_DATA_SIZE) {
					uart_rx_frame.data_buf[uart_rx_frame.data_len++] = byte;
				} else {
					if (!uart_rx_frame.truncated) {
						qmDebugMessage(QmDebug::Warning, "uart rx: - too long frame");
						uart_rx_frame.truncated = true;
					}
				}
			} else {
				qmDebugMessage(QmDebug::Dump, "uart rx: - frame EOT");
				qmDebugMessage(QmDebug::Info, "received frame (id=0x%02X, data_len=%u%s)", uart_rx_frame.id, uart_rx_frame.data_len, (uart_rx_frame.truncated?"TRUNCATED":""));
				uart_rx_state = uartrxNone;
				processReceivedFrame(uart_rx_frame.id, uart_rx_frame.data_buf, uart_rx_frame.data_len);
			}
			break;
		}
	}
}

void AtuController::setAntenna(uint32_t frequency) {
	if ((4000000 <= frequency) && (frequency < 10500000)) {
		antenna = 1;
	} else if ((10500000 <= frequency) && (frequency < 30000000)) {
		antenna = 2;
	} else {
		antenna = 0;
	}
}

void AtuController::processDeferred() {
	if (deferred_enterbypass_active) {
		deferred_enterbypass_active = false;
		executeEnterBypassMode();
	}
	if (deferred_tunetx_active) {
		deferred_tunetx_active = false;
		executeTuneTxMode();
	}
}

void AtuController::executeEnterBypassMode() {
	startCommand(commandEnterBypassMode, &antenna, 1, 2);
	setMode(modeStartingBypass);
}

void AtuController::executeTuneTxMode() {
	setMode(modeTestTuning);
//	setRadioPowerOff(false);
//	QmThread::msleep(20);
	startCommand(commandRequestTWF, 0, 0, 2, 50);
}

} /* namespace Multiradio */

#include "qmdebug_domains_start.h"
QMDEBUG_DEFINE_DOMAIN(atucontroller, LevelDefault)
#include "qmdebug_domains_end.h"
