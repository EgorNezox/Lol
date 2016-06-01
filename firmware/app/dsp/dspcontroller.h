/**
 ******************************************************************************
 * @file    dspcontroller.h
 * @author  Artem Pisarenko, PMR dept. software team, ONIIP, PJSC
 * @author  неизвестные
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

    enum GucStage
    {
        GucNone = 0,
        GucTx = 1,
        GucRx = 2,
        GucTxQuit = 3,
        GucRxQuit = 4
    };

    enum ModemTimeSyncMode {
    	modemtimesyncManual = 0,
		modemtimesyncGPS = 1
    };
    enum ModemPhase {
    	modemphaseWaitingCall = 0,
		modemphaseALE = 1,
		modemphaseLinkEstablished = 2
    };
    enum ModemRole {
    	modemroleResponder = 0,
		modemroleCaller = 1
    };
    enum ModemBandwidth {
    	modembw3100Hz = 1,
		modembw20kHz = 2,
		modembwAll = 3
    };
    enum ModemPacketType {
    	modempacket_Call = 1,
		modempacket_HshakeReceiv = 2,
		modempacket_HshakeTrans = 3,
		modempacket_HshakeTransMode = 4,
		modempacket_RespCallQual = 5,
		modempacket_LinkRelease = 6,
		modempacket_msgHead = 21,
		modempacket_packHead = 22,
		modempacket_RespPackQual = 23
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

    void startGucTransmitting(int r_adr, int speed_tx, std::vector<int> command);
    void startGucTransmitting();
    void startGucRecieving();
    void checkGucQuit();

    void *getContentPSWF();

    char* getSmsContent();

    void processSyncPulse();

uint8_t* get_guc_vector();
    void enableModemReceiver();
    void disableModemReceiver();
    void setModemReceiverBandwidth(ModemBandwidth value);
    void setModemReceiverTimeSyncMode(ModemTimeSyncMode value);
    void setModemReceiverPhase(ModemPhase value);
    void setModemReceiverRole(ModemRole value);
    void enableModemTransmitter();
    void disableModemTransmitter();
    void sendModemPacket(ModemPacketType type, ModemBandwidth bandwidth, const uint8_t *data, int data_len);
    void sendModemPacket_packHead(ModemBandwidth bandwidth, uint8_t param_signForm, uint8_t param_packCode, const uint8_t *data, int data_len);

    sigc::signal<void> started;
    sigc::signal<void> setRadioCompleted;
    sigc::signal<void,int> firstPacket;
    sigc::signal<void> smsReceived;
    sigc::signal<void,int> smsFailed;
    sigc::signal<void> smsPacketMessage;
    sigc::signal<void, ModemPacketType/*type*/> transmittedModemPacket;
    sigc::signal<void> failedTxModemPacket;
    sigc::signal<void, ModemPacketType/*type*/, uint8_t/*snr*/, ModemBandwidth/*bandwidth*/, uint8_t*/*data*/, int/*data_len*/> receivedModemPacket;
    sigc::signal<void, ModemPacketType/*type*/, uint8_t/*snr*/, ModemBandwidth/*bandwidth*/, uint8_t*/*data*/, int/*data_len*/> startedRxModemPacket;
    sigc::signal<void, uint8_t/*snr*/, ModemBandwidth/*bandwidth*/, uint8_t/*param_signForm*/, uint8_t/*param_packCode*/, uint8_t*/*data*/, int/*data_len*/> startedRxModemPacket_packHead;
    sigc::signal<void, ModemPacketType/*type*/> failedRxModemPacket;
sigc::signal<void> recievedGucResp;

    float swf_res = 2; // надо изменить значение на нижнее предельное

    PackageManager *pack_manager;

    int decode_bit[255];

private:
    friend struct DspCommand;

    enum  Module {
        RxRadiopath,
        TxRadiopath,
        Audiopath,
        PSWFReceiver,		//0x60
        PSWFTransmitter,    //0x72
        RadioLineNotPswf,   // 0x68
        GucPath,            // 0x7A
		ModemReceiver
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

    enum ModemRxParameterCode {
    	ModemRxState = 0,
		ModemRxBandwidth = 1,
		ModemRxTimeSyncMode = 2,
		ModemRxPhase = 3,
		ModemRxRole = 4
    };

    enum ModemState {
    	ModemRxOff = 0,
		ModemRxDetectingStart = 3,
		ModemRxReceiving = 5
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
        uint8_t guc_mode;
 		uint8_t guc_adr;
        ModemState modem_rx_state;
        ModemBandwidth modem_rx_bandwidth;
        ModemTimeSyncMode modem_rx_time_sync_mode;
        ModemPhase modem_rx_phase;
        ModemRole modem_rx_role;
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

    struct GucContent
    {
        uint8_t indicator;
        uint8_t type;
        uint8_t chip_time;
        uint8_t WIDTH_SIGNAL;
        uint8_t S_ADR;
        uint8_t R_ADR;
        uint8_t NUM_com;
        uint8_t ckk;
        uint8_t uin;
        uint8_t Coord;
        uint8_t stage;
        uint8_t command[100];

    } ContentGuc;


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
    void sendCommand(Module module, int code, ParameterValue value,bool state = 0);
    void sendCommandEasy(Module module, int code, ParameterValue value);
    void sendPswf(Module module);
    void sendGuc();
    void recGuc();
    void processReceivedFrame(uint8_t address, uint8_t *data, int data_len);


    int CalcShiftFreq(int RN_KEY, int SEC, int DAY, int HRS, int MIN);
    int prevSecond(int second);

    void RecievedPswf();
    int getFrequencyPswf();

    void getSwr();
    void transmitPswf();
    void addSeconds(int *date_time);
    void changePswfRxFrequency();
    void syncPulseDetected();
    void getDataTime();
    void transmitSMS();
    void sendSms(Module module);
    void recSms();
    void sendGucQuit();

    void changeSmsRxFrequency();

    void startSMSCmdTransmitting(SmsStage stage);

    void generateSmsReceived();

    int check_rx_call();

    uint8_t calc_ack_code(uint8_t ack);

    Navigation::Navigator *navigator;

    bool is_ready;
    QmIopin *reset_iopin;
    DspTransport *transport;
    QmTimer *startup_timer, *command_timer;
    QmTimer *quit_timer;
    QmTimer *sync_pulse_delay_timer; //delay is needed for Navigator NMEA processing after sync pulse
    QmTimer *guc_timer;
    QmTimer *guc_rx_quit_timer;

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
        radiostateSmsRxTxSwitch,
        radiostateGucTxPrepare,
        radiostateGucRxPrepare,
        radiostateGucSwitch,
        radiostateGucTx,
        radiostateGucRx
    } radio_state;
    RadioMode current_radio_mode;
    RadioOperation  current_radio_operation;
    uint32_t current_radio_frequency;
    DspCommand *pending_command;

    std::list<DspCommand> *cmd_queue;

    int fwd_wave;
    int ref_wave;

    int command_tx30;
    int command_rx30;

    int pswfRxStateSync;
    int pswfTxStateSync;

    int smsRxStateSync;
    int smsTxStateSync;

    int gucRxStateSync;
    int gucTxStateSync;

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
    std::vector<std::vector<uint8_t>> guc_vector;


    int rs_data_clear[255];

    uint8_t guc_text[50];
    uint8_t rec_uin_guc;
    uint8_t rec_s_adr;
    int guc_tx_num;

    char sms_content[100];
    uint8_t ack;
    int ok_quit = 0;
    bool guc_trans;
bool modem_rx_on, modem_tx_on;
    int clear_que;
    int trans_guc;

    bool sms_call_received;
};



} /* namespace Multiradio */

#endif /* FIRMWARE_APP_DSP_DSPCONTROLLER_H_ */
