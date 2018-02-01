/**
 ******************************************************************************
 * @file    PswfModes.cpp
 * @author  Artem Pisarenko, PMR dept. software team, ONIIP, PJSC
 * @date    25.09.2017
 *
 ******************************************************************************
 */

#include "qmendian.h"
#include <stdio.h>
#include "dspcontroller.h"
#include <vector>
#include "qmthread.h"
#include "qmdebug.h"
#include "dsptransport.h"
#include <cstring>

namespace Multiradio
{

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

void DspController::recModem(uint8_t address, uint8_t* data, int data_len)
{
	uint8_t indicator  = qmFromBigEndian<uint8_t>(data+0);
	uint8_t code       = qmFromBigEndian<uint8_t>(data+1);

	uint8_t *value_ptr = data + 2;
	int value_len      = data_len - 2;

	if (address == 0x6F)
	{
		value_ptr -= 1;
		value_len += 1;

		switch (indicator)
		{
		case 30:
		{
			ModemPacketType type = (ModemPacketType)qmFromBigEndian<uint8_t>(value_ptr+1);
			int data_offset;
			data_offset = (type == modempacket_packHead) ? 6 : 4;

			uint8_t snr              = (uint8_t)qmFromBigEndian<int8_t>(value_ptr+2);
			uint8_t errors           = (uint8_t)qmFromBigEndian<int8_t>(value_ptr+3);
			ModemBandwidth bandwidth = (ModemBandwidth)qmFromBigEndian<uint8_t>(value_ptr+0);

			receivedModemPacket.emit(type, snr, errors, bandwidth, value_ptr + data_offset, value_len - data_offset);
			break;
		}

		case 31:
		{
			if (!(value_len >= 1))
				break;

			ModemPacketType type = (ModemPacketType)qmFromBigEndian<uint8_t>(value_ptr+1);
			failedRxModemPacket.emit(type);
			break;
		}
		case 32:
		{
			ModemPacketType type = (ModemPacketType)qmFromBigEndian<uint8_t>(value_ptr+1);
			int data_offset;
			data_offset = (type == modempacket_packHead) ? 6 : 4;

			uint8_t snr              = (uint8_t)qmFromBigEndian<int8_t>(value_ptr+2);
			uint8_t errors           = (uint8_t)qmFromBigEndian<int8_t>(value_ptr+3);
			ModemBandwidth bandwidth = (ModemBandwidth)qmFromBigEndian<uint8_t>(value_ptr+0);

			if (type == modempacket_packHead)
			{
				uint8_t param_signForm = qmFromBigEndian<uint8_t>(value_ptr+4);
				uint8_t param_packCode = qmFromBigEndian<uint8_t>(value_ptr+5);
				startedRxModemPacket_packHead.emit(snr, errors, bandwidth, param_signForm, param_packCode, value_ptr + data_offset, value_len - data_offset);
			}
			else
			{
				startedRxModemPacket.emit(type, snr, errors, bandwidth, value_ptr + data_offset, value_len - data_offset);
			}

			break;
		}
		default: break;
		}
	}
	if (address == 0x7F)
	{
		value_ptr -= 1;
		value_len += 1;

		switch (indicator)
		{
		case 22:
		{
			if (!(value_len >= 1))
				break;

			ModemPacketType type = (ModemPacketType)qmFromBigEndian<uint8_t>(value_ptr+0);
			transmittedModemPacket.emit(type);
			break;
		}
		case 23:
		{
			if (!(value_len >= 1))
				break;
			failedTxModemPacket.emit();
			break;
		}
		default: break;
		}
	}
}

} /* namespace Multiradio */
