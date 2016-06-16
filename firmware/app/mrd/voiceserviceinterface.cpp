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
}

VoiceServiceInterface::~VoiceServiceInterface()
{
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

void VoiceServiceInterface::TurnPSWFMode(uint8_t mode, int R_ADR, int COM_N, int retr)
{
    if ((R_ADR == 0) && (COM_N ==0)) {
    	if (mode == 0)
    		dispatcher->dsp_controller->startPSWFReceiving(false);
    	else
    		dispatcher->dsp_controller->startPSWFReceiving(true);
    } else {
    	if (mode == 0)
    		dispatcher->dsp_controller->startPSWFTransmitting(false, R_ADR, COM_N,retr);
    	else
    		dispatcher->dsp_controller->startPSWFTransmitting(true, R_ADR, COM_N,retr);
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

void VoiceServiceInterface::TurnSMSMode(int r_adr, char *message)
{
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


void VoiceServiceInterface::TurnGuc(int r_adr, int speed_tx, std::vector<int> command)
{
    dispatcher->dsp_controller->startGucTransmitting(r_adr,speed_tx,command);
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

void VoiceServiceInterface::responseGuc()
{
    respGuc();
}

void VoiceServiceInterface::smsMessage()
{
	smsMess();
}

} /* namespace Multiradio */
