/**
 ******************************************************************************
 * @file    voiceserviceinterface.cpp
 * @author  Artem Pisarenko, PMR dept. software team, ONIIP, PJSC
 * @date    30.10.2015
 *
 ******************************************************************************
 */

#include "qm.h"

#include "voiceserviceinterface.h"
#include "dispatcher.h"


namespace Multiradio {

VoiceServiceInterface::VoiceServiceInterface(Dispatcher *dispatcher) :
	QmObject(dispatcher),
	dispatcher(dispatcher),
	current_channel_status(ChannelDisabled)
{
}

VoiceServiceInterface::~VoiceServiceInterface()
{
}

VoiceServiceInterface::ChannelStatus VoiceServiceInterface::getCurrentChannelStatus() {
	return current_channel_status;
}

int VoiceServiceInterface::getCurrentChannelNumber() {
	return (dispatcher->voice_channel - dispatcher->voice_channels_table.begin() + 1);
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
    dispatcher->dsp_controller->setRadioParameters(DspController::RadioMode::RadioModeFM,Frequency);
    dispatcher->dsp_controller->setRadioOperation(DspController::RadioOperation::RadioOperationRxMode);
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

void VoiceServiceInterface::TurnAGCMode(uint8_t mode)
{
    dispatcher->dsp_controller->setAGCParameters(mode,0);
}

void VoiceServiceInterface::setCurrentChannel(ChannelStatus status) {
	current_channel_status = status;
	currentChannelChanged();
}

} /* namespace Multiradio */
