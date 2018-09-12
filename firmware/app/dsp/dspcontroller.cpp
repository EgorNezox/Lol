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
#include "../synchro/virtual_timer.h"
#include "../../../system/usb_cdc.h"

#define DEFAULT_PACKET_HEADER_LEN	2
#define hw_rtc                      1
#define DefkeyValue 631

#define GUC_TIMER_ACK_WAIT_INTERVAL 180000
#define GUC_TIMER_INTERVAL_REC 30000

#define VIRTUAL_TIME 120

#define NUMS 0 // need = 0   9 for debug
#define startVirtTxPhaseIndex 0;

namespace Multiradio {

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
	//wakeup_dsp_pin = new QmIopin(platformhwDspWakeUpIopin,this);

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

	swr_timer = new QmTimer(false,this);
    swr_timer->setInterval(500);
    swr_timer->timeout.connect(sigc::mem_fun(this, &DspController::getSwr));

	initResetState();

#ifndef PORT__PCSIMULATOR
    rtc = new QmRtc(hw_rtc);
	rtc->wakeup.connect(sigc::mem_fun(this,&DspController::wakeUpTimer));
#endif

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

    pswf_first_packet_received = false;
    pswf_ack 				   = false;
    pswf_ack_tx 			   = false;

    cmd_queue  = new std::list<DspCommand>();
    counterSms = new int[8]{18,18,37,6,18,18,37,6};
    ContentSms.stage = StageNone;

    galuaInit();

    pack_manager = new PackageManager();

    for(int i = 0; i<18;i++)
    {
    	syncro_recieve.push_back(99);
    	snr.push_back(0);
    	waveZone.push_back(0);
    }

    waveZone.push_back(0); // size must be 19

    for(int i = 0;i<50;i++) guc_text[i] = '\0';

    sms_call_received    = false;
    retranslation_active = false;

    for(int i = 0;i<255;i++) 		rs_data_clear[i] = 1;
    for(uint8_t i = 0; i <= 100; i++) sms_content[i] = 0;

    ContentPSWF.RN_KEY = DefkeyValue;
    ContentSms.RN_KEY = DefkeyValue;

    waveInfoTimer = new QmTimer();

    waveInfoTimer->setInterval(1700);
    waveInfoTimer->setSingleShot(true);
    waveInfoTimer->timeout.connect(sigc::mem_fun(this, &DspController::clearWaveInfo));

#ifndef PORT__PCSIMULATOR
    usb = new QmUsb(hw_usb);
    usb->usbwakeup.connect(sigc::mem_fun(this, &DspController::wakeUpUsb));
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

bool DspController::isReady()
{
	return is_ready;
}

void DspController::startServicing()
{
	// old
	//qmDebugMessage(QmDebug::Info, "start servicing...");
	initResetState();
	reset_iopin->writeOutput(QmIopin::Level_Low);
	QmThread::msleep(10);
	transport->enable();
	reset_iopin->writeOutput(QmIopin::Level_High);
	//new (fast)

#ifndef PORT__PCSIMULATOR
	usb_start();
#endif
////	qmDebugMessage(QmDebug::Info, "start servicing...");
//	initResetState();
//	reset_iopin->writeOutput(QmIopin::Level_Low);
//	QmThread::msleep(20);
////	reset_iopin->writeOutput(QmIopin::Level_High);
////	QmThread::msleep(8000);
//	transport->enable();
//	reset_iopin->writeOutput(QmIopin::Level_High);
////	ParameterValue command;
////	command.radio_mode = RadioModeUSB;
////	sendCommandEasy(TxRadiopath, 2, command);
////	QmThread::msleep(4000);
////	dspReset();

	startup_timer->start(10000);
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

void DspController::getSwr()
{
    QM_ASSERT(is_ready);
    if (!resyncPendingCommand()) return;

    ParameterValue commandValue;
    commandValue.swf_mode = 6;
    sendCommandEasy(TxRadiopath, 6, commandValue);
}

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

void DspController::sendPswf_short()
{
    trFrame f = sendPswf();
    transport->transmitFrame(f.address, f.data, f.len);
}

void DspController::exitVoceMode()
{
	completedStationMode(true);
	ParameterValue command_value;
	command_value.frequency = current_radio_frequency;
	sendCommandEasy(RxRadiopath, RxFrequency, command_value);
	sendCommandEasy(TxRadiopath, TxFrequency, command_value);
}

bool DspController::checkForTxAnswer()
{
	if (tx_call_ask_vector.size() >= 2)
	{
		wzn_value = wzn_change(tx_call_ask_vector);
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

    ParameterValue param;
    param.frequency = ContentSms.Frequency;
    if ((SmsLogicRole == SmsRoleRx) && (sms_counter >= 38 && sms_counter < 77))
    	sendCommandEasy(PSWFReceiver, PswfRxFreqSignal, param);
    else
    sendCommandEasy(PSWFReceiver, PswfRxFrequency, param);
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

	//qmDebugMessage(QmDebug::Dump,"frequency:  %d ", fr_sh);
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
}

void DspController::processStartupTimeout()
{
//	qmDebugMessage(QmDebug::Warning, "DSP startup timeout");
	is_ready = true;
	started();
    //swr_timer.start();
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
}

void DspController::magic()
{
	ParameterValue comandValue;
	comandValue.guc_mode = RadioModeSazhenData; // 11 mode
	sendCommandEasy(RxRadiopath, RxRadioMode, comandValue);
}

void DspController::wakeUpTimer()
{
#ifndef PORT__PCSIMULATOR
  if(virtual_mode) { vm1Pps();};

	if ((virtual_mode) && (RtcRxRole) && (!antiSync))
	{
		t = rtc->getTime();

		if (IsStart(t.seconds))
		{
			freqVirtual = getCommanderFreq(ContentPSWF.RN_KEY,t.seconds,d.day,t.hours,t.minutes);
			ParameterValue param;
			param.frequency = freqVirtual;
//			qmDebugMessage(QmDebug::Dump, "freq virtual %d",freqVirtual);
			sendCommandEasy(RxRadiopath, 1, param);
		}
	}
	if ((virtual_mode) && (RtcTxRole) && (!boomVirtualPPS) && (masterVirtualPps))
	{
#ifndef PORT__PCSIMULATOR
		t = rtc->getTime();
		d = rtc->getDate();
#endif
		ParameterValue comandValue;
		comandValue.radio_mode = RadioModeOff;
		sendCommandEasy(VirtualPps,1,comandValue);
		boomVirtualPPS = true;
	}
#endif
}

void DspController::playSoundSignal(uint8_t mode, uint8_t speakerVolume, uint8_t gain, uint8_t soundNumber, uint8_t duration, uint8_t micLevel)
{
	ParameterValue value;

	if (is_ready)
	{
		value.signal_duration = duration;
		sendCommandEasy(Module::Audiopath, AudioSignalDuration, value);

		value.signal_number = soundNumber;
		sendCommandEasy(Module::Audiopath, AudioSignalNumber, value);

		value.audio_mode = (AudioMode)mode;
		sendCommandEasy(Module::Audiopath, AudioModeParameter, value);
	}
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

int DspController::prevSecond(int second)
{
	return  (second == 0) ? (59) : (second - 1);
}

void DspController::prevTime()
{
	if (virtual_mode)
	{
//		qmDebugMessage(QmDebug::Dump, "prevTime() seconds: %d", t.seconds);
		t.seconds = t.seconds - 1;
	//	qmDebugMessage(QmDebug::Dump, "prevTime() seconds: %d", t.seconds);
	}
	else
	{
//		qmDebugMessage(QmDebug::Dump, "prevTime() seconds: %d", date_time[3]);
		date_time[3]  = date_time[3] - 1;
//		qmDebugMessage(QmDebug::Dump, "prevTime() seconds: %d", date_time[3]);
	}
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

void DspController::sendBatteryVoltage(int voltage)
{
	// вольт * 10
    ParameterValue command;
    command.voltage = voltage / 100;
    sendCommandEasy(TxRadiopath, 10, command);
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

void DspController::setAdr()
{
	ParameterValue param;
    param.pswf_r_adr = stationAddress;
	sendCommand(PSWFReceiver, PswfRxRAdr, param);
}

static uint8_t out_buf[2048];

volatile int isCadr = false;
volatile int isOk   = 0;
volatile int counter = 0;
volatile int size    = 0;
volatile int start   = 0;

uint16_t DspController::parsing(uint8_t *cadr, uint16_t len )
{

     for(int i = 0; i < len; i++)
     {
    	 if (counter == 0)
    	 {
    		 if (cadr[i] == 0x10) counter = 1;
    		 start = i;
    	 }
    	 else
    	 {
    		 if (counter == 1)
    		 {
    			 size  = cadr[i];
    		 }

    		 if (counter == 2)
    		 {
    			 size += cadr[i] << 8;
    		 }

    		 if (counter == size + 3)
    		 {
    			 isCadr  = false;
    			 isOk    = false;
    			 counter = 0;
    			 size    = 0;
    			 start   = 0;

    			 if (cadr[i] == 0x11)
    			 {
    				 manageCadr(out_buf,size);
    				 //return size;

    				 qmDebugMessage(QmDebug::Info, "BLA BLA: %i ", i);
    				 continue;
    			 }

    		 }

    		 if (counter - 1 >= len) qmDebugMessage(QmDebug::Info, "ERROR: CNT %i LEN %i", counter, len);
    		 out_buf[counter - 1] = cadr[i];
    		 ++counter;

    	 }

	 }

     return 0;
}

void DspController::manageCadr(uint8_t *cadr, uint16_t len)
{
	uint8_t userid = cadr[2];

	uint16_t size = 0;

	size = cadr[0] + (cadr[1] << 8);


	int pos = 0;


	for(int i = 0; i < 4; i++)
		pos  += cadr[i + 3] << (i*8);


	if (userid == 0x8)
	{
		keyEmulate(cadr[3]);
	}
}

bool DspController::getUsbStatus()
{
	return isUsbReady;
}

void DspController::wakeUpUsb()
{
#ifndef PORT__PCSIMULATOR
	isUsbReady = true;

	//UseUsb = usb->getStatus();
	/* получаем содержимое буфера от компьютера */
	uint8_t * buff = usb->getbuffer();

	/* получаем текущую длинну сообщения */
	int len =  usb->getLen();

	qmDebugMessage(QmDebug::Info, "recieved len: %i ", usb->getLen());

	/*функция обработки кадра для приемного буфера*/
	parsing(buff,len);

	/*сбрасываем поле длинны для приема нового кадра */
	usb->resetLen();

//	if (usb->getrtc())
//	{
//		reset_iopin->writeOutput(QmIopin::Level_Low);
//		QmThread::msleep(10);
//		reset_iopin->writeOutput(QmIopin::Level_High);
//	}
//	if (usb->getLen())
//	{
//		wakeup_dsp_pin->writeOutput(QmIopin::Level_Low);
//		QmThread::msleep(10);
//		wakeup_dsp_pin->writeOutput(QmIopin::Level_High);
//	}

	//QmThread::msleep(300);
	UseUsb = getUsbStatus();
#endif
}

}
/* namespace Multiradio */

#include "qmdebug_domains_start.h"
QMDEBUG_DEFINE_DOMAIN(dspcontroller, LevelVerbose)//LevelVerbose)//LevelVerbose)
#include "qmdebug_domains_end.h"
