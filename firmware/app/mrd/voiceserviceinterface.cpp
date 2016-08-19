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
#include "dispatcher.h"
#include <math.h>


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
    return (dispatcher->voice_channel - dispatcher->voice_channels_table.begin() + 1);
}

int VoiceServiceInterface::getCurrentChannelFrequency()
{
	if (current_channel_status == ChannelDisabled)
		return 0;
    return dispatcher->voice_channels_table.at(getCurrentChannelNumber()-1).frequency;
}

voice_channel_t VoiceServiceInterface::getCurrentChannelType() {
	if (current_channel_status == ChannelDisabled)
		return channelInvalid;
	return (*(dispatcher->voice_channel)).type;
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
	dispatcher->updateVoiceChannel();
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
    dispatcher->updateVoiceChannel();
}

void VoiceServiceInterface::TuneFrequency(int Frequency)
{

    if (Frequency >= 30000000)
        dispatcher->dsp_controller->setRadioParameters(DspController::RadioModeFM,Frequency);
    else
        dispatcher->dsp_controller->setRadioParameters(DspController::RadioModeUSB,Frequency);
    dispatcher->dsp_controller->setRadioOperation(DspController::RadioOperationRxMode);
}

void VoiceServiceInterface::tuneSquelch(uint8_t value) {
	if (value != 0 && (value < 6 || value > 24)) {
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
    if ((r_adr == 0) && (cmd ==0)) {
    	if (mode == 0)
    		dispatcher->dsp_controller->startPSWFReceiving(false);
    	else
    		dispatcher->dsp_controller->startPSWFReceiving(true);
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
   dispatcher->dsp_controller->startSMSTransmitting(r_adr,(uint8_t*)message);
}

void VoiceServiceInterface::TurnSMSMode()
{
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

void VoiceServiceInterface::smsMessage()
{
	smsMess();
}

void VoiceServiceInterface::gucCoordRec(){
	gucCoord();
}

} /* namespace Multiradio */
