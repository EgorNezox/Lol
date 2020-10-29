/**
 ******************************************************************************
 * @file    atucontroller.cpp
 * @author  Pankov Denis
 * @date    17.02.20
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

#define PIN_READY_RESOURCE_ENUM 27

namespace Multiradio {

AtuController::AtuController(int uart_resource, int iopin_resource, QmObject *parent) :
	QmObject(parent),
	mode(modeNone), force_next_tunetx_full(false), antenna(1), last_tune_setup_valid(false),
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
	uart_config.rx_buffer_size = 1024;
	uart_config.tx_buffer_size = 1024;
	uart_config.io_pending_interval = 1;

	uart = new QmUart(uart_resource, &uart_config, this);
	uart->dataReceived.connect(sigc::mem_fun(this, &AtuController::processUartReceivedData));
	scan_timer = new QmTimer(false, this);

	scan_timer->setInterval(5000);
	scan_timer->timeout.connect(sigc::mem_fun(this, &AtuController::scan));
	command_timeout_timer = new QmTimer(true, this);

	command_timeout_timer->timeout.connect(sigc::mem_fun(this, &AtuController::tryRepeatCommand));
	tx_tune_timer = new QmTimer(true, this);

	tx_tune_timer->setInterval(20);
	tx_tune_timer->timeout.connect(sigc::mem_fun(this, &AtuController::processTxTuneTimeout));
	poff_iopin = new QmIopin(iopin_resource, this);
	pin_ready = new QmIopin(PIN_READY_RESOURCE_ENUM,this);
}

AtuController::~AtuController()
{
	uart->close();
	delete[] uart_rx_frame.data_buf;
	delete[] command.data_buf;
}

bool AtuController::isReady()
{
	return (bool)pin_ready->readInput();
}

bool AtuController::isDeviceConnected()
{
	return (mode != modeNone);
}

AtuController::Mode AtuController::getMode()
{
	return mode;
}

void AtuController::startServicing()
{
	uart->open();
	scan();
	scan_timer->start();
}

// <F> - 0x46 : set freq and antenna, ap17 - 0x00
void AtuController::setAnsuFreq()
{
	last_tune_setup_valid = false;
	uint32_t encoded_freq = tunetx_frequency/10;
	uint8_t command_data[4];
	command_data[0] = (encoded_freq >> 16) & 0xFF;
	command_data[1] = (encoded_freq >> 8) & 0xFF;
	command_data[2] = (encoded_freq) & 0xFF;
	command_data[3] = antenna;
	startCommand(commandEnterFullTuningMode, command_data, sizeof(command_data), 0);
	tx_tune_timer->start();
}

// <X> - 0x58 : command exit mode bypass
void AtuController::executeExitBypassMode()
{
	startCommand(commandExitBypassMode, 0, 0, 20);
}
// <Y> - 0x59 : command enter mode bypass
void AtuController::executeEnterBypassMode()
{
	startCommand(commandEnterBypassMode, 0, 0, 20);
}

// <A> - 0x41 : command for scan command
void AtuController::scan()
{
	const uint8_t frame[] = {frameid_A, FRAME_SYMBOL_EOT};
	uart->writeData(frame, sizeof(frame));
	//startCommand(commandRequestState,0,0,10);
}

// <K> - 0x4B: request kbw value
void AtuController::requestKBW()
{
	const uint8_t frame[] = {frameid_K, FRAME_SYMBOL_EOT};
	uart->writeData(frame, sizeof(frame));
}

// <ACK> - 0x6: answer for rec ansu frame
void AtuController::ack()
{
	const uint8_t frame[] = {frameid_ack, FRAME_SYMBOL_EOT};
	uart->writeData(frame, sizeof(frame));
}

void AtuController::getVersion()
{
	const uint8_t frame[] = {frameid_V, FRAME_SYMBOL_EOT};
	uart->writeData(frame, sizeof(frame));
}

void AtuController::sendNak()
{
	const uint8_t frame[] = {frameid_NAK, FRAME_SYMBOL_EOT};
	uart->writeData(frame, sizeof(frame));
}

void AtuController::processReceivedFrame(uint8_t id, uint8_t *data)
{
   //processReceivedTuningFrame(id, data);
	if (id != 65)
	qmDebugMessage(QmDebug::Info, " received ansu cadr cadr id 0x%02X, 0x%02X 0x%02X 0x%02X 0x%02X ", id,data[0],data[1],data[2],data[3]);
	switch (id)
	{
	case frameid_A:
		processReceivedStateMessage(data);
//		finishCommand();
		break;
	case frameid_X:
		break;
	case frameid_Y:
		finishCommand();
		ack();
		break;
	case frameid_NAK:
		// user info
		setMode(modeBypass);
		break;
	case frameid_K:
		kbw_value = data[0];
		ack();
		break;
	case frameid_F:
		setForFastTune(data);
		setMode(modeActiveTx);
		ack();
		break;
	case frameid_V:
		//set version
		ansu_version = (data[0] << 24) + (data[1] << 16) + (data[2] << 8) + data[0];
		ack();
		break;
	default:
		processReceivedUnexpectedFrame(id);
		break;
	}
}

bool AtuController::enterBypassMode(uint32_t frequency)
{
//	setAntenna(frequency);
//	if (!((mode == modeActiveTx) || (mode == modeBypass)) || (antenna == 0))
//		return false;
//	if (command.id != commandInactive) {
//		deferred_enterbypass_active = true;
//		return true;
//	}
	executeEnterBypassMode();
	return true;
}

void AtuController::setFreq(uint32_t frequency)
{
	tunetx_frequency = frequency;
}

bool AtuController::tuneTxMode(uint32_t frequency)
{
	setAntenna(frequency);
	if (antenna == 0)
		return false;
	if (!((mode == modeBypass) || (mode == modeActiveTx)
			|| ((mode == modeFault) && force_next_tunetx_full)))
		return false;
	tunetx_frequency = frequency;
	if (command.id != commandInactive) {
		deferred_tunetx_active = true;
		return true;
	}
	executeTuneTxMode();
	return true;
}

void AtuController::setNextTuningParams(bool force_full)
{
	force_next_tunetx_full = force_full;
	if (force_full)
		last_tune_setup_valid = false;
}

void AtuController::setMinimalActivityMode(bool enabled)
{
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

void AtuController::setMode(Mode mode)
{
	switch (mode) {
	case modeNone:
		scan_timer->start();
		break;
	case modeFault:
		tx_tune_timer->stop();
		scan_timer->start();
		executeEnterBypassMode();
		break;
	case modeBypass:
		executeEnterBypassMode();
		break;
	case modeTuning:
	//	qmDebugMessage(QmDebug::Info, "tuning tx...");
		break;
	case modeActiveTx:
		//qmDebugMessage(QmDebug::Info, "active tx mode");
		scan_timer->start();
		break;
	}

	if (mode != this->mode)
	{
	 this->mode = mode;
     modeChanged(mode);
	}
}


void AtuController::startCommand(CommandId id, const uint8_t* data, int data_len, int repeat_count, int timeout)
{
	//QM_ASSERT(data_len <= MAX_FRAME_DATA_SIZE);
	command.id = id;
	memcpy(command.data_buf, data, data_len);
	command.data_len = data_len;
	command.repeat_count = repeat_count;
	sendFrame(id, data, data_len);
	if (timeout > 0)
		command_timeout_timer->start(timeout);
}

void AtuController::processReceivedStateMessage(uint8_t *data)
{
	uint8_t error_code = data[0];

	if ((mode == modeNone) && (error_code == 0))
	{
		getVersion();
		setMode(modeBypass);
	}
	else if ((mode != modeFault) && (error_code != 0))
	{
		stateError(error_code);

		error = error_code;
		// TODO:User inform
		setMode(modeFault);
	}
}

void AtuController::stateError(uint8_t error_code)
{
	if (error_code == 1) qmDebugMessage(QmDebug::Info, "1: received incorrect frequency code, cadr <F> ");
	if (error_code == 2) qmDebugMessage(QmDebug::Info, "2: received incorrect antennta  code, cadr <F> ");
	if (error_code == 3) qmDebugMessage(QmDebug::Info, "3: loop in setting mode ");
	if (error_code == 4) qmDebugMessage(QmDebug::Info, "4: not power error ");
	if (error_code == 5) qmDebugMessage(QmDebug::Info, "5: not cool level of power");
	if (error_code == 6) qmDebugMessage(QmDebug::Info, "6: try set in fault ");
	if (error_code == 7) qmDebugMessage(QmDebug::Info, "7: power for set READY not OFF");
	if (error_code == 8) qmDebugMessage(QmDebug::Info, "8: temperature error ");
	if (error_code == 9) qmDebugMessage(QmDebug::Info, "9: critical electrical");
}

void AtuController::setForFastTune(uint8_t *data)
{
	memcpy(last_tune_setup, data, sizeof(last_tune_setup));
	last_tune_setup_valid = true;
}

void AtuController::finishCommand()
{
	command_timeout_timer->stop();
	command.id = commandInactive;
}

void AtuController::tryRepeatCommand()
{
	/* check repeat count */
	if (command.repeat_count > 0)
	{
		sendFrame(command.id, command.data_buf, command.data_len);
		command_timeout_timer->start();
		command.repeat_count--;
		return;
	}

//	command.id = commandInactive;
//	deferred_enterbypass_active = false;
//	deferred_tunetx_active      = false;

	/* restart timer if not detect device */
	if (mode == modeNone)
	{
		if (!scan_timer->isActive())
			scan_timer->start();
		return;
	}

//
//	if (!((mode == modeBypass) || (mode == modeFault)) && (antenna != 0))
//		startCommand(commandEnterBypassMode, &antenna, 1, 2, 10);

//	switch (mode)
//	{
//	//case modeBypass:
//	case modeTuning:
//		setMode(modeFault);
//		break;
//	default:
//		setMode(modeNone);
//		break;
//	}
}

void AtuController::processTxTuneTimeout()
{
//	QM_ASSERT(mode == modeTuning);
//	setMode(modeFault);
}


void AtuController::processReceivedBypassModeMessage()
{
	if (!(command.id == commandEnterBypassMode)) {
		sendNak();
		return;
	}
	finishCommand();
	if (mode != modeBypass)
		return;
	setMode(modeBypass);
}

void AtuController::processReceivedUnexpectedFrame(uint8_t id)
{
	if (id != frameid_NAK)
		sendNak();
}

void AtuController::sendFrame(uint8_t id, const uint8_t *data, int data_len)
{
//	if (!(data_len <= MAX_FRAME_DATA_SIZE))
//		QM_ASSERT(0);
//		return;

#if 1
	qmDebugMessage(QmDebug::Info, "frame data id: 0x%02X", id);
	for (int i = 0; i < data_len; i++)
		qmDebugMessage(QmDebug::Info, "frame data: 0x%02X", data[i]);
#endif

	uint8_t frame_buf[2 + MAX_FRAME_DATA_SIZE];
	frame_buf[0] = id;

	memcpy(frame_buf + 1, data, data_len);
	frame_buf[1 + data_len] = FRAME_SYMBOL_EOT;

	int64_t written = uart->writeData(frame_buf, (2 + data_len));
	QM_ASSERT(written == (2 + data_len));
}

bool AtuController::checkFeq(uint32_t frequency)
{
	/* set special algoritm freq */
	if (2000000 < frequency && frequency <= 4000000)
	{
		antenna = 1;
		return true;
	}
	/* check valid freq and set bypass if not */
	if (frequency <= 2000000)
	{
		enterBypassMode(frequency);
		return false;
	}

	return true;
}


void AtuController::setAntenna(uint8_t antenna)
{
	this->antenna = antenna;
}

void AtuController::processDeferred()
{
	if (deferred_enterbypass_active) {
		deferred_enterbypass_active = false;
		executeEnterBypassMode();
	}
	if (deferred_tunetx_active) {
		deferred_tunetx_active = false;
		executeTuneTxMode();
	}
}

void AtuController::executeTuneTxMode()
{
	setMode(modeTuning);
	if (last_tune_setup_valid && !force_next_tunetx_full)
	{
		tx_quick_tuning_attempt = true;
		startCommand(commandEnterQuickTuningMode, last_tune_setup, sizeof(last_tune_setup),0, 20);
	}
	else
	{
		tx_quick_tuning_attempt = false;
		setAnsuFreq();
	}

	force_next_tunetx_full = false;
}

void AtuController::processUartReceivedData()
{
	uint8_t byte;
	//qmDebugMessage(QmDebug::Dump, "uart rx data...");
	while (uart->readData(&byte, 1)) {
		switch (uart_rx_state) {
		case uartrxNone: {
			if (byte == FRAME_SYMBOL_EOT) {
			//	qmDebugMessage(QmDebug::Dump, "uart rx: - ignoring EOT");
				break;
			}
			uart_rx_frame.id = byte;
			switch (uart_rx_frame.id) {
			case frameid_NAK:
			//	qmDebugMessage(QmDebug::Dump, "uart rx: - frame id NAK");
				uart_rx_frame.data_len = 0;
				break;
			case frameid_A:
			//	qmDebugMessage(QmDebug::Dump, "uart rx: - frame id A");
				uart_rx_frame.data_len = 1;
				break;
			case frameid_D:
			//	qmDebugMessage(QmDebug::Dump, "uart rx: - frame id D");
				uart_rx_frame.data_len = 0;
				break;
			case frameid_F:
			//	qmDebugMessage(QmDebug::Dump, "uart rx: - frame id F");
				uart_rx_frame.data_len = 6;
				break;
			case frameid_K:
			//	qmDebugMessage(QmDebug::Dump, "uart rx: - frame id K");
				uart_rx_frame.data_len = 1;
				break;
			case frameid_U:
			//	qmDebugMessage(QmDebug::Dump, "uart rx: - frame id U");
				uart_rx_frame.data_len = 0;
				break;
			case frameid_Y:
			//	qmDebugMessage(QmDebug::Dump, "uart rx: - frame id Y");
				uart_rx_frame.data_len = 0;
				break;
			case frameid_V:
			//	qmDebugMessage(QmDebug::Dump, "uart rx: - frame id V");
				uart_rx_frame.data_len = 2;
				break;
			default:
				qmDebugMessage(QmDebug::Warning, "uart rx: - UNKNOWN FRAME ID 0x%02X, frame sync may be broken !", uart_rx_frame.id);
				uart_rx_frame.data_len = 0;
				sendNak();
				break;
			}
			uart_rx_frame.data_pos = 0;
			uart_rx_state = uartrxFrame;
			break;
		}
		case uartrxFrame: {
			if (uart_rx_frame.data_pos < uart_rx_frame.data_len) {
			//	qmDebugMessage(QmDebug::Dump, "uart rx: - frame data 0x%02X", byte);
				uart_rx_frame.data_buf[uart_rx_frame.data_pos++] = byte;
			} else if (byte == FRAME_SYMBOL_EOT) {
			//	qmDebugMessage(QmDebug::Dump, "uart rx: - frame EOT");
			//	qmDebugMessage(QmDebug::Dump, "received frame (id=0x%02X, data_len=%u)", uart_rx_frame.id, uart_rx_frame.data_len);
				uart_rx_state = uartrxNone;
				processReceivedFrame(uart_rx_frame.id, uart_rx_frame.data_buf);
			} else {
			//	qmDebugMessage(QmDebug::Warning, "uart rx: - unexpected symbol in frame EOT position !");
				uart_rx_state = uartrxNone;
			}
			break;
		}
		}
	}
}


} /* namespace Multiradio */

#include "qmdebug_domains_start.h"
QMDEBUG_DEFINE_DOMAIN(atucontroller, LevelVerbose)
#include "qmdebug_domains_end.h"
