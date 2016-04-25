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
#include <string.h>
#include <stdlib.h>

#define PSWF_SELF_ADR	1

#define DEFAULT_PACKET_HEADER_LEN	2 // индикатор кадра + код параметра ("адрес" на самом деле не входит сюда, это "адрес назначения" из канального уровня)

namespace Multiradio {

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
	command_timer->setInterval(100);
	command_timer->timeout.connect(sigc::mem_fun(this, &DspController::processCommandTimeout));
	reset_iopin = new QmIopin(reset_iopin_resource, this);
	// max_tx_queue_size: 1 команда радиотракта + 1 запас
	transport = new DspTransport(uart_resource, 2, this);
	transport->receivedFrame.connect(sigc::mem_fun(this, &DspController::processReceivedFrame));
	initResetState();

    #if defined(PORT__TARGET_DEVICE_REV1)
	this->navigator = navigator;
	navigator->syncPulse.connect(sigc::mem_fun(this, &DspController::syncPulseDetected));
    #endif

    cmd_queue = new std::list<DspCommand>();

    fwd_wave = 0;
    ref_wave = 0;

    command_tx30 = 0;
    command_rx30 = 0;

    pswfRxStateSync = 0;
    pswfTxStateSync = 0;

    smsRxStateSync = 0;
    smsTxStateSync = 0;

    success_pswf = 30;
    pswf_first_packet_received = false;
    pswf_ack = false;
    private_lcode = 0;

    counterSms = new int[8]{18,18,37,6,18,18,37,6};

    ContentSms.stage = -1;
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
	if (!is_ready)
		return;
	switch (radio_state) {
	case radiostatePswfRx : {
		qmDebugMessage(QmDebug::Dump, "syncPulseDetected() radiostatePswfRx");
		changePswfRxFrequency();
		break;
	}
	case radiostatePswfTx : {
		qmDebugMessage(QmDebug::Dump, "syncPulseDetected() radiostatePswfTx");
		transmitPswf();
		break;
	}
    case radiostateSmsRx:{
        qmDebugMessage(QmDebug::Dump, "syncPulseDetected() SmsRx");
        recSms();
        break;
    }
    case radiostateSmsTx:{
        qmDebugMessage(QmDebug::Dump, "syncPulseDetected() SmsTx");
        transmitSMS();
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

    int day = atoi(day_ch); // TODO:
    int hrs = atoi(hr_ch);
    int min = atoi(mn_ch);
    int sec = atoi(sec_ch);

    date_time[0] = day;
    date_time[1] = hrs;
    date_time[2] = min;
    date_time[3] = sec;
}

void DspController::transmitSMS()
{
    getDataTime();

    ContentSms.L_CODE = navigator->Calc_LCODE(
                0 /*ContentPSWF.R_ADR*/, 0 /*ContentPSWF.S_ADR*/,
                ContentSms.COM_N, ContentSms.RN_KEY,
                date_time[0], date_time[1], date_time[2], date_time[3]);


    if (ContentSms.stage == StageTx_info)
    {
        if (counterSms[StageTx_info] == 0)
        {
            counterSms[StageTx_info] = 18;
            qmDebugMessage(QmDebug::Dump, "Sms info finished");
            radio_state = radiostateSmsRx;
            ContentSms.stage = StageTx_rec;
        }
        else
        {
            counterSms[StageTx_info] = counterSms[StageTx_info] - 1;
        }
    }

    if (ContentSms.stage == StageRx_trans)
    {
        if (counterSms[StageRx_trans] == 0)
        {
            counterSms[StageRx_trans] = 18;
            qmDebugMessage(QmDebug::Dump, "Sms rx transmit finished");
            radio_state = radiostateSmsRx;
            ContentSms.stage = StageRx_data;
        }
        else
        {
            counterSms[StageRx_trans] = counterSms[StageRx_trans] - 1;
        }
    }

    sendSms(PSWFTransmitter);

}

void DspController::transmitPswf()
{
    getDataTime();
    ContentPSWF.L_CODE = navigator->Calc_LCODE(
    		0 /*ContentPSWF.R_ADR*/, 0 /*ContentPSWF.S_ADR*/,
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
    setFrequencyPswf();

    ParameterValue param;
    param.frequency = ContentPSWF.Frequency;
    sendCommand(PSWFReceiver, PswfRxFrequency, param);
}

void DspController::RecievedPswf()
{
	pswf_first_packet_received = true;
    qmDebugMessage(QmDebug::Dump, "RecievedPswf() command_rx30 = %d", command_rx30);
    if (command_rx30 == 30) {
        command_rx30 = 0;
        recievedPswfBuffer.erase(recievedPswfBuffer.begin());
    }

    private_lcode = navigator->Calc_LCODE(0,0,recievedPswfBuffer.at(command_rx30).at(0),1,
            date_time[0],date_time[1], date_time[2],prevSecond(date_time[3]));

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

int DspController::setFrequencyPswf()
{
    int frequency = 0;

	int sum = 0;
	int fr_sh = CalcShiftFreq(0,date_time[3],date_time[0],date_time[1],date_time[2]);

	QM_ASSERT(fr_sh >= 0 && fr_sh <= 6670);

	fr_sh = fr_sh * 1000; // Гц

	bool find_fr = false;
	int i = 0;

	while(find_fr == false)
	{
		sum += (frequence_bandwidth[i+1] - frequence_bandwidth[i]);
		if (fr_sh < sum)
		{
			fr_sh = fr_sh - (sum - (frequence_bandwidth[i+1] - frequence_bandwidth[i]));
            frequency = (frequence_bandwidth[i] + fr_sh);
			find_fr = true;
		}

		i++;
	}

    return frequency;
}


int DspController::CalcShiftFreq(int RN_KEY, int SEC, int DAY, int HRS, int MIN)
{
    int TOT_W = 6670; // ширина разрешенных участков

    int SEC_MLT = value_sec[SEC]; // SEC_MLT выбираем в массиве

    // RN_KEY - ключ подсети, аналогия с ip-сетью, поставим 0 по умолчанию

    int FR_SH = (RN_KEY + 230*SEC_MLT + 19*MIN + 31*HRS + 37*DAY)% TOT_W;
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
			case PswfRxFrequency: {
				qmToBigEndian(value.frequency, tx_data+tx_data_len);
				tx_data_len += 4;
				break;
			}
			default: break;
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

    ContentPSWF.Frequency = setFrequencyPswf();

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
        if (pswfTxStateSync == 2) {
            radio_state = radiostateSmsTx;
            qmDebugMessage(QmDebug::Dump, "processReceivedFrame() radio_state = radiostateSmsTx");
        }
    }

    //-------------------------------------------------------------------


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
		if (radio_state == radiostatePswfRxPrepare) {
			qmDebugMessage(QmDebug::Dump, "processReceivedFrame() 0x61 received");
			ParameterValue value;
			processCommandResponse((indicator == 1), PSWFReceiver, code, value);
		}
		break;
	}
    case 0x63: {
    	qmDebugMessage(QmDebug::Dump, "0x63 received");
        if (indicator == 30)
        {
            if (ContentSms.stage > -1)
            {
                std::vector<char> sms_data;
                sms_data.push_back(data[8]);
                sms_data.push_back(data[9]);
                recievedSmsBuffer.push_back(sms_data);
                recSms();
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
        ParameterValue value;
        value.frequency = 0;
        processCommandResponse((indicator == 3), PSWFTransmitter, code, value);
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
void DspController::parsingData()
{
}

void *DspController::getContentPSWF()
{
    return &ContentPSWF;
}

void DspController::sendSms(Module module)
{
    ContentSms.Frequency = setFrequencyPswf();
    ContentSms.indicator = 20;
    ContentSms.SNR =  7;

    if (ContentSms.stage < 1)
        ContentSms.TYPE = 0;
    else
        ContentSms.TYPE = 1;

    uint8_t tx_address = 0x72;
    uint8_t tx_data[DspTransport::MAX_FRAME_DATA_SIZE];
    int tx_data_len = 0;

    qmToBigEndian((uint8_t)ContentSms.indicator, tx_data + tx_data_len);
    ++tx_data_len;
    qmToBigEndian((uint8_t)ContentSms.TYPE, tx_data + tx_data_len);
    ++tx_data_len;
    qmToBigEndian((uint32_t)ContentSms.Frequency, tx_data + tx_data_len);
    tx_data_len += 4;
    // tx
    if (ContentSms.stage == StageTx_info)
    {
        static int counter = 0;
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
    if (ContentSms.stage == StageTx_trans)
    {
        int FST_N = 0;
        static int cntChvc = 7;
        if (cntChvc == 259) cntChvc = 0;
        qmToBigEndian((uint8_t)ContentPSWF.SNR, tx_data+tx_data_len);
        ++tx_data_len;
        qmToBigEndian((uint8_t)FST_N, tx_data+tx_data_len);
        ++tx_data_len;
        for(int i = 0;i<=cntChvc;i++)
        {
            qmToBigEndian((uint8_t)ContentSms.message[i], tx_data+tx_data_len);
            ++tx_data_len;
        }
       cntChvc += 7;
    }
    //rx
    if (ContentSms.stage == StageRx_trans)
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


    QM_ASSERT(pending_command->in_progress == false);
    pending_command->in_progress = true;
    pending_command->sync_next = true;
    pending_command->module = module;
    transport->transmitFrame(tx_address, tx_data, tx_data_len);
}

void DspController::recSms()
{
    qmDebugMessage(QmDebug::Dump, "RecievedSMS");

    if (ContentSms.stage == StageTx_rec)
    {
        if (counterSms[StageTx_rec] == 0)
        {
            ContentSms.stage = StageTx_trans;
            radio_state =  radiostateSmsTx;
            counterSms[StageTx_rec] = 18;
        }
        else
        {
            counterSms[StageTx_rec] = counterSms[StageTx_rec] - 1;
        }
    }

    if (ContentSms.stage == StageTx_quit)
    {
        if (counterSms[StageTx_quit] == 0)
        {
            ContentSms.stage = -1;
            radio_state = radiostateSync;
            counterSms[StageTx_quit] = 6;
        }
        else
        {
            counterSms[StageTx_quit] = counterSms[StageTx_quit] - 1;
        }
    }

    if (ContentSms.stage == StageRx_info)
    {
        if (counterSms[StageRx_info] == 0)
        {
            ContentSms.stage = StageRx_trans;
            radio_state = radiostateSmsTx;
            counterSms[StageRx_info] = 18;
        }

        else
        {
           counterSms[StageRx_info] = counterSms[StageRx_info] - 1;
        }
    }

}

void DspController::startPSWFReceiving(bool ack) {
	qmDebugMessage(QmDebug::Dump, "startPSWFReceiving(%d)", ack);
	QM_ASSERT(is_ready);
	if (!resyncPendingCommand())
		return;

	pswf_ack = ack;
	getDataTime();
    ContentPSWF.Frequency = setFrequencyPswf();

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
    ContentPSWF.Frequency = setFrequencyPswf();

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

void DspController::startSMSRecieving()
{
    qmDebugMessage(QmDebug::Dump, "startSmsReceiving");
    QM_ASSERT(is_ready);
    if (!resyncPendingCommand())
        return;

    getDataTime();
    ContentSms.Frequency = setFrequencyPswf();

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
    ContentSms.stage =  StageRx_info;
}

void DspController::startSMSTransmitting(uint8_t r_adr,uint8_t* message)
{
    qmDebugMessage(QmDebug::Dump, "SMS tranmit (%d, %d)",r_adr, message);
    QM_ASSERT(is_ready);
    if (!resyncPendingCommand())
        return;

    getDataTime();
    ContentSms.Frequency = setFrequencyPswf();

    ContentSms.indicator = 20;
    ContentSms.TYPE = 0;
    strcpy((char*)ContentSms.message,(const char*)message);
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
    ContentSms.stage = StageTx_info;

}

} /* namespace Multiradio */

#include "qmdebug_domains_start.h"
QMDEBUG_DEFINE_DOMAIN(dspcontroller, LevelVerbose)
#include "qmdebug_domains_end.h"
