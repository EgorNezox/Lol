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
#include "qmiopin.h"
#include "../../system/platform_hw_map.h"

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
	headset_controller->statusChanged.connect              (sigc::mem_fun(this, &Dispatcher::setupVoiceMode));
	headset_controller->pttStateChanged.connect            (sigc::mem_fun(this, &Dispatcher::processHeadsetPttStateChange));
	headset_controller->smartCurrentChannelChanged.connect (sigc::mem_fun(this, &Dispatcher::processHeadsetSmartCurrentChannelChange));

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
    headset_controller->autoSpeedChanged.connect(sigc::mem_fun(this, &Dispatcher::returnSpeed));

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

	pin_debug = new QmIopin(platformhwDebugIoPin, this);
	//pin_debug->writeOutput(QmIopin::Level_Low);
}

void Dispatcher::setFlash(QmM25PDevice *device)
{
	this->dsp_controller->mydevice = device;
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
	//atu_controller->startServicing();
    initAle();
}

void Dispatcher::startAtu()
{
	atu_controller->startServicing();
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
	dsp_controller->setAdr();
	setupVoiceMode(headset_controller->getStatus());
#ifdef PORT__TARGET_DEVICE_REV1
    navigator->coldStart();
#endif
    dsp_controller->setAudioVolumeLevel(100);
}

bool Dispatcher::processHeadsetPttStateChange(bool new_state)
{
	//pin_debug->writeOutput(QmIopin::Level_High);

	//qmDebugMessage(QmDebug::Dump, "PttStateChange MODE = %d", voice_service->current_status);
	static bool lastChenged= false;
	static bool lastVoiseStart = false;

	if (!new_state)
	{
		QmThread::msleep(5);
//		if (atu_controller->isDeviceConnected())
//			//dsp_controller->setRadioOperation(DspController::RadioOperationOff);
			//dsp_controller->setAtuTXOff();
		setVoiceDirection(false);
	}



	//qmDebugMessage(QmDebug::Info, "atu MODE = %d", atu_controller->getMode());


	if (isVoiceMode())
	{
		if (atu_controller->isDeviceConnected() && lastChenged && new_state && isCurrentFreq())
		{
			//dsp_controller->setRadioCompleted();
			dsp_controller->VoiceStart();
			flagDrawWithAnsy = false;
			//lastChenged= false;
			voice_service->setStatus(VoiceServiceInterface::StatusVoiceTx);
		}
		else
		{
			setVoiceDirection(new_state);
			//qmDebugMessage(QmDebug::Info, "atu MODE = %d", atu_controller->getMode());
			if (atu_controller->getMode() ==  5)
				lastChenged= true;
		}
	}

	//pin_debug->writeOutput(QmIopin::Level_Low);

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

	uint8_t type_garn = 1;
	if (!dsp_controller->isReady())
		return;

	switch (headset_status) {
	case Headset::Controller::StatusAnalog:
	case Headset::Controller::StatusSmartOk:
	{
		if (headset_status == Headset::Controller::StatusSmartOk)
		{
			int smart_ch_number = 1;
			voice_channel_t smart_ch_type = Multiradio::channelInvalid;
			headset_controller->getSmartCurrentChannel(smart_ch_number, smart_ch_type);

			// устанаваливаем усиление микрофона - если канал закрытый 16, если открытый 24
			setSmartChannelMicLevel(smart_ch_type);

			// если канал открытый, то тип 2, если закрытый, то 0 тип
			type_garn = (smart_ch_type == channelOpen) ? 0 : 2;

			// устанавливаем канал
			if (!changeVoiceChannel(smart_ch_number, smart_ch_type))
				break;

		}
		else
		{
			// MRU
			dsp_controller->setAudioMicLevel(255);

			// установить тип полевой гарнитуры
			type_garn = 1;

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
		// установка уровня громкости
		dsp_controller->setAudioTypeGarniture(type_garn);
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
		if (!atu_controller->isDeviceConnected())
		{
			startVoiceTx();
		}
		else
		{
			static uint32_t prev_freq = 0;

			uint32_t curr_freq = voice_service->getCurrentChannelFrequency();

			if (prev_freq == curr_freq)
			{
				tuningTxCurrFreq();
			}
			else
			{
				prepareTuningTx();
			}

			prev_freq = curr_freq;


		}
	}
	else
	{
		//dsp_controller->setRadioOperation(DspController::RadioOperationRxMode);
		if (atu_controller->isDeviceConnected())
			atu_controller->enterBypassMode(voice_service->getCurrentChannelFrequency());
		dsp_controller->VoiceStop();
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

    	if (voice_channel != voice_channels_table.end())
    	{
    		frequency = (*voice_channel).frequency;
    		emission_type = getVoiceEmissionFromFrequency(frequency);
    	}
    	else
    	{
    		voice_channel = voice_channels_table.begin();
    	}
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


void Dispatcher::returnSpeed()
{
    // добавлено для возврата на скорость, установленную на автоканале
	int number = 0; Multiradio::voice_channel_t type;
	headset_controller->getSmartCurrentChannel(number,type);

	QmThread::msleep(2000);

	if (voice_service->current_mode == VoiceServiceInterface::VoiceModeAuto)
		headset_controller->setSmartCurrentChannelSpeed(voice_channels_table[number-1].speed);
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
	dsp_controller->setRadioOperation(DspController::RadioOperationTxMode);
    voice_service->setStatus(VoiceServiceInterface::StatusVoiceTx);
}

void Dispatcher::prepareTuningTx()
{
	/* send cmd for current and rx off */
	dsp_controller->ansuTxMode();

	/* need syncronize with dsp rx cadr*/
	QmThread::msleep(50);

	uint8_t typeAnt = voice_service->getWorkAtu();

	/* get frequency */
	uint32_t freq = (uint32_t)voice_service->getCurrentChannelFrequency();

	atu_controller->setAntenna(typeAnt);

	/* check freq valid, if not set in bypass */
	bool isNotBypass = atu_controller->checkFeq(freq);

	if (isNotBypass)
	{
		atu_controller->setFreq(freq);
		atu_controller->executeTuneTxMode();
	}

    voice_service->setStatus(VoiceServiceInterface::StatusTuningTx);
}

void Dispatcher::tuningTxCurrFreq()
{
	//uint32_t freq = (uint32_t)voice_service->getCurrentChannelFrequency();

	//bool isNotBypass = atu_controller->checkFeq(freq);

	//if (isNotBypass)
	{
		dsp_controller->ansuTxCurrFreq();
		//atu_controller->setFreq(freq);
		atu_controller->executeTuneTxMode();
	}

}

bool Dispatcher::isCurrentFreq()
{
	static int freq_prev = 0;
	bool res = false;
	int freq = voice_service->getCurrentChannelFrequency();
	if (freq_prev > 0 && freq_prev == freq)
		res = true;

	freq_prev = freq;
	return res;
}

void Dispatcher::processAtuModeChange(AtuController::Mode new_mode)
{
	switch (new_mode)
	{
	case AtuController::modeNone:
	{
		//atu_controller->setNextTuningParams(true);
		break;
	}
	case AtuController::modeBypass:
	{
//        switch (voice_service->current_status)
//        {
//        case VoiceServiceInterface::StatusVoiceTx:
//			prepareTuningTx();
//			break;
//        case VoiceServiceInterface::StatusTuningTx:
		//atu_controller->tuneTxMode(voice_service->getCurrentChannelFrequency());
		//atu_controller->executeTuneTxMode();
//			break;
//		default: break;
//		}
//		break;
	}
	case AtuController::modeActiveTx:
	{
		atu_controller->setNextTuningParams(false);
        switch (voice_service->current_status)
        {
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
    case AtuController::modeFault:
    {
        atu_controller->setNextTuningParams(false);
        setVoiceDirection(false);
        voice_service->atuMalfunction(atu_controller->error);
        break;
    }
	default:
	{
        if ((voice_service->current_status == VoiceServiceInterface::StatusTuningTx)
				&& ((new_mode == AtuController::modeFault) || !atu_controller->isDeviceConnected()))
        {
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
QMDEBUG_DEFINE_DOMAIN(mrd, LevelVerbose)
#include "qmdebug_domains_end.h"
