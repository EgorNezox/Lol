/**
 ******************************************************************************
 * @file    dspcontroller.cpp
 * @author  Artem Pisarenko, PMR dept. software team, ONIIP, PJSC
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



#define PSWF_SELF_ADR	1

#define DEFAULT_PACKET_HEADER_LEN	2 // индикатор кадра + код параметра ("адрес" на самом деле не входит сюда, это "адрес назначения" из канального уровня)



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
	command_timer->setInterval(50);
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
//    guc_timer->setInterval(180000);
    guc_timer->setInterval(10000);
    guc_timer->timeout.connect(sigc::mem_fun(this,&DspController::checkGucQuit));

    guc_rx_quit_timer = new QmTimer(true,this);
    guc_rx_quit_timer->timeout.connect(sigc::mem_fun(this,&DspController::sendGucQuit));

    for(int i = 0;i<50;i++) guc_adr[i] = '\0';
    guc_tx_num = 2;

    sms_call_received = false;
    for(int i = 0;i<255;i++) rs_data_clear[i] = 0;
}

DspController::~DspController()
{
	delete pending_command;
    delete counterSms;
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
	if (!resyncPendingCommand())
		return;
	ParameterValue command_value;
	command_value.squelch = value;
	sendCommand(RxRadiopath, RxSquelch, command_value);
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
	qmDebugMessage(QmDebug::Dump, "syncPulseDetected()");
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
        //qmDebugMessage(QmDebug::Dump, "processSyncPulse() radiostateGuc Tx");
        //sendGuc(GucPath);
        break;
    }
    case radiostateGucRx:{
        //qmDebugMessage(QmDebug::Dump, "processSyncPulse() radiostateGuc Rx");
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
    char hr_ch[3] = {0,0,0};
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
}

void DspController::transmitSMS()
{
    getDataTime();

    ContentSms.L_CODE = navigator->Calc_LCODE(
                0 /*ContentPSWF.R_ADR*/, 0 /*ContentPSWF.S_ADR*/,
                ContentSms.COM_N, ContentSms.RN_KEY,
                date_time[0], date_time[1], date_time[2], date_time[3]);


    if (ContentSms.stage == StageTx_call)
    {
        if (counterSms[StageTx_call] == 0)
        {
            counterSms[StageTx_call] = 18;
            qmDebugMessage(QmDebug::Dump, "ContentSms.stage = StageTx_call_ack");
            ContentSms.stage = StageTx_call_ack;
            radio_state = radiostateSmsTxRxSwitch;
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
            ContentSms.stage = StageRx_data;
            radio_state = radiostateSmsTxRxSwitch;
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
   // addSeconds(date_time);
    ContentPSWF.L_CODE = navigator->Calc_LCODE(
    		ContentPSWF.R_ADR /*ContentPSWF.R_ADR*/, ContentPSWF.S_ADR /*ContentPSWF.S_ADR*/,
    		ContentPSWF.COM_N, ContentPSWF.RN_KEY,
			date_time[0], date_time[1], date_time[2], date_time[3]);

    qmDebugMessage(QmDebug::Dump, "transmitPswf() command_tx30 = %d", command_tx30);
    if (command_tx30 == 30)
    {
        qmDebugMessage(QmDebug::Dump, "PSWF trinsmitting finished");
        command_tx30 = 0;
        if (pswf_ack) {
            startPSWFReceiving(false);
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
				startPSWFTransmitting(false, ContentPSWF.R_ADR, ContentPSWF.COM_N);
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
	ContentSms.Frequency = getFrequencyPswf();

	ParameterValue param;
	param.frequency = ContentSms.Frequency;
	if (ContentSms.stage == StageRx_data)
		sendCommand(PSWFReceiver, 3, param);
	else
		sendCommand(PSWFReceiver, PswfRxFrequency, param);

	if (pswf_first_packet_received)
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

//    private_lcode = navigator->Calc_LCODE(0,0,recievedPswfBuffer.at(command_rx30).at(0),1,
//            date_time[0],date_time[1], date_time[2],prevSecond(date_time[3]));

    private_lcode = navigator->Calc_LCODE(0,0,recievedPswfBuffer.at(command_rx30).at(0),1,
    		date_time[0],date_time[1], date_time[2], date_time[3]); //TODO: fix receiving

    qmDebugMessage(QmDebug::Dump, "private_lcode = %d,lcode = %d", private_lcode,recievedPswfBuffer.at(command_rx30).at(1));

    for(int i =0; i<recievedPswfBuffer.size(); i++)
    {
    	if (i != command_rx30)
    		if (recievedPswfBuffer.at(command_rx30).at(0) == recievedPswfBuffer.at(i).at(0)) {
    			ContentPSWF.COM_N = recievedPswfBuffer.at(command_rx30).at(0);
    			ContentPSWF.R_ADR = ContentPSWF.S_ADR;
    			ContentPSWF.S_ADR = PSWF_SELF_ADR;
    			if (private_lcode == recievedPswfBuffer.at(command_rx30).at(1))
    				firstPacket(ContentPSWF.COM_N); // COM_N
    		}
    }
    ++command_rx30;
}

int DspController::prevSecond(int second) {
	if (second == 0)
		return 59;
	return second - 1;
}

int DspController::getFrequencyPswf()
{
    int frequency = 0;

	int sum = 0;
	int fr_sh = CalcShiftFreq(ContentPSWF.RN_KEY,date_time[3],date_time[0],date_time[1],date_time[2]);

	fr_sh += 1622;

	//QM_ASSERT(fr_sh >= 0 && fr_sh <= 6671);

	fr_sh = fr_sh * 1000; // Гц


	bool find_fr = false;
	int i = 0;


	for(int i = 0; i<32;i+=2)
	{
		if((fr_sh >= frequence_bandwidth[i]) && (fr_sh <= frequence_bandwidth[i+1]))
		{
			break;
		}else{
			fr_sh += (frequence_bandwidth[i+2] - frequence_bandwidth[i+1]);
		}
	}

//	while(find_fr == false)
//	{
//		sum += (frequence_bandwidth[i+1] - frequence_bandwidth[i]);
//		if (fr_sh < sum)
//		{
//			fr_sh = fr_sh - (sum - (frequence_bandwidth[i+1] - frequence_bandwidth[i]));
//            frequency = (frequence_bandwidth[i] + fr_sh);
//			find_fr = true;
//		}
//
//		i++;
//	}

	qmDebugMessage(QmDebug::Dump,"frequency:  %d ", fr_sh);

    return fr_sh;
}


int DspController::CalcShiftFreq(int RN_KEY, int SEC, int DAY, int HRS, int MIN)
{
    int TOT_W = 6671; // ширина разрешенных участков

    int SEC_MLT = value_sec[SEC]; // SEC_MLT выбираем в массиве

    // RN_KEY - ключ подсети, аналогия с ip-сетью, поставим 0 по умолчанию

    int FR_SH = (RN_KEY + 230*SEC_MLT + 19*MIN + 31*HRS + 37*DAY)% TOT_W;

    qmDebugMessage(QmDebug::Dump, "Calc freq formula %d", FR_SH);
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

void DspController::sendCommand(Module module, int code, ParameterValue value) {
	qmDebugMessage(QmDebug::Dump, "sendCommand(%d, %d)", module, code);
	if (pending_command->in_progress) {
		qmDebugMessage(QmDebug::Dump, "pushed to queue");
		DspCommand cmd;
		cmd.module = module;
		cmd.code = code;
		cmd.value = value;
		cmd_queue->push_back(cmd);
	} else {
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
	qmToBigEndian((uint8_t)ContentPSWF.R_ADR, tx_data+tx_data_len);
	++tx_data_len;
	qmToBigEndian((uint8_t)ContentPSWF.S_ADR, tx_data+tx_data_len);
	++tx_data_len;
	qmToBigEndian((uint8_t)ContentPSWF.COM_N, tx_data+tx_data_len);
    ++tx_data_len;
	qmToBigEndian((uint8_t)ContentPSWF.L_CODE, tx_data+tx_data_len);
	++tx_data_len;


	QM_ASSERT(pending_command->in_progress == false);
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
    for(int i = 0; i < ContentGuc.NUM_com; ++i) {
        qmToBigEndian((uint8_t)ContentGuc.command[i], tx_data + tx_data_len);
        ++tx_data_len;
    }
    uint32_t crc = pack_manager->CRC32(ContentGuc.command, ContentGuc.NUM_com);
    qmToBigEndian(crc, tx_data + tx_data_len);
    tx_data_len += 4;

//            qmToBigEndian((uint8_t)ContentGuc.uin, tx_data + tx_data_len);
//            ++tx_data_len;
//
//            Navigation::Coord_Date date = navigator->getCoordDate();
//
//            std::string lon((const char*)date.longitude);
//            std::string lat((const char*)date.latitude);
//
//            uint16_t coord[4];
//            coord[0] = atoi(lon.substr(0,4).c_str());
//            coord[1] = atoi(lon.substr(0,5).c_str());
//            coord[2] = atoi(lat.substr(7,4).c_str());
//            coord[3] = atoi(lat.substr(6,4).c_str());
//
//            for(int i = 0;i<4;i++){
//                qmToBigEndian((uint16_t)(coord[i]), tx_data + tx_data_len);
//                tx_data_len += 2;
//            }
//            uint8_t quadrant = 0;
//
//            if ((strstr((const char*)date.longitude[0],"N") !=0) && strstr((const char*)date.latitude[0],"E") !=0)
//                quadrant = 0;
//            if ((strstr((const char*)date.longitude[0],"S") !=0) && strstr((const char*)date.latitude[0],"E") !=0)
//                quadrant = 1;
//            if ((strstr((const char*)date.longitude,"S") !=0) && strstr((const char*)date.latitude[0],"W") !=0)
//                quadrant = 2;
//            if ((strstr((const char*)date.longitude,"N") !=0) && strstr((const char*)date.latitude[0],"W") !=0)
//                quadrant = 3;
//
//            qmToBigEndian((uint8_t)quadrant, tx_data + tx_data_len);
//            ++tx_data_len;
//
//            uint8_t *data_crc;
//            data_crc = &tx_data[tx_data_len - 9];
//            uint32_t crc = pack_manager->CRC32(data_crc,9);
//
//            qmToBigEndian((uint32_t)crc, tx_data + tx_data_len);
//            tx_data_len+=4;

    QM_ASSERT(pending_command->in_progress == false);
    pending_command->in_progress = true;
    pending_command->sync_next = true;

    transport->transmitFrame(tx_address, tx_data, tx_data_len);
}

void DspController::recGuc()
{
    // todo
	srand(time(0));

    if (ContentGuc.stage == GucTxQuit)
    {
        guc_timer->start();
        startGucRecieving();
    }
    if (ContentGuc.stage == GucRxQuit)
    {
    	startGucTransmitting();
    	uint32_t time = 3 - ((uint32_t)rand() % 3);
    	guc_rx_quit_timer->start(time);
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
            sendGuc();
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
        } else if (indicator == 30) {
        	qmDebugMessage(QmDebug::Dump, "0x63 indicator 30");
            if (ContentSms.stage > -1)
            {
                qmDebugMessage(QmDebug::Dump, "processReceivedFrame() data_len = %d", data_len);
                std::vector<uint8_t> sms_data;
                if (ContentSms.stage == StageRx_data)
                {  static int cnt = 0;
                    if (cnt  > 36) cnt = 0;
                    for(int i = 8; i < 15; i++) {
                        sms_data.push_back(data[i]);
                        qmDebugMessage(QmDebug::Dump, "data[%d] = %d", i, data[i]);
                    }
                    recievedSmsBuffer.push_back(sms_data);

                }

                if (ContentSms.stage == StageTx_call_ack)
                {
                    tx_call_ask_vector.push_back(data[9]); // CYC_N response
                }
                if (ContentSms.stage == StageRx_call)
                {
                    syncro_recieve.erase(syncro_recieve.begin());
                    syncro_recieve.push_back(data[9]); // CYC_N
                    qmDebugMessage(QmDebug::Dump, "recieve frame() count = %d", syncro_recieve.size());

                    if (check_rx_call())
                    {
                    	sms_call_received = true;
                    	qmDebugMessage(QmDebug::Dump, "sms call received");
//                    	startSMSCmdTransmitting(StageRx_call_ack);
//                    	pswf_first_packet_received = false;
//                    	counterSms[StageRx_call] = 18;
                    	syncro_recieve.clear();
                    	for(int i = 0; i<18;i++)
                    		syncro_recieve.push_back(0);
                    }
                }

                if (ContentSms.stage == StageTx_quit)
                {
                	getDataTime();
                	if (calc_ack_code(data[9]) == (data[10]-2))
                		 ++ok_quit;
                    quit_vector.push_back(data[9]);  // ack
                    quit_vector.push_back(data[10]); // ack code
                }
                pswf_first_packet_received = true;
            }
            else
            {
                std::vector<char> pswf_data;
                pswf_data.push_back(data[9]);
                pswf_data.push_back(data[10]);
                recievedPswfBuffer.push_back(pswf_data);
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
        }

        break;
    }
    case 0x6B:
    {
    	qmDebugMessage(QmDebug::Dump, "processReceivedFrame() 0x6B received, intdicator = %d", indicator);
        if (ContentGuc.stage != GucNone){
        	if (indicator == 21) {
        		break;
        	}
            if (indicator == 22) {
            	ContentGuc.stage = GucTxQuit;
            	recGuc();
            }
            if (indicator == 30) {
            	qmDebugMessage(QmDebug::Dump, "0x6B recieved frame: %d , indicator %d", indicator);

//            	uint32_t crc;
//                for(int i = data_len; i>data_len-4;i--){
//                    crc += (data[i] >> (8*i));
//
//                    uint32_t crc32 = pack_manager->CRC32(data,10);
//                    if (crc32 == crc) {
//                        rec_s_adr = data[7];    // s_adr
//                        rec_uin_guc = data[11]; // uin
//                        ContentGuc.stage = GucRxQuit;
//                        recGuc();
//                    }
//
//                    else{
//                        // generate failed guc
//                    }
//                }
            	if (radio_state != radiostateGucRx) break;

                std::vector<uint8_t> guc;
                for(int i = 2;i<data_len;i++){qmDebugMessage(QmDebug::Dump, "0x7B recieved frame: %d , num %d", data[i],i);
                	guc.push_back(data[i]); // по N едениц данных

                	}
				if (guc_vector.size() < 50) {
					guc_vector.push_back(guc);
				}

				if (ContentGuc.stage != GucTxQuit)
					ContentGuc.stage = GucRxQuit;
				recGuc();
            }
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



void *DspController::getContentPSWF()
{
    return &ContentPSWF;
}

void DspController::sendSms(Module module)
{
    ContentSms.Frequency = getFrequencyPswf();
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
    	int FST_N = 0;
    	static int cntChvc = 7;
    	if (cntChvc > 259) cntChvc = 7;
    	qmToBigEndian((uint8_t)ContentSms.SNR, tx_data+tx_data_len);
    	++tx_data_len;
    	qmToBigEndian((uint8_t)FST_N, tx_data+tx_data_len);
    	++tx_data_len;
    	for(int i = cntChvc - 7;i<cntChvc;i++)
    	{
    		qmToBigEndian(ContentSms.message[i], tx_data+tx_data_len);
    		++tx_data_len;
    		qmDebugMessage(QmDebug::Dump, "MESSG: %d",ContentSms.message[i]);
    	}

    	cntChvc = cntChvc + 7;
    }
    //rx
    if (ContentSms.stage == StageRx_call_ack)
    {
    	int wzn = 0;

    	qmToBigEndian((uint8_t)ContentSms.SNR, tx_data+tx_data_len);
    	++tx_data_len;
    	qmToBigEndian((uint8_t)ContentSms.R_ADR, tx_data+tx_data_len);
    	++tx_data_len;
    	qmToBigEndian((uint8_t)ContentSms.S_ADR, tx_data+tx_data_len);
    	++tx_data_len;
    	qmToBigEndian((uint8_t)wzn, tx_data+tx_data_len);
    	++tx_data_len;

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
        if (counterSms[StageTx_call_ack] == 0)
        {
            qmDebugMessage(QmDebug::Dump, "recSms() recievedSmsBuffer.size() =  %d", recievedSmsBuffer.size());
            if (tx_call_ask_vector.size() >= 3/*recievedSmsBuffer.size() == 18*/) {
                startSMSTransmitting(0, ContentSms.message, StageTx_data);//StageTx_data);  //TODO:
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
            if (quit_vector.size() >= 2/*recievedSmsBuffer.size() == 18 + 6*/) {
                qmDebugMessage(QmDebug::Dump, "recSms() SMS transmitting successfully finished");
                //TODO: sms tx finished
                if (ok_quit >= 2) smsFailed(-1);
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

	int temp=eras_dec_rs(data,rs_data_clear,&rs_255_93);

    uint8_t crc_chk[88];


	for(int i = 0;i<100;i++) packed[i] = 0;

    for(int i = 0;i<88;i++) crc_chk[i] = data[i];

    int diagn = pack_manager->decompressMass(crc_chk,88,packed,100,7); //test

    pack_manager->to_Win1251(packed); //test

	uint32_t crc_calc = pack_manager->CRC32(crc_chk,88);


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
		for(int i = 0;i<90;i++) sms_content[i] = (char)packed[i];
		recievedSmsBuffer.erase(recievedSmsBuffer.begin(),recievedSmsBuffer.end());
		sms_content[99] = '\0';
        qmDebugMessage(QmDebug::Dump, "generateSmsReceived() sms_content = %s", sms_content);
		smsPacketMessage();
	}
}

int DspController::check_rx_call()
{
    int cnt_index = 0;
    for(int i = 0; i<18;i++)
    {
       if (syncro_recieve.at(i) == (i+1))
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
}

void DspController::startPSWFTransmitting(bool ack, uint8_t r_adr, uint8_t cmd) {
	qmDebugMessage(QmDebug::Dump, "startPSWFTransmitting(%d, %d, %d)", ack, r_adr, cmd);
    QM_ASSERT(is_ready);
    if (!resyncPendingCommand())
        return;

    pswf_ack = ack;
    getDataTime();
    ContentPSWF.Frequency = getFrequencyPswf();

    ContentPSWF.indicator = 20;
    ContentPSWF.TYPE = 0;
    ContentPSWF.COM_N = cmd;
    ContentPSWF.RN_KEY = 1;
    ContentPSWF.R_ADR = r_adr;
    ContentPSWF.S_ADR = PSWF_SELF_ADR;

    ParameterValue comandValue;
    comandValue.radio_mode = RadioModeOff;
    sendCommand(RxRadiopath, RxRadioMode, comandValue);
    comandValue.pswf_indicator = RadioModePSWF;
    sendCommand(TxRadiopath, TxRadioMode, comandValue);
    pswfTxStateSync = 0;
    radio_state = radiostatePswfTxPrepare;
}

void DspController::startSMSRecieving(SmsStage stage)
{
    qmDebugMessage(QmDebug::Dump, "startSmsReceiving");
    QM_ASSERT(is_ready);
//    if (!resyncPendingCommand())
//        return;

    getDataTime();
    ContentSms.Frequency = getFrequencyPswf();

//    if (ContentSms.stage == StageNone)
//    recievedSmsBuffer.erase(recievedSmsBuffer.begin(),recievedSmsBuffer.end());

    tx_call_ask_vector.erase(tx_call_ask_vector.begin(),tx_call_ask_vector.end());
    quit_vector.erase(quit_vector.begin(),quit_vector.end());

    ParameterValue param;
    param.pswf_r_adr = PSWF_SELF_ADR;
    sendCommand(PSWFReceiver, PswfRxRAdr, param);

    ParameterValue comandValue;
    comandValue.radio_mode = RadioModeOff;
    sendCommand(TxRadiopath, TxRadioMode, comandValue);
    comandValue.pswf_indicator = RadioModePSWF;
    sendCommand(RxRadiopath, RxRadioMode, comandValue);
    smsRxStateSync = 0;
    radio_state = radiostateSmsRxPrepare;
    ContentSms.stage = stage;

    //for(int i = 0;i<100;i++) sms_content[i] = '0';
}

void DspController::startSMSTransmitting(uint8_t r_adr,uint8_t* message, SmsStage stage)
{
    qmDebugMessage(QmDebug::Dump, "SMS tranmit (%d, %s)",r_adr, message);
    QM_ASSERT(is_ready);

    getDataTime();
    ContentSms.Frequency = getFrequencyPswf();

    ContentSms.indicator = 20;
    ContentSms.TYPE = 0;
    int ind = strlen((const char*)message);


    int data_sms[255];

    int sms[255];

    //pack_manager->Text(message,sms,ind);

    if (ContentSms.stage == StageNone)
    {
    	for(int i = 0; i<ind;i++) ContentSms.message[i] = message[i];

    	pack_manager->to_Koi7(ContentSms.message); // test

    	pack_manager->compressMass(ContentSms.message,87,7); //test

        ContentSms.message[87] = ContentSms.message[87] & 0x0F; //set 4 most significant bits to 0
        ContentSms.message[88] = 0;

        uint32_t abc = pack_manager->CRC32(ContentSms.message,88);


        for(int i = 0;i<4;i++) ContentSms.message[89+i] = (uint8_t)((abc >> (8*i)) & 0xFF);
    	for(int i = 0;i<255;i++) rs_data_clear[i] = 0;
    	for(int i = 0; i<255;i++) data_sms[i] = (int)ContentSms.message[i];

    	int temp=encode_rs(data_sms,&data_sms[93],&rs_255_93);
    	for(int i = 0; i<255;i++)ContentSms.message[i]  = data_sms[i];

    }


    ContentSms.RN_KEY = 1;
    ContentSms.R_ADR = r_adr;
    ContentSms.S_ADR = PSWF_SELF_ADR;


    ParameterValue comandValue;
    comandValue.radio_mode = RadioModeOff;
    sendCommand(RxRadiopath, RxRadioMode, comandValue);
    comandValue.pswf_indicator = RadioModePSWF;
    sendCommand(TxRadiopath, TxRadioMode, comandValue);
    smsTxStateSync = 0;
    radio_state = radiostateSmsTxPrepare;
    ContentSms.stage = stage;
}

void DspController::startGucTransmitting(int r_adr, int speed_tx, uint8_t *command)
{
    qmDebugMessage(QmDebug::Dump, "startGucTransmitting(%d, %d, 0x%X)", r_adr, speed_tx, command);
    QM_ASSERT(is_ready);

    ContentGuc.indicator = 20;
    ContentGuc.type = 1;
    ContentGuc.chip_time = 2;
    ContentGuc.WIDTH_SIGNAL = 1;
    ContentGuc.S_ADR = 0;
    ContentGuc.R_ADR = 1;

//    int num_cmd = strlen((const char*) command);
    ContentGuc.NUM_com = 5;

    for(int i = 0;i<100;i++) ContentGuc.command[i] = -1;
//    for(int i = 0; i<num_cmd;i++) ContentGuc.command[i] = command[i];

    ContentGuc.ckk = 0;
    ContentGuc.ckk |= (1 & 0x01);
    ContentGuc.ckk |= (ContentGuc.WIDTH_SIGNAL & 0x01) << 1;
    ContentGuc.ckk |= (1 & 0x03) << 2;
    ContentGuc.ckk |= (ContentGuc.chip_time & 0x03) << 4;

    ContentGuc.uin = 0;
    ContentGuc.Coord = 0;

    ParameterValue comandValue;
    comandValue.radio_mode = RadioModeOff;// отключили прием
    sendCommand(RxRadiopath, RxRadioMode, comandValue);
    comandValue.guc_mode = RadioModeSazhenData; // включили 11 режим
    sendCommand(TxRadiopath, TxRadioMode, comandValue);
    comandValue.frequency = 3000000;
    sendCommand(RxRadiopath, RxFrequency, comandValue);
    sendCommand(TxRadiopath, TxFrequency, comandValue);
    radio_state = radiostateGucTxPrepare;
    gucTxStateSync = 0;
    ContentGuc.stage =  GucTx;
}

void DspController::startGucTransmitting()
{
    qmDebugMessage(QmDebug::Dump, "startGucTransmitting");
    QM_ASSERT(is_ready);

    ParameterValue comandValue;
//    comandValue.radio_mode = RadioModeOff;// отключили прием
//    sendCommand(RxRadiopath, RxRadioMode, comandValue);
    comandValue.guc_mode = RadioModeSazhenData; // включили 11 режим
    sendCommand(TxRadiopath, TxRadioMode, comandValue);
    comandValue.frequency = 3000000;
    sendCommand(RxRadiopath, RxFrequency, comandValue);
    sendCommand(TxRadiopath, TxFrequency, comandValue);
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
	ContentGuc.S_ADR = 0;
	ContentGuc.R_ADR = 1;

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

//	QM_ASSERT(pending_command->in_progress == false);
	pending_command->in_progress = true;
	pending_command->sync_next = true;

	transport->transmitFrame(tx_address, tx_data, tx_data_len);
}

void DspController::startGucRecieving()
{
    qmDebugMessage(QmDebug::Dump, "startGucRecieving");
    QM_ASSERT(is_ready);

    ParameterValue comandValue;
    comandValue.radio_mode = RadioModeOff;// отключили радиорежим
    sendCommand(TxRadiopath, TxRadioMode, comandValue);
    comandValue.guc_mode = 3;
    sendCommand(RadioLineNotPswf, 0 ,comandValue); // отключить низкоскоростной модем
    comandValue.guc_mode = RadioModeSazhenData; // включили 11 режим
    sendCommand(RxRadiopath, RxRadioMode, comandValue);

    radio_state = radiostateGucRxPrepare;
    gucRxStateSync = 0;
    if (ContentGuc.stage != GucTxQuit)
    	ContentGuc.stage =  GucRx;
}

void DspController::checkGucQuit()
{
	int size = 0;
	if (guc_vector.size() > 50)
		size = 50;
	else
		size  = guc_vector.size();

	qmDebugMessage(QmDebug::Dump, "checkGucQuit() number of recieved responses %d", guc_vector.size());

//    for(int i = 0; i< size;i++)
//        guc_adr[i] = guc_vector.at(i).at(3);
    guc_vector.clear();

//    guc_adr[size-1] = '\0';
    radio_state = radiostateSync;
    // TODO string guc_adr to GUI
}


void DspController::startSMSCmdTransmitting(SmsStage stage)
{
    qmDebugMessage(QmDebug::Dump, "startSMSCmdTransmitting(%d)", stage);
    QM_ASSERT(is_ready);
//    if (!resyncPendingCommand())
//        return;

    getDataTime();
    ContentSms.Frequency = getFrequencyPswf();

    ContentSms.indicator = 20;
    ContentSms.TYPE = 0;
    ContentSms.RN_KEY = 1;
    ContentSms.R_ADR = 0;
    ContentSms.S_ADR = PSWF_SELF_ADR;


    ParameterValue comandValue;
    comandValue.radio_mode = RadioModeOff;
    sendCommand(RxRadiopath, RxRadioMode, comandValue);
    comandValue.pswf_indicator = RadioModePSWF;


    sendCommand(TxRadiopath, TxRadioMode, comandValue);
    smsTxStateSync = 0;
    radio_state = radiostateSmsTxPrepare;
    ContentSms.stage = stage;
}




} /* namespace Multiradio */

#include "qmdebug_domains_start.h"
QMDEBUG_DEFINE_DOMAIN(dspcontroller, LevelVerbose)
#include "qmdebug_domains_end.h"
