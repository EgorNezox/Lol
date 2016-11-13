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
#include "../datastorage/fs.h"
#include <list>
#include <vector>
#include "packagemanager.h"


class QmTimer;
class QmIopin;

namespace Multiradio {


class DspTransport;
struct DspCommand;


/**
 @file
 @brief Класс предназначен для выполения операций обмена между DSP и HOST
 @version 0.5
 @date 29.07.2015
 Описывает абстакции протокола обмена на канальном уровне, формирует пакеты, принимает и разбирает ответы от DSP, отсылает команды на установку
 различных режимов для цифровых и аналоговых функций станции
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

    void startPSWFReceiving(bool ack);   // запускает прием  условных команд(УК)
    void startPSWFTransmitting(bool ack,  uint8_t r_adr, uint8_t cmd,int retr); // запускает отправку групп ук

    void startSMSRecieving(SmsStage stage = StageRx_call);  // запускает прим смс
    void startSMSTransmitting(uint8_t r_adr,uint8_t *message, SmsStage stage = StageTx_call); // запускает отправку смс

    void startGucTransmitting(int r_adr, int speed_tx, std::vector<int> command,bool isGps); // запускает отправку групп
    void startGucTransmitting();        // запуск отправки групп ук - перегруженный метод
    void startGucRecieving();           // запуск приема групп ук
    void GucSwichRxTxAndViewData();     // переход от Tx к Rx, или от Rx к Tx в группах УК

    void *getContentPSWF();             // функция получения структуры ПП� Ч
    char *getSmsContent();              // функция получения структуры СМС
    void setRnKey(int keyValue);        // выполняет сохранения значения ключа радиосети
    void resetContentStructState();     // сброс логических состояний для ПП� Ч-режимов
    void processSyncPulse();            // функция, вызываемая по секундной метке -  способ отсчета времени для ПП� Ч-режимов
    uint8_t* get_guc_vector();          // функция доступа к структуре УК

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

    void defaultSMSTransmit();              // сброс текста сообщения
    void setSmsRetranslation(uint8_t retr); // сохранения параметра ретрансляции
    uint8_t getSmsRetranslation();          // функция получения статуса ретрансляции
    void setFreq(int value);                // функция получения частоты из модуля ui/service
    int getSmsForUiStage();                 // функция обновления статуса для Sms-режима
    uint8_t* getGucCoord();                 // получение координат для отображения на экране пользователя в режиме групп ук


    sigc::signal<void> started;
    sigc::signal<void> setRadioCompleted;
    sigc::signal<void,int> firstPacket;     // получен первый пакет в УК
    sigc::signal<void> smsReceived;         // принято СМС
    sigc::signal<void,int> smsFailed;       // ошибка приема СМС
    sigc::signal<void,int> smsPacketMessage;    // вывод сообщения на экран
    sigc::signal<void, ModemPacketType/*type*/> transmittedModemPacket;
    sigc::signal<void> failedTxModemPacket;
    sigc::signal<void, ModemPacketType/*type*/, uint8_t/*snr*/, uint8_t/*errors*/, ModemBandwidth/*bandwidth*/, uint8_t*/*data*/, int/*data_len*/> receivedModemPacket;
    sigc::signal<void, ModemPacketType/*type*/, uint8_t/*snr*/, uint8_t/*errors*/, ModemBandwidth/*bandwidth*/, uint8_t*/*data*/, int/*data_len*/> startedRxModemPacket;
    sigc::signal<void, uint8_t/*snr*/, uint8_t/*errors*/, ModemBandwidth/*bandwidth*/, uint8_t/*param_signForm*/, uint8_t/*param_packCode*/, uint8_t*/*data*/, int/*data_len*/> startedRxModemPacket_packHead;
    sigc::signal<void, ModemPacketType/*type*/> failedRxModemPacket;
    sigc::signal<void,int> recievedGucResp;    // ответ на группу ук
    sigc::signal<void,int> recievedGucQuitForTransm; // прием квитанции групп ук
    sigc::signal<void,int> updateSmsStatus;          // статус СМС
    sigc::signal<void> updateGucGpsStatus;    float swf_res = 2; // надо изменить значение на нижнее предельное
    sigc::signal<void> gucCrcFailed;                 // ошибка crc-суммы
    sigc::signal<void, uint8_t/*subdevice_code*/, uint8_t/*error_code*/> hardwareFailed;

    PackageManager *pack_manager;
    bool retranslation_active = false;

    void goToVoice();

    enum SmsRole
	{
    	SmsRoleTx = 0,
		SmsRoleRx = 1
	};

    uint8_t SmsLogicRole;
    uint8_t sms_counter;

private:
    friend struct DspCommand;

    int sms_data_count = 0;

    // перечисления для адресов передачи к DSP
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
    void sendCommand(Module module, int code, ParameterValue value,bool state = 0);                 // функция отправки пакетов для dsp с буфером
    void sendCommandEasy(Module module, int code, ParameterValue value);                            // функция отправки пакетов для dsp без буфера
    void sendPswf(Module module);                                                                   // функция отправки УК
    void sendGuc();                                                                                 // функция отправки групп УК
    void recGuc();                                                                                  // функция переключения RX->TX | TX->RX для групп УК
    void processReceivedFrame(uint8_t address, uint8_t *data, int data_len);                        // функция приема кадров от DSP


    int CalcShiftFreq(int RN_KEY, int SEC, int DAY, int HRS, int MIN);                              // функция рассчета частоты смещения для УК
    int CalcSmsTransmitFreq(int RN_KEY, int SEC, int DAY, int HRS, int MIN);                        // функция рассчета частоты смещения для СМС
    int CalcSmsTransmitRxRoleFreq(int RN_KEY, int SEC, int DAY, int HRS, int MIN);
    int CalcSmsTransmitTxRoleFreq(int RN_KEY, int SEC, int DAY, int HRS, int MIN);
    int prevSecond(int second);                                                                     // функция получения предыдущей секунды

    void RecievedPswf();                                                                            // функция проверки для lcode
    int getFrequencyPswf();                                                                         // функция рассчета СЛЕДУЮЩЕЙ ЧАСТОТЫ В УК
    int getFrequencySms();                                                                          // функция рассчета СЛЕДУЮЩЕЙ ЧАСТОЫТ В СМС

    void getSwr();                                                                                  // функция настройки шумоподавителя
    void transmitPswf();                                                                            // функция отправки УК
    void addSeconds(int *date_time);                                                                // функция добавления секунды к текущей секунде
    void changePswfRxFrequency();                                                                   // функция приема УК
    void syncPulseDetected();                                                                       // функция выполения задач по секундной метке
    void getDataTime();                                                                             // функция получения времени
    void transmitSMS();                                                                             // функция обработки состояний СМС-передачи
    void sendSms(Module module);                                                                    // функция отправки СМС
    void recSms();                                                                                  // функция обработки состояний СМС-приема
    void sendGucQuit();                                                                             // функция отправки квитанции в группе УК
    uint8_t *getGpsGucCoordinat(uint8_t *coord);                                                    // функция получения координат в группе УК

    void changeSmsFrequency();
    void startSMSCmdTransmitting(SmsStage stage);


    void generateSmsReceived();
    int wzn_change(std::vector<int> &vect);
    int calcFstn(int R_ADR, int S_ADR, int RN_KEY, int SEC, int MIN, int HRS, int DAY, int QNB);

    int check_rx_call();

    uint8_t calc_ack_code(uint8_t ack);

    void setRx();
    void setTx();
    void setrRxFreq();
    void RxSmsWork();
    void TxSmsWork();

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

    int command_tx30;                                           // счетчик для tx в sms
    int command_rx30;                                           // счетчик для rx в sms

    // переменные, отвечающие за синхронизацию обмена с DSP-контроллером
    int pswfRxStateSync;
    int pswfTxStateSync;
    int smsRxStateSync;
    int smsTxStateSync;
    int gucRxStateSync;
    int gucTxStateSync;
    //-------------------

    int success_pswf;                                           // флаг успешной доставки ук
    bool pswf_first_packet_received;                            // флаг, прием  минимум 3 пакетов в фазе вызова
    bool pswf_ack;                                              // флаг, означающий наличие квитанции в УК

    int date_time[4];                                           // массив даты-времени для обмена и отображения
    char private_lcode;                                         // переменная - хранит текущий lcode для сравнения с полученным
    int pswf_retranslator = 0;                                  // наличие ретрансляции

    std::vector< std::vector<char> > recievedPswfBuffer;        // буфер приема для УК
    std::vector< std::vector<uint8_t> > recievedSmsBuffer;      // буфер приема для СМС

    std::vector<int> syncro_recieve;                            // буфер для приема CYC_N  в СМС
    std::vector<int> tx_call_ask_vector;                        // буфер для приема значений WZN
    std::vector<int> quit_vector;                               // буфер для приема квитанции в СМС
    std::vector<std::vector<uint8_t>> guc_vector;

    int QNB = 0;                                                // счетчик для fstn-параметра SMS Tx
    int QNB_RX = 0;                                             // счетчик для fstn-параметра SMS Rx
    int count_clear = 0;                                        // счетчик для массива стираний (недостоверные элементы)
    int rs_data_clear[255];                                     // массив стираний для кода рида-соломона
    int cntChvc = 7;                                            // счетчик пакетов передачи для


    char sms_content[100];                                      // промежуточный массив для режима смс для подготовки вывода
    uint8_t ack;                                                // переменная для sms, хранит код успешной или потерянной передачи 73 | 99
    int ok_quit = 0;                                            // флаг для квитанции СМС
bool modem_rx_on, modem_tx_on;

    int trans_guc;                                              // флаг отправки групп ук
    int pswf_rec = 0;                                           // флаг для отображения принятого сообщения в УК
    bool state_pswf = 0;                                        // ?

    int wzn_value;                                              // значение вызывной зоны (от 1 до 4) режима смс
    uint8_t sms_retranslation;                                  // флаг ретрансляции для режима смс
    bool sms_call_received;                                     // флаг успешного прохождения вызова для смс

    bool isGpsGuc = false;                                      // наличие координат в приеме
    bool unblockGucTx = false;                                  // флаг для задержки - костыль(спасение от sendCommand)
    bool failQuitGuc = false;                                   // флаг, true если crc не сошлось, переходит в состояние простоя
    uint8_t guc_text[120];                                      // массив хранения и вывода на экран команд и коодинат для режима групп ук
    uint8_t guc_coord[10];                                      // массив для хранения координат в гук
    int  freqGucValue = 0;                                      // частота для установки в гук
};



} /* namespace Multiradio */

#endif /* FIRMWARE_APP_DSP_DSPCONTROLLER_H_ */
