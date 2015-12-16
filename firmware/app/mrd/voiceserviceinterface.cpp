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

namespace Multiradio {

VoiceServiceInterface::VoiceServiceInterface() {
	//...
}

VoiceServiceInterface::~VoiceServiceInterface() {
	//...
}

VoiceServiceInterface::ChannelStatus VoiceServiceInterface::getCurrentChannelStatus() {
	//...
	return ChannelDisabled;
}

int VoiceServiceInterface::getCurrentChannelNumber() {
	//...
	return -1;
}

voice_channel_t VoiceServiceInterface::getCurrentChannelType() {
	//...
	return channelInvalid;
}

void VoiceServiceInterface::tuneNextChannel() {
	//...
}

void VoiceServiceInterface::tunePreviousChannel() {
	//...
}

} /* namespace Multiradio */
