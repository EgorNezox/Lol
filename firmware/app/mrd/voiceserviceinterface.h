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
    int getCurrentChannelFrequency();
    voice_emission_t getCurrentChannelEmissionType();
	voice_channel_t getCurrentChannelType();
	void tuneNextChannel();
	void tunePreviousChannel();
    void tuneFrequency(int frequency);
    void tuneEmissionType(voice_emission_t type);
    void tuneSquelch(uint8_t value);
    void TuneAudioLevel(uint8_t volume_level);
    void TurnAGCMode(uint8_t mode, int radio_path);
    void TurnPSWFMode(uint8_t mode,int cmd,int r_adr,int retr);
    const char* ReturnSwfStatus();

    void TurnSMSMode(int r_adr, char *message, uint8_t retr);
    void TurnSMSMode();
    void SmsFailStage(int stage);
    void saveFreq(int value);

    void setRnKey(int value);

    void TurnGuc(int r_adr, int speed_tx, std::vector<int> command);
    void TurnGuc();

    char* getSmsContent();
    uint8_t* getGucCommand();

    void defaultSMSTrans();
    void rerror();
    void messageGucQuit();

	sigc::signal<void> currentChannelChanged;
    sigc::signal<void> PswfRead;
    sigc::signal<void,int> firstPacket;
    sigc::signal<void,int> smsFailed;
    sigc::signal<void> smsMess;
    sigc::signal<void> respGuc;
    sigc::signal<void> errorAsu;
    sigc::signal<void> messageGucTxQuit;

private:
	friend class Dispatcher;

	VoiceServiceInterface(Dispatcher *dispatcher);
	~VoiceServiceInterface();
	void setCurrentChannel(ChannelStatus status);


    void fistPacketRecieve(int packet);
    void responseGuc();
    void smsMessage();

	Dispatcher *dispatcher;
    ChannelStatus current_channel_status;
};



} /* namespace Multiradio */

#endif /* FIRMWARE_APP_MRD_VOICESERVICEINTERFACE_H_ */
