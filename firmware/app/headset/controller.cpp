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
#include "qmpushbuttonkey.h"
#include "qmtimer.h"

#include "controller.h"
#include "smarttransport.h"

namespace Headset {

#define PTT_DEBOUNCE_TIMEOUT		50
#define HS_UART_POLLING_INTERVAL	1000
#define HS_RESPONCE_TIMEOUT			30

Controller::Controller(int rs232_uart_resource, int ptt_iopin_resource) :
	status(StatusNone), ptt_state(false)
{
	ptt_key = new QmPushButtonKey(ptt_iopin_resource);
	ptt_state = ptt_key->isPressed();
	ptt_key->stateChanged.connect(sigc::mem_fun(this, &Controller::processPttStateChanged));
	ptt_debounce_timer = new QmTimer(true, this);
	ptt_debounce_timer->timeout.connect(sigc::mem_fun(this, &Controller::processPttDobounceTimeout));
	ptt_debounce_timer->setInterval(PTT_DEBOUNCE_TIMEOUT);
	transport = new SmartTransport(rs232_uart_resource, 1, this);
	transport->receivedCmd.connect(sigc::mem_fun(this, &Controller::processReceivedCmd));
	poll_timer = new QmTimer(false, this);
	poll_timer->timeout.connect(sigc::mem_fun(this, &Controller::processHSUartPolling));
	poll_timer->setInterval(HS_UART_POLLING_INTERVAL);
	responce_timer = new QmTimer(true, this);
	responce_timer->timeout.connect(sigc::mem_fun(this, &Controller::processCmdResponceTimeout));
	responce_timer->setInterval(HS_RESPONCE_TIMEOUT);
}

Controller::~Controller() {
	//...
}

void Controller::startServicing(const Multiradio::voice_channels_table_t& local_channels_table) {
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
	return status == StatusAnalog;
}

bool Controller::getPTTState(bool& state) {
	state = ptt_state;
	return status == StatusAnalog || status == StatusSmartOk;
}

bool Controller::getSmartCurrentChannel(int& number, Multiradio::voice_channel_t &type) {
	QM_UNUSED(number);
	QM_UNUSED(type);
	//...
	return false;
}

void Controller::processPttStateChanged() {
	ptt_debounce_timer->start();
}

void Controller::processPttDobounceTimeout() {
	if (ptt_state != ptt_key->isPressed()) {
		ptt_state = !ptt_state;
		qmDebugMessage(QmDebug::Dump, "ptt state changed: %d", ptt_state);
		pttStateChanged(ptt_state);
	}
}

void Controller::processReceivedCmd(uint8_t cmd, uint8_t* data, int data_len) {
	switch (cmd) {
	case 0xB0: {
		if (data_len == 0) {
			responce_timer->stop();
			setStatus(StatusAnalog);
		}
		break;
	}
	}
}

void Controller::processHSUartPolling() {
	transport->transmitCmd(0xB0, NULL, 0);
	responce_timer->start();
}

void Controller::processCmdResponceTimeout() {
	setStatus(StatusNone);
}

void Controller::setStatus(Status new_status) {
	if (status != new_status) {
		qmDebugMessage(QmDebug::Dump, "status changed: %d", new_status);
		status = new_status;
		statusChanged(new_status);
	}
}

} /* namespace Headset */

#include "qmdebug_domains_start.h"
QMDEBUG_DEFINE_DOMAIN(hscontroller, LevelVerbose)
#include "qmdebug_domains_end.h"
