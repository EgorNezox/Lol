/**
 ******************************************************************************
 * @file    PswfModes.cpp
 * @author  Artem Pisarenko, PMR dept. software team, ONIIP, PJSC
 * @date    25.09.2017
 *
 ******************************************************************************
 */

#include "PswfModes.h"
#include "qmendian.h"

namespace Multiradio
{

PswfModes::PswfModes(DspController *control)
{
	this->control = control;
}

PswfModes::~PswfModes()
{

}

void PswfModes::startPswfTx(bool ack, uint8_t r_adr, uint8_t cmd,int retr)
{
	//qmDebugMessage(QmDebug::Dump, "startPSWFTransmitting(%d, %d, %d)", ack, r_adr, cmd);
	//QM_ASSERT(control->is_ready);
	control->pswf_retranslator 		 = retr;
	control->ContentPSWF.RET_end_adr = retr;
	control->pswf_ack_tx 		     = ack;

	control->ContentPSWF.indicator   = 20;
	control->ContentPSWF.TYPE        = 0;
	control->ContentPSWF.COM_N       = cmd;
	control->ContentPSWF.R_ADR       = r_adr;

	if (control->pswf_retranslator > 0)
		control->ContentPSWF.R_ADR += 32;
	control->ContentPSWF.S_ADR = control->stationAddress;

	control->CondComLogicRole = control->CondComTx;
	control->radio_state = control->radiostatePswf;
	control->SmsLogicRole = control->SmsRoleIdle;

	control->setAsk       = false;
	control->isPswfFull   = false;
	control->waitAckTimer = 0;

	if (control->virtual_mode) control->startVirtualPpsModeTx();
	else control->setPswfTx();
}

void PswfModes::startSmsRx()
{
	//qmDebugMessage(QmDebug::Dump, "startSmsReceiving");
	//QM_ASSERT(is_ready);
	for(int i = 0; i<255; i++) control->rs_data_clear[i] = 1;

	control->tx_call_ask_vector.erase(
	control->tx_call_ask_vector.begin(),
	control->tx_call_ask_vector.end());

	control->quit_vector.erase(
	control->quit_vector.begin(),
	control->quit_vector.end());

	control->radio_state     = control->radiostateSms;
	control->sms_counter     = 0;
	control->smsSmallCounter = 0;

	control->syncro_recieve.clear();
	control->snr.clear();
	control->waveZone.clear();

	for(int i = 0; i<18;i++)
	{
		control->syncro_recieve.push_back(99);
		control->snr.push_back(0);
		control->waveZone.push_back(0);
	}
	control->waveZone.push_back(0); // size must be 19

	for(uint8_t i = 0; i <= 100; i++)
		control->sms_content[i] = 0;

	if (control->virtual_mode) control->startVirtualPpsModeRx();
	else control->setRx();

}

void PswfModes::startPswfRx()
{
	//qmDebugMessage(QmDebug::Dump, "startPSWFReceiving(%d)", ack);
	//QM_ASSERT(is_ready);
	//	for(int i = 0; i<30;i++) pswfDataPacket[i] = 255;
	if (control->virtual_mode)control->startVirtualPpsModeRx();
	else control->setPswfRx();

	control->command_tx30  	 = 0;
	control->CondComLogicRole = control->CondComRx;
	control->SmsLogicRole 	 = control->SmsRoleIdle;
	control->radio_state 	 = control->radiostatePswf;
	control->pswf_rec 		 = 0;
	control->pswf_in 		 = 0;

	control->setAsk 	   	 = false;
	control->isPswfFull 	 = false;
	control->waitAckTimer    = 0;
}


void PswfModes::LogicSmsRx()
{
	 //  qmDebugMessage(QmDebug::Dump, "RxSmsWork()");
		if (control->radio_state == control->radiostateSync) return;

		if (control->sms_counter < 20)
		{
		//	 qmDebugMessage(QmDebug::Dump, "_____RxSmsWork() smsSmallCounter = %d", smsSmallCounter);
		     if (control->syncro_recieve.size() % 2 == 0 )
		    	 control->smsCounterChanged(control->smsSmallCounter);
		}

		if (control->smsFind)
		{
			++control->sms_counter;

			if (control->sms_counter ==  20)
			{
				control->setTx();
				sendSms();
			}

	        if (control->sms_counter > 20 && control->sms_counter < 38)
	        	control->sendSms(control->PSWFTransmitter);

	        if (control->sms_counter == 39)
			{
	        	control->setRx();
	        	control->setrRxFreq();
			}

			if (control->sms_counter > 39 && control->sms_counter < 76)
			{
				control->setrRxFreq();
			}
			if (control->sms_counter == 77)
			{
				control->setTx();
				control->quest = control->generateSmsReceived();
				sendSms();
			}
			if (control->sms_counter > 77 && control->sms_counter < 83)
			{
				sendSms();
			}

	        if (control->sms_counter == 84)
			{
	        	if (control->quest)
	        	{
	        		control->smsPacketMessage(control->indexSmsLen); control->quest = false;
	        	}

	        	control->sms_counter = 0;
	        	control->smsSmallCounter = 0;
	        	control->setRx();
	        	control->smsFind = false;
	            uint8_t ret = control->getSmsRetranslation();
	            if (ret == 0)
	            	control->resetSmsState();
			}
		}
		else
		{
			if (control->sms_call_received)
			{
				control->smsFind  = true;
				control->sms_call_received = false;
				control->sms_counter = 19;
				control->pswf_in_virt = 0;
			}
			else
			{
				control->setrRxFreq();
			}
		}
}

void PswfModes::LogicSmsTx()
{
	//   qmDebugMessage(QmDebug::Dump, "TxSmsWork()");
		if (control->radio_state == control->radiostateSync) return;

	    ++control->sms_counter;

	    if (control->sms_counter < 19)
	    {
	    	sendSms();
	    }

	    if (control->sms_counter == 20)
	    {
	    	control->setRx();
	    	control->setrRxFreq();
	    }

	    if (control->sms_counter > 20 && control->sms_counter < 38)
	    {
	    	control->setrRxFreq();
	    }

	    if (control->sms_counter == 39)
	    {
	    	if (control->checkForTxAnswer())
	    	{
	    		control->setTx();
	    		sendSms();
	    	}
	    	else
	    	{
	    		control->resetSmsState();
	    		control->smsFailed(0);
	    	}
	    }

	    if (control->sms_counter > 39 && control->sms_counter < 76)
	    {
	    	sendSms();
	    }

	    if (control->sms_counter == 77)
	    {
	    	control->setRx();
	    	control->setrRxFreq();
	    }

	    if (control->sms_counter > 77 && control->sms_counter < 83)
	    {
	    	control->setrRxFreq();
	    }

	    if (control->sms_counter == 84)
	    {
	    	if (control->ok_quit >= 1)
	    		control->smsFailed(-1); //correct recieved
	    	else
	    	{
	    		if (control->smsError >= 1)
	    			control->smsFailed(2); // negative ack recieved
	    		else
	    			control->smsFailed(1); // not ack recieved
	    	}
	    	control->resetSmsState();
	    }
}

void PswfModes::LogicPswfRx()
{
	 control->setPswfRxFreq();

	   //  qmDebugMessage(QmDebug::Dump, "LogicPswfRx() waitAckTimer = %d", waitAckTimer);
		if (control->waitAckTimer)
		{
			control->waitAckTimer++;
			control->qwitCounterChanged(65 - control->waitAckTimer);
			if (control->waitAckTimer >= 65)
			{
				control->waitAckTimer = 0;
				control->firstPacket(100, control->ContentPSWF.S_ADR - 32, false); // no ack recieved
				control->exitVoceMode();
	            //qmDebugMessage(QmDebug::Dump, "LogicPswfRx() completedStationMode");
			}
		}
		if (control->isPswfFull)
		{
			if (control->pswf_ack)
			{
				control->CondComLogicRole = control->CondComTx;
				control->ContentPSWF.R_ADR = control->ContentPSWF.S_ADR;
				control->ContentPSWF.S_ADR = control->stationAddress;
				control->pswf_ack = false;
				control->isPswfFull = false;
				control->command_tx30 = 1;
				control->setPswfTx();
				control->sendPswf();
				control->transmitAsk(true);
				control->isTxAsk = true;
			}
			else
			{
				control->exitVoceMode();
			}
		}
}

void PswfModes::LogicPswfTx()
{
	++control->command_tx30;

	if (control->isTxAsk)
		control->qwitCounterChanged(32 - control->command_tx30);

	if ((control->command_tx30 % 3 == 0) && (!control->setAsk))
		control->TxCondCmdPackageTransmit(control->command_tx30);

	if (control->command_tx30 <= 30)
		control->sendPswf();

	if (control->command_tx30 > 31)
	{
		control->command_tx30 = 0;
		control->isTxAsk = false;

		if (control->pswf_ack_tx)
		{
			control->pswf_ack_tx = false;
			control->CondComLogicRole = 1;
			control->waitAckTimer = 1;
			control->setPswfRx();
			control->qwitCounterChanged(65 - control->waitAckTimer);
		}
		else
		{
			control->exitVoceMode();
		}
	}
}

uint32_t PswfModes::CalcShiftFreq(uint32_t RN_KEY, uint32_t DAY, uint32_t HRS, uint32_t MIN, uint32_t SEC)
{
    int TOT_W = 6671; //
    int SEC_MLT = value_sec[SEC]; // SEC_MLT
    int FR_SH = (RN_KEY + 230*SEC_MLT + 19*MIN + 31*HRS + 37*DAY)% TOT_W;
    //qmDebugMessage(QmDebug::Dump, "Calc freq formula %d", FR_SH);
    return FR_SH;
}

uint32_t PswfModes::wzn_change(std::vector<int> &vect)
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

uint32_t PswfModes::calcFstn(int R_ADR, int S_ADR, int RN_KEY, int DAY, int HRS, int MIN, int SEC, int QNB)
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

uint32_t PswfModes::check_rx_call(int* wzn)
{
	int snr_mas[5] = {0,0,0,0,0};
	int wzn_mas[5] = {0,0,0,0,0};

    int cnt_index = 0;
    for(int i = 0; i<18;i++)
    {
       if (control->syncro_recieve.at(i) == i)
       {
           ++cnt_index;
           wzn_mas[control->waveZone[i]] += 1;
           snr_mas[control->waveZone[i]] += control->snr.at(i);
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


uint32_t PswfModes::CalcSmsTransmitFreq(uint32_t RN_KEY, uint32_t DAY, uint32_t HRS, uint32_t MIN, uint32_t SEC)
{
	int wzn = 0;
	int FR_SH = 0;
	int TOT_W = 6671;
	int wz_base = 0;

	int SEC_MLT = value_sec[SEC];

	// Tx data, Tx_quit
	if (control->sms_counter > 38 && control->sms_counter < 84)
	{
		wzn = control->wzn_value;
		if (wzn > 0) wz_base = 6*wzn;
		else wzn  = 0;
		int wz_shift = SEC % 6;
		SEC_MLT = wz_shift + wz_base;
		//qmDebugMessage(QmDebug::Dump, "wzn_base %d" ,wz_base);
		//qmDebugMessage(QmDebug::Dump, "wzn_shift %d" ,wz_shift);

		if (control->sms_counter < 77)
		{
			FR_SH = (RN_KEY + 3*SEC + 230*SEC_MLT + 17*MIN + 29*HRS + 43*DAY)% TOT_W;
			//qmDebugMessage(QmDebug::Dump, "Calc freq sms  formula %d", FR_SH);
		}

		if (control->sms_counter > 77 && control->sms_counter < 83)
		{
			FR_SH = (RN_KEY + 5*SEC + 230*SEC_MLT + 17*MIN + 29*HRS + 43*DAY)% TOT_W;
			//qmDebugMessage(QmDebug::Dump, "Calc freq sms quit formula %d", FR_SH);
		}
	}

	if (control->sms_counter < 38)
	{
		FR_SH = (RN_KEY + 73 + 230*SEC_MLT + 17*MIN + 29*HRS + 43*DAY)% TOT_W;
		//qmDebugMessage(QmDebug::Dump, "Calc freq sms tx formula %d", FR_SH);
	}

	if ((control->sms_counter < 19) && (control->SmsLogicRole == control->SmsRoleRx))
	{
		control->waveZone.erase(control->waveZone.begin());
		control->waveZone.push_back(SEC_MLT / 6);
	}

	//qmDebugMessage(QmDebug::Dump, ">>> CalcSmsTransmitFreq() wzn_value %d" ,wzn_value);
	return FR_SH;
}

uint32_t PswfModes::calc_ack_code(uint8_t ack)
{
	uint8_t ACK_CODE  = 0;

	if (control->virtual_mode)
		ACK_CODE = (control->ContentSms.R_ADR  +
				control->ContentSms.S_ADR  +
				control->ack 			   +
				control->ContentSms.RN_KEY +
				control->d.day  		   +
				control->t.hours    	   +
				control->t.minutes 		   +
				control->t.seconds)		   % 100;
	else
		ACK_CODE = (control->ContentSms.R_ADR  +
				control->ContentSms.S_ADR      +
				control->ack 		           +
				control->ContentSms.RN_KEY     +
				control->date_time[0] 	       +
				control->date_time[1]	       +
				control->date_time[2] 	       +
				control->date_time[3])         % 100;

//	qmDebugMessage(QmDebug::Dump, "radr = %d", ContentSms.R_ADR);
//	qmDebugMessage(QmDebug::Dump, "sadr = %d", ContentSms.S_ADR);
//	qmDebugMessage(QmDebug::Dump, "ack  = %d", ack);
//	qmDebugMessage(QmDebug::Dump, "radr = %d", ContentSms.RN_KEY);
//	qmDebugMessage(QmDebug::Dump, "radr = %d", d.day,t.hours,t.minutes,t.seconds);
    return ACK_CODE;
}

void PswfModes::DataHandler(uint8_t* data, uint8_t indicator, int data_len)
{
	if (control->virtual_mode && control->SmsLogicRole == control->SmsRoleRx && !control->smsFind)
	{
		//qmDebugMessage(QmDebug::Dump, "LogicPswfModes() pswf_in_virt = %d ", pswf_in_virt);
		control->pswf_in_virt++;
		if (control->pswf_in_virt >= 90)
		{
			control->virtGuiCounter = 0;
			control->sms_counter = 0;
			control->smsSmallCounter = 0;
			control->startVirtualPpsModeRx();
		}
	}
	//qmDebugMessage(QmDebug::Dump, "pswf_in_virt");
	if (indicator == 31)
	{
		//qmDebugMessage(QmDebug::Dump, "0x63 indicator 31");
		if (control->sms_counter < 19)
		{
			control->snr.erase(control->snr.begin());
			control->snr.push_back(0);

			control->syncro_recieve.erase(control->syncro_recieve.begin());
			control->syncro_recieve.push_back(99);

			if (check_rx_call(&control->wzn_value))
			{
				control->sms_call_received = true;
				control->syncro_recieve.clear();
				for(int i = 0; i<18;i++)
				control->syncro_recieve.push_back(99);
		    	//qmDebugMessage(QmDebug::Dump, "sms call received");
			}
			control->sms_data_count = 0;
		}
	}

	else if (indicator == 30)
	{
		//qmDebugMessage(QmDebug::Dump, "0x63 indicator 30");
		if (control->SmsLogicRole == control->SmsRoleIdle)
		{
			control->ContentPSWF.R_ADR = data[7];
			control->ContentPSWF.S_ADR = data[8];
	       //qmDebugMessage(QmDebug::Dump, "R_ADR = %d, S_ADR = %d", ContentPSWF.R_ADR, ContentPSWF.S_ADR);
		}

		if (data[1] == 2)  // synchro packet
		{
			control->antiSync = true;
			//qmDebugMessage(QmDebug::Dump, "Sync anti turn on");
			control->correctTime(data[7]);
			control->virtGuiCounter = data[7] * 12 - 7;
			control->virtualCounterChanged(control->virtGuiCounter);
		}

	    if (control->SmsLogicRole != control->SmsRoleIdle)
	    {
	        recSms(data);
			control->pswf_first_packet_received = true;
		}

	}

	if (control->SmsLogicRole == control->SmsRoleIdle)
	{
		recPswf(data[9],data[10],indicator);
	}
}

void PswfModes::recPswf(uint8_t data,uint8_t code, uint8_t indicator)
{
 if   (control->virtual_mode) control->private_lcode = (char)control->navigator->Calc_LCODE(control->ContentPSWF.R_ADR,
		 control->ContentPSWF.S_ADR,
		 data,
		 control->ContentPSWF.RN_KEY,
		 control->d.day,
		 control->t.hours,
		 control->t.minutes,
		 control->prevSecond(control->t.seconds));
 if   (!control->virtual_mode) control->private_lcode = (char)control->navigator->Calc_LCODE(control->ContentPSWF.R_ADR,
		 control->ContentPSWF.S_ADR,
		 data,
		 control->ContentPSWF.RN_KEY,
		 control->date_time[0],
		 control->date_time[1],
		 control->date_time[2],
		 control->prevSecond(control->date_time[3]));

 //qmDebugMessage(QmDebug::Dump, "recPswf() r_adr = %d,s_adr = %d", ContentPSWF.R_ADR,ContentPSWF.S_ADR);
 //qmDebugMessage(QmDebug::Dump, "recPswf() private_lcode = %d, lcode = %d", private_lcode,code);
 //qmDebugMessage(QmDebug::Dump, "recPswf() pswf_in = %d, pswf_rec = %d, pswf_in_virt = %d ",pswf_in, pswf_rec, pswf_in_virt);
 if (control->virtual_mode && indicator == 31)
 {
	 control->pswf_in_virt++;
	 uint8_t counter = 37;
	 if (control->virtual_mode)  counter += 65;
	 if (control->pswf_in_virt > counter && !control->pswf_rec)
	 {
		 control->startVirtualPpsModeRx();
	 }
 }

 if (code == control->private_lcode)
 {
	 //lcode can be overflow (==0) TODO fix
	 if (indicator == 30)
	 {
		 control->firstTrueCommand = control->ContentPSWF.COM_N;
		 ++control->pswf_rec;
		 if (control->pswf_rec == 2)
		 {
			 control->ContentPSWF.COM_N = data;
			 control->firstPacket(control->ContentPSWF.COM_N, control->ContentPSWF.S_ADR , true);
			 control->waitAckTimer = 0;
		 }

		 if (control->pswf_rec >= 2)
		 {
			 control->qwitCounterChanged(60 - control->pswf_in);
		 }
	 }
 }
 if (control->pswf_in < 30)
 {
	 if (control->pswf_rec)
	 {
		 control->pswf_in++;
		 if (control->pswf_in == 1) control->startCondReceiving();
	 }
 }
 else
 {
	 if (control->pswf_rec >= 1)
	 {
		 if (control->pswf_rec == 1)
		 {
			 control->firstPacket(control->firstTrueCommand, control->ContentPSWF.S_ADR - 32, false); // false - not reliable data
			 control->waitAckTimer = 0;
		 }

		 if (control->ContentPSWF.R_ADR > 32)
		 {
			 control->pswf_ack = true; // need transmit ask
			 control->setAsk   = true; // doing transmit ask
			 if (control->pswf_rec >= 2)  control->isPswfFull = true;
		 }
	 }
	 if (!control->pswf_ack) control->exitVoceMode();

	 control->pswf_rec = 0;
	 control->pswf_in = 0;
}
}

void PswfModes::recSms(uint8_t *data)
{
	//qmDebugMessage(QmDebug::Dump, "processReceivedFrame() data_len = %d", data_len);
	if (control->sms_counter > 38 && control->sms_counter < 76)
	{
		uint8_t index_sms = 7 * (control->sms_counter - 40);
		for(int i = 0; i <  7; i++)
		{
			control->smsDataPacket[index_sms + i ] = data[i+8];
			control->rs_data_clear[index_sms + i ] = data[i+16];
			//qmDebugMessage(QmDebug::Dump, "data[%d] = %d", i, data[i+8]);
		}
	}

	if (control->sms_counter > 19 && control->sms_counter < 38)
	{
		control->tx_call_ask_vector.push_back(data[9]); // wzn response
		//qmDebugMessage(QmDebug::Dump, "LogicPswfModes() (19;38) WAVE_ZONE = %d", data[9]);
	}

	if (control->sms_counter < 19)
	{
		control->snr.erase(control->snr.begin());
		control->snr.push_back(data[6]);
		control->syncro_recieve.erase(control->syncro_recieve.begin());
		control->syncro_recieve.push_back(data[9]); // CYC_N
		control->smsSmallCounter++;
		//qmDebugMessage(QmDebug::Dump, "LogicPswfModes() (0;19) WAVE_ZONE = %d", data[9]);
		//qmDebugMessage(QmDebug::Dump, "recieve frame() count = %d", syncro_recieve.size());

		control->ContentSms.R_ADR = data[8]; // todo: check
		if (check_rx_call(&control->wzn_value))
		{
			control->sms_call_received = true;

			if (control->ContentSms.R_ADR > 32) control->pswf_ack = true;
			//	qmDebugMessage(QmDebug::Dump, "sms call received");
			control->syncro_recieve.clear();
			for(int i = 0; i<18;i++)
				control->syncro_recieve.push_back(99);
		}
	}

	if (control->sms_counter > 76 && control->sms_counter < 83)
	{
		control->prevTime();

		uint8_t ack           = data[9];
		uint8_t ack_code      = data[10];
		uint8_t ack_code_calc = calc_ack_code(ack);

		//qmDebugMessage(QmDebug::Dump, "recieve count sms = %d %d", ack_code_calc, data[10]);
		if (ack_code_calc == ack_code)
		{
			if (ack == 73) ++control->ok_quit;
			if (ack == 99) ++control->smsError;
		}

		control->quit_vector.push_back(ack);      // ack
		control->quit_vector.push_back(ack_code); // ack code
	}
}

PswfModes::trFrame PswfModes::sendPswf()
{
	int time[4] = {0,0,0,0};

	if (control->virtual_mode)
	{
		time[0] = control->d.day;
		time[1] = control->t.hours;
		time[2] = control->t.minutes;
		time[3] = control->t.seconds;
		//qmDebugMessage(QmDebug::Dump, "time now: %d %d %d %d" ,d.day,t.hours,t.minutes,t.seconds);
	}
	else
	{
		for(int i = 0; i<4;i++) time[i] = control->date_time[i];
	}

	if (control->ContentPSWF.RET_end_adr > 0)
	{
		control->ContentPSWF.L_CODE = control->navigator->Calc_LCODE_RETR(
				control->ContentPSWF.RET_end_adr,
				control->ContentPSWF.R_ADR,
				control->ContentPSWF.COM_N,
				control->ContentPSWF.RN_KEY,
				time[0],
				time[1],
				time[2],
				time[3]);
	}
	else
	{
		control->ContentPSWF.L_CODE = control->navigator->Calc_LCODE(
				control->ContentPSWF.R_ADR,
				control->ContentPSWF.S_ADR,
				control->ContentPSWF.COM_N,
				control->ContentPSWF.RN_KEY,
				time[0],
				time[1],
				time[2],
				time[3]);
	}

	control->ContentPSWF.Frequency = control->getFrequency(0); //pswf = 0, sms = 1
	control->ContentPSWF.indicator = 20;
	control->ContentPSWF.TYPE = 0;
	control->ContentPSWF.SNR =  9;

	uint8_t tx_address = 0x72;
	uint8_t tx_data[249];
	int tx_data_len = 0;

	qmToBigEndian((uint8_t)control->ContentPSWF.indicator,  tx_data + tx_data_len); ++tx_data_len;
	qmToBigEndian((uint8_t)control->ContentPSWF.TYPE,       tx_data + tx_data_len); ++tx_data_len;

	qmToBigEndian((uint32_t)control->ContentPSWF.Frequency, tx_data + tx_data_len); tx_data_len += 4;
	qmToBigEndian((uint8_t)control->ContentPSWF.SNR,        tx_data + tx_data_len); ++tx_data_len;

	qmToBigEndian((uint8_t)(control->pswf_retranslator ? control->ContentPSWF.RET_end_adr : control->ContentPSWF.R_ADR), tx_data+tx_data_len);   ++tx_data_len;
	qmToBigEndian((uint8_t)(control->pswf_retranslator ? control->ContentPSWF.R_ADR       : control->ContentPSWF.S_ADR), tx_data + tx_data_len); ++tx_data_len;

	qmToBigEndian((uint8_t)control->ContentPSWF.COM_N,  tx_data + tx_data_len); ++tx_data_len;
	qmToBigEndian((uint8_t)control->ContentPSWF.L_CODE, tx_data + tx_data_len); ++tx_data_len;

	frame.address = tx_address;
	frame.data 	  = tx_data;
	frame.len     = tx_data_len;
	return frame;
//	transport->transmitFrame(tx_address, tx_data, tx_data_len);
}

PswfModes::trFrame PswfModes::sendSms()
{
	control->ContentSms.Frequency =  control->getFrequency(1); // sms = 1
	control->smsCounterFreq(control->ContentSms.Frequency);
	control->ContentSms.indicator = 20;
	control->ContentSms.SNR =  7;
	int time[4] = {0,0,0,0};
	if (control->virtual_mode)
	{
		time[0] = control->d.day;
		time[1] = control->t.hours;
		time[2] = control->t.minutes;
		time[3] = control->t.seconds;
	}
	else
	{
		for(int i = 0; i<4;i++) time[i] = control->date_time[i];
	}

	if (control->sms_counter >= 19 && control->sms_counter <= 38)
	{
		control->ContentSms.L_CODE = control->navigator->Calc_LCODE_SMS(
				control->ContentSms.R_ADR,
				control->ContentSms.S_ADR,
				control->wzn_value,
				control->ContentSms.RN_KEY,
				time[0],
				time[1],
				time[2],
				time[3]);
	}
	else
	{
		control->ContentSms.L_CODE = control->navigator->Calc_LCODE(
				control->ContentSms.R_ADR,
				control->ContentSms.S_ADR,
				control->sms_counter,
				control->ContentSms.RN_KEY,
				time[0],
				time[1],
				time[2],
				time[3]);
	}
	//qmDebugMessage(QmDebug::Dump, "LCODE: %d",ContentSms.L_CODE);
	control->ContentSms.TYPE  = (control->sms_counter > 38 && control->sms_counter < 76) ? 1 : 0;

	uint8_t tx_address = 0x72;
	uint8_t tx_data[249];
	int tx_data_len = 0;

	qmToBigEndian((uint8_t)control->ContentSms.indicator, tx_data + tx_data_len); ++tx_data_len;
	qmToBigEndian((uint8_t)control->ContentSms.TYPE, tx_data      + tx_data_len); ++tx_data_len;
	qmToBigEndian((uint32_t)control->ContentSms.Frequency, tx_data+ tx_data_len); tx_data_len += 4;

	// for(int i = 0;i<6;i++)
	// qmDebugMessage(QmDebug::Dump,"ContentSms.massive = %d",ContentSms.message[i]);
	// qmDebugMessage(QmDebug::Dump, " ContentSms.Frequency =  %d " ,ContentSms.Frequency);
	// tx

	if (control->sms_counter < 19 && control->SmsLogicRole == control->SmsRoleTx)
	{
		static int counter = 0;
		if (counter == 18) counter = 0;
		++counter;

		qmToBigEndian((uint8_t)control->ContentSms.SNR,   tx_data + tx_data_len); ++tx_data_len;
		qmToBigEndian((uint8_t)control->ContentSms.R_ADR, tx_data + tx_data_len); ++tx_data_len;
		qmToBigEndian((uint8_t)control->ContentSms.S_ADR, tx_data + tx_data_len); ++tx_data_len;
		qmToBigEndian((uint8_t)counter,          		  tx_data + tx_data_len); ++tx_data_len;
		qmToBigEndian((uint8_t)control->ContentSms.L_CODE,tx_data + tx_data_len); ++tx_data_len;
	}
	// tx
	if ((control->sms_counter > 38 && control->sms_counter < 77) && (control->SmsLogicRole == control->SmsRoleTx))
	{
		uint8_t FST_N = 0;
		if (control->virtual_mode)
			FST_N = calcFstn(control->ContentSms.R_ADR,
					control->ContentSms.S_ADR,
					control->ContentSms.RN_KEY,
					control->d.day,
					control->t.hours,
					control->t.minutes,
					control->t.seconds,
					control->sms_counter - 39);
		else
			FST_N = calcFstn(control->ContentSms.R_ADR,
					control->ContentSms.S_ADR,
					control->ContentSms.RN_KEY,
					control->date_time[0],
					control->date_time[1],
					control->date_time[2],
					control->date_time[3],
					control->sms_counter - 39);
		++control->QNB;
		//qmDebugMessage(QmDebug::Dump, "sendSms() FSTN: %d", FST_N);
		if (control->cntChvc > 255) control->cntChvc = 7;
		qmToBigEndian((uint8_t)control->ContentSms.SNR, tx_data+tx_data_len); ++tx_data_len;
		qmToBigEndian((uint8_t)FST_N,          			tx_data+tx_data_len); ++tx_data_len;

		for(int i = control->cntChvc - 7;i<control->cntChvc;i++)
		{
			qmToBigEndian(control->ContentSms.message[i], tx_data+tx_data_len);++tx_data_len;
			//qmDebugMessage(QmDebug::Dump, "MESSG: %d",ContentSms.message[i]);
		}
		control->cntChvc = control->cntChvc + 7;
	}
	//rx
	if ((control->sms_counter > 19 && control->sms_counter < 38) && (control->SmsLogicRole == control->SmsRoleRx))
	{
		int wzn = control->wzn_value;
		qmToBigEndian((uint8_t)control->ContentSms.SNR,   tx_data+tx_data_len); ++tx_data_len;
		qmToBigEndian((uint8_t)control->ContentSms.R_ADR, tx_data+tx_data_len); ++tx_data_len;
		qmToBigEndian((uint8_t)control->ContentSms.S_ADR, tx_data+tx_data_len); ++tx_data_len;
		qmToBigEndian((uint8_t)wzn,              		  tx_data+tx_data_len); ++tx_data_len;
		//qmDebugMessage(QmDebug::Dump, "SADR: %d",ContentSms.S_ADR);
		//qmDebugMessage(QmDebug::Dump, "RADR: %d",ContentSms.R_ADR);
		//qmDebugMessage(QmDebug::Dump, "LCODE: %d",ContentSms.L_CODE);
		qmToBigEndian((uint8_t)control->ContentSms.L_CODE, tx_data+tx_data_len);++tx_data_len;
	}

	if ((control->sms_counter > 76 && control->sms_counter < 83) && (control->SmsLogicRole == control->SmsRoleRx))
	{
		qmToBigEndian((uint8_t)control->ContentSms.SNR,   tx_data+tx_data_len); ++tx_data_len;
		qmToBigEndian((uint8_t)control->ContentSms.R_ADR, tx_data+tx_data_len); ++tx_data_len;
		qmToBigEndian((uint8_t)control->ContentSms.S_ADR, tx_data+tx_data_len); ++tx_data_len;
		qmToBigEndian((uint8_t)control->ack, 			 		  tx_data+tx_data_len); ++tx_data_len;

		uint8_t ack_code  = calc_ack_code(control->ack);
		qmToBigEndian((uint8_t)ack_code, 				  tx_data+tx_data_len);  ++tx_data_len;
	}

	frame.address = tx_address;
	frame.data 	  = tx_data;
	frame.len     = tx_data_len;

//	control->transport->transmitFrame(tx_address, tx_data, tx_data_len);
//	transmitFrame(frame.address, frame.data,frame.len);

	return frame;
	}


} /* namespace Multiradio */
