/**
 ******************************************************************************
 * @file    dspcontroller.cpp
 * @author  Artem Pisarenko, PMR dept. software team, ONIIP, PJSC
 * @author  � � � …� � Вµ� � С‘� � В·� � � � � � Вµ� Ў� ѓ� ЎвЂљ� � � …� ЎвЂ№� � Вµ
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
#include <cstring>
#include "../synchro/virtual_timer.h"

#define DEFAULT_PACKET_HEADER_LEN	2
#define hw_rtc                      1
#define DefkeyValue 631

//#define GUC_TIMER_INTERVAL 3000
#define GUC_TIMER_INTERVAL_REC 30000

#define VIRTUAL_TIME 120

#define NUMS 0 // need = 0   9 for debug
#define startVirtTxPhaseIndex 0;

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

DspController::DspController(int uart_resource, int reset_iopin_resource, Navigation::Navigator *navigator, DataStorage::FS *data_storage_fs, QmObject *parent) :
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
	// max_tx_queue_size: 1 � � С”� � С•� � С�� � В°� � � …� � Т‘� � В° � Ў� ‚� � В°� � Т‘� � С‘� � С•� ЎвЂљ� Ў� ‚� � В°� � С”� ЎвЂљ� � В° + 1 � � В·� � В°� � С—� � В°� Ў� ѓ
	transport = new DspTransport(uart_resource, 2, this);
	transport->receivedFrame.connect(sigc::mem_fun(this, &DspController::processReceivedFrame));
	initResetState();

#ifndef PORT__PCSIMULATOR
    rtc = new QmRtc(hw_rtc);
	rtc->wakeup.connect(sigc::mem_fun(this,&DspController::wakeUpTimer));
#endif

    if (navigator != 0) {
    	this->navigator = navigator;
    	navigator->syncPulse.connect(sigc::mem_fun(this, &DspController::syncPulseDetected));
    }
    this->data_storage_fs = data_storage_fs;

	sync_pulse_delay_timer = new QmTimer(true, this);
	sync_pulse_delay_timer->setInterval(100);
	sync_pulse_delay_timer->timeout.connect(sigc::mem_fun(this, &DspController::processSyncPulse));

    cmd_queue = new std::list<DspCommand>();

    fwd_wave = 0;
    ref_wave = 0;

    command_tx30 = 0;

    pswfRxStateSync = 0;
    pswfTxStateSync = 0;
    smsRxStateSync = 0;
    smsTxStateSync = 0;
    //gucRxStateSync = 0;
    gucTxStateSync = 0;

    success_pswf = 30;
    pswf_first_packet_received = false;
    pswf_ack = false;
    pswf_ack_tx = false;
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
        syncro_recieve.push_back(99);
        snr.push_back(0);
        waveZone.push_back(0);
    }
    	waveZone.push_back(0); // size must be 19

    //guc_timer = new QmTimer(true,this);
    //guc_timer->setInterval(GUC_TIMER_INTERVAL);


    for(int i = 0;i<50;i++) guc_text[i] = '\0';


    sms_call_received = false;
    for(int i = 0;i<255;i++) rs_data_clear[i] = 1;\


   data_storage_fs->getAleStationAddress(stationAddress);
    ContentSms.S_ADR = stationAddress;
    ContentPSWF.S_ADR = stationAddress;
    //data_storage_fs->getAleStationAddress(ContentPSWF.S_ADR);
    QNB = 0;
    pswf_rec = 0;

    ContentPSWF.RN_KEY = DefkeyValue;
    ContentSms.RN_KEY = DefkeyValue;

    retranslation_active = false;

    for(uint8_t i = 0; i <= 100; i++)
    	sms_content[i] = 0;

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
   // delete guc_timer;
    delete cmd_queue;
    delete rtc;
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
	startup_timer->start(10000);
}

void DspController::setRadioParameters(RadioMode mode, uint32_t frequency) {
	bool processing_required = true;
	QM_ASSERT(is_ready);
	switch (radio_state) {
	case radiostateSync: {
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
	case radiostateCmdTxPower:
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


void DspController::powerControlAsk()
{
	// вольт * 10
	/*ParameterValue command;
	ind = 2, param = 10,
	sendCommandEasy(TxRadioMode,)*/
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
	if (virtual_mode == true)
		return;

	switch (radio_state) {
	case radiostatePswf : {
        changePswfFrequency();
        break;
	}
	case radiostateSms:
	{
		changeSmsFrequency();
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

    qmDebugMessage(QmDebug::Dump, ">>> getDataTime(): %d %d %d %d", day, hrs, min, sec);

    addSeconds(date_time);
}

void DspController::setRx()
{
    ParameterValue comandValue;
    comandValue.radio_mode = RadioModeOff;
    sendCommandEasy(TxRadiopath, TxRadioMode, comandValue);
    comandValue.pswf_indicator = RadioModePSWF;
    sendCommandEasy(RxRadiopath, RxRadioMode, comandValue);
}

void DspController::setTx()
{
	ParameterValue comandValue;
	comandValue.radio_mode = RadioModeOff;
	sendCommandEasy(RxRadiopath, RxRadioMode, comandValue);
	comandValue.pswf_indicator = RadioModePSWF;
	sendCommandEasy(TxRadiopath, TxRadioMode, comandValue);
}

void DspController::sendPswf()
{
	int time[4] = {0,0,0,0};

	if (virtual_mode)
	{
		time[0] = d.day;
		time[1] = t.hours;
		time[2] = t.minutes;
		time[3] = t.seconds;
		qmDebugMessage(QmDebug::Dump, "time now: %d %d %d %d" ,d.day,t.hours,t.minutes,t.seconds);
	}
	else
	{
		for(int i = 0; i<4;i++) time[i] = date_time[i];
	}
	if (ContentPSWF.RET_end_adr > 0)
	{
		ContentPSWF.L_CODE = navigator->Calc_LCODE_RETR(
				ContentPSWF.RET_end_adr,
				ContentPSWF.R_ADR,
				ContentPSWF.COM_N,
				ContentPSWF.RN_KEY,
				time[0],
				time[1],
				time[2],
				time[3]);
	}
	else
	{
		ContentPSWF.L_CODE = navigator->Calc_LCODE(
				ContentPSWF.R_ADR,
				ContentPSWF.S_ADR,
				ContentPSWF.COM_N,
				ContentPSWF.RN_KEY,
				time[0],
				time[1],
				time[2],
				time[3]);
	}

    ContentPSWF.Frequency = getFrequency(0); //pswf = 0, sms = 1
    ContentPSWF.indicator = 20;
    ContentPSWF.TYPE = 0;
    ContentPSWF.SNR =  9;

    uint8_t tx_address = 0x72;
    uint8_t tx_data[DspTransport::MAX_FRAME_DATA_SIZE];
    int tx_data_len = 0;
    qmToBigEndian((uint8_t)ContentPSWF.indicator,  tx_data + tx_data_len); ++tx_data_len;
    qmToBigEndian((uint8_t)ContentPSWF.TYPE,       tx_data + tx_data_len); ++tx_data_len;
    qmToBigEndian((uint32_t)ContentPSWF.Frequency, tx_data + tx_data_len); tx_data_len += 4;
    qmToBigEndian((uint8_t)ContentPSWF.SNR,        tx_data + tx_data_len); ++tx_data_len;

    qmToBigEndian((uint8_t)(pswf_retranslator ? ContentPSWF.RET_end_adr : ContentPSWF.R_ADR), tx_data+tx_data_len);  ++tx_data_len;
    qmToBigEndian((uint8_t)(pswf_retranslator ? ContentPSWF.R_ADR : ContentPSWF.S_ADR),  tx_data + tx_data_len); ++tx_data_len;
    qmToBigEndian((uint8_t)ContentPSWF.COM_N,  tx_data + tx_data_len); ++tx_data_len;
    qmToBigEndian((uint8_t)ContentPSWF.L_CODE, tx_data + tx_data_len); ++tx_data_len;

    transport->transmitFrame(tx_address, tx_data, tx_data_len);
}
//TODO
void DspController::addSeconds(int *date_time)
{
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

void DspController::addSeconds(QmRtc::Time *t)
{
    t->seconds += 1;
    if (t->seconds >= 60) {
        t->seconds %= 60;
        t->minutes++;
        if (t->minutes >= 60) {
            t->minutes %= 60;
            t->hours++;  //TODO why not check hours overflow?
        }
    }
}

void DspController::LogicPswfTx()
{
	++command_tx30;

    if ((command_tx30 % 3 == 0) && (setAsk == false))
    	TxCondCmdPackageTransmit(command_tx30);

	if (command_tx30 <= 30)
		sendPswf();

	if (command_tx30 > 31)
	{
		command_tx30 = 0;

        if(pswf_ack_tx)
		{
			pswf_ack_tx = false;
			CondComLogicRole = CondComRx;
			waitAckTimer = 1;
			setPswfRx();
		}
		else
		{
            stationModeIsCompleted();
            //radio_state = radiostateSync;
		}
	}

	qmDebugMessage(QmDebug::Dump, "RX____ ContentPSWF.R_ADR = %d, ContentPSWF.S_ADR = %d ", ContentPSWF.R_ADR, ContentPSWF.S_ADR);
}

void DspController::LogicPswfRx()
{
    setPswfRxFreq();

	 qmDebugMessage(QmDebug::Dump, " >>>>>>>>> LogicPswfRx() waitAckTimer = %d", waitAckTimer);
	if (waitAckTimer){
		waitAckTimer++;
		if (waitAckTimer >= 65){
			waitAckTimer = 0;
			firstPacket(100, false); // no ack recieved
            stationModeIsCompleted();
		}
	}
	if (isPswfFull)
	{
		pswf_rec = 0;

		if (pswf_ack)
		{
			CondComLogicRole = CondComTx;
			pswf_ack = false;
			ContentPSWF.R_ADR = ContentPSWF.S_ADR;
			ContentPSWF.S_ADR = stationAddress;
			setPswfTx();
			isPswfFull = false;
			command_tx30 = 1;
			sendPswf();
		}
		else
		{
            //radio_state = radiostateSync;
            stationModeIsCompleted();
		}
	}

	qmDebugMessage(QmDebug::Dump, "RX____ ContentPSWF.R_ADR = %d, ContentPSWF.S_ADR = %d ", ContentPSWF.R_ADR, ContentPSWF.S_ADR);
}


void DspController::changePswfFrequency()
{
	qmDebugMessage(QmDebug::Dump, " >>>>>>>>> changePswfFrequency() r_adr = %d,s_adr = %d", ContentPSWF.R_ADR,ContentPSWF.S_ADR);
	if (!virtual_mode)
	{
		getDataTime();
		addSeconds(date_time);
	}

	if (CondComLogicRole == CondComTx)
	{
		LogicPswfTx();
	}
	else if (CondComLogicRole == CondComRx)
	{
		LogicPswfRx();
	}
}

void DspController::setPswfRxFreq()
{
	ContentPSWF.Frequency = getFrequency(0); //pswf = 0, sms = 1
	ParameterValue param;
	param.frequency = ContentPSWF.Frequency;
	sendCommandEasy(PSWFReceiver, PswfRxFrequency, param);
}


void DspController::RxSmsWork()
{
	if (radio_state == radiostateSync) return;

	if (smsFind)
	{
		++sms_counter;

		if (sms_counter ==  20)
		{
			setTx();
			sendSms(PSWFTransmitter);
		}

        if (sms_counter > 20 && sms_counter < 38)
        	sendSms(PSWFTransmitter);

        if (sms_counter == 39)
		{
			setRx();
			setrRxFreq();
		}

		if (sms_counter > 39 && sms_counter<76)
		{
			setrRxFreq();
		}
		if (sms_counter == 77)
		{
			setTx();
			quest = generateSmsReceived();
			sendSms(PSWFTransmitter);
		}
		if (sms_counter > 77 && sms_counter < 83)
		{
            sendSms(PSWFTransmitter);
		}

        if (sms_counter == 84)
		{
        	if (quest)
        	{
        		smsPacketMessage(indexSmsLen); quest = false;
        	}

			sms_counter = 0;
			setRx();
			smsFind = false;
			// TODO: recieved
            uint8_t ret = getSmsRetranslation();
            if (ret == 0) resetSmsState();
		}
	}
	else
	{
		if (sms_call_received)
		{
			smsFind  = true;
			sms_call_received = false;
			sms_counter = 19;
			pswf_in_virt = 0;
		}
		else
		{
			setrRxFreq();
		}
	}
}

void DspController::TxSmsWork()
{
	if (radio_state == radiostateSync) return;

    ++sms_counter;

    if (sms_counter < 19)
    {
        sendSms(PSWFTransmitter);
    }

    if (sms_counter == 20)
    {
    	setRx();
    	setrRxFreq();
    }

    if (sms_counter > 20 && sms_counter < 38)
    {
        setrRxFreq();
    }

    if (sms_counter == 39)
    {
    	if (checkForTxAnswer())
    	{
    		setTx();
    		sendSms(PSWFTransmitter);
    	}
    	else
    	{
    		resetSmsState();
    		smsFailed(0);
    	}
    }

    if (sms_counter > 39 && sms_counter < 76)
    {
        sendSms(PSWFTransmitter);
    }

    if (sms_counter == 77)
    {
    	setRx();
    	setrRxFreq();
    }

    if (sms_counter > 77 && sms_counter < 83)
    {
        setrRxFreq();
    }

    if (sms_counter == 84)
    {
    	if (ok_quit >= 1)
    		smsFailed(-1); //correct recieved
    	else
    	{
    		if (smsError >= 1)
    			smsFailed(2); // negative ack recieved
    		else
    			smsFailed(1); // not ack recieved
    	}
        resetSmsState();
    }

}

void DspController::changeSmsFrequency()
{
	qmDebugMessage(QmDebug::Dump, " >>>>>>>>> changeSmsFrequency() r_adr = %d,s_adr = %d", ContentSms.R_ADR,ContentSms.S_ADR);
	if (!virtual_mode)
	{
	  getDataTime();
	  addSeconds(date_time);
	  qmDebugMessage(QmDebug::Dump, "getDataTime(): %d %d %d %d", date_time[0], date_time[1], date_time[2], date_time[3]);
	}

	if (SmsLogicRole == SmsRoleTx)
	{
        TxSmsWork();
	}

	if (SmsLogicRole == SmsRoleRx)
	{
        RxSmsWork();
	}

    static uint8_t tempCounter = sms_counter;
    if (tempCounter != sms_counter && sms_counter % 11 == 0 )
      smsCounterChanged(sms_counter);
}

void DspController::resetSmsState()
{
	sms_counter = 0;
	radio_state = radiostateSync;
	smsFind  = false;
	ok_quit = 0;
	smsError = 0;
	std::memset(rs_data_clear,1,sizeof(rs_data_clear));
    stationModeIsCompleted();
}

bool DspController::checkForTxAnswer()
{
	if (tx_call_ask_vector.size() >= 2)
	{
		wzn_value = wzn_change(tx_call_ask_vector);
		qmDebugMessage(QmDebug::Dump, "checkForTxAnswer() wzn_value" ,wzn_value);
		tx_call_ask_vector.resize(0);

		return true;
	}

	tx_call_ask_vector.resize(0);
	return false;
}

void DspController::setrRxFreq()
{
    ContentSms.Frequency =  getFrequency(1); //pswf = 0, sms = 1

    ParameterValue param;
    param.frequency = ContentSms.Frequency;
    if ((SmsLogicRole == SmsRoleRx) && (sms_counter >= 38 && sms_counter < 77))
    	sendCommandEasy(PSWFReceiver, PswfRxFreqSignal, param);
    else
    sendCommandEasy(PSWFReceiver, PswfRxFrequency, param);
}

void DspController::recPswf(uint8_t data, uint8_t code, uint8_t indicator)
{
    if (virtual_mode)
    	private_lcode = (char)navigator->Calc_LCODE(ContentPSWF.R_ADR, ContentPSWF.S_ADR, data, ContentPSWF.RN_KEY, d.day, t.hours, t.minutes, prevSecond(t.seconds));
    else
    	private_lcode = (char)navigator->Calc_LCODE(ContentPSWF.R_ADR, ContentPSWF.S_ADR, data, ContentPSWF.RN_KEY, date_time[0], date_time[1], date_time[2], prevSecond(date_time[3]));

    qmDebugMessage(QmDebug::Dump, " >>>>>>>>> recPswf() r_adr = %d,s_adr = %d", ContentPSWF.R_ADR,ContentPSWF.S_ADR);
    qmDebugMessage(QmDebug::Dump, " >>>>>>>>> recPswf() private_lcode = %d,lcode = %d", private_lcode,code);
    qmDebugMessage(QmDebug::Dump, " >>>>>>>>> recPswf() pswf_in = %d, pswf_rec = %d, pswf_in_virt = %d ",pswf_in, pswf_rec, pswf_in_virt);


	if (virtual_mode && indicator == 31){
		pswf_in_virt++;
		uint8_t counter = 37;
		if (virtual_mode)
		   counter += 65;
		if (pswf_in_virt > counter && !pswf_rec){
			startVirtualPpsModeRx();
		}
	}

    if (code == private_lcode){ //lcode can be overflow (==0) TODO fix
    	if (indicator == 30){
			firstTrueCommand = ContentPSWF.COM_N;
			++pswf_rec;
			if (pswf_rec == 2)
			{
				ContentPSWF.COM_N = data;
				firstPacket(ContentPSWF.COM_N, true);
				waitAckTimer = 0;
			}
    	}
    }
    if (pswf_in < 30)
    {
    	if (pswf_rec)
    		pswf_in++;
    }
    else
    {
    	if (pswf_rec >= 1)
    	{
    		if (pswf_rec == 1){
    			firstPacket(firstTrueCommand, false); // false - not reliable data
    			waitAckTimer = 0;
    		}

    		if (ContentPSWF.R_ADR > 32)
    		{
    			pswf_ack = true;
    			setAsk = true;
    			if (pswf_rec >= 2)
    				isPswfFull = true;
    		}
    	}
    	pswf_rec = 0;
    	pswf_in = 0;
    }
}

int DspController::prevSecond(int second)
{
	if (second == 0)
		return 59;
	return second - 1;
}

int DspController::getFrequency(uint8_t mode) //pswf = 0, sms = 1
{
	int fr_sh = 0;

	int RN_KEY = mode ? ContentSms.RN_KEY : ContentPSWF.RN_KEY;

	switch (mode){
		case 0: {
			if (virtual_mode)
				fr_sh = CalcShiftFreq(RN_KEY,d.day,t.hours,t.minutes,t.seconds);
			else
				fr_sh = CalcShiftFreq(RN_KEY,date_time[0],date_time[1],date_time[2],date_time[3]);
			break;
		}
		case 1: {
			if (virtual_mode)
				fr_sh = CalcSmsTransmitFreq(RN_KEY,d.day,t.hours,t.minutes,t.seconds);
			else
				fr_sh = CalcSmsTransmitFreq(RN_KEY,date_time[0],date_time[1],date_time[2],date_time[3]);
			break;
		}
	}

	fr_sh += 1622;
	fr_sh = fr_sh * 1000; // � � вЂњ� ЎвЂ�

	for(int i = 0; i < 32; i += 2)
	{
		if((fr_sh >= frequence_bandwidth[i]) && (fr_sh <= frequence_bandwidth[i+1]))
			break;
		else
			fr_sh += (frequence_bandwidth[i+2] - frequence_bandwidth[i+1]);
	}

	qmDebugMessage(QmDebug::Dump,"frequency:  %d ", fr_sh);
    return fr_sh;
}

void DspController::setRnKey(int keyValue)
{
    ContentPSWF.RN_KEY = keyValue;
    ContentSms.RN_KEY  = keyValue;
    // somthing else setting rn_key
}

void DspController::resetContentStructState()
{
    ContentSms.stage = StageNone;
    // � � Т‘� � С•� � В±� � В°� � � � � � С‘� ЎвЂљ� Ў� Љ � � С•� � С—� Ў� ‚� � Вµ� � Т‘� � Вµ� � В»� � Вµ� � � …� � С‘� � Вµ � � Т‘� Ў� ‚� ЎС“� � С–� � С‘� ЎвЂ¦ � ЎвЂћ� ЎС“� � � …� � С”� ЎвЂ� � � С‘� � в„–
}

int DspController::CalcShiftFreq(int RN_KEY, int DAY, int HRS, int MIN, int SEC)
{
    int TOT_W = 6671; // � Ўв‚¬� � С‘� Ў� ‚� � С‘� � � …� � В° � Ў� ‚� � В°� � В·� Ў� ‚� � Вµ� Ўв‚¬� � Вµ� � � …� � � …� ЎвЂ№� ЎвЂ¦ � ЎС“� ЎвЂЎ� � В°� Ў� ѓ� ЎвЂљ� � С”� � С•� � � �
    int SEC_MLT = value_sec[SEC]; // SEC_MLT � � � � � ЎвЂ№� � В±� � С‘� Ў� ‚� � В°� � Вµ� � С� � � � �  � � С�� � В°� Ў� ѓ� Ў� ѓ� � С‘� � � � � � Вµ
    int FR_SH = (RN_KEY + 230*SEC_MLT + 19*MIN + 31*HRS + 37*DAY)% TOT_W;
    qmDebugMessage(QmDebug::Dump, "Calc freq formula %d", FR_SH);
    return FR_SH;
}

int DspController::CalcSmsTransmitFreq(int RN_KEY, int DAY, int HRS, int MIN, int SEC)
{
    int wzn = 0;
    int FR_SH = 0;
    int TOT_W = 6671;
    int wz_base = 0;

    int SEC_MLT = value_sec[SEC];

    // Tx data, Tx_quit
    if (sms_counter > 38 && sms_counter < 84)
    {
        wzn = wzn_value;
        if (wzn > 0) wz_base = 6*wzn;
        else wzn  = 0;
        int wz_shift = SEC % 6;
        SEC_MLT = wz_shift + wz_base;
        qmDebugMessage(QmDebug::Dump, "wzn_base %d" ,wz_base);
        qmDebugMessage(QmDebug::Dump, "wzn_shift %d" ,wz_shift);

        if (sms_counter < 77)
        {
            FR_SH = (RN_KEY + 3*SEC + 230*SEC_MLT + 17*MIN + 29*HRS + 43*DAY)% TOT_W;
            qmDebugMessage(QmDebug::Dump, "Calc freq sms  formula %d", FR_SH);
        }

        if (sms_counter > 77 && sms_counter < 83)
        {
            FR_SH = (RN_KEY + 5*SEC + 230*SEC_MLT + 17*MIN + 29*HRS + 43*DAY)% TOT_W;
            qmDebugMessage(QmDebug::Dump, "Calc freq sms quit formula %d", FR_SH);
        }
    }

    if (sms_counter < 38)
    {
        FR_SH = (RN_KEY + 73 + 230*SEC_MLT + 17*MIN + 29*HRS + 43*DAY)% TOT_W;
        qmDebugMessage(QmDebug::Dump, "Calc freq sms tx formula %d", FR_SH);
    }

    if ((sms_counter < 19) && (SmsLogicRole == SmsRoleRx))
    {
    	waveZone.erase(waveZone.begin());
    	waveZone.push_back(SEC_MLT / 6);
    }

    qmDebugMessage(QmDebug::Dump, ">>> CalcSmsTransmitFreq() wzn_value %d" ,wzn_value);
    return FR_SH;
}

void DspController::initResetState()
{
	radio_state = radiostateSync;
	current_radio_operation = RadioOperationOff;
	current_radio_mode = RadioModeOff;
    current_radio_frequency = 0;
	pending_command->in_progress = false;
}

void DspController::setAdr()
{
	ParameterValue param;
    param.pswf_r_adr = stationAddress;
	sendCommand(PSWFReceiver, PswfRxRAdr, param);
}

void DspController::processStartup(uint16_t id, uint16_t major_version, uint16_t minor_version)
{
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

void DspController::processStartupTimeout()
{
	qmDebugMessage(QmDebug::Warning, "DSP startup timeout");
	is_ready = true;
	started();
}

bool DspController::startRadioOff()
{
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
	case radiostateCmdTxPower:
		radio_state = radiostateSync;
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
	case radiostateCmdTxPower:
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
			radio_state = radiostateCmdTxPower;
		else
			radio_state = radiostateCmdRxOff;
		break;
	case radiostateCmdRxOff:
	case radiostateCmdTxOff:
	case radiostateCmdCarrierTx:
		radio_state = radiostateCmdTxPower;
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
			radio_state = radiostateCmdTxPower;
		else
			radio_state = radiostateCmdRxOff;
		break;
	case radiostateCmdRxOff:
	case radiostateCmdTxOff:
	case radiostateCmdTxMode:
		radio_state = radiostateCmdTxPower;
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
	case radiostateCmdTxPower: {
		if (current_radio_operation != RadioOperationCarrierTx) {
			if (current_radio_frequency >= 30000000)
				command_value.power = 80;
			else
				command_value.power = 100;
		} else {
			command_value.power = 80;
		}
		sendCommand(TxRadiopath, TxPower, command_value);
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
		case RadioOperationCarrierTx:
			radio_state = radiostateCmdTxPower;
			break;
		}
		break;
	}
	case radiostateCmdRxMode: {
		radio_state = radiostateSync;
		break;
	}
	case radiostateCmdTxPower: {
		switch (current_radio_operation) {
		case RadioOperationTxMode:
			radio_state = radiostateCmdTxMode;
			break;
		case RadioOperationCarrierTx:
			radio_state = radiostateCmdCarrierTx;
			break;
		default:
			radio_state = radiostateSync;
			break;
		}
		break;
	}
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

void DspController::syncPendingCommand()
{
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

bool DspController::resyncPendingCommand()
{
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
	qmToBigEndian((uint8_t)2, tx_data+0); // � � С‘� � � …� � Т‘� � С‘� � С”� � В°� ЎвЂљ� � С•� Ў� ‚: "� � С”� � С•� � С�� � В°� � � …� � Т‘� � В° (� ЎС“� Ў� ѓ� ЎвЂљ� � В°� � � …� � С•� � � � � � С”� � В°)"
	qmToBigEndian((uint8_t)code, tx_data+1); // � � С”� � С•� � Т‘ � � С—� � В°� Ў� ‚� � В°� � С�� � Вµ� ЎвЂљ� Ў� ‚� � В°
	switch (module) {
	case RxRadiopath:
	case TxRadiopath: {
		if (module == RxRadiopath)
			tx_address = 0x50;
		else
			tx_address = 0x80;

		switch (code) {
		case 1:
			qmToBigEndian(value.frequency, tx_data+tx_data_len); tx_data_len += 4;
			break;
		case 2:
			qmToBigEndian((uint8_t)value.radio_mode, tx_data+tx_data_len); tx_data_len += 1;
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
		case AudioSignalNumber:
			qmToBigEndian((uint8_t)value.signal_number, tx_data+tx_data_len);
			tx_data_len += 1;
			break;
		case AudioSignalDuration:
			qmToBigEndian((uint8_t)value.signal_duration, tx_data+tx_data_len);
			tx_data_len += 1;
			break;
		case AudioSignalMicLevel:
			qmToBigEndian((uint8_t)value.signal_mic_level, tx_data+tx_data_len);
			tx_data_len += 1;
			break;
		default: QM_ASSERT(0);
		}
		break;
	}
	// � � Т‘� � В»� Ў� Џ � � Сџ� � Сџ� їС—� … � � В§
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
		case RadioModeVirtualPpps:
		case RadioModeVirtualRvv:
		{
			qmToBigEndian((uint8_t)value.param, tx_data+tx_data_len); // sync
			tx_data_len += 1;
			break;
		}
		case PswfRxFrequency:
		case PswfRxFreqSignal:
		{
			qmToBigEndian(value.frequency, tx_data+tx_data_len);
			tx_data_len += 4;
			if (sms_counter > 38 && sms_counter < 77)
			{
				uint8_t fstn = 0;
				if (virtual_mode == true)
					fstn = calcFstn(ContentSms.R_ADR,ContentSms.S_ADR,ContentSms.RN_KEY,d.day,t.hours,t.minutes,t.seconds,sms_counter - 39);
				else
					fstn = calcFstn(ContentSms.R_ADR,ContentSms.S_ADR,ContentSms.RN_KEY,date_time[0],date_time[1],date_time[2],date_time[3],sms_counter - 39); // TODO: fix that;
				QNB_RX++;
				qmDebugMessage(QmDebug::Dump, "sendCommandEasy() FSTN: %d", fstn);
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
    case VirtualPps:
    {
    	tx_address = 0x64;
    	qmToBigEndian((uint8_t)0, tx_data+tx_data_len);
    	tx_data_len += 1;
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
		qmToBigEndian((uint8_t)2, tx_data+0); // � � С‘� � � …� � Т‘� � С‘� � С”� � В°� ЎвЂљ� � С•� Ў� ‚: "� � С”� � С•� � С�� � В°� � � …� � Т‘� � В° (� ЎС“� Ў� ѓ� ЎвЂљ� � В°� � � …� � С•� � � � � � С”� � В°)"
		qmToBigEndian((uint8_t)code, tx_data+1); // � � С”� � С•� � Т‘ � � С—� � В°� Ў� ‚� � В°� � С�� � Вµ� ЎвЂљ� Ў� ‚� � В°
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
			case AudioSignalNumber:
				qmToBigEndian((uint8_t)value.signal_number, tx_data+tx_data_len);
				tx_data_len += 1;
				break;
			case AudioSignalDuration:
				qmToBigEndian((uint8_t)value.signal_duration, tx_data+tx_data_len);
				tx_data_len += 1;
				break;
			case AudioSignalMicLevel:
				qmToBigEndian((uint8_t)value.signal_mic_level, tx_data+tx_data_len);
				tx_data_len += 1;
				break;
			default: QM_ASSERT(0);
			}
			break;
		}
		// � � Т‘� � В»� Ў� Џ � � Сџ� � Сџ� їС—� … � � В§
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
					uint8_t fstn  = 0;
			        if (virtual_mode == true)
			        	fstn = calcFstn(ContentSms.R_ADR,ContentSms.S_ADR,ContentSms.RN_KEY,d.day,t.hours,t.minutes,t.seconds,sms_counter - 39);
			        else
			        	fstn = calcFstn(ContentSms.R_ADR,ContentSms.S_ADR,ContentSms.RN_KEY,date_time[0],date_time[1],date_time[2],date_time[3],sms_counter - 39); // TODO: fix that;
					QNB_RX++;
					qmDebugMessage(QmDebug::Dump, "sendCommand() FSTN: %d", fstn);
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

    ContentGuc.Coord = isGpsGuc;

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

    int crc32_len = ContentGuc.NUM_com; // � Ў� ‚� � Вµ� � В°� � В»� Ў� Љ� � � …� � С•� � Вµ � � С”� � С•� � В»� � С‘� ЎвЂЎ� � Вµ� Ў� ѓ� ЎвЂљ� � � � � � С• � � С”� � С•� � С�� � В°� � � …� � Т‘
    int real_len = crc32_len;

    // � � вЂ™� ЎвЂ№� � В±� � С•� Ў� ‚ � � С”� � С•� � В»� � С‘� ЎвЂЎ� � Вµ� Ў� ѓ� ЎвЂљ� � � � � � В° � � С—� � Вµ� Ў� ‚� � Вµ� � Т‘� � В°� � � � � � В°� � Вµ� � С�� ЎвЂ№� ЎвЂ¦ � � В±� � В°� � в„–� ЎвЂљ� � С•� � � �  � Ў� ѓ � � С”� � С•� � С•� Ў� ‚� � Т‘� � С‘� � � …� � В°� ЎвЂљ� � В°� � С�� � С‘ � � С‘� � В»� � С‘ � � В±� � Вµ� � В· � � С—� � С• � Ў� ‚� � Вµ� � С–� � В»� � В°� � С�� � Вµ� � � …� ЎвЂљ� ЎС“
    if (isGpsGuc){
        if (ContentGuc.NUM_com <= 6) ContentGuc.NUM_com = 6;
        if ((ContentGuc.NUM_com > 6) && (ContentGuc.NUM_com <= 10))    ContentGuc.NUM_com = 10;
        if ((ContentGuc.NUM_com > 10) && (ContentGuc.NUM_com <= 26))   ContentGuc.NUM_com = 26;
        if ((ContentGuc.NUM_com > 26) && (ContentGuc.NUM_com <= 100))  ContentGuc.NUM_com = 100;
    }
    else
    {
        if (ContentGuc.NUM_com <= 5) ContentGuc.NUM_com = 5;
        if ((ContentGuc.NUM_com > 5) && (ContentGuc.NUM_com <= 11))    ContentGuc.NUM_com = 11;
        if ((ContentGuc.NUM_com > 11) && (ContentGuc.NUM_com <= 25))   ContentGuc.NUM_com = 25;
        if ((ContentGuc.NUM_com > 25) && (ContentGuc.NUM_com <= 100))  ContentGuc.NUM_com = 100;
    }

    for(int i = 0; i < ContentGuc.NUM_com; i++) {
        qmToBigEndian((uint8_t)ContentGuc.command[i], tx_data + tx_data_len);
        ++tx_data_len;
    }

    // � � С•� � В±� Ў� ‚� � В°� � В±� � С•� ЎвЂљ� � С”� � В° � � С‘ � � С—� � С•� � В»� ЎС“� ЎвЂЎ� � Вµ� � � …� � С‘� � Вµ � � С”� � С•� � С•� Ў� ‚� � Т‘� � С‘� � � …� � В°� ЎвЂљ, � � Т‘� � С•� � В±� � В°� � � � � � В»� � Вµ� � � …� � С‘� � Вµ � � � �  � � С‘� Ў� ѓ� ЎвЂ¦� � С•� � Т‘� � � …� ЎвЂ№� � в„– � � С�� � В°� Ў� ѓ� Ў� ѓ� � С‘� � � �  � � Т‘� � В»� Ў� Џ � � В·� � В°� ЎвЂ°� � С‘� ЎвЂљ� ЎвЂ№  crc32 � Ў� ѓ� ЎС“� � С�� � С•� � в„– (� � вЂќ� � С’� � Сњ� � Сњ� � В«� � вЂў + � � С™� � С›� � С›� � вЂќ� � пїЅ� � Сњ� � С’� � Сћ� � В«)
    if (isGpsGuc)
    {
       uint8_t coord[9] = {0,0,0,0,0,0,0,0,0};
       getGpsGucCoordinat(coord);
       for(int i = 0;i<9;i++){
           ContentGuc.command[ContentGuc.NUM_com+i] = coord[i];
           qmToBigEndian((uint8_t)coord[i],tx_data + tx_data_len);
           ++tx_data_len;
       }
    }

    // � � � � � ЎвЂ№� � В±� � С•� Ў� ‚ � � Т‘� � В»� � С‘� � � …� � � …� ЎвЂ№ � � С”� � С•� � Т‘� � С‘� Ў� ‚� ЎС“� � Вµ� � С�� � С•� � С–� � С• � � С�� � В°� Ў� ѓ� Ў� ѓ� � С‘� � � � � � В°
     crc32_len = (isGpsGuc) ? (ContentGuc.NUM_com + 9) : (ContentGuc.NUM_com);

     std::vector<bool> data_guc;

     if (isGpsGuc)
     {
    	 uint8_t mas[9]; int index = 0;
    	 for(int i = crc32_len - 9; i< crc32_len;i++)
    	 {
    		 mas[index] = ContentGuc.command[i];
    		 ++index;
    		 ContentGuc.command[i] = 0;
    	 }

    	 uint8_t value[120];
    	 for(int i = 0; i< ContentGuc.NUM_com;i++) value[i] = ContentGuc.command[i];
    	 for(int i = 9;i < 9+ContentGuc.NUM_com;i++) {
    		  ContentGuc.command[i] = value[i- 9];
    	 }
         for(int i = 0; i<9;i++) {
             ContentGuc.command[i] = mas[i];
             if (i != 8)
             pack_manager->addBytetoBitsArray(ContentGuc.command[i],data_guc,8);
         }

         bool quadrant = ContentGuc.command[8] & 1;
         data_guc.push_back(quadrant);
         quadrant = ContentGuc.command[8] & (1 << 1);
         data_guc.push_back(quadrant);

         for(int i = 0; i<real_len;i++){
             pack_manager->addBytetoBitsArray(ContentGuc.command[i+9],data_guc,7);
         }
     }

    // � Ў� ѓ� � Т‘� � � � � � С‘� � С– � � С�� � В°� Ў� ѓ� Ў� ѓ� � С‘� � � � � � В° � � Т‘� � В»� Ў� Џ crc32-� Ў� ѓ� ЎС“� � С�� � С�� ЎвЂ№
    if (isGpsGuc){
        pack_manager->getArrayByteFromBit(data_guc,ContentGuc.command);
        crc32_len = data_guc.size() / 8;

        for(int i = 0; i< crc32_len;i++){
        	qmDebugMessage(QmDebug::Dump,"packet guc: %d", ContentGuc.command[i]);
        }
    }
    else
    //for(int i = 0; i < crc32_len;i++)
    {
    	std::vector<bool> data;
    	for(int i = 0; i<ContentGuc.NUM_com;i++) pack_manager->addBytetoBitsArray(ContentGuc.command[i],data,7);
    	for(int i = 0; i<crc32_len;i++) pack_manager->getArrayByteFromBit(data,ContentGuc.command);

//    	int sdvig  = (i+1) % 8;
//    	if (sdvig != 0)
//    	ContentGuc.command[i] = (ContentGuc.command[i] << sdvig) + (ContentGuc.command[i+1] >> (7 -  sdvig));
    }

    if (!isGpsGuc)
    {
    	crc32_len = ((real_len*7)/8); uint8_t ost =  (real_len*7)% 8;
    	if (ost !=0)
    	{
    		uint8_t mask = 0;
    		for(int i = 0; i<ost;i++) mask +=  1 << (7 - i);
    		crc32_len +=1;
    		ContentGuc.command[crc32_len-1] = ContentGuc.command[crc32_len-1] & mask;
    	}
    }
     // � � Т‘� � С•� � В±� � В°� � � � � � В»� � Вµ� � � …� � С‘� � Вµ crc32 � � С” � � С—� � В°� � С”� � Вµ� ЎвЂљ� ЎС“ � � Т‘� � В°� � � …� � � …� ЎвЂ№� ЎвЂ¦
     uint32_t crc = pack_manager->CRC32(ContentGuc.command, crc32_len);
     qmToBigEndian((uint32_t)crc, tx_data + tx_data_len);
     tx_data_len += 4;

    transport->transmitFrame(tx_address, tx_data, tx_data_len);
}

void DspController::processReceivedFrame(uint8_t address, uint8_t* data, int data_len) {
	if (data_len < DEFAULT_PACKET_HEADER_LEN)
		return;

	uint8_t indicator = qmFromBigEndian<uint8_t>(data+0);
	uint8_t code = qmFromBigEndian<uint8_t>(data+1);
	uint8_t *value_ptr = data + 2;
	int value_len = data_len - 2;

	switch (address) {
	case 0x11: {
		if ((indicator == 5) && (code == 2) && (value_len == 6)) // � � С‘� � � …� � С‘� ЎвЂ� � � С‘� � В°� ЎвЂљ� � С‘� � � � � � � …� � С•� � Вµ � Ў� ѓ� � С•� � С•� � В±� ЎвЂ°� � Вµ� � � …� � С‘� � Вµ � Ў� ѓ � ЎвЂ� � � С‘� ЎвЂћ� Ў� ‚� � С•� � � � � � С•� � в„– � � С‘� � � …� ЎвЂћ� � С•� Ў� ‚� � С�� � В°� ЎвЂ� � � С‘� � Вµ� � в„– � � С• � � С—� Ў� ‚� � С•� Ўв‚¬� � С‘� � � � � � С”� � Вµ ?
			processStartup(qmFromBigEndian<uint16_t>(value_ptr+0), qmFromBigEndian<uint16_t>(value_ptr+2), qmFromBigEndian<uint16_t>(value_ptr+4));
		break;
	}
	case 0x31: {
    	value_ptr -= 1; // � � С”� � С•� Ў� ѓ� ЎвЂљ� ЎвЂ№� � В»� � � …� � С•� � Вµ � � С—� Ў� ‚� � Вµ� � � � � Ў� ‚� � В°� ЎвЂ°� � Вµ� � � …� � С‘� � Вµ � � � �  � � � …� � Вµ� Ў� ѓ� ЎвЂљ� � В°� � � …� � Т‘� � В°� Ў� ‚� ЎвЂљ� � � …� ЎвЂ№� � в„– � ЎвЂћ� � С•� Ў� ‚� � С�� � В°� ЎвЂљ � � С”� � В°� � Т‘� Ў� ‚� � В°
    	value_len += 1; // � � С”� � С•� Ў� ѓ� ЎвЂљ� ЎвЂ№� � В»� � � …� � С•� � Вµ � � С—� Ў� ‚� � Вµ� � � � � Ў� ‚� � В°� ЎвЂ°� � Вµ� � � …� � С‘� � Вµ � � � �  � � � …� � Вµ� Ў� ѓ� ЎвЂљ� � В°� � � …� � Т‘� � В°� Ў� ‚� ЎвЂљ� � � …� ЎвЂ№� � в„– � ЎвЂћ� � С•� Ў� ‚� � С�� � В°� ЎвЂљ � � С”� � В°� � Т‘� Ў� ‚� � В°
    	if (indicator == 5) {
    		uint8_t subdevice_code = (uint8_t)qmFromBigEndian<int8_t>(value_ptr+0);
    		uint8_t error_code = (uint8_t)qmFromBigEndian<int8_t>(value_ptr+2);
    		hardwareFailed.emit(subdevice_code, error_code);
    	}
    	break;
	}
	case 0x51:
	case 0x81: {
		if ((indicator == 3) || (indicator == 4)) { // "� � С”� � С•� � С�� � В°� � � …� � Т‘� � В° � � � � � ЎвЂ№� � С—� � С•� � В»� � � …� � Вµ� � � …� � В°", "� � С”� � С•� � С�� � В°� � � …� � Т‘� � В° � � � …� � Вµ � � � � � ЎвЂ№� � С—� � С•� � В»� � � …� � Вµ� � � …� � В°" ?
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
    	LogicPswfModes(data,indicator,data_len);
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
    	qmDebugMessage(QmDebug::Dump, "processReceivedFrame() 0x7B received, %d" ,indicator);
        if (indicator == 22)
        {
        	if (ContentGuc.stage == GucRx)
        	{
        		goToVoice();
        	}
        	if (ContentGuc.stage == GucTx)
        	{
        		startGucRecieving();
        		ContentGuc.stage = GucTx;
        		startRxQuit();
        	}
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
                ContentGuc.R_ADR = ((data[2] & 0xF8) >> 3);
            	ContentGuc.uin   = ((data[4] & 0x1) << 7) + ((data[5] & 0xFE) >> 1);
                isGpsGuc = data[5] & 0x1;
                if (ContentGuc.stage == GucTx)
                {
                	ContentGuc.S_ADR = ((data[2] & 0x7) << 2) + ((data[3] & 0xC0) >> 6);
                	recievedGucQuitForTransm(ContentGuc.S_ADR);
                	goToVoice();
                    //stationModeIsCompleted();
                }
            	else{
            		qmDebugMessage(QmDebug::Dump, "0x6B R_ADR %d : ", ContentGuc.R_ADR);
            		std::vector<uint8_t> guc;
            		for(int i = 0;i<data_len;i++)
            		{
            			qmDebugMessage(QmDebug::Dump, "0x6B recieved frame: %d , num %d", data[i],i);
            			guc.push_back(data[i]);
            		}
                    guc_vector.push_back(guc);
                    //guc_timer->start();
                    (isGpsGuc) ? recievedGucResp(1) : recievedGucResp(0);
                    startGucTransmitting();
            		sendGucQuit();
            	}
            }
        }
        break;
    }
    case 0x6F:
    {
    	value_ptr -= 1; // � � С”� � С•� Ў� ѓ� ЎвЂљ� ЎвЂ№� � В»� � � …� � С•� � Вµ � � С—� Ў� ‚� � Вµ� � � � � Ў� ‚� � В°� ЎвЂ°� � Вµ� � � …� � С‘� � Вµ � � � �  � � � …� � Вµ� Ў� ѓ� ЎвЂљ� � В°� � � …� � Т‘� � В°� Ў� ‚� ЎвЂљ� � � …� ЎвЂ№� � в„– � ЎвЂћ� � С•� Ў� ‚� � С�� � В°� ЎвЂљ � � С”� � В°� � Т‘� Ў� ‚� � В°
    	value_len += 1; // � � С”� � С•� Ў� ѓ� ЎвЂљ� ЎвЂ№� � В»� � � …� � С•� � Вµ � � С—� Ў� ‚� � Вµ� � � � � Ў� ‚� � В°� ЎвЂ°� � Вµ� � � …� � С‘� � Вµ � � � �  � � � …� � Вµ� Ў� ѓ� ЎвЂљ� � В°� � � …� � Т‘� � В°� Ў� ‚� ЎвЂљ� � � …� ЎвЂ№� � в„– � ЎвЂћ� � С•� Ў� ‚� � С�� � В°� ЎвЂљ � � С”� � В°� � Т‘� Ў� ‚� � В°
    	switch (indicator) {
    	case 30: {
    		ModemPacketType type = (ModemPacketType)qmFromBigEndian<uint8_t>(value_ptr+1);
    		int data_offset;
    		if (type == modempacket_packHead)
    			data_offset = 6;
    		else
    			data_offset = 4;
    		uint8_t snr = (uint8_t)qmFromBigEndian<int8_t>(value_ptr+2);
    		uint8_t errors = (uint8_t)qmFromBigEndian<int8_t>(value_ptr+3);
    		ModemBandwidth bandwidth = (ModemBandwidth)qmFromBigEndian<uint8_t>(value_ptr+0);
    		receivedModemPacket.emit(type, snr, errors, bandwidth, value_ptr + data_offset, value_len - data_offset);
    		break;
    	}
    	case 31: {
    		if (!(value_len >= 1))
    			break;
    		ModemPacketType type = (ModemPacketType)qmFromBigEndian<uint8_t>(value_ptr+1);
    		failedRxModemPacket.emit(type);
    		break;
    	}
    	case 32: {
    		ModemPacketType type = (ModemPacketType)qmFromBigEndian<uint8_t>(value_ptr+1);
    		int data_offset;
    		if (type == modempacket_packHead)
    			data_offset = 6;
    		else
    			data_offset = 4;
    		uint8_t snr = (uint8_t)qmFromBigEndian<int8_t>(value_ptr+2);
    		uint8_t errors = (uint8_t)qmFromBigEndian<int8_t>(value_ptr+3);
    		ModemBandwidth bandwidth = (ModemBandwidth)qmFromBigEndian<uint8_t>(value_ptr+0);
    		if (type == modempacket_packHead) {
    			uint8_t param_signForm = qmFromBigEndian<uint8_t>(value_ptr+4);
    			uint8_t param_packCode = qmFromBigEndian<uint8_t>(value_ptr+5);
        		startedRxModemPacket_packHead.emit(snr, errors, bandwidth, param_signForm, param_packCode, value_ptr + data_offset, value_len - data_offset);
    		} else {
        		startedRxModemPacket.emit(type, snr, errors, bandwidth, value_ptr + data_offset, value_len - data_offset);
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
    	value_ptr -= 1; // � � С”� � С•� Ў� ѓ� ЎвЂљ� ЎвЂ№� � В»� � � …� � С•� � Вµ � � С—� Ў� ‚� � Вµ� � � � � Ў� ‚� � В°� ЎвЂ°� � Вµ� � � …� � С‘� � Вµ � � � �  � � � …� � Вµ� Ў� ѓ� ЎвЂљ� � В°� � � …� � Т‘� � В°� Ў� ‚� ЎвЂљ� � � …� ЎвЂ№� � в„– � ЎвЂћ� � С•� Ў� ‚� � С�� � В°� ЎвЂљ � � С”� � В°� � Т‘� Ў� ‚� � В°
    	value_len += 1; // � � С”� � С•� Ў� ѓ� ЎвЂљ� ЎвЂ№� � В»� � � …� � С•� � Вµ � � С—� Ў� ‚� � Вµ� � � � � Ў� ‚� � В°� ЎвЂ°� � Вµ� � � …� � С‘� � Вµ � � � �  � � � …� � Вµ� Ў� ѓ� ЎвЂљ� � В°� � � …� � Т‘� � В°� Ў� ‚� ЎвЂљ� � � …� ЎвЂ№� � в„– � ЎвЂћ� � С•� Ў� ‚� � С�� � В°� ЎвЂљ � � С”� � В°� � Т‘� Ў� ‚� � В°
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

    case 0x65:
    {
    	// get number of the catch packet ...
#ifndef PORT__PCSIMULATOR
    	if (!virtual_mode) return;
    	if (indicator == 5)
    	{
			addSeconds(&t);

    		if (RtcTxRole)
    		{
    			if (count_VrtualTimer <= VrtualTimerMagic)
    			{
    				//qmDebugMessage(QmDebug::Dump, "0x65 frame");
    				qmDebugMessage(QmDebug::Dump, "0x65 count_VrtualTimer %d",count_VrtualTimer);
    				qmDebugMessage(QmDebug::Dump, "0x65 RtcTxCounter %d",RtcTxCounter);

    				if (RtcTxCounter)
    					++RtcTxCounter;

    				if (IsStart(t.seconds))
    				{
    					freqVirtual = getCommanderFreq(ContentPSWF.RN_KEY,t.seconds,d.day,t.hours,t.minutes);
    					qmDebugMessage(QmDebug::Dump, "0x65 frame %d %d",t.minutes,t.seconds);
    					RtcTxCounter = 1;
    					++count_VrtualTimer;
    					if (count_VrtualTimer > VrtualTimerMagic)
    					{
    						qmDebugMessage(QmDebug::Dump, "0x65 changeFrequency()");
    						addSeconds(&t);
    						if (radio_state == radiostatePswf)
    							changePswfFrequency();
    						if (radio_state == radiostateSms)
    							changeSmsFrequency();
    					}
    					qmDebugMessage(QmDebug::Dump, "0x65 frame %d %d",t.minutes,t.seconds);
    				}

    				if (RtcTxCounter == 5)
    				{
    					sendSynchro(freqVirtual,count_VrtualTimer);
    					qmDebugMessage(QmDebug::Dump, "0x65 sendSynchro");
    				}
    				qmDebugMessage(QmDebug::Dump, "0x65 count_VrtualTimer %d",count_VrtualTimer);
    				qmDebugMessage(QmDebug::Dump, "0x65 RtcTxCounter %d",RtcTxCounter);

    			}
    			else
    			{
    				if (radio_state == radiostatePswf)
    					changePswfFrequency();
    				if (radio_state == radiostateSms)
    					changeSmsFrequency();
    			}
    		}
    		//-------- RXROLE------------------------------------
    		if (RtcRxRole)
    		{
    			if (count_VrtualTimer > 10)
    			{
    				if (radio_state == radiostatePswf)
    					changePswfFrequency();
    				if (radio_state == radiostateSms)
    					changeSmsFrequency();
    			}
    			else
    			{
    				if (IsStart(t.seconds))
    					++count_VrtualTimer;
    			}
    		}
    	}
#endif
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
    ContentSms.Frequency =  getFrequency(1); // sms = 1
    ContentSms.indicator = 20;
    ContentSms.SNR =  7;

    int time[4] = {0,0,0,0};

    if (virtual_mode)
    {
    	time[0] = d.day;
    	time[1] = t.hours;
    	time[2] = t.minutes;
    	time[3] = t.seconds;
    }
    else
    {
    	for(int i = 0; i<4;i++) time[i] = date_time[i];
    }

    if (sms_counter >= 19 && sms_counter <= 38){
    	ContentSms.L_CODE = navigator->Calc_LCODE_SMS(
    	ContentSms.R_ADR,
		ContentSms.S_ADR,
		wzn_value,
		ContentSms.RN_KEY,
		time[0],
		time[1],
		time[2],
		time[3]);
    }
    else
    {
    	ContentSms.L_CODE = navigator->Calc_LCODE(
    	ContentSms.R_ADR,
		ContentSms.S_ADR,
		sms_counter,
		ContentSms.RN_KEY,
		time[0],
		time[1],
		time[2],
		time[3]);
    }

    qmDebugMessage(QmDebug::Dump, "LCODE: %d",ContentSms.L_CODE);

    if (sms_counter > 38 && sms_counter < 76)
        ContentSms.TYPE = 1;
    else
        ContentSms.TYPE = 0;

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
    if (sms_counter < 19 && SmsLogicRole == SmsRoleTx)
    {
    	static int counter = 0;
    	if (counter == 18) counter = 0;
    	++counter;

    	qmToBigEndian((uint8_t)ContentSms.SNR, tx_data+tx_data_len);    ++tx_data_len;
        qmToBigEndian((uint8_t)ContentSms.R_ADR, tx_data+tx_data_len);  ++tx_data_len;
    	qmToBigEndian((uint8_t)ContentSms.S_ADR, tx_data+tx_data_len);  ++tx_data_len;
    	qmToBigEndian((uint8_t)counter, tx_data+tx_data_len);           ++tx_data_len;
    	qmToBigEndian((uint8_t)ContentSms.L_CODE, tx_data+tx_data_len); ++tx_data_len;
    }
    // tx
    if ((sms_counter > 38 && sms_counter < 77) && (SmsLogicRole == SmsRoleTx))
    {
        uint8_t FST_N = 0;
        if (virtual_mode)
        	FST_N = calcFstn(ContentSms.R_ADR,ContentSms.S_ADR,ContentSms.RN_KEY,d.day,t.hours,t.minutes,t.seconds,sms_counter - 39);
        else
        	FST_N = calcFstn(ContentSms.R_ADR,ContentSms.S_ADR,ContentSms.RN_KEY,date_time[0],date_time[1],date_time[2],date_time[3],sms_counter - 39);
        ++QNB;
        qmDebugMessage(QmDebug::Dump, "sendSms() FSTN: %d", FST_N);
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
    if ((sms_counter > 19 && sms_counter < 38) && (SmsLogicRole == SmsRoleRx))
    {
        int wzn = wzn_value;

    	qmToBigEndian((uint8_t)ContentSms.SNR, tx_data+tx_data_len);   ++tx_data_len;
    	qmToBigEndian((uint8_t)ContentSms.R_ADR, tx_data+tx_data_len); ++tx_data_len;
    	qmToBigEndian((uint8_t)ContentSms.S_ADR, tx_data+tx_data_len); ++tx_data_len;
    	// todo: � � С—� � С•� � С�� � Вµ� � � …� Ў� Џ� � В» � � С�� � Вµ� Ў� ѓ� ЎвЂљ� � В°� � С�� � С‘
    	qmToBigEndian((uint8_t)wzn, tx_data+tx_data_len);              ++tx_data_len;

    	qmDebugMessage(QmDebug::Dump, "SADR: %d",ContentSms.S_ADR);
    	qmDebugMessage(QmDebug::Dump, "RADR: %d",ContentSms.R_ADR);
    	qmDebugMessage(QmDebug::Dump, "LCODE: %d",ContentSms.L_CODE);

    	qmToBigEndian((uint8_t)ContentSms.L_CODE, tx_data+tx_data_len);
    	++tx_data_len;
    }

    if ((sms_counter > 76 && sms_counter < 83) && (SmsLogicRole == SmsRoleRx))
    {
    	qmToBigEndian((uint8_t)ContentSms.SNR, tx_data+tx_data_len);   ++tx_data_len;
    	qmToBigEndian((uint8_t)ContentSms.R_ADR, tx_data+tx_data_len); ++tx_data_len;
    	qmToBigEndian((uint8_t)ContentSms.S_ADR, tx_data+tx_data_len); ++tx_data_len;
    	qmToBigEndian((uint8_t)ack, tx_data+tx_data_len);              ++tx_data_len;

    	uint8_t ack_code  = calc_ack_code(ack);
    	qmToBigEndian((uint8_t)ack_code, tx_data+tx_data_len);         ++tx_data_len;
    }

    transport->transmitFrame(tx_address, tx_data, tx_data_len);
}

void DspController::startGucTransmitting()
{
    qmDebugMessage(QmDebug::Dump, "startGucTransmitting");
    QM_ASSERT(is_ready);

    ParameterValue comandValue;
    comandValue.radio_mode = RadioModeOff;
    sendCommandEasy(RxRadiopath, RxRadioMode, comandValue);

    comandValue.guc_mode = RadioModeSazhenData; // mode 11
    sendCommandEasy(TxRadiopath, TxRadioMode, comandValue);

    comandValue.frequency = freqGucValue;
    sendCommandEasy(RxRadiopath, RxFrequency, comandValue); //  � ·� °С‡� µ� ј � ·� ґ� µСЃСЊ � І� їСЂ� ё� ЅС� � ё� ї� µ ..
    sendCommandEasy(TxRadiopath, TxFrequency, comandValue);
    radio_state = radiostateGucTxPrepare;
    gucTxStateSync = 0;
}

void DspController::prevTime()
{
	if (virtual_mode)
	{
		qmDebugMessage(QmDebug::Dump, "prevTime() seconds: %d", t.seconds);
		t.seconds = t.seconds - 1;
		qmDebugMessage(QmDebug::Dump, "prevTime() seconds: %d", t.seconds);
	}
	else
	{
		qmDebugMessage(QmDebug::Dump, "prevTime() seconds: %d", date_time[3]);
		date_time[3]  = date_time[3] - 1;
		qmDebugMessage(QmDebug::Dump, "prevTime() seconds: %d", date_time[3]);
	}
}

void DspController::getZone()
{
	for(int i = 0; i<18;i++)
	{
		if ( waveZone[i] >= 0  && waveZone[i] < 6  ) syncro_recieve[i] = 0;
		if ( waveZone[i] >  5  && waveZone[i] < 12 ) syncro_recieve[i] = 1;
		if ( waveZone[i] >= 12 && waveZone[i] < 18 ) syncro_recieve[i] = 2;
		if ( waveZone[i] >= 18 && waveZone[i] < 24 ) syncro_recieve[i] = 3;
		if ( waveZone[i] >= 24 && waveZone[i] < 30 ) syncro_recieve[i] = 4;
    }
}

uint8_t DspController::getSmsCounter()
{
    return sms_counter;
}

bool DspController::generateSmsReceived()
{
    // 1. params for storage operation

	//qmDebugMessage(QmDebug::Dump,"sms data %d:",  recievedSmsBuffer.size());

    int data[255];
    uint8_t crc_calcs[100];
    uint8_t packet[110];

    std::memset(data,0,sizeof(data));
    std::memset(crc_calcs,0,sizeof(crc_calcs));
    std::memset(packet,0, sizeof(packet));

    int count  = 0;

    // 2. copy data in int mas
    for(int i = 0; i< 255;i++) data[i] = smsDataPacket[i];

    // 3. get a weight for data
    int temp = eras_dec_rs(data,rs_data_clear,&rs_255_93);

    qmDebugMessage(QmDebug::Dump,"Result of erase %d:", temp);

    // 4. check valid value
    if (temp >= 0)
    {
        // 5. copy 89 bytes for  crc_calcs massive
        std::copy(&data[0],&data[89],crc_calcs);
        // 6. calculate CRC32 for 89 bytes
        uint32_t code_calc = pack_manager->CRC32(crc_calcs,89);

        // 7. calculate crc code for crc get and crc calculate
        uint32_t code_get = 0;
        int k = 3;

        while(k >=0){
            code_get += (data[89+k] & 0xFF) << (8*k);
            k--;
        }

        qmDebugMessage(QmDebug::Dump," Calc sms  code  crc %d %d:", code_get,code_calc);

        if (code_get != code_calc)
        {
            smsFailed(3);
            ack = 99;
            return false;
        }
        else
        {
        	indexSmsLen = 100;
        	for(int i = 0;i<100;i++)
        	{
        		if (crc_calcs[i] == 0) {
        			indexSmsLen = i;
        			break;
        		}
        	}

          // 8. calculate text without CRC32 code
          pack_manager->decompressMass(crc_calcs,89,packet,110,7);

          if (indexSmsLen == 88){
          	indexSmsLen = 100;
          	for(int i = 0;i<100;i++)
          	{
          		if (packet[i] == 0) {
          			indexSmsLen = i;
          			break;
          		}
          	}
          }

          // 9. interpretate to Win1251 encode
          pack_manager->to_Win1251(packet);

          // 10. create str consist data split ''

          std::copy(&packet[0],&packet[100],sms_content);
          ack = 73;
          for (uint8_t i = indexSmsLen; i <= 100; i++)
            sms_content[i] = 0;
          return true;
        }

    }
    else
    {
        // wrong params
        smsFailed(3);
        ack = 99;
        return false;
    }
}

int DspController::wzn_change(std::vector<int> &vect)
{
    int wzn_mas[5] = {0,0,0,0,0};
    for (uint8_t i = 0; i < vect.size() - 1; i++)
    {
        if (vect[i] <= 4) wzn_mas[vect[i]] += 1;
    }
    int index = 0;
    for (int i = 1; i < 5; i++)
    {
        if (wzn_mas[index] < wzn_mas[i]) index = i;
    }
    return index;
}

int DspController::calcFstn(int R_ADR, int S_ADR, int RN_KEY, int DAY, int HRS, int MIN, int SEC, int QNB)
{
    int FST_N = (R_ADR + S_ADR + RN_KEY + SEC + MIN + HRS + DAY + QNB) % 100;
//    qmDebugMessage(QmDebug::Dump, "calcFstn() R_ADR: %d", R_ADR);
//    qmDebugMessage(QmDebug::Dump, "calcFstn() S_ADR: %d", S_ADR);
//    qmDebugMessage(QmDebug::Dump, "calcFstn() RN_KEY: %d", RN_KEY);
//    qmDebugMessage(QmDebug::Dump, "calcFstn() DAY: %d", DAY);
//    qmDebugMessage(QmDebug::Dump, "calcFstn() HRS: %d", HRS);
//    qmDebugMessage(QmDebug::Dump, "calcFstn() MIN: %d", MIN);
//    qmDebugMessage(QmDebug::Dump, "calcFstn() SEC: %d", SEC);
//    qmDebugMessage(QmDebug::Dump, "calcFstn() QNB: %d", QNB);
//    qmDebugMessage(QmDebug::Dump, "calcFstn() FST_N: %d", FST_N);
    return FST_N;
}

int DspController::check_rx_call(int* wzn)
{
	int snr_mas[5] = {0,0,0,0,0};
	int wzn_mas[5] = {0,0,0,0,0};

    int cnt_index = 0;
    for(int i = 0; i<18;i++)
    {
       if (syncro_recieve.at(i) == i){
           ++cnt_index;
           wzn_mas[waveZone[i]] += 1;
           snr_mas[waveZone[i]] += snr.at(i);
       //qmDebugMessage(QmDebug::Dump, "syncro_recieve value = %d", syncro_recieve.at(i));
       }
    }

    qmDebugMessage(QmDebug::Dump, "check rx call = %d", cnt_index);

    if (cnt_index < 2) return false;

    int index = 0;
    for(int i = 1; i < 5; i++)
    {
        if (wzn_mas[index] < wzn_mas[i])
        	index = i;
        else
        	if(wzn_mas[index] == wzn_mas[i])
        {
        	if (snr_mas[i] >= snr_mas[index])
        		index = i;
        }
    }
    *wzn = index;

    return true;
}

uint8_t DspController::calc_ack_code(uint8_t ack)
{
	uint8_t ACK_CODE  = 0;

	if (virtual_mode == true)
		ACK_CODE = (ContentSms.R_ADR + ContentSms.S_ADR + ack + ContentSms.RN_KEY +
				   d.day + t.hours + t.minutes + t.seconds) % 100;
	else
		ACK_CODE = (ContentSms.R_ADR + ContentSms.S_ADR + ack + ContentSms.RN_KEY +
				   date_time[0] + date_time[1]+ date_time[2] + date_time[3]) % 100;

	qmDebugMessage(QmDebug::Dump, "radr = %d", ContentSms.R_ADR);
	qmDebugMessage(QmDebug::Dump, "sadr = %d", ContentSms.S_ADR);
	qmDebugMessage(QmDebug::Dump, "ack  = %d", ack);
	qmDebugMessage(QmDebug::Dump, "radr = %d", ContentSms.RN_KEY);
	qmDebugMessage(QmDebug::Dump, "radr = %d", d.day,t.hours,t.minutes,t.seconds);
    return ACK_CODE;
}

char* DspController::getSmsContent()
{
	return sms_content;
}

void DspController::setPswfRx()
{
	ParameterValue comandValue;
	comandValue.radio_mode = RadioModeOff;
	sendCommand(TxRadiopath, TxRadioMode, comandValue);
	comandValue.pswf_indicator = RadioModePSWF;
	sendCommand(RxRadiopath, RxRadioMode, comandValue);
}

void DspController::setPswfTx()
{
    ParameterValue comandValue;
    comandValue.radio_mode = RadioModeOff;
    sendCommandEasy(RxRadiopath, RxRadioMode, comandValue);
    comandValue.pswf_indicator = RadioModePSWF;
    sendCommandEasy(TxRadiopath, TxRadioMode, comandValue);
}

void DspController::startPSWFReceiving()
{
	qmDebugMessage(QmDebug::Dump, "startPSWFReceiving(%d)", ack);
	QM_ASSERT(is_ready);

//	for(int i = 0; i<30;i++) pswfDataPacket[i] = 255;

	if (virtual_mode == true)
		startVirtualPpsModeRx();
	else
		setPswfRx();

	command_tx30  = 0;

	CondComLogicRole = CondComRx;
	SmsLogicRole = SmsRoleIdle;
	radio_state = radiostatePswf;
	pswf_rec = 0;
	pswf_in = 0;

	setAsk = false;
	isPswfFull = false;
	waitAckTimer = 0;
}

void DspController::startPSWFTransmitting(bool ack, uint8_t r_adr, uint8_t cmd,int retr)
{
	qmDebugMessage(QmDebug::Dump, "startPSWFTransmitting(%d, %d, %d)", ack, r_adr, cmd);
    QM_ASSERT(is_ready);

    pswf_retranslator = retr;

    ContentPSWF.RET_end_adr = retr;

    pswf_ack_tx = ack;

    ContentPSWF.indicator = 20;
    ContentPSWF.TYPE = 0;
    ContentPSWF.COM_N = cmd;
    ContentPSWF.R_ADR = r_adr;
    if (pswf_retranslator > 0) ContentPSWF.R_ADR += 32;
    ContentPSWF.S_ADR = stationAddress;

    CondComLogicRole = CondComTx;
    radio_state = radiostatePswf;
    SmsLogicRole = SmsRoleIdle;

    if (virtual_mode)
    	startVirtualPpsModeTx();
    else
    	setPswfTx();

	setAsk = false;
	isPswfFull = false;
	waitAckTimer = 0;
}

void DspController::startSMSRecieving(SmsStage stage)
{
    qmDebugMessage(QmDebug::Dump, "startSmsReceiving");
    QM_ASSERT(is_ready);

    for(int i = 0; i<255; i++) rs_data_clear[i] = 1;

    tx_call_ask_vector.erase(tx_call_ask_vector.begin(),tx_call_ask_vector.end());
    quit_vector.erase(quit_vector.begin(),quit_vector.end());


    if (virtual_mode) startVirtualPpsModeRx();
    setRx();

    smsRxStateSync = 0;
    radio_state = radiostateSms;
    sms_counter  = 0;

	syncro_recieve.clear();
	snr.clear();
	waveZone.clear();
    for(int i = 0; i<18;i++)
    {
        syncro_recieve.push_back(99);
        snr.push_back(0);
        waveZone.push_back(0);
    }
    	waveZone.push_back(0); // size must be 19

    	   for(uint8_t i = 0; i <= 100; i++)
    	     sms_content[i] = 0;
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

    ContentSms.indicator = 20;
    ContentSms.TYPE = 0;
    ContentSms.S_ADR = stationAddress;
    ContentSms.R_ADR = r_adr;
    ContentSms.CYC_N = 0;

    count_clear = 0;

    cntChvc = 7;

    int ind = strlen((const char*)message);

    int data_sms[255];

    for(int i = 0;   i < ind; i++) ContentSms.message[i] = message[i];
    for(int i = ind; i < 259; i++) ContentSms.message[i] = 0;

    pack_manager->to_Koi7(ContentSms.message); // test

    pack_manager->compressMass(ContentSms.message,ind,7); //test

    ContentSms.message[87] = ContentSms.message[87] & 0x0F; //set 4 most significant bits to 0

    ContentSms.message[88] = 0;

    uint8_t ret = getSmsRetranslation();
    if (ret != 0){
        ContentSms.message[87] = ContentSms.message[87] | (ret  << 4);
        ContentSms.message[88] = ContentSms.message[88] | ((ret >> 4) & 0x3);
    }

    uint32_t abc = pack_manager->CRC32(ContentSms.message,89);

    for(int i = 0;i<4;i++) ContentSms.message[89+i] = (uint8_t)((abc >> (8*i)) & 0xFF);
    for(int i = 0;i<255;i++) rs_data_clear[i] = 1;
    for(int i = 0; i<255;i++) data_sms[i] = (int)ContentSms.message[i];

    encode_rs(data_sms,&data_sms[93],&rs_255_93);
    for(int i = 0; i<255;i++)ContentSms.message[i]  = data_sms[i];


    radio_state = radiostateSms;

    sms_counter  = 0;

    if (virtual_mode)
    	startVirtualPpsModeTx();
    else
    	setTx();
}

void DspController::startGucTransmitting(int r_adr, int speed_tx, std::vector<int> command, bool isGps)
{
    qmDebugMessage(QmDebug::Dump, "startGucTransmitting(%i, %i)", r_adr, speed_tx);
    QM_ASSERT(is_ready);

    ContentGuc.indicator = 20;
    ContentGuc.type = 1;
	ContentGuc.chip_time = 3; // super versia new, last value = 2
	ContentGuc.WIDTH_SIGNAL = 0; // last value  = 1, thi is freq mode 0 - 3k1, 1 - 20k maybe it works:)
    //data_storage_fs->getAleStationAddress(ContentGuc.S_ADR);
	ContentGuc.S_ADR = stationAddress;

    ContentGuc.R_ADR = r_adr;

    uint8_t num_cmd = command.size();
    ContentGuc.NUM_com = num_cmd;

    isGpsGuc = isGps;

    for(int i = 0;i<num_cmd; i++)
    ContentGuc.command[i] = command[i];

    ContentGuc.ckk = 0;
    ContentGuc.ckk |= (1 & 0x01);
    ContentGuc.ckk |= (ContentGuc.WIDTH_SIGNAL & 0x01) << 1;
    ContentGuc.ckk |= (1 & 0x03) << 2;
    ContentGuc.ckk |= (ContentGuc.chip_time & 0x03) << 4;

    ContentGuc.Coord = 0;

    ContentGuc.stage =  GucTx;

    //initResetState();
    ParameterValue comandValue;
    comandValue.radio_mode = RadioModeOff;//
    sendCommandEasy(RxRadiopath, RxRadioMode, comandValue);
    comandValue.guc_mode = RadioModeSazhenData;
    sendCommandEasy(TxRadiopath, TxRadioMode, comandValue);
    if (freqGucValue != 0)
    comandValue.frequency =  freqGucValue;//3000000;
    sendCommandEasy(RxRadiopath, RxFrequency, comandValue);
    QmThread::msleep(100);
    sendCommandEasy(TxRadiopath, TxFrequency, comandValue);
    radio_state = radiostateGucTxPrepare;
    gucTxStateSync = 0;
    command.clear();

    sendGuc();
}


void DspController::setFreq(int value){
    freqGucValue  = value;
}


void DspController::sendGucQuit()
{
	qmDebugMessage(QmDebug::Dump, "sendGucQuit");

	uint8_t tx_address = 0x7A;
	uint8_t tx_data[DspTransport::MAX_FRAME_DATA_SIZE];
	int tx_data_len = 0;

	ContentGuc.indicator = 20;
	ContentGuc.type = 4;
	ContentGuc.chip_time = 3; // super versia new, last value = 2
	ContentGuc.WIDTH_SIGNAL = 0; // last value  = 1, thi is freq mode 0 - 3k1, 1 - 20k maybe it works:)
	//data_storage_fs->getAleStationAddress(ContentGuc.S_ADR);

	ContentGuc.ckk = 0;
	ContentGuc.ckk |= (1 & 0x01);
	ContentGuc.ckk |= (ContentGuc.WIDTH_SIGNAL & 0x01) << 1;
	ContentGuc.ckk |= (1 & 0x03) << 2;
	ContentGuc.ckk |= (ContentGuc.chip_time & 0x03) << 4;

	//ContentGuc.uin = 0;

	qmToBigEndian((uint8_t)ContentGuc.indicator, tx_data + tx_data_len);
	++tx_data_len;
	qmToBigEndian((uint8_t)ContentGuc.type, tx_data + tx_data_len);
	++tx_data_len;


	uint8_t pack[3] = {0, 0, 0};
	pack[2] = (ContentGuc.R_ADR & 0x1F) << 3;  // 5 � � В±� � С‘� ЎвЂљ
	pack[2] |= (ContentGuc.S_ADR & 0x1F) >> 2; // 3 � � В±� � С‘� ЎвЂљ� � В°
	pack[1] |= (ContentGuc.S_ADR & 0x1F) << 6; // 2 � � В±� � С‘� ЎвЂљ� � В°
	pack[1] |= (ContentGuc.uin >> 2) & 0x3F;   // 6 � � В±� � С‘� ЎвЂљ
	pack[0] = (ContentGuc.uin << 6) & 0xC0;    // 2 � � В±� � С‘� ЎвЂљ� � В°

    for(int i = 2; i >= 0; --i) {
    	qmToBigEndian((uint8_t)pack[i], tx_data + tx_data_len);
    	++tx_data_len;
    }

    transport->transmitFrame(tx_address, tx_data, tx_data_len);
    //stationModeIsCompleted();
}

uint8_t *DspController::getGpsGucCoordinat(uint8_t *coord)
{
	Navigation::Coord_Date date = navigator->getCoordDate();
	std::string lon((const char*)date.longitude);
	std::string lat((const char*)date.latitude);

	coord[0] = (uint8_t)atoi(lat.substr(0,2).c_str());
	coord[1] = (uint8_t)atoi(lat.substr(2,2).c_str());
	coord[2] = (uint8_t)atoi(lat.substr(5,2).c_str());
	coord[3] = (uint8_t)atoi(lat.substr(7,2).c_str());
	coord[4] = (uint8_t)atoi(lon.substr(0,3).c_str());
	coord[5] = (uint8_t)atoi(lon.substr(3,2).c_str());
	coord[6] = (uint8_t)atoi(lon.substr(6,2).c_str());
	coord[7] = (uint8_t)atoi(lon.substr(8,2).c_str());

	if ((strstr((const char*)date.latitude,"N") !=0) && strstr((const char*)date.longitude,"E") !=0)
		coord[8] = 0;
	if ((strstr((const char*)date.latitude,"S") !=0) && strstr((const char*)date.longitude,"E") !=0)
		coord[8] = 1;
	if ((strstr((const char*)date.latitude,"S") !=0) && strstr((const char*)date.longitude,"W") !=0)
		coord[8] = 2;
	if ((strstr((const char*)date.latitude,"N") !=0) && strstr((const char*)date.longitude,"W") !=0)
		coord[8] = 3;

//	for(int i = 0; i< 9;i++) coord[i] = i+1;
//	coord[8] = 0xc0;
    return coord;
}

uint8_t* DspController::getGucCoord(){
     return guc_coord;
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

    initResetState();

    ParameterValue comandValue;
    comandValue.radio_mode = RadioModeOff;
    sendCommandEasy(TxRadiopath, TxRadioMode, comandValue);
    comandValue.guc_mode = 3;

    sendCommandEasy(RadioLineNotPswf, 0 ,comandValue);
    comandValue.guc_mode = stationAddress;
    sendCommandEasy(RadioLineNotPswf, 3 ,comandValue);

    comandValue.guc_mode = 3;
    sendCommandEasy(RadioLineNotPswf, 1, comandValue);
    QmThread::msleep(100);
    //-----------------------------------

    comandValue.guc_mode = RadioModeSazhenData; // 11 mode
    sendCommandEasy(RxRadiopath, RxRadioMode, comandValue);

    comandValue.frequency = freqGucValue;
    sendCommandEasy(RxRadiopath, RxFrequency, comandValue);

   // gucRxStateSync = 0;
    ContentGuc.stage =  GucRx;
    guc_vector.clear();
}

uint8_t* DspController::get_guc_vector()
{
	int num = (guc_vector.at(0).at(3) & 0x3f) << 1;
	num +=    (guc_vector.at(0).at(4) & 0x80) >> 7;

	//� � С—� � С•� � В»� ЎС“� ЎвЂЎ� � Вµ� � � …� � С‘� � Вµ � � С”� � С•� � В»� � С‘� ЎвЂЎ� � Вµ� Ў� ѓ� ЎвЂљ� � � � � � В° � Ў� Њ� � В»� � Вµ� � С�� � Вµ� � � …� ЎвЂљ� � С•� � � �  � � � �  � � � � � � Вµ� � С”� ЎвЂљ� � С•� Ў� ‚� � Вµ
	guc_text[0] = num;

    uint8_t out[120];
    for(int i = 0; i<120;i++) out[i] = 0;
    int crc_coord_len = 0;

    // � � Вµ� Ў� ѓ� � В»� � С‘ � Ў� ѓ � � С”� � С•� � С•� Ў� ‚� � Т‘� � С‘� � � …� � В°� ЎвЂљ� � В°� � С�� � С‘, � ЎвЂљ� � С• � � � � � ЎвЂ№� � В±� � С•� Ў� ‚� � С”� � В° � � С—� � С• � � С•� � Т‘� � � …� � С•� � С�� ЎС“ � � В°� � В»� � С–� � С•� Ў� ‚� � С‘� ЎвЂљ� � С�� ЎС“, � � С‘� � � …� � В°� ЎвЂЎ� � Вµ � � С—� � С• � � Т‘� Ў� ‚� ЎС“� � С–� � С•� � С�� ЎС“
	int count = 0;
    if (isGpsGuc == 0)
    {
        if (num <= 5) count = 5;
        if ((num > 5) && (num <= 11))   count = 11;
        if ((num > 11) && (num <= 25))  count = 25;
        if ((num > 25) && (num <= 100)) count = 100;
    }
    else
    {
        if (num <= 6) count = 6;
        if ((num > 6) && (num <= 10))   count = 10;
        if ((num > 10) && (num <= 26))  count = 26;
        if ((num > 26) && (num <= 100)) count = 100;
    }

    if (isGpsGuc)
    {
        for (int i = 0; i< 9;i++)
        {
            guc_text[i+1] = guc_vector.at(0).at(7+i+count);
        }
        // -- � � вЂ”� � В°� � С—� � С‘� Ў� ѓ� � В°� � В»� � С‘ � � С”� � С•� � С•� Ў� ‚� � Т‘� � С‘� � � …� � В°� ЎвЂљ� ЎвЂ№, � � � …� � В°� ЎвЂЎ� � С‘� � � …� � В°� Ў� Џ � Ў� ѓ � � С—� � Вµ� Ў� ‚� � � � � � С•� � в„– � � С—� � С•� � В·� � С‘� ЎвЂ� � � С‘� � С‘ � � С�� � В°� Ў� ѓ� Ў� ѓ� � С‘� � � � � � В° guc_text
        for(int i = 0; i< count;i++){
        	if (i < num)
        		guc_text[9 + i+1] = guc_vector.at(0).at(7+i);
        	else
        		guc_text[9 + i+1] = 0;
        }
        // -- � � вЂ”� � В°� � С—� � С‘c� � В°� � В»� � С‘ � � Т‘� � В°� � � …� � � …� ЎвЂ№� � Вµ, � � � …� � В°� ЎвЂЎ� � С‘� � � …� � В°� Ў� Џ � Ў� ѓ 10-� � в„– � � С—� � С•� � В·� � С‘� ЎвЂ� � � С‘� � С‘ � � � …� � В°� Ўв‚¬� � Вµ� � С–� � С• � � С�� � В°� Ў� ѓ� Ў� ѓ� � С‘� � � � � � В°
        std::vector<bool> data;
        for(int i = 0; i< 8; i++) pack_manager->addBytetoBitsArray(guc_text[i+1],data,8);
        // � � Т‘� � С•� � В±� � В°� � � � � � С‘� � В»� � С‘ � � С”� � С•� � С•� Ў� ‚� � Т‘� � С‘� � � …� � В°� ЎвЂљ� ЎвЂ№ � � С” � � В±� � С‘� ЎвЂљ� � С•� � � � � � С•� � С�� ЎС“ � � � � � � Вµ� � С”� ЎвЂљ� � С•� Ў� ‚� ЎС“

        bool quadrant = guc_text[9] & (1 << 7);
        data.push_back(quadrant);
        quadrant = guc_text[9] & (1 >> 6);
        data.push_back(quadrant);
        // � � Т‘� � С•� � В±� � В°� � � � � � С‘� � В»� � С‘ � � С” � � В±� � С‘� ЎвЂљ� � С•� � � � � � С•� � С�� ЎС“ � � � � � � Вµ� � С”� ЎвЂљ� � С•� Ў� ‚� ЎС“ � � С”� � � � � � В°� � Т‘� Ў� ‚� � В°� � � …� ЎвЂљ
        for(int i = 0; i<num;i++) pack_manager->addBytetoBitsArray(guc_text[9 + i+1],data,7);
        // � � Т‘� � С•� � В±� � В°� � � � � � С‘� � В»� � С‘ � � С” � � В±� � С‘� ЎвЂљ� � С•� � � � � � С•� � С�� ЎС“ � � � � � � Вµ� � С”� ЎвЂљ� � С•� Ў� ‚� ЎС“ � � Т‘� � В°� � � …� � � …� ЎвЂ№� � Вµ � � С—� � С• 7 � � В±� � С‘� ЎвЂљ
        pack_manager->getArrayByteFromBit(data,out);
        // � � В·� � В°� � С—� � С‘� Ў� ѓ� � В°� � В»� � С‘ � � � �  � � � � � ЎвЂ№� ЎвЂ¦� � С•� � Т‘� � � …� � С•� � в„– � � С�� � В°� Ў� ѓ� Ў� ѓ� � С‘� � � �  � � С—� Ў� ‚� � Вµ� � С•� � В±� Ў� ‚� � В°� � В·� � С•� � � � � � В°� � � …� � � …� ЎвЂ№� � Вµ � � Т‘� � В°� � � …� � � …� ЎвЂ№� � Вµ � � С‘� � В· � � В±� � С‘� ЎвЂљ� � С•� � � � � � С•� � С–� � С• � � С�� � В°� Ў� ѓ� Ў� ѓ� � С‘� � � � � � В° � � С—� � С• � � В°� � � …� � С•� � В»� � С•� � С–� � С‘� � С‘ � Ў� ѓ � ЎвЂћ� � С•� Ў� ‚� � С�� � С‘� Ў� ‚� � С•� � � � � � В°� � � …� � С‘� � Вµ � � С—� � В°� � С”� � Вµ� ЎвЂљ� � В° � � Т‘� � В»� Ў� Џ CRC32 � � � …� � В° � � С—� � Вµ� Ў� ‚� � Вµ� � Т‘� � В°� ЎвЂЎ� � Вµ
        crc_coord_len = data.size() / 8;
        // � � С—� � С•� � В»� ЎС“� ЎвЂЎ� � С‘� � В»� � С‘ � � Т‘� � В»� � С‘� � � …� � � …� ЎС“ � � С—� � В°� � С”� � Вµ� ЎвЂљ� � В°

        uint8_t cord[9];

        for(int i = 1; i<9;i++) cord[i] = guc_text[i];
        for(int i = 0;i<num;i++) guc_text[i+1] = guc_text[9+i+1];
        for(int i = 1;i<9; i++)  guc_text[num+i] = cord[i];
    }
    else
    {
    	std::vector<bool> data;
        for(int i = 0; i<num;i++) pack_manager->addBytetoBitsArray(guc_vector.at(0).at(7+i),data,7);
        for(int i = 0; i<count;i++) pack_manager->getArrayByteFromBit(data,out);
        guc_text[0] = num;
        for(int i = 0; i<num;i++) guc_text[i+1] = guc_vector.at(0).at(7+i);
    }

	// � � Т‘� � С•� Ў� ѓ� ЎвЂљ� � В°� � Вµ� � С� crc32 � Ў� ѓ� ЎС“� � С�� � С�� ЎС“ � � С‘� � В· � � С”� � С•� � � …� ЎвЂ� � � В° � � С—� � В°� � С”� � Вµ� ЎвЂљ� � В°
	int m = 3;
	uint32_t crc_packet = 0;
	int l = 0;
	while(m >=0){
		uint8_t sum = guc_vector.at(0).at(guc_vector.at(0).size() - 1 - m);
		crc_packet += sum << (8*m);
		l++;
		m--;
	}

	// � Ў� ѓ� ЎвЂЎ� � С‘� ЎвЂљ� � В°� � Вµ� � С� crc32 � Ў� ѓ� ЎС“� � С�� � С�� ЎС“
    uint32_t crc = 0;
    int value  = (isGpsGuc) ? crc_coord_len : num;
    // � � � � � ЎвЂ№� � В±� Ў� ‚� � В°� � В»� � С‘ � � Т‘� � В»� � С‘� � � …� � � …� ЎС“, � � С‘� Ў� ѓ� ЎвЂ¦� � С•� � Т‘� Ў� Џ � � С‘� � В· � Ў� ‚� � Вµ� � В¶� � С‘� � С�� � В° � � С—� � Вµ� Ў� ‚� � Вµ� � Т‘� � В°� ЎвЂЎ� � С‘

    if (!isGpsGuc) {value = ((num*7)/8); uint8_t ost = (num*7)% 8;

    if (ost !=0)
        	{
        		uint8_t mask = 0;
        		for(int i = 0; i<ost;i++) mask +=  1 << (7 - i);
        		value +=1;
        		out[value-1] = out[value-1] & mask;
        	}
    }

    crc = pack_manager->CRC32(out,value);

    if (crc != crc_packet)
    {
        gucCrcFailed();
        guc_text[0] = '\0';
        failQuitGuc = true;
    }
	guc_vector.clear();

	return guc_text;
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
	std::vector<uint8_t> payload(5);
	payload[0] = 20;
	payload[1] = bandwidth;
	payload[2] = type;
	payload[3] = 0;
	payload[4] = 0;
	if (data_len > 0) {
		QM_ASSERT(data);
		payload.insert(std::end(payload), data, data + data_len);
	}
	transport->transmitFrame(0x7E, &payload[0], payload.size());
}

void DspController::sendModemPacket_packHead(ModemBandwidth bandwidth,
		uint8_t param_signForm, uint8_t param_packCode,
		const uint8_t *data, int data_len) {
	std::vector<uint8_t> payload(7);
	payload[0] = 20;
	payload[1] = bandwidth;
	payload[2] = 22;
	payload[3] = 0;
	payload[4] = 0;
	payload[5] = param_signForm;
	payload[6] = param_packCode;
	QM_ASSERT(data);
	QM_ASSERT(data_len > 0);
	payload.insert(std::end(payload), data, data + data_len);
	transport->transmitFrame(0x7E, &payload[0], payload.size());
}

void DspController::goToVoice(){
	ParameterValue comandValue;
	comandValue.radio_mode = RadioModeOff;
	radio_state = radiostateSync;
	sendCommandEasy(RxRadiopath,2,comandValue);
	sendCommandEasy(TxRadiopath,2,comandValue);
}

bool DspController::getIsGucCoord()
{
    return isGpsGuc;
}


void DspController::startVirtualPpsModeTx()
{
	setPswfTx();
	ParameterValue comandValue;  //0x60 2 5 1
	comandValue.param = 1;
	sendCommandEasy(PSWFReceiver,5,comandValue);
	comandValue.radio_mode = RadioModeOff;
	sendCommandEasy(VirtualPps,1,comandValue);
#ifndef PORT__PCSIMULATOR
	t = rtc->getTime();
	d = rtc->getDate();
#endif
	RtcTxRole = true;
	RtcRxRole = false;
	RtcTxCounter = 0;
	//radio_state = radiostatePswf;

	count_VrtualTimer = startVirtTxPhaseIndex;
	txrtx = 0;
}


void DspController::startVirtualPpsModeRx()
{
	setRx();

	ParameterValue comandValue;
	comandValue.param = 1;
	sendCommandEasy(PSWFReceiver,5,comandValue);
	comandValue.param = 2;
	sendCommandEasy(PSWFReceiver,4,comandValue);

	RtcRxCounter = 0;
	RtcRxRole = true;
	RtcTxRole = false;
	RtcFirstCatch = 0;
#ifndef PORT__PCSIMULATOR
	t = rtc->getTime();
	d = rtc->getDate();
#endif
	antiSync = false;
	pswf_in_virt = 0;
	count_VrtualTimer = NUMS;

}

void DspController::sendSynchro(uint32_t freq, uint8_t cnt)
{
	qmDebugMessage(QmDebug::Dump, "freq virtual %d", freq);
	if (RtcTxRole)
	{
		uint8_t tx_address = 0x72;
		uint8_t tx_data[DspTransport::MAX_FRAME_DATA_SIZE];
		int tx_data_len = 0;
		qmToBigEndian((uint8_t)20, tx_data + tx_data_len);
		++tx_data_len;
		qmToBigEndian((uint8_t)2, tx_data + tx_data_len);
		++tx_data_len;
		qmToBigEndian((uint32_t)freqVirtual, tx_data + tx_data_len);
		tx_data_len = tx_data_len + 4;
		qmToBigEndian((uint8_t)1, tx_data + tx_data_len);
		++tx_data_len;
		qmToBigEndian((uint8_t)cnt, tx_data + tx_data_len);
		++tx_data_len;

		transport->transmitFrame(tx_address,tx_data,tx_data_len);
	}
}

void DspController::correctTime(uint8_t num)
{
	// correction time
	qmDebugMessage(QmDebug::Dump, "correctTime() data[7] as num %d", num);
	qmDebugMessage(QmDebug::Dump, "correctTime() before getTime t.seconds %d", t.seconds);
#ifndef PORT__PCSIMULATOR
	t = rtc->getTime();
#endif
	qmDebugMessage(QmDebug::Dump, "correctTime() after getTime t.seconds %d", t.seconds);
	t.seconds = 12 * (t.seconds / 12) + 7;
	qmDebugMessage(QmDebug::Dump, "correctTime() after correct t.seconds %d", t.seconds);
	count_VrtualTimer = num;
	qmDebugMessage(QmDebug::Dump, "COUNTER VIRTUAL %d",count_VrtualTimer);

	RtcFirstCatch = -1;
}

void DspController::wakeUpTimer()
{
#ifndef PORT__PCSIMULATOR
	if ((virtual_mode) && (RtcRxRole) && (!antiSync))
	{
		t = rtc->getTime();

		if (IsStart(t.seconds))
		{
			freqVirtual = getCommanderFreq(ContentPSWF.RN_KEY,t.seconds,d.day,t.hours,t.minutes);
			ParameterValue param;
			param.frequency = freqVirtual;
			qmDebugMessage(QmDebug::Dump, "freq virtual %d",freqVirtual);
			sendCommandEasy(RxRadiopath, 1, param);
		}
	}
#endif
}

void DspController::LogicPswfModes(uint8_t* data, uint8_t indicator, int data_len)
{

	qmDebugMessage(QmDebug::Dump, "LogicPswfModes() pswf_in_virt = %d ", pswf_in_virt);
	if (virtual_mode && SmsLogicRole == SmsRoleRx && !smsFind){
		pswf_in_virt++;
		if (pswf_in_virt >= 90){
			sms_counter = 0;
			startVirtualPpsModeRx();
		}
	}
	qmDebugMessage(QmDebug::Dump, "pswf_in_virt");

	if (indicator == 31)
	{
		qmDebugMessage(QmDebug::Dump, "0x63 indicator 31");
		if (sms_counter < 19)
		{
			snr.erase(snr.begin());
			snr.push_back(0);

			syncro_recieve.erase(syncro_recieve.begin());
			syncro_recieve.push_back(99);

			if (check_rx_call(&wzn_value)) {
				sms_call_received = true;
				syncro_recieve.clear();
				for(int i = 0; i<18;i++)
					syncro_recieve.push_back(99);
				qmDebugMessage(QmDebug::Dump, "sms call received");
			}
			sms_data_count = 0;
		}

    }
	else if (indicator == 30)
	{
		if (SmsLogicRole == SmsRoleIdle)
		{
			ContentPSWF.R_ADR = data[7];
			ContentPSWF.S_ADR = data[8];
			qmDebugMessage(QmDebug::Dump, "____R_ADR = %d ", ContentPSWF.R_ADR);
			qmDebugMessage(QmDebug::Dump, "____S_ADR = %d ", ContentPSWF.S_ADR);
		}
		if (data[1] == 2)  // synchro packet
		{
			antiSync = true;
			qmDebugMessage(QmDebug::Dump, "Sync anti turn on");
			correctTime(data[7]);
		}

		qmDebugMessage(QmDebug::Dump, "0x63 indicator 30");
		if (SmsLogicRole != SmsRoleIdle)

			qmDebugMessage(QmDebug::Dump, "processReceivedFrame() data_len = %d", data_len);
			std::vector<uint8_t> sms_data;
			if (sms_counter > 38 && sms_counter < 76)
			{
				uint8_t index_sms = 7 * (sms_counter-40);
				for(int i = 0; i <  7; i++)
				{
					smsDataPacket[index_sms + i ] = data[i+8];
					rs_data_clear[index_sms + i ] = data[i+16];
					qmDebugMessage(QmDebug::Dump, "data[%d] = %d", i, data[i+8]);
				}
			}

			if (sms_counter > 19 && sms_counter < 38)
			{
				tx_call_ask_vector.push_back(data[9]); // wzn response
				qmDebugMessage(QmDebug::Dump, "LogicPswfModes() (19;38) WAVE_ZONE = %d", data[9]);
			}
			if (sms_counter < 19)
			{
				snr.erase(snr.begin());
				snr.push_back(data[6]);

				syncro_recieve.erase(syncro_recieve.begin());
				syncro_recieve.push_back(data[9]); // CYC_N
				qmDebugMessage(QmDebug::Dump, "LogicPswfModes() (0;19) WAVE_ZONE = %d", data[9]);

				qmDebugMessage(QmDebug::Dump, "recieve frame() count = %d", syncro_recieve.size());

				ContentSms.R_ADR = data[8]; // todo: check
				if (check_rx_call(&wzn_value))
				{
					sms_call_received = true;

					if (ContentSms.R_ADR > 32) pswf_ack = true;
					qmDebugMessage(QmDebug::Dump, "sms call received");
					syncro_recieve.clear();
					for(int i = 0; i<18;i++)
						syncro_recieve.push_back(99);
				}
			}

			if (sms_counter > 76 && sms_counter < 83)
			{
				prevTime();

				uint8_t ack      = data[9];
				uint8_t ack_code = data[10];
				uint8_t ack_code_calc = calc_ack_code(ack);

				qmDebugMessage(QmDebug::Info, "recieve count sms = %d %d", ack_code_calc, data[10]);
				qmDebugMessage(QmDebug::Dump, "recieve count sms = %d %d", ack_code_calc, data[10]);
				if (ack_code_calc == ack_code){
						if (ack == 73)
							++ok_quit;
						if (ack == 99)
							++smsError;
				}

				quit_vector.push_back(ack);  // ack
				quit_vector.push_back(ack_code); // ack code
			}
			pswf_first_packet_received = true;
		}

	if (SmsLogicRole == SmsRoleIdle)
	{
		recPswf(data[9],data[10],indicator);
	}
}


bool DspController::getVirtualMode()
{
	return virtual_mode;
}

void DspController::setVirtualMode(bool param)
{
	virtual_mode = param;
	if (!virtual_mode)
	{
		ParameterValue comandValue;  //0x60 2 5 1
		comandValue.param = 0;
		sendCommandEasy(PSWFReceiver,5,comandValue);
	}
}

void DspController::setVirtualDate(uint8_t *param)
{
    uint8_t date[3];

    for(int i = 0; i<6;i++) param[i] = param[i] - 48;

    date[0] =  10*param[0] + param[1];
    date[1] =  10*param[2] + param[3];
    date[2] =  10*param[4] + param[5];

#ifndef PORT__PCSIMULATOR
    d.day = date[0];
    d.month = date[1];
    d.year  = date[2];
    rtc->setDate(d);
    d = rtc->getDate();
#endif
}

void DspController::setVirtualTime(uint8_t *param)
{
    for(int i = 0; i<6;i++) param[i] = param[i] - 48;

    uint8_t time[3] = {0,0,0};

    time[2] = 10*param[0] + param[1];
    time[1] = 10*param[2] + param[3];
    time[0] = 10*param[4] + param[5];

#ifndef PORT__PCSIMULATOR
    t.seconds  = time[0];
    t.minutes  = time[1];
    t.hours    = time[2];
    rtc->setTime(t);
#endif

}

uint8_t* DspController::getVirtualTime()
{
#ifndef PORT__PCSIMULATOR
	QmRtc::Time time = rtc->getTime();
	char param = 0;
	char ms[3] = {0,0,0};

	ms[2] = time.seconds;
	ms[1] = time.minutes;
	ms[0] = time.hours;

	for(int i = 0; i<3;i++)
	if (ms[i] > 9)
	{
		param = ms[i] / 10;
		virtualTime[2*i]   = param + 48;
		param = ms[i] % 10;
		virtualTime[2*i+1] = param + 48;
	}
	else
	{
		virtualTime[2*i] = 48;
		virtualTime[2*i+1] = ms[i] + 48;
	}
#endif
	return &virtualTime[0];
}

void DspController::playSoundSignal(uint8_t mode, uint8_t speakerVolume, uint8_t gain, uint8_t soundNumber, uint8_t duration, uint8_t micLevel)
{
	ParameterValue value;

	if (is_ready){

	value.signal_duration = duration;
	sendCommandEasy(Module::Audiopath, AudioSignalDuration, value);

	value.signal_number = soundNumber;
	sendCommandEasy(Module::Audiopath, AudioSignalNumber, value);

	value.audio_mode = (AudioMode)mode;
	sendCommandEasy(Module::Audiopath, AudioModeParameter, value);

//	value.volume_level = speakerVolume;
//	sendCommandEasy(Module::Audiopath, AudioVolumeLevel, value);
//
//	value.mic_amplify = gain;
//	sendCommandEasy(Module::Audiopath, AudioMicAmplify, value);

//
//	value.signal_mic_level = micLevel;
//	sendCommandEasy(Module::Audiopath, AudioSignalMicLevel, value);
	}
}
}
/* namespace Multiradio */

#include "qmdebug_domains_start.h"
QMDEBUG_DEFINE_DOMAIN(dspcontroller, LevelVerbose)
#include "qmdebug_domains_end.h"
