#include "ale_fxn.h"

#define QMDEBUGDOMAIN	ALE

namespace Multiradio {

unsigned short table16[256];    //  CRC
unsigned int table32[256];      //  CRC

const char unk[]="unknown";
const char call_manual[]="CALL_MANUAL";
const char call_gps[]="CALL_GPS";
const char hshake[]="HSHAKE";
const char trans_mode[]="TRANS_MODE";
const char call_qual[]="CALL_QUAL";
const char link_release[]="LINK_RELEASE";
const char short_sound[]="SHORT_SOUND";
const char long_sound[]="LONG_SOUND";
const char sound_qual[]="SOUND_QUAL";
const char msg_head[]="MSG_HEAD";
const char pack_head[]="PACK_HEAD";
const char pack_qual[]="PACK_QUAL";

const char* msg_names[]={ 	call_manual, 	call_gps,		hshake, 		hshake,		trans_mode,
							call_qual, 		link_release,	unk,			unk,		unk,
							short_sound,	long_sound,		sound_qual,		unk,		unk,
							unk,			unk,			unk,			unk,		unk,
							unk,			msg_head,		pack_head,		pack_qual,	unk	};

const char off[]="OFF";
const char call[]="CALL";
const char trans[]="TRANSMODE";
const char probe[]="PROBES";
const char probe_qual[]="PROBE_QUAL";
//const char msg_head[]="MSG_HEAD";
const char data_trans[]="DATA_TRANSPORT";
const char data_end[]="END_OF_TRANSPORT";

const char* superphase_names[]={ off, call, trans, probe, probe_qual, off, off, msg_head, off, data_trans, data_end };

AleFxn::AleFxn(ContTimer *tmr, Dispatcher *disp, ale_data* tmp_ale, ext_ale_settings* ale_s,  OldAleData* oldAle )
{
    timer=tmr;
    dispatcher=disp;
    temp_ale=tmp_ale;
    ale_settings=ale_s;
    ale=oldAle;
}

int8s AleFxn::get_min_value(int8s* data, int8s length)
{
    int8s min;
    min=data[0];
    for(int8s i=1;i<length;i++)
    {
        if(data[i]<min)
            min=data[i];
    }
    return min;
}

void AleFxn::aleprocessTX_modem(int8s packet_type, int8s* data, int8s length) {
    DspController::ModemPacketType type;
    type=(DspController::ModemPacketType)packet_type;
    dispatcher->dsp_controller->sendModemPacket(type, DspController::modembw20kHz, (uint8_t*)data, length);
}

void AleFxn::set_freq(long freq)
{
    dispatcher->dsp_controller->tuneModemFrequency(freq);
    //ale_log("Set ALE freq %u",(int)(freq));
}

void AleFxn::set_tx(int8s mode)
{
    if(mode!=0)
        dispatcher->dsp_controller->enableModemTransmitter();
    else
        dispatcher->dsp_controller->disableModemTransmitter();
}

void AleFxn::set_rx(int8s mode)
{
    if(mode!=0)
        dispatcher->dsp_controller->enableModemReceiver();
    else
        dispatcher->dsp_controller->disableModemReceiver();
}

void AleFxn::set_rx_bw(int8s bandwidth)
{
    dispatcher->dsp_controller->setModemReceiverBandwidth((DspController::ModemBandwidth)bandwidth);
}

void AleFxn::set_wait_msg(int8s msg)
{
    dispatcher->dsp_controller->setModemReceiverPhase((DspController::ModemPhase) msg);
}

void AleFxn::set_caller_mode(bool mode)		// 0 - responder, 1 - caller
{
    if(mode)	dispatcher->dsp_controller->setModemReceiverRole(DspController::modemroleCaller);
    else		dispatcher->dsp_controller->setModemReceiverRole(DspController::modemroleResponder);
}

void AleFxn::set_rx_mode(int mode)
{
    if(temp_ale->rx==mode)
        return;
    temp_ale->rx=mode;
    set_rx(mode);
}

void AleFxn::set_tx_mode(int mode)
{
    if(temp_ale->tx==mode)
        return;
    temp_ale->tx=mode;
    set_tx(mode);
}

void AleFxn::error_detect(int error_num)
{

}

void AleFxn::ale_logic_error(int8s error_num)
{

}

void AleFxn::tx_off_control()
{
    if(temp_ale->tx==0)
        return;
    set_tx_mode(0);
    error_detect(2);
}

void AleFxn::return_to_call()
{
	if((ale_settings->phase==2)||(ale_settings->superphase!=1))
		ale_log("ALE error, superphase %u, phase %u", ale_settings->superphase, ale_settings->phase);
    timer->stop_timer();
    ale_settings->result=1;		//	ERROR DETECTED
    if(ale_settings->superphase<7)
    	set_next_superphase(1);	//	IF ALE, RETURN TO CALL PROCEDURE
    else
    	set_next_superphase(0);	//	if DATA_TRANSPORT, RETURN TO IDLE
    //ale_settings->superphase=1;
    //ale_settings->phase=0;
    //ale_settings->neg_counter=0;
    set_tx_mode(0);
    set_rx_mode(0);
}

void AleFxn::gen_msg_call()
{
    temp_ale->tx_msg.data_length=AleCom::generate_call(temp_ale->tx_msg.data, true,
                                          ale_settings->call_supercounter, ale_settings->adress_dst);
}

void AleFxn::gen_trans_mode()
{
    if(ale_settings->probe_on)
        temp_ale->tx_msg.data_length=AleCom::generate_trans_mode(temp_ale->tx_msg.data, 3, 3, 1,
                                             ale_settings->schedule, ale_settings->own_adress);
    else
        temp_ale->tx_msg.data_length=AleCom::generate_trans_mode(temp_ale->tx_msg.data, 0, 3, 1,
                                             ale_settings->schedule, ale_settings->own_adress);
}

void AleFxn::gen_call_qual()
{
    temp_ale->tx_msg.data_length=AleCom::generate_resp_call_qual(temp_ale->tx_msg.data,
                                 (int8s)(((int16s)temp_ale->call_err)*19/100), temp_ale->call_snr);
}

void AleFxn::gen_sound_qual()
{
    temp_ale->tx_msg.data_length=AleCom::generate_sound_qual(temp_ale->tx_msg.data,temp_ale->best_freq_index,
                                         temp_ale->best_freq_snr,temp_ale->best_freq_sign_form);
}

void AleFxn::gen_msg_head()
{
    temp_ale->tx_msg.data_length=AleCom::generate_msg_head(temp_ale->tx_msg.data, get_msg_size());
}

int16s AleFxn::get_msg_size()
//  RETURN LENGTH IN 72 BIT FORMAT
{
    return ale_settings->data72bit_length;
}

void AleFxn::gen_pack_head()
{
    temp_ale->tx_msg.data[0]=temp_ale->sign_form;
    temp_ale->tx_msg.data[1]=0;
    get_msg_fragment(ale_settings->phase/3,&(temp_ale->tx_msg.data[2]));
    temp_ale->tx_msg.data_length=68;
}

void AleFxn::gen_pack_qual()
{
    temp_ale->tx_msg.data_length=
            AleCom::generate_resp_pack_qual(temp_ale->tx_msg.data, temp_ale->pack_result,temp_ale->pack_snr);
    ale_log("Send PACK_QUAL, RES %u, SNR %u",(int)temp_ale->pack_result, temp_ale->pack_snr);
}

bool AleFxn::get_work_freq()
{
    int8s counter;
    counter=0;
    for(int8s i=0;i<ale_settings->work_freq_num;i++)
    {
        if((ale_settings->work_freq[i]>=(temp_ale->call_freq/2))&&(ale_settings->work_freq[i]<=(temp_ale->call_freq*2)))
        {
            temp_ale->work_freq[counter]=ale_settings->work_freq[i];
            counter++;
        }
    }
    temp_ale->work_freq_num=counter;
    if(counter==0)
        return false;
    return true;
}

void AleFxn::send_tx_msg(int8s msg_num)
{
    set_tx_mode(11);
    switch(msg_num)
    {
        case 0:
        case 1:
            gen_msg_call();
            break;
        case 4:
            gen_trans_mode();
            break;
        case 5:
            gen_call_qual();
            break;
        case 12:
            gen_sound_qual();
            break;
        case 21:
            gen_msg_head();
            break;
        case 22:
            temp_ale->time[PACK_HEAD][EMIT_PERIOD]=ideal_timings[PACK_HEAD][IDEAL_EMIT_PERIOD]+DSP_MSG_PACK_HEAD_TX_WAITING+pack_head_data_time[temp_ale->sign_form]+DSP_TX_STOP_TIME;
            gen_pack_head();
            ale_log("Send PACK_HEAD, sign_form %u",temp_ale->sign_form);
            break;
        case 23:
            gen_pack_qual();
            break;
        default:
            temp_ale->tx_msg.data_length=0;
    }
    if((msg_num!=PACK_HEAD)&&(msg_num!=RESP_PACK_QUAL))
    	ale_log("Send msg, type %s", msg_names[msg_num]);
    aleprocessTX_modem(msg_num,temp_ale->tx_msg.data,temp_ale->tx_msg.data_length);
    timer->set_timer(temp_ale->time[msg_num][EMIT_PERIOD]);
    temp_ale->pause_state=false;
}

void AleFxn::wait_end_tx(int8s msg_num, int8s next_msg_num, bool next_mode)	// next mode true - TX
{
    int next_msg_wait_mode=0;
    if(next_mode)	next_msg_wait_mode=START_EMIT;
    else			next_msg_wait_mode=START_RECEIVE;
    tx_off_control();
    timer->set_timer(temp_ale->time[msg_num][EMIT_LAST_TIME]+temp_ale->time[next_msg_num][next_msg_wait_mode]);
    temp_ale->pause_state=true;
}

void AleFxn::wait_end_tx(int8s msg_num, int8s next_msg_num, bool next_mode, int32s inc)	// next mode true - TX
{
    int next_msg_wait_mode=0;
    if(next_mode)	next_msg_wait_mode=START_EMIT;
    else			next_msg_wait_mode=START_RECEIVE;
    tx_off_control();
    timer->set_timer(temp_ale->time[msg_num][EMIT_LAST_TIME]+
                     temp_ale->time[next_msg_num][next_msg_wait_mode]+inc);
    temp_ale->pause_state=true;
}

void AleFxn::start_receive_msg(int8s msg_num)
{
#ifdef	OLD_DSP_VERSION
    int8s msg_num_to_old_dsp_version[]={ 0, 0, 1, 0, 1,		1, 2, 0, 0, 0,		3, 4, 1, 0, 0,		0, 0, 0, 0, 0,		0, 2, 2, 2, 0,		2, 2 };
    set_wait_msg(msg_num_to_old_dsp_version[msg_num]);
#else
    set_wait_msg(msg_num);
#endif
    temp_ale->received_msg.data_length=-1;
    set_rx_mode(11);
    timer->set_timer(temp_ale->time[msg_num][RECEIVE_PERIOD]);
    temp_ale->pause_state=false;
    temp_ale->last_msg=false;
}

#define	ALE_UNKNOWN_STATE	25
void AleFxn::set_next_superphase(int8s superphase_num)
//	DO NOT USE IN MIX MSG AND PACK HEAD !!!
{
    ale_settings->phase=0;
    if(ale_settings->caller)
    {
		switch(superphase_num)
		{
			case 0:
				if(ale_settings->superphase==1)
					ale_settings->ale_state=11;	//	TX_CALL_FAIL
				else if(((ale_settings->superphase==10)||(ale_settings->superphase==9))&&(ale_settings->result==0))
					ale_settings->ale_state=16;	//	TX DATA FULL
				else if((ale_settings->superphase==9)&&(ale_settings->result!=0))
					ale_settings->ale_state=15;	//	TX DATA PART
				else if((ale_settings->superphase==7)&&(ale_settings->result!=0))
					ale_settings->ale_state=14;	//	TX DATA FAIL (MSG_HEAD UNCORRECT)
				else
					ale_settings->ale_state=ALE_UNKNOWN_STATE;
				break;
			case 1:
				ale_settings->ale_state=10;		//	TX_CALLING
				break;
			case 2:
				ale_settings->ale_state=12;		//	TX_CALL_NEG
				break;
			case 3:
				ale_settings->ale_state=21;		//	TX_PROBE
				break;
			case 4:
				ale_settings->ale_state=22;		//	TX_PROBE_QUAL
				break;
			case 7:
			case 9:
			case 10:
				ale_settings->ale_state=13;		//	TX_DATA_TRANS
				break;
			default:
				ale_settings->ale_state=ALE_UNKNOWN_STATE;
		}
    }
    else
    {
    	switch(superphase_num)
		{
			case 0:
				if(ale_settings->superphase==1)
					ale_settings->ale_state=11;	//	TX_CALL_FAIL
				else if((ale_settings->superphase==10)&&(ale_settings->result==0))
					ale_settings->ale_state=16;	//	TX DATA FULL
				else if((ale_settings->superphase==9)&&(ale_settings->result!=0))
					ale_settings->ale_state=15;	//	TX DATA PART
				else if((ale_settings->superphase==7)&&(ale_settings->result!=0))
					ale_settings->ale_state=14;	//	TX DATA FAIL (MSG_HEAD UNCORRECT)
				else
					ale_settings->ale_state=ALE_UNKNOWN_STATE;
				break;
			case 1:
				ale_settings->ale_state=10;		//	TX_CALLING
				break;
			case 2:
				ale_settings->ale_state=12;		//	TX_CALL_NEG
				break;
			case 3:
				ale_settings->ale_state=21;		//	TX_PROBE
				break;
			case 4:
				ale_settings->ale_state=22;		//	TX_PROBE_QUAL
				break;
			case 7:
			case 9:
			case 10:
				ale_settings->ale_state=13;		//	TX_DATA_TRANS
				break;
		}
    }
    ale_settings->superphase=superphase_num;
    ale_settings->neg_counter=0;
    ale_settings->nres0=0;
    ale_settings->nres1=0;
    ale_log("----------- SUPERPHASE: %s ------------",superphase_names[superphase_num]);
}

bool AleFxn::check_msg(int8s msg_type, bool rx_stop)
{
	if(rx_stop)
	{
        temp_ale->pause_state=true;
        set_rx_mode(0);
	}
	else
        temp_ale->last_msg=true;
    if((temp_ale->received_msg.data_length!=-1)&&(temp_ale->received_msg.type==msg_type)&&(temp_ale->received_msg.error!=100))
        return true;
    return false;
}

int8s AleFxn::get_packet_num()
{
    return ale_settings->data490bit_length;
}

int8s AleFxn::set_packet_num(int8s num_msg_head)
{
    ale->vm_size = num_msg_head;
    ale->vm_f_count = ceilf((float)ale->vm_size*72/490);
    ale->vm_fragments.resize(ale->vm_f_count);
    ale_settings->data72bit_length=ale->vm_size;
    ale_settings->data490bit_length=ale->vm_f_count;
    return ale->vm_f_count;
}

bool AleFxn::check_pack_head_crc(int8s* data)
{
    return true;
}

void AleFxn::get_msg_fragment(int16s num, int8s* data)
{
    for(int16s i = 0; i < 66; i++)
        data[i]=ale_settings->data[num*66+i];    //ale->vm_fragments[num].num_data[i];
}

void AleFxn::makeTable16()
{
    unsigned short r;
    for(int i=0; i<256; i++)
    {
        r = ((unsigned short)i)<<8;
        for(unsigned char j=0; j<8; j++)
        {
            if(r&(1<<15)) r=(r<<1)^0x8005;
            else r=r<<1;
        }
        table16[i]=r;
    }
}

//Функция вычисления CRC
unsigned short AleFxn::CRC16(int8s *buf, int len)
{
    unsigned short crc;
    crc = 0xFFFF;
    while(len--)
    {
        crc = table16[((crc>>8)^*buf++)&0xFF] ^ (crc<<8);
    }
    crc ^= 0xFFFF;
    return crc;
}

void AleFxn::makeTable32()
{
    const unsigned int CRC_POLY = 0xEDB88320;
    unsigned int i, j, r;
    for (i = 0; i< 256; i++)
    {
        for (r = i, j = 8; j; j--)
            r = r &1? (r>> 1) ^ CRC_POLY: r >> 1;
        table32[i] = r;
    }
}

//Функция вычисления CRC
unsigned int AleFxn::CRC32(int8s* pData, int len)
{
    const unsigned int CRC_MASK = 0xD202EF8D;
    unsigned int crc = 0;
    while (len--)
    {
        crc = table32[(unsigned char)crc ^ *pData++] ^ crc>> 8;
        crc ^= CRC_MASK;
    }
    return crc;
}

void AleFxn::ale_log(const char* text)
{
    QmDebug::message("ALE", QmDebug::Info, text);
}



void AleFxn::ale_log(const char* text, const char* arg)
{
    QmDebug::message("ALE", QmDebug::Info, text, arg);
}

void AleFxn::ale_log(const char* text, int arg)
{
    QmDebug::message("ALE", QmDebug::Info, text, arg);
}

void AleFxn::ale_log(const char* text, int arg1, int arg2)
{
    QmDebug::message("ALE", QmDebug::Info, text, arg1, arg2);
}

void AleFxn::ale_log(const char* text, int arg1, int arg2, int arg3)
{
    QmDebug::message("ALE", QmDebug::Info, text, arg1, arg2, arg3);
}

void AleFxn::ale_log(const char* text, int arg1, int arg2, int arg3, int arg4)
{
    QmDebug::message("ALE", QmDebug::Info, text, arg1, arg2, arg3, arg4);
}

void AleFxn::ale_log(const char* text, const char* arg1, int arg2, int arg3)
{
    QmDebug::message("ALE", QmDebug::Info, text, arg1, arg2, arg3);
}

}

#include "qmdebug_domains_start.h"
QMDEBUG_DEFINE_DOMAIN(ALE, LevelVerbose)
#include "qmdebug_domains_end.h"
