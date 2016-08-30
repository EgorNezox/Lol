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
						DataStorage::FS *data_storage_fs,
						Power::Battery *power_battery
                        ) :
                        QmObject(0),
                        headset_controller(headset_controller),
						navigator(navigator),
                        voice_channel(voice_channels_table.end()),
						data_storage_fs(data_storage_fs),
						power_battery(power_battery)
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
	main_service = new MainServiceInterface(this);
	voice_service = new VoiceServiceInterface(this);
	if (!data_storage_fs->getVoiceFrequency(voice_manual_frequency))
		voice_manual_frequency = 0;
	if (!data_storage_fs->getVoiceEmissionType(voice_manual_emission_type))
		voice_manual_emission_type = voiceemissionInvalid;
	if (!data_storage_fs->getVoiceChannelSpeed(voice_manual_channel_speed))
		voice_manual_channel_speed = voicespeedInvalid;
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
	if (isVoiceMode())
		setVoiceDirection(new_state);
	return true;
}

void Dispatcher::processHeadsetSmartCurrentChannelChange(int new_channel_number,
		voice_channel_t new_channel_type) {
	if (!dsp_controller->isReady())
		return;
	setSmartChannelMicLevel(new_channel_type);
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
			voice_channel_t smart_ch_type = Multiradio::channelInvalid;
			headset_controller->getSmartCurrentChannel(smart_ch_number, smart_ch_type);
			setSmartChannelMicLevel(smart_ch_type);
			if (!changeVoiceChannel(smart_ch_number, smart_ch_type))
				break;
		} else {
			dsp_controller->setAudioMicLevel(24);
			voice_channel = std::find_if( std::begin(voice_channels_table), std::end(voice_channels_table),
					[&](const voice_channel_entry_t entry){ return (entry.type == channelOpen); } );
			if (voice_channel == voice_channels_table.end()) {
				voice_service->setCurrentChannel(VoiceServiceInterface::ChannelDisabled);
				if (main_service->current_mode == MainServiceInterface::VoiceModeAuto) {
					startIdle();
					break;
				}
			}
		}
		updateVoiceChannel();
		break;
	}
	default: {
		voice_service->setCurrentChannel(VoiceServiceInterface::ChannelDisabled);
		main_service->setStatus(MainServiceInterface::StatusIdle);
		break;
	}
	}
}

void Dispatcher::setSmartChannelMicLevel(voice_channel_t type) {
	uint8_t audio_mic_level;
	switch (type) {
	case Multiradio::channelOpen:
		audio_mic_level = 24;
		break;
	case Multiradio::channelClose:
		audio_mic_level = 16;
		break;
	default:
		audio_mic_level = 0;
		break;
	}
	dsp_controller->setAudioMicLevel(audio_mic_level);
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
			atu_controller->enterBypassMode(voice_service->getCurrentChannelFrequency());
		main_service->setStatus(MainServiceInterface::StatusVoiceRx);
	}
}

voice_emission_t Dispatcher::getVoiceEmissionFromFrequency(uint32_t frequency) {
	if ((1500000 <= frequency) && (frequency < 30000000)) {
		return voiceemissionUSB;
	} else if ((30000000 <= frequency) && (frequency < 300000000)) {
		return voiceemissionFM;
	}
	return voiceemissionInvalid;
}

void Dispatcher::setVoiceChannel() {
	uint32_t frequency = 0;
	voice_emission_t emission_type = voiceemissionInvalid;
	switch (main_service->current_mode) {
	case MainServiceInterface::VoiceModeAuto: {
		frequency = (*voice_channel).frequency;
		emission_type = getVoiceEmissionFromFrequency(frequency);
		break;
	}
	case MainServiceInterface::VoiceModeManual: {
		frequency = voice_manual_frequency;
		emission_type = voice_manual_emission_type;
		break;
	}
	}
	DspController::RadioMode mode;
	switch (emission_type) {
	case voiceemissionUSB:
		mode = DspController::RadioModeUSB;
		break;
	case voiceemissionFM:
		mode = DspController::RadioModeFM;
		break;
	case voiceemissionInvalid:
		mode = DspController::RadioModeOff;
		break;
	}
	if ((main_service->current_status == MainServiceInterface::StatusVoiceTx) && atu_controller->isDeviceOperational())
		prepareTuningTx();
	if ((main_service->current_status == MainServiceInterface::StatusVoiceRx) && atu_controller->isDeviceOperational())
		atu_controller->enterBypassMode(frequency);
	dsp_controller->setRadioParameters(mode, frequency);
}

bool Dispatcher::changeVoiceChannel(int number, voice_channel_t type) {
	if (((number-1) < (int)voice_channels_table.size()) && (voice_channels_table[number-1].type == type)) {
		voice_channel = voice_channels_table.begin() + number - 1;
	} else {
		voice_channel = voice_channels_table.end();
		voice_service->setCurrentChannel(VoiceServiceInterface::ChannelInvalid);
		if (main_service->current_mode == MainServiceInterface::VoiceModeAuto) {
			startIdle();
			return false;
		} else {
			return true;
		}
	}
	if (main_service->current_mode == MainServiceInterface::VoiceModeAuto)
		headset_controller->setSmartCurrentChannelSpeed(voice_channels_table[number-1].speed);
	return true;
}

void Dispatcher::updateVoiceChannel() {
	setVoiceChannel();
	if ((main_service->current_status == MainServiceInterface::StatusNotReady)
			|| (main_service->current_status == MainServiceInterface::StatusIdle)) {
		bool ptt_state = false;
		headset_controller->getPTTState(ptt_state);
		setVoiceDirection(ptt_state);
	}
	if (isVoiceMode()) {
		if (voice_channel != voice_channels_table.end()) {
			if ((main_service->current_mode == MainServiceInterface::VoiceModeAuto)
					|| ((main_service->current_mode == MainServiceInterface::VoiceModeManual)
							&& (headset_controller->getStatus() == Headset::Controller::StatusSmartOk)))
				voice_service->setCurrentChannel(VoiceServiceInterface::ChannelActive);
			else
				voice_service->setCurrentChannel(VoiceServiceInterface::ChannelDisabled);
		} else {
			voice_service->setCurrentChannel(VoiceServiceInterface::ChannelInvalid);
		}
	}
}

bool Dispatcher::isVoiceMode() {
	return ((main_service->current_status == MainServiceInterface::StatusVoiceRx) || (main_service->current_status == MainServiceInterface::StatusVoiceTx));
}

bool Dispatcher::isVoiceChannelTunable() {
	return ((voice_channel != voice_channels_table.end())
			&& (headset_controller->getStatus() == Headset::Controller::StatusAnalog)
			&& (main_service->current_mode == MainServiceInterface::VoiceModeAuto));
}

void Dispatcher::processDspSetRadioCompletion() {
	if (main_service->current_status == MainServiceInterface::StatusTuningTx) {
		if (atu_controller->getMode() != AtuController::modeTuning) {
			if (!atu_controller->tuneTxMode(voice_service->getCurrentChannelFrequency())) {
				startVoiceTx();
				voice_service->setCurrentChannel(VoiceServiceInterface::ChannelActive);
			}
		}
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
			atu_controller->tuneTxMode(voice_service->getCurrentChannelFrequency());
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
			atu_controller->enterBypassMode(voice_service->getCurrentChannelFrequency());
			break;
		}
		break;
	}
    case AtuController::modeMalfunction:
    {
        voice_service->atuMalfunction();
        /* no break */
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
