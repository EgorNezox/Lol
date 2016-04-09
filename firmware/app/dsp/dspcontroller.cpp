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
#include <string>


#define DEFAULT_PACKET_HEADER_LEN	2 // индикатор кадра + код параметра ("адрес" на самом деле не входит сюда, это "адрес назначения" из канального уровня)

namespace Multiradio {

struct DspCommand {
	bool in_progress;
	bool sync_next;
	DspController::Module module;
	int code;
	DspController::ParameterValue value;
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

	navigator->syncPulse.connect(sigc::mem_fun(this, &DspController::syncPulseDetected));

    command_tx30 = 0;
    command_rx30 = 0;
    cmd_queue = new std::list<DspCommand>();

    timer_tx_pswf  = new QmTimer(false,this);
    timer_tx_pswf->setInterval(1000);
    timer_tx_pswf->timeout.connect(sigc::mem_fun(this, &DspController::transmitPswf));
    timer_rx_pswf  = new QmTimer(false,this);
    timer_rx_pswf->setInterval(1000);
    timer_rx_pswf->timeout.connect(sigc::mem_fun(this, &DspController::changePswfRxFrequency));

    timer_rx_pswf = new QmTimer(false,this);
    timer_rx_pswf->setInterval(1000);

//    quit_timer = new QmTimer(true,this);
//    quit_timer->setInterval(30000);
//    quit_timer->timeout.connect(sigc::mem_fun(this,&DspController::transmitPswf));

    for (int i = 0; i < 30; ++i)
    	bufer_pswf[i] = (char*)malloc(12 * sizeof(char));

    quite = 0;
}

DspController::~DspController()
{
	delete pending_command;
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

void DspController::setPSWFParametres(int RadioPath,int LCODE, int RN_KEY, int COM_N,uint32_t FREQ)
{
    QM_ASSERT(is_ready);
    if (!resyncPendingCommand())
        return;

    ContentPSWF.indicator = 20;
    ContentPSWF.TYPE = 0;
    ContentPSWF.L_CODE = LCODE;
    ContentPSWF.COM_N = COM_N;
    ContentPSWF.Frequency = FREQ;
    ContentPSWF.RN_KEY = RN_KEY;

    if (RadioPath == 0) {
        radio_state = radiostateCmdPswfRx;
        ParameterValue param;
        param.pswf_r_adr = 1;
        sendCommand(PSWFReceiver, PswfRxRAdr, param);
        setPswfMode(0);
//        timer_rx_pswf->start();
    } else {
    	setPswfMode(1);
//    	timer_tx_pswf->start();
    }
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

//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
void DspController::setPswfMode(int radio_path)
{
    ParameterValue comandValue;
    switch (radio_path) {
    case 0: {
    	comandValue.radio_mode = RadioModeOff;
    	sendCommand(TxRadiopath, TxRadioMode, comandValue);
    	comandValue.pswf_indicator = RadioModePSWF;
    	sendCommand(RxRadiopath, RxRadioMode, comandValue);
    	radio_state = radiostateCmdPswfRx;
    	break;
    }
    case 1: {
    	comandValue.radio_mode = RadioModeOff;
    	sendCommand(RxRadiopath, RxRadioMode, comandValue);
    	comandValue.pswf_indicator = RadioModePSWF;
    	sendCommand(TxRadiopath, TxRadioMode, comandValue);
    	radio_state = radiostateCmdPswfTx;
    	break;
    }
    default: QM_ASSERT(0); break;
    }
}

void DspController::syncPulseDetected() {
	if (!is_ready)
		return;
	switch (radio_state) {
	case radiostateCmdPswfRx : {
		qmDebugMessage(QmDebug::Dump, "syncPulseDetected() radiostateCmdPswfRx");
		changePswfRxFrequency();
		break;
	}
	case radiostateCmdPswfTx : {
		qmDebugMessage(QmDebug::Dump, "syncPulseDetected() radiostateCmdPswfTx");
		transmitPswf();
		break;
	}
	default: break;
	}
}

void DspController::transmitPswf()
{
    sendPswf(PSWFTransmitter);
    if (command_tx30 == 30)
    {
    	qmDebugMessage(QmDebug::Dump, "PSWF trinsmitting finished");
        timer_tx_pswf->stop();
        command_tx30 = 0;
        setPswfMode(0);
    }
    command_tx30++;
}



void DspController::changePswfRxFrequency() {
    ParameterValue param;
    param.frequency = ContentPSWF.Frequency;
    sendCommand(RxRadiopath, RxFrequency, param);
}

void DspController::RecievedPswf()
{
	qmDebugMessage(QmDebug::Warning, "RecievedPswf() command_rx30 = %d", command_rx30);
	if (command_rx30 == 30) {
		command_rx30 = 0;
		radio_state = radiostateSync;
	}

	// поиск совпадений в массиве
	command[command_rx30] = bufer_pswf[command_rx30][9];
	char lcode  = bufer_pswf[command_rx30][10];

	if (command_rx30 - 1 >=0)
		for(int j = command_rx30 - 1; j>0;j--)
		{
			if (command[j] == command[command_rx30]){
				sucsess_pswf = true;
				ContentPSWF.COM_N = command[command_rx30];
				ContentPSWF.L_CODE = lcode; // изменяем параметры для передачи
				ContentPSWF.R_ADR = ContentPSWF.S_ADR;
				// Пусть пока свой адрес равен 1
				ContentPSWF.S_ADR = 1;
//				if (quite == 1) //TODO: это пока не реализовано
//					quit_timer->start();
			}

		}

	++command_rx30;
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
		// для ППРЧ
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
			default: break;
			}
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
	uint8_t tx_address = 0x72;
	uint8_t tx_data[DspTransport::MAX_FRAME_DATA_SIZE];
	int tx_data_len = 0;
	qmToBigEndian((uint8_t)ContentPSWF.indicator, tx_data + tx_data_len);
	++tx_data_len;
	qmToBigEndian((uint8_t)ContentPSWF.TYPE, tx_data + tx_data_len);
	++tx_data_len;
	qmToBigEndian((uint8_t)ContentPSWF.Frequency, tx_data + tx_data_len);
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
    if (sucsess_pswf == false) { //TODO: продумать
    	sucsess_pswf = true;
//    	command_timer->start(); //TODO: ?
    }
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
				break;
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
    case 0x63: {

        commandOK(); // пришла команда
        if ((indicator == 30) /*&& (value_len == 1)*/)
        {
        	QM_ASSERT(command_rx30 < 30);
        	QM_ASSERT(value_len <= 12);
            memcpy(bufer_pswf[command_rx30],data,value_len);
            // копируем значения в  bufer_command

            for(int i = 0;i<12;i++)
            {
                if (i == 1)
                {
                    for(int j = 1; i<=4; i++)
                    pswf_mas[i]  = (pswf_mas[i] << 8) + data[j];
                    i+=3;
                }
                pswf_mas[i] = (int) data[i];

            }
//            parsingData();
            RecievedPswf();
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

bool DspController::questPending()
{
   ListSheldure.push_back(*pending_command); //  добавляем неотправленную команду
   return true;
}

void DspController::ReturnPswfFromDSP()
{

}

} /* namespace Multiradio */

#include "qmdebug_domains_start.h"
QMDEBUG_DEFINE_DOMAIN(dspcontroller, LevelVerbose)
#include "qmdebug_domains_end.h"
