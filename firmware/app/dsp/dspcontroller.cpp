/**
 ******************************************************************************
 * @file    dspcontroller.cpp
 * @author  Artem Pisarenko, PMR dept. software team, ONIIP, PJSC
 * @author  РЅРµРёР·РІРµСЃС‚РЅС‹Рµ
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


#define DEFAULT_PACKET_HEADER_LEN	2 // РёРЅРґРёРєР°С‚РѕСЂ РєР°РґСЂР° + РєРѕРґ РїР°СЂР°РјРµС‚СЂР° ("Р°РґСЂРµСЃ" РЅР° СЃР°РјРѕРј РґРµР»Рµ РЅРµ РІС…РѕРґРёС‚ СЃСЋРґР°, СЌС‚Рѕ "Р°РґСЂРµСЃ РЅР°Р·РЅР°С‡РµРЅРёСЏ" РёР· РєР°РЅР°Р»СЊРЅРѕРіРѕ СѓСЂРѕРІРЅСЏ)

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
	// max_tx_queue_size: 1 РєРѕРјР°РЅРґР° СЂР°РґРёРѕС‚СЂР°РєС‚Р° + 1 Р·Р°РїР°СЃ
	transport = new DspTransport(uart_resource, 2, this);
	transport->receivedFrame.connect(sigc::mem_fun(this, &DspController::processReceivedFrame));
	initResetState();

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
        syncro_recieve.push_back(99);
    }

    ContentGuc.stage = GucNone;

    guc_timer = new QmTimer(true,this);
    guc_timer->setInterval(GUC_TIMER_INTERVAL);
    guc_timer->timeout.connect(sigc::mem_fun(this,&DspController::GucSwichRxTxAndViewData));

    guc_rx_quit_timer = new QmTimer(true,this);
    guc_rx_quit_timer->timeout.connect(sigc::mem_fun(this,&DspController::sendGucQuit));

    for(int i = 0;i<50;i++) guc_text[i] = '\0';

    sms_call_received = false;
    for(int i = 0;i<255;i++) rs_data_clear[i] = 1;

    data_storage_fs->getAleStationAddress(ContentSms.S_ADR);
    data_storage_fs->getAleStationAddress(ContentPSWF.S_ADR);
    QNB = 0;
    pswf_rec = 0;

    ContentPSWF.RN_KEY = DefkeyValue;
    ContentSms.RN_KEY = DefkeyValue;

    retranslation_active = false;

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
	case radiostateSms:
	{
		changeSmsFrequency();
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

void DspController::transmitSMS()
{
    //getDataTime();

    ContentSms.CYC_N += 1;

    if (sms_counter >= 19 && sms_counter <= 38){
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


        if (sms_counter == 19)
        {
            qmDebugMessage(QmDebug::Dump, "ContentSms.stage = StageTx_call_ack");
            startSMSRecieving(ContentSms.stage);
            updateSmsStatus(getSmsForUiStage());
            return;
        }


        if (sms_counter == 39 && SmsLogicRole == SmsRoleRx)
        {
            qmDebugMessage(QmDebug::Dump, "ContentSms.stage = StageRx_data");
            QNB_RX = 0;
            ContentSms.stage = StageRx_data;
            for(int i = 0;i<255;i++) rs_data_clear[i] = 0;
            startSMSRecieving(ContentSms.stage);
            updateSmsStatus(getSmsForUiStage());
            return;
        }

    	if (sms_counter == 78 && SmsLogicRole == SmsRoleTx)
    	{
    		qmDebugMessage(QmDebug::Dump, "ContentSms.stage = StageTx_quit");
    		ContentSms.stage = StageTx_quit;
    		startSMSRecieving(ContentSms.stage);
            updateSmsStatus(getSmsForUiStage());
    		QNB = 0;
    	}

        if (sms_counter == 83)
        {
            qmDebugMessage(QmDebug::Dump, "ContentSms.stage = StageNone");
            ContentSms.stage = StageNone;
            radio_state = radiostateSync;
            qmDebugMessage(QmDebug::Dump, "Sms receiving finished");
            goToVoice();
            return;
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
        if (pswf_ack || (ContentPSWF.RET_end_adr > 0)) {
            startPSWFReceiving(false);
            state_pswf = true;
        } else {
        	qmDebugMessage(QmDebug::Dump, "radio_state = radiostateSync");
        	radio_state = radiostateSync;
        	goToVoice();
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
				started();
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

        if (sms_counter > 20 && sms_counter < 38) sendSms(PSWFTransmitter);
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

			generateSmsReceived();

			setTx();
			sendSms(PSWFTransmitter);
		}
		if (sms_counter > 77 && sms_counter < 83)
		{
            sendSms(PSWFTransmitter);
		}

        if (sms_counter == 84)
		{
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
			smsFind  = true; sms_call_received = false;
			sms_counter = 19;
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
    		goToVoice();
    	}
    }

    if (sms_counter > 39 && sms_counter <76)
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
    	if (ok_quit >= 2) smsFailed(-1);  else smsFailed(0);
    	resetSmsState();
    }

}

void DspController::changeSmsFrequency()
{
	getDataTime();


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
      smsCounterChanged();

    //setrRxFreq();

	////////////////////////////////////
}


void DspController::resetSmsState()
{
	sms_counter = 0;
	radio_state = radiostateSync;
	smsFind  = false;
	ok_quit = 0;
	std::memset(rs_data_clear,1,sizeof(rs_data_clear));
}

void DspController::checkForRxAction()
{
//   if (sms_counter == 77)
//   {
//		   updateSmsStatus(getSmsForUiStage());
//		   generateSmsReceived();
//	   }
//	   else
//	   {
//		   smsFailed(0);
//		   resetSmsState();
//		   goToVoice();
//	   }
//   }
}




bool DspController::checkForTxAnswer()
{

	if (tx_call_ask_vector.size() >= 3)
	{
		wzn_value = wzn_change(tx_call_ask_vector);
		qmDebugMessage(QmDebug::Dump, "wzn_value" ,wzn_value);
		tx_call_ask_vector.erase(tx_call_ask_vector.begin());

		return true;
	}
	return false;
}

void DspController::setrRxFreq()
{
    ContentSms.Frequency =  getFrequencySms();

    ParameterValue param;
    param.frequency = ContentSms.Frequency;
    if ((SmsLogicRole == SmsRoleRx) && (sms_counter >= 38 && sms_counter < 77))
    	sendCommandEasy(PSWFReceiver, PswfRxFreqSignal, param);
    else
    sendCommandEasy(PSWFReceiver, PswfRxFrequency, param);
}


void DspController::RecievedPswf()
{
	pswf_first_packet_received = true;
    qmDebugMessage(QmDebug::Dump, "RecievedPswf() command_rx30 = %d", command_rx30);
    if (command_rx30 == 30) {
        command_rx30 = 0;
        recievedPswfBuffer.erase(recievedPswfBuffer.begin());
        if (!pswf_ack) goToVoice();
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
    		data_storage_fs->getAleStationAddress(ContentPSWF.S_ADR);
    		if (ContentPSWF.R_ADR > 32) ContentPSWF.R_ADR = ContentPSWF.R_ADR - 32;
    		qmDebugMessage(QmDebug::Dump, "r_adr = %d,s_adr = %d", ContentPSWF.R_ADR,ContentPSWF.S_ADR);
    	}
    }

    if (pswf_rec == 3) firstPacket(ContentPSWF.COM_N);
    if ((pswf_ack == false) && (command_rx30 == 30)) {radio_state = radiostateSync;}

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

	fr_sh = fr_sh * 1000; // Р“С†

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

    fr_sh = fr_sh * 1000; // Р“С†

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

void DspController::resetContentStructState()
{
    ContentGuc.stage = GucNone;
    ContentSms.stage = StageNone;
    // РґРѕР±Р°РІРёС‚СЊ РѕРїСЂРµРґРµР»РµРЅРёРµ РґСЂСѓРіРёС… С„СѓРЅРєС†РёР№
}




int DspController::CalcShiftFreq(int RN_KEY, int SEC, int DAY, int HRS, int MIN)
{
    int TOT_W = 6671; // С€РёСЂРёРЅР° СЂР°Р·СЂРµС€РµРЅРЅС‹С… СѓС‡Р°СЃС‚РєРѕРІ

    int SEC_MLT = value_sec[SEC]; // SEC_MLT РІС‹Р±РёСЂР°РµРј РІ РјР°СЃСЃРёРІРµ

    int FR_SH = (RN_KEY + 230*SEC_MLT + 19*MIN + 31*HRS + 37*DAY)% TOT_W;

    qmDebugMessage(QmDebug::Dump, "Calc freq formula %d", FR_SH);
    return FR_SH;
}

//int DspController::CalcSmsTransmitFreq(int RN_KEY, int SEC, int DAY, int HRS, int MIN)
//{
//    int wzn = 0;
//    int FR_SH = 0;
//    int TOT_W = 6671;
//    int wz_base = 0;

//    int SEC_MLT = value_sec[SEC];

//    if (ContentSms.stage == StageTx_data ||(ContentSms.stage == StageRx_data) ||
//       (ContentSms.stage == StageTx_quit)||(ContentSms.stage == StageRx_quit))
//    {
////    	if  (SEC_MLT >=0   && SEC_MLT <6 ) wzn = 0;
////    	if  (SEC_MLT > 5   && SEC_MLT <12) wzn = 1;
////    	if  (SEC_MLT >= 12 && SEC_MLT <18) wzn = 2;
////    	if  (SEC_MLT >=18  && SEC_MLT <24) wzn = 3;
////    	if  (SEC_MLT >= 24 && SEC_MLT <30) wzn = 4;

//    	wzn = wzn_value;

//    	if (wzn > 0) wz_base = 6*wzn;
//    	else wzn  = 0;

//    	int wz_shift = SEC % 6;
//    	SEC_MLT = wz_shift + wz_base;

//    	qmDebugMessage(QmDebug::Dump, "wzn_base %d" ,wz_base);
//    	qmDebugMessage(QmDebug::Dump, "wzn_shift %d" ,wz_shift);
//    }

//    qmDebugMessage(QmDebug::Dump, "SEC_MLT %d" ,SEC_MLT);

//    if ((ContentSms.stage == StageTx_call) || (ContentSms.stage == StageRx_call) ||
//            (ContentSms.stage == StageTx_call_ack) || (ContentSms.stage == StageRx_call_ack) || (ContentSms.stage == StageNone)) {
//        FR_SH = (RN_KEY + 73 + 230*SEC_MLT + 17*MIN + 29*HRS + 43*DAY)% TOT_W;
//        qmDebugMessage(QmDebug::Dump, "Calc freq sms tx formula %d", FR_SH);
//    }
//    if ((ContentSms.stage == StageTx_data) || (ContentSms.stage == StageRx_data) ){
//        FR_SH = (RN_KEY + 3*SEC + 230*SEC_MLT + 17*MIN + 29*HRS + 43*DAY)% TOT_W;
//        qmDebugMessage(QmDebug::Dump, "Calc freq sms  formula %d", FR_SH);
//    }
//    if ((ContentSms.stage == StageTx_quit) || (ContentSms.stage == StageRx_quit)){
//        FR_SH = (RN_KEY + 5*SEC + 230*SEC_MLT + 17*MIN + 29*HRS + 43*DAY)% TOT_W;
//        qmDebugMessage(QmDebug::Dump, "Calc freq sms quit formula %d", FR_SH);
//    }
//    return FR_SH;
//}

int DspController::CalcSmsTransmitFreq(int RN_KEY, int SEC, int DAY, int HRS, int MIN)
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

    return FR_SH;
}



void DspController::initResetState() {
	radio_state = radiostateSync;
	current_radio_operation = RadioOperationOff;
	current_radio_mode = RadioModeOff;
    current_radio_frequency = 0;
	pending_command->in_progress = false;
}

void DspController::setAdr()
{
	ParameterValue param;
	param.pswf_r_adr = ContentPSWF.S_ADR;
	sendCommand(PSWFReceiver, PswfRxRAdr, param);
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
	qmToBigEndian((uint8_t)2, tx_data+0); // РёРЅРґРёРєР°С‚РѕСЂ: "РєРѕРјР°РЅРґР° (СѓСЃС‚Р°РЅРѕРІРєР°)"
	qmToBigEndian((uint8_t)code, tx_data+1); // РєРѕРґ РїР°СЂР°РјРµС‚СЂР°
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
	// РґР»СЏ РџРџпїЅ Р§
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
			if (sms_counter > 38 && sms_counter < 77)
			{
                uint8_t fstn = calcFstn(ContentSms.R_ADR,ContentSms.S_ADR,ContentSms.RN_KEY,date_time[0],date_time[1],date_time[2],date_time[3],sms_counter - 39); // TODO: fix that;
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
		qmToBigEndian((uint8_t)2, tx_data+0); // РёРЅРґРёРєР°С‚РѕСЂ: "РєРѕРјР°РЅРґР° (СѓСЃС‚Р°РЅРѕРІРєР°)"
		qmToBigEndian((uint8_t)code, tx_data+1); // РєРѕРґ РїР°СЂР°РјРµС‚СЂР°
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
		// РґР»СЏ РџРџпїЅ Р§
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
                    uint8_t fstn = calcFstn(ContentSms.R_ADR,ContentSms.S_ADR,ContentSms.RN_KEY,date_time[0],date_time[1],date_time[2],date_time[3],sms_counter - 39); // TODO: fix that;
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

    ContentGuc.Coord = (isGpsGuc == true) ? 1 : 0;

    // Р·Р°РїРѕР»РЅРµРЅРёРµ С‡РІСЃ РґР»СЏ РѕСЃРЅРѕРІРЅС‹С… РґР°РЅРЅС‹С… РїР°РєРµС‚Р°
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

    int crc32_len = ContentGuc.NUM_com; // СЂРµР°Р»СЊРЅРѕРµ РєРѕР»РёС‡РµСЃС‚РІРѕ РєРѕРјР°РЅРґ
    int real_len = crc32_len;

    // Р’С‹Р±РѕСЂ РєРѕР»РёС‡РµСЃС‚РІР° РїРµСЂРµРґР°РІР°РµРјС‹С… Р±Р°Р№С‚РѕРІ СЃ РєРѕРѕСЂРґРёРЅР°С‚Р°РјРё РёР»Рё Р±РµР· РїРѕ СЂРµРіР»Р°РјРµРЅС‚Сѓ
    if (isGpsGuc){
        if (ContentGuc.NUM_com <= 6) ContentGuc.NUM_com = 6;
        if ((ContentGuc.NUM_com > 6) && (ContentGuc.NUM_com <= 10))    ContentGuc.NUM_com = 10;
        if ((ContentGuc.NUM_com > 10) && (ContentGuc.NUM_com <= 26))   ContentGuc.NUM_com = 26;
        if ((ContentGuc.NUM_com > 26) && (ContentGuc.NUM_com <= 100))  ContentGuc.NUM_com = 100;
    }  else

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

    // РѕР±СЂР°Р±РѕС‚РєР° Рё РїРѕР»СѓС‡РµРЅРёРµ РєРѕРѕСЂРґРёРЅР°С‚, РґРѕР±Р°РІР»РµРЅРёРµ РІ РёСЃС…РѕРґРЅС‹Р№ РјР°СЃСЃРёРІ РґР»СЏ Р·Р°С‰РёС‚С‹  crc32 СЃСѓРјРѕР№ (Р”РђРќРќР«Р• + РљРћРћР”Р�РќРђРўР«)
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

    // РІС‹Р±РѕСЂ РґР»РёРЅРЅС‹ РєРѕРґРёСЂСѓРµРјРѕРіРѕ РјР°СЃСЃРёРІР°
     crc32_len = (isGpsGuc == true) ? (ContentGuc.NUM_com + 9) : (ContentGuc.NUM_com);

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

    // СЃРґРІРёРі РјР°СЃСЃРёРІР° РґР»СЏ crc32-СЃСѓРјРјС‹
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
     // РґРѕР±Р°РІР»РµРЅРёРµ crc32 Рє РїР°РєРµС‚Сѓ РґР°РЅРЅС‹С…
     uint32_t crc = pack_manager->CRC32(ContentGuc.command, crc32_len);
     qmToBigEndian((uint32_t)crc, tx_data + tx_data_len);
     tx_data_len += 4;

    transport->transmitFrame(tx_address, tx_data, tx_data_len);
}

void DspController::recGuc()
{
	srand(time(0));
    if (ContentGuc.stage == GucTx){
    	ContentGuc.stage = GucTxQuit;
        startGucRecieving();
        guc_timer->setInterval(GUC_TIMER_INTERVAL_REC);
        guc_timer->start();
    }
    if (ContentGuc.stage == GucRx){
    	ContentGuc.stage = GucRxQuit;
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
		if ((indicator == 5) && (code == 2) && (value_len == 6)) // РёРЅРёС†РёР°С‚РёРІРЅРѕРµ СЃРѕРѕР±С‰РµРЅРёРµ СЃ С†РёС„СЂРѕРІРѕР№ РёРЅС„РѕСЂРјР°С†РёРµР№ Рѕ РїСЂРѕС€РёРІРєРµ ?
			processStartup(qmFromBigEndian<uint16_t>(value_ptr+0), qmFromBigEndian<uint16_t>(value_ptr+2), qmFromBigEndian<uint16_t>(value_ptr+4));
		break;
	}
	case 0x31: {
    	value_ptr -= 1; // РєРѕСЃС‚С‹Р»РЅРѕРµ РїСЂРµРІСЂР°С‰РµРЅРёРµ РІ РЅРµСЃС‚Р°РЅРґР°СЂС‚РЅС‹Р№ С„РѕСЂРјР°С‚ РєР°РґСЂР°
    	value_len += 1; // РєРѕСЃС‚С‹Р»РЅРѕРµ РїСЂРµРІСЂР°С‰РµРЅРёРµ РІ РЅРµСЃС‚Р°РЅРґР°СЂС‚РЅС‹Р№ С„РѕСЂРјР°С‚ РєР°РґСЂР°
    	if (indicator == 5) {
    		uint8_t subdevice_code = (uint8_t)qmFromBigEndian<int8_t>(value_ptr+0);
    		uint8_t error_code = (uint8_t)qmFromBigEndian<int8_t>(value_ptr+2);
    		hardwareFailed.emit(subdevice_code, error_code);
    	}
    	break;
	}
	case 0x51:
	case 0x81: {
		if ((indicator == 3) || (indicator == 4)) { // "РєРѕРјР°РЅРґР° РІС‹РїРѕР»РЅРµРЅР°", "РєРѕРјР°РЅРґР° РЅРµ РІС‹РїРѕР»РЅРµРЅР°" ?
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

            if (ContentGuc.stage == GucTx) unblockGucTx = true;
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
        		if (sms_counter < 19)
        		{
        			syncro_recieve.erase(syncro_recieve.begin());
        			syncro_recieve.push_back(99);

        			if (check_rx_call()) {
        				sms_call_received = true;
        				qmDebugMessage(QmDebug::Dump, "sms call received");
        			}

        			sms_data_count = 0;
        		}

        } else if (indicator == 30) {
        	qmDebugMessage(QmDebug::Dump, "0x63 indicator 30");
            if (SmsLogicRole != SmsRoleIdle)
            {
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

                }
                if (sms_counter < 19)
                {
                    syncro_recieve.erase(syncro_recieve.begin());
                    syncro_recieve.push_back(data[9]); // CYC_N
                    qmDebugMessage(QmDebug::Dump, "recieve frame() count = %d", syncro_recieve.size());

                    if (check_rx_call())
                    {
                        sms_call_received = true;
                        ContentSms.R_ADR = data[8]; // todo: check
                        if (ContentSms.R_ADR > 32) pswf_ack = true;
                        qmDebugMessage(QmDebug::Dump, "sms call received");
                		getZone();
                		wzn_value = wzn_change(syncro_recieve);
                		syncro_recieve.clear();
                        for(int i = 0; i<18;i++)
                            syncro_recieve.push_back(99);
                    }
                }

                if (sms_counter > 76 && sms_counter < 83)
                {
                	prevTime();
                	uint8_t ack_code_calc = calc_ack_code(data[9]);
                	qmDebugMessage(QmDebug::Info, "recieve count sms = %d %d", ack_code_calc, data[10]);
                	if ((ack_code_calc == data[10]) && (data[9] != 99))
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
                if (ContentPSWF.R_ADR > 32) pswf_ack = true;
                recievedPswfBuffer.push_back(pswf_data);
                //getDataTime();
               // if (state_pswf == 0)
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
        	if (ContentGuc.stage != GucNone)
            recGuc();
        	else
        	{
                //initResetState();
        		goToVoice();
        		radio_state = radiostateSync;
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
                isGpsGuc = data[5] & 0x1; // TODO: С‚СЂРµР±СѓРµС‚СЃСЏ РїСЂРѕРІРµСЂРёС‚СЊ РІ СЂРµР°Р»СЊРЅС‹С… СѓСЃР»РѕРІРёСЏС…

                if (ContentGuc.stage == GucTxQuit){ ContentGuc.S_ADR = ((data[2] & 0x7) << 2) + ((data[3] & 0xC0) >> 6);  recievedGucQuitForTransm(ContentGuc.S_ADR); ContentGuc.stage = GucNone;}
            	else{
            		qmDebugMessage(QmDebug::Dump, "0x6B R_ADR %d : ", ContentGuc.R_ADR);
            		std::vector<uint8_t> guc;
            		for(int i = 0;i<data_len;i++){
            			qmDebugMessage(QmDebug::Dump, "0x6B recieved frame: %d , num %d", data[i],i);
            			guc.push_back(data[i]); // РїРѕ N РµРґРµРЅРёС† РґР°РЅРЅС‹С…
            		}
                    guc_vector.push_back(guc);
            		recGuc();
            	}
            }
        }
        break;
    }
    case 0x6F:
    {
    	value_ptr -= 1; // РєРѕСЃС‚С‹Р»РЅРѕРµ РїСЂРµРІСЂР°С‰РµРЅРёРµ РІ РЅРµСЃС‚Р°РЅРґР°СЂС‚РЅС‹Р№ С„РѕСЂРјР°С‚ РєР°РґСЂР°
    	value_len += 1; // РєРѕСЃС‚С‹Р»РЅРѕРµ РїСЂРµРІСЂР°С‰РµРЅРёРµ РІ РЅРµСЃС‚Р°РЅРґР°СЂС‚РЅС‹Р№ С„РѕСЂРјР°С‚ РєР°РґСЂР°
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
    	value_ptr -= 1; // РєРѕСЃС‚С‹Р»РЅРѕРµ РїСЂРµРІСЂР°С‰РµРЅРёРµ РІ РЅРµСЃС‚Р°РЅРґР°СЂС‚РЅС‹Р№ С„РѕСЂРјР°С‚ РєР°РґСЂР°
    	value_len += 1; // РєРѕСЃС‚С‹Р»РЅРѕРµ РїСЂРµРІСЂР°С‰РµРЅРёРµ РІ РЅРµСЃС‚Р°РЅРґР°СЂС‚РЅС‹Р№ С„РѕСЂРјР°С‚ РєР°РґСЂР°
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


void *DspController::getContentPSWF()
{
    return &ContentPSWF;
}

void DspController::sendSms(Module module)
{
    ContentSms.Frequency =  getFrequencySms();
    ContentSms.indicator = 20;
    ContentSms.SNR =  7;


    if (sms_counter >= 19 && sms_counter <= 38){
    	ContentSms.L_CODE = navigator->Calc_LCODE_SMS(
    			ContentSms.R_ADR, ContentSms.S_ADR,
				wzn_value, ContentSms.RN_KEY,
				date_time[0], date_time[1], date_time[2], date_time[3]);
    }
    else
    {

    	ContentSms.L_CODE = navigator->Calc_LCODE(
    			ContentSms.R_ADR, ContentSms.S_ADR,
				sms_counter-1, ContentSms.RN_KEY,
				date_time[0], date_time[1], date_time[2], date_time[3]);
    }


    if (sms_counter > 38 && sms_counter < 76) {
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
    if (sms_counter < 19 && SmsLogicRole == SmsRoleTx)
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
    if ((sms_counter > 38 && sms_counter < 77) && (SmsLogicRole == SmsRoleTx))
    {
        uint8_t FST_N =  calcFstn(ContentSms.R_ADR,ContentSms.S_ADR,ContentSms.RN_KEY,date_time[0],date_time[1],date_time[2],date_time[3],sms_counter - 39);
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
    if ((sms_counter > 19 && sms_counter < 38) && (SmsLogicRole == SmsRoleRx))
    {
        int wzn = wzn_value;

    	qmToBigEndian((uint8_t)ContentSms.SNR, tx_data+tx_data_len);
    	++tx_data_len;
    	qmToBigEndian((uint8_t)ContentSms.R_ADR, tx_data+tx_data_len);
    	++tx_data_len;
    	qmToBigEndian((uint8_t)ContentSms.S_ADR, tx_data+tx_data_len); // todo: РїРѕРјРµРЅСЏР» РјРµСЃС‚Р°РјРё
    	++tx_data_len;
    	qmToBigEndian((uint8_t)wzn, tx_data+tx_data_len);
    	++tx_data_len;

    	qmDebugMessage(QmDebug::Dump, "SADR: %d",ContentSms.S_ADR);
    	qmDebugMessage(QmDebug::Dump, "RADR: %d",ContentSms.R_ADR);

    	qmToBigEndian((uint8_t)ContentSms.L_CODE, tx_data+tx_data_len);
    	++tx_data_len;
    }

    if ((sms_counter > 76 && sms_counter < 83) && (SmsLogicRole == SmsRoleRx))
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



void DspController::prevTime()
{
	date_time[3]  = date_time[3] - 1;
	bool overflow = false;
   /* if (date_time[3] < 0)
    {
    	date_time[3] = 59; overflow = true;
    }
    if (overflow)
    {
    	date_time[2] = date_time[2] - 1;
    	if (date_time[2] > 0) overflow = false;
    }
    else
    {
    	date_time[2] = 59;
    }
    if (overflow)
    {
    	date_time[1] = date_time[1] - 1;
    	if (date_time[1] > 0) overflow = false;
    }
    else
    {
    	date_time[1] = 11;
    }*/
}


void DspController::recSms()
{
}


void DspController::getZone()
{
	for(int i = 0; i<syncro_recieve.size();i++)
	{
		if  (syncro_recieve.at(i) >=0   && syncro_recieve.at(i) <6 ) syncro_recieve[i] = 0;
		if  (syncro_recieve.at(i) > 5   && syncro_recieve.at(i) <12) syncro_recieve[i] = 1;
		if  (syncro_recieve.at(i) >= 12 && syncro_recieve.at(i) <18) syncro_recieve[i] = 2;
		if  (syncro_recieve.at(i) >=18  && syncro_recieve.at(i) <24) syncro_recieve[i] = 3;
		if  (syncro_recieve.at(i) >= 24 && syncro_recieve.at(i) <30) syncro_recieve[i] = 4;

    }
}

uint8_t DspController::getSmsCounter()
{
    return sms_counter;
}

bool DspController::generateSmsReceived()
{
    // 1. params for storage operation

	qmDebugMessage(QmDebug::Dump,"РљРѕР»РёС‡РµСЃС‚РІРѕ РїР°РєРµС‚РѕРІ sms data %d:",  recievedSmsBuffer.size());

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
          // 8. calculate text without CRC32 code
          pack_manager->decompressMass(crc_calcs,89,packet,110,7);
          // 9. interpretate to Win1251 encode
          pack_manager->to_Win1251(packet);

          // 10. create str consist data split ''
//          std::string str;
//          for(int i = 0; i<100;i++){
//          if ((i % 8 == 0) && (i>0)){
//             //str.push_back('\r');
//             str.push_back('\n');
//            }
//             str.push_back(packet[i]);
//          }

         // int len = str.length();
          int len = 100;
//          QM_ASSERT(len == 0);
          //if (len < 150)
         // std::copy(str.begin(),str.end(),sms_content);
          std::copy(&packet[0],&packet[99],sms_content);
          //str.erase(str.begin());
          sms_content[len] = '\0';
          // return sms status and signal(len, sms_content)
          smsPacketMessage(len);

          qmDebugMessage(QmDebug::Dump," Count of symbol for sms message %d", len);

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
    for(int i = 0; i<vect.size()-1;i++){
        if (vect[i] <= 4) wzn_mas[vect[i]] += 1;
    }
    int index = 0;
    for(int i = 0; i<4;i++)
    {
        if (wzn_mas[index] < wzn_mas[i]) index = i;
    }
    return index;
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
       //qmDebugMessage(QmDebug::Dump, "syncro_recieve value = %d", syncro_recieve.at(i));
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
	data_storage_fs->getAleStationAddress(param.pswf_r_adr);
	sendCommand(PSWFReceiver, PswfRxRAdr, param);

	ParameterValue comandValue;
	comandValue.radio_mode = RadioModeOff;
	sendCommand(TxRadiopath, TxRadioMode, comandValue);
	comandValue.pswf_indicator = RadioModePSWF;
	sendCommand(RxRadiopath, RxRadioMode, comandValue);
	pswfRxStateSync = 0;
	radio_state = radiostatePswfRxPrepare;
	pswf_rec = 0;

	SmsLogicRole = SmsRoleIdle;
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
    data_storage_fs->getAleStationAddress(ContentPSWF.S_ADR);

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

    for(int i = 0; i<255; i++) rs_data_clear[i] = 1;

    tx_call_ask_vector.erase(tx_call_ask_vector.begin(),tx_call_ask_vector.end());
    quit_vector.erase(quit_vector.begin(),quit_vector.end());

//    ParameterValue param;
//    data_storage_fs->getAleStationAddress(param.pswf_r_adr);
//    sendCommand(PSWFReceiver, PswfRxRAdr, param);

    setRx();

    smsRxStateSync = 0;
    radio_state = radiostateSms;
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
    ContentSms.R_ADR = r_adr;
    ContentSms.CYC_N = 0;

    count_clear = 0;

    cntChvc = 7;

    int ind = strlen((const char*)message);

    int data_sms[255];

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
    for(int i = 0;i<255;i++) rs_data_clear[i] = 1;
    for(int i = 0; i<255;i++) data_sms[i] = (int)ContentSms.message[i];

    encode_rs(data_sms,&data_sms[93],&rs_255_93);
    for(int i = 0; i<255;i++)ContentSms.message[i]  = data_sms[i];


    radio_state = radiostateSms;

    setTx();

    updateSmsStatus(getSmsForUiStage());
}

void DspController::startGucTransmitting(int r_adr, int speed_tx, std::vector<int> command, bool isGps)
{
    qmDebugMessage(QmDebug::Dump, "startGucTransmitting(%i, %i)", r_adr, speed_tx);
    QM_ASSERT(is_ready);

    ContentGuc.indicator = 20;
    ContentGuc.type = 1;
    ContentGuc.chip_time = 2;
    ContentGuc.WIDTH_SIGNAL = 1;
    data_storage_fs->getAleStationAddress(ContentGuc.S_ADR);
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

    initResetState();


    ParameterValue comandValue;
    comandValue.radio_mode = RadioModeOff;// РѕС‚РєР»СЋС‡РёР»Рё РїСЂРёРµРј
    sendCommandEasy(RxRadiopath, RxRadioMode, comandValue);
    comandValue.guc_mode = RadioModeSazhenData; // РІРєР»СЋС‡РёР»Рё 11 СЂРµР¶РёРј
    sendCommandEasy(TxRadiopath, TxRadioMode, comandValue);
    if (freqGucValue != 0)
    comandValue.frequency =  freqGucValue;//3000000;
    sendCommandEasy(RxRadiopath, RxFrequency, comandValue);
    if (unblockGucTx == false){ /* do sleep*/ QmThread::msleep(100); }
    unblockGucTx = false;
    sendCommandEasy(TxRadiopath, TxFrequency, comandValue);
    radio_state = radiostateGucTxPrepare;
    gucTxStateSync = 0;

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
    comandValue.radio_mode = RadioModeOff;// РѕС‚РєР»СЋС‡РёР»Рё РїСЂРёРµРј
    sendCommand(RxRadiopath, RxRadioMode, comandValue);
    comandValue.guc_mode = RadioModeSazhenData; // РІРєР»СЋС‡РёР»Рё 11 СЂРµР¶РёРј
    sendCommandEasy(TxRadiopath, TxRadioMode, comandValue);
    comandValue.frequency = freqGucValue;//3000000;
    sendCommandEasy(RxRadiopath, RxFrequency, comandValue);
    sendCommandEasy(TxRadiopath, TxFrequency, comandValue);
    radio_state = radiostateGucTxPrepare;
    gucTxStateSync = 0;
    if (ContentGuc.stage == GucRxQuit)
    	ContentGuc.stage = GucNone;
    else
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
	data_storage_fs->getAleStationAddress(ContentGuc.S_ADR);


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
	pack[2] = (ContentGuc.R_ADR & 0x1F) << 3;  // 5 Р±РёС‚
	pack[2] |= (ContentGuc.S_ADR & 0x1F) >> 2; // 3 Р±РёС‚Р°
	pack[1] |= (ContentGuc.S_ADR & 0x1F) << 6; // 2 Р±РёС‚Р°
	pack[1] |= (ContentGuc.uin >> 2) & 0x3F;   // 6 Р±РёС‚
	pack[0] = (ContentGuc.uin << 6) & 0xC0;    // 2 Р±РёС‚Р°

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
    comandValue.radio_mode = RadioModeOff;// РѕС‚РєР»СЋС‡РёР»Рё СЂР°РґРёРѕСЂРµР¶РёРј
    sendCommandEasy(TxRadiopath, TxRadioMode, comandValue);
    comandValue.guc_mode = 3;
    sendCommandEasy(RadioLineNotPswf, 0 ,comandValue);
    data_storage_fs->getAleStationAddress(comandValue.guc_mode);
    sendCommandEasy(RadioLineNotPswf, 3 ,comandValue); // РѕС‚РєР»СЋС‡РёС‚СЊ РЅРёР·РєРѕСЃРєРѕСЂРѕСЃС‚РЅРѕР№ РјРѕРґРµРј

    // TODO: СѓСЃС‚Р°РЅРѕРІРєР° РїРѕР»РѕСЃС‹ С‡Р°СЃС‚РѕС‚ 3,1 РєР“С†
    comandValue.guc_mode = 3;
   sendCommandEasy(RadioLineNotPswf, 1, comandValue);
   QmThread::msleep(100);
    //-----------------------------------

    comandValue.guc_mode = RadioModeSazhenData; // РІРєР»СЋС‡РёР»Рё 11 СЂРµР¶РёРј
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
    guc_timer->setInterval(GUC_TIMER_INTERVAL); // TODO: РІРѕР·РјРѕР¶РЅРѕ РёР·РјРµРЅРµРЅРёРµ РёРЅС‚РµСЂРІР°Р»Р°
    int size = guc_vector.size();

    qmDebugMessage(QmDebug::Dump, "size guc command %d", guc_vector.size());

    if (size > 0){
        (isGpsGuc) ? recievedGucResp(1) : recievedGucResp(0);
        if (ContentGuc.stage != GucTxQuit)
        startGucTransmitting();
        if (!failQuitGuc)
        sendGucQuit();
        else failQuitGuc = false;
    }
    else
    {
        radio_state = radiostateSync;
        if (ContentGuc.stage == GucTxQuit) recievedGucQuitForTransm(-1);
    }
    guc_vector.clear();
}

uint8_t* DspController::get_guc_vector()
{
	int num = (guc_vector.at(0).at(3) & 0x3f) << 1;
	num +=    (guc_vector.at(0).at(4) & 0x80) >> 7;

	//РїРѕР»СѓС‡РµРЅРёРµ РєРѕР»РёС‡РµСЃС‚РІР° СЌР»РµРјРµРЅС‚РѕРІ РІ РІРµРєС‚РѕСЂРµ
	guc_text[0] = num;

    uint8_t out[120];
    for(int i = 0; i<120;i++) out[i] = 0;
    int crc_coord_len = 0;

    // РµСЃР»Рё СЃ РєРѕРѕСЂРґРёРЅР°С‚Р°РјРё, С‚Рѕ РІС‹Р±РѕСЂРєР° РїРѕ РѕРґРЅРѕРјСѓ Р°Р»РіРѕСЂРёС‚РјСѓ, РёРЅР°С‡Рµ РїРѕ РґСЂСѓРіРѕРјСѓ
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
        // -- Р—Р°РїРёСЃР°Р»Рё РєРѕРѕСЂРґРёРЅР°С‚С‹, РЅР°С‡РёРЅР°СЏ СЃ РїРµСЂРІРѕР№ РїРѕР·РёС†РёРё РјР°СЃСЃРёРІР° guc_text
        for(int i = 0; i< count;i++){
        	if (i < num)
        		guc_text[9 + i+1] = guc_vector.at(0).at(7+i);
        	else
        		guc_text[9 + i+1] = 0;
        }
        // -- Р—Р°РїРёcР°Р»Рё РґР°РЅРЅС‹Рµ, РЅР°С‡РёРЅР°СЏ СЃ 10-Р№ РїРѕР·РёС†РёРё РЅР°С€РµРіРѕ РјР°СЃСЃРёРІР°
        std::vector<bool> data;
        for(int i = 0; i< 8; i++) pack_manager->addBytetoBitsArray(guc_text[i+1],data,8);
        // РґРѕР±Р°РІРёР»Рё РєРѕРѕСЂРґРёРЅР°С‚С‹ Рє Р±РёС‚РѕРІРѕРјСѓ РІРµРєС‚РѕСЂСѓ

        bool quadrant = guc_text[9] & (1 << 7);
        data.push_back(quadrant);
        quadrant = guc_text[9] & (1 >> 6);
        data.push_back(quadrant);
        // РґРѕР±Р°РІРёР»Рё Рє Р±РёС‚РѕРІРѕРјСѓ РІРµРєС‚РѕСЂСѓ РєРІР°РґСЂР°РЅС‚
        for(int i = 0; i<num;i++) pack_manager->addBytetoBitsArray(guc_text[9 + i+1],data,7);
        // РґРѕР±Р°РІРёР»Рё Рє Р±РёС‚РѕРІРѕРјСѓ РІРµРєС‚РѕСЂСѓ РґР°РЅРЅС‹Рµ РїРѕ 7 Р±РёС‚
        pack_manager->getArrayByteFromBit(data,out);
        // Р·Р°РїРёСЃР°Р»Рё РІ РІС‹С…РѕРґРЅРѕР№ РјР°СЃСЃРёРІ РїСЂРµРѕР±СЂР°Р·РѕРІР°РЅРЅС‹Рµ РґР°РЅРЅС‹Рµ РёР· Р±РёС‚РѕРІРѕРіРѕ РјР°СЃСЃРёРІР° РїРѕ Р°РЅРѕР»РѕРіРёРё СЃ С„РѕСЂРјРёСЂРѕРІР°РЅРёРµ РїР°РєРµС‚Р° РґР»СЏ CRC32 РЅР° РїРµСЂРµРґР°С‡Рµ
        crc_coord_len = data.size() / 8;
        // РїРѕР»СѓС‡РёР»Рё РґР»РёРЅРЅСѓ РїР°РєРµС‚Р°

        for(int i = 1;i<9; i++)  guc_text[num+i] = guc_text[i]; // РїРѕСЃР»Рµ РґР°РЅРЅС‹С… РїРµСЂРµРґР°РґРёРј РєРѕРѕСЂРґРёРЅР°С‚С‹
        for(int i = 0;i<num;i++) guc_text[i+1] = guc_text[9+i+1];

    }

    else
    {
    	std::vector<bool> data;
        for(int i = 0; i<num;i++) pack_manager->addBytetoBitsArray(guc_vector.at(0).at(7+i),data,7);
        for(int i = 0; i<count;i++) pack_manager->getArrayByteFromBit(data,out);
        guc_text[0] = num;
        for(int i = 0; i<num;i++) guc_text[i+1] = guc_vector.at(0).at(7+i);
//        for(int i = 0;  i< count;i++){
//            int sdvig  = (i+1) % 8;
//            if (sdvig != 0)
//                out[i] = (guc_text[i+1] << sdvig) + (guc_text[i+2] >> (7 -  sdvig));
//            else
//                out[i] = guc_text[i+1];
//
//        }

    }


	// РґРѕСЃС‚Р°РµРј crc32 СЃСѓРјРјСѓ РёР· РєРѕРЅС†Р° РїР°РєРµС‚Р°
	int m = 3;
	uint32_t crc_packet = 0;
	int l = 0;
	while(m >=0){
		uint8_t sum = guc_vector.at(0).at(guc_vector.at(0).size() - 1 - m);
		crc_packet += sum << (8*m);
		l++;
		m--;
	}

	// СЃС‡РёС‚Р°РµРј crc32 СЃСѓРјРјСѓ
    uint32_t crc = 0;
    int value  = (isGpsGuc) ? crc_coord_len : num;
    // РІС‹Р±СЂР°Р»Рё РґР»РёРЅРЅСѓ, РёСЃС…РѕРґСЏ РёР· СЂРµР¶РёРјР° РїРµСЂРµРґР°С‡Рё

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


void DspController::startSMSCmdTransmitting(SmsStage stage)
{
    qmDebugMessage(QmDebug::Dump, "startSMSCmdTransmitting(%d)", stage);
    QM_ASSERT(is_ready);

    //getDataTime();
    ContentSms.Frequency =   getFrequencySms();
    ContentSms.indicator = 20;
    ContentSms.TYPE = 0;

    ParameterValue comandValue;
    comandValue.radio_mode = RadioModeOff;
    sendCommandEasy(RxRadiopath, RxRadioMode, comandValue);
    comandValue.pswf_indicator = RadioModePSWF;
    sendCommandEasy(TxRadiopath, TxRadioMode, comandValue);

    smsTxStateSync = 0;
    radio_state = radiostateSms;
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


} /* namespace Multiradio */

#include "qmdebug_domains_start.h"
QMDEBUG_DEFINE_DOMAIN(dspcontroller, LevelVerbose)
#include "qmdebug_domains_end.h"
