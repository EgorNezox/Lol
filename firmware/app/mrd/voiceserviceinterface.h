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

#include "../ui/texts.h"
#include <string>

#include <math.h>
#include <string.h>
#include <stdio.h>

#include "qm.h"
#include "qmthread.h"
#include "qmdebug.h"
#include "qmcrc.h"
#include "qmendian.h"

#include "voiceserviceinterface.h"
#include "dispatcher.h"
#include "multiradio.h"
#include "aleservice.h"
#include "../datastorage/fs.h"

namespace Multiradio {

class Dispatcher;

//typedef AleService::AleState AleState;

class VoiceServiceInterface : public QmObject
{
public:

    enum Status {
        StatusNotReady,
        StatusIdle,
        StatusVoiceRx,
        StatusVoiceTx,
        StatusTuningTx
    };
    enum VoiceMode {
        VoiceModeAuto,
        VoiceModeManual
    };

	enum ChannelStatus {
		ChannelDisabled,
		ChannelActive,
		ChannelInvalid
	};

    sigc::signal<void, Status/*new_status*/> statusChanged;
    sigc::signal<void, uint8_t/*subdevice_code*/, uint8_t/*error_code*/> dspHardwareFailed;
    sigc::signal<void, AleState/*new_state*/> aleStateChanged;
    sigc::signal<void, uint8_t/*new_value*/> aleVmProgressUpdated;

	ChannelStatus getCurrentChannelStatus();
	int getCurrentChannelNumber();
    int getCurrentChannelFrequency();
    voice_emission_t getCurrentChannelEmissionType();
	voice_channel_t getCurrentChannelType();
	voice_channel_speed_t getCurrentChannelSpeed();
	void setCurrentChannelSpeed(voice_channel_speed_t speed);
	void tuneNextChannel();
	void tunePreviousChannel();
    void tuneFrequency(int frequency, bool isRecord = false);
    void tuneEmissionType(voice_emission_t type);
    void tuneSquelch(uint8_t value);
    void TuneAudioLevel(uint8_t volume_level);
    void TurnAGCMode(uint8_t mode, int radio_path);
    void TurnPSWFMode(uint8_t mode,int cmd,int r_adr,int retr);
    const char* ReturnSwfStatus();




    void setRnKey(int value);


    // ----- GUC -------
    void TurnGuc(int r_adr, int speed_tx, std::vector<int> command, bool isGps);
    void TurnGuc();
    uint8_t* getGucCommand();
    void saveFreq(int value);
    void messageGucQuit(int ans);
    void gucCrcFail();
    void gucCoordRec();
    uint8_t* requestGucCoord();
    bool getIsGucCoord();

    // ----- SMS -------
    void TurnSMSMode(int r_adr, char *message, uint8_t retr);
    void TurnSMSMode();
    void SmsFailStage(int stage);
    char* getSmsContent();
    void defaultSMSTrans();
    void getSmsForUiStage(int value);
    uint8_t getSmsCounter();



    void turnVirtualPswfTx();
    void turnVirtualPswfRx();

    void setVirtualMode(bool param);
    bool getVirtualMode();


    // ----- GUC -------
    sigc::signal<void,int> respGuc;
    sigc::signal<void,int> messageGucTxQuit;
    sigc::signal<void> gucCrcFailed;
    sigc::signal<void> gucCoord;

    // ----- SMS -------
    sigc::signal<void,int> smsFailed;
    sigc::signal<void,int> smsMess;
    sigc::signal<void,int> getSmsStageUi;
    sigc::signal<void,int> smsCounterChanged;

    sigc::signal<void> dspStarted;
    sigc::signal<void, bool> stationModeIsCompleted;
	sigc::signal<void> currentChannelChanged;
    sigc::signal<void> PswfRead;
    sigc::signal<void,int,bool> firstPacket;


    sigc::signal<void> atuMalfunction;



    sigc::signal<void> startRxQuitSignal;

    sigc::signal<void, int> command_tx30;   // Передача УК  пакеты
    void TxCondCmdTransmit(int value);

    void goToVoice();


    void setVirtualDate(std::string s);
    void setVirtualTime(std::string s);

    void startRxQuit();

    void setVoiceMode(VoiceMode mode);
    VoiceMode getVoiceMode();


    void onStationModeIsCompleted(bool isGoToVoice){stationModeIsCompleted(isGoToVoice);}

    uint8_t* getVirtualTime();
    void playSoundSignal(uint8_t mode, uint8_t speakerVolume, uint8_t gain, uint8_t soundNumber, uint8_t duration, uint8_t micLevel);
    void sendBatteryVoltage(int voltage);
    void sendHeadsetType(uint8_t type);
    uint8_t playVoiceMessage(uint8_t fileNumber, DataStorage::FS::TransitionFileType transFileType);
    void setFS(DataStorage::FS* fs);
    void startAleRx();
    void startAleTx(uint8_t address, voice_message_t message);
    void stopAle();
    int getAleState();//AleState getAleState();
    uint8_t getAleVmProgress();
    void initAle(ale_call_freqs_t call_freqs, ale_call_freqs_t work_freqs, int8_t own_adress);
    uint8_t getAleRxAddress();
    Multiradio::voice_message_t getAleRxVmMessage();
    void setStatus(Status value);
    uint8_t getStationAddress();
    VoiceServiceInterface::Status getStatus();
private:
    friend class Dispatcher;

    VoiceServiceInterface(Dispatcher *dispatcher);
    ~VoiceServiceInterface();

    void onSmsCounterChange(int param);

	void setCurrentChannel(ChannelStatus status);
	void updateChannel();

    void fistPacketRecieve(int packet, bool rec);
    void responseGuc(int value);
    void smsMessage(int value);
    void onDspStarted(){dspStarted();}

	Dispatcher *dispatcher;
    ChannelStatus current_channel_status;

    Status current_status;
    VoiceMode current_mode;

    uint8_t param[6];
    DataStorage::FS* storageFs = 0;
};

} /* namespace Multiradio */

#endif /* FIRMWARE_APP_MRD_VOICESERVICEINTERFACE_H_ */
