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
#include "../ui/texts.h"

namespace Multiradio {

class Dispatcher;

class VoiceServiceInterface : public QmObject
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
    void tuneSquelch(uint8_t value);
    void TuneAudioLevel(uint8_t volume_level);
    void TurnAGCMode(uint8_t mode, int radio_path);
    void TurnPSWFMode(uint8_t mode,int R_ADR,int COM_N);
    int *ReturnDataPSWF();
    const char* ReturnSwfStatus();

    void TurnSMSMode(int r_adr,char *message);
    void TurnSMSMode();

	sigc::signal<void> currentChannelChanged;
    sigc::signal<void> PswfRead;
    sigc::signal<void,int> firstPacket;

private:
	friend class Dispatcher;

	VoiceServiceInterface(Dispatcher *dispatcher);
	~VoiceServiceInterface();
	void setCurrentChannel(ChannelStatus status);

    void fistPacketRecieve(int packet);

	Dispatcher *dispatcher;
	ChannelStatus current_channel_status;
};

} /* namespace Multiradio */

#endif /* FIRMWARE_APP_MRD_VOICESERVICEINTERFACE_H_ */
