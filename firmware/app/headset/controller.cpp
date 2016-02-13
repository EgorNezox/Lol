/**
 ******************************************************************************
 * @file    Controller.cpp
 * @author  Artem Pisarenko, PMR dept. software team, ONIIP, PJSC
 * @author  Petr Dmitriev
 * @date    29.10.2015
 *
 ******************************************************************************
 */

#define QMDEBUGDOMAIN	headset

#include "qm.h"
#include "qmdebug.h"
#include "qmpushbuttonkey.h"
#include "qmtimer.h"

#include "controller.h"
#include "smarttransport.h"

namespace Headset {

#define HS_UART_POLLING_INTERVAL	1000
#define HS_RESPONCE_TIMEOUT			30

Controller::Controller(int rs232_uart_resource, int ptt_iopin_resource) :
	status(StatusNone), ptt_state(false)
{
	QM_UNUSED(rs232_uart_resource);
	ptt_key = new QmPushButtonKey(ptt_iopin_resource);
	ptt_state = ptt_key->isPressed();
	ptt_key->stateChanged.connect(sigc::mem_fun(this, &Controller::pttStateChangedSlot));
	transport = new SmartTransport(rs232_uart_resource, 1, this);
	transport->receivedCmd.connect(sigc::mem_fun(this, &Controller::smartReceivedCmd));
	poll_timer = new QmTimer(false, this);
	poll_timer->timeout.connect(sigc::mem_fun(this, &Controller::processHSUartPolling));
	poll_timer->setInterval(HS_UART_POLLING_INTERVAL);
	responce_timer = new QmTimer(true, this);
	responce_timer->timeout.connect(sigc::mem_fun(this, &Controller::processResponceTimeout));
	responce_timer->setInterval(HS_RESPONCE_TIMEOUT);
}

Controller::~Controller() {
	//...
}

void Controller::startServicing(
		const Multiradio::voice_channels_table_t& local_channels_table) {
	QM_UNUSED(local_channels_table);
	qmDebugMessage(QmDebug::Info, "start servicing...");
	transport->enable();
	poll_timer->start();
}

Controller::Status Controller::getStatus() {
	return status;
}

bool Controller::getSmartStatus(SmartStatusDescription& description) {
	QM_UNUSED(description);
	//...
	return false;
}

bool Controller::getAnalogStatus(bool& open_channels_missing) {
	open_channels_missing = false;
	return true;
}

bool Controller::getPTTState(bool& state) {
	state = ptt_state;
	return true;
}

bool Controller::getSmartCurrentChannel(int& number, Multiradio::voice_channel_t &type) {
	QM_UNUSED(number);
	QM_UNUSED(type);
	//...
	return false;
}

void Controller::pttStateChangedSlot()
{
	ptt_state = !ptt_state;
	pttStateChanged(ptt_state);
}

void Controller::smartReceivedCmd(uint8_t cmd, uint8_t* data, int data_len) {
	if (cmd == 0xB0 && data_len == 0) {
		responce_timer->stop();
		if (status != StatusAnalog) {
			qmDebugMessage(QmDebug::Dump, "StatusAnalog");
			status = StatusAnalog;
		}
	}
}

void Controller::processHSUartPolling() {
	transport->transmitCmd(0xB0, NULL, 0);
	responce_timer->start();
}

void Controller::processResponceTimeout() {
	if (status != StatusNone) {
		qmDebugMessage(QmDebug::Dump, "StatusNone");
		status = StatusNone;
	}
}

} /* namespace Headset */

#include "qmdebug_domains_start.h"
QMDEBUG_DEFINE_DOMAIN(headset, LevelVerbose)
#include "qmdebug_domains_end.h"
