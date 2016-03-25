/**
 ******************************************************************************
 * @file    voiceserviceinterface.h
 * @author  Artem Pisarenko, PMR dept. software team, ONIIP, PJSC
 * @date    30.10.2015
 *
 ******************************************************************************
 */

#ifndef FIRMWARE_APP_MRD_VOICESERVICEINTERFACE_H_
#define FIRMWARE_APP_MRD_VOICESERVICEINTERFACE_H_

#include "qmobject.h"
#include "multiradio.h"

namespace Multiradio {

class Dispatcher;

class VoiceServiceInterface : QmObject
{
public:
	enum ChannelStatus {
		ChannelDisabled,
		ChannelActive,
		ChannelInvalid
	};

	ChannelStatus getCurrentChannelStatus();
	int getCurrentChannelNumber();
	voice_channel_t getCurrentChannelType();
	void tuneNextChannel();
	void tunePreviousChannel();
    void TuneFrequency(int Frequency);

	sigc::signal<void> currentChannelChanged;

private:
	friend class Dispatcher;

	VoiceServiceInterface(Dispatcher *dispatcher);
	~VoiceServiceInterface();
	void setCurrentChannel(ChannelStatus status);

	Dispatcher *dispatcher;
	ChannelStatus current_channel_status;
};

} /* namespace Multiradio */

#endif /* FIRMWARE_APP_MRD_VOICESERVICEINTERFACE_H_ */
