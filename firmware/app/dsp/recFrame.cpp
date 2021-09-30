/**
  ******************************************************************************
  * @file    recFrame.c
  * @author  Pankov Denis, PMR dept. software team, ONIIP, PJSC
  * @date    30.01.2018
  *
  ******************************************************************************
 */

#include "qm.h"
#include "qmdebug.h"
#include "qmendian.h"
#include "qmtimer.h"
#include "qmthread.h"
#include "qmiopin.h"
#include "../synchro/virtual_timer.h"
#include "dspcontroller.h"
#include "PswfModes.h"

namespace Multiradio
{

#define SAZEN 1
#define TROPA 0

#define DEVICE_VALUE 400000

//#if SAZEN
//
//#else
//	#if TROPA
//		#define DEVICE_VALUE 201600
//	#endif
//#endif

#define DEFAULT_PACKET_HEADER_LEN	2
//280000 - sazhen 201600 -tropa


void DspController::recStart(uint8_t address, uint8_t* data, int data_len)
{
	uint8_t indicator = qmFromBigEndian<uint8_t>(data+0);
	uint8_t code      = qmFromBigEndian<uint8_t>(data+1);

	uint8_t *value_ptr = data + 2;
	int value_len = data_len - 2;

	if (code == 2)
	{
		versionDSP = 0;
		versionDSP = data[4] << 12 | data[5] << 8 | data[6] << 4 | data[7];
		recivedVersions(versionDSP, versionPLD);
	}

	if (code == 3)
	{
		versionPLD = 0;
		versionPLD = data[4] << 8 | data[5];
		recivedVersions(versionDSP, versionPLD);
	}

	if ((indicator == 5 || indicator == 3) && (code == 2) && (value_len == 6)) // инициативный кадр о старте dsp с указанием номера прошивки
	{
		processStartup(qmFromBigEndian<uint16_t>(value_ptr+0), qmFromBigEndian<uint16_t>(value_ptr+2), qmFromBigEndian<uint16_t>(value_ptr+4));
	}
}

inline void DspController::recUndef(uint8_t address, uint8_t* data, int data_len)
{
	uint8_t indicator  = qmFromBigEndian<uint8_t>(data+0);
	uint8_t code       = qmFromBigEndian<uint8_t>(data+1);

	uint8_t *value_ptr = data + 2;
	int value_len      = data_len - 2;


	value_ptr -= 1; // превращение в нестандартный формат кадра
	value_len += 1; // превращение в нестандартный формат кадра

	if (indicator == 5)
	{
		uint8_t subdevice_code = (uint8_t)qmFromBigEndian<int8_t>(value_ptr+0);
		uint8_t error_code     = (uint8_t)qmFromBigEndian<int8_t>(value_ptr+2);

		hardwareFailed.emit(subdevice_code, error_code);
	}
}

inline void DspController::recTractCmd(uint8_t address, uint8_t* data, int data_len)
{
	ParameterValue value;
	uint8_t indicator = qmFromBigEndian<uint8_t>(data+0);
	uint8_t code      = qmFromBigEndian<uint8_t>(data+1);

	uint8_t *value_ptr = data + 2;
	int value_len = data_len - 2;

	// если принимаем rssi
	if (indicator == 5 && code == 7)
	{
		rxRssiLevel = data[2];
		reciveRssiLevel(rxRssiLevel);
	}

	// если индикатор допустим
	if ((indicator == 1) || (indicator == 3) || (indicator == 4) || (indicator == 5))
	{
		// установка частоты
		if ((code == 1) && (value_len == 4))
		{
			value.frequency = qmFromBigEndian<uint32_t>(value_ptr+0);
		}
		// установка режима работы
		if ((code == 2) && (value_len == 1))
		{
			value.radio_mode = (RadioMode)qmFromBigEndian<uint8_t>(value_ptr+0);
		}
		// установка значения ксв
		if ((indicator == 1 || indicator == 5) && code == 6)
		{
			waveInfoTimer->stop();

			ref_wave = (float)qmFromBigEndian<uint16_t>(value_ptr+0);
			fwd_wave = (float)qmFromBigEndian<uint16_t>(value_ptr+2);

			if (fwd_wave > 0 && (fwd_wave - ref_wave != 0))
			{
				swf_res = (fwd_wave + ref_wave) / (fwd_wave - ref_wave);
			}
			if (fwd_wave < ref_wave)
				swf_res = 99.0;

			power_res = (fwd_wave * fwd_wave) / DEVICE_VALUE; //280000 - sazhen 201600 -tropa
			waveInfoRecieved(swf_res, power_res);

			waveInfoTimer->start();
		}

		// ответ на команду
		Module module;
		if (address == 0x51) module = RxRadiopath;
		if (address == 0x81) module = TxRadiopath;

		processCommandResponse((indicator == 3), module, code, value);
	}
}


inline void DspController::recRxTxMod(uint8_t address, uint8_t* data, int data_len)
{
	uint8_t indicator  = qmFromBigEndian<uint8_t>(data+0);
	uint8_t code       = qmFromBigEndian<uint8_t>(data+1);

	uint8_t *value_ptr = data + 2;
	int value_len      = data_len - 2;

	if (address == 0x61) // кадры от DSP приемного модуля
	{
		if ((radio_state == radiostatePswfRxPrepare) || (radio_state == radiostateSmsRxPrepare))
		{
			ParameterValue value;
			processCommandResponse((indicator == 1), PSWFReceiver, code, value);
		}

		if ((indicator == 3) && (virtual_mode == true) && (code == 5)) masterVirtualPps = true;
	}

	if (address == 0x63)
	{
		pswf_module->DataHandler(data,indicator,data_len);
	}

	if (address == 0x73) // кадры от DSP передающего модуля
	{
		ParameterValue value;
		value.frequency = 0;

		if (radio_state == radiostateSmsTx)
		{
			if (indicator == 22)
			{
				value.frequency = qmFromBigEndian<uint32_t>(value_ptr+0);
				//qmDebugMessage(QmDebug::Dump, " frequency =  %d " ,value.frequency);
			}
			processCommandResponse((indicator == 24), PSWFTransmitter, code, value);
		}

        if (radio_state == radiostateSmsRx)
        {
			processCommandResponse((indicator == 1), PSWFTransmitter, code, value);
        }

        if (radio_state == radiostatePswfTx || radio_state == radiostatePswfRx)
        {
        	processCommandResponse((indicator == 3), PSWFTransmitter, code, value);
        }

	}
}

inline void DspController::recGucLog(uint8_t address, uint8_t* data, int data_len)
{
	uint8_t indicator  = qmFromBigEndian<uint8_t>(data+0);
	uint8_t code       = qmFromBigEndian<uint8_t>(data+1);

	uint8_t *value_ptr = data 	  + 2;
	int value_len      = data_len - 2;


	if (address == 0x7B)
	{
		if (indicator == 22)
		{
			if(code == 4)
			{
				//qmDebugMessage(QmDebug::Dump, "---- 0x7B indicator:22 GucRx");
				exitVoceMode();
				magic();
			}
			else if (code == 1) // wait recieving ack
			{
				//qmDebugMessage(QmDebug::Dump, "---- 0x7B indicator:22 GucTx");
				if (isGucWaitReceipt)
				{
					//qmDebugMessage(QmDebug::Dump, "---- isGucWaitReceipt");
					startGucRecieving();
					//ContentGuc.stage = GucTx;
					startRxQuit();
					startGucTimer();
				}
				else
				{
					//qmDebugMessage(QmDebug::Dump, "---- NOT isGucWaitReceipt");
					startRxQuit();
					completedStationMode(true);
				}
			}
		}
	}

	if (address == 0x6B)
	{
		if (indicator == 32)
		{
			//qmDebugMessage(QmDebug::Dump, "0x6B recieved frame: indicator %d", indicator);
		}
		if (indicator == 30)
		{
			ContentGuc.R_ADR = ((data[2] & 0xF8) >> 3);
			ContentGuc.uin   = ((data[4] & 0x1)  << 7) + ((data[5] & 0xFE) >> 1);

			isGpsGuc = data[5] & 0x1;
			ContentGuc.S_ADR = ((data[2] & 0x7) << 2)  + ((data[3] & 0xC0) >> 6);

			if (code == 4) /* если код квитанция */
			{
				//ContentGuc.S_ADR = ((data[2] & 0x7) << 2) + ((data[3] & 0xC0) >> 6);
				//qmDebugMessage(QmDebug::Dump, "---- 0x6B indicator:30 stage: GucTx R_ADR: %d S_ADR: %d", ContentGuc.R_ADR, ContentGuc.S_ADR);
				recievedGucQuitForTransm(ContentGuc.S_ADR);
				completedStationMode(true); // было false 03.07.18
				stopGucTimer();
			}
			else if (code == 1) /* если сообщение */
			{
				//qmDebugMessage(QmDebug::Dump, "---- 0x6B indicator:30 stage: GucRx R_ADR: %d S_ADR: %d", ContentGuc.R_ADR, ContentGuc.S_ADR);
				std::vector<uint8_t> guc;
				for(int i = 0;i<data_len;i++)
				{
					guc.push_back(data[i]);
				}
				guc_vector.push_back(guc);
				//guc_timer->start();
				recievedGucResp(ContentGuc.R_ADR, ContentGuc.S_ADR != 0);

				if (ContentGuc.S_ADR != 0)
				{
					startGucTransmitting();
					sendGucQuit();
				}

			}
		}
	}
}


inline void DspController::recModem(uint8_t address, uint8_t* data, int data_len)
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

inline void DspController::rec1ppsV(uint8_t address, uint8_t* data, int data_len)
{
	uint8_t indicator  = qmFromBigEndian<uint8_t>(data+0);
	uint8_t code       = qmFromBigEndian<uint8_t>(data+1);

	uint8_t *value_ptr = data + 2;
	int value_len      = data_len - 2;

	// get number of the catch packet ...
#ifndef PORT__PCSIMULATOR
    if (!virtual_mode)
        return;
    if ((indicator == 5) && (data[2] == 1))
	{
        //qmDebugMessage(QmDebug::Dump, "0x65 frame indicator == 5");
        if ((masterVirtualPps == 0) && (RtcTxRole)) return;
		addSeconds(&timeVirtual);

		if (RtcTxRole)
		{
			if (count_VrtualTimer <= VrtualTimerMagic)
			{
				//qmDebugMessage(QmDebug::Dump, "0x65 frame");
                //qmDebugMessage(QmDebug::Dump, "0x65 count_VrtualTimer %d",count_VrtualTimer);
                //qmDebugMessage(QmDebug::Dump, "0x65 RtcTxCounter %d",RtcTxCounter);

				if (RtcTxCounter)
					++RtcTxCounter;
				//qmDebugMessage(QmDebug::Dump, "0x65 RtcTxCounter %d",RtcTxCounter);


				if (IsStart(timeVirtual.seconds))
				{

					freqVirtual = getCommanderFreq(ContentPSWF.RN_KEY,timeVirtual.seconds,d.day,timeVirtual.hours,timeVirtual.minutes);
					//qmDebugMessage(QmDebug::Dump, "0x65 frame %d %d",t.minutes,t.seconds);
					RtcTxCounter = 1;
					++count_VrtualTimer;
					if (count_VrtualTimer > VrtualTimerMagic)
					{
                        //qmDebugMessage(QmDebug::Dump, "0x65 changeFrequency()");
						addSeconds(&timeVirtual);
						if (radio_state == radiostatePswf)
							changePswfFrequency();
						if (radio_state == radiostateSms)
							changeSmsFrequency();
					}
					//qmDebugMessage(QmDebug::Dump, "0x65 frame %d %d",t.minutes,t.seconds);
				}

				//static bool isCor = false;
				if (count_VrtualTimer)
				{
					virtGuiCounter++;
					if ( virtGuiCounter >= 2)
						virtualCounterChanged(virtGuiCounter - 2);
//						else
//							virtualCounterChanged(virtGuiCounter);
	//					if (virtGuiCounter == 120)
	//						virtGuiCounter = 0;
				}

				if (RtcTxCounter == 5)
				{
					sendSynchro(freqVirtual,count_VrtualTimer);
					//qmDebugMessage(QmDebug::Dump, "0x65 sendSynchro");
					//qmDebugMessage(QmDebug::Dump, "tm %d :" ,rtc->getTime().seconds);

				}
                //qmDebugMessage(QmDebug::Dump, "0x65 count_VrtualTimer %d",count_VrtualTimer);


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
				if (IsStart(timeVirtual.seconds))
				{
					++count_VrtualTimer;
				}

				if (antiSync)
				{
					virtGuiCounter++;
					virtualCounterChanged(virtGuiCounter);
	//					if (virtGuiCounter == 120)
	//						virtGuiCounter = 0;
				}
			}
		}
	}

#endif
}



void DspController::processReceivedFrame(uint8_t address, uint8_t* data, int data_len) {
	if (data_len < DEFAULT_PACKET_HEADER_LEN)
		return;

	if (address == 0x11)
		recStart(address, data, data_len);

	if (address == 0x31)
		recUndef(address, data, data_len);

	if (address == 0x51 || address == 0x81)
		recTractCmd(address, data, data_len);

	if (address == 0x61 || address == 0x63 || address == 0x73)
		recRxTxMod(address, data, data_len);

	if (address == 0x6B || address == 0x7B)
		recGucLog(address, data, data_len);

	if (address == 0x6F || address == 0x7F)
		recModem(address, data, data_len);

	if (address == 0x65)
		rec1ppsV(address, data, data_len);

	push_queue();
}

void DspController::VoiceStop()
{
	ParameterValue command_value;

	current_radio_operation = RadioOperationRxMode;
	command_value.radio_mode = RadioModeOff;
	sendCommandEasy(TxRadiopath, TxRadioMode, command_value);


	command_value.radio_mode = current_radio_mode;
	sendCommandEasy(RxRadiopath, RxRadioMode, command_value);
}

}
