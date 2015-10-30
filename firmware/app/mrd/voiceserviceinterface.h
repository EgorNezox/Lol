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

#include "sigc++/signal.h"
#include "multiradio.h"

namespace Multiradio {

class Dispatcher;

class VoiceServiceInterface {
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

	sigc::signal<void> currentChannelChanged;

private:
	friend class Dispatcher;

	VoiceServiceInterface();
	~VoiceServiceInterface();
};

} /* namespace Multiradio */

#endif /* FIRMWARE_APP_MRD_VOICESERVICEINTERFACE_H_ */
