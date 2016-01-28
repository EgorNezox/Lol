/**
 ******************************************************************************
 * @file    Controller.cpp
 * @author  Artem Pisarenko, PMR dept. software team, ONIIP, PJSC
 * @date    29.10.2015
 *
 ******************************************************************************
 */

#define QMDEBUGDOMAIN	headset

#include "qm.h"
#include "qmdebug.h"
#include "qmpushbuttonkey.h"

#include "controller.h"
#include "smarttransport.h"

namespace Headset {

Controller::Controller(int rs232_uart_resource, int ptt_iopin_resource) :
	ptt_state(false)
{
	QM_UNUSED(rs232_uart_resource);
	ptt_key = new QmPushButtonKey(ptt_iopin_resource);
	ptt_state = ptt_key->isPressed();
	ptt_key->stateChanged.connect(sigc::mem_fun(this, &Controller::pttStateChangedSlot));
	transport = new SmartTransport(rs232_uart_resource, 1, this);
}

Controller::~Controller() {
	//...
}

void Controller::startServicing(
		const Multiradio::voice_channels_table_t& local_channels_table) {
	QM_UNUSED(local_channels_table);
	qmDebugMessage(QmDebug::Info, "start servicing...");
	transport->enable();
}

Controller::Status Controller::getStatus() {
	return StatusAnalog;
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

bool Controller::getSmartCurrentChannel(int& number) {
	QM_UNUSED(number);
	//...
	return false;
}

void Controller::pttStateChangedSlot()
{
	ptt_state = !ptt_state;
	pttStateChanged(ptt_state);
}

} /* namespace Headset */

#include "qmdebug_domains_start.h"
QMDEBUG_DEFINE_DOMAIN(headset, LevelDefault)
#include "qmdebug_domains_end.h"
