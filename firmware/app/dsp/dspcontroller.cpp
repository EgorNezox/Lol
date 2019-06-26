/**
 ******************************************************************************
 * @file    dspcontroller.cpp
 * @author  Artem Pisarenko, PMR dept. software team, ONIIP, PJSC
 * @date    22.12.2015
 *
 ******************************************************************************
 */

#include "qm.h"
#define QMDEBUGDOMAIN	dspcontroller
#include "qmdebug.h"
#include "qmendian.h"
#include "qmtimer.h"
#include "qmthread.h"
#include "qmiopin.h"
#include "dspcontroller.h"
#include "dsptransport.h"
#include <cstring>
#include "../dsp/rs_tms.h"
#include "../synchro/virtual_timer.h"
#include "PswfModes.h"
#include "qmm25pdevice.h"

#define	platformhwKeyEnter		15
#define	platformhwKeyBack		0
#define	platformhwKeyUp			1
#define	platformhwKeyDown		2
#define	platformhwKeyLeft		3
#define	platformhwKeyRight		7
#define	platformhwKey0			11
#define	platformhwKey1			4
#define	platformhwKey2			8
#define	platformhwKey3			12
#define	platformhwKey4			5
#define	platformhwKey5			9
#define	platformhwKey6			13
#define	platformhwKey7			6
#define	platformhwKey8			10
#define	platformhwKey9			14



#define DEFAULT_PACKET_HEADER_LEN	2
#define hw_rtc                      1
#define hw_usb						2
#define DefkeyValue 631


#define GUC_TIMER_ACK_WAIT_INTERVAL 180000
#define GUC_TIMER_INTERVAL_REC 30000

#define VIRTUAL_TIME 120

#define NUMS 0 // need = 0   9 for debug
#define startVirtTxPhaseIndex 0;

#define SAZHEN_BATTARY_VOLTAGE 0

namespace Multiradio {

using namespace Galua;
rs_settings rs_255_93;



DspController::DspController(int uart_resource, int reset_iopin_resource, Navigation::Navigator *navigator, DataStorage::FS *data_storage_fs, QmObject *parent) :
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
	transport = new DspTransport(uart_resource, 2, this);
	transport->receivedFrame.connect(sigc::mem_fun(this, &DspController::processReceivedFrame));

	sync_pulse_delay_timer = new QmTimer(true, this);
    sync_pulse_delay_timer->setSingleShot(true);
    sync_pulse_delay_timer->setInterval(100);
	sync_pulse_delay_timer->timeout.connect(sigc::mem_fun(this, &DspController::processSyncPulse));

	guc_rx_quit_timer = new QmTimer(true,this);
	guc_rx_quit_timer->setInterval(1000);
	guc_rx_quit_timer->setSingleShot(true);
	guc_rx_quit_timer->timeout.connect(sigc::mem_fun(this, &DspController::onGucWaitingQuitTimeout));

	initResetState();

	pswf_module = new PswfModes(this);

    if (navigator != 0)
    {
    	this->navigator = navigator;
    	navigator->syncPulse.connect(sigc::mem_fun(this, &DspController::syncPulseDetected));
    }
    this->data_storage_fs = data_storage_fs;

    fwd_wave 	 = 0;
    ref_wave 	 = 0;
    command_tx30 = 0;
    private_lcode= 0;
    count_clear	 = 0;
    QNB 		 = 0;
    pswf_rec 	 = 0;
    success_pswf = 30;
    rxRssiLevel  = 0;

    pswf_first_packet_received = false;
    pswf_ack 				   = false;
    pswf_ack_tx 			   = false;

    cmd_queue  = new std::list<DspCommand>();
    counterSms = new int[8]{18,18,37,6,18,18,37,6};
    ContentSms.stage = StageNone;

    initrs(rs_255_93);
    GenerateGaloisField(&rs_255_93);
    gen_poly(&rs_255_93);

    pack_manager = new PackageManager();

    for (int i = 0; i<18;i++)
    {
    	syncro_recieve.push_back(99);
    	snr.push_back(0);
    	waveZone.push_back(0);
    }

    waveZone.push_back(0); // size must be 19

    for(int i = 0;i<50;i++)
    	guc_text[i] = '\0';

    sms_call_received    = false;
    retranslation_active = false;

    for(int i = 0;i<255;i++)
    	rs_data_clear[i] = 1;
    for(uint8_t i = 0; i <= 100; i++)
    	sms_content[i] = 0;

    ContentPSWF.RN_KEY = DefkeyValue;
    ContentSms.RN_KEY = DefkeyValue;

    uint16_t rn_key = 0;

    bool isKeyReaded = data_storage_fs->getFhssKey(rn_key);
    if (isKeyReaded)
    {
        ContentPSWF.RN_KEY = rn_key;
        ContentSms.RN_KEY = rn_key;
    }

    waveInfoTimer = new QmTimer();

    waveInfoTimer->setInterval(1500);
    waveInfoTimer->setSingleShot(true);
    waveInfoTimer->timeout.connect(sigc::mem_fun(this, &DspController::clearWaveInfo));

#ifndef PORT__PCSIMULATOR
    rtc = new QmRtc(hw_rtc);
	rtc->wakeup.connect(sigc::mem_fun(this,&DspController::wakeUpTimer));
#endif
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
    delete guc_rx_quit_timer;
    delete cmd_queue;
    delete rtc;
}

void DspController::dspReset()
{
	reset_iopin->writeOutput(QmIopin::Level_Low);
	//QmThread::msleep(20);
	reset_iopin->writeOutput(QmIopin::Level_High);
	if (guc_rx_quit_timer)stopGucTimer();
}

bool DspController::isReady() {
	return is_ready;
}

void DspController::startServicing()
{

	initResetState();
//	reset_iopin->writeOutput(QmIopin::Level_Low);
//	QmThread::msleep(10);
	transport->enable();
//	reset_iopin->writeOutput(QmIopin::Level_High);

	queryVersionDSP();

	queryVersionPLD();

	startup_timer->start(10000);
}

void DspController::queryVersionDSP()
{
	uint8_t data[10] =  {0};
	uint8_t data_len =   0;
	data[0]   = 0;
	data[1]   = 2;
	data_len += 2;
	transmithFrame(0x10,data,data_len);
}

void DspController::queryVersionPLD()
{
	uint8_t data[10] =  {0};
	uint8_t data_len =   0;
	data[0]   = 0;
	data[1]   = 3;
	data_len += 2;
	transmithFrame(0x10,data,data_len);
}


void DspController::push_queue()
{
	if (!cmd_queue->empty())
	{
		DspCommand cmd;
		cmd = cmd_queue->front();
		cmd_queue->pop_front();
		sendCommand(cmd.module, cmd.code, cmd.value);
	}
}

void DspController::setRadioParameters(RadioMode mode, uint32_t frequency) {
	bool processing_required = true;
	QM_ASSERT(is_ready);
	current_radio_mode = mode;
	current_radio_frequency = frequency;

	if (radio_state == radiostateSync)
	{
		if (current_radio_operation == RadioOperationOff)
		{
			radio_state = radiostateCmdRxFreq;
			processRadioState();
			return;
		}

		if (current_radio_operation == RadioOperationRxMode)
		{
			radio_state = radiostateCmdModeOffRx;
			processRadioState();
			return;
		}

		if (current_radio_operation == RadioOperationTxMode || RadioOperationCarrierTx)
		{
			radio_state = radiostateCmdModeOffTx;
			processRadioState();
			return;
		}
	}

	if (radio_state == radiostateCmdRxOff  || radiostateCmdTxOff
					|| radiostateCmdRxFreq || radiostateCmdTxFreq)
	{
		radio_state = radiostateCmdRxFreq;
		processRadioState();
		return;
	}

	if (radio_state == radiostateCmdTxPower || radiostateCmdTxMode
					|| radiostateCmdCarrierTx)
	{
		radio_state = radiostateCmdModeOffTx;
		processRadioState();
		return;
	}

	if (radio_state == radiostateCmdRxMode)
	{
		radio_state = radiostateCmdModeOffRx;
		processRadioState();
		return;
	}
}

void DspController::sendBatteryVoltage(int voltage)
{
#if SAZHEN_BATTARY_VOLTAGE
	// вольт * 10
    ParameterValue command;
    command.voltage = voltage / 100;
    sendCommandEasy(TxRadiopath, 10, command);
#endif
}

void DspController::sendHeadsetType(uint8_t type)
{
//  0 - skzi open
//  1 - polev open
//  2 - skzi close

    ParameterValue command;
    command.headsetType = type;

    uint8_t tx_address = 0x90;
    uint8_t indicator = 2;
    uint8_t valueType = 7;
    uint8_t tx_data[DspTransport::MAX_FRAME_DATA_SIZE];
    uint8_t tx_data_len = 0;

    qmToBigEndian(indicator, tx_data + tx_data_len);             ++tx_data_len;
    qmToBigEndian(valueType, tx_data + tx_data_len);             ++tx_data_len;
    qmToBigEndian(command.headsetType, tx_data + tx_data_len);   ++tx_data_len;

    transport->transmitFrame(tx_address, tx_data, tx_data_len);
}

void DspController::setRadioOperation(RadioOperation operation)
{
	QM_ASSERT(is_ready);
	if (operation == current_radio_operation)
		return;
	bool processing_required = false;
	switch (operation)
	{
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
	if (processing_required)processRadioState();
}

void DspController::setRadioSquelch(uint8_t value)
{
	QM_ASSERT(is_ready);
	ParameterValue command_value;
	command_value.squelch = value;
	sendCommandEasy(RxRadiopath, RxSquelch, command_value);
}

void DspController::setAudioVolumeLevel(uint8_t volume_level)
{
    QM_ASSERT(is_ready);
    if (!resyncPendingCommand())return;
    ParameterValue command_value;
    command_value.volume_level = volume_level;
    sendCommand(Audiopath, AudioVolumeLevel, command_value);
}

void DspController::setAudioTypeGarniture(uint8_t type)
{
	QM_ASSERT(is_ready);
	if (!resyncPendingCommand())return;
	ParameterValue command_value;
	command_value.signal_type_garn = type;
	sendCommand(Audiopath, AudioTypeGarnityre, command_value);
}

void DspController::setAudioMicLevel(uint8_t value)
{
    QM_ASSERT(is_ready);
    ParameterValue command_value;
    command_value.mic_amplify = value;
    sendCommandEasy(Audiopath, AudioMicAmplify, command_value);
}

void DspController::setAGCParameters(uint8_t agc_mode,int RadioPath)
{
    QM_ASSERT(is_ready);
    if (!resyncPendingCommand()) return;
    ParameterValue command_value;
    command_value.agc_mode = agc_mode;
    if (RadioPath == 0)
      sendCommand(RxRadiopath,AGCRX,command_value);
    else
      sendCommand(TxRadiopath,AGCTX,command_value);
    resyncPendingCommand();
}

//void DspController::getSwr()
//
//{
//    QM_ASSERT(is_ready);
//    if (!resyncPendingCommand()) return;
//
//    ParameterValue commandValue;
//    commandValue.swf_mode = 6;
//    sendCommandEasy(TxRadiopath, 6, commandValue);
//}

void DspController::syncPulseDetected()
{
	sync_pulse_delay_timer->start();
    if (!virtual_mode) {vm1Pps();};
}

void DspController::processSyncPulse()
{
	//qmDebugMessage(QmDebug::Dump, "processSyncPulse() SmsLogicRole = %d", SmsLogicRole);
	if (!is_ready || virtual_mode) return;
    if (radio_state == radiostatePswf) changePswfFrequency();
    if (radio_state == radiostateSms)  changeSmsFrequency();
}

void DspController::getDataTime()
{
    Navigation::Coord_Date date = navigator->getCoordDate();

    char day_ch[3] = {0,0,0};
    char hr_ch [3] = {0,0,0};
    char mn_ch [3] = {0,0,0};
    char sec_ch[3] = {0,0,0};

    memcpy(day_ch,&date.data[0],2);
    memcpy(hr_ch, &date.time[0],2);
    memcpy(mn_ch, &date.time[2],2);
    memcpy(sec_ch,&date.time[4],2);

    int day = atoi(day_ch);
    int hrs = atoi(hr_ch );
    int min = atoi(mn_ch );
    int sec = atoi(sec_ch);

    date_time[0] = day;
    date_time[1] = hrs;
    date_time[2] = min;
    date_time[3] = sec;

    addSeconds(date_time);
    //qmDebugMessage(QmDebug::Dump, "getDataTime(): %d %d %d %d", date_time[0], date_time[1], date_time[2], date_time[3]);
}

void DspController::setRx()
{
    ParameterValue comandValue;
    comandValue.radio_mode = RadioModeOff;
    sendCommandEasy(TxRadiopath, TxRadioMode, comandValue);
    comandValue.pswf_indicator = RadioModePSWF;
    sendCommandEasy(RxRadiopath, RxRadioMode, comandValue);
    rxModeSetting();
}

void DspController::setTx()
{
	ParameterValue comandValue;
	comandValue.radio_mode = RadioModeOff;
	sendCommandEasy(RxRadiopath, RxRadioMode, comandValue);
	comandValue.pswf_indicator = RadioModePSWF;
	sendCommandEasy(TxRadiopath, TxRadioMode, comandValue);
    txModeSetting();
}

void DspController::sendPswf()
{
    PswfModes::trFrame f = pswf_module->sendPswf();
    transport->transmitFrame(f.address, f.data, f.len);
}

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

void DspController::exitVoceMode()
{
	completedStationMode(true);
	ParameterValue command_value;
	command_value.frequency = current_radio_frequency;
	sendCommandEasy(RxRadiopath, RxFrequency, command_value);
	sendCommandEasy(TxRadiopath, TxFrequency, command_value);
}

void DspController::changePswfFrequency()
{
 //   qmDebugMessage(QmDebug::Dump, "changePswfFrequency() r_adr = %d,s_adr = %d", ContentPSWF.R_ADR,ContentPSWF.S_ADR);
	if (!virtual_mode)
	{
		getDataTime();
		addSeconds(date_time);
	}

	if (CondComLogicRole == CondComTx)
	{
		pswf_module->LogicPswfTx();
	}
	else if (CondComLogicRole == CondComRx)
	{
		pswf_module->LogicPswfRx();
	}
}

void DspController::setPswfRxFreq()
{
	ContentPSWF.Frequency = getFrequency(0); //pswf = 0, sms = 1
	ParameterValue param;
	param.frequency = ContentPSWF.Frequency;
	sendCommandEasy(PSWFReceiver, PswfRxFrequency, param);
}

void DspController::changeSmsFrequency()
{
    //qmDebugMessage(QmDebug::Dump, "changeSmsFrequency() r_adr = %d,s_adr = %d", ContentSms.R_ADR,ContentSms.S_ADR);
	if (!virtual_mode)
	{
	  getDataTime();
	  addSeconds(date_time);
      //qmDebugMessage(QmDebug::Dump, "changeSmsFrequency()): %d %d %d %d", date_time[0], date_time[1], date_time[2], date_time[3]);
	}

	if (SmsLogicRole == SmsRoleTx)
	{
		pswf_module->LogicSmsTx();
	}
	if (SmsLogicRole == SmsRoleRx)
	{
        pswf_module->LogicSmsRx();
	}

//	 qmDebugMessage(QmDebug::Dump, "changeSmsFrequency() sms_counter = %d", sms_counter);
    static uint8_t tempCounter = sms_counter;
    if ((tempCounter != sms_counter && sms_counter % 2 == 0))
      smsCounterChanged(sms_counter);
}

void DspController::resetSmsState()
{
	smsSmallCounter = 0;
	sms_counter = 0;
	radio_state = radiostateSync;

	smsFind  = false;
	ok_quit = 0;
	smsError = 0;
	std::memset(rs_data_clear,1,sizeof(rs_data_clear));
    SmsLogicRole = SmsRoleIdle;
    exitVoceMode();
}

bool DspController::checkForTxAnswer()
{
	if (tx_call_ask_vector.size() >= 2)
	{
		wzn_value = pswf_module->wzn_change(tx_call_ask_vector);
		//qmDebugMessage(QmDebug::Dump, "checkForTxAnswer() wzn_value" ,wzn_value);
		tx_call_ask_vector.resize(0);

		return true;
	}

	tx_call_ask_vector.resize(0);
	return false;
}

void DspController::setrRxFreq()
{
    ContentSms.Frequency =  getFrequency(1); //pswf = 0, sms = 1

#if DEBUGSHOWFREQ
    smsCounterFreq(ContentSms.Frequency);
#endif
    ParameterValue param;
    param.frequency = ContentSms.Frequency;
    if ((SmsLogicRole == SmsRoleRx) && (sms_counter >= 38 && sms_counter < 77))
    	sendCommandEasy(PSWFReceiver, PswfRxFreqSignal, param);
    else
    sendCommandEasy(PSWFReceiver, PswfRxFrequency, param);
}

int DspController::prevSecond(int second)
{
	return  (second == 0) ? (59) : (second - 1);
}

int DspController::getFrequency(uint8_t mode) //pswf = 0, sms = 1
{
	int fr_sh = 0;

	int RN_KEY = mode ? ContentSms.RN_KEY : ContentPSWF.RN_KEY;

	switch (mode){
		case 0: {
			if (virtual_mode)
				fr_sh = pswf_module->CalcShiftFreq(RN_KEY,d.day,timeVirtual.hours,timeVirtual.minutes,timeVirtual.seconds);
			else
				fr_sh = pswf_module->CalcShiftFreq(RN_KEY,date_time[0],date_time[1],date_time[2],date_time[3]);
			break;
		}
		case 1: {
			if (virtual_mode)
				fr_sh = pswf_module->CalcSmsTransmitFreq(RN_KEY,d.day,timeVirtual.hours,timeVirtual.minutes,timeVirtual.seconds);
			else
				fr_sh = pswf_module->CalcSmsTransmitFreq(RN_KEY,date_time[0],date_time[1],date_time[2],date_time[3]);
			break;
		}
	}

	fr_sh += 1622;
	fr_sh = fr_sh * 1000;

	for(int i = 0; i < 32; i += 2)
	{
		if((fr_sh >= frequence_bandwidth[i]) && (fr_sh <= frequence_bandwidth[i+1]))
			break;
		else
			fr_sh += (frequence_bandwidth[i+2] - frequence_bandwidth[i+1]);
	}

	//qmDebugMessage(QmDebug::Dump,"frequency:  %d ", fr_sh);
	if (sms_counter >= 77)
		qmDebugMessage(QmDebug::Dump,"frequency: freq %d RN_KEY %d day %d  h %d m %d s %d", fr_sh, RN_KEY,d.day,timeVirtual.hours,timeVirtual.minutes,timeVirtual.seconds);
    return fr_sh;
}

void DspController::setRnKey(int keyValue)
{
    ContentPSWF.RN_KEY = keyValue;
    ContentSms.RN_KEY  = keyValue;
}

void DspController::initResetState()
{
	radio_state = radiostateSync;
	current_radio_operation = RadioOperationOff;
	current_radio_mode      = RadioModeOff;
    current_radio_frequency = 0;

	pending_command->in_progress = false;
	if (guc_rx_quit_timer)stopGucTimer();
}

void DspController::setAdr()
{
	ParameterValue param;
    param.pswf_r_adr = stationAddress;
	sendCommandEasy(PSWFReceiver, PswfRxRAdr, param);

	param.guc_mode = stationAddress;
	sendCommandEasy(RadioLineNotPswf, 3, param);

	startGucIntoVoice();

}

void DspController::processStartup(uint16_t id, uint16_t major_version, uint16_t minor_version)
{

	if (!is_ready)
	{
	//	qmDebugMessage(QmDebug::Info, "DSP started (id=0x%02X, version=%u.%u)", id, major_version, minor_version);
		startup_timer->stop();
		is_ready = true;
	}
	else
	{
	//	qmDebugMessage(QmDebug::Warning, "DSP restart detected");
		 hardwareFailed.emit(2, 99);
		initResetState();
	}
	started();

	//goToVoice();
	//startGucIntoVoice();
}

void DspController::processStartupTimeout()
{
//	qmDebugMessage(QmDebug::Warning, "DSP startup timeout");
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

bool DspController::startRadioRxMode()
{
	switch (radio_state)
	{
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

bool DspController::startRadioTxMode()
{
	switch (radio_state)
	{
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

bool DspController::startRadioCarrierTx()
{
	switch (radio_state)
	{
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

void DspController::processRadioState()
{
	if (!resyncPendingCommand()) return;
	ParameterValue command_value;
	switch (radio_state)
	{
	case radiostateSync:
		break;
	case radiostateCmdRxFreq:
	{
		command_value.frequency = current_radio_frequency;
		sendCommand(RxRadiopath, RxFrequency, command_value);
		break;
	}
	case radiostateCmdTxFreq:
	{
		command_value.frequency = current_radio_frequency;
		sendCommand(TxRadiopath, TxFrequency, command_value);
		break;
	}
	case radiostateCmdModeOffRx:
	case radiostateCmdRxOff:
	{
		command_value.radio_mode = RadioModeOff;
		sendCommand(RxRadiopath, RxRadioMode, command_value);
		break;
	}
	case radiostateCmdModeOffTx:
	case radiostateCmdTxOff:
	{
		command_value.radio_mode = RadioModeOff;
		sendCommand(TxRadiopath, TxRadioMode, command_value);
		break;
	}
	case radiostateCmdRxMode:
	{
		command_value.radio_mode = current_radio_mode;
		sendCommand(RxRadiopath, RxRadioMode, command_value);
		//startGucIntoVoice();
		break;
	}
	case radiostateCmdTxPower:
	{
		if (current_radio_operation != RadioOperationCarrierTx)
			command_value.power = (current_radio_frequency >= 30000000) ? 80 : 100;
		else
			command_value.power = 80;
		sendCommand(TxRadiopath, TxPower, command_value);
		break;
	}
	case radiostateCmdTxMode:
	{
		command_value.radio_mode = current_radio_mode;
		sendCommand(TxRadiopath, TxRadioMode, command_value);
		break;
	}
	case radiostateCmdCarrierTx:
	{
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
//	qmDebugMessage(QmDebug::Warning, "dsp response timed out");
	syncPendingCommand();
}

void DspController::processCommandResponse(bool success, Module module, int code, ParameterValue value) {
	QM_UNUSED(value);
	if(!pending_command->in_progress) {
//		qmDebugMessage(QmDebug::Warning, "dsp response, but no command was sent");
		return;
	}
	if ((module == pending_command->module) /*&& (code == pending_command->code)*/) {
		command_timer->stop();
//		if (!success)
//			qmDebugMessage(QmDebug::Info, "dsp command failed (module=0x%02X, code=0x%02X)", module, code);
		syncPendingCommand();
	} else {
		//qmDebugMessage(QmDebug::Warning, "dsp command response was unexpected (module=0x%02X, code=0x%02X)", module, code);
	}

}

void DspController::syncPendingCommand()
{
	//qmDebugMessage(QmDebug::Dump,"reload progress state");
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
	//qmDebugMessage(QmDebug::Dump, "sendCommand(%d, %d) transmiting", module, code);
	uint8_t tx_address;
	uint8_t tx_data[DspTransport::MAX_FRAME_DATA_SIZE];
	int tx_data_len = DEFAULT_PACKET_HEADER_LEN;


	qmToBigEndian((uint8_t)2, tx_data+0);
	qmToBigEndian((uint8_t)code, tx_data+1);

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
		case 6:
		{
			//qmToBigEndian((uint8_t)value.swf_mode, tx_data+tx_data_len);
			//tx_data_len += 1;
			tx_data[0] = 0;
			break;
		}
		case 7:
		case 8:
			qmToBigEndian((uint8_t)value.agc_mode, tx_data+tx_data_len);
			tx_data_len += 1;
			break;
        case 10:
            qmToBigEndian((uint8_t)value.voltage, tx_data+tx_data_len);
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
					fstn = pswf_module->calcFstn(ContentSms.R_ADR,ContentSms.S_ADR,ContentSms.RN_KEY,d.day,timeVirtual.hours,timeVirtual.minutes,timeVirtual.seconds,sms_counter - 39);
				else
					fstn = pswf_module->calcFstn(ContentSms.R_ADR,ContentSms.S_ADR,ContentSms.RN_KEY,date_time[0],date_time[1],date_time[2],date_time[3],sms_counter - 39); // TODO: fix that;
				QNB_RX++;
				//qmDebugMessage(QmDebug::Dump, "sendCommandEasy() FSTN: %d", fstn);
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

	qmDebugMessage(QmDebug::Dump, "sendCommand 0x%x 0x%x 0x%x 0x%x 0x%x", tx_address, code, tx_data[2], tx_data[3], tx_data[4]);
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

		uint8_t tx_address;
		uint8_t tx_data[DspTransport::MAX_FRAME_DATA_SIZE];
		int tx_data_len = DEFAULT_PACKET_HEADER_LEN;
		qmToBigEndian((uint8_t)2, tx_data+0);
		qmToBigEndian((uint8_t)code, tx_data+1);
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
			case AudioTypeGarnityre:
				qmToBigEndian((uint8_t)value.signal_type_garn, tx_data+tx_data_len);
				tx_data_len += 1;
				break;
			default: QM_ASSERT(0);
			}
			break;
		}
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
			        	fstn = pswf_module->calcFstn(ContentSms.R_ADR,ContentSms.S_ADR,ContentSms.RN_KEY,d.day,timeVirtual.hours,timeVirtual.minutes,timeVirtual.seconds,sms_counter - 39);
			        else
			        	fstn = pswf_module->calcFstn(ContentSms.R_ADR,ContentSms.S_ADR,ContentSms.RN_KEY,date_time[0],date_time[1],date_time[2],date_time[3],sms_counter - 39); // TODO: fix that;
					QNB_RX++;
					//qmDebugMessage(QmDebug::Dump, "sendCommand() FSTN: %d", fstn);
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

		qmDebugMessage(QmDebug::Dump, "sendCommand 0x%x 0x%x 0x%x 0x%x 0x%x", tx_address, code, tx_data[2], tx_data[3], tx_data[4]);

		}
}

void DspController::sendGuc()
{
    //qmDebugMessage(QmDebug::Dump, "sendGuc()");
    uint8_t tx_data[DspTransport::MAX_FRAME_DATA_SIZE];
    std::vector<bool> data_guc;
    uint8_t tx_address = 0x7A;
    int tx_data_len    = 0;
    ContentGuc.Coord   = isGpsGuc;
    uint8_t pack[5]    = {0, 0, 0, 0, 0};

    qmToBigEndian((uint8_t)ContentGuc.indicator, tx_data + tx_data_len); ++tx_data_len;
    qmToBigEndian((uint8_t)ContentGuc.type, tx_data + tx_data_len);      ++tx_data_len;

    pack[4]  = (ContentGuc.S_ADR   & 0x1F) << 3;
    pack[4] |= (ContentGuc.R_ADR   & 0x1F) >> 2;

    pack[3]  = (ContentGuc.R_ADR   & 0x03) << 6;
    pack[3] |= (ContentGuc.NUM_com & 0x7F) >> 1;

    pack[2]  = (ContentGuc.NUM_com & 0x01) << 7;
    pack[2] |= (ContentGuc.ckk     & 0x3F) << 1;
    pack[2] |=  ContentGuc.uin             >> 7;

    pack[1]  = (ContentGuc.uin     & 0x7F) << 1;
    pack[1] |=  ContentGuc.Coord   & 0x01;

    for(int i = 4; i >= 0; --i)
    {
    	qmToBigEndian((uint8_t)pack[i], tx_data + tx_data_len);
    	++tx_data_len;
    }

    int crc32_len = ContentGuc.NUM_com; // реальное количество команд
    int real_len  = crc32_len;

    // Выбор количества передаваемых байтов с координатами или без
    if (isGpsGuc)
    {
        if (ContentGuc.NUM_com  <= 6) 									ContentGuc.NUM_com = 6;
        if ((ContentGuc.NUM_com > 6 )  && (ContentGuc.NUM_com <= 10) )  ContentGuc.NUM_com = 10;
        if ((ContentGuc.NUM_com > 10)  && (ContentGuc.NUM_com <= 26) )  ContentGuc.NUM_com = 26;
        if ((ContentGuc.NUM_com > 26)  && (ContentGuc.NUM_com <= 100))  ContentGuc.NUM_com = 100;
    }
    else
    {
        if (ContentGuc.NUM_com <= 5) 								   ContentGuc.NUM_com = 5;
        if ((ContentGuc.NUM_com > 5)  && (ContentGuc.NUM_com <= 11) )  ContentGuc.NUM_com = 11;
        if ((ContentGuc.NUM_com > 11) && (ContentGuc.NUM_com <= 25) )  ContentGuc.NUM_com = 25;
        if ((ContentGuc.NUM_com > 25) && (ContentGuc.NUM_com <= 100))  ContentGuc.NUM_com = 100;
    }

    for(int i = 0; i < ContentGuc.NUM_com; i++)
    {
        qmToBigEndian((uint8_t)ContentGuc.command[i], tx_data + tx_data_len);
        ++tx_data_len;
    }
    // обработка и получение координат, добавление в исходный массив для защиты  crc32 сумой (ДАННЫЕ + КООДИНАТЫ)
    if (isGpsGuc)
    {
       uint8_t coord[9] = {0,0,0,0,0,0,0,0,0};
       getGpsGucCoordinat(coord);
       for(int i = 0; i < 9; i++)
       {
           ContentGuc.command[ContentGuc.NUM_com+i] = coord[i];
           qmToBigEndian((uint8_t)coord[i],tx_data + tx_data_len);
           ++tx_data_len;
       }
    }
    // выбор длинны кодируемого массива
     crc32_len = (isGpsGuc) ? (ContentGuc.NUM_com + 9) : (ContentGuc.NUM_com);

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
    	 for(int i = 9;i < 9+ContentGuc.NUM_com;i++)
    	 {
    		  ContentGuc.command[i] = value[i- 9];
    	 }
         for(int i = 0; i<9;i++)
         {
             ContentGuc.command[i] = mas[i];
             if (i != 8) pack_manager->addBytetoBitsArray(ContentGuc.command[i],data_guc,8);
         }

         bool quadrant = ContentGuc.command[8] & 1;
         data_guc.push_back(quadrant);

         quadrant = ContentGuc.command[8] & (1 << 1);
         data_guc.push_back(quadrant);

         for(int i = 0; i<real_len;i++)
         {
             pack_manager->addBytetoBitsArray(ContentGuc.command[i+9],data_guc,7);
         }
     }
     // сдвиг массива для crc32-суммы
    if (isGpsGuc)
    {
        pack_manager->getArrayByteFromBit(data_guc,ContentGuc.command);
        crc32_len = data_guc.size() / 8;

//        for(int i = 0; i< crc32_len;i++){
//        	qmDebugMessage(QmDebug::Dump,"packet guc: %d", ContentGuc.command[i]);
//        }
    }
    else
    {
    	std::vector<bool> data;
    	for(int i = 0; i<ContentGuc.NUM_com;i++) pack_manager->addBytetoBitsArray(ContentGuc.command[i],data,7);
    	for(int i = 0; i<crc32_len;i++) pack_manager->getArrayByteFromBit(data,ContentGuc.command);
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
    // добавление crc32 к пакету данных
     uint32_t crc = pack_manager->CRC32(ContentGuc.command, crc32_len);
     qmToBigEndian((uint32_t)crc, tx_data + tx_data_len);
     tx_data_len += 4;

    transport->transmitFrame(tx_address, tx_data, tx_data_len);
}

void *DspController::getContentPSWF()
{
    return &ContentPSWF;
}

void DspController::sendSms(Module module)
{
	PswfModes::trFrame f =  pswf_module->sendSms();
    transport->transmitFrame(f.address, f.data, f.len);
}

void DspController::startGucTransmitting()
{
 //   qmDebugMessage(QmDebug::Dump, "startGucTransmitting");
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

}

void DspController::prevTime(uint8_t date_time[4])
{
	for(int i = 3; i>=0 ;i++){
		if (date_time[i] > 0){
			date_time[i] -=1;
			return;
		}
	}

	qmDebugMessage(QmDebug::Dump,"curr date_time d %d h %d m %d s %d ", date_time[0], date_time[1],date_time[2], date_time[3]);
}

void DspController::getZone()
{
	for (int i = 0; i < 18; i++)
	{
		if ( waveZone[i] >= 0  && waveZone[i] < 6  ) syncro_recieve[i] = 0;
		if ( waveZone[i] >= 6  && waveZone[i] < 12 ) syncro_recieve[i] = 1;
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

   // qmDebugMessage(QmDebug::Dump,"Result of erase %d:", temp);

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

        while(k >=0)
        {
            code_get += (data[89+k] & 0xFF) << (8*k);
            k--;
        }

   //     qmDebugMessage(QmDebug::Dump," Calc sms  code  crc %d %d:", code_get,code_calc);

        if (code_get != code_calc)
        {
            smsFailed(3);
            ack = 99;
            return false;
        }
        else
        {
          // 8. calculate text without CRC32 code
          pack_manager->decompressMass(crc_calcs, 89, packet, 110, 7);

          indexSmsLen = 100;
          for(int i = 0;i<100;i++)
          {
        	  if (packet[i] == 0)
        	  {
        		  indexSmsLen = i;
        		  break;
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
	//qmDebugMessage(QmDebug::Dump, "startPSWFReceiving(%d)", ack);
	QM_ASSERT(is_ready);
	stopGucIntoVoice();
	pswf_module->startPswfRx();
}

void DspController::startPSWFTransmitting(bool ack, uint8_t r_adr, uint8_t cmd,int retr)
{
    QM_ASSERT(is_ready);
    stopGucIntoVoice();
    pswf_module->startPswfTx(ack,r_adr,cmd,retr);
}

void DspController::startSMSRecieving(SmsStage stage)
{
	QM_ASSERT(is_ready);
	stopGucIntoVoice();

	pswf_module->startSmsRx();

}

void DspController::defaultSMSTransmit()
{
	for(int i = 0; i<255;i++)  ContentSms.message[i] = 0;
	//ContentSms.stage = StageNone;
}

void DspController::startSMSTransmitting(uint8_t r_adr,uint8_t* message, SmsStage stage)
{
  //  qmDebugMessage(QmDebug::Dump, "SMS tranmit (%d, %s)",r_adr, message);
    QM_ASSERT(is_ready);
    stopGucIntoVoice();

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

    uint8_t  dlt_bits_compession = (ind * 7) / 8;
    if ((ind * 7) % 8 != 0)
    	++dlt_bits_compession;

    for (int i = dlt_bits_compession; i < 259; i++)
    	ContentSms.message[i] = 0;

    ContentSms.message[87] = ContentSms.message[87] & 0x0F; //set 4 most significant bits to 0

    ContentSms.message[88] = 0;

    uint8_t ret = getSmsRetranslation();
    if (ret != 0)
    {
        ContentSms.message[87] = ContentSms.message[87] | (ret  << 4);
        ContentSms.message[88] = ContentSms.message[88] | ((ret >> 4) & 0x3);
    }

    uint32_t abc = pack_manager->CRC32(ContentSms.message,89);

    for (int i = 0;i<4;i++) ContentSms.message[89+i] = (uint8_t)((abc >> (8*i)) & 0xFF);
    for (int i = 0;i<255;i++) rs_data_clear[i] = 1;
    for (int i = 0; i<255;i++) data_sms[i] = (int)ContentSms.message[i];

    encode_rs(data_sms,&data_sms[93],&rs_255_93);
    for(int i = 0; i<255;i++)ContentSms.message[i]  = data_sms[i];


    radio_state = radiostateSms;

    sms_counter  = 0;
    smsSmallCounter = 0;

    if (virtual_mode)
    	startVirtualPpsModeTx();
    else
    	setTx();
}


void DspController::startGucIntoVoice()
{
	//startGucRecieving();
	if (is_ready)
	{
		ParameterValue comandValue;
		comandValue.guc_mode = 3;
		sendCommandEasy(RadioLineNotPswf, 0 ,comandValue);
	}
}

void DspController::stopGucIntoVoice()
{
	ParameterValue comandValue;
	comandValue.guc_mode = 0;
	sendCommandEasy(RadioLineNotPswf, 0 ,comandValue);
}

void DspController::startGucTransmitting(int r_adr, int speed_tx, std::vector<int> command, bool isGps)
{
//    qmDebugMessage(QmDebug::Dump, "startGucTransmitting(%i, %i)", r_adr, speed_tx);
    QM_ASSERT(is_ready);

    ContentGuc.indicator = 20;
    ContentGuc.type = 1;
	ContentGuc.chip_time = 3; // super versia new, last value = 2
	ContentGuc.WIDTH_SIGNAL = 0; // last value  = 1, thi is freq mode 0 - 3k1, 1 - 20k maybe it works:)
    //data_storage_fs->getAleStationAddress(ContentGuc.S_ADR);
	ContentGuc.S_ADR = stationAddress;

    ContentGuc.R_ADR = r_adr;
    if (r_adr == 0)
    	isGucWaitReceipt = false;
    else
    	isGucWaitReceipt = true;

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

    //ContentGuc.stage =  GucTx;

    //initResetState();
    ParameterValue comandValue;
    comandValue.radio_mode = RadioModeOff;
    sendCommandEasy(RxRadiopath, RxRadioMode, comandValue);
    comandValue.guc_mode = RadioModeSazhenData;
    sendCommandEasy(TxRadiopath, TxRadioMode, comandValue);
    if (freqGucValue != 0)
    comandValue.frequency =  freqGucValue;
    sendCommandEasy(RxRadiopath, RxFrequency, comandValue);
    QmThread::msleep(100);
    sendCommandEasy(TxRadiopath, TxFrequency, comandValue);
    radio_state = radiostateGucTxPrepare;
    command.clear();

    sendGuc();
}


void DspController::setFreq(int value){
    freqGucValue  = value;
}


void DspController::sendGucQuit()
{
//	qmDebugMessage(QmDebug::Dump, "sendGucQuit");

	uint8_t tx_address = 0x7A;
	uint8_t tx_data[DspTransport::MAX_FRAME_DATA_SIZE];
	int tx_data_len = 0;

	ContentGuc.indicator    = 20;
	ContentGuc.type         = 4;
	ContentGuc.chip_time    = 3; // super versia new, last value = 2
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
	pack[2] =  (ContentGuc.R_ADR & 0x1F) << 3; // 5 � � В±� � С‘� ЎвЂљ
	pack[2] |= (ContentGuc.S_ADR & 0x1F) >> 2; // 3 � � В±� � С‘� ЎвЂљ� � В°
	pack[1] |= (ContentGuc.S_ADR & 0x1F) << 6; // 2 � � В±� � С‘� ЎвЂљ� � В°
	pack[1] |= (ContentGuc.uin >> 2) & 0x3F;   // 6 � � В±� � С‘� ЎвЂљ
	pack[0] =  (ContentGuc.uin << 6) & 0xC0;   // 2 � � В±� � С‘� ЎвЂљ� � В°

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

uint8_t* DspController::getGucCoord()
{
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
    QM_ASSERT(is_ready);

   // initResetState();

    ParameterValue comandValue;
    comandValue.radio_mode = RadioModeOff;
    sendCommandEasy(TxRadiopath, TxRadioMode, comandValue);
    comandValue.guc_mode = 3;

    /* установка режима на включение */
    sendCommandEasy(RadioLineNotPswf, 0 ,comandValue);

    /* установка адреса */
    comandValue.guc_mode = stationAddress;
    sendCommandEasy(RadioLineNotPswf, 3 ,comandValue);

    /* установка  в две полосы */
    comandValue.guc_mode = 3;
    sendCommandEasy(RadioLineNotPswf, 1, comandValue);

    QmThread::msleep(100);
    //-----------------------------------

    comandValue.guc_mode = RadioModeSazhenData; // 11 mode, need for guc receiving
    sendCommandEasy(RxRadiopath, RxRadioMode, comandValue);

//	switch (emissionType)
//	{
//		case voiceemissionUSB: comandValue.radio_mode = RadioModeUSB;
//		case voiceemissionFM: comandValue.radio_mode = RadioModeFM;
//		default: comandValue.radio_mode = current_radio_mode;
//	}
//
//	sendCommandEasy(RxRadiopath,RxRadioMode,comandValue);
//
//	comandValue.radio_mode = RadioModeOff;
//	sendCommandEasy(TxRadiopath,2,comandValue);

	   //----------------------------------------

    comandValue.frequency = freqGucValue;
    sendCommandEasy(RxRadiopath, RxFrequency, comandValue);



    //ContentGuc.stage =  GucRx;
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

void DspController::setReceiverState(int state)
{
	//disableModemTransmitter();
	//current_radio_mode = RadioModeSazhenData;
	ParameterValue comandValue;
	comandValue.radio_mode = (RadioMode)state;
	sendCommandEasy(RxRadiopath, RxRadioMode, comandValue);
	//comandValue.modem_rx_state = ModemRxDetectingStart;
	//sendCommandEasy(ModemReceiver, ModemRxState, comandValue);
	//modem_rx_on = true;
    rxModeSetting();
}

void DspController::setTransmitterState(int state)
{
	//disableModemTransmitter();
	//current_radio_mode = RadioModeSazhenData;
	ParameterValue comandValue;
	comandValue.radio_mode = (RadioMode)state;
	sendCommandEasy(TxRadiopath, TxRadioMode, comandValue);
	//comandValue.modem_rx_state = ModemRxDetectingStart;
	//sendCommandEasy(ModemReceiver, ModemRxState, comandValue);
	//modem_rx_on = true;
    txModeSetting();
}

void DspController::setModemState(int state)
{
	ParameterValue comandValue;
	comandValue.modem_rx_state = (ModemState)state;
	sendCommandEasy(ModemReceiver, ModemRxState, comandValue);
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
	//QM_ASSERT(type != modempacket_packHead);
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

void DspController::goToVoice()
{
	ParameterValue comandValue;

	switch (emissionType)
	{
		case voiceemissionUSB: comandValue.radio_mode = RadioModeUSB;
		case voiceemissionFM: comandValue.radio_mode = RadioModeFM;
		default: comandValue.radio_mode = current_radio_mode;
	}

	sendCommandEasy(RxRadiopath,2,comandValue);
	comandValue.radio_mode = RadioModeOff;
	sendCommandEasy(TxRadiopath,2,comandValue);
	virtGuiCounter = 0;
	if (guc_rx_quit_timer)
		stopGucTimer();
	radio_state = radiostateCmdRxMode;//radiostateSync;
	if (virtual_mode)
	{
		RtcRxRole = false;
		RtcTxRole = false;
	}

	comandValue.frequency = current_radio_frequency;
	sendCommandEasy(RxRadiopath, RxFrequency, comandValue);
	sendCommandEasy(TxRadiopath, TxFrequency, comandValue);

	startGucIntoVoice();
}

void DspController::magic()
{
	ParameterValue comandValue;
	comandValue.guc_mode = RadioModeSazhenData; // 11 mode
	sendCommandEasy(RxRadiopath, RxRadioMode, comandValue);
}

bool DspController::getIsGucCoord()
{
    return isGpsGuc;
}

void DspController::startVirtualPpsModeTx()
{
	setPswfTx();

	boomVirtualPPS = false;
	masterVirtualPps = 0;

	ParameterValue comandValue;  //0x60 2 5 1
	comandValue.param = 1;
	sendCommandEasy(PSWFReceiver,5,comandValue);

	RtcTxRole = true;
	RtcRxRole = false;
	RtcTxCounter = 0;
	virtGuiCounter = 0;
	//radio_state = radiostatePswf;

	count_VrtualTimer = startVirtTxPhaseIndex;
}

void DspController::startVirtualPpsModeRx()
{
	setRx();

	ParameterValue comandValue;
	comandValue.param = 1;
	sendCommandEasy(PSWFReceiver,5,comandValue);
	comandValue.param = 2;
	sendCommandEasy(PSWFReceiver,4,comandValue);

	RtcRxRole = true;
	RtcTxRole = false;
	RtcFirstCatch = 0;
#ifndef PORT__PCSIMULATOR
	timeVirtual = rtc->getTime();
	d = rtc->getDate();
#endif
	antiSync = false;
	pswf_in_virt = 0;
	count_VrtualTimer = NUMS;

	virtGuiCounter = 0;
}

void DspController::sendSynchro(uint32_t freq, uint8_t cnt)
{
//	qmDebugMessage(QmDebug::Dump, "freq virtual %d", freq);
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
//	qmDebugMessage(QmDebug::Dump, "correctTime() data[7] as num %d", num);
//	qmDebugMessage(QmDebug::Dump, "correctTime() before getTime t.seconds %d", t.seconds);

//	qmDebugMessage(QmDebug::Dump, "correctTime() after getTime t.seconds %d", t.seconds);
	timeVirtual.seconds = 12 * (timeVirtual.seconds / 12) + 7;
//	qmDebugMessage(QmDebug::Dump, "correctTime() after correct t.seconds %d", t.seconds);
	count_VrtualTimer = num;
//    qmDebugMessage(QmDebug::Dump, "correctTime() COUNTER VIRTUAL %d",count_VrtualTimer);

	RtcFirstCatch = -1;
}


void DspController::wakeUpTimer()
{
#ifndef PORT__PCSIMULATOR
  if(virtual_mode) { vm1Pps();};

	if ((virtual_mode) && (RtcRxRole) && (!antiSync))
	{
		timeVirtual = rtc->getTime();

		if (IsStart(timeVirtual.seconds))
		{
			freqVirtual = getCommanderFreq(ContentPSWF.RN_KEY,timeVirtual.seconds,d.day,timeVirtual.hours,timeVirtual.minutes);
			ParameterValue param;
			param.frequency = freqVirtual;
//			qmDebugMessage(QmDebug::Dump, "freq virtual %d",freqVirtual);
			sendCommandEasy(RxRadiopath, 1, param);
		}
	}
	if ((virtual_mode) && (RtcTxRole) && (!boomVirtualPPS) && (masterVirtualPps))
	{
#ifndef PORT__PCSIMULATOR
		timeVirtual = rtc->getTime();
		d = rtc->getDate();
#endif
		ParameterValue comandValue;
		comandValue.radio_mode = RadioModeOff;
		sendCommandEasy(VirtualPps,1,comandValue);
		boomVirtualPPS = true;
	}
#endif
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
		virtGuiCounter = 0;
	}
}

void DspController::getVirtualDate(uint8_t *day, uint8_t *month, uint8_t *year)
{
#ifndef PORT__PCSIMULATOR
	d = rtc->getDate();
	*day = d.day;
	*month = d.month;
	*year = d.year;
#endif
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
    timeVirtual.seconds  = time[0];
    timeVirtual.minutes  = time[1];
    timeVirtual.hours    = time[2];
    rtc->setTime(timeVirtual);
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

void DspController::onGucWaitingQuitTimeout()
{
	if (guc_rx_quit_timer_counter)
	{
		guc_rx_quit_timer->start();
		guc_rx_quit_timer_counter--;
		qwitCounterChanged(guc_rx_quit_timer_counter);
	}
	else
	{
		recievedGucQuitForTransm(-1);
		completedStationMode(false);
	}
}

void DspController::startGucTimer()
{
	guc_rx_quit_timer_counter = 180;
	qwitCounterChanged(guc_rx_quit_timer_counter);
	guc_rx_quit_timer->start();
}

void DspController::stopGucTimer()
{
	guc_rx_quit_timer->stop();
	guc_rx_quit_timer_counter = 180;
	//qwitCounterChanged(guc_rx_quit_timer_counter);
}

void DspController::vm1Pps()
{
    int hrs(0), min(0), sec(0);

    if (virtual_mode)
    {
#ifndef PORT__PCSIMULATOR
        QmRtc::Time time;
        time = rtc->getTime();

        hrs = time.hours;
        min = time.minutes;
        sec = time.seconds;
#endif
    }
    else
    {
        Navigation::Coord_Date date = navigator->getCoordDate();

        char hr_ch[3] = {0,0,0};
        char mn_ch[3] = {0,0,0};
        char sc_ch[3] = {0,0,0};

        memcpy(hr_ch,&date.time[0],2);
        memcpy(mn_ch,&date.time[2],2);
        memcpy(sc_ch,&date.time[4],2);

        hrs = atoi(hr_ch);
        min = atoi(mn_ch);
        sec = atoi(sc_ch);

        sec += 1;
        if (sec >= 60) {
        	sec %= 60;
        	min++;
        	if (min >= 60) {
        		min %= 60;
        		hrs++;
        		if (hrs >= 24) {
        			hrs %= 24;
        			min = 0;
        			sec = 0;
        		}
        	}
        }

    }

    vm1PpsPulse(hrs, min, sec);
}

void DspController::setStationAddress(uint8_t address)
{
    //data_storage_fs->getAleStationAddress(stationAddress);
    stationAddress = address;
     ContentSms.S_ADR = stationAddress;
     ContentPSWF.S_ADR = stationAddress;
}

void DspController::clearWaveInfo()
{
	waveInfoRecieved(0.000, 0.000);
}

void DspController::transmithFrame(uint8_t address, uint8_t *data, int data_len)
{
	this->transport->transmitFrame(address,data,data_len);
}

void DspController::setAtuTXOff()
{
    ParameterValue comandValue;
    comandValue.radio_mode = RadioModeOff;
    sendCommandEasy(TxRadiopath, TxRadioMode, comandValue);
}

}

/* namespace Multiradio */

#include "qmdebug_domains_start.h"
QMDEBUG_DEFINE_DOMAIN(dspcontroller, LevelVerbose)
#include "qmdebug_domains_end.h"
