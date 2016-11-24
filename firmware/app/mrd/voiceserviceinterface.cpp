/**
 ******************************************************************************
 * @file    voiceserviceinterface.cpp
 * @author  Artem Pisarenko, PMR dept. software team, ONIIP, PJSC
 * @author  неизвестные
 * @date    30.10.2015
 *
 ******************************************************************************
 */

#include "qm.h"

#include "voiceserviceinterface.h"
#include "mainserviceinterface.h"
#include "dispatcher.h"
#include <math.h>
#include "../datastorage/fs.h"


namespace Multiradio {

VoiceServiceInterface::VoiceServiceInterface(Dispatcher *dispatcher) :
	QmObject(dispatcher),
	dispatcher(dispatcher),
	current_channel_status(ChannelDisabled)
{
    dispatcher->dsp_controller->firstPacket.connect(sigc::mem_fun(this,&VoiceServiceInterface::fistPacketRecieve));
    dispatcher->dsp_controller->smsPacketMessage.connect(sigc::mem_fun(this,&VoiceServiceInterface::smsMessage));
    dispatcher->dsp_controller->smsFailed.connect(sigc::mem_fun(this,&VoiceServiceInterface::SmsFailStage));
    dispatcher->dsp_controller->recievedGucResp.connect(sigc::mem_fun(this,&VoiceServiceInterface::responseGuc));
    dispatcher->dsp_controller->recievedGucQuitForTransm.connect(sigc::mem_fun(this,&VoiceServiceInterface::messageGucQuit));
	dispatcher->dsp_controller->updateSmsStatus.connect(sigc::mem_fun(this,&VoiceServiceInterface::getSmsForUiStage));
    dispatcher->dsp_controller->gucCrcFailed.connect(sigc::mem_fun(this,&VoiceServiceInterface::gucCrcFail));
    dispatcher->dsp_controller->updateGucGpsStatus.connect(sigc::mem_fun(this,&VoiceServiceInterface::gucCoordRec));
    dispatcher->dsp_controller->smsCounterChanged.connect(sigc::mem_fun(this,&VoiceServiceInterface::onSmsCounterChange));

    dispatcher->dsp_controller->TxCondCmdPackageTransmit.connect(sigc::mem_fun(this,&VoiceServiceInterface::TxCondCmdTransmit));
}

VoiceServiceInterface::~VoiceServiceInterface()
{

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
	switch (dispatcher->main_service->current_mode) {
	case MainServiceInterface::VoiceModeAuto: {
		if (current_channel_status != ChannelDisabled)
			return dispatcher->voice_channels_table.at(getCurrentChannelNumber()-1).frequency;
		break;
	}
	case MainServiceInterface::VoiceModeManual: {
		return dispatcher->voice_manual_frequency;
	}
	}
	return 0;
}

voice_emission_t VoiceServiceInterface::getCurrentChannelEmissionType() {
	switch (dispatcher->main_service->current_mode) {
	case MainServiceInterface::VoiceModeAuto: {
		if (current_channel_status != ChannelDisabled) {
			uint32_t frequency = dispatcher->voice_channels_table.at(getCurrentChannelNumber()-1).frequency;
			return dispatcher->getVoiceEmissionFromFrequency(frequency);
		}
		break;
	}
	case MainServiceInterface::VoiceModeManual: {
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

voice_channel_speed_t VoiceServiceInterface::getCurrentChannelSpeed() {
	switch (dispatcher->main_service->current_mode) {
	case MainServiceInterface::VoiceModeAuto: {
		if (current_channel_status != ChannelDisabled)
			return dispatcher->voice_channels_table.at(getCurrentChannelNumber()-1).speed;
		break;
	}
	case MainServiceInterface::VoiceModeManual: {
		return dispatcher->voice_manual_channel_speed;
	}
	}
	return voicespeedInvalid;
}

void VoiceServiceInterface::setCurrentChannelSpeed(voice_channel_speed_t speed) {
	dispatcher->data_storage_fs->setVoiceChannelSpeed(speed);
	dispatcher->voice_manual_channel_speed = speed;
	if (!((dispatcher->main_service->current_mode == MainServiceInterface::VoiceModeManual)
			&& (dispatcher->headset_controller->getStatus() == Headset::Controller::StatusSmartOk)))
		return;
	dispatcher->headset_controller->setSmartCurrentChannelSpeed(speed);
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

void VoiceServiceInterface::tuneFrequency(int frequency)
{
	dispatcher->data_storage_fs->setVoiceFrequency(frequency);
	dispatcher->voice_manual_frequency = frequency;
	if (dispatcher->main_service->current_mode != MainServiceInterface::VoiceModeManual)
		return;
    dispatcher->updateVoiceChannel(true);
}

void VoiceServiceInterface::tuneEmissionType(voice_emission_t type) {
	dispatcher->data_storage_fs->setVoiceEmissionType(type);
	dispatcher->voice_manual_emission_type = type;
	if (dispatcher->main_service->current_mode != MainServiceInterface::VoiceModeManual)
		return;
    dispatcher->updateVoiceChannel(false);
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

void VoiceServiceInterface::TurnAGCMode(uint8_t mode, int radio_path)
{
    dispatcher->dsp_controller->setAGCParameters(mode, radio_path);
}

void VoiceServiceInterface::TurnPSWFMode(uint8_t mode, int cmd, int r_adr, int retr)
{
    if ((r_adr == 0) && (cmd ==0))
    {
      dispatcher->dsp_controller->startPSWFReceiving();
    } else {
    	if (mode == 0)
    		dispatcher->dsp_controller->startPSWFTransmitting(false, r_adr, cmd,retr);
    	else
    		dispatcher->dsp_controller->startPSWFTransmitting(true, r_adr, cmd,retr);
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

void VoiceServiceInterface::updateChannel() {
	if (dispatcher->voice_channel != dispatcher->voice_channels_table.end()) {
		if ((dispatcher->main_service->current_mode == MainServiceInterface::VoiceModeAuto)
				|| ((dispatcher->main_service->current_mode == MainServiceInterface::VoiceModeManual)
						&& (dispatcher->headset_controller->getStatus() == Headset::Controller::StatusSmartOk)))
			setCurrentChannel(ChannelActive);
		else
			setCurrentChannel(ChannelDisabled);
	} else {
		setCurrentChannel(ChannelInvalid);
	}
}

void VoiceServiceInterface::fistPacketRecieve(int packet)
{
    firstPacket(packet);
}

void VoiceServiceInterface::saveFreq(int value)
{
    dispatcher->dsp_controller->setFreq(value);
}

void VoiceServiceInterface::responseGuc(int value)
{
    respGuc(value);
}

void VoiceServiceInterface::smsMessage(int value)
{
	smsMess(value);
}

void VoiceServiceInterface::gucCoordRec(){
	gucCoord();
}

void VoiceServiceInterface::goToVoice()
{
    dispatcher->dsp_controller->goToVoice();
}

uint8_t VoiceServiceInterface::getSmsCounter()
{
   return dispatcher->dsp_controller->getSmsCounter();

}

void VoiceServiceInterface::TxCondCmdTransmit(int value)
{
    command_tx30(value);
}
void VoiceServiceInterface::onSmsCounterChange(int param){
   smsCounterChanged(param);
}

bool VoiceServiceInterface::getIsGucCoord()
{
    return dispatcher->dsp_controller->getIsGucCoord();
}

} /* namespace Multiradio */
