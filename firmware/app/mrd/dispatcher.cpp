/**
 ******************************************************************************
 * @file    dispatcher.cpp
 * @author  Artem Pisarenko, PMR dept. software team, ONIIP, PJSC
 * @date    30.10.2015
 *
 * TODO: синхронизировать обновления статусов с завершением операций DspController/AtuController
 ******************************************************************************
 */

#include <algorithm>

#include "qm.h"
#define QMDEBUGDOMAIN	mrd
#include "qmdebug.h"

#include "dispatcher.h"
#include "mainserviceinterface.h"
#include "voiceserviceinterface.h"

namespace Multiradio {

Dispatcher::Dispatcher(int dsp_uart_resource, int dspreset_iopin_resource, int atu_uart_resource,
		Headset::Controller *headset_controller) :
	QmObject(0),
	headset_controller(headset_controller)
{
	headset_controller->statusChanged.connect(sigc::mem_fun(this, &Dispatcher::setupVoiceMode));
	headset_controller->pttStateChanged.connect(sigc::mem_fun(this, &Dispatcher::processHeadsetPttStateChange));
	dsp_controller = new DspController(dsp_uart_resource, dspreset_iopin_resource, this);
	dsp_controller->started.connect(sigc::mem_fun(this, &Dispatcher::processDspStartup));
	dsp_controller->setRadioCompleted.connect(sigc::mem_fun(this, &Dispatcher::processDspSetRadioCompletion));
	atu_controller = new AtuController(atu_uart_resource, this);
	atu_controller->modeChanged.connect(sigc::mem_fun(this, &Dispatcher::processAtuModeChange));
	atu_controller->requestTx.connect(sigc::mem_fun(this, &Dispatcher::processAtuRequestTx));
	main_service = new MainServiceInterface(this);
	voice_service = new VoiceServiceInterface(this);
}

Dispatcher::~Dispatcher()
{
}

void Dispatcher::startServicing(const Multiradio::voice_channels_table_t& voice_channels_table) {
	this->voice_channels_table = voice_channels_table;
	dsp_controller->startServicing();
	atu_controller->startServicing();
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
	if (ptt_state) {
		if (!atu_controller->isDeviceOperational()) {
			dsp_controller->setRadioOperation(DspController::RadioOperationTxMode);
			main_service->setStatus(MainServiceInterface::StatusVoiceTx);
		} else {
			prepareTuningTx();
		}
	} else {
		dsp_controller->setRadioOperation(DspController::RadioOperationRxMode);
		if (atu_controller->isDeviceOperational())
			atu_controller->enterBypassMode();
		main_service->setStatus(MainServiceInterface::StatusVoiceRx);
	}
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
	if ((main_service->current_status == MainServiceInterface::StatusVoiceTx) && atu_controller->isDeviceOperational())
		prepareTuningTx();
	dsp_controller->setRadioParameters(mode, frequency);
}

void Dispatcher::updateVoiceChannel() {
	setVoiceChannel();
	voice_service->setCurrentChannel(VoiceServiceInterface::ChannelActive);
}

bool Dispatcher::isVoiceMode() {
	return ((main_service->current_status == MainServiceInterface::StatusVoiceRx) || (main_service->current_status == MainServiceInterface::StatusVoiceTx));
}

void Dispatcher::processDspSetRadioCompletion() {
	if (main_service->current_status == MainServiceInterface::StatusTuningTx) {
		if (atu_controller->getMode() != AtuController::modeTuning)
			atu_controller->tuneTxMode((*voice_channel).frequency);
		else
			atu_controller->acknowledgeTxRequest();
	}
}

void Dispatcher::prepareTuningTx() {
	dsp_controller->setRadioOperation(DspController::RadioOperationOff);
	main_service->setStatus(MainServiceInterface::StatusTuningTx);
}

void Dispatcher::processAtuModeChange(AtuController::Mode new_mode) {
	switch (new_mode) {
	case AtuController::modeBypass: {
		switch (main_service->current_status) {
		case MainServiceInterface::StatusVoiceTx:
			prepareTuningTx();
			break;
		case MainServiceInterface::StatusTuningTx:
			atu_controller->tuneTxMode((*voice_channel).frequency);
			break;
		default: break;
		}
		break;
	}
	case AtuController::modeActiveTx: {
		switch (main_service->current_status) {
		case MainServiceInterface::StatusTuningTx:
			dsp_controller->setRadioOperation(DspController::RadioOperationTxMode);
			main_service->setStatus(MainServiceInterface::StatusVoiceTx);
			break;
		default:
			atu_controller->enterBypassMode();
			break;
		}
		break;
	}
	default: break;
	}
}

void Dispatcher::processAtuRequestTx(bool enable) {
	if (main_service->current_status != MainServiceInterface::StatusTuningTx)
		return;
	dsp_controller->setRadioOperation((enable)?(DspController::RadioOperationCarrierTx):(DspController::RadioOperationOff));
}

} /* namespace Multiradio */

#include "qmdebug_domains_start.h"
QMDEBUG_DEFINE_DOMAIN(mrd, LevelDefault)
#include "qmdebug_domains_end.h"
