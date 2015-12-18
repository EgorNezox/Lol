/**
 ******************************************************************************
 * @file    dispatcher.cpp
 * @author  Artem Pisarenko, PMR dept. software team, ONIIP, PJSC
 * @date    30.10.2015
 *
 ******************************************************************************
 */

#include "qm.h"

#include "dispatcher.h"
#include "mainserviceinterface.h"
#include "voiceserviceinterface.h"

namespace Multiradio {

Dispatcher::Dispatcher(int dsp_uart_resource, int dspreset_iopin_resource, int atu_uart_resource,
		Headset::Controller *headset_controller) {
	QM_UNUSED(dsp_uart_resource);
	QM_UNUSED(dspreset_iopin_resource);
	QM_UNUSED(atu_uart_resource);
	QM_UNUSED(headset_controller);
	//...
}

Dispatcher::~Dispatcher() {
	//...
}

void Dispatcher::startServicing(
		const Multiradio::voice_channels_table_t& voice_channels_table) {
	QM_UNUSED(voice_channels_table);
	//...
}

MainServiceInterface* Dispatcher::getMainServiceInterface() {
	MainServiceInterface *p=new MainServiceInterface();
	return p;
}

VoiceServiceInterface* Dispatcher::getVoiceServiceInterface() {
	VoiceServiceInterface *p=new VoiceServiceInterface();
	return p;
}

} /* namespace Multiradio */
