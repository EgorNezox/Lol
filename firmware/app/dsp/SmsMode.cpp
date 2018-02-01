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
#include "qmtimer.h"
#include <cstring>
#include "../dsp/rs_tms.h"
#include "../synchro/virtual_timer.h"

namespace Multiradio
{

using namespace Galua;

rs_settings rs_255_93;

void DspController::galuaInit()
{
    initrs(rs_255_93);
    GenerateGaloisField(&rs_255_93);
    gen_poly(&rs_255_93);
}

void DspController::LogicSmsRx()
{
	 //  qmDebugMessage(QmDebug::Dump, "RxSmsWork()");
		if (radio_state == radiostateSync) return;

		if (sms_counter < 20)
		{
		//	 qmDebugMessage(QmDebug::Dump, "_____RxSmsWork() smsSmallCounter = %d", smsSmallCounter);
		     if (syncro_recieve.size() % 2 == 0 )
		    	 smsCounterChanged(smsSmallCounter);
		}

		if (smsFind)
		{
			++sms_counter;

			if (sms_counter ==  20)
			{
				setTx();
				sendSms();
			}

	        if (sms_counter > 20 && sms_counter < 38)
	        	sendSms(PSWFTransmitter);

	        if (sms_counter == 39)
			{
	        	setRx();
	        	setrRxFreq();
			}

			if (sms_counter > 39 && sms_counter < 76)
			{
				setrRxFreq();
			}
			if (sms_counter == 77)
			{
				setTx();
				quest = generateSmsReceived();
				sendSms();
			}
			if (sms_counter > 77 && sms_counter < 83)
			{
				sendSms();
			}

	        if (sms_counter == 84)
			{
	        	if (quest)
	        	{
	        		smsPacketMessage(indexSmsLen); quest = false;
	        	}

	        	sms_counter = 0;
	        	smsSmallCounter = 0;
	        	setRx();
	        	smsFind = false;
	            uint8_t ret = getSmsRetranslation();
	            if (ret == 0)
	            	resetSmsState();
			}
		}
		else
		{
			if (sms_call_received)
			{
				smsFind  = true;
				sms_call_received = false;
				sms_counter = 19;
				pswf_in_virt = 0;
			}
			else
			{
				setrRxFreq();
			}
		}
}

void DspController::LogicSmsTx()
{
	//   qmDebugMessage(QmDebug::Dump, "TxSmsWork()");
		if (radio_state == radiostateSync) return;

	    ++sms_counter;

	    if (sms_counter < 19)
	    {
	    	sendSms();
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
	    		sendSms();
	    	}
	    	else
	    	{
	    		resetSmsState();
	    		smsFailed(0);
	    	}
	    }

	    if (sms_counter > 39 && sms_counter < 76)
	    {
	    	sendSms();
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
	    	if (ok_quit >= 1)
	    		smsFailed(-1); //correct recieved
	    	else
	    	{
	    		if (smsError >= 1)
	    			smsFailed(2); // negative ack recieved
	    		else
	    			smsFailed(1); // not ack recieved
	    	}
	    	resetSmsState();
	    }
}

uint32_t DspController::CalcSmsTransmitFreq(uint32_t RN_KEY, uint32_t DAY, uint32_t HRS, uint32_t MIN, uint32_t SEC)
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
		//qmDebugMessage(QmDebug::Dump, "wzn_base %d" ,wz_base);
		//qmDebugMessage(QmDebug::Dump, "wzn_shift %d" ,wz_shift);

		if (sms_counter < 77)
		{
			FR_SH = (RN_KEY + 3*SEC + 230*SEC_MLT + 17*MIN + 29*HRS + 43*DAY)% TOT_W;
			//qmDebugMessage(QmDebug::Dump, "Calc freq sms  formula %d", FR_SH);
		}

		if (sms_counter > 77 && sms_counter < 83)
		{
			FR_SH = (RN_KEY + 5*SEC + 230*SEC_MLT + 17*MIN + 29*HRS + 43*DAY)% TOT_W;
			//qmDebugMessage(QmDebug::Dump, "Calc freq sms quit formula %d", FR_SH);
		}
	}

	if (sms_counter < 38)
	{
		FR_SH = (RN_KEY + 73 + 230*SEC_MLT + 17*MIN + 29*HRS + 43*DAY)% TOT_W;
		//qmDebugMessage(QmDebug::Dump, "Calc freq sms tx formula %d", FR_SH);
	}

	if ((sms_counter < 19) && (SmsLogicRole == SmsRoleRx))
	{
		waveZone.erase(waveZone.begin());
		waveZone.push_back(SEC_MLT / 6);
	}

	//qmDebugMessage(QmDebug::Dump, ">>> CalcSmsTransmitFreq() wzn_value %d" ,wzn_value);
	return FR_SH;
}

void DspController::recSms(uint8_t *data)
{
	//qmDebugMessage(QmDebug::Dump, "processReceivedFrame() data_len = %d", data_len);
	if (sms_counter > 38 && sms_counter < 76)
	{
		uint8_t index_sms = 7 * (sms_counter - 40);
		for(int i = 0; i <  7; i++)
		{
			smsDataPacket[index_sms + i ] = data[i+8];
			rs_data_clear[index_sms + i ] = data[i+16];
			//qmDebugMessage(QmDebug::Dump, "data[%d] = %d", i, data[i+8]);
		}
	}

	if (sms_counter > 19 && sms_counter < 38)
	{
		tx_call_ask_vector.push_back(data[9]); // wzn response
		//qmDebugMessage(QmDebug::Dump, "LogicDspController() (19;38) WAVE_ZONE = %d", data[9]);
	}

	if (sms_counter < 19)
	{
		snr.erase(snr.begin());
		snr.push_back(data[6]);
		syncro_recieve.erase(syncro_recieve.begin());
		syncro_recieve.push_back(data[9]); // CYC_N
		smsSmallCounter++;
		//qmDebugMessage(QmDebug::Dump, "LogicDspController() (0;19) WAVE_ZONE = %d", data[9]);
		//qmDebugMessage(QmDebug::Dump, "recieve frame() count = %d", syncro_recieve.size());

		ContentSms.R_ADR = data[8]; // todo: check
		if (check_rx_call(&wzn_value))
		{
			sms_call_received = true;

			if (ContentSms.R_ADR > 32) pswf_ack = true;
			//	qmDebugMessage(QmDebug::Dump, "sms call received");
			syncro_recieve.clear();
			for(int i = 0; i<18;i++)
				syncro_recieve.push_back(99);
		}
	}

	if (sms_counter > 76 && sms_counter < 83)
	{
		prevTime();

		uint8_t ack           = data[9];
		uint8_t ack_code      = data[10];
		uint8_t ack_code_calc = calc_ack_code(ack);

		//qmDebugMessage(QmDebug::Dump, "recieve count sms = %d %d", ack_code_calc, data[10]);
		if (ack_code_calc == ack_code)
		{
			if (ack == 73) ++ok_quit;
			if (ack == 99) ++smsError;
		}

		quit_vector.push_back(ack);      // ack
		quit_vector.push_back(ack_code); // ack code
	}
}

void DspController::startSmsRx()
{
	//qmDebugMessage(QmDebug::Dump, "startSmsReceiving");
	//QM_ASSERT(is_ready);
	for(int i = 0; i<255; i++) rs_data_clear[i] = 1;

	tx_call_ask_vector.erase(
	tx_call_ask_vector.begin(),
	tx_call_ask_vector.end());

	quit_vector.erase(
	quit_vector.begin(),
	quit_vector.end());

	radio_state     = radiostateSms;
	sms_counter     = 0;
	smsSmallCounter = 0;

	syncro_recieve.clear();
	snr.clear();
	waveZone.clear();

	for(int i = 0; i<18;i++)
	{
		syncro_recieve.push_back(99);
		snr.push_back(0);
		waveZone.push_back(0);
	}
	waveZone.push_back(0); // size must be 19

	for(uint8_t i = 0; i <= 100; i++)
		sms_content[i] = 0;

	if (virtual_mode) startVirtualPpsModeRx();
	else setRx();

}

DspController::trFrame DspController::sendSms()
{
	ContentSms.Frequency =  getFrequency(1); // sms = 1
	ContentSms.indicator = 20;
	ContentSms.SNR =  7;
	int time[4] = {0,0,0,0};
	if (virtual_mode)
	{
		time[0] = d.day;
		time[1] = t.hours;
		time[2] = t.minutes;
		time[3] = t.seconds;
	}
	else
	{
		for(int i = 0; i<4;i++) time[i] = date_time[i];
	}

	if (sms_counter >= 19 && sms_counter <= 38)
	{
		ContentSms.L_CODE = navigator->Calc_LCODE_SMS(
				ContentSms.R_ADR,
				ContentSms.S_ADR,
				wzn_value,
				ContentSms.RN_KEY,
				time[0],
				time[1],
				time[2],
				time[3]);
	}
	else
	{
		ContentSms.L_CODE = navigator->Calc_LCODE(
				ContentSms.R_ADR,
				ContentSms.S_ADR,
				sms_counter,
				ContentSms.RN_KEY,
				time[0],
				time[1],
				time[2],
				time[3]);
	}
	//qmDebugMessage(QmDebug::Dump, "LCODE: %d",ContentSms.L_CODE);
	ContentSms.TYPE  = (sms_counter > 38 && sms_counter < 76) ? 1 : 0;

	uint8_t tx_address = 0x72;
	uint8_t tx_data[249];
	int tx_data_len = 0;

	qmToBigEndian((uint8_t)ContentSms.indicator, tx_data + tx_data_len); ++tx_data_len;
	qmToBigEndian((uint8_t)ContentSms.TYPE, tx_data      + tx_data_len); ++tx_data_len;
	qmToBigEndian((uint32_t)ContentSms.Frequency, tx_data+ tx_data_len); tx_data_len += 4;

	// for(int i = 0;i<6;i++)
	// qmDebugMessage(QmDebug::Dump,"ContentSms.massive = %d",ContentSms.message[i]);
	// qmDebugMessage(QmDebug::Dump, " ContentSms.Frequency =  %d " ,ContentSms.Frequency);
	// tx

	if (sms_counter < 19 && SmsLogicRole == SmsRoleTx)
	{
		static int counter = 0;
		if (counter == 18) counter = 0;
		++counter;

		qmToBigEndian((uint8_t)ContentSms.SNR,   tx_data + tx_data_len); ++tx_data_len;
		qmToBigEndian((uint8_t)ContentSms.R_ADR, tx_data + tx_data_len); ++tx_data_len;
		qmToBigEndian((uint8_t)ContentSms.S_ADR, tx_data + tx_data_len); ++tx_data_len;
		qmToBigEndian((uint8_t)counter,          		  tx_data + tx_data_len); ++tx_data_len;
		qmToBigEndian((uint8_t)ContentSms.L_CODE,tx_data + tx_data_len); ++tx_data_len;
	}
	// tx
	if ((sms_counter > 38 && sms_counter < 77) && (SmsLogicRole == SmsRoleTx))
	{
		uint8_t FST_N = 0;
		if (virtual_mode)
			FST_N = calcFstn(ContentSms.R_ADR,
					ContentSms.S_ADR,
					ContentSms.RN_KEY,
					d.day,
					t.hours,
					t.minutes,
					t.seconds,
					sms_counter - 39);
		else
			FST_N = calcFstn(ContentSms.R_ADR,
					ContentSms.S_ADR,
					ContentSms.RN_KEY,
					date_time[0],
					date_time[1],
					date_time[2],
					date_time[3],
					sms_counter - 39);
		++QNB;
		//qmDebugMessage(QmDebug::Dump, "sendSms() FSTN: %d", FST_N);
		if (cntChvc > 255) cntChvc = 7;
		qmToBigEndian((uint8_t)ContentSms.SNR, tx_data+tx_data_len); ++tx_data_len;
		qmToBigEndian((uint8_t)FST_N,          			tx_data+tx_data_len); ++tx_data_len;

		for(int i = cntChvc - 7;i<cntChvc;i++)
		{
			qmToBigEndian(ContentSms.message[i], tx_data+tx_data_len);++tx_data_len;
			//qmDebugMessage(QmDebug::Dump, "MESSG: %d",ContentSms.message[i]);
		}
		cntChvc = cntChvc + 7;
	}
	//rx
	if ((sms_counter > 19 && sms_counter < 38) && (SmsLogicRole == SmsRoleRx))
	{
		int wzn = wzn_value;
		qmToBigEndian((uint8_t)ContentSms.SNR,   tx_data+tx_data_len); ++tx_data_len;
		qmToBigEndian((uint8_t)ContentSms.R_ADR, tx_data+tx_data_len); ++tx_data_len;
		qmToBigEndian((uint8_t)ContentSms.S_ADR, tx_data+tx_data_len); ++tx_data_len;
		qmToBigEndian((uint8_t)wzn,              		  tx_data+tx_data_len); ++tx_data_len;
		//qmDebugMessage(QmDebug::Dump, "SADR: %d",ContentSms.S_ADR);
		//qmDebugMessage(QmDebug::Dump, "RADR: %d",ContentSms.R_ADR);
		//qmDebugMessage(QmDebug::Dump, "LCODE: %d",ContentSms.L_CODE);
		qmToBigEndian((uint8_t)ContentSms.L_CODE, tx_data+tx_data_len);++tx_data_len;
	}

	if ((sms_counter > 76 && sms_counter < 83) && (SmsLogicRole == SmsRoleRx))
	{
		qmToBigEndian((uint8_t)ContentSms.SNR,   tx_data+tx_data_len); ++tx_data_len;
		qmToBigEndian((uint8_t)ContentSms.R_ADR, tx_data+tx_data_len); ++tx_data_len;
		qmToBigEndian((uint8_t)ContentSms.S_ADR, tx_data+tx_data_len); ++tx_data_len;
		qmToBigEndian((uint8_t)ack, 			 		  tx_data+tx_data_len); ++tx_data_len;

		uint8_t ack_code  = calc_ack_code(ack);
		qmToBigEndian((uint8_t)ack_code, 				  tx_data+tx_data_len);  ++tx_data_len;
	}

	frame.address = tx_address;
	frame.data 	  = tx_data;
	frame.len     = tx_data_len;
	return frame;
	//transport->transmitFrame(tx_address, tx_data, tx_data_len);
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
		LogicSmsTx();
	}
	if (SmsLogicRole == SmsRoleRx)
	{
        LogicSmsRx();
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

void DspController::sendSms(Module module)
{
	trFrame f = sendSms();
    transport->transmitFrame(f.address, f.data, f.len);
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

void DspController::startSMSRecieving(SmsStage stage)
{
	QM_ASSERT(is_ready);
	startSmsRx();
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

void DspController::setSmsRetranslation(uint8_t retr)
{
    sms_retranslation = retr;
}

uint8_t DspController::getSmsRetranslation()
{
    return sms_retranslation;
}


} /* namespace Multiradio */
