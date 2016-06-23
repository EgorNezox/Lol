/**
 ******************************************************************************
 * @file    dispatcher.cpp
 * @author  Artem Pisarenko, PMR dept. software team, ONIIP, PJSC
 * @author  неизвестные
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

Dispatcher::Dispatcher( int dsp_uart_resource,
                        int dspreset_iopin_resource,
                        int atu_uart_resource,
						int atu_iopin_resource,
                        Headset::Controller *headset_controller,
                        Navigation::Navigator *navigator,
						DataStorage::FS *data_storage_fs
                        ) :
                        QmObject(0),
                        headset_controller(headset_controller),
                        voice_channel(voice_channels_table.end()),
						data_storage_fs(data_storage_fs)
{
	headset_controller->statusChanged.connect(sigc::mem_fun(this, &Dispatcher::setupVoiceMode));
	headset_controller->pttStateChanged.connect(sigc::mem_fun(this, &Dispatcher::processHeadsetPttStateChange));
	headset_controller->smartCurrentChannelChanged.connect(sigc::mem_fun(this, &Dispatcher::processHeadsetSmartCurrentChannelChange));
	dsp_controller = new DspController(dsp_uart_resource, dspreset_iopin_resource, navigator, this);
	dsp_controller->started.connect(sigc::mem_fun(this, &Dispatcher::processDspStartup));
	dsp_controller->setRadioCompleted.connect(sigc::mem_fun(this, &Dispatcher::processDspSetRadioCompletion));
	atu_controller = new AtuController(atu_uart_resource, atu_iopin_resource, this);
	atu_controller->modeChanged.connect(sigc::mem_fun(this, &Dispatcher::processAtuModeChange));
	atu_controller->requestTx.connect(sigc::mem_fun(this, &Dispatcher::processAtuRequestTx));
	main_service = new MainServiceInterface(this, navigator);
	voice_service = new VoiceServiceInterface(this);

}

Dispatcher::~Dispatcher()
{
    delete dsp_controller;
    delete atu_controller;
    delete main_service;
    delete voice_service;
}

void Dispatcher::startServicing(const Multiradio::voice_channels_table_t& voice_channels_table) {
	this->voice_channels_table = voice_channels_table;
	voice_channel = this->voice_channels_table.end();
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
//	if (!isVoiceMode())
//		return false;
	setVoiceDirection(new_state);
	return true;
}

void Dispatcher::processHeadsetSmartCurrentChannelChange(int new_channel_number,
		voice_channel_t new_channel_type) {
	if (!dsp_controller->isReady())
		return;
	if (!changeVoiceChannel(new_channel_number, new_channel_type))
		return;
	updateVoiceChannel();
}

void Dispatcher::setupVoiceMode(Headset::Controller::Status headset_status) {
	if (!dsp_controller->isReady())
		return;
	switch (headset_status) {
	case Headset::Controller::StatusAnalog:
	case Headset::Controller::StatusSmartOk: {
		if (headset_status == Headset::Controller::StatusSmartOk) {
			int smart_ch_number = 1;
			Multiradio::voice_channel_t smart_ch_type = Multiradio::channelInvalid;
			headset_controller->getSmartCurrentChannel(smart_ch_number, smart_ch_type);
			if (!changeVoiceChannel(smart_ch_number, smart_ch_type))
				break;
			dsp_controller->setAudioMicLevel(16);
		} else {
			voice_channel = std::find_if( std::begin(voice_channels_table), std::end(voice_channels_table),
					[&](const voice_channel_entry_t entry){ return (entry.type == channelOpen); } );
			if (voice_channel == voice_channels_table.end()) {
				voice_service->setCurrentChannel(VoiceServiceInterface::ChannelDisabled);
				startIdle();
				break;
			}
			dsp_controller->setAudioMicLevel(50);
		}
		setVoiceChannel();
		bool ptt_state = false;
		headset_controller->getPTTState(ptt_state);
		setVoiceDirection(ptt_state);
		voice_service->setCurrentChannel(VoiceServiceInterface::ChannelActive);
		break;
	}
	default: {
		voice_service->setCurrentChannel(VoiceServiceInterface::ChannelDisabled);
		main_service->setStatus(MainServiceInterface::StatusIdle);
		break;
	}
	}
}

void Dispatcher::setVoiceDirection(bool ptt_state) {
	if (ptt_state) {
		if (!atu_controller->isDeviceOperational()) {
			startVoiceTx();
		} else {
			prepareTuningTx();
		}
	} else {
		dsp_controller->setRadioOperation(DspController::RadioOperationRxMode);
		if (atu_controller->isDeviceOperational())
			atu_controller->enterBypassMode((*voice_channel).frequency);
		main_service->setStatus(MainServiceInterface::StatusVoiceRx);
	}
}

void Dispatcher::setVoiceChannel() {
	uint32_t frequency = (*voice_channel).frequency;
	DspController::RadioMode mode;
	if ((1500000 <= frequency) && (frequency < 30000000)) {
		mode = DspController::RadioModeUSB;
	} else if ((30000000 <= frequency) && (frequency < 300000000)) {
		mode = DspController::RadioModeFM;
	} else {
		mode = DspController::RadioModeOff;
	}
	if ((main_service->current_status == MainServiceInterface::StatusVoiceTx) && atu_controller->isDeviceOperational())
		prepareTuningTx();
	if ((main_service->current_status == MainServiceInterface::StatusVoiceRx) && atu_controller->isDeviceOperational())
		atu_controller->enterBypassMode(frequency);
	dsp_controller->setRadioParameters(mode, frequency);
}

bool Dispatcher::changeVoiceChannel(int number, voice_channel_t type) {
	voice_channel = voice_channels_table.begin() + number - 1;
	if (!(((number-1) < (int)voice_channels_table.size()) && (voice_channels_table[number-1].type == type))) {
		voice_service->setCurrentChannel(VoiceServiceInterface::ChannelInvalid);
		startIdle();
		return false;
	}
	return true;
}

void Dispatcher::updateVoiceChannel() {
	setVoiceChannel();
	if (isVoiceMode())
		voice_service->setCurrentChannel(VoiceServiceInterface::ChannelActive);
}

bool Dispatcher::isVoiceMode() {
	return ((main_service->current_status == MainServiceInterface::StatusVoiceRx) || (main_service->current_status == MainServiceInterface::StatusVoiceTx));
}

bool Dispatcher::isVoiceChannelTunable() {
	return (isVoiceMode() && (headset_controller->getStatus() == Headset::Controller::StatusAnalog));
}

void Dispatcher::processDspSetRadioCompletion() {
	if (main_service->current_status == MainServiceInterface::StatusTuningTx) {
		if (atu_controller->getMode() != AtuController::modeTuning)
			atu_controller->tuneTxMode((*voice_channel).frequency);
//		else
//			atu_controller->acknowledgeTxRequest();
	}
}

void Dispatcher::startIdle() {
	if (main_service->current_status == MainServiceInterface::StatusIdle)
		return;
	dsp_controller->setRadioOperation(DspController::RadioOperationOff);
//	if (atu_controller->isDeviceOperational())
//		atu_controller->enterBypassMode();
	main_service->setStatus(MainServiceInterface::StatusIdle);
}

void Dispatcher::startVoiceTx() {
	atu_controller->setRadioPowerOff(false);
	dsp_controller->setRadioOperation(DspController::RadioOperationTxMode);
	main_service->setStatus(MainServiceInterface::StatusVoiceTx);
}

void Dispatcher::prepareTuningTx() {
//	dsp_controller->setRadioOperation(DspController::RadioOperationOff);
	atu_controller->setRadioPowerOff(true);
	dsp_controller->setRadioOperation(DspController::RadioOperationCarrierTx);
	main_service->setStatus(MainServiceInterface::StatusTuningTx);
	voice_service->setCurrentChannel(VoiceServiceInterface::ChannelDisabled);
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
			startVoiceTx();
			voice_service->setCurrentChannel(VoiceServiceInterface::ChannelActive);
			break;
		default:
			atu_controller->enterBypassMode((*voice_channel).frequency);
			break;
		}
		break;
	}
    case AtuController::modeMalfunction:
    {
        voice_service->errorAsu();
        break;
    }
	default: {
		if ((main_service->current_status == MainServiceInterface::StatusTuningTx) && !atu_controller->isDeviceOperational()) {
			startVoiceTx();
			voice_service->setCurrentChannel(VoiceServiceInterface::ChannelActive);
		}
		break;
	}
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
