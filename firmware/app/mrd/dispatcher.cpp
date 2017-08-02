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
	dsp_controller = new DspController(dsp_uart_resource, dspreset_iopin_resource, navigator, data_storage_fs, this);
	dsp_controller->started.connect(sigc::mem_fun(this, &Dispatcher::processDspStartup));
	dsp_controller->setRadioCompleted.connect(sigc::mem_fun(this, &Dispatcher::processDspSetRadioCompletion));
	atu_controller = new AtuController(atu_uart_resource, atu_iopin_resource, this);
	atu_controller->modeChanged.connect(sigc::mem_fun(this, &Dispatcher::processAtuModeChange));
	atu_controller->requestTx.connect(sigc::mem_fun(this, &Dispatcher::processAtuRequestTx));

	//dsp_controller->getEmissionType.connect(sigc::mem_fun(this, &Dispatcher::onGetEmissionType));

    ale_service = new AleService(this);

	voice_service = new VoiceServiceInterface(this);
    voice_service->setFS(data_storage_fs);


    latency_draw = new QmTimer();
    latency_draw->setInterval(200);
    latency_draw->timeout.connect(sigc::mem_fun(this,&Dispatcher::latencyGui));

    if (data_storage_fs)
        data_storage_fs->getAleStationAddress(stationAddress);
    dsp_controller->setStationAddress(stationAddress);

	if (!data_storage_fs->getVoiceFrequency(voice_manual_frequency))
		voice_manual_frequency = 10000000;
	if (!data_storage_fs->getVoiceEmissionType(voice_manual_emission_type))
	{
		voice_manual_emission_type = voiceemissionUSB;
		dsp_controller->emissionType = voice_manual_emission_type;
	}
	if (!data_storage_fs->getVoiceChannelSpeed(voice_manual_channel_speed))
		voice_manual_channel_speed = voicespeed1200;

}

Dispatcher::~Dispatcher()
{
    delete dsp_controller;
    delete atu_controller;
    delete ale_service;
    delete voice_service;
}

void Dispatcher::startServicing(const Multiradio::voice_channels_table_t& voice_channels_table) {
	this->voice_channels_table = voice_channels_table;
	voice_channel = this->voice_channels_table.end();
	dsp_controller->startServicing();
	atu_controller->startServicing();
    initAle();
}


void Dispatcher::latencyGui()
{
	latency_draw->stop();
	voice_service->updateChannel();
}

void Dispatcher::initAle()
{
    ale_call_freqs_t call_freqs;
    ale_work_freqs_t work_freqs;
    bool probe_on=true;
    if (data_storage_fs > 0){
        data_storage_fs->getAleDefaultWorkFreqs(work_freqs);
        data_storage_fs->getAleDefaultCallFreqs(call_freqs);
        //	probe_on=data_storage_fs->getAleProbeState();	//	TODO: WRITE THIS FXN !!!
    }
    ale_service->initAle(call_freqs,work_freqs,stationAddress,probe_on);
}

VoiceServiceInterface* Dispatcher::getVoiceServiceInterface()
{
	return voice_service;
}

void Dispatcher::processDspStartup()
{
	setupVoiceMode(headset_controller->getStatus());
	dsp_controller->setAdr();
	navigator->coldStart();
}

bool Dispatcher::processHeadsetPttStateChange(bool new_state)
{
	if (isVoiceMode())
		setVoiceDirection(new_state);
	return true;
}

void Dispatcher::processHeadsetSmartCurrentChannelChange(int new_channel_number,
                                                         voice_channel_t new_channel_type)
{
	if (!dsp_controller->isReady())
		return;
	setSmartChannelMicLevel(new_channel_type);
	if (!changeVoiceChannel(new_channel_number, new_channel_type))
		return;
    updateVoiceChannel((voice_service->current_mode == VoiceServiceInterface::VoiceModeAuto));
}

void Dispatcher::setupVoiceMode(Headset::Controller::Status headset_status)
{
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
			voice_channel = voice_channels_table.end();
			uint8_t analog_ch_number;
			if (data_storage_fs->getAnalogHeadsetChannel(analog_ch_number))
				if (((analog_ch_number-1) < (int)voice_channels_table.size()) && (voice_channels_table[analog_ch_number-1].type == channelOpen))
					voice_channel = voice_channels_table.begin() + analog_ch_number - 1;
			if (voice_channel == voice_channels_table.end())
				voice_channel = std::find_if( std::begin(voice_channels_table), std::end(voice_channels_table),
						[&](const voice_channel_entry_t entry){ return (entry.type == channelOpen); } );
			if (voice_channel == voice_channels_table.end()) {
                if (voice_service->current_mode == VoiceServiceInterface::VoiceModeAuto) {
					voice_service->setCurrentChannel(VoiceServiceInterface::ChannelDisabled);
					startIdle();
					break;
				}
			}
		}
		updateVoiceChannel(true);
		break;
	}
	default: {
		voice_service->setCurrentChannel(VoiceServiceInterface::ChannelDisabled);
        voice_service->setStatus(VoiceServiceInterface::StatusIdle);
		break;
	}
	}
}

void Dispatcher::setSmartChannelMicLevel(voice_channel_t type)
{
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

void Dispatcher::setVoiceDirection(bool ptt_state)
{
	if (ptt_state) {
		if (!atu_controller->isDeviceConnected()) {
			startVoiceTx();
		} else {
			prepareTuningTx();
		}
	} else {
		dsp_controller->setRadioOperation(DspController::RadioOperationRxMode);
        voice_service->setStatus(VoiceServiceInterface::StatusVoiceRx);
	}
}

voice_emission_t Dispatcher::getVoiceEmissionFromFrequency(uint32_t frequency)
{
	if ((1500000 <= frequency) && (frequency < 30000000))
	{
		dsp_controller->emissionType = voiceemissionUSB;
		return voiceemissionUSB;
	}
	else if ((30000000 <= frequency) && (frequency < 300000000))
	{
		dsp_controller->emissionType = voiceemissionFM;
		return voiceemissionFM;
	}
	return voiceemissionInvalid;
}

void Dispatcher::setVoiceChannel()
{
	uint32_t frequency = 0;
	voice_emission_t emission_type = voiceemissionInvalid;
    switch (voice_service->current_mode) {
    case VoiceServiceInterface::VoiceModeAuto: {
		frequency = (*voice_channel).frequency;
		emission_type = getVoiceEmissionFromFrequency(frequency);
		break;
	}
    case VoiceServiceInterface::VoiceModeManual: {
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
    if ((voice_service->current_status == VoiceServiceInterface::StatusVoiceTx) && atu_controller->isDeviceConnected())
		prepareTuningTx();
	dsp_controller->setRadioParameters(mode, frequency);
}

bool Dispatcher::changeVoiceChannel(int number, voice_channel_t type)
{
	if (((number-1) < (int)voice_channels_table.size()) && (voice_channels_table[number-1].type == type)) {
		voice_channel = voice_channels_table.begin() + number - 1;
	} else {
		voice_channel = voice_channels_table.end();
		voice_service->setCurrentChannel(VoiceServiceInterface::ChannelInvalid);
        if (voice_service->current_mode == VoiceServiceInterface::VoiceModeAuto) {
			startIdle();
			return false;
		} else {
			return true;
		}
	}
    if (voice_service->current_mode == VoiceServiceInterface::VoiceModeAuto)
		headset_controller->setSmartCurrentChannelSpeed(voice_channels_table[number-1].speed);
	return true;
}

void Dispatcher::updateVoiceChannel(bool user_request_frequency)
{
	if (user_request_frequency)
		atu_controller->setNextTuningParams(true);
	setVoiceChannel();
    if ((voice_service->current_status == VoiceServiceInterface::StatusNotReady)
            || (voice_service->current_status == VoiceServiceInterface::StatusIdle)) {
		bool ptt_state = false;
		headset_controller->getPTTState(ptt_state);
		setVoiceDirection(ptt_state);
	}
	if (isVoiceMode())
		voice_service->updateChannel();
}

void Dispatcher::saveAnalogHeadsetChannel()
{
	if (voice_channel == voice_channels_table.end())
		return;
	data_storage_fs->setAnalogHeadsetChannel(voice_channel - voice_channels_table.begin() + 1);
}

bool Dispatcher::isVoiceMode()
{
    return ((voice_service->current_status == VoiceServiceInterface::StatusVoiceRx)
         || (voice_service->current_status == VoiceServiceInterface::StatusVoiceTx));
}

bool Dispatcher::isVoiceChannelTunable()
{
	return ((voice_channel != voice_channels_table.end())
			&& (headset_controller->getStatus() == Headset::Controller::StatusAnalog)
            && (voice_service->current_mode == VoiceServiceInterface::VoiceModeAuto));
}

void Dispatcher::processDspSetRadioCompletion()
{
    switch (voice_service->current_status) {
    case VoiceServiceInterface::StatusTuningTx:
		if (atu_controller->getMode() != AtuController::modeTuning) {
			if (!atu_controller->tuneTxMode(voice_service->getCurrentChannelFrequency())) {
				startVoiceTx();
				voice_service->updateChannel();
			}
		}
		break;
    case VoiceServiceInterface::StatusVoiceRx:
		atu_controller->enterBypassMode(voice_service->getCurrentChannelFrequency());
		break;
	default:
		break;
	}
}

void Dispatcher::startIdle()
{
    if (voice_service->current_status == VoiceServiceInterface::StatusIdle)
		return;
	dsp_controller->setRadioOperation(DspController::RadioOperationOff);
//	if (atu_controller->isDeviceOperational())
//		atu_controller->enterBypassMode();
    voice_service->setStatus(VoiceServiceInterface::StatusIdle);
}

void Dispatcher::startVoiceTx()
{
	atu_controller->setRadioPowerOff(false);
	dsp_controller->setRadioOperation(DspController::RadioOperationTxMode);
    voice_service->setStatus(VoiceServiceInterface::StatusVoiceTx);
}

void Dispatcher::prepareTuningTx()
{
//	dsp_controller->setRadioOperation(DspController::RadioOperationOff);
	atu_controller->setRadioPowerOff(true);
	dsp_controller->setRadioOperation(DspController::RadioOperationCarrierTx);
    voice_service->setStatus(VoiceServiceInterface::StatusTuningTx);
}

void Dispatcher::processAtuModeChange(AtuController::Mode new_mode)
{
	switch (new_mode) {
	case AtuController::modeNone: {
		atu_controller->setNextTuningParams(true);
		break;
	}
	case AtuController::modeBypass: {
        switch (voice_service->current_status) {
        case VoiceServiceInterface::StatusVoiceTx:
			prepareTuningTx();
			break;
        case VoiceServiceInterface::StatusTuningTx:
			atu_controller->tuneTxMode(voice_service->getCurrentChannelFrequency());
			break;
		default: break;
		}
		break;
	}
	case AtuController::modeActiveTx: {
		atu_controller->setNextTuningParams(false);
        switch (voice_service->current_status) {
        case VoiceServiceInterface::StatusTuningTx:
			startVoiceTx();
			//voice_service->updateChannel();
			latency_draw->start();
			break;
		default:
			atu_controller->enterBypassMode(voice_service->getCurrentChannelFrequency());
			break;
		}
		break;
	}
    case AtuController::modeMalfunction:
    {
        atu_controller->setNextTuningParams(false);
        voice_service->atuMalfunction();
        /* no break */
    }
	default: {
        if ((voice_service->current_status == VoiceServiceInterface::StatusTuningTx)
				&& ((new_mode == AtuController::modeMalfunction) || !atu_controller->isDeviceConnected())) {
			startVoiceTx();
			voice_service->updateChannel();
		}
		break;
	}
	}
}

void Dispatcher::processAtuRequestTx(bool enable)
{
    if (voice_service->current_status != VoiceServiceInterface::StatusTuningTx)
		return;
	dsp_controller->setRadioOperation((enable)?(DspController::RadioOperationCarrierTx):(DspController::RadioOperationOff));
}

void Dispatcher::DspReset()
{
	dsp_controller->dspReset();
}

//voice_emission_t Dispatcher::onGetEmissionType()
//{
//	return voice_manual_emission_type;
//}

} /* namespace Multiradio */

#include "qmdebug_domains_start.h"
QMDEBUG_DEFINE_DOMAIN(mrd, LevelDefault)
#include "qmdebug_domains_end.h"
