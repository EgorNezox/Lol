/**
 ******************************************************************************
 * @file    dspcontroller.h
 * @author  Artem Pisarenko, PMR dept. software team, ONIIP, PJSC
 * @date    22.12.2015
 *
 ******************************************************************************
 */

#ifndef FIRMWARE_APP_DSP_DSPCONTROLLER_H_
#define FIRMWARE_APP_DSP_DSPCONTROLLER_H_

#include "qmobject.h"
#include "../navigation/navigator.h"
#include <list>
#include <vector>
#include "packagemanager.h"


class QmTimer;
class QmIopin;

namespace Multiradio {


class DspTransport;
struct DspCommand;

class DspController : public QmObject
{
public:
    enum RadioMode {
        RadioModeOff = 0,
        RadioModeCarrierTx = 1,
        RadioModeUSB = 7,
        RadioModeFM = 9,
        RadioModeSazhenData = 11,
        RadioModePSWF = 20
    };
    enum RadioOperation {
        RadioOperationOff,
        RadioOperationRxMode,
        RadioOperationTxMode,
        RadioOperationCarrierTx
    };
    enum AudioMode {
        AudioModeOff = 0,
        AudioModeRadiopath = 1,
        AudioModeMicTest = 2,
        AudioModePlayLongSignal = 3,
        AudioModePlayShortSignal = 4
    };

    enum SmsStage
	{
    	StageNone = -1,
		StageTx_call = 0,
		StageTx_call_ack  = 1,
		StageTx_data = 2,
		StageTx_quit = 3,
		StageRx_call = 4,
		StageRx_call_ack = 5,
		StageRx_data = 6,
		StageRx_quit = 7
	};

    DspController(int uart_resource, int reset_iopin_resource, Navigation::Navigator *navigator, QmObject *parent);
    ~DspController();
    bool isReady();
    void startServicing();
    void setRadioParameters(RadioMode mode, uint32_t frequency);
    void setRadioOperation(RadioOperation operation);
    void setRadioSquelch(uint8_t value);
    void setAudioVolumeLevel(uint8_t volume_level);
    void setAGCParameters(uint8_t agc_mode,int RadioPath);

    void startPSWFReceiving(bool ack);
    void startPSWFTransmitting(bool ack, uint8_t r_adr, uint8_t cmd);

    void startSMSRecieving(SmsStage stage = StageRx_call);
    void startSMSTransmitting(uint8_t r_adr,uint8_t *message, SmsStage stage = StageTx_call);

    void parsingData();
    void *getContentPSWF();

    char* getSmsContent();



    sigc::signal<void> started;
    sigc::signal<void> setRadioCompleted;
    sigc::signal<void,int> firstPacket;
    sigc::signal<void> smsReceived;
    sigc::signal<void,int> smsFailed;
    sigc::signal<void> smsPacketMessage;

    float swf_res = 2; // надо изменить значение на нижнее предельное

    PackageManager *pack_manager;

    int decode_bit[255];

private:
    friend struct DspCommand;

    enum Module {
        RxRadiopath,
        TxRadiopath,
        Audiopath,
        PSWFReceiver,		//0x60
        PSWFTransmitter		//0x72
    };
    enum RxParameterCode {
        RxFrequency = 1,
        RxRadioMode = 2,
		RxSquelch = 4,
        AGCRX = 8
    };
    enum TxParameterCode {
        TxFrequency = 1,
        TxRadioMode = 2,
		TxPower = 4,
        AGCTX = 7
    };
    enum AudioParameterCode {
        AudioModeParameter = 0,
        AudioVolumeLevel = 2,
        AudioMicAmplify = 3
    };

    enum PSWF
    {
        PSWF_RX = 0,
        PSWF_TX = 1
    };
    enum PswfRxParameterCode {
    	PswfRxState = 0,
		PswfRxRAdr = 1,
		PswfRxFrequency = 2,
		PswfRxFreqSignal = 3, // TODO:
		PswfRxMode = 4
    };

    union ParameterValue {
        uint32_t frequency;
        uint8_t power;
        RadioMode radio_mode;
        AudioMode audio_mode;
        uint8_t squelch;
        uint8_t volume_level;
        uint8_t mic_amplify;
        uint8_t agc_mode;
        uint8_t pswf_indicator;
        uint8_t pswf_r_adr;
        uint8_t swf_mode;
    };

    struct PswfContent{
    	uint8_t indicator;
        uint8_t TYPE;
        uint32_t Frequency;
        uint8_t SNR;
        uint8_t R_ADR;
        uint8_t S_ADR;
        uint8_t COM_N;
        uint8_t L_CODE;
        uint8_t RN_KEY;
        uint8_t Conditional_Command;
    } ContentPSWF;


    struct SmsContent
    {
        uint8_t indicator;
        uint8_t TYPE;
        uint32_t Frequency;
        uint8_t SNR;
        uint8_t R_ADR;
        uint8_t S_ADR;
        uint8_t COM_N;
        uint8_t L_CODE;
        uint8_t RN_KEY;
        SmsStage stage;
        uint8_t message[259];
    } ContentSms;

    int *counterSms;


    void initResetState();
    void processStartup(uint16_t id, uint16_t major_version, uint16_t minor_version);
    void processStartupTimeout();
    bool startRadioOff();
    bool startRadioRxMode();
    bool startRadioTxMode();
    bool startRadioCarrierTx();
    void processRadioState();
    void syncNextRadioState();
    void processCommandTimeout();
    void processCommandResponse(bool success, Module module, int code, ParameterValue value);
    void syncPendingCommand();
    bool resyncPendingCommand();
    void sendCommand(Module module, int code, ParameterValue value);
    void sendPswf(Module module);
    void processReceivedFrame(uint8_t address, uint8_t *data, int data_len);

    int CalcShiftFreq(int RN_KEY, int SEC, int DAY, int HRS, int MIN);
    int prevSecond(int second);

    void RecievedPswf();
    int getFrequencyPswf();

    void getSwr();
    void transmitPswf();
    void changePswfRxFrequency();
    void syncPulseDetected();
    void getDataTime();
    void transmitSMS();
    void sendSms(Module module);
    void recSms();

    void changeSmsRxFrequency();

    void startSMSCmdTransmitting(SmsStage stage);

    void generateSmsReceived();

    int check_rx_call();

    int calc_asc_code(int ack);

    Navigation::Navigator *navigator;

    bool is_ready;
    QmIopin *reset_iopin;
    DspTransport *transport;
    QmTimer *startup_timer, *command_timer;
    QmTimer *quit_timer;
    enum {
        radiostateSync,
        radiostateCmdModeOffRx,
        radiostateCmdModeOffTx,
		radiostateCmdTxPower,
        radiostateCmdRxFreq,
        radiostateCmdTxFreq,
        radiostateCmdRxOff,
        radiostateCmdTxOff,
        radiostateCmdRxMode,
        radiostateCmdTxMode,
        radiostateCmdCarrierTx,
		radiostatePswfTxPrepare,
		radiostatePswfTx,
        radiostatePswfRxPrepare,
        radiostatePswfRx,
        radiostateSmsTx,
        radiostateSmsRx,
        radiostateSmsRxPrepare,
        radiostateSmsTxPrepare,
		radiostateSmsTxRxSwitch,
		radiostateSmsRxTxSwitch
    } radio_state;
    RadioMode current_radio_mode;
    RadioOperation  current_radio_operation;
    uint32_t current_radio_frequency;
    DspCommand *pending_command;

    std::list<DspCommand> *cmd_queue;

    uint32_t fwd_wave;
    uint32_t ref_wave;

    int command_tx30;
    int command_rx30;

    int pswfRxStateSync;
    int pswfTxStateSync;

    int smsRxStateSync;
    int smsTxStateSync;

    int success_pswf;
    bool pswf_first_packet_received;
    bool pswf_ack;

    char rec_sms[37];

    int date_time[4];

    char private_lcode;

    std::vector< std::vector<char> > recievedPswfBuffer;
    std::vector< std::vector<uint8_t> > recievedSmsBuffer;

    std::vector<int> syncro_recieve;
    std::vector<int> tx_call_ask_vector;
    std::vector<int> quit_vector;


    int rs_data_clear[255];

    char sms_content[100];
};



} /* namespace Multiradio */

#endif /* FIRMWARE_APP_DSP_DSPCONTROLLER_H_ */
