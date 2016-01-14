/**
 ******************************************************************************
 * @file    dispatcher.cpp
 * @author  Artem Pisarenko, PMR dept. software team, ONIIP, PJSC
 * @date    30.10.2015
 *
 * TODO: синхронизировать обновления статусов с завершением операций DspController
 ******************************************************************************
 */

#include <algorithm>

#include "qm.h"
#define QMDEBUGDOMAIN	mrd
#include "qmdebug.h"

#include "../dsp/dspcontroller.h"
#include "dispatcher.h"
#include "mainserviceinterface.h"
#include "voiceserviceinterface.h"

namespace Multiradio {

Dispatcher::Dispatcher(int dsp_uart_resource, int dspreset_iopin_resource, int atu_uart_resource,
		Headset::Controller *headset_controller) :
	QmObject(0),
	headset_controller(headset_controller)
{
	QM_UNUSED(atu_uart_resource);
	headset_controller->statusChanged.connect(sigc::mem_fun(this, &Dispatcher::setupVoiceMode));
	headset_controller->pttStateChanged.connect(sigc::mem_fun(this, &Dispatcher::processHeadsetPttStateChange));
	dsp_controller = new DspController(dsp_uart_resource, dspreset_iopin_resource, this);
	dsp_controller->started.connect(sigc::mem_fun(this, &Dispatcher::processDspStartup));
	main_service = new MainServiceInterface(this);
	voice_service = new VoiceServiceInterface(this);
}

Dispatcher::~Dispatcher()
{
}

void Dispatcher::startServicing(const Multiradio::voice_channels_table_t& voice_channels_table) {
	this->voice_channels_table = voice_channels_table;
	dsp_controller->startServicing();
}

MainServiceInterface* Dispatcher::getMainServiceInterface() {
	return main_service;
}

VoiceServiceInterface* Dispatcher::getVoiceServiceInterface() {
	return voice_service;
}

void Dispatcher::processDspStartup() {
	setupVoiceMode(headset_controller->getStatus());
}

bool Dispatcher::processHeadsetPttStateChange(bool new_state) {
	if (!isVoiceMode())
		return false;
	setVoiceDirection(new_state);
	return true;
}

void Dispatcher::setupVoiceMode(Headset::Controller::Status headset_status) {
	main_service->setStatus(MainServiceInterface::StatusIdle);
	switch (headset_status) {
	case Headset::Controller::StatusAnalog: {
		voice_channel = std::find_if( std::begin(voice_channels_table), std::end(voice_channels_table),
				[&](const voice_channel_entry_t entry){ return (entry.type == channelOpen); } );
		if (voice_channel == voice_channels_table.end()) {
			voice_service->setCurrentChannel(VoiceServiceInterface::ChannelDisabled);
			break;
		}
		setVoiceChannel();
		bool ptt_state;
		headset_controller->getPTTState(ptt_state);
		setVoiceDirection(ptt_state);
		voice_service->setCurrentChannel(VoiceServiceInterface::ChannelActive);
		break;
	}
	default: break;
	}
}

void Dispatcher::setVoiceDirection(bool ptt_state) {
	MainServiceInterface::Status new_status;
	if (ptt_state) {
		dsp_controller->setRadioOperation(DspController::RadioOperationTxMode);
		new_status = MainServiceInterface::StatusVoiceTx;
	} else {
		dsp_controller->setRadioOperation(DspController::RadioOperationRxMode);
		new_status = MainServiceInterface::StatusVoiceRx;
	}
	main_service->setStatus(new_status);
}

void Dispatcher::setVoiceChannel() {
	uint32_t frequency = (*voice_channel).frequency;
	DspController::RadioMode mode;
	if ((3000000 <= frequency) && (frequency < 30000000)) {
		mode = DspController::RadioModeUSB;
	} else if ((30000000 <= frequency) && (frequency < 300000000)) {
		mode = DspController::RadioModeFM;
	} else {
		mode = DspController::RadioModeOff;
	}
	dsp_controller->setRadioParameters(mode, frequency);
}

void Dispatcher::updateVoiceChannel() {
	setVoiceChannel();
	voice_service->setCurrentChannel(VoiceServiceInterface::ChannelActive);
}

bool Dispatcher::isVoiceMode() {
	return ((main_service->current_status == MainServiceInterface::StatusVoiceRx) || (main_service->current_status == MainServiceInterface::StatusVoiceTx));
}

} /* namespace Multiradio */

#include "qmdebug_domains_start.h"
QMDEBUG_DEFINE_DOMAIN(mrd, LevelDefault)
#include "qmdebug_domains_end.h"
