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
#include "qmdebug.h"

namespace Multiradio
{

void DspController::startPswfTx(bool ack, uint8_t r_adr, uint8_t cmd,int retr)
{
	//qmDebugMessage(QmDebug::Dump, "startPSWFTransmitting(%d, %d, %d)", ack, r_adr, cmd);
	//QM_ASSERT(is_ready);
	pswf_retranslator 		 = retr;
	ContentPSWF.RET_end_adr = retr;
	pswf_ack_tx 		     = ack;

	ContentPSWF.indicator   = 20;
	ContentPSWF.TYPE        = 0;
	ContentPSWF.COM_N       = cmd;
	ContentPSWF.R_ADR       = r_adr;

	if (pswf_retranslator > 0)
		ContentPSWF.R_ADR += 32;
	ContentPSWF.S_ADR = stationAddress;

	CondComLogicRole = CondComTx;
	radio_state = radiostatePswf;
	SmsLogicRole = SmsRoleIdle;

	setAsk       = false;
	isPswfFull   = false;
	waitAckTimer = 0;

	if (virtual_mode) startVirtualPpsModeTx();
	else setPswfTx();
}

void DspController::startPswfRx()
{
	//qmDebugMessage(QmDebug::Dump, "startPSWFReceiving(%d)", ack);
	//QM_ASSERT(is_ready);
	//	for(int i = 0; i<30;i++) pswfDataPacket[i] = 255;
	if (virtual_mode)startVirtualPpsModeRx();
	else setPswfRx();

	command_tx30  	 = 0;
	CondComLogicRole = CondComRx;
	SmsLogicRole 	 = SmsRoleIdle;
	radio_state 	 = radiostatePswf;
	pswf_rec 		 = 0;
	pswf_in 		 = 0;

	setAsk 	   	 = false;
	isPswfFull 	 = false;
	waitAckTimer    = 0;
}

void DspController::LogicPswfRx()
{
	 setPswfRxFreq();

	   //  qmDebugMessage(QmDebug::Dump, "LogicPswfRx() waitAckTimer = %d", waitAckTimer);
		if (waitAckTimer)
		{
			waitAckTimer++;
			qwitCounterChanged(65 - waitAckTimer);
			if (waitAckTimer >= 65)
			{
				waitAckTimer = 0;
				firstPacket(100, ContentPSWF.S_ADR - 32, false); // no ack recieved
				exitVoceMode();
	            //qmDebugMessage(QmDebug::Dump, "LogicPswfRx() completedStationMode");
			}
		}
		if (isPswfFull)
		{
			if (pswf_ack)
			{
				CondComLogicRole = CondComTx;
				ContentPSWF.R_ADR = ContentPSWF.S_ADR;
				ContentPSWF.S_ADR = stationAddress;
				pswf_ack = false;
				isPswfFull = false;
				command_tx30 = 1;
				setPswfTx();
				sendPswf();
				transmitAsk(true);
				isTxAsk = true;
			}
			else
			{
				exitVoceMode();
			}
		}
}

void DspController::LogicPswfTx()
{
	++command_tx30;

	if (isTxAsk)
		qwitCounterChanged(32 - command_tx30);

	if ((command_tx30 % 3 == 0) && (!setAsk))
		TxCondCmdPackageTransmit(command_tx30);

	if (command_tx30 <= 30)
		sendPswf();

	if (command_tx30 > 31)
	{
		command_tx30 = 0;
		isTxAsk = false;

		if (pswf_ack_tx)
		{
			pswf_ack_tx = false;
			CondComLogicRole = 1;
			waitAckTimer = 1;
			setPswfRx();
			qwitCounterChanged(65 - waitAckTimer);
		}
		else
		{
			exitVoceMode();
		}
	}
}

uint32_t DspController::CalcShiftFreq(uint32_t RN_KEY, uint32_t DAY, uint32_t HRS, uint32_t MIN, uint32_t SEC)
{
    int TOT_W = 6671; //
    int SEC_MLT = value_sec[SEC]; // SEC_MLT
    int FR_SH = (RN_KEY + 230*SEC_MLT + 19*MIN + 31*HRS + 37*DAY)% TOT_W;
    //qmDebugMessage(QmDebug::Dump, "Calc freq formula %d", FR_SH);
    return FR_SH;
}

uint32_t DspController::wzn_change(std::vector<int> &vect)
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

uint32_t DspController::calcFstn(int R_ADR, int S_ADR, int RN_KEY, int DAY, int HRS, int MIN, int SEC, int QNB)
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

uint32_t DspController::check_rx_call(int* wzn)
{
	int snr_mas[5] = {0,0,0,0,0};
	int wzn_mas[5] = {0,0,0,0,0};

    int cnt_index = 0;
    for(int i = 0; i<18;i++)
    {
       if (syncro_recieve.at(i) == i)
       {
           ++cnt_index;
           wzn_mas[waveZone[i]] += 1;
           snr_mas[waveZone[i]] += snr.at(i);
           //qmDebugMessage(QmDebug::Dump, "syncro_recieve value = %d", syncro_recieve.at(i));
       }
    }
    //qmDebugMessage(QmDebug::Dump, "check rx call = %d", cnt_index);
    if (cnt_index < 2) return false;
    int index = 0;
    for(int i = 1; i < 5; i++)
    {
        if (wzn_mas[index] < wzn_mas[i]) index = i;
        else
        if(wzn_mas[index] == wzn_mas[i])
        {
           if (snr_mas[i] >= snr_mas[index]) index = i;
        }
    }

    *wzn = index;
    return true;
}

uint32_t DspController::calc_ack_code(uint8_t ack)
{
	uint8_t ACK_CODE  = 0;

	if (virtual_mode)
		ACK_CODE = (ContentSms.R_ADR  +
				ContentSms.S_ADR  +
				ack 			   +
				ContentSms.RN_KEY +
				d.day  		   +
				t.hours    	   +
				t.minutes 		   +
				t.seconds)		   % 100;
	else
		ACK_CODE = (ContentSms.R_ADR  +
				ContentSms.S_ADR      +
				ack 		           +
				ContentSms.RN_KEY     +
				date_time[0] 	       +
				date_time[1]	       +
				date_time[2] 	       +
				date_time[3])         % 100;

//	qmDebugMessage(QmDebug::Dump, "radr = %d", ContentSms.R_ADR);
//	qmDebugMessage(QmDebug::Dump, "sadr = %d", ContentSms.S_ADR);
//	qmDebugMessage(QmDebug::Dump, "ack  = %d", ack);
//	qmDebugMessage(QmDebug::Dump, "radr = %d", ContentSms.RN_KEY);
//	qmDebugMessage(QmDebug::Dump, "radr = %d", d.day,t.hours,t.minutes,t.seconds);
    return ACK_CODE;
}

void DspController::DataHandler(uint8_t* data, uint8_t indicator, int data_len)
{
	if (virtual_mode && SmsLogicRole == SmsRoleRx && !smsFind)
	{
		//qmDebugMessage(QmDebug::Dump, "LogicDspController() pswf_in_virt = %d ", pswf_in_virt);
		pswf_in_virt++;
		if (pswf_in_virt >= 90)
		{
			virtGuiCounter = 0;
			sms_counter = 0;
			smsSmallCounter = 0;
			startVirtualPpsModeRx();
		}
	}
	//qmDebugMessage(QmDebug::Dump, "pswf_in_virt");
	if (indicator == 31)
	{
		//qmDebugMessage(QmDebug::Dump, "0x63 indicator 31");
		if (sms_counter < 19)
		{
			snr.erase(snr.begin());
			snr.push_back(0);

			syncro_recieve.erase(syncro_recieve.begin());
			syncro_recieve.push_back(99);

			if (check_rx_call(&wzn_value))
			{
				sms_call_received = true;
				syncro_recieve.clear();
				for(int i = 0; i<18;i++)
				syncro_recieve.push_back(99);
		    	//qmDebugMessage(QmDebug::Dump, "sms call received");
			}
			sms_data_count = 0;
		}
	}

	else if (indicator == 30)
	{
		//qmDebugMessage(QmDebug::Dump, "0x63 indicator 30");
		if (SmsLogicRole == SmsRoleIdle)
		{
			ContentPSWF.R_ADR = data[7];
			ContentPSWF.S_ADR = data[8];
	       //qmDebugMessage(QmDebug::Dump, "R_ADR = %d, S_ADR = %d", ContentPSWF.R_ADR, ContentPSWF.S_ADR);
		}

		if (data[1] == 2)  // synchro packet
		{
			antiSync = true;
			//qmDebugMessage(QmDebug::Dump, "Sync anti turn on");
			correctTime(data[7]);
			virtGuiCounter = data[7] * 12 - 7;
			virtualCounterChanged(virtGuiCounter);
		}

	    if (SmsLogicRole != SmsRoleIdle)
	    {
	        recSms(data);
			pswf_first_packet_received = true;
		}

	}

	if (SmsLogicRole == SmsRoleIdle)
	{
		recPswf(data[9],data[10],indicator);
	}
}

void DspController::recPswf(uint8_t data,uint8_t code, uint8_t indicator)
{
 if   (virtual_mode) private_lcode = (char)navigator->Calc_LCODE(ContentPSWF.R_ADR,
		 ContentPSWF.S_ADR,
		 data,
		 ContentPSWF.RN_KEY,
		 d.day,
		 t.hours,
		 t.minutes,
		 prevSecond(t.seconds));
 if   (!virtual_mode) private_lcode = (char)navigator->Calc_LCODE(ContentPSWF.R_ADR,
		 ContentPSWF.S_ADR,
		 data,
		 ContentPSWF.RN_KEY,
		 date_time[0],
		 date_time[1],
		 date_time[2],
		 prevSecond(date_time[3]));

 //qmDebugMessage(QmDebug::Dump, "recPswf() r_adr = %d,s_adr = %d", ContentPSWF.R_ADR,ContentPSWF.S_ADR);
 //qmDebugMessage(QmDebug::Dump, "recPswf() private_lcode = %d, lcode = %d", private_lcode,code);
 //qmDebugMessage(QmDebug::Dump, "recPswf() pswf_in = %d, pswf_rec = %d, pswf_in_virt = %d ",pswf_in, pswf_rec, pswf_in_virt);
 if (virtual_mode && indicator == 31)
 {
	 pswf_in_virt++;
	 uint8_t counter = 37;
	 if (virtual_mode)  counter += 65;
	 if (pswf_in_virt > counter && !pswf_rec)
	 {
		 startVirtualPpsModeRx();
	 }
 }

 if (code == private_lcode)
 {
	 //lcode can be overflow (==0) TODO fix
	 if (indicator == 30)
	 {
		 firstTrueCommand = ContentPSWF.COM_N;
		 ++pswf_rec;
		 if (pswf_rec == 2)
		 {
			 ContentPSWF.COM_N = data;
			 firstPacket(ContentPSWF.COM_N, ContentPSWF.S_ADR , true);
			 waitAckTimer = 0;
		 }

		 if (pswf_rec >= 2)
		 {
			 qwitCounterChanged(60 - pswf_in);
		 }
	 }
 }
 if (pswf_in < 30)
 {
	 if (pswf_rec)
	 {
		 pswf_in++;
		 if (pswf_in == 1) startCondReceiving();
	 }
 }
 else
 {
	 if (pswf_rec >= 1)
	 {
		 if (pswf_rec == 1)
		 {
			 firstPacket(firstTrueCommand, ContentPSWF.S_ADR - 32, false); // false - not reliable data
			 waitAckTimer = 0;
		 }

		 if (ContentPSWF.R_ADR > 32)
		 {
			 pswf_ack = true; // need transmit ask
			 setAsk   = true; // doing transmit ask
			 if (pswf_rec >= 2)  isPswfFull = true;
		 }
	 }
	 if (!pswf_ack) exitVoceMode();

	 pswf_rec = 0;
	 pswf_in = 0;
}
}

DspController::trFrame DspController::sendPswf()
{
	int time[4] = {0,0,0,0};

	if (virtual_mode)
	{
		time[0] = d.day;
		time[1] = t.hours;
		time[2] = t.minutes;
		time[3] = t.seconds;
		//qmDebugMessage(QmDebug::Dump, "time now: %d %d %d %d" ,d.day,t.hours,t.minutes,t.seconds);
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
	uint8_t tx_data[249];
	int tx_data_len = 0;

	qmToBigEndian((uint8_t)ContentPSWF.indicator,  tx_data + tx_data_len); ++tx_data_len;
	qmToBigEndian((uint8_t)ContentPSWF.TYPE,       tx_data + tx_data_len); ++tx_data_len;

	qmToBigEndian((uint32_t)ContentPSWF.Frequency, tx_data + tx_data_len); tx_data_len += 4;
	qmToBigEndian((uint8_t)ContentPSWF.SNR,        tx_data + tx_data_len); ++tx_data_len;

	qmToBigEndian((uint8_t)(pswf_retranslator ? ContentPSWF.RET_end_adr : ContentPSWF.R_ADR), tx_data+tx_data_len);   ++tx_data_len;
	qmToBigEndian((uint8_t)(pswf_retranslator ? ContentPSWF.R_ADR       : ContentPSWF.S_ADR), tx_data + tx_data_len); ++tx_data_len;

	qmToBigEndian((uint8_t)ContentPSWF.COM_N,  tx_data + tx_data_len); ++tx_data_len;
	qmToBigEndian((uint8_t)ContentPSWF.L_CODE, tx_data + tx_data_len); ++tx_data_len;

	frame.address = tx_address;
	frame.data 	  = tx_data;
	frame.len     = tx_data_len;
	return frame;
//	transport->transmitFrame(tx_address, tx_data, tx_data_len);
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

void *DspController::getContentPSWF()
{
    return &ContentPSWF;
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
	startPswfRx();
}

void DspController::startPSWFTransmitting(bool ack, uint8_t r_adr, uint8_t cmd,int retr)
{
    QM_ASSERT(is_ready);
    startPswfTx(ack,r_adr,cmd,retr);
}

} /* namespace Multiradio */
