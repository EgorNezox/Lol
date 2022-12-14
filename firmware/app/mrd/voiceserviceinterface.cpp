/**
 ******************************************************************************
 * @file    voiceserviceinterface.cpp
 * @author  Artem Pisarenko, PMR dept. software team, ONIIP, PJSC
 * @author  неизвестные
 * @date    30.10.2015
 *
 ******************************************************************************
 */

#include "voiceserviceinterface.h"

namespace Multiradio {

//typedef AleService::AleState AleState;

VoiceServiceInterface::VoiceServiceInterface(Dispatcher *dispatcher) :
	QmObject(dispatcher),
	dispatcher(dispatcher),
	current_channel_status(ChannelDisabled)
{
    dispatcher->dsp_controller->started.connect(sigc::mem_fun(this,&VoiceServiceInterface::onDspStarted));
    dispatcher->dsp_controller->firstPacket.connect(sigc::mem_fun(this,&VoiceServiceInterface::fistPacketRecieve));

    dispatcher->dsp_controller->recievedGucResp.connect(sigc::mem_fun(this,&VoiceServiceInterface::responseGuc));
    dispatcher->dsp_controller->recievedGucQuitForTransm.connect(sigc::mem_fun(this,&VoiceServiceInterface::messageGucQuit));
    dispatcher->dsp_controller->gucCrcFailed.connect(sigc::mem_fun(this,&VoiceServiceInterface::gucCrcFail));

    dispatcher->dsp_controller->smsPacketMessage.connect(sigc::mem_fun(this,&VoiceServiceInterface::smsMessage));
    dispatcher->dsp_controller->smsFailed.connect(sigc::mem_fun(this,&VoiceServiceInterface::SmsFailStage));
    dispatcher->dsp_controller->smsCounterChanged.connect(sigc::mem_fun(this,&VoiceServiceInterface::onSmsCounterChange));

    dispatcher->dsp_controller->TxCondCmdPackageTransmit.connect(sigc::mem_fun(this,&VoiceServiceInterface::TxCondCmdTransmit));
    dispatcher->dsp_controller->startRxQuit.connect(sigc::mem_fun(this,&VoiceServiceInterface::startRxQuit));
    dispatcher->dsp_controller->stationModeIsCompleted.connect(sigc::mem_fun(this,&VoiceServiceInterface::onStationModeIsCompleted));
    dispatcher->dsp_controller->reciveRssiLevel.connect(sigc::mem_fun(this,&VoiceServiceInterface::RxRSSI));

    dispatcher->dsp_controller->waveInfoRecieved.connect(sigc::mem_fun(this,&VoiceServiceInterface::onWaveInfoRecieved));

    dispatcher->dsp_controller->rxModeSetting.connect(sigc::mem_fun(this,&VoiceServiceInterface::onRxModeSetting));
    dispatcher->dsp_controller->txModeSetting.connect(sigc::mem_fun(this,&VoiceServiceInterface::onTxModeSetting));

    dispatcher->ale_service->aleStateChanged.connect(sigc::mem_fun(this, &VoiceServiceInterface::updateAleState));
    dispatcher->ale_service->aleVmProgressUpdated.connect(sigc::mem_fun(this, &VoiceServiceInterface::updateAleVmProgress));

    dispatcher->ale_service->settingAleFreq.connect(sigc::mem_fun(this, &VoiceServiceInterface::onSettingAleFreq));

    dispatcher->dsp_controller->startCondReceiving.connect(sigc::mem_fun(this,&VoiceServiceInterface::onStartCondReceiving));

    dispatcher->dsp_controller->virtualCounterChanged.connect(sigc::mem_fun(this,&VoiceServiceInterface::onVirtualCounterChanged));

    dispatcher->dsp_controller->qwitCounterChanged.connect(sigc::mem_fun(this,&VoiceServiceInterface::onQwitCounterChanged));

    dispatcher->dsp_controller->transmitAsk.connect(sigc::mem_fun(this,&VoiceServiceInterface::onTransmitAsk));

    dispatcher->dsp_controller->keyEmulate.connect(sigc::mem_fun(this, &VoiceServiceInterface::onKeyEmulate));

    dispatcher->dsp_controller->recivedVersions.connect(sigc::mem_fun(this, &VoiceServiceInterface::recivedVersions));

    current_status = StatusNotReady;
    current_mode = VoiceModeManual;

    dispatcher->dsp_controller->hardwareFailed.connect(sigc::mem_fun(this, &VoiceServiceInterface::forwardDspHardwareFailure));
    dispatcher->dsp_controller->emulateKey.connect(sigc::mem_fun(this, &VoiceServiceInterface::emulatorKey));


    dispatcher->dsp_controller->smsCounterFreq.connect(sigc::mem_fun(this,&VoiceServiceInterface::onSmsFreq));
}


void VoiceServiceInterface::onSmsFreq(int param)
{
	smsFreq(param);

}



void VoiceServiceInterface::emulatorKey(int key)
{
	emulKey(key);
}

VoiceServiceInterface::~VoiceServiceInterface()
{

}

void VoiceServiceInterface::startRxQuit()
{
	startRxQuitSignal();
}


void VoiceServiceInterface::forwardDspHardwareFailure(uint8_t subdevice_code, uint8_t error_code)
{
	dspHardwareFailed(subdevice_code,error_code);
}

void VoiceServiceInterface::messageGucQuit(int ans){
    messageGucTxQuit(ans);
}

VoiceServiceInterface::ChannelStatus VoiceServiceInterface::getCurrentChannelStatus()
{
	return current_channel_status;
}

int VoiceServiceInterface::getCurrentChannelNumber()
{
    int number = 0;
    switch (current_channel_status) {
    case ChannelDisabled:
    	break;
    case ChannelActive:
    case ChannelInvalid:
    	if (dispatcher->voice_channel != dispatcher->voice_channels_table.end()) {
    		number = dispatcher->voice_channel - dispatcher->voice_channels_table.begin() + 1;
    	} else {
    		int smart_ch_number;
    		voice_channel_t smart_ch_type;
    		if (dispatcher->headset_controller->getSmartCurrentChannel(smart_ch_number, smart_ch_type))
    			number = smart_ch_number;
    	}
    	break;
    }
    return number;
}

int VoiceServiceInterface::getCurrentChannelFrequency()
{
    switch (current_mode) {
    case VoiceModeAuto: {
		if (current_channel_status != ChannelDisabled)
			return dispatcher->voice_channels_table.at(getCurrentChannelNumber()-1).frequency;
		break;
	}
    case VoiceModeManual: {
		return dispatcher->voice_manual_frequency;
	}
	}
	return 0;
}

voice_emission_t VoiceServiceInterface::getCurrentChannelEmissionType() {
    switch (current_mode) {
    case VoiceModeAuto: {
		if (current_channel_status != ChannelDisabled) {
			uint32_t frequency = dispatcher->voice_channels_table.at(getCurrentChannelNumber()-1).frequency;
			return dispatcher->getVoiceEmissionFromFrequency(frequency);
		}
		break;
	}
    case VoiceModeManual: {
		return dispatcher->voice_manual_emission_type;
	}
	}
	return voiceemissionInvalid;
}

voice_channel_t VoiceServiceInterface::getCurrentChannelType() {
	if (current_channel_status != ChannelActive)
		return channelInvalid;
	return (*(dispatcher->voice_channel)).type;
}

voice_channel_speed_t VoiceServiceInterface::getCurrentChannelSpeed()
{
    switch (current_mode) {
    case VoiceModeAuto: {
		if (current_channel_status != ChannelDisabled)
			return dispatcher->voice_channels_table.at(getCurrentChannelNumber()-1).speed;
		break;
	}
    case VoiceModeManual: {
		return dispatcher->voice_manual_channel_speed;
	}
	}
	return voicespeedInvalid;
}

void VoiceServiceInterface::setCurrentChannelSpeed(voice_channel_speed_t speed) {
	dispatcher->data_storage_fs->setVoiceChannelSpeed(speed);
	dispatcher->voice_manual_channel_speed = speed;
    if (!((current_mode == VoiceModeManual)
			&& (dispatcher->headset_controller->getStatus() == Headset::Controller::StatusSmartOk)))
		return;
	dispatcher->headset_controller->setSmartCurrentChannelSpeed(speed);
}

void VoiceServiceInterface::tuneChannel(uint8_t channel)
{
	if (!dispatcher->isVoiceChannelTunable())
		return;

	if (channel < dispatcher->voice_channels_table.size())
	{
		dispatcher->voice_channel = dispatcher->voice_channels_table.begin();
		dispatcher->voice_channel += channel;
		dispatcher->voice_channel--;

		if ((*dispatcher->voice_channel).type != channelOpen)
			dispatcher->voice_channel--;
	}

	dispatcher->updateVoiceChannel(true);
	dispatcher->saveAnalogHeadsetChannel();
}

void VoiceServiceInterface::tuneNextChannel() {
	if (!dispatcher->isVoiceChannelTunable())
		return;
	auto current_channel = dispatcher->voice_channel;
	bool wrapped = false;
	while ((dispatcher->voice_channel != current_channel) || !wrapped) {
		++dispatcher->voice_channel;
		if (dispatcher->voice_channel == dispatcher->voice_channels_table.end()) {
			dispatcher->voice_channel = dispatcher->voice_channels_table.begin();
			wrapped = true;
		}
		if ((*dispatcher->voice_channel).type == channelOpen)
			break;
	}
	dispatcher->updateVoiceChannel(true);
	dispatcher->saveAnalogHeadsetChannel();
}

void VoiceServiceInterface::tunePreviousChannel() {
	if (!dispatcher->isVoiceChannelTunable())
		return;
	auto current_channel = dispatcher->voice_channel;
	bool wrapped = false;
	while ((dispatcher->voice_channel != current_channel) || !wrapped) {
		if (dispatcher->voice_channel == dispatcher->voice_channels_table.begin()) {
			dispatcher->voice_channel = dispatcher->voice_channels_table.end();
			wrapped = true;
		}
		--dispatcher->voice_channel;
		if ((*dispatcher->voice_channel).type == channelOpen)
			break;
	}
	dispatcher->updateVoiceChannel(true);
	dispatcher->saveAnalogHeadsetChannel();
}

void VoiceServiceInterface::tuneFrequency(int frequency, bool isRecord)
{
	dispatcher->voice_manual_frequency = frequency;
    if (isRecord && current_mode != VoiceModeManual)
		return;
    dispatcher->updateVoiceChannel(true);
    if (isRecord)
        dispatcher->data_storage_fs->setVoiceFrequency(frequency);
}

void VoiceServiceInterface::tuneEmissionType(voice_emission_t type) {
	dispatcher->data_storage_fs->setVoiceEmissionType(type);
	dispatcher->voice_manual_emission_type = type;
	dispatcher->dsp_controller->emissionType = type;
    if (current_mode != VoiceModeManual)
		return;
    dispatcher->updateVoiceChannel(false);
}

void VoiceServiceInterface::newTuneEmissionType(voice_emission_t type)
{
	dispatcher->data_storage_fs->setVoiceEmissionType(type);
	dispatcher->voice_manual_emission_type = type;
	dispatcher->dsp_controller->emissionType = type;
	if (current_mode != VoiceModeManual)
		return;

	dispatcher->dsp_controller->newTuneEType();
}

void VoiceServiceInterface::newUsbState(uint8_t state)
{
	dispatcher->data_storage_fs->setUsbOnOff(state);
}

void VoiceServiceInterface::tuneSquelch(uint8_t value) {
	if (/*value != 0 &&*/ (value < 0 || value > 24)) {
		return;
	}
	dispatcher->dsp_controller->setRadioSquelch(value);
}

void VoiceServiceInterface::TuneAudioLevel(uint8_t volume_level)
{
    dispatcher->dsp_controller->setAudioVolumeLevel(volume_level);
}

void VoiceServiceInterface::TuneMicLevel(uint8_t mic_level)
{
	dispatcher->dsp_controller->setAudioMicLevel(mic_level);
}

void VoiceServiceInterface::TurnAGCMode(uint8_t mode, int radio_path)
{
    dispatcher->dsp_controller->setAGCParameters(mode, radio_path);
}

void VoiceServiceInterface::TurnPSWFMode(uint8_t mode, int cmd, int r_adr, int retr)
{

	if ((r_adr == 0) && (cmd ==0))
	{
		dispatcher->dsp_controller->startPSWFReceiving();
	}
	else
	{
		if (mode == 0)
			dispatcher->dsp_controller->startPSWFTransmitting(false, r_adr, cmd, retr);
		else
			dispatcher->dsp_controller->startPSWFTransmitting(true, r_adr, cmd, retr);
	}

}

const char* VoiceServiceInterface::ReturnSwfStatus()
{
    const char *text;
    if (dispatcher->dsp_controller->swf_res <=2)
        text = error_SWF;
    else
        text = true_SWF;

    return text;
}

void VoiceServiceInterface::getSmsForUiStage(int value)
{
    getSmsStageUi(value);
}

void VoiceServiceInterface::gucCrcFail()
{
    gucCrcFailed();
}

void VoiceServiceInterface::turnVirtualPswfTx()
{
	dispatcher->dsp_controller->startVirtualPpsModeTx();
}

void VoiceServiceInterface::turnVirtualPswfRx()
{
	dispatcher->dsp_controller->startVirtualPpsModeRx();
}

void VoiceServiceInterface::defaultSMSTrans()
{
	dispatcher->dsp_controller->defaultSMSTransmit();
}

void VoiceServiceInterface::TurnSMSMode(int r_adr, char *message, uint8_t retr)
{
   dispatcher->dsp_controller->setSmsRetranslation(retr);
   dispatcher->dsp_controller->sms_counter = 0;
   dispatcher->dsp_controller->SmsLogicRole = dispatcher->dsp_controller->SmsRoleTx;
   dispatcher->dsp_controller->startSMSTransmitting(r_adr,(uint8_t*)message);
}

void VoiceServiceInterface::TurnSMSMode()
{
	dispatcher->dsp_controller->sms_counter = 0;
	dispatcher->dsp_controller->SmsLogicRole = dispatcher->dsp_controller->SmsRoleRx;
    dispatcher->dsp_controller->startSMSRecieving();
}

void VoiceServiceInterface::SmsFailStage(int stage)
{
    smsFailed(stage);
}

void VoiceServiceInterface::setRnKey(int value)
{
   dispatcher->dsp_controller->setRnKey(value);
}

uint8_t* VoiceServiceInterface::getVirtualTime()
{
	uint8_t *pointer = 0;
	pointer = dispatcher->dsp_controller->getVirtualTime();
	return pointer;
}


void VoiceServiceInterface::TurnGuc(int r_adr, int speed_tx, std::vector<int> command, bool isGps)
{
    dispatcher->dsp_controller->startGucTransmitting(r_adr,speed_tx,command, isGps);
}

uint8_t* VoiceServiceInterface::requestGucCoord(){
	return dispatcher->dsp_controller->getGucCoord();
}

uint8_t* VoiceServiceInterface::getGucCommand()
{
	return dispatcher->dsp_controller->get_guc_vector();
}

void VoiceServiceInterface::TurnGuc()
{
	dispatcher->dsp_controller->startGucRecieving();
}

char* VoiceServiceInterface::getSmsContent()
{
	return dispatcher->dsp_controller->getSmsContent();
}

void VoiceServiceInterface::setCurrentChannel(ChannelStatus status) {
	current_channel_status = status;
    currentChannelChanged();
}

bool VoiceServiceInterface::getDrawWithAnsy()
{
	return dispatcher->flagDrawWithAnsy;
}

void VoiceServiceInterface::setDrawWithAnsy(bool flagDrawWithAnsy)
{
	dispatcher->flagDrawWithAnsy = flagDrawWithAnsy;
}


void VoiceServiceInterface::updateChannel() {
	if (dispatcher->voice_channel != dispatcher->voice_channels_table.end())
	{
        if ((current_mode == VoiceModeAuto) ||
           ((current_mode == VoiceModeManual) &&
           (dispatcher->headset_controller->getStatus() == Headset::Controller::StatusSmartOk)))
			setCurrentChannel(ChannelActive);
		else
			setCurrentChannel(ChannelDisabled);
	}
	else
	{
		setCurrentChannel(ChannelInvalid);
	}
}

void VoiceServiceInterface::fistPacketRecieve(int packet, uint8_t address, bool rec)
{
    firstPacket(packet, address, rec);
}

void VoiceServiceInterface::saveFreq(int value)
{
    dispatcher->dsp_controller->setFreq(value);
}

void VoiceServiceInterface::responseGuc(int value, bool isTxAsk)
{
    respGuc(value, isTxAsk);
}

void VoiceServiceInterface::smsMessage(int value)
{
    smsMess(value);
}

void VoiceServiceInterface::sendBatteryVoltage(int voltage)
{
   dispatcher->dsp_controller->sendBatteryVoltage(voltage);
}

void VoiceServiceInterface::sendHeadsetType(uint8_t type)
{
   dispatcher->dsp_controller->sendHeadsetType(type);
}

void VoiceServiceInterface::goToVoice()
{
    dispatcher->dsp_controller->goToVoice();
}

uint8_t VoiceServiceInterface::getSmsCounter()
{
   return dispatcher->dsp_controller->getSmsCounter();

}

void VoiceServiceInterface::resetDSPLogic()
{
	dispatcher->dsp_controller->initResetState();
}

std::string VoiceServiceInterface::getVirtualDate()
{
	uint8_t day, month, year;
	dispatcher->dsp_controller->getVirtualDate(&day, &month, &year);

    char res[8] = {0,0,0,0,0,0,0,0};
    sprintf(res, "%02u.%02u.%02u", day, month, year);

    return std::string(res);
}

void VoiceServiceInterface::setVirtualDate(std::string s)
{
    uint8_t cnt = 0;

    if (s.length() == 8)
    {
        for(int i = 0; i<8;i++)
        {
            if (s[i] != '.')
            {
                param[cnt] = s[i];
                ++cnt;
            }

        }

        dispatcher->dsp_controller->setVirtualDate(param);
    }

}

void VoiceServiceInterface::setVirtualTime(std::string s)
{
    uint8_t cnt = 0;
    for(int i = 0; i< s.length();i++)
    {
        if (s[i] != ':')
        {
            param[cnt] = s[i]; ++cnt;
        }
    }

    dispatcher->dsp_controller->setVirtualTime(param);
}

void VoiceServiceInterface::TxCondCmdTransmit(int value)
{
    command_tx30(value);
}

void VoiceServiceInterface::RxRSSI(int value)
{
	rxRssiLevel(value);
}

void VoiceServiceInterface::onSmsCounterChange(int param)
{
   smsCounterChanged(param);
}

bool VoiceServiceInterface::getIsGucCoord()
{
    return dispatcher->dsp_controller->getIsGucCoord();
}

void VoiceServiceInterface::setVirtualMode(bool param)
{
	dispatcher->dsp_controller->setVirtualMode(param);
	dispatcher->ale_service->setManualTimeMode(param);
}

bool VoiceServiceInterface::getVirtualMode()
{
	return dispatcher->dsp_controller->getVirtualMode();
}

void VoiceServiceInterface::playSoundSignal(uint8_t mode, uint8_t speakerVolume, uint8_t gain, uint8_t soundNumber, uint8_t duration, uint8_t micLevel)
{
    dispatcher->dsp_controller->playSoundSignal(mode, speakerVolume, gain, soundNumber, duration, micLevel);
}

uint8_t VoiceServiceInterface::playVoiceMessage(uint8_t fileNumber, DataStorage::FS::TransitionFileType transFileType, uint8_t num)
{
    Headset::Controller::Status status;
    uint8_t result = 1; // error read
    if (storageFs > 0)
    {
        voice_message_t msg;
        if (storageFs->readMessage(DataStorage::FS::FT_VM,transFileType,&msg,fileNumber))
           result = 0;
        dispatcher->headset_controller->setSmartMessageToPlay(msg);
        status = dispatcher->headset_controller->getStatus();
        if (status == Headset::Controller::Status::StatusNone)
            result = 2;
        else
        {
            dispatcher->headset_controller->startSmartPlay(num);
        }
    }
    return result;
}

uint16_t VoiceServiceInterface::smsSender()
{
	return dispatcher->dsp_controller->sender;
}

void VoiceServiceInterface::setFS(DataStorage::FS *fs)
{
    storageFs = fs;
}

void VoiceServiceInterface::startAleRx()
{
    dispatcher->ale_service->startAleRx();
}

void VoiceServiceInterface::startAleTx(uint8_t address, voice_message_t message)
{
	dispatcher->dsp_controller->stopGucIntoVoice();
    dispatcher->ale_service->startAleTxVoiceMail(address, message);
}

void VoiceServiceInterface::stopAle()
{
    dispatcher->ale_service->stopAle();
}

int VoiceServiceInterface::getAleState()	//AleState VoiceServiceInterface::getAleState()
{
   return dispatcher->ale_service->getAleState();
}

uint8_t VoiceServiceInterface::getAleVmProgress()
{
    return dispatcher->ale_service->getAleVmProgress();
}

Multiradio::voice_message_t VoiceServiceInterface::getAleRxVmMessage()
{
    return dispatcher->ale_service->getAleRxVmMessage();
}

uint8_t VoiceServiceInterface::getStationAddress()
{
    return dispatcher->stationAddress;
}

uint8_t VoiceServiceInterface::getAleRxAddress()
{
    return dispatcher->ale_service->getAleRxAddress();
}

VoiceServiceInterface::Status VoiceServiceInterface::getStatus()
{
    return current_status;
}

void VoiceServiceInterface::setVoiceMode(VoiceMode mode)
{
    if (mode == current_mode)
        return;
    current_mode = mode;
    if ((mode == VoiceModeAuto) && !dispatcher->isVoiceMode())
        return;
    dispatcher->updateVoiceChannel(true);
    dispatcher->headset_controller->setSmartCurrentChannelSpeed(getCurrentChannelSpeed());
}

VoiceServiceInterface::VoiceMode VoiceServiceInterface::getVoiceMode()
{
    return current_mode;
}

void VoiceServiceInterface::setStatus(Status value)
{
    if (current_status != value) {
        current_status = value;
        statusChanged(value);
    }
}

void VoiceServiceInterface::setWorkAtu(uint8_t antenna)
{
	antennaType = antenna;
}

uint8_t VoiceServiceInterface::getWorkAtu()
{
	return antennaType;
}

void VoiceServiceInterface::updateAleState(int state)
{
	aleStateChanged((AleState)state);
}

void VoiceServiceInterface::updateAleVmProgress(uint8_t progress)
{
	aleVmProgressUpdated(progress);
}

void VoiceServiceInterface::onWaveInfoRecieved(float wave, float power)
{
    waveInfoRecieved(wave, power);
}

void VoiceServiceInterface::onRxModeSetting()
{
    rxModeSetting();
}

void VoiceServiceInterface::onTxModeSetting()
{
    txModeSetting();
}

void VoiceServiceInterface::onSettingAleFreq(uint32_t freq)
{
    settingAleFreq(freq);
}

void VoiceServiceInterface::setSwrTimerState(bool state)
{
//	if (state && dispatcher->dsp_controller->isReady())
//		dispatcher->dsp_controller->swr_timer->start();
//	else
//		dispatcher->dsp_controller->swr_timer->stop();

}

void VoiceServiceInterface::onStartCondReceiving()
{
	startCondReceiving();
}

void VoiceServiceInterface::onVirtualCounterChanged(uint8_t counter)
{
	virtualCounterChanged(counter);
}

void VoiceServiceInterface::onQwitCounterChanged(uint8_t counter)
{
	qwitCounterChanged(counter);
}

void VoiceServiceInterface::onTransmitAsk(bool on)
{
	transmitAsk(on);
}

void VoiceServiceInterface::stopGucQuit()
{
	if (dispatcher->dsp_controller->guc_rx_quit_timer != 0)
		dispatcher->dsp_controller->guc_rx_quit_timer->stop();
}


void VoiceServiceInterface::onKeyEmulate(int key)
{
	keyEmulate(key);
}

void VoiceServiceInterface::recivedVersions(uint16_t versionDSP, uint16_t versionPLD)
{
	sigRecivedVersions(versionDSP, versionPLD);
}

} /* namespace Multiradio */
