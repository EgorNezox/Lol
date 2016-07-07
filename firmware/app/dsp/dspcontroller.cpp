/**
 ******************************************************************************
 * @file    dspcontroller.cpp
 * @author  Artem Pisarenko, PMR dept. software team, ONIIP, PJSC
 * @author  неизвестные
 * @date    22.12.2015
 *
 ******************************************************************************
 */

#include <malloc.h>
#include "qm.h"
#define QMDEBUGDOMAIN	dspcontroller
#include "qmdebug.h"
#include "qmendian.h"
#include "qmtimer.h"
#include "qmthread.h"
#include "qmiopin.h"

#include "dspcontroller.h"
#include "dsptransport.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include "..\dsp\rs_tms.h"

#include "../../../sazhenn.h"


#define PSWF_SELF_ADR	SAZHEN_NETWORK_ADDRESS

#define DEFAULT_PACKET_HEADER_LEN	2 // индикатор кадра + код параметра ("адрес" на самом деле не входит сюда, это "адрес назначения" из канального уровня)

#define DefkeyValue 631

#define GUC_TIMER_INTERVAL 2000
#define GUC_TIMER_INTERVAL_REC 30000

namespace Multiradio {

using namespace Galua;
rs_settings rs_255_93;

struct DspCommand {
	bool in_progress;
	bool sync_next;
	DspController::Module module;
	int code;
	DspController::ParameterValue value;
};

static int value_sec[60] =
{

    0,        5,       10,       15 ,      20,       25,
    1,        6,       11 ,      16,       21,       26,
    2,        7,       12,       17,       22,       27,
    3,        8,       13,       18,       23,       28,
    4,        9,       14,       19,       24,       29,
    0,        5,       10,       15,       20,       25,
    1,        6,       11 ,      16 ,      21,       26,
    2,        7,       12,       17,       22,       27,
    3,        8,       13,       18,       23,       28,
    4,        9,       14 ,      19,       24,       29

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

DspController::DspController(int uart_resource, int reset_iopin_resource, Navigation::Navigator *navigator, QmObject *parent) :
	QmObject(parent),
	is_ready(false)
{
	pending_command = new DspCommand;
	startup_timer = new QmTimer(true, this);
	startup_timer->timeout.connect(sigc::mem_fun(this, &DspController::processStartupTimeout));
	command_timer = new QmTimer(true, this);
	command_timer->setInterval(50); //50
	command_timer->timeout.connect(sigc::mem_fun(this, &DspController::processCommandTimeout));
	reset_iopin = new QmIopin(reset_iopin_resource, this);
	// max_tx_queue_size: 1 команда радиотракта + 1 запас
	transport = new DspTransport(uart_resource, 2, this);
	transport->receivedFrame.connect(sigc::mem_fun(this, &DspController::processReceivedFrame));
	initResetState();

    if (navigator != 0) {
    	this->navigator = navigator;
    	navigator->syncPulse.connect(sigc::mem_fun(this, &DspController::syncPulseDetected));
    }

	sync_pulse_delay_timer = new QmTimer(true, this);
	sync_pulse_delay_timer->setInterval(500);
	sync_pulse_delay_timer->timeout.connect(sigc::mem_fun(this, &DspController::processSyncPulse));

    cmd_queue = new std::list<DspCommand>();

    fwd_wave = 0;
    ref_wave = 0;

    command_tx30 = 0;
    command_rx30 = 0;

    pswfRxStateSync = 0;
    pswfTxStateSync = 0;
    smsRxStateSync = 0;
    smsTxStateSync = 0;
    gucRxStateSync = 0;
    gucTxStateSync = 0;

    success_pswf = 30;
    pswf_first_packet_received = false;
    pswf_ack = false;
    private_lcode = 0;
    count_clear = 0;

    counterSms = new int[8]{18,18,37,6,18,18,37,6};

    ContentSms.stage = StageNone;


    initrs(rs_255_93);
    GenerateGaloisField(&rs_255_93);
    gen_poly(&rs_255_93);

    pack_manager = new PackageManager();

    for(int i = 0; i<18;i++)
    {
        syncro_recieve.push_back(0);
    }

    ContentGuc.stage = GucNone;

    guc_timer = new QmTimer(true,this);
    guc_timer->setInterval(GUC_TIMER_INTERVAL);
    guc_timer->timeout.connect(sigc::mem_fun(this,&DspController::GucSwichRxTxAndViewData));

    guc_rx_quit_timer = new QmTimer(true,this);
    guc_rx_quit_timer->timeout.connect(sigc::mem_fun(this,&DspController::sendGucQuit));

    for(int i = 0;i<50;i++) guc_text[i] = '\0';
    guc_tx_num = 2;

    sms_call_received = false;
    for(int i = 0;i<255;i++) rs_data_clear[i] = 0;

    ContentSms.S_ADR = PSWF_SELF_ADR;
    ContentPSWF.S_ADR = PSWF_SELF_ADR;
    QNB = 0;
    pswf_rec = 0;

    ContentPSWF.RN_KEY = DefkeyValue;
    ContentSms.RN_KEY = DefkeyValue;

}
DspController::~DspController()

{
    delete reset_iopin;
    delete transport;
	delete pending_command;
    delete counterSms;
    delete startup_timer;
    delete command_timer;
    delete quit_timer;
    delete sync_pulse_delay_timer;
    delete guc_timer;
    delete guc_rx_quit_timer;
    delete cmd_queue;
}


bool DspController::isReady() {
	return is_ready;
}

void DspController::startServicing() {
	qmDebugMessage(QmDebug::Info, "start servicing...");
	initResetState();
	reset_iopin->writeOutput(QmIopin::Level_Low);
	QmThread::msleep(10);
	transport->enable();
	reset_iopin->writeOutput(QmIopin::Level_High);
	startup_timer->start(5000);
}

void DspController::setRadioParameters(RadioMode mode, uint32_t frequency) {
	bool processing_required = true;
	QM_ASSERT(is_ready);
	switch (radio_state) {
	case radiostateSync: {
		radio_state = radiostateCmdTxPower;
		break;
	}
	case radiostateCmdRxOff:
	case radiostateCmdTxOff:
	case radiostateCmdRxFreq:
	case radiostateCmdTxFreq: {
		radio_state = radiostateCmdRxFreq;
		break;
	}
	case radiostateCmdRxMode: {
		radio_state = radiostateCmdModeOffRx;
		break;
	}
	case radiostateCmdTxMode:
	case radiostateCmdCarrierTx: {
		radio_state = radiostateCmdModeOffTx;
		break;
	}
	default:
		processing_required = false;
		break;
	}
	current_radio_mode = mode;
	current_radio_frequency = frequency;
	if (processing_required)
		processRadioState();
}

void DspController::setRadioOperation(RadioOperation operation) {
	QM_ASSERT(is_ready);
	if (operation == current_radio_operation)
		return;
	bool processing_required = false;
	switch (operation) {
	case RadioOperationOff:
		processing_required = startRadioOff();
		break;
	case RadioOperationRxMode:
		processing_required = startRadioRxMode();
		break;
	case RadioOperationTxMode:
		processing_required = startRadioTxMode();
		break;
	case RadioOperationCarrierTx:
		processing_required = startRadioCarrierTx();
		break;
	}
	current_radio_operation = operation;
	if (processing_required)
        processRadioState();
}

void DspController::setRadioSquelch(uint8_t value) {
	QM_ASSERT(is_ready);
	ParameterValue command_value;
	command_value.squelch = value;
	sendCommandEasy(RxRadiopath, RxSquelch, command_value);
}

void DspController::setAudioVolumeLevel(uint8_t volume_level)
{
    QM_ASSERT(is_ready);
    if (!resyncPendingCommand())
        return;
    ParameterValue command_value;
    command_value.volume_level = volume_level;
    sendCommand(Audiopath, AudioVolumeLevel, command_value);
}

void DspController::setAudioMicLevel(uint8_t value) {
    QM_ASSERT(is_ready);
    ParameterValue command_value;
    command_value.mic_amplify = value;
    sendCommandEasy(Audiopath, AudioMicAmplify, command_value);
}

void DspController::setAGCParameters(uint8_t agc_mode,int RadioPath)
{
    QM_ASSERT(is_ready);
    if (!resyncPendingCommand())
        return;
    ParameterValue command_value;
    command_value.agc_mode = agc_mode;
    if (RadioPath == 0)
      sendCommand(RxRadiopath,AGCRX,command_value);
    else
      sendCommand(TxRadiopath,AGCTX,command_value);
    resyncPendingCommand();
}

void DspController::getSwr()
{
    QM_ASSERT(is_ready);
    if (!resyncPendingCommand())
        return;

    ParameterValue commandValue;
    commandValue.swf_mode = 5;
    sendCommand(TxRadiopath,0,commandValue);

    commandValue.swf_mode = 6;
    sendCommand(TxRadiopath,0,commandValue);
}


void DspController::syncPulseDetected() {
	sync_pulse_delay_timer->start();
}


void DspController::processSyncPulse(){
	if (!is_ready)
		return;
	switch (radio_state) {
	case radiostatePswfRx : {
        qmDebugMessage(QmDebug::Dump, "processSyncPulse() radiostatePswfRx");
        changePswfRxFrequency();
        break;
	}
	case radiostatePswfTx : {
        qmDebugMessage(QmDebug::Dump, "processSyncPulse() radiostatePswfTx");
        transmitPswf();
        break;
	}
    case radiostateSmsRx:{
        qmDebugMessage(QmDebug::Dump, "processSyncPulse() radiostateSmsRx");
        changeSmsRxFrequency();
        break;
    }
    case radiostateSmsTx:{
        qmDebugMessage(QmDebug::Dump, "processSyncPulse() radiostateSmsTx");
        transmitSMS();
        break;
    }
    case radiostateSmsTxRxSwitch: {
    	qmDebugMessage(QmDebug::Dump, "processSyncPulse() radiostateSmsTxRxSwitch");
    	radio_state = radiostateSmsRxPrepare;
    	startSMSRecieving(ContentSms.stage);
    	break;
    }
    case radiostateGucTx:{
    	if (trans_guc == 1){
    	 sendGuc();
    	 trans_guc = 0;
    	}
        break;
    }
    case radiostateGucRx:{
        break;
    }
    case radiostateGucSwitch:{
        qmDebugMessage(QmDebug::Dump, "processSyncPulse() radiostateGucTxRxSwitch");
        radio_state = radiostateGucRxPrepare;
        startGucRecieving();
        break;
    }
	default: break;
    }
}

void DspController::getDataTime()
{
    Navigation::Coord_Date date = navigator->getCoordDate();

    char day_ch[3] = {0,0,0};
    char hr_ch[3] =  {0,0,0};
    char mn_ch[3] = {0,0,0};
    char sec_ch[3] = {0,0,0};

    memcpy(day_ch,&date.data[0],2);
    memcpy(hr_ch,&date.time[0],2);
    memcpy(mn_ch,&date.time[2],2);
    memcpy(sec_ch,&date.time[4],2);

    int day = atoi(day_ch);
    int hrs = atoi(hr_ch);
    int min = atoi(mn_ch);
    int sec = atoi(sec_ch);

    date_time[0] = day;
    date_time[1] = hrs;
    date_time[2] = min;
    date_time[3] = sec;

    qmDebugMessage(QmDebug::Dump, "getDataTime(): %d %d %d %d", day, hrs, min, sec);

    addSeconds(date_time);
}

void DspController::transmitSMS()
{
    getDataTime();

    ContentSms.CYC_N += 1;

    if (ContentSms.stage == StageRx_call_ack){
        ContentSms.L_CODE = navigator->Calc_LCODE_SMS(
                    ContentSms.R_ADR, ContentSms.S_ADR,
                    wzn_value, ContentSms.RN_KEY,
                    date_time[0], date_time[1], date_time[2], date_time[3]);
    }
    else
    {

        ContentSms.L_CODE = navigator->Calc_LCODE(
                    ContentSms.R_ADR, ContentSms.S_ADR,
                    ContentSms.CYC_N, ContentSms.RN_KEY,
                    date_time[0], date_time[1], date_time[2], date_time[3]);
    }


    if (ContentSms.stage == StageTx_call)
    {
        if (counterSms[StageTx_call] == 0)
        {
            counterSms[StageTx_call] = 18;
            qmDebugMessage(QmDebug::Dump, "ContentSms.stage = StageTx_call_ack");
            ContentSms.stage = StageTx_call_ack;
            radio_state = radiostateSmsTxRxSwitch;
            updateSmsStatus(getSmsForUiStage());
            return;
        }
        else
        {
            counterSms[StageTx_call] = counterSms[StageTx_call] - 1;
            qmDebugMessage(QmDebug::Dump, "counterSms[StageTx_call] = %d", counterSms[StageTx_call]);
        }
    }

    if (ContentSms.stage == StageRx_call_ack)
    {
        if (counterSms[StageRx_call_ack] == 0)
        {
            counterSms[StageRx_call_ack] = 18;
            qmDebugMessage(QmDebug::Dump, "ContentSms.stage = StageRx_data");
            QNB_RX = 0;
            ContentSms.stage = StageRx_data;
            //radio_state = radiostateSmsTxRxSwitch;
            radio_state = radiostateSmsRxPrepare;
            startSMSRecieving(ContentSms.stage);
            updateSmsStatus(getSmsForUiStage());
            return;
        }
        else
        {
            counterSms[StageRx_call_ack] = counterSms[StageRx_call_ack] - 1;
            qmDebugMessage(QmDebug::Dump, "counterSms[StageRx_call_ack] = %d", counterSms[StageRx_call_ack]);
        }
    }

    if (ContentSms.stage == StageTx_data)
    {
    	if (counterSms[StageTx_data] == 0)
    	{
    		counterSms[StageTx_data] = 37;
    		qmDebugMessage(QmDebug::Dump, "ContentSms.stage = StageTx_quit");
    		ContentSms.stage = StageTx_quit;
    		radio_state = radiostateSmsTxRxSwitch;
            updateSmsStatus(getSmsForUiStage());
    		QNB = 0;
    	}
    	else
    	{
    		counterSms[StageTx_data] = counterSms[StageTx_data] - 1;
    		qmDebugMessage(QmDebug::Dump, "counterSms[StageTx_data] = %d", counterSms[StageTx_data]);
    	}
    }
    if (ContentSms.stage == StageRx_quit)
    {
        if (counterSms[StageRx_quit] == 0)
        {
            counterSms[StageRx_quit] = 6;
            qmDebugMessage(QmDebug::Dump, "ContentSms.stage = StageNone");
            ContentSms.stage = StageNone;
            radio_state = radiostateSync;
            qmDebugMessage(QmDebug::Dump, "Sms receiving finished");
            return;
        }
        else
        {
            counterSms[StageRx_quit] = counterSms[StageRx_quit] - 1; // TODO:
            qmDebugMessage(QmDebug::Dump, "counterSms[StageRx_quit] = %d", counterSms[StageRx_quit]);
        }
    }


    sendSms(PSWFTransmitter);
}

void DspController::transmitPswf()
{
    getDataTime();

    if (ContentPSWF.RET_end_adr > 0) {
    	ContentPSWF.L_CODE = navigator->Calc_LCODE_RETR(ContentPSWF.RET_end_adr,
    			ContentPSWF.R_ADR,
				ContentPSWF.COM_N, ContentPSWF.RN_KEY,
				date_time[0], date_time[1], date_time[2], date_time[3]);
    }

    else{
    	ContentPSWF.L_CODE = navigator->Calc_LCODE(
    			ContentPSWF.R_ADR, ContentPSWF.S_ADR,
				ContentPSWF.COM_N, ContentPSWF.RN_KEY,
				date_time[0], date_time[1], date_time[2], date_time[3]);
    }

    qmDebugMessage(QmDebug::Dump, "transmitPswf() command_tx30 = %d", command_tx30);
    if (command_tx30 == 31)
    {
        qmDebugMessage(QmDebug::Dump, "PSWF trinsmitting finished");
        command_tx30 = 0;
        if (pswf_ack) {
            startPSWFReceiving(false);
            state_pswf = true;
        } else {
        	qmDebugMessage(QmDebug::Dump, "radio_state = radiostateSync");
        	radio_state = radiostateSync;
        }
        return;
    }
    ++command_tx30;
    sendPswf(PSWFTransmitter);
}
//TODO
void DspController::addSeconds(int *date_time) {
    date_time[3] += 1;
    if (date_time[3] >= 60) {
        date_time[3] %= 60;
        date_time[2]++;
        if (date_time[2] >= 60) {
            date_time[2] %= 60;
            date_time[1]++;
            if (date_time[1] >= 24) {
                date_time[1] %= 24;
                date_time[0]++;
            }
        }
    }
}

void DspController::changePswfRxFrequency()
{
	if (pswf_first_packet_received) {
		success_pswf--;
		qmDebugMessage(QmDebug::Dump, "changePswfRxFrequency() success_pswf = %d", success_pswf);
		if (success_pswf == 0) {
			qmDebugMessage(QmDebug::Dump, "PSWF receiving finished");
			pswf_first_packet_received = false;
			success_pswf = 30;
			command_rx30 = 0;
			if (pswf_ack) {
                startPSWFTransmitting(false, ContentPSWF.S_ADR, ContentPSWF.COM_N,0); //TODO:
			} else {
				qmDebugMessage(QmDebug::Dump, "radio_state = radiostateSync");
				radio_state = radiostateSync;
			}
			return;
		}
	}

    getDataTime();
    ContentPSWF.Frequency = getFrequencyPswf();

    ParameterValue param;
    param.frequency = ContentPSWF.Frequency;
    sendCommand(PSWFReceiver, PswfRxFrequency, param);
}


void DspController::changeSmsRxFrequency()
{
	getDataTime();
    ContentSms.Frequency =  getFrequencySms();

	ParameterValue param;
	param.frequency = ContentSms.Frequency;
	if (ContentSms.stage == StageRx_data){
		sendCommand(PSWFReceiver, 3, param);
	}
	else
		sendCommand(PSWFReceiver, PswfRxFrequency, param);

		recSms();
}


void DspController::RecievedPswf()
{
	pswf_first_packet_received = true;
    qmDebugMessage(QmDebug::Dump, "RecievedPswf() command_rx30 = %d", command_rx30);
    if (command_rx30 == 30) {
        command_rx30 = 0;
        recievedPswfBuffer.erase(recievedPswfBuffer.begin());
    }


    private_lcode = (char)navigator->Calc_LCODE(ContentPSWF.R_ADR,ContentPSWF.S_ADR,recievedPswfBuffer.at(recievedPswfBuffer.size()-1).at(0),ContentPSWF.RN_KEY,
            date_time[0],date_time[1], date_time[2], prevSecond(date_time[3])); //TODO: fix receiving ? prevSEC

    // TODO: make to 32 to pswf masters

    qmDebugMessage(QmDebug::Dump, "private_lcode = %d,lcode = %d", private_lcode,recievedPswfBuffer.at(recievedPswfBuffer.size()-1).at(1));


    if (recievedPswfBuffer.at(recievedPswfBuffer.size()-1).at(1) == private_lcode){
    	++pswf_rec;
    	if (pswf_rec == 1)
    	{
    		ContentPSWF.COM_N = recievedPswfBuffer.at(recievedPswfBuffer.size()-1).at(0);
    		ContentPSWF.R_ADR = ContentPSWF.S_ADR;
    		ContentPSWF.S_ADR = PSWF_SELF_ADR;
    		if (ContentPSWF.R_ADR > 32) ContentPSWF.R_ADR = ContentPSWF.R_ADR - 32;
    		qmDebugMessage(QmDebug::Dump, "r_adr = %d,s_adr = %d", ContentPSWF.R_ADR,ContentPSWF.S_ADR);
    	}
    }

    if (pswf_rec == 3) firstPacket(ContentPSWF.COM_N);

//    for(uint8_t i =0; i<recievedPswfBuffer.size(); i++)
//    {
//    	if (i != command_rx30)
//    		if (recievedPswfBuffer.at(command_rx30).at(0) == recievedPswfBuffer.at(i).at(0)) {
//    			ContentPSWF.COM_N = recievedPswfBuffer.at(command_rx30).at(0);
//    			ContentPSWF.R_ADR = ContentPSWF.S_ADR;
//    			ContentPSWF.S_ADR = PSWF_SELF_ADR;
//    			if (private_lcode == recievedPswfBuffer.at(command_rx30).at(1)) {
//    				++pswf_rec;
//    			}
//    			if (pswf_rec == 3)
//    			firstPacket(ContentPSWF.COM_N);
//    		}
//    }
    ++command_rx30;
}

int DspController::prevSecond(int second) {
	if (second == 0)
		return 59;
	return second - 1;
}

int DspController::getFrequencyPswf()
{

	int fr_sh = CalcShiftFreq(ContentPSWF.RN_KEY,date_time[3],date_time[0],date_time[1],date_time[2]);
	fr_sh += 1622;

	fr_sh = fr_sh * 1000; // Гц

	for(int i = 0; i<32;i+=2)
	{
		if((fr_sh >= frequence_bandwidth[i]) && (fr_sh <= frequence_bandwidth[i+1]))
		{
			break;
		}else{
			fr_sh += (frequence_bandwidth[i+2] - frequence_bandwidth[i+1]);
		}
	}

	qmDebugMessage(QmDebug::Dump,"frequency:  %d ", fr_sh);

    return fr_sh;
}

int DspController::getFrequencySms()
{
    int fr_sh = CalcSmsTransmitFreq(ContentSms.RN_KEY,date_time[3],date_time[0],date_time[1],date_time[2]);
    fr_sh += 1622;

    fr_sh = fr_sh * 1000; // Гц

    for(int i = 0; i<32;i+=2)
    {
        if((fr_sh >= frequence_bandwidth[i]) && (fr_sh <= frequence_bandwidth[i+1]))
        {
            break;
        }else{
            fr_sh += (frequence_bandwidth[i+2] - frequence_bandwidth[i+1]);
        }
    }

    qmDebugMessage(QmDebug::Dump,"frequency:  %d ", fr_sh);

    return  fr_sh;
}

void DspController::setRnKey(int keyValue)
{
    ContentPSWF.RN_KEY = keyValue;
    ContentSms.RN_KEY  = keyValue;
    // somthing else setting rn_key
}




int DspController::CalcShiftFreq(int RN_KEY, int SEC, int DAY, int HRS, int MIN)
{
    int TOT_W = 6671; // ширина разрешенных участков

    int SEC_MLT = value_sec[SEC]; // SEC_MLT выбираем в массиве

    int FR_SH = (RN_KEY + 230*SEC_MLT + 19*MIN + 31*HRS + 37*DAY)% TOT_W;

    qmDebugMessage(QmDebug::Dump, "Calc freq formula %d", FR_SH);
    return FR_SH;
}

int DspController::CalcSmsTransmitFreq(int RN_KEY, int SEC, int DAY, int HRS, int MIN)
{
    int wzn = 0;
    int FR_SH = 0;
    int TOT_W = 6671;
    int wz_base = 0;

    int SEC_MLT = value_sec[SEC];

    if (ContentSms.stage == StageTx_data ||(ContentSms.stage == StageRx_data) ||
       (ContentSms.stage == StageTx_quit)||(ContentSms.stage == StageRx_quit))
    {
    	if  (SEC_MLT >=0   && SEC_MLT <6 ) wzn = 0;
    	if  (SEC_MLT > 5   && SEC_MLT <12) wzn = 1;
    	if  (SEC_MLT >= 12 && SEC_MLT <18) wzn = 2;
    	if  (SEC_MLT >=18  && SEC_MLT <24) wzn = 3;
    	if  (SEC_MLT >= 24 && SEC_MLT <30) wzn = 4;

    	if (wzn > 0) wz_base = 6*wzn;
    	else wzn  = 0;

    	int wz_shift = SEC % 6;
    	SEC_MLT = wz_shift + wz_base;

    	qmDebugMessage(QmDebug::Dump, "wzn_base %d" ,wz_base);
    	qmDebugMessage(QmDebug::Dump, "wzn_shift %d" ,wz_shift);
    }

    qmDebugMessage(QmDebug::Dump, "SEC_MLT %d" ,SEC_MLT);

    if ((ContentSms.stage == StageTx_call) || (ContentSms.stage == StageRx_call) ||
            (ContentSms.stage == StageTx_call_ack) || (ContentSms.stage == StageRx_call_ack) || (ContentSms.stage == StageNone)) {
        FR_SH = (RN_KEY + 73 + 230*SEC_MLT + 17*MIN + 29*HRS + 43*DAY)% TOT_W;
        qmDebugMessage(QmDebug::Dump, "Calc freq sms tx formula %d", FR_SH);
    }
    if ((ContentSms.stage == StageTx_data) || (ContentSms.stage == StageRx_data) ){
        FR_SH = (RN_KEY + 3*SEC + 230*SEC_MLT + 17*MIN + 29*HRS + 43*DAY)% TOT_W;
        qmDebugMessage(QmDebug::Dump, "Calc freq sms  formula %d", FR_SH);
    }
    if ((ContentSms.stage == StageTx_quit) || (ContentSms.stage == StageRx_quit)){
        FR_SH = (RN_KEY + 5*SEC + 230*SEC_MLT + 17*MIN + 29*HRS + 43*DAY)% TOT_W;
        qmDebugMessage(QmDebug::Dump, "Calc freq sms quit formula %d", FR_SH);
    }
    return FR_SH;
}


void DspController::initResetState() {
	radio_state = radiostateSync;
	current_radio_operation = RadioOperationOff;
	current_radio_mode = RadioModeOff;
    current_radio_frequency = 0;
	pending_command->in_progress = false;
}

void DspController::processStartup(uint16_t id, uint16_t major_version, uint16_t minor_version) {
	if (!is_ready) {
		qmDebugMessage(QmDebug::Info, "DSP started (id=0x%02X, version=%u.%u)", id, major_version, minor_version);
		startup_timer->stop();
		is_ready = true;
	} else {
		qmDebugMessage(QmDebug::Warning, "DSP restart detected");
		initResetState();
	}
	started();
}

void DspController::processStartupTimeout() {
	qmDebugMessage(QmDebug::Warning, "DSP startup timeout");
	is_ready = true;
	started();
}

bool DspController::startRadioOff() {
	switch (radio_state) {
	case radiostateSync: {
		switch (current_radio_operation) {
		case RadioOperationRxMode:
			radio_state = radiostateCmdRxOff;
			break;
		case RadioOperationTxMode:
		case RadioOperationCarrierTx:
			radio_state = radiostateCmdTxOff;
			break;
		default:
			return false;
		}
		break;
	}
	case radiostateCmdRxMode:
		radio_state = radiostateCmdRxOff;
		break;
	case radiostateCmdTxMode:
	case radiostateCmdCarrierTx:
		radio_state = radiostateCmdTxOff;
		break;
	default:
		return false;
	}
	return true;
}

bool DspController::startRadioRxMode() {
	switch (radio_state) {
	case radiostateSync:
	case radiostateCmdTxMode:
	case radiostateCmdCarrierTx:
		if (current_radio_operation == RadioOperationOff)
			radio_state = radiostateCmdRxMode;
		else
			radio_state = radiostateCmdTxOff;
		break;
	case radiostateCmdRxOff:
	case radiostateCmdTxOff:
		radio_state = radiostateCmdRxMode;
		break;
	default:
		return false;
	}
	return true;
}

bool DspController::startRadioTxMode() {
	switch (radio_state) {
	case radiostateSync:
	case radiostateCmdRxMode:
		if (current_radio_operation == RadioOperationOff)
			radio_state = radiostateCmdTxMode;
		else
			radio_state = radiostateCmdRxOff;
		break;
	case radiostateCmdRxOff:
	case radiostateCmdTxOff:
	case radiostateCmdCarrierTx:
		radio_state = radiostateCmdTxMode;
		break;
	default:
		return false;
	}
	return true;
}

bool DspController::startRadioCarrierTx() {
	switch (radio_state) {
	case radiostateSync:
	case radiostateCmdRxMode:
		if (current_radio_operation == RadioOperationOff)
			radio_state = radiostateCmdCarrierTx;
		else
			radio_state = radiostateCmdRxOff;
		break;
	case radiostateCmdRxOff:
	case radiostateCmdTxOff:
	case radiostateCmdTxMode:
		radio_state = radiostateCmdCarrierTx;
		break;
	default:
		return false;
	}
	return true;
}

void DspController::processRadioState() {
	if (!resyncPendingCommand())
		return;
	ParameterValue command_value;
	switch (radio_state) {
	case radiostateSync:
		break;
	case radiostateCmdTxPower: {
		if (current_radio_frequency >= 30000000)
			command_value.power = 80;
		else
			command_value.power = 100;
		sendCommand(TxRadiopath, TxPower, command_value);
		break;
	}
	case radiostateCmdRxFreq: {
		command_value.frequency = current_radio_frequency;
		sendCommand(RxRadiopath, RxFrequency, command_value);
		break;
	}
	case radiostateCmdTxFreq: {
		command_value.frequency = current_radio_frequency;
		sendCommand(TxRadiopath, TxFrequency, command_value);
		break;
	}
	case radiostateCmdModeOffRx:
	case radiostateCmdRxOff: {
		command_value.radio_mode = RadioModeOff;
		sendCommand(RxRadiopath, RxRadioMode, command_value);
		break;
	}
	case radiostateCmdModeOffTx:
	case radiostateCmdTxOff: {
		command_value.radio_mode = RadioModeOff;
		sendCommand(TxRadiopath, TxRadioMode, command_value);
		break;
	}
	case radiostateCmdRxMode: {
		command_value.radio_mode = current_radio_mode;
		sendCommand(RxRadiopath, RxRadioMode, command_value);
		break;
	}
	case radiostateCmdTxMode: {
		command_value.radio_mode = current_radio_mode;
		sendCommand(TxRadiopath, TxRadioMode, command_value);
		break;
	}
	case radiostateCmdCarrierTx: {
		command_value.radio_mode = RadioModeCarrierTx;
		sendCommand(TxRadiopath, TxRadioMode, command_value);
		break;
	}
	default: break;
	}
}

void DspController::syncNextRadioState() {
	switch (radio_state) {
	case radiostateSync:
		QM_ASSERT(0);
		break;
	case radiostateCmdTxPower: {
		switch (current_radio_operation) {
		case RadioOperationOff:
			radio_state = radiostateCmdRxFreq;
			break;
		case RadioOperationRxMode:
			radio_state = radiostateCmdModeOffRx;
			break;
		case RadioOperationTxMode:
		case RadioOperationCarrierTx:
			radio_state = radiostateCmdModeOffTx;
			break;
		}
		break;
	}
	case radiostateCmdModeOffRx:
	case radiostateCmdModeOffTx: {
		radio_state = radiostateCmdRxFreq;
		break;
	}
	case radiostateCmdRxFreq: {
		radio_state = radiostateCmdTxFreq;
		break;
	}
	case radiostateCmdTxFreq:
	case radiostateCmdRxOff:
	case radiostateCmdTxOff: {
		switch (current_radio_operation) {
		case RadioOperationOff:
			radio_state = radiostateSync;
			break;
		case RadioOperationRxMode:
			radio_state = radiostateCmdRxMode;
			break;
		case RadioOperationTxMode:
			radio_state = radiostateCmdTxMode;
			break;
		case RadioOperationCarrierTx:
			radio_state = radiostateCmdCarrierTx;
			break;
		}
		break;
	}
	case radiostateCmdRxMode:
	case radiostateCmdTxMode:
	case radiostateCmdCarrierTx: {
		radio_state = radiostateSync;
		break;
	}
	default: break;
	}
	if (radio_state == radiostateSync)
		setRadioCompleted();
}

void DspController::processCommandTimeout() {
	QM_ASSERT(pending_command->in_progress);
	qmDebugMessage(QmDebug::Warning, "dsp response timed out");
	syncPendingCommand();
}

void DspController::processCommandResponse(bool success, Module module, int code, ParameterValue value) {
	QM_UNUSED(value);
	if(!pending_command->in_progress) {
		qmDebugMessage(QmDebug::Warning, "dsp response, but no command was sent");
		return;
	}
	if ((module == pending_command->module) /*&& (code == pending_command->code)*/) {
		command_timer->stop();
		if (!success)
			qmDebugMessage(QmDebug::Info, "dsp command failed (module=0x%02X, code=0x%02X)", module, code);
		syncPendingCommand();
	} else {
		qmDebugMessage(QmDebug::Warning, "dsp command response was unexpected (module=0x%02X, code=0x%02X)", module, code);
	}
}

void DspController::syncPendingCommand() {
	qmDebugMessage(QmDebug::Dump,"reload progress state");
	pending_command->in_progress = false;
	switch (pending_command->module) {
	case RxRadiopath:
	case TxRadiopath:
		if (pending_command->sync_next)
			syncNextRadioState();
		processRadioState();
		break;
    case Audiopath:
        radio_state = radiostateSync;
        break;
    case PSWFReceiver:
    case PSWFTransmitter:
    	break;
    default:
    	break;
	}
}

bool DspController::resyncPendingCommand() {
	if (pending_command->in_progress) {
		pending_command->sync_next = false;
		return false;
	}
	return true;
}



void DspController::sendCommandEasy(Module module, int code, ParameterValue value){
	qmDebugMessage(QmDebug::Dump, "sendCommand(%d, %d) transmiting", module, code);
	uint8_t tx_address;
	uint8_t tx_data[DspTransport::MAX_FRAME_DATA_SIZE];
	int tx_data_len = DEFAULT_PACKET_HEADER_LEN;
	qmToBigEndian((uint8_t)2, tx_data+0); // индикатор: "команда (установка)"
	qmToBigEndian((uint8_t)code, tx_data+1); // код параметра
	switch (module) {
	case RxRadiopath:
	case TxRadiopath: {
		if (module == RxRadiopath)
			tx_address = 0x50;
		else
			tx_address = 0x80;

		switch (code) {
		case 1:
			qmToBigEndian(value.frequency, tx_data+tx_data_len);
			tx_data_len += 4;
			break;
		case 2:
			qmToBigEndian((uint8_t)value.radio_mode, tx_data+tx_data_len);
			tx_data_len += 1;
			break;
		case 4:
			if (module == RxRadiopath) {
				qmToBigEndian((uint8_t)value.squelch, tx_data+tx_data_len);
				tx_data_len += 1;
			} else {
				qmToBigEndian((uint8_t)value.power, tx_data+tx_data_len);
				tx_data_len += 1;
			}
			break;
		case 7:
		case 8:
			qmToBigEndian((uint8_t)value.agc_mode, tx_data+tx_data_len);
			tx_data_len += 1;
			break;
		default: QM_ASSERT(0);
		}
		break;
	}
	case Audiopath: {
		tx_address = 0x90;
		switch (code) {
		case AudioModeParameter:
			qmToBigEndian((uint8_t)value.audio_mode, tx_data+tx_data_len);
			tx_data_len += 1;
			break;
		case AudioVolumeLevel:
			qmToBigEndian((uint8_t)value.volume_level, tx_data+tx_data_len);
			tx_data_len += 1;
			break;
		case AudioMicAmplify:
			qmToBigEndian((uint8_t)value.mic_amplify, tx_data+tx_data_len);
			tx_data_len += 1;
			break;
		default: QM_ASSERT(0);
		}
		break;
	}
	// для ПП� Ч
	case PSWFTransmitter: {
		QM_ASSERT(0);
		break;
	}

	case PSWFReceiver: {
		tx_address = 0x60;
		switch (code) {
		case PswfRxRAdr: {
			qmToBigEndian((uint8_t)value.pswf_r_adr, tx_data+tx_data_len);
			tx_data_len += 1;
			break;
		}
		case PswfRxFrequency:
		case PswfRxFreqSignal:
		{
			qmToBigEndian(value.frequency, tx_data+tx_data_len);
			tx_data_len += 4;
			if (ContentSms.stage == StageRx_data)
			{
				qmToBigEndian(value.frequency, tx_data+tx_data_len);
				tx_data_len += 4;
				qmToBigEndian(0, tx_data+tx_data_len);
				tx_data_len += 1;

			}
			break;
		}
		default: break;
		}
		break;
	}
	case RadioLineNotPswf:
	{
		tx_address = 0x68;
		qmToBigEndian(value.guc_mode, tx_data + tx_data_len);
		++tx_data_len;
		break;
	}
    case ModemReceiver:
    {
   		tx_address = 0x6C;
    	switch (code) {
    	case ModemRxState:
    		qmToBigEndian((uint8_t)value.modem_rx_state, tx_data+tx_data_len);
    		tx_data_len += 1;
    		break;
    	case ModemRxBandwidth:
    		qmToBigEndian((uint8_t)value.modem_rx_bandwidth, tx_data+tx_data_len);
    		tx_data_len += 1;
    		break;
    	case ModemRxTimeSyncMode:
    		qmToBigEndian((uint8_t)value.modem_rx_time_sync_mode, tx_data+tx_data_len);
    		tx_data_len += 1;
    		break;
    	case ModemRxPhase:
    		qmToBigEndian((uint8_t)value.modem_rx_phase, tx_data+tx_data_len);
    		tx_data_len += 1;
    		break;
    	case ModemRxRole:
    		qmToBigEndian((uint8_t)value.modem_rx_role, tx_data+tx_data_len);
    		tx_data_len += 1;
    		break;
    	default: QM_ASSERT(0);
    	}
    	break;
    }

	default: QM_ASSERT(0);
	}


	transport->transmitFrame(tx_address, tx_data, tx_data_len);

}



void DspController::sendCommand(Module module, int code, ParameterValue value,bool state) {
		if (pending_command->in_progress) {
			qmDebugMessage(QmDebug::Dump, "new sendCommand(%d, %d) pushed to queue ", module, code);
			DspCommand cmd;
			cmd.module = module;
			cmd.code = code;
			cmd.value = value;
			cmd_queue->push_back(cmd);
	}else {
		qmDebugMessage(QmDebug::Dump, "sendCommand(%d, %d) transmiting", module, code);
		uint8_t tx_address;
		uint8_t tx_data[DspTransport::MAX_FRAME_DATA_SIZE];
		int tx_data_len = DEFAULT_PACKET_HEADER_LEN;
		qmToBigEndian((uint8_t)2, tx_data+0); // индикатор: "команда (установка)"
		qmToBigEndian((uint8_t)code, tx_data+1); // код параметра
		switch (module) {
		case RxRadiopath:
		case TxRadiopath: {
			if (module == RxRadiopath)
				tx_address = 0x50;
			else
				tx_address = 0x80;

			switch (code) {
			case 1:
				qmToBigEndian(value.frequency, tx_data+tx_data_len);
				tx_data_len += 4;
				break;
			case 2:
				qmToBigEndian((uint8_t)value.radio_mode, tx_data+tx_data_len);
				tx_data_len += 1;
				break;
			case 4:
				if (module == RxRadiopath) {
					qmToBigEndian((uint8_t)value.squelch, tx_data+tx_data_len);
					tx_data_len += 1;
				} else {
					qmToBigEndian((uint8_t)value.power, tx_data+tx_data_len);
					tx_data_len += 1;
				}
				break;
			case 7:
			case 8:
				qmToBigEndian((uint8_t)value.agc_mode, tx_data+tx_data_len);
				tx_data_len += 1;
				break;
			default: QM_ASSERT(0);
			}
			break;
		}
		case Audiopath: {
			tx_address = 0x90;
			switch (code) {
			case AudioModeParameter:
				qmToBigEndian((uint8_t)value.audio_mode, tx_data+tx_data_len);
				tx_data_len += 1;
				break;
			case AudioVolumeLevel:
				qmToBigEndian((uint8_t)value.volume_level, tx_data+tx_data_len);
				tx_data_len += 1;
				break;
			case AudioMicAmplify:
				qmToBigEndian((uint8_t)value.mic_amplify, tx_data+tx_data_len);
				tx_data_len += 1;
				break;
			default: QM_ASSERT(0);
			}
			break;
		}
		// для ПП� Ч
		case PSWFTransmitter: {
			QM_ASSERT(0);
			break;
		}

		case PSWFReceiver: {
			tx_address = 0x60;
			switch (code) {
			case PswfRxRAdr: {
				qmToBigEndian((uint8_t)value.pswf_r_adr, tx_data+tx_data_len);
				tx_data_len += 1;
				break;
			}
			case PswfRxFrequency:
			case PswfRxFreqSignal:
			{
				qmToBigEndian(value.frequency, tx_data+tx_data_len);
				tx_data_len += 4;
				if (ContentSms.stage == StageRx_data)
				{
					uint8_t fstn = calcFstn(ContentSms.R_ADR,ContentSms.S_ADR,ContentSms.RN_KEY,date_time[0],date_time[1],date_time[2],date_time[3],QNB_RX); // TODO: fix that;
					QNB_RX++;
					qmDebugMessage(QmDebug::Dump, "FSTN: %d", fstn);
					uint32_t abc = (fstn << 24);
					//qmToBigEndian(value.frequency, tx_data+tx_data_len);
					//tx_data_len += 4;
					qmToBigEndian(abc, tx_data+tx_data_len);
				    tx_data_len += 1;

				}
				break;
			}
			default: break;
			}
			break;
		}
        case RadioLineNotPswf:
        {
            tx_address = 0x68;
            qmToBigEndian(value.guc_mode, tx_data + tx_data_len);
            ++tx_data_len;
            break;
        }
        case ModemReceiver:
        {
       		tx_address = 0x6C;
        	switch (code) {
        	case ModemRxState:
        		qmToBigEndian((uint8_t)value.modem_rx_state, tx_data+tx_data_len);
        		tx_data_len += 1;
        		break;
        	case ModemRxBandwidth:
        		qmToBigEndian((uint8_t)value.modem_rx_bandwidth, tx_data+tx_data_len);
        		tx_data_len += 1;
        		break;
        	case ModemRxTimeSyncMode:
        		qmToBigEndian((uint8_t)value.modem_rx_time_sync_mode, tx_data+tx_data_len);
        		tx_data_len += 1;
        		break;
        	case ModemRxPhase:
        		qmToBigEndian((uint8_t)value.modem_rx_phase, tx_data+tx_data_len);
        		tx_data_len += 1;
        		break;
        	case ModemRxRole:
        		qmToBigEndian((uint8_t)value.modem_rx_role, tx_data+tx_data_len);
        		tx_data_len += 1;
        		break;
        	default: QM_ASSERT(0);
        	}
        	break;
        }

		default: QM_ASSERT(0);
		}

		QM_ASSERT(pending_command->in_progress == false);
		pending_command->in_progress = true;
		pending_command->sync_next = true;
		pending_command->module = module;
		pending_command->code = code;
		pending_command->value = value;
		transport->transmitFrame(tx_address, tx_data, tx_data_len);
		command_timer->start();
		}

}

void DspController::sendPswf(Module module) {

	qmDebugMessage(QmDebug::Dump, "sendPswf(%d)", module);

    ContentPSWF.Frequency = getFrequencyPswf();
    qmDebugMessage(QmDebug::Dump, "time:  %d, %d, %d,%d", date_time[3],date_time[2],date_time[1],date_time[0]);
    //qmDebugMessage(QmDebug::Dump, "data:  %d, %d, %d,%d", date_time[3],date_time[2],date_time[1],date_time[0]);
    qmDebugMessage(QmDebug::Dump, "freq:  %d", ContentPSWF.Frequency);
    qmDebugMessage(QmDebug::Dump, "LCode:  %d", ContentPSWF.L_CODE);

    ContentPSWF.indicator = 20;
    ContentPSWF.TYPE = 0;
    ContentPSWF.SNR =  9;

    uint8_t tx_address = 0x72;
    uint8_t tx_data[DspTransport::MAX_FRAME_DATA_SIZE];
    int tx_data_len = 0;
    qmToBigEndian((uint8_t)ContentPSWF.indicator, tx_data + tx_data_len);
    ++tx_data_len;
    qmToBigEndian((uint8_t)ContentPSWF.TYPE, tx_data + tx_data_len);
    ++tx_data_len;
    qmToBigEndian((uint32_t)ContentPSWF.Frequency, tx_data + tx_data_len);
    tx_data_len += 4;
    qmToBigEndian((uint8_t)ContentPSWF.SNR, tx_data+tx_data_len);
    ++tx_data_len;

    if (pswf_retranslator == 0)
    {
    	qmToBigEndian((uint8_t)ContentPSWF.R_ADR, tx_data+tx_data_len);
    	++tx_data_len;
    	qmToBigEndian((uint8_t)ContentPSWF.S_ADR, tx_data+tx_data_len);
    	++tx_data_len;
    	qmToBigEndian((uint8_t)ContentPSWF.COM_N, tx_data+tx_data_len);
    	++tx_data_len;
    	qmToBigEndian((uint8_t)ContentPSWF.L_CODE, tx_data+tx_data_len);
    	++tx_data_len;
    }

    else
    {
    	qmToBigEndian((uint8_t)ContentPSWF.RET_end_adr, tx_data+tx_data_len);
    	++tx_data_len;
    	qmToBigEndian((uint8_t)ContentPSWF.R_ADR, tx_data+tx_data_len);
    	++tx_data_len;
    	qmToBigEndian((uint8_t)ContentPSWF.COM_N, tx_data+tx_data_len);
    	++tx_data_len;
    	qmToBigEndian((uint8_t)ContentPSWF.L_CODE, tx_data+tx_data_len);
    	++tx_data_len;
    }


	//QM_ASSERT(pending_command->in_progress == false);
	pending_command->in_progress = true;
	pending_command->sync_next = true;
	pending_command->module = module;
//	pending_command->code = code;
//	pending_command->value = value;
    transport->transmitFrame(tx_address, tx_data, tx_data_len);
}

void DspController::sendGuc()
{
    qmDebugMessage(QmDebug::Dump, "sendGuc()");

    uint8_t tx_address = 0x7A;
    uint8_t tx_data[DspTransport::MAX_FRAME_DATA_SIZE];
    int tx_data_len = 0;

    qmToBigEndian((uint8_t)ContentGuc.indicator, tx_data + tx_data_len);
    ++tx_data_len;
    qmToBigEndian((uint8_t)ContentGuc.type, tx_data + tx_data_len);
    ++tx_data_len;

    uint8_t pack[5] = {0, 0, 0, 0, 0};
    pack[4] = (ContentGuc.S_ADR & 0x1F) << 3;
    pack[4] |= (ContentGuc.R_ADR & 0x1F) >> 2;
    pack[3] = (ContentGuc.R_ADR & 0x03) << 6;
    pack[3] |= (ContentGuc.NUM_com & 0x7F) >> 1;
    pack[2] = (ContentGuc.NUM_com & 0x01) << 7;
    pack[2] |= (ContentGuc.ckk & 0x3F) << 1;
    pack[2] |= ContentGuc.uin >> 7;
    pack[1] = (ContentGuc.uin & 0x7F) << 1;
    pack[1] |= ContentGuc.Coord & 0x01;


    for(int i = 4; i >= 0; --i) {
    	qmToBigEndian((uint8_t)pack[i], tx_data + tx_data_len);
    	++tx_data_len;
    }

    if (ContentGuc.NUM_com <= 5) ContentGuc.NUM_com = 5;
    if ((ContentGuc.NUM_com > 5) && (ContentGuc.NUM_com <= 11))  ContentGuc.NUM_com = 11;
    if ((ContentGuc.NUM_com > 11) && (ContentGuc.NUM_com <= 25)) 	ContentGuc.NUM_com = 25;
    if ((ContentGuc.NUM_com > 25) && (ContentGuc.NUM_com <= 100))  ContentGuc.NUM_com = 100;

    for(int i = 0; i < ContentGuc.NUM_com; i++) {
        qmToBigEndian((uint8_t)ContentGuc.command[i], tx_data + tx_data_len);
        ++tx_data_len;
    }

    for(int i = 0; i<ContentGuc.NUM_com;i++)
    {
    	int sdvig  = (i+1) % 8;
    	if (sdvig != 0)
    	ContentGuc.command[i] = (ContentGuc.command[i] << sdvig) + (ContentGuc.command[i+1] >> (7 -  sdvig));
    }


    uint32_t crc = pack_manager->CRC32(ContentGuc.command, ContentGuc.NUM_com);
    qmToBigEndian((uint32_t)crc, tx_data + tx_data_len);
    tx_data_len += 4;

     if (isGpsGuc)
     {
        uint8_t coord[13] = {0,0,0,0,0,0,0,0,0,0,0,0,0};
        getGpsGucCoordinat(coord);
        for(int i = 0;i<13;i++)
        {
            qmToBigEndian((uint8_t)ContentGuc.command[i], tx_data + tx_data_len);
            ++tx_data_len;
        }
     }

    transport->transmitFrame(tx_address, tx_data, tx_data_len);
}

void DspController::recGuc()
{
	srand(time(0));
    if (ContentGuc.stage == GucTxQuit){
        startGucRecieving();
        guc_timer->setInterval(GUC_TIMER_INTERVAL_REC);
        guc_timer->start();
    }
    if (ContentGuc.stage == GucRxQuit){
        startGucTransmitting();
        guc_timer->start();
    }
}

void DspController::processReceivedFrame(uint8_t address, uint8_t* data, int data_len) {
	if (data_len < DEFAULT_PACKET_HEADER_LEN)
		return;
	if (radio_state == radiostatePswfRxPrepare) {
		++pswfRxStateSync;
		qmDebugMessage(QmDebug::Dump, "processReceivedFrame() pswfRxStateSync = %d", pswfRxStateSync);
		if (pswfRxStateSync == 3) {
			radio_state = radiostatePswfRx;
			qmDebugMessage(QmDebug::Dump, "processReceivedFrame() radio_state = radiostatePswfRx");
		}
	}
	if (radio_state == radiostatePswfTxPrepare) {
		++pswfTxStateSync;
		qmDebugMessage(QmDebug::Dump, "processReceivedFrame() pswfTxStateSync = %d", pswfTxStateSync);
		if (pswfTxStateSync == 2) {
			radio_state = radiostatePswfTx;
			qmDebugMessage(QmDebug::Dump, "processReceivedFrame() radio_state = radiostatePswfTx");
		}
	}
    //----------sms-----------------------------------------------------
    if (radio_state == radiostateSmsRxPrepare) {
        ++smsRxStateSync;
        qmDebugMessage(QmDebug::Dump, "processReceivedFrame() smsRxStateSync = %d", smsRxStateSync);
        if (smsRxStateSync == 3) {
            radio_state = radiostateSmsRx;
            qmDebugMessage(QmDebug::Dump, "processReceivedFrame() radio_state = radiostateSmsfRx");
        }
    }
    if (radio_state == radiostateSmsTxPrepare) {
        ++smsTxStateSync;
        qmDebugMessage(QmDebug::Dump, "processReceivedFrame() smsTxStateSync = %d", smsTxStateSync);
        if (smsTxStateSync == 2) {
            radio_state = radiostateSmsTx;
            qmDebugMessage(QmDebug::Dump, "processReceivedFrame() radio_state = radiostateSmsTx");
        }
    }
    //-------------guc---------------------------------------------------

    if (radio_state == radiostateGucRxPrepare) {
        ++gucRxStateSync;
        qmDebugMessage(QmDebug::Dump, "processReceivedFrame() GucRxSync = %d", gucRxStateSync);
        if (gucRxStateSync == 2) {
            radio_state = radiostateGucRx;
            qmDebugMessage(QmDebug::Dump, "processReceivedFrame() radio_state = radiostateGucRx");
        }
    }
    if (radio_state == radiostateGucTxPrepare) {
        ++gucTxStateSync;
        qmDebugMessage(QmDebug::Dump, "processReceivedFrame() gucTxStateSync = %d", gucTxStateSync);
        if (gucTxStateSync == 4) {
            radio_state = radiostateGucTx;
            qmDebugMessage(QmDebug::Dump, "processReceivedFrame() radio_state = radiostateGucTx");
            pending_command->in_progress = false;
            trans_guc = 1;
        }
    }
   //---------------------------------------------------------------------

	uint8_t indicator = qmFromBigEndian<uint8_t>(data+0);
	uint8_t code = qmFromBigEndian<uint8_t>(data+1);
	uint8_t *value_ptr = data + 2;
	int value_len = data_len - 2;

	switch (address) {
	case 0x11: {
		if ((indicator == 5) && (code == 2) && (value_len == 6)) // инициативное сообщение с цифровой информацией о прошивке ?
			processStartup(qmFromBigEndian<uint16_t>(value_ptr+0), qmFromBigEndian<uint16_t>(value_ptr+2), qmFromBigEndian<uint16_t>(value_ptr+4));
		break;
	}
	case 0x51:
	case 0x81: {
		if ((indicator == 3) || (indicator == 4)) { // "команда выполнена", "команда не выполнена" ?
			ParameterValue value;
			if ((code == 1) && (value_len == 4)) {
				value.frequency = qmFromBigEndian<uint32_t>(value_ptr+0);
			} else if ((code == 2) && (value_len == 1)) {
                value.radio_mode = (RadioMode)qmFromBigEndian<uint8_t>(value_ptr+0);
			} else {
//				break;
			}
			Module module;
			if (address == 0x51)
				module = RxRadiopath;
			else
				module = TxRadiopath;
            if (code == 5)
                fwd_wave = qmFromBigEndian<uint32_t>(value_ptr+0);
            if (code == 6)
            {
                ref_wave = qmFromBigEndian<uint32_t>(value_ptr+0);
                if (fwd_wave > 0)
                {
                    swf_res = (fwd_wave+ref_wave)/(fwd_wave-ref_wave);
                }
            }
			processCommandResponse((indicator == 3), module, code, value);
		}
		break;
	}
	case 0x61: {
	if ((radio_state == radiostatePswfRxPrepare) || (radio_state == radiostateSmsRxPrepare))
		{
			qmDebugMessage(QmDebug::Dump, "processReceivedFrame() 0x61 received");
			ParameterValue value;
			processCommandResponse((indicator == 1), PSWFReceiver, code, value);
		}
		break;
	}
    case 0x63: {
    	qmDebugMessage(QmDebug::Dump, "0x63 received");
        if (indicator == 31) {
            qmDebugMessage(QmDebug::Dump, "0x63 indicator 31");
            if (ContentSms.stage == StageRx_call)
            {
                syncro_recieve.erase(syncro_recieve.begin());
                syncro_recieve.push_back(0);

                if (check_rx_call()) {
                	sms_call_received = true;
                	qmDebugMessage(QmDebug::Dump, "sms call received");
                }
            }
            if (ContentSms.stage == StageRx_data){
            	// todo: massive clear
               count_clear += 7;
               for(int i = count_clear - 7; i<count_clear;i++)
               rs_data_clear[i] = 1; //REVIEW: count_clear может быть 259? тогда выход за границы rs_data_clear
            }
        } else if (indicator == 30) {
        	qmDebugMessage(QmDebug::Dump, "0x63 indicator 30");
            if (ContentSms.stage > StageNone)
            {
                qmDebugMessage(QmDebug::Dump, "processReceivedFrame() data_len = %d", data_len);
                std::vector<uint8_t> sms_data;
                if (ContentSms.stage == StageRx_data)
                {  static int cnt = 0;
                    if (cnt  > 36) cnt = 0;
                    //ContentSms.R_ADR = data[8];
                    count_clear += 7; // todo:count clear
                    for(int i = 8; i < 15; i++) {
                        sms_data.push_back(data[i]);
                        qmDebugMessage(QmDebug::Dump, "data[%d] = %d", i, data[i]);
                    }
                    if (counterSms[StageRx_data] < 37)
                    recievedSmsBuffer.push_back(sms_data);
                }

                if (ContentSms.stage == StageTx_call_ack)
                {
                    tx_call_ask_vector.push_back(data[9]); // wzn response

                }
                if (ContentSms.stage == StageRx_call)
                {
                    syncro_recieve.erase(syncro_recieve.begin());
                    syncro_recieve.push_back(data[9]); // CYC_N
                    qmDebugMessage(QmDebug::Dump, "recieve frame() count = %d", syncro_recieve.size());

                    if (check_rx_call())
                    {
                        sms_call_received = true;
                        ContentSms.R_ADR = data[8]; // todo: check
                        qmDebugMessage(QmDebug::Dump, "sms call received");
                        syncro_recieve.clear();
                        for(int i = 0; i<18;i++)
                            syncro_recieve.push_back(0);
                    }
                }

                if (ContentSms.stage == StageTx_quit)
                {
                	getDataTime();
                	uint8_t ack_code_calc = calc_ack_code(data[9]);
                	if ((ack_code_calc == (data[10] + 1)) && (data[9] != 99))
                		 ++ok_quit;
                    quit_vector.push_back(data[9]);  // ack
                    quit_vector.push_back(data[10]); // ack code
                }
                pswf_first_packet_received = true;
            }
            else
            {
                std::vector<char> pswf_data;
                ContentPSWF.R_ADR =  data[7];
                ContentPSWF.S_ADR = data[8];
                pswf_data.push_back(data[9]);
                pswf_data.push_back(data[10]);
                recievedPswfBuffer.push_back(pswf_data);
                getDataTime();
                if (state_pswf == 1){
                	pswfQuitRec();
                	state_pswf = 0;
                }
                else
                RecievedPswf();
            }
        }
        break;
    }
    case 0x73: {
    	qmDebugMessage(QmDebug::Dump, "processReceivedFrame() 0x73 received, %d" ,indicator);
        ParameterValue value;
        value.frequency = 0;
        switch(radio_state)
        {
        case radiostateSmsTx:
            if (indicator == 22) {
                value.frequency = qmFromBigEndian<uint32_t>(value_ptr+0);
                qmDebugMessage(QmDebug::Dump, " frequency =  %d " ,value.frequency);
            }
            processCommandResponse((indicator == 24), PSWFTransmitter, code, value);
            break;
        case radiostateSmsRx:
            processCommandResponse((indicator == 1), PSWFTransmitter, code, value);
            break;
        case radiostatePswfTx:
        case radiostatePswfRx:
            processCommandResponse((indicator == 3), PSWFTransmitter, code, value);
            break;
        default:
        break;
        }

        break;
    }
    case 0x7B:{
        if (indicator == 22) {
            recGuc();
        }
        break;
    }
    case 0x6B:
    {
        qmDebugMessage(QmDebug::Dump, "processReceivedFrame() 0x6B received");
        if (ContentGuc.stage != GucNone){
        	if (indicator == 32){
                qmDebugMessage(QmDebug::Dump, "0x6B recieved frame: indicator %d", indicator);
        	}
            if (indicator == 30) {
            	ContentGuc.R_ADR = ((data[2] & 0xF0) << 2) + (data[3] & 0x3F);
            	qmDebugMessage(QmDebug::Dump, "0x6B R_ADR %d : ", ContentGuc.R_ADR);
                std::vector<uint8_t> guc;
                for(int i = 0;i<data_len;i++){
                    qmDebugMessage(QmDebug::Dump, "0x6B recieved frame: %d , num %d", data[i],i);
                    guc.push_back(data[i]); // по N едениц данных
                }
                if (guc_vector.size() < 50)     guc_vector.push_back(guc);
                if (ContentGuc.stage != GucTxQuit) ContentGuc.stage = GucRxQuit;
				recGuc();
            }
        }
        break;
    }
    case 0x6F:
    {
    	value_ptr -= 1; // костылное превращение в нестандартный формат кадра
    	value_len += 1; // костылное превращение в нестандартный формат кадра
    	switch (indicator) {
    	case 30: {
    		ModemPacketType type = (ModemPacketType)qmFromBigEndian<uint8_t>(value_ptr+1);
    		int data_offset;
    		if (type == modempacket_packHead)
    			data_offset = 5;
    		else
    			data_offset = 3;
    		uint8_t snr = (uint8_t)qmFromBigEndian<int8_t>(value_ptr+2);
    		ModemBandwidth bandwidth = (ModemBandwidth)qmFromBigEndian<uint8_t>(value_ptr+0);
    		receivedModemPacket.emit(type, snr, bandwidth, value_ptr + data_offset, value_len - data_offset);
    		break;
    	}
    	case 31: {
    		if (!(value_len >= 1))
    			break;
    		ModemPacketType type = (ModemPacketType)qmFromBigEndian<uint8_t>(value_ptr+0);
    		failedRxModemPacket.emit(type);
    		break;
    	}
    	case 32: {
    		ModemPacketType type = (ModemPacketType)qmFromBigEndian<uint8_t>(value_ptr+1);
    		int data_offset;
    		if (type == modempacket_packHead)
    			data_offset = 5;
    		else
    			data_offset = 3;
    		uint8_t snr = (uint8_t)qmFromBigEndian<int8_t>(value_ptr+2);
    		ModemBandwidth bandwidth = (ModemBandwidth)qmFromBigEndian<uint8_t>(value_ptr+0);
    		if (type == modempacket_packHead) {
    			uint8_t param_signForm = qmFromBigEndian<uint8_t>(value_ptr+3);
    			uint8_t param_packCode = qmFromBigEndian<uint8_t>(value_ptr+4);
        		startedRxModemPacket_packHead.emit(snr, bandwidth, param_signForm, param_packCode, value_ptr + data_offset, value_len - data_offset);
    		} else {
        		startedRxModemPacket.emit(type, snr, bandwidth, value_ptr + data_offset, value_len - data_offset);
    		}
    		break;
    	}
    	default:
    		break;
    	}
    	break;
    }
    case 0x7F:
    {
    	value_ptr -= 1; // костылное превращение в нестандартный формат кадра
    	value_len += 1; // костылное превращение в нестандартный формат кадра
    	switch (indicator) {
    	case 22: {
    		if (!(value_len >= 1))
    			break;
    		ModemPacketType type = (ModemPacketType)qmFromBigEndian<uint8_t>(value_ptr+0);
    		transmittedModemPacket.emit(type);
    		break;
    	}
    	case 23: {
    		if (!(value_len >= 1))
    			break;
    		failedTxModemPacket.emit();
    		break;
    	}
    	default:
    		break;
    	}
    	break;
    }

	default: break;
    }
	if (!cmd_queue->empty()) {
		DspCommand cmd;
		cmd = cmd_queue->front();
		cmd_queue->pop_front();
		sendCommand(cmd.module, cmd.code, cmd.value);
	}
}

// maybe two sides of pswf
// заглушка для сообещения о заполнении структуры
//void DspController::parsingData(){}

void *DspController::getContentPSWF()
{
    return &ContentPSWF;
}

void DspController::sendSms(Module module)
{
    ContentSms.Frequency =  getFrequencySms();
    ContentSms.indicator = 20;
    ContentSms.SNR =  7;

    if (ContentSms.stage == StageTx_data) {
    	ContentSms.TYPE = 1;
    } else {
    	ContentSms.TYPE = 0;
    }

    uint8_t tx_address = 0x72;
    uint8_t tx_data[DspTransport::MAX_FRAME_DATA_SIZE];
    int tx_data_len = 0;

    qmToBigEndian((uint8_t)ContentSms.indicator, tx_data + tx_data_len);
    ++tx_data_len;
    qmToBigEndian((uint8_t)ContentSms.TYPE, tx_data + tx_data_len);
    ++tx_data_len;
    qmToBigEndian((uint32_t)ContentSms.Frequency, tx_data + tx_data_len);
    tx_data_len += 4;

    for(int i = 0;i<6;i++)
    qmDebugMessage(QmDebug::Dump,"ContentSms.massive = %d",ContentSms.message[i]);

    qmDebugMessage(QmDebug::Dump, " ContentSms.Frequency =  %d " ,ContentSms.Frequency);
    // tx
    if (ContentSms.stage == StageTx_call)
    {
    	static int counter = 0;
    	if (counter == 18) counter = 0;
    	++counter;

    	qmToBigEndian((uint8_t)ContentSms.SNR, tx_data+tx_data_len);
    	++tx_data_len;


        qmToBigEndian((uint8_t)ContentSms.R_ADR, tx_data+tx_data_len);
    	++tx_data_len;
    	qmToBigEndian((uint8_t)ContentSms.S_ADR, tx_data+tx_data_len);
    	++tx_data_len;
    	qmToBigEndian((uint8_t)counter, tx_data+tx_data_len);
    	++tx_data_len;
    	qmToBigEndian((uint8_t)ContentSms.L_CODE, tx_data+tx_data_len);
    	++tx_data_len;
    }
    // tx
    if (ContentSms.stage == StageTx_data)
    {
        uint8_t FST_N =  calcFstn(ContentSms.R_ADR,ContentSms.S_ADR,ContentSms.RN_KEY,date_time[0],date_time[1],date_time[2],date_time[3],QNB);
        ++QNB;
        qmDebugMessage(QmDebug::Dump, "FSTN: %d", FST_N);
        if (cntChvc > 255) cntChvc = 7;
    	qmToBigEndian((uint8_t)ContentSms.SNR, tx_data+tx_data_len);
    	++tx_data_len;
    	qmToBigEndian((uint8_t)FST_N, tx_data+tx_data_len);
    	++tx_data_len;
    	for(int i = cntChvc - 7;i<cntChvc;i++)
    	{
    		qmToBigEndian(ContentSms.message[i], tx_data+tx_data_len);
    		++tx_data_len;
    		//qmDebugMessage(QmDebug::Dump, "MESSG: %d",ContentSms.message[i]);
    	}

    	cntChvc = cntChvc + 7;
    }
    //rx
    if (ContentSms.stage == StageRx_call_ack)
    {
        int wzn = wzn_value;

    	qmToBigEndian((uint8_t)ContentSms.SNR, tx_data+tx_data_len);
    	++tx_data_len;
    	qmToBigEndian((uint8_t)ContentSms.R_ADR, tx_data+tx_data_len);
    	++tx_data_len;
    	qmToBigEndian((uint8_t)ContentSms.S_ADR, tx_data+tx_data_len); // todo: поменял местами
    	++tx_data_len;
    	qmToBigEndian((uint8_t)wzn, tx_data+tx_data_len);
    	++tx_data_len;

    	qmDebugMessage(QmDebug::Dump, "SADR: %d",ContentSms.S_ADR);
    	qmDebugMessage(QmDebug::Dump, "RADR: %d",ContentSms.R_ADR);

    	qmToBigEndian((uint8_t)ContentSms.L_CODE, tx_data+tx_data_len);
    	++tx_data_len;
    }

    if (ContentSms.stage == StageRx_quit)
    {
    	qmToBigEndian((uint8_t)ContentSms.SNR, tx_data+tx_data_len);
    	++tx_data_len;
    	qmToBigEndian((uint8_t)ContentSms.R_ADR, tx_data+tx_data_len);
    	++tx_data_len;
    	qmToBigEndian((uint8_t)ContentSms.S_ADR, tx_data+tx_data_len);
    	++tx_data_len;
    	qmToBigEndian((uint8_t)ack, tx_data+tx_data_len);
    	++tx_data_len;

    	uint8_t ack_code  = calc_ack_code(ack);
    	qmToBigEndian((uint8_t)ack_code, tx_data+tx_data_len);
    	++tx_data_len;

    }

    transport->transmitFrame(tx_address, tx_data, tx_data_len);
}

void DspController::recSms()
{
    qmDebugMessage(QmDebug::Dump, "recSms() ContentSms.stage =  %d", ContentSms.stage);
    if (ContentSms.stage == StageTx_call_ack)
    {
        qmDebugMessage(QmDebug::Dump, "recSms() counterSms[StageTx_rec] =  %d", counterSms[StageTx_call_ack]);
        if (counterSms[StageTx_call_ack] == 1)
        {
            qmDebugMessage(QmDebug::Dump, "recSms() recievedSmsBuffer.size() =  %d", recievedSmsBuffer.size());
            if (tx_call_ask_vector.size() >= 3) {
                wzn_value = wzn_change(tx_call_ask_vector);
                qmDebugMessage(QmDebug::Dump, "wzn_value" ,wzn_value);
                startSMSTransmitting(ContentSms.R_ADR, ContentSms.message, StageTx_data);
                qmDebugMessage(QmDebug::Dump, "start stage Data TX");
                updateSmsStatus(getSmsForUiStage());
            } else {
                qmDebugMessage(QmDebug::Dump, "recSms() smsFailed, radio_state = radiostateSync");
                smsFailed(0);
                radio_state = radiostateSync;
                ContentSms.stage = StageNone;
            }
            counterSms[StageTx_call_ack] = 18;
            pswf_first_packet_received = false;
        }
        else
        {
            counterSms[StageTx_call_ack] = counterSms[StageTx_call_ack] - 1;
        }
    }

    if (ContentSms.stage == StageTx_quit)
    {
        qmDebugMessage(QmDebug::Dump, "recSms() counterSms[StageTx_quit] =  %d", counterSms[StageTx_quit]);
        if (counterSms[StageTx_quit] == 0)
        {
            qmDebugMessage(QmDebug::Dump, "recSms() recievedSmsBuffer.size() =  %d", recievedSmsBuffer.size());
            if (quit_vector.size() >= 2) {
            	radio_state = radiostateSync;
                qmDebugMessage(QmDebug::Dump, "recSms() SMS transmitting successfully finished");
                 updateSmsStatus(100);
                if (ok_quit >= 2)
                {
                	smsFailed(-1);
                	if (getSmsRetranslation() != 0){
                		startSMSRecieving();
                	}
                }
                else
                    smsFailed(0);

            } else {
                qmDebugMessage(QmDebug::Dump, "recSms() smsFailed, radio_state = radiostateSync");
                smsFailed(1);
            }
            radio_state = radiostateSync;
            ContentSms.stage = StageNone;
            counterSms[StageTx_quit] = 6;
            pswf_first_packet_received = false;
        }
        else
        {
            counterSms[StageTx_quit] = counterSms[StageTx_quit] - 1;
        }
    }

    if (ContentSms.stage == StageRx_call)
    {
    	if (sms_call_received) {
    		syncro_recieve.clear();
    		for(int i = 0; i < 18; i++)
    		syncro_recieve.push_back(0);
    		sms_call_received = false;
    		qmDebugMessage(QmDebug::Dump, "recSms() sms call received");
    		startSMSCmdTransmitting(StageRx_call_ack);
            updateSmsStatus(getSmsForUiStage());
    		pswf_first_packet_received = false;
    		counterSms[StageRx_call] = 18;
    	}
    }

    if (ContentSms.stage == StageRx_data) {
        qmDebugMessage(QmDebug::Dump, "recSms() counterSms[StageRx_data] =  %d", counterSms[StageRx_data]);
        if (counterSms[StageRx_data] == 0)
        {
            qmDebugMessage(QmDebug::Dump, "recSms() recievedSmsBuffer.size() =  %d", recievedSmsBuffer.size());
            if (recievedSmsBuffer.size() > 10) { //TODO:
                updateSmsStatus(getSmsForUiStage());
                startSMSCmdTransmitting(StageRx_quit);
                generateSmsReceived(); //TODO:
            } else {
                qmDebugMessage(QmDebug::Dump, "recSms() smsFailed, radio_state = radiostateSync");
                smsFailed(0);
                radio_state = radiostateSync;
                ContentSms.stage = StageNone;
            }
            counterSms[StageRx_data] = 37;
            pswf_first_packet_received = false;
        }
        else
        {
            counterSms[StageRx_data] = counterSms[StageRx_data] - 1;
        }
    }

}

void DspController::generateSmsReceived()
{
	int count = 0;
	int data[255];
	for(int i = 0;i<255;i++) data[i] = 0;
	uint8_t packed[100];

	for(int j = 0;j<=recievedSmsBuffer.size()-1;j++){
		for(int i = 0; i<7;i++)
		{
			if (count < 255){
				data[count] = recievedSmsBuffer.at(j).at(i);
				++count;
			}
		}
	}

    int temp = eras_dec_rs(data,rs_data_clear,&rs_255_93);

    uint8_t crc_chk[100];
    uint8_t calcs_crc[100];
    for(int i = 0;i<89;i++) calcs_crc[i] = data[i];
    uint32_t crc_calc = pack_manager->CRC32(calcs_crc,89);


	for(int i = 0;i<100;i++) packed[i] = 0;
    for(int i = 0;i<87;i++) crc_chk[i] = data[i];

    pack_manager->decompressMass(crc_chk,87,packed,100,7); // todo: not work

    pack_manager->to_Win1251(packed); //test


    std::string str;
    str.push_back(packed[0]);
    for(int i = 1; i<100;i++){
    	str.push_back(packed[i]);
    	if (str[i-1] == ' ' && str[i] ==' ')
    		str.pop_back();
    }

    if (str.length() > 0)
    for(int i = 0; i<str.length();i++){
    	packed[i] = str[i];
    }

   str.push_back('\0');

	recievedSmsBuffer.clear();

	uint32_t crc_packet = 0;
	int k = 3;
	while(k >=0){
		crc_packet += (data[89+k] & 0xFF) << (8*k);
		k--;
	}

	if (crc_packet != crc_calc)
	{
		smsFailed(3);
		ack = 99;
	}
	else
	{
		ack = 73;
		for(int i = 0; i < 99; i++) sms_content[i] = str[i];
		sms_content[99] = '\0'; //REVIEW: начиная c sms_content[90] по sms_content[98] будет мусор
        qmDebugMessage(QmDebug::Dump, "generateSmsReceived() sms_content = %s", sms_content);
		smsPacketMessage();
    }
}

int DspController::wzn_change(std::vector<int> &vect)
{
    int wzn_mas[5] = {0,0,0,0,0};
    for(int i = 0; i<vect.size()-1;i++){
        if (vect[i] <= 4) wzn_mas[vect[i]] += 1;
    }
    int index = 0;
    for(int i = 0; i<4;i++)
    {
        if (wzn_mas[i] > wzn_mas[i+1]) index = i + 1;
        else index = i;
    }
    return wzn_mas[index];
}

int DspController::calcFstn(int R_ADR, int S_ADR, int RN_KEY, int SEC, int MIN, int HRS, int DAY, int QNB)
{
    int FST_N = (R_ADR + S_ADR + RN_KEY + SEC + MIN + HRS + DAY + QNB) % 100;
    return FST_N;
}

int DspController::check_rx_call()
{
    int cnt_index = 0;
    for(int i = 0; i<18;i++)
    {
       if (syncro_recieve.at(i) == i)
           ++cnt_index;
       qmDebugMessage(QmDebug::Dump, "syncro_recieve value = %d", syncro_recieve.at(i));
    }

    qmDebugMessage(QmDebug::Dump, "check rx call = %d", cnt_index);

    if (cnt_index>=3) return true;

    return false;
}

uint8_t DspController::calc_ack_code(uint8_t ack)
{
    uint8_t ACK_CODE = (ContentSms.R_ADR + ContentSms.S_ADR + ack + ContentSms.RN_KEY +
                    date_time[0] + date_time[1]+ date_time[2] + date_time[3]) % 100;
    return ACK_CODE;
}

char* DspController::getSmsContent()
{
	return sms_content;
}

void DspController::startPSWFReceiving(bool ack) {
	qmDebugMessage(QmDebug::Dump, "startPSWFReceiving(%d)", ack);
	QM_ASSERT(is_ready);
	if (!resyncPendingCommand())
		return;

    if (recievedPswfBuffer.size() > 0)
    recievedPswfBuffer.erase(recievedPswfBuffer.begin()); // TODO : test this

	pswf_ack = ack;
	getDataTime();
    ContentPSWF.Frequency = getFrequencyPswf();

	ParameterValue param;
	param.pswf_r_adr = PSWF_SELF_ADR;
	sendCommand(PSWFReceiver, PswfRxRAdr, param);

	ParameterValue comandValue;
	comandValue.radio_mode = RadioModeOff;
	sendCommand(TxRadiopath, TxRadioMode, comandValue);
	comandValue.pswf_indicator = RadioModePSWF;
	sendCommand(RxRadiopath, RxRadioMode, comandValue);
	pswfRxStateSync = 0;
	radio_state = radiostatePswfRxPrepare;
	pswf_rec = 0;
}

void DspController::startPSWFTransmitting(bool ack, uint8_t r_adr, uint8_t cmd,int retr) {
	qmDebugMessage(QmDebug::Dump, "startPSWFTransmitting(%d, %d, %d)", ack, r_adr, cmd);
    QM_ASSERT(is_ready);
    if (!resyncPendingCommand())
        return;

    pswf_retranslator = retr;

    ContentPSWF.RET_end_adr = retr;

    pswf_ack = ack;
    getDataTime();
    ContentPSWF.Frequency = getFrequencyPswf();

    ContentPSWF.indicator = 20;
    ContentPSWF.TYPE = 0;
    ContentPSWF.COM_N = cmd;
    ContentPSWF.R_ADR = r_adr;
    if (pswf_retranslator > 0) ContentPSWF.R_ADR += 32;
    ContentPSWF.S_ADR = PSWF_SELF_ADR;

    ParameterValue comandValue;
    comandValue.radio_mode = RadioModeOff;
    sendCommandEasy(RxRadiopath, RxRadioMode, comandValue);
    comandValue.pswf_indicator = RadioModePSWF;
    sendCommandEasy(TxRadiopath, TxRadioMode, comandValue);
    pswfTxStateSync = 0;
    radio_state = radiostatePswfTxPrepare;
}

void DspController::startSMSRecieving(SmsStage stage)
{
    qmDebugMessage(QmDebug::Dump, "startSmsReceiving");
    QM_ASSERT(is_ready);

    getDataTime();
    ContentSms.Frequency = getFrequencySms();

    for(int i = 0; i<255; i++) rs_data_clear[i] = 0;

    tx_call_ask_vector.erase(tx_call_ask_vector.begin(),tx_call_ask_vector.end());
    quit_vector.erase(quit_vector.begin(),quit_vector.end());

    ParameterValue param;
    param.pswf_r_adr = PSWF_SELF_ADR;
    sendCommand(PSWFReceiver, PswfRxRAdr, param);

    ParameterValue comandValue;
    if ((stage == StageRx_data) || (ContentSms.stage == StageTx_quit)){
    	comandValue.radio_mode = RadioModeOff;
    	    sendCommandEasy(TxRadiopath, TxRadioMode, comandValue);
    	    comandValue.pswf_indicator = RadioModePSWF;
    	    sendCommandEasy(RxRadiopath, RxRadioMode, comandValue);
    }else{
    comandValue.radio_mode = RadioModeOff;
    sendCommand(TxRadiopath, TxRadioMode, comandValue);
    comandValue.pswf_indicator = RadioModePSWF;
    sendCommand(RxRadiopath, RxRadioMode, comandValue);}
    smsRxStateSync = 0;
    radio_state = radiostateSmsRxPrepare;
    ContentSms.stage = stage;

}


void DspController::defaultSMSTransmit()
{
	for(int i = 0; i<255;i++)  ContentSms.message[i] = 0;
	//ContentSms.stage = StageNone;
}

void DspController::startSMSTransmitting(uint8_t r_adr,uint8_t* message, SmsStage stage)
{
    qmDebugMessage(QmDebug::Dump, "SMS tranmit (%d, %s)",r_adr, message);
    QM_ASSERT(is_ready);

    getDataTime();
    ContentSms.Frequency =  getFrequencySms();
    ContentSms.indicator = 20;
    ContentSms.TYPE = 0;
    ContentSms.R_ADR = r_adr;
    ContentSms.CYC_N = 0;

    count_clear = 0;

    cntChvc = 7;

    int ind = strlen((const char*)message);

    int data_sms[255];

    if (ContentSms.stage == StageNone)
    {
    	for(int i = 0; i<ind;i++) ContentSms.message[i] = message[i];

    	pack_manager->to_Koi7(ContentSms.message); // test

    	pack_manager->compressMass(ContentSms.message,87,7); //test

        ContentSms.message[87] = ContentSms.message[87] & 0x0F; //set 4 most significant bits to 0

        ContentSms.message[88] = 0;

        uint8_t ret = getSmsRetranslation();
        if (ret != 0){
            ContentSms.message[87] = ContentSms.message[87] | (ret  << 4);
            ContentSms.message[88] = ContentSms.message[88] | ((ret >> 4) & 0x3);
        }

        uint32_t abc = pack_manager->CRC32(ContentSms.message,89);

        for(int i = 0;i<4;i++) ContentSms.message[89+i] = (uint8_t)((abc >> (8*i)) & 0xFF);
    	for(int i = 0;i<255;i++) rs_data_clear[i] = 0;
    	for(int i = 0; i<255;i++) data_sms[i] = (int)ContentSms.message[i];

        encode_rs(data_sms,&data_sms[93],&rs_255_93);
    	for(int i = 0; i<255;i++)ContentSms.message[i]  = data_sms[i];

    }


    ContentSms.R_ADR = r_adr;
    ContentSms.S_ADR = PSWF_SELF_ADR;


    ParameterValue comandValue;
    if (stage == StageTx_data)
    {
    	comandValue.radio_mode = RadioModeOff;
    	sendCommandEasy(RxRadiopath, RxRadioMode, comandValue);
    	comandValue.pswf_indicator = RadioModePSWF;
    	sendCommandEasy(TxRadiopath, TxRadioMode, comandValue);
    }else
    {
    comandValue.radio_mode = RadioModeOff;
    sendCommand(RxRadiopath, RxRadioMode, comandValue);
    comandValue.pswf_indicator = RadioModePSWF;
    sendCommand(TxRadiopath, TxRadioMode, comandValue);}
    smsTxStateSync = 0;
    radio_state = radiostateSmsTxPrepare;
    ContentSms.stage = stage;

    updateSmsStatus(getSmsForUiStage());
}

void DspController::startGucTransmitting(int r_adr, int speed_tx, std::vector<int> command)
{
    qmDebugMessage(QmDebug::Dump, "startGucTransmitting(%i, %i, 0x%X)", r_adr, speed_tx);
    QM_ASSERT(is_ready);

    ContentGuc.indicator = 20;
    ContentGuc.type = 1;
    ContentGuc.chip_time = 2;
    ContentGuc.WIDTH_SIGNAL = 1;
    ContentGuc.S_ADR = SAZHEN_NETWORK_ADDRESS;
    ContentGuc.R_ADR = r_adr;

    uint8_t num_cmd = command.size();
    ContentGuc.NUM_com = num_cmd;


    for(int i = 0;i<100;i++) ContentGuc.command[i] = 0;
    for(int i = 0;i<num_cmd; i++)
    ContentGuc.command[i] = command[i];

    ContentGuc.ckk = 0;
    ContentGuc.ckk |= (1 & 0x01);
    ContentGuc.ckk |= (ContentGuc.WIDTH_SIGNAL & 0x01) << 1;
    ContentGuc.ckk |= (1 & 0x03) << 2;
    ContentGuc.ckk |= (ContentGuc.chip_time & 0x03) << 4;

    ContentGuc.uin = 0;
    ContentGuc.Coord = 0;

    ParameterValue comandValue;
    comandValue.radio_mode = RadioModeOff;// отключили прием
    sendCommandEasy(RxRadiopath, RxRadioMode, comandValue);
    comandValue.guc_mode = RadioModeSazhenData; // включили 11 режим
    sendCommandEasy(TxRadiopath, TxRadioMode, comandValue);
    comandValue.frequency =  freqGucValue;//3000000;
    sendCommandEasy(RxRadiopath, RxFrequency, comandValue);
    sendCommandEasy(TxRadiopath, TxFrequency, comandValue);
    radio_state = radiostateGucTxPrepare;
    gucTxStateSync = 0;
    ContentGuc.stage =  GucTx;

    command.clear();
}


void DspController::setFreq(int value){
    freqGucValue  = value;
}

int DspController::getSmsForUiStage()
{
    switch (ContentSms.stage)
    {
    case StageNone:
        return 0;
    case StageTx_call:
    case StageRx_call:
        return 10;
    case StageRx_call_ack:
    case StageTx_call_ack:
        return 25;
    case StageTx_data:
    case StageRx_data:
        return 45;
    case StageTx_quit:
    case StageRx_quit:
        return 90;
    default: return 0;
    }
}

void DspController::startGucTransmitting()
{
    qmDebugMessage(QmDebug::Dump, "startGucTransmitting");
    QM_ASSERT(is_ready);

    ParameterValue comandValue;
    comandValue.radio_mode = RadioModeOff;// отключили прием
    sendCommand(RxRadiopath, RxRadioMode, comandValue);
    comandValue.guc_mode = RadioModeSazhenData; // включили 11 режим
    sendCommandEasy(TxRadiopath, TxRadioMode, comandValue);
    comandValue.frequency = freqGucValue;//3000000;
    sendCommandEasy(RxRadiopath, RxFrequency, comandValue);
    sendCommandEasy(TxRadiopath, TxFrequency, comandValue);
    radio_state = radiostateGucTxPrepare;
    gucTxStateSync = 0;
    ContentGuc.stage =  GucRxQuit;
}

void DspController::sendGucQuit()
{
	qmDebugMessage(QmDebug::Dump, "sendGucQuit");

	uint8_t tx_address = 0x7A;
	uint8_t tx_data[DspTransport::MAX_FRAME_DATA_SIZE];
	int tx_data_len = 0;

	ContentGuc.indicator = 20;
	ContentGuc.type = 4;
	ContentGuc.chip_time = 2;
	ContentGuc.WIDTH_SIGNAL = 1;
	ContentGuc.S_ADR = SAZHEN_NETWORK_ADDRESS;
	//ContentGuc.R_ADR = 1;

	ContentGuc.ckk = 0;
	ContentGuc.ckk |= (1 & 0x01);
	ContentGuc.ckk |= (ContentGuc.WIDTH_SIGNAL & 0x01) << 1;
	ContentGuc.ckk |= (1 & 0x03) << 2;
	ContentGuc.ckk |= (ContentGuc.chip_time & 0x03) << 4;

	ContentGuc.uin = 0;

	qmToBigEndian((uint8_t)ContentGuc.indicator, tx_data + tx_data_len);
	++tx_data_len;
	qmToBigEndian((uint8_t)ContentGuc.type, tx_data + tx_data_len);
	++tx_data_len;


	uint8_t pack[3] = {0, 0, 0};
	pack[2] = (ContentGuc.S_ADR & 0x1F) << 3;
	pack[2] |= (ContentGuc.R_ADR & 0x1F) >> 2;
	pack[1] |= (ContentGuc.ckk & 0x3F);
	pack[0] = ContentGuc.uin;

    for(int i = 2; i >= 0; --i) {
    	qmToBigEndian((uint8_t)pack[i], tx_data + tx_data_len);
    	++tx_data_len;
    }

    transport->transmitFrame(tx_address, tx_data, tx_data_len);
}

uint8_t *DspController::getGpsGucCoordinat(uint8_t *coord)
{

    Navigation::Coord_Date date = navigator->getCoordDate();
    std::string lon((const char*)date.longitude);
    std::string lat((const char*)date.latitude);

    coord[0] = (uint8_t)atoi(lon.substr(0,2).c_str());
    coord[1] = (uint8_t)atoi(lon.substr(2,2).c_str());
    coord[2] = (uint8_t)atoi(lon.substr(5,2).c_str());
    coord[3] = (uint8_t)atoi(lon.substr(7,2).c_str());
    coord[4] = (uint8_t)atoi(lat.substr(0,3).c_str());
    coord[5] = (uint8_t)atoi(lat.substr(3,2).c_str());
    coord[6] = (uint8_t)atoi(lat.substr(6,2).c_str());
    coord[7] = (uint8_t)atoi(lat.substr(8,2).c_str());

    if ((strstr((const char*)date.longitude[0],"N") !=0) && strstr((const char*)date.latitude[0],"E") !=0)
        coord[8] = 0;
    if ((strstr((const char*)date.longitude[0],"S") !=0) && strstr((const char*)date.latitude[0],"E") !=0)
        coord[8] = 1;
    if ((strstr((const char*)date.longitude,"S") !=0) && strstr((const char*)date.latitude[0],"W") !=0)
        coord[8] = 2;
    if ((strstr((const char*)date.longitude,"N") !=0) && strstr((const char*)date.latitude[0],"W") !=0)
        coord[8] = 3;

    uint32_t crc = pack_manager->CRC32(coord,9);
    for(int i = 0; i<4;i++){coord[i+9] = (uint8_t)((crc >> (8*i)) & 0xFF);}
    return coord;
}

uint8_t *DspController::returnGpsCoordinat(uint8_t *data,uint8_t* res,int index)
{
    std::string lon1;
    std::string lat1;

    memcpy(res,&data[index],9);
    std::string str((const char*)res);

    lon1 = str.substr(0,2).append(",").append(str.substr(2,4));
    lat1 = str.substr(4,6).append(",").append(str.substr(6,8));

    cordGucValue.lat = lat1;
    cordGucValue.lon = lon1;

    updateGucGpsStatus(cordGucValue);
}

void DspController::setSmsRetranslation(uint8_t retr)
{
    sms_retranslation = retr;
}

uint8_t DspController::getSmsRetranslation()
{
    return sms_retranslation;
}

void DspController::startGucRecieving()
{
    qmDebugMessage(QmDebug::Dump, "startGucRecieving");
    QM_ASSERT(is_ready);

    ParameterValue comandValue;
    comandValue.radio_mode = RadioModeOff;// отключили радиорежим
    sendCommandEasy(TxRadiopath, TxRadioMode, comandValue);
    comandValue.guc_mode = 3;
    sendCommandEasy(RadioLineNotPswf, 0 ,comandValue);
    comandValue.guc_mode = SAZHEN_NETWORK_ADDRESS;
    sendCommandEasy(RadioLineNotPswf, 3 ,comandValue); // отключить низкоскоростной модем
    comandValue.guc_mode = RadioModeSazhenData; // включили 11 режим
    sendCommandEasy(RxRadiopath, RxRadioMode, comandValue);
    comandValue.frequency = freqGucValue;//3000000;
    sendCommandEasy(RxRadiopath, RxFrequency, comandValue);

    radio_state = radiostateGucRxPrepare;
    gucRxStateSync = 0;
    if (ContentGuc.stage != GucTxQuit)
    	ContentGuc.stage =  GucRx;
}

void DspController::GucSwichRxTxAndViewData()
{
    guc_timer->setInterval(GUC_TIMER_INTERVAL); // TODO: возможно изменение интервала
	int size = 0;
    if (guc_vector.size() > 50) size = 50;
    else size  = guc_vector.size();

    qmDebugMessage(QmDebug::Dump, "size guc command %d", guc_vector.size());

    if (size > 0){
        recievedGucResp();
        if (ContentGuc.stage != GucTxQuit)
        startGucTransmitting();
        sendGucQuit();
    }
    else {radio_state = radiostateSync;}
    guc_vector.clear();
}

uint8_t* DspController::get_guc_vector()
{
    int num = (guc_vector.at(0).at(3) & 0x3f) << 1;
    num +=    (guc_vector.at(0).at(4) & 0x80) >> 7;

    //REVIEW: вставить assert для контроля num, чтобы не выйти за границы guc_text
    //QM_ASSERT(num < guc_text size);
    guc_text[0] = num;
    for(int i = 0; i<num;i++){
        guc_text[i+1] = guc_vector.at(0).at(7+i);
    }

    uint8_t out[100];
    for(int i = 0; i<100;i++) out[i] = 0;

    for(int i = 0;  i< num;i++)
    {
        int sdvig  = (i+1) % 8;
        if (sdvig != 0)
            out[i] = (guc_text[i+1] << sdvig) + (guc_text[i+2] >> (7 -  sdvig));
        else
            out[i] = guc_text[i+1];
    }

    int m = 3;
    uint32_t crc_packet = 0;
    int l = 0;
    while(m >=0){
        uint8_t sum = guc_vector.at(0).at(guc_vector.at(0).size() - 1 - m);
        crc_packet += sum << (8*m);
        l++;
        m--;
    }

	for(int i = 0;  i< num;i++)
	{
		int sdvig  = (i+1) % 8;
		if (sdvig != 0)
			out[i] = (guc_text[i+1] << sdvig) + (guc_text[i+2] >> (7 -  sdvig));
		else
			out[i] = guc_text[i+1];
	}


    int count = 0;
    if (num <= 5) count = 5;
    if ((num > 5) && (num <= 11))   count = 11;
    if ((num > 11) && (num <= 25)) count = 25;
    if ((num > 25) && (num <= 100))  count = 100;

    uint32_t crc = 0;
    crc = pack_manager->CRC32(out,count);

    if (crc != crc_packet) //TODO: crc check pro
    {
        qmDebugMessage(QmDebug::Dump, "Crc failded for guc vector %i, %i", crc_packet);
    }

    guc_vector.clear();

    return guc_text;
}


void DspController::startSMSCmdTransmitting(SmsStage stage)
{
    qmDebugMessage(QmDebug::Dump, "startSMSCmdTransmitting(%d)", stage);
    QM_ASSERT(is_ready);

    getDataTime();
    ContentSms.Frequency =   getFrequencySms();
    ContentSms.indicator = 20;
    ContentSms.TYPE = 0;

    ParameterValue comandValue;
    if (stage == StageRx_call_ack){
    	comandValue.radio_mode = RadioModeOff;
    	sendCommandEasy(RxRadiopath, RxRadioMode, comandValue);
    	comandValue.pswf_indicator = RadioModePSWF;
    	sendCommandEasy(TxRadiopath, TxRadioMode, comandValue);
    } else{
    	comandValue.radio_mode = RadioModeOff;
    	sendCommand(RxRadiopath, RxRadioMode, comandValue);
    	comandValue.pswf_indicator = RadioModePSWF;
    	sendCommand(TxRadiopath, TxRadioMode, comandValue);
    }
    smsTxStateSync = 0;
    radio_state = radiostateSmsTxPrepare;
    ContentSms.stage = stage;
}

void DspController::tuneModemFrequency(uint32_t value) {
	ParameterValue comandValue;
	if (value >= 30000000)
		comandValue.power = 80;
	else
		comandValue.power = 100;
	sendCommandEasy(TxRadiopath, TxPower, comandValue);
	comandValue.frequency = value;
	sendCommandEasy(RxRadiopath, RxFrequency, comandValue);
	sendCommandEasy(TxRadiopath, TxFrequency, comandValue);
}

void DspController::enableModemReceiver() {
	disableModemTransmitter();
	current_radio_mode = RadioModeSazhenData;
	ParameterValue comandValue;
	comandValue.radio_mode = RadioModeSazhenData;
	sendCommandEasy(RxRadiopath, RxRadioMode, comandValue);
	comandValue.modem_rx_state = ModemRxDetectingStart;
	sendCommandEasy(ModemReceiver, ModemRxState, comandValue);
	modem_rx_on = true;
}

void DspController::disableModemReceiver() {
	if (!modem_rx_on)
		return;
	modem_rx_on = false;
	ParameterValue comandValue;
	comandValue.modem_rx_state = ModemRxOff;
	sendCommandEasy(ModemReceiver, ModemRxState, comandValue);
	comandValue.radio_mode = RadioModeOff;
	sendCommandEasy(RxRadiopath, RxRadioMode, comandValue);
	current_radio_mode = RadioModeOff;
}

void DspController::setModemReceiverBandwidth(ModemBandwidth value) {
	ParameterValue comandValue;
	comandValue.modem_rx_bandwidth = value;
	sendCommandEasy(ModemReceiver, ModemRxBandwidth, comandValue);
}

void DspController::setModemReceiverTimeSyncMode(ModemTimeSyncMode value) {
	ParameterValue comandValue;
	comandValue.modem_rx_time_sync_mode = value;
	sendCommandEasy(ModemReceiver, ModemRxTimeSyncMode, comandValue);
}

void DspController::setModemReceiverPhase(ModemPhase value) {
	ParameterValue comandValue;
	comandValue.modem_rx_phase = value;
	sendCommandEasy(ModemReceiver, ModemRxPhase, comandValue);
}

void DspController::setModemReceiverRole(ModemRole value) {
	ParameterValue comandValue;
	comandValue.modem_rx_role = value;
	sendCommandEasy(ModemReceiver, ModemRxRole, comandValue);
}

void DspController::enableModemTransmitter() {
	disableModemReceiver();
	current_radio_mode = RadioModeSazhenData;
	ParameterValue comandValue;
	comandValue.radio_mode = RadioModeSazhenData;
	sendCommandEasy(TxRadiopath, TxRadioMode, comandValue);
	modem_tx_on = true;
}

void DspController::disableModemTransmitter() {
	if (!modem_tx_on)
		return;
	modem_tx_on = false;
	ParameterValue comandValue;
	comandValue.radio_mode = RadioModeOff;
	sendCommandEasy(TxRadiopath, TxRadioMode, comandValue);
	current_radio_mode = RadioModeOff;
}

void DspController::sendModemPacket(ModemPacketType type,
		ModemBandwidth bandwidth, const uint8_t *data, int data_len) {
	QM_ASSERT(type != modempacket_packHead);
	std::vector<uint8_t> payload(4);
	payload[0] = 20;
	payload[1] = bandwidth;
	payload[2] = type;
	payload[3] = 0;
	if (data_len > 0) {
		QM_ASSERT(data);
		payload.insert(std::end(payload), data, data + data_len);
	}
	transport->transmitFrame(0x7E, &payload[0], payload.size());
}

void DspController::sendModemPacket_packHead(ModemBandwidth bandwidth,
		uint8_t param_signForm, uint8_t param_packCode,
		const uint8_t *data, int data_len) {
	std::vector<uint8_t> payload(6);
	payload[0] = 20;
	payload[1] = bandwidth;
	payload[2] = 22;
	payload[3] = 0;
	payload[4] = param_signForm;
	payload[5] = param_packCode;
	QM_ASSERT(data);
	QM_ASSERT(data_len > 0);
	payload.insert(std::end(payload), data, data + data_len);
	transport->transmitFrame(0x7E, &payload[0], payload.size());
}


} /* namespace Multiradio */

#include "qmdebug_domains_start.h"
QMDEBUG_DEFINE_DOMAIN(dspcontroller, LevelDefault)
#include "qmdebug_domains_end.h"
