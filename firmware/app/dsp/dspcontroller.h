/**
 ******************************************************************************
 * @file    dspcontroller.h
 * @author  Artem Pisarenko, PMR dept. software team, ONIIP, PJSC
 * @author  �����������
 * @date    22.12.2015
 *
 ******************************************************************************
 */



#ifndef FIRMWARE_APP_DSP_DSPCONTROLLER_H_
#define FIRMWARE_APP_DSP_DSPCONTROLLER_H_

#include "qmobject.h"
#include "../navigation/navigator.h"
#include "../datastorage/fs.h"
#include <list>
#include <vector>
#include "packagemanager.h"
#include "qmrtc.h"


class QmTimer;
class QmIopin;

namespace Multiradio {


class DspTransport;
struct DspCommand;


/**
 @file
 @brief ����� ������������ ��� ��������� �������� ������ ����� DSP � HOST
 @version 0.5
 @date 29.07.2015
 ��������� ��������� ��������� ������ �� ��������� ������, ��������� ������, ��������� � ��������� ������ �� DSP, �������� ������� �� ���������
 ��������� ������� ��� �������� � ���������� ������� �������
*/

class DspController : public QmObject
{
public:
    enum RadioMode {
        RadioModeOff = 0,
        RadioModeCarrierTx = 1,
        RadioModeUSB = 7,
        RadioModeFM = 9,
        RadioModeSazhenData = 11,
        RadioModePSWF = 20,
		RadioModeVirtualPpps = 5,
		RadioModeVirtualRvv = 4
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

    DspController(int uart_resource, int reset_iopin_resource, Navigation::Navigator *navigator, DataStorage::FS *data_storage_fs, QmObject *parent);
    ~DspController();
    bool isReady();
    void startServicing();
    void setRadioParameters(RadioMode mode, uint32_t frequency);
    void setRadioOperation(RadioOperation operation);
    void setRadioSquelch(uint8_t value);
    void setAudioVolumeLevel(uint8_t volume_level);
    void setAudioMicLevel(uint8_t value);
    void setAGCParameters(uint8_t agc_mode,int RadioPath);

    void startPSWFReceiving();   // ��������� �����  �������� ������(��)
    void startPSWFTransmitting(bool ack,  uint8_t r_adr, uint8_t cmd,int retr); // ��������� �������� ����� ��

    void startSMSRecieving(SmsStage stage = StageRx_call);  // ��������� ���� ���
    void startSMSTransmitting(uint8_t r_adr,uint8_t *message, SmsStage stage = StageTx_call); // ��������� �������� ���

    void startGucTransmitting(int r_adr, int speed_tx, std::vector<int> command,bool isGps); // ��������� �������� �����
    void startGucTransmitting();        // ������ �������� ����� �� - ������������� �����
    void startGucRecieving();           // ������ ������ ����� ��
    void GucSwichRxTxAndViewData();     // ������� �� Tx � Rx, ��� �� Rx � Tx � ������� ��

    void *getContentPSWF();             // ������� ��������� ��������� ��? �
    char *getSmsContent();              // ������� ��������� ��������� ���
    void setRnKey(int keyValue);        // ��������� ���������� �������� ����� ���������
    void resetContentStructState();     // ����� ���������� ��������� ��� ��? �-�������
    void processSyncPulse();            // �������, ���������� �� ��������� ����� -  ������ ������� ������� ��� ��? �-�������
    uint8_t* get_guc_vector();          // ������� ������� � ��������� ��

	void tuneModemFrequency(uint32_t value);
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

    void defaultSMSTransmit();              // ����� ������ ���������
    void setSmsRetranslation(uint8_t retr); // ���������� ��������� ������������
    uint8_t getSmsRetranslation();          // ������� ��������� ������� ������������
    void setFreq(int value);                // ������� ��������� ������� �� ������ ui/service

    uint8_t* getGucCoord();                 // ��������� ��������� ��� ����������� �� ������ ������������ � ������ ����� ��

    bool getVirtualMode();
    void setVirtualMode(bool param);

    sigc::signal<void> started;
    sigc::signal<void> setRadioCompleted;
    sigc::signal<void,int,bool> firstPacket;     // ������� ������ ����� � ��
    sigc::signal<void> smsReceived;         // ������� ���
    sigc::signal<void,int> smsFailed;       // ������ ������ ���
    sigc::signal<void,int> smsPacketMessage;    // ����� ��������� �� �����
    sigc::signal<void, ModemPacketType/*type*/> transmittedModemPacket;
    sigc::signal<void> failedTxModemPacket;
    sigc::signal<void, ModemPacketType/*type*/, uint8_t/*snr*/, uint8_t/*errors*/, ModemBandwidth/*bandwidth*/, uint8_t*/*data*/, int/*data_len*/> receivedModemPacket;
    sigc::signal<void, ModemPacketType/*type*/, uint8_t/*snr*/, uint8_t/*errors*/, ModemBandwidth/*bandwidth*/, uint8_t*/*data*/, int/*data_len*/> startedRxModemPacket;
    sigc::signal<void, uint8_t/*snr*/, uint8_t/*errors*/, ModemBandwidth/*bandwidth*/, uint8_t/*param_signForm*/, uint8_t/*param_packCode*/, uint8_t*/*data*/, int/*data_len*/> startedRxModemPacket_packHead;
    sigc::signal<void, ModemPacketType/*type*/> failedRxModemPacket;
    sigc::signal<void,int> recievedGucResp;    // ����� �� ������ ��
    sigc::signal<void,int> recievedGucQuitForTransm; // ����� ��������� ����� ��

    sigc::signal<void> updateGucGpsStatus;    float swf_res = 2; // ���� �������� �������� �� ������ ����������
    sigc::signal<void> gucCrcFailed;                 // ������ crc-�����
    sigc::signal<void, uint8_t/*subdevice_code*/, uint8_t/*error_code*/> hardwareFailed;
    sigc::signal<void,int> smsCounterChanged;
    sigc::signal<void> startRxQuit;

    sigc::signal<void, int/*command_tx30*/> TxCondCmdPackageTransmit;   // �������� ��  ������

    PackageManager *pack_manager;
    bool retranslation_active = false;

    void goToVoice();

    void resetSmsState();

    void prevTime();

    enum SmsRole
	{
    	SmsRoleIdle = 2,
    	SmsRoleTx = 0,
		SmsRoleRx = 1
	};

    enum CondComRole
	{
    	CondComTx = 0,
		CondComRx = 1
	};


    uint8_t CondComLogicRole;
    uint8_t SmsLogicRole;
    uint8_t sms_counter;

    void setAdr();

    void startVirtualPpsModeTx();
    void startVirtualPpsModeRx();

    void setVirtualDate(uint8_t *param);
    void setVirtualTime(uint8_t *param);

    uint8_t* getVirtualTime();

private:
    friend struct DspCommand;

    bool checkForTxAnswer();

    int sms_data_count = 0;

    // ������������ ��� ������� �������� � DSP
    enum  Module {
        RxRadiopath,
        TxRadiopath,
        Audiopath,
        PSWFReceiver,		//0x60
        PSWFTransmitter,    //0x72
        RadioLineNotPswf,   // 0x68
        GucPath,            // 0x7A
		ModemReceiver,
		VirtualPps         // 0x64
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
 		uint8_t param;
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
        int RN_KEY;
        uint8_t Conditional_Command;
        uint8_t RET_end_adr;
    } ContentPSWF;


    struct SmsContent
    {
        uint8_t indicator;
        uint8_t TYPE;
        uint32_t Frequency;
        uint8_t SNR;
        uint8_t R_ADR;
        uint8_t S_ADR;
        uint8_t CYC_N;
        uint8_t L_CODE;
        int RN_KEY;
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
        uint8_t command[120];

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
    void sendCommand(Module module, int code, ParameterValue value,bool state = 0);                 // ������� �������� ������� ��� dsp � �������
    void sendCommandEasy(Module module, int code, ParameterValue value);                            // ������� �������� ������� ��� dsp ��� ������
    void sendGuc();                                                                                 // ������� �������� ����� ��
    void processReceivedFrame(uint8_t address, uint8_t *data, int data_len);                        // ������� ������ ������ �� DSP


    int CalcShiftFreq(int RN_KEY, int SEC, int DAY, int HRS, int MIN);                              // ������� �������� ������� �������� ��� ��
    int CalcSmsTransmitFreq(int RN_KEY, int DAY, int HRS, int MIN, int SEC);                        // ������� �������� ������� �������� ��� ���
    int CalcSmsTransmitRxRoleFreq(int RN_KEY, int SEC, int DAY, int HRS, int MIN);
    int CalcSmsTransmitTxRoleFreq(int RN_KEY, int SEC, int DAY, int HRS, int MIN);
    int prevSecond(int second);                                                                     // ������� ��������� ���������� �������

    void recPswf(uint8_t data,uint8_t code, uint8_t indicator);                                                       // ������� �������� ��� lcode
    int getFrequencyPswf();                                                                         // ������� �������� ��������� ������� � ��
    int getFrequencySms();                                                                          // ������� �������� ��������� ������� � ���

    void getSwr();                                                                                  // ������� ��������� ��������������
    void sendPswf();                                                                            // ������� �������� ��
    void addSeconds(int *date_time);                                                                // ������� ���������� ������� � ������� �������
    void addSeconds(QmRtc::Time *t);
    void changePswfFrequency();                                                                   // ������� ������ ��
    void syncPulseDetected();                                                                       // ������� ��������� ����� �� ��������� �����
    void getDataTime();                                                                             // ������� ��������� �������
    void sendSms(Module module);                                                                    // ������� �������� ���
    void sendGucQuit();                                                                             // ������� �������� ��������� � ������ ��
    uint8_t *getGpsGucCoordinat(uint8_t *coord);                                                    // ������� ��������� ��������� � ������ ��

    void changeSmsFrequency();

    bool generateSmsReceived();
    int wzn_change(std::vector<int> &vect);
    int calcFstn(int R_ADR, int S_ADR, int RN_KEY, int DAY, int HRS, int MIN, int SEC, int QNB);

    int check_rx_call(int* wzn);

    uint8_t calc_ack_code(uint8_t ack);

    void setRx();
    void setTx();
    void setrRxFreq();
    void RxSmsWork();
    void TxSmsWork();

    void setPswfRx();
    void setPswfTx();
    void setPswfRxFreq();

    void LogicPswfTx();
    void LogicPswfRx();


    void sendSynchro(uint32_t freq, uint8_t cnt);
    void wakeUpTimer();
    void correctTime(uint8_t num);
    void LogicPswfModes(uint8_t* data, uint8_t indicator, int data_len); // func for 0x63 cadr from dsp

    void powerControlAsk();


    void recGucQuit();

    bool smsFind;

    void getZone();

    Navigation::Navigator *navigator;
    DataStorage::FS *data_storage_fs;

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
        radiostateCmdRxFreq,
        radiostateCmdTxFreq,
        radiostateCmdRxOff,
        radiostateCmdTxOff,
        radiostateCmdRxMode,
		radiostateCmdTxPower,
        radiostateCmdTxMode,
        radiostateCmdCarrierTx,
		radiostatePswfTxPrepare,
		radiostatePswfTx,
        radiostatePswfRxPrepare,
		radiostatePswf,
        radiostatePswfRx,
        radiostateSmsTx,
        radiostateSmsRx,
		radiostateSms,
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

    int command_tx30;                                           // ������� ��� tx � sms
    int command_rx30;                                           // ������� ��� rx � sms

    // ����������, ���������� �� ������������� ������ � DSP-������������
    int pswfRxStateSync;
    int pswfTxStateSync;
    int smsRxStateSync;
    int smsTxStateSync;
    int gucRxStateSync;
    int gucTxStateSync;
    //-------------------

    int success_pswf;                                           // ���� �������� �������� ��
    bool pswf_first_packet_received;                            // ����, �����  ������� 3 ������� � ���� ������
    bool pswf_ack;                                              // ����, ���������� ������� ��������� � ��
    bool pswf_ack_tx;
    int date_time[4];                                           // ������ ����-������� ��� ������ � �����������
    char private_lcode;                                         // ���������� - ������ ������� lcode ��� ��������� � ����������
    int pswf_retranslator = 0;                                  // ������� ������������
    bool isPswfFull = false;
    std::vector< std::vector<char> > recievedPswfBuffer;        // ����� ������ ��� ��
    std::vector< std::vector<uint8_t> > recievedSmsBuffer;      // ����� ������ ��� ���

    std::vector<int> snr;
    std::vector<int> syncro_recieve;                            // ����� ��� ������ CYC_N  � ���
    std::vector<int> tx_call_ask_vector;                        // ����� ��� ������ �������� WZN
    std::vector<int> quit_vector;                               // ����� ��� ������ ��������� � ���
    std::vector<std::vector<uint8_t>> guc_vector;

    int QNB = 0;                                                // ������� ��� fstn-��������� SMS Tx
    int QNB_RX = 0;                                             // ������� ��� fstn-��������� SMS Rx
    int count_clear = 0;                                        // ������� ��� ������� �������� (������������� ��������)
    int rs_data_clear[255];                                     // ������ �������� ��� ���� ����-��������
    int cntChvc = 7;                                            // ������� ������� �������� ���

    char sms_content[100];                                      // ������������� ������ ��� ������ ��� ��� ���������� ������
    uint8_t ack;                                                // ���������� ��� sms, ������ ��� �������� ��� ���������� �������� 73 | 99
    int ok_quit = 0;                                            // ���� ��� ��������� ���
    int smsError = 0;
    bool modem_rx_on, modem_tx_on;

    int trans_guc;                                              // ���� �������� ����� ��
    int pswf_rec = 0;                                           // ���� ��� ����������� ��������� ��������� � ��
    bool state_pswf = 0;                                        // ?
    int pswf_in = 0;
    int pswf_in_virt = 0;
    int wzn_value;                                              // �������� �������� ���� (�� 1 �� 4) ������ ���
    uint8_t sms_retranslation;                                  // ���� ������������ ��� ������ ���
    bool sms_call_received;                                     // ���� ��������� ����������� ������ ��� ���
    uint8_t firstTrueCommand = 100;
    bool isGpsGuc = false;                                      // ������� ��������� � ������
    bool unblockGucTx = false;                                  // ���� ��� �������� - �������(�������� �� sendCommand)
    bool failQuitGuc = false;                                   // ����, true ���� crc �� �������, ��������� � ��������� �������
    uint8_t guc_text[120];                                      // ������ �������� � ������ �� ����� ������ � �������� ��� ������ ����� ��
    uint8_t guc_coord[10];                                      // ������ ��� �������� ��������� � ���
    int  freqGucValue = 0;                                      // ������� ��� ��������� � ���
    uint8_t waitAckTimer = 0;
    uint8_t smsDataPacket[255];
    uint8_t pswfDataPacket[30];

    int CondCmdRxIndexer = 0;

    //----------- RTC LOGIC
    QmRtc *rtc;
    QmRtc::Time t;
    bool RtcTxRole;
    bool RtcRxRole;
    uint8_t RtcRxCounter;
    uint8_t RtcTxCounter;
    int8_t RtcFirstCatch;
    bool virtual_mode;
    uint8_t txrtx = 0;
    uint32_t freqVirtual;
    uint8_t count_VrtualTimer = 0;
    bool antiSync = false;

    uint8_t virtualTime[6];

    QmRtc::Date d;

    bool quest = false;

    bool setAsk = false;

    uint8_t indexSmsLen = 100;

    std::vector<int> waveZone;


    uint8_t indexerWaze = 0;
    uint8_t stationAddress;
    uint8_t VrtualTimerMagic = 10; // synchro packets count = 10   temporary for fast debug = 1

public:
    uint8_t getSmsCounter();
    bool getIsGucCoord();
};



} /* namespace Multiradio */

#endif /* FIRMWARE_APP_DSP_DSPCONTROLLER_H_ */
