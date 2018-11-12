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
#include "../multiradio.h"
#include "aleservice.h"
#include "../datastorage/fs.h"

#define DEBUGSHOWFREQ = false

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

	void initResetState();

    sigc::signal<void, Status/*new_status*/> statusChanged;
    sigc::signal<void, uint8_t/*subdevice_code*/, uint8_t/*error_code*/> dspHardwareFailed;
    sigc::signal<void, AleState/*new_state*/> aleStateChanged;
    sigc::signal<void, uint8_t/*new_value*/> aleVmProgressUpdated;
    sigc::signal<void> startCondReceiving;
    sigc::signal<void, uint8_t> virtualCounterChanged;
    sigc::signal<void, uint8_t> qwitCounterChanged;
    sigc::signal<void, bool> transmitAsk;
    sigc::signal<void, int> keyEmulate;

    void onKeyEmulate(int key);

	ChannelStatus getCurrentChannelStatus();
	int getCurrentChannelNumber();
    int getCurrentChannelFrequency();
    voice_emission_t getCurrentChannelEmissionType();
	voice_channel_t getCurrentChannelType();
	voice_channel_speed_t getCurrentChannelSpeed();
	void setCurrentChannelSpeed(voice_channel_speed_t speed);
	void tuneNextChannel();
	void tunePreviousChannel();
	void tuneChannel(uint8_t channel);
    void tuneFrequency(int frequency, bool isRecord = false);
    void tuneEmissionType(voice_emission_t type);
    void tuneSquelch(uint8_t value);
    void TuneAudioLevel(uint8_t volume_level);
    void TurnAGCMode(uint8_t mode, int radio_path);
    void TurnPSWFMode(uint8_t mode,int cmd,int r_adr,int retr);
    const char* ReturnSwfStatus();


    void resetDSPLogic();

    void setRnKey(int value);


    // ----- GUC -------
    void TurnGuc(int r_adr, int speed_tx, std::vector<int> command, bool isGps);
    void TurnGuc();
    uint8_t* getGucCommand();
    void saveFreq(int value);
    void messageGucQuit(int ans);
    void gucCrcFail();
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

    void stopGucQuit();

    // ----- GUC -------
    sigc::signal<void,int,bool> respGuc;
    sigc::signal<void,int> messageGucTxQuit;
    sigc::signal<void> gucCrcFailed;

    // ----- SMS -------
    sigc::signal<void,int> smsFailed;
    sigc::signal<void,int> smsMess;
    sigc::signal<void,int> getSmsStageUi;
    sigc::signal<void,int> smsCounterChanged;

    sigc::signal<void> dspStarted;
    sigc::signal<void, bool> stationModeIsCompleted;
	sigc::signal<void> currentChannelChanged;

    sigc::signal<void,int,uint8_t,bool> firstPacket;

    sigc::signal<void, float, float> waveInfoRecieved; //wave, power

    sigc::signal<void> rxModeSetting;
    sigc::signal<void> txModeSetting;


    sigc::signal<void> atuMalfunction;


    sigc::signal<void,int> smsFreq;



    sigc::signal<void> startRxQuitSignal;
    sigc::signal<void, uint32_t> settingAleFreq;

    sigc::signal<void, int> command_tx30;   // Передача УК  пакеты
    void TxCondCmdTransmit(int value);

    void goToVoice();

    std::string getVirtualDate();

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
    uint8_t playVoiceMessage(uint8_t fileNumber, DataStorage::FS::TransitionFileType transFileType, uint8_t num);
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

    void forwardDspHardwareFailure(uint8_t subdevice_code, uint8_t error_code);

    void emulatorKey(int key);
    sigc::signal<void, int> emulKey;

    void setSwrTimerState(bool state);
    void onStartCondReceiving();
    void onVirtualCounterChanged(uint8_t counter);
    void onQwitCounterChanged(uint8_t counter);
    void onTransmitAsk(bool on);

    uint16_t smsSender();

private:
    friend class Dispatcher;

    VoiceServiceInterface(Dispatcher *dispatcher);
    ~VoiceServiceInterface();

    void onSmsCounterChange(int param);

    void onSmsFreq(int param);

	void setCurrentChannel(ChannelStatus status);
	void updateChannel();

    void fistPacketRecieve(int packet, uint8_t address, bool rec);
    void responseGuc(int value, bool isTxAsk);
    void smsMessage(int value);
    void onDspStarted(){dspStarted();}

    void updateAleState(int state);
    void updateAleVmProgress(uint8_t progress);

	Dispatcher *dispatcher;
    ChannelStatus current_channel_status;

    Status current_status;
    VoiceMode current_mode;

    uint8_t param[6];
    DataStorage::FS* storageFs = 0;
    void onWaveInfoRecieved(float wave, float power);
    void onRxModeSetting();
    void onTxModeSetting();
    void onSettingAleFreq(uint32_t freq);
};

} /* namespace Multiradio */

#endif /* FIRMWARE_APP_MRD_VOICESERVICEINTERFACE_H_ */
