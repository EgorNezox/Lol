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

#include "packagemanager.h"
#include "qmrtc.h"
#ifndef PORT__PCSIMULATOR
#include "qmusb.h"
#endif
#include "../navigation/navigator.h"
#include "../datastorage/fs.h"
#include "../../../system/usb_cdc.h"
#include "qmm25pdevice.h"
/**
 @file
 @brief Класс предназначен для выполения операций обмена между DSP и HOST
 @version 0.5
 @date 29.07.2015
 Описывает абстакции протокола обмена на канальном уровне, формирует пакеты, принимает и разбирает ответы от DSP, отсылает команды на установку
 различных режимов для цифровых и аналоговых функций станции
*/

#define DEFAULT_PACKET_HEADER_LEN	2
#define hw_rtc                      1
#define DefkeyValue 631


#define GUC_TIMER_ACK_WAIT_INTERVAL 180000
#define GUC_TIMER_INTERVAL_REC 30000

#define VIRTUAL_TIME 120

#define NUMS 0 // need = 0   9 for debug
#define startVirtTxPhaseIndex 0;


class QmTimer;
class QmIopin;

namespace Multiradio {

class  DspTransport;
struct DspCommand;
class PswfModes;

static int value_sec[60] =
{
    0,        5,       10,       15,       20,       25,
    1,        6,       11,       16,       21,       26,
    2,        7,       12,       17,       22,       27,
    3,        8,       13,       18,       23,       28,
    4,        9,       14,       19,       24,       29,
    0,        5,       10,       15,       20,       25,
    1,        6,       11,       16,       21,       26,
    2,        7,       12,       17,       22,       27,
    3,        8,       13,       18,       23,       28,
    4,        9,       14,       19,       24,       29
};

static int frequence_bandwidth[34] =
{
    1622000,     2158000,
    2206000,     2483000,
    2517000,     2610000,
    2665000,     2835000,
    3170000,     3385000,
    3515000,     3885000,
    4015000,     4635000,
    4765000,     4980000,
    5075000,     5465000,
    5745000,     5885000,
    6215000,     6510000,
    6780000,     7185000,
    7465000,     8800000,
    9055000,     9385000,
    9915000,     9980000,
    10115000,    11160000,
    11415000,    11585000

};


class DspController : public QmObject
{

public:
	#include "pubenumdspcontroller.h"
    DspController				(int uart_resource, int reset_iopin_resource, Navigation::Navigator *navigator, DataStorage::FS *data_storage_fs, QmObject *parent);
    ~DspController();

    QmM25PDevice *mydevice;

    sigc::signal<void, int> keyEmulate;

    void startServicing();
    void setRadioParameters		(RadioMode mode, uint32_t frequency);
    void setRadioOperation		(RadioOperation operation);
    void setRadioSquelch		(uint8_t value);
    void setAudioVolumeLevel	(uint8_t volume_level);
    void setAudioMicLevel		(uint8_t value);
    void setAGCParameters		(uint8_t agc_mode,int RadioPath);

    void startPSWFReceiving();   																// запускает прием  условных команд(УК)
    void startGucRecieving();           														// запуск приема групп ук
    void startSMSRecieving		(SmsStage stage = StageRx_call);  							    // запускает прим смс
    void startPSWFTransmitting	(bool ack,  uint8_t r_adr, uint8_t cmd,int retr); 				// запускает отправку групп ук
    void startSMSTransmitting	(uint8_t r_adr,uint8_t *message, SmsStage stage = StageTx_call);// запускает отправку смс
    void startGucTransmitting	(int r_adr, int speed_tx, std::vector<int> command,bool isGps); // запускает отправку групп
    void startGucTransmitting();        														// запуск отправки групп ук - перегруженный метод


    void startGucIntoVoice();
    void stopGucIntoVoice();


	void setReceiverState		(int state);
	void setTransmitterState	(int state);
	void setModemState			(int state);

    void GucSwichRxTxAndViewData();     														// переход от Tx к Rx, или от Rx к Tx в группах УК
    void *getContentPSWF();             														// функция получения структуры ППРЧ
    char *getSmsContent();              														// функция получения структуры СМС
    void setRnKey(int keyValue);        														// выполняет сохранения значения ключа радиосети

    void processSyncPulse();            														// функция, вызываемая по секундной метке -  способ отсчета времени для ППРЧ-режимов
    uint8_t* get_guc_vector();          														// функция доступа к структуре УК

    void defaultSMSTransmit();              // сброс текста сообщения
    void setSmsRetranslation(uint8_t retr); // сохранения параметра ретрансляции
    void setFreq(int value);                // функция получения частоты из модуля ui/service
    uint8_t getSmsRetranslation();          // функция получения статуса ретрансляции

    void enableModemReceiver();
    void disableModemReceiver();
    void setModemReceiverBandwidth(ModemBandwidth value);
    void setModemReceiverTimeSyncMode(ModemTimeSyncMode value);
    void setModemReceiverPhase(ModemPhase value);
    void setModemReceiverRole(ModemRole value);

    void enableModemTransmitter();
    void disableModemTransmitter();
	void tuneModemFrequency		(uint32_t value);

    void sendModemPacket		 (ModemPacketType type, ModemBandwidth bandwidth, const uint8_t *data, int data_len);
    void sendModemPacket_packHead(ModemBandwidth bandwidth, uint8_t param_signForm, uint8_t param_packCode, const uint8_t *data, int data_len);

    uint8_t  getSmsCounter();
    uint8_t *getGucCoord();              // получение координат для отображения на экране пользователя в режиме групп ук
    bool 	 getIsGucCoord();
    bool 	 isReady();
    bool 	 getVirtualMode();
    void 	 setVirtualMode(bool param);

    void goToVoice();
    void magic();
    void resetSmsState();
    void prevTime(uint8_t date_time[4]);

    void setAdr();
    void dspReset();

    void startVirtualPpsModeTx();
    void startVirtualPpsModeRx();

    void setVirtualDate(uint8_t *param);
    void setVirtualTime(uint8_t *param);

    void getVirtualDate(uint8_t *day, uint8_t *month, uint8_t *year);

    uint8_t* getVirtualTime();
    void completedStationMode(bool isGoToVoice){stationModeIsCompleted(isGoToVoice);}

    void startGucTimer();
    void stopGucTimer();

    void initResetState();
    void playSoundSignal(uint8_t mode, uint8_t speakerVolume, uint8_t gain, uint8_t soundNumber, uint8_t duration, uint8_t micLevel);
    void sendBatteryVoltage(int voltage);
    void sendHeadsetType(uint8_t type);
    void setStationAddress(uint8_t address);
    void clearWaveInfo();

    uint32_t CalcSmsTransmitFreq(uint32_t RN_KEY, uint32_t DAY, uint32_t HRS, uint32_t MIN, uint32_t SEC);


    sigc::signal<void> started;
    sigc::signal<void> rxModeSetting;
    sigc::signal<void> txModeSetting;
    sigc::signal<void> setRadioCompleted;
    sigc::signal<void> smsReceived;
    sigc::signal<void> startCondReceiving;
    sigc::signal<void> gucCrcFailed;
	sigc::signal<void> startRxQuit;
    sigc::signal<void> failedTxModemPacket;

    sigc::signal<void,int> smsFailed;       		   // ошибка приема СМС
    sigc::signal<void,int> smsPacketMessage;    	   // вывод сообщения на экран
    sigc::signal<void,int> smsCounterChanged;
    sigc::signal<void,int> recievedGucQuitForTransm;   // прием квитанции групп ук
    sigc::signal<void,int> TxCondCmdPackageTransmit;   // int - /*command_tx30*/
    sigc::signal<void,int> reciveRssiLevel;			   // ������� ����� �������

    sigc::signal<void,int> smsCounterFreq;

    sigc::signal<void, bool> stationModeIsCompleted; 	//bool gotovoice
    sigc::signal<void, bool> transmitAsk;

    sigc::signal<void, uint8_t> virtualCounterChanged;
    sigc::signal<void, uint8_t> qwitCounterChanged;

    sigc::signal<void, uint8_t, uint8_t> hardwareFailed; // arg - subdevice_code ,error_code
    sigc::signal<void,int,uint8_t,bool> firstPacket;     // получен первый пакет в УК

    sigc::signal<void,float,float> 	waveInfoRecieved; 	 //wave, power
    sigc::signal<void,int,  bool> 	recievedGucResp;    	 // ответ на группу ук

    sigc::signal<void, ModemPacketType/*type*/> transmittedModemPacket;
    sigc::signal<void, ModemPacketType/*type*/, uint8_t/*snr*/, uint8_t/*errors*/, ModemBandwidth/*bandwidth*/, uint8_t*/*data*/, int/*data_len*/> receivedModemPacket;
    sigc::signal<void, ModemPacketType/*type*/, uint8_t/*snr*/, uint8_t/*errors*/, ModemBandwidth/*bandwidth*/, uint8_t*/*data*/, int/*data_len*/> startedRxModemPacket;
    sigc::signal<void, uint8_t/*snr*/, uint8_t/*errors*/, ModemBandwidth/*bandwidth*/, uint8_t/*param_signForm*/, uint8_t/*param_packCode*/, uint8_t*/*data*/, int/*data_len*/> startedRxModemPacket_packHead;
    sigc::signal<void, ModemPacketType/*type*/> failedRxModemPacket;
    sigc::signal<void, int, int, int /*hrs,min,sec*/> vm1PpsPulse;

    sigc::signal<void, int> emulateKey;

    QmTimer *swr_timer;
    QmTimer *guc_rx_quit_timer 		   = 0;
    uint32_t guc_rx_quit_timer_counter = 0;

    QmTimer *waveInfoTimer;

    bool isTxAsk 			  = false;
    bool retranslation_active = false;

    float swf_res   = 2;
    float power_res = 0;

    uint8_t sms_counter;
    uint8_t CondComLogicRole;
    uint8_t SmsLogicRole = SmsRoleIdle;

    PackageManager 	*pack_manager;
    voice_emission_t emissionType;

    uint16_t sender = 0;



private:
	#include "privenumdspcontroller.h"
    friend struct DspCommand;
    friend class  PswfModes;

    void setRx();
    void setTx();

    void setrRxFreq();
    void RxSmsWork();
    void TxSmsWork();

    void setPswfRx();
    void setPswfTx();
    void setPswfRxFreq();
    void exitVoceMode();

    void powerControlAsk();
    void getZone();
    void vm1Pps();

    bool checkForTxAnswer();
    void processStartupTimeout();

    bool startRadioOff();
    bool startRadioRxMode();
    bool startRadioTxMode();
    bool startRadioCarrierTx();
    void processRadioState();
    void syncNextRadioState();

   // void getSwr();
    void sendPswf();
    void addSeconds(int *date_time);
    void addSeconds(QmRtc::Time *t);

    void changePswfFrequency();
    void syncPulseDetected();
    void getDataTime();
    void sendGucQuit();

    void syncPendingCommand();
    bool resyncPendingCommand();

    void processCommandTimeout();
    void processStartup			 (uint16_t id, uint16_t major_version, uint16_t minor_version);
    void processCommandResponse	 (bool success, Module module, int code, ParameterValue value);

    void sendCommand			 (Module module, int code, ParameterValue value,bool state = 0);    // функция отправки пакетов для dsp с буфером
    void sendCommandEasy		 (Module module, int code, ParameterValue value);                   // функция отправки пакетов для dsp без буфера

    void recGucQuit();
    void sendGuc();                                                                                 // функция отправки УК
    void onGucWaitingQuitTimeout();
    void processReceivedFrame	 (uint8_t address, uint8_t *data, int data_len);                    // функция приема кадров от DSP

    int prevSecond				 (int second);                                                      // функция получения предыдущей секунды
    int calcFstn				 (int R_ADR, int S_ADR, int RN_KEY, int DAY, int HRS, int MIN, int SEC, int QNB);
    int getFrequency			 (uint8_t mode);

    void wakeUpTimer();
    void correctTime			 (uint8_t num);
    void sendSynchro			 (uint32_t freq, uint8_t cnt);

    void parsing_cadr_form_pc(uint8_t* buffer);
    void transmit_answer_to_pc(uint8_t id, uint8_t* data, uint16_t size);

    uint32_t recSym = 0;

    void sendSms			     (Module module);
    uint8_t *getGpsGucCoordinat  (uint8_t *coord);

    void changeSmsFrequency();
    bool generateSmsReceived();

    int wzn_change		 	     (std::vector<int> &vect);
    int check_rx_call	 	     (int* wzn);
    uint8_t calc_ack_code	     (uint8_t ack);


    inline void recStart   (uint8_t address, uint8_t* data, int data_len);
    inline void recUndef   (uint8_t address, uint8_t* data, int data_len);
    inline void recGuc     (uint8_t address, uint8_t* data, int data_len);
    inline void recPps     (uint8_t address, uint8_t* data, int data_len);
    inline void recTractCmd(uint8_t address, uint8_t* data, int data_len);
    inline void recRxTxMod (uint8_t address, uint8_t* data, int data_len);
    inline void recGucLog  (uint8_t address, uint8_t* data, int data_len);
    inline void recModem   (uint8_t address, uint8_t* data, int data_len);
    inline void rec1ppsV   (uint8_t address, uint8_t* data, int data_len);

    void push_queue();


    int date_time[4];                                           // массив даты-времени для обмена и отображения
    char private_lcode;                                         // переменная - хранит текущий lcode для сравнения с полученным

    std::vector<int> snr;
    std::vector<int> syncro_recieve;                            // буфер для приема CYC_N  в СМС
    std::vector<int> waveZone;
    std::vector<int> tx_call_ask_vector;                        // буфер для приема значений WZN
    std::vector<int> quit_vector;                               // буфер для приема квитанции в СМС
    std::vector<std::vector<uint8_t>> guc_vector;
    std::vector< std::vector<uint8_t> > recievedSmsBuffer;      // буфер приема для СМС
    std::vector< std::vector<char> > recievedPswfBuffer;        // буфер приема для УК

    char sms_content [101];                                     // промежуточный массив для режима смс для подготовки вывода
    int rs_data_clear[255];                                     // массив стираний для кода рида-соломона
    int cntChvc = 7;                                            // счетчик пакетов передачи для

    int command_tx30;                                           // счетчик для tx в sms
    int command_rx30;                                           // счетчик для rx в sms
    int success_pswf;                                           // флаг успешной доставки ук

    int rxRssiLevel;

    int freqGucValue        = 0;                                // частота для установки в гук
    int pswf_retranslator   = 0;                                // наличие ретрансляции
    int CondCmdRxIndexer    = 0;

    int QNB 				= 0;                                // счетчик для fstn-параметра SMS Tx
    int QNB_RX 				= 0;                                // счетчик для fstn-параметра SMS Rx
    int count_clear 		= 0;                                // счетчик для массива стираний (недостоверные элементы)

    int *counterSms;
    int wzn_value;                                              // значение вызывной зоны (от 1 до 4) режима смс
    int trans_guc;                                              // флаг отправки групп ук
    int pswf_rec     		= 0;                                // флаг для отображения принятого сообщения в УК
    int pswf_in      		= 0;
    int pswf_in_virt 		= 0;
    int ok_quit        	    = 0;                                // флаг для квитанции СМС
    int sms_data_count 	    = 0;
    int smsError         	= 0;

    bool pswf_first_packet_received;                            // флаг, прием  минимум 3 пакетов в фазе вызова
    bool pswf_ack;                                              // флаг, означающий наличие квитанции в УК
    bool pswf_ack_tx;

    bool smsFind;
    bool is_ready;

    bool sms_call_received;                                     // флаг успешного прохождения вызова для смс
    bool modem_rx_on, modem_tx_on;
    bool state_pswf = 0;

    bool isGpsGuc     = false;                                  // наличие координат в приеме
    bool unblockGucTx = false;                                  // флаг для задержки - костыль(спасение от sendCommand)
    bool failQuitGuc  = false;                                  // флаг, true если crc не сошлось, переходит в состояние простоя

    bool antiSync 	  = false;
    bool quest        = false;
    bool setAsk       = false;
    bool isPswfFull   = false;

    bool boomVirtualPPS   = false;
    bool isGucWaitReceipt = false;

    bool RtcTxRole;
    bool RtcRxRole;
    bool virtual_mode;

    uint8_t ack;                                                // переменная для sms, хранит код успешной или потерянной передачи 73 | 99
    uint8_t firstTrueCommand = 100;
    uint8_t sms_retranslation;                                  // флаг ретрансляции для режима смс
    uint8_t guc_text	 [120];                                  // массив хранения и вывода на экран команд и коодинат для режима групп ук
    uint8_t guc_coord	  [10];                                  // массив для хранения координат в гук
    uint8_t smsDataPacket[255];
    uint8_t pswfDataPacket[30];

    uint8_t waitAckTimer    = 0;
    uint8_t txAckTimer      = 0;
    uint8_t smsSmallCounter = 0;
    uint8_t stationAddress;

    uint8_t indexSmsLen 	 = 100;
    uint8_t indexerWaze      = 0;
    uint8_t VrtualTimerMagic = 10; 							    // synchro packets count = 10   temporary for fast debug = 1
    uint8_t masterVirtualPps = 0;
    uint8_t virtGuiCounter 	 = 0;
    uint8_t RtcTxCounter;
    uint8_t count_VrtualTimer = 0;
    uint8_t virtualTime[6];

    uint8_t max_erase_sector = 0;

    int8_t RtcFirstCatch;
    uint32_t freqVirtual;

    float fwd_wave;
    float ref_wave;

    RadioMode 			  current_radio_mode;
    RadioOperation  	  current_radio_operation;
    uint32_t 			  current_radio_frequency;
    DspCommand 			  *pending_command;
    std::list<DspCommand> *cmd_queue;

    Navigation::Navigator *navigator;
    DataStorage::FS 	  *data_storage_fs;
    QmIopin 			  *reset_iopin;
    DspTransport 		  *transport;

    QmTimer *startup_timer, *command_timer;
    QmTimer *quit_timer;
    QmTimer *sync_pulse_delay_timer; //delay is needed for Navigator NMEA processing after sync pulse
    QmTimer *guc_timer;

    QmRtc *rtc;
#ifndef PORT__PCSIMULATOR
    QmUsb *usb;
#endif
    QmRtc::Time timeVirtual;
    QmRtc::Date d;

    bool isUsbReady = false;
    bool UseUsb = false;

    static void* usbrx;

public:
    PswfModes *pswf_module;
    bool newPacketUsb = false;
    void transmithFrame(uint8_t address, uint8_t *data, int data_len);


    sigc::signal<void, int> eraseUsbSector;
};

struct DspCommand {
	bool in_progress;
	bool sync_next;
	DspController::Module module;
	int code;
	DspController::ParameterValue value;
};

} /* namespace Multiradio */

#endif /* FIRMWARE_APP_DSP_DSPCONTROLLER_H_ */
