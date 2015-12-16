/**
 ******************************************************************************
 * @file    Controller.cpp
 * @author  Artem Pisarenko, PMR dept. software team, ONIIP, PJSC
 * @date    29.10.2015
 *
 ******************************************************************************
 */

#include "qm.h"

#include "controller.h"

namespace Headset {

Controller::Controller(int rs232_uart_resource, int ptt_iopin_resource) {
	QM_UNUSED(rs232_uart_resource);
	QM_UNUSED(ptt_iopin_resource);
	//...
}

Controller::~Controller() {
	//...
}

void Controller::startServicing(
		const Multiradio::voice_channels_table_t& local_channels_table) {
	QM_UNUSED(local_channels_table);
	//...
}

Controller::Status Controller::getStatus() {
	//...
	return StatusNone;
}

bool Controller::getSmartStatus(SmartStatusDescription& description) {
	QM_UNUSED(description);
	//...
	return false;
}

bool Controller::getAnalogStatus(bool& open_channels_missing) {
	QM_UNUSED(open_channels_missing);
	//...
	return false;
}

bool Controller::getPTTState(bool& state) {
	QM_UNUSED(state);
	//...
	return false;
}

bool Controller::getSmartCurrentChannel(int& number) {
	QM_UNUSED(number);
	//...
	return false;
}

} /* namespace Headset */
