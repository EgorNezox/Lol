/**
 ******************************************************************************
 * @file    PswfModes.cpp
 * @author  Artem Pisarenko, PMR dept. software team, ONIIP, PJSC
 * @date    25.09.2017
 *
 ******************************************************************************
 */

#include "dspcontroller.h"
#include "dsptransport.h"
#include "qmendian.h"
#include <stdio.h>
#include <vector>
#include "qmthread.h"
#include "qmdebug.h"
#include "qmtimer.h"
#include <cstring>

namespace Multiradio
{

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
					fstn = calcFstn(ContentSms.R_ADR,ContentSms.S_ADR,ContentSms.RN_KEY,d.day,t.hours,t.minutes,t.seconds,sms_counter - 39);
				else
					fstn = calcFstn(ContentSms.R_ADR,ContentSms.S_ADR,ContentSms.RN_KEY,date_time[0],date_time[1],date_time[2],date_time[3],sms_counter - 39); // TODO: fix that;
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
}

void DspController::sendCommand(Module module, int code, ParameterValue value,bool state) {
		if (pending_command->in_progress) {
			//qmDebugMessage(QmDebug::Dump, "new sendCommand(%d, %d) pushed to queue ", module, code);
			DspCommand cmd;
			cmd.module = module;
			cmd.code = code;
			cmd.value = value;
			cmd_queue->push_back(cmd);
	}else {
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
		}

}

} /* namespace Multiradio */
