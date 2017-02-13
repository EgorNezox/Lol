
#include "ale_main.h"

namespace Multiradio {

AleMain::AleMain(ContTimer* tmr, ale_data* tmp_ale, ext_ale_settings* ale_s, AleFxn* ale_f)
{
    temp_ale=tmp_ale;
    ale_settings=ale_s;
    timer=tmr;
    ale_fxn=ale_f;
    for(int8s i=0;i<27;i++)
	{
		if(i!=PACK_HEAD)
		{
            temp_ale->time[i][START_EMIT]=ideal_timings[i][IDEAL_START_EMIT]-DSP_LIGHT_MSG_TX_WAITING;
            temp_ale->time[i][EMIT_PERIOD]=ideal_timings[i][IDEAL_EMIT_PERIOD]+DSP_LIGHT_MSG_TX_WAITING+DSP_TX_STOP_TIME;
			if(i==CALL_MANUAL)
			{
                temp_ale->time[i][START_RECEIVE]=ideal_timings[i][IDEAL_START_EMIT]-DT_MANUAL-DSP_RX_TURN_ON_WAITING;
                temp_ale->time[i][RECEIVE_PERIOD]=ideal_timings[i][EMIT_PERIOD]+2*DT_MANUAL+DSP_RX_TURN_ON_WAITING+DSP_LIGHT_MSG_RX_WAITING;
                temp_ale->time[i][RECEIVE_LAST_TIME]=ideal_timings[i][IDEAL_PHASE_TIME]-ideal_timings[i][IDEAL_START_EMIT]-ideal_timings[i][IDEAL_EMIT_PERIOD]-DSP_CALL_WAITING;
			}
			else if(i==CALL_GPS)
			{
                temp_ale->time[i][START_RECEIVE]=ideal_timings[i][IDEAL_START_EMIT]-DT_GPS-DSP_RX_TURN_ON_WAITING;
                temp_ale->time[i][RECEIVE_PERIOD]=ideal_timings[i][EMIT_PERIOD]+2*DT_GPS+DSP_RX_TURN_ON_WAITING+DSP_LIGHT_MSG_RX_WAITING;
                temp_ale->time[i][RECEIVE_LAST_TIME]=ideal_timings[i][IDEAL_PHASE_TIME]-ideal_timings[i][IDEAL_START_EMIT]-ideal_timings[i][IDEAL_EMIT_PERIOD]-DSP_CALL_WAITING;
			}
			else
			{
                temp_ale->time[i][START_RECEIVE]=ideal_timings[i][IDEAL_START_EMIT]-DT_ALE-DSP_RX_TURN_ON_WAITING;
                temp_ale->time[i][RECEIVE_PERIOD]=ideal_timings[i][EMIT_PERIOD]+2*DT_ALE+DSP_RX_TURN_ON_WAITING+DSP_LIGHT_MSG_RX_WAITING;
                temp_ale->time[i][RECEIVE_LAST_TIME]=ideal_timings[i][IDEAL_PHASE_TIME]-temp_ale->time[i][START_RECEIVE]-temp_ale->time[i][RECEIVE_PERIOD];
			}
            temp_ale->time[i][EMIT_LAST_TIME]=ideal_timings[i][IDEAL_PHASE_TIME]-temp_ale->time[i][START_EMIT]-temp_ale->time[i][EMIT_PERIOD];
            temp_ale->time[i][PHASE_TIME]=ideal_timings[i][IDEAL_PHASE_TIME];
		}
		else
		{
            temp_ale->time[i][START_EMIT]=ideal_timings[i][IDEAL_START_EMIT]-DSP_MSG_PACK_HEAD_TX_WAITING;
            temp_ale->time[i][EMIT_PERIOD]=ideal_timings[i][IDEAL_EMIT_PERIOD]+DSP_MSG_PACK_HEAD_TX_WAITING+29568+DSP_TX_STOP_TIME;
            temp_ale->time[i][EMIT_LAST_TIME]=PACK_HEAD_IDEAL_EMIT_LAST_TIME-DSP_TX_STOP_TIME;
            temp_ale->time[i][START_RECEIVE]=ideal_timings[i][IDEAL_START_EMIT]-DT_ALE-DSP_RX_TURN_ON_WAITING;
            temp_ale->time[i][RECEIVE_PERIOD]=ideal_timings[i][EMIT_PERIOD]+2*DT_ALE+DSP_RX_TURN_ON_WAITING+DSP_LIGHT_MSG_RX_WAITING;
            temp_ale->time[i][RECEIVE_LAST_TIME]=PACK_HEAD_IDEAL_EMIT_LAST_TIME-DSP_MSG_PACK_HEAD_RX_WAITING-DT_ALE;
		}	
	}
    temp_ale->pack_head_emit_time=32144;
    temp_ale->pack_head_phase_time=32917;
}

void AleMain::call_tx_mgr()
{
    switch(ale_settings->phase)
	{
		case 0:	// call tx
            if(temp_ale->pause_state)	// wait to start emit call
                ale_fxn->send_tx_msg(CALL_MANUAL+ale_settings->gps_en);
			else
			{
                ale_fxn->wait_end_tx(CALL_MANUAL+ale_settings->gps_en,HSHAKE,false);
                ale_settings->phase=1;
			}
			break;
		case 1:	// hshake wait
            if(temp_ale->pause_state)	// wait to start receive
                ale_fxn->start_receive_msg(HSHAKE);
			else
			{
                if((!ale_fxn->check_msg(HSHAKE,true))||(temp_ale->received_msg.snr<ale_hshake_snr_lim[ale_settings->call_supercounter]))
				{
                    ale_fxn->return_to_call();
					break;
				}
                timer->set_timer(temp_ale->time[HSHAKE][RECEIVE_LAST_TIME]+temp_ale->time[HSHAKE][START_EMIT]);
                ale_fxn->set_rx_bw(temp_ale->received_msg.bandwidth);
                ale_settings->phase=2;
			}
			break;
		case 2: // hshake tx
            if(temp_ale->pause_state)	// wait to start emit call
                ale_fxn->send_tx_msg(HSHAKE);
			else
			{
                ale_fxn->wait_end_tx(HSHAKE,RESP_CALL_QUAL,false,call_end_temp_ale[ale_settings->gps_en]);
				//	GOTO NEXT SUPERPHASE
                ale_fxn->set_next_superphase(2);
			}
			break;
		default:
            ale_fxn->ale_logic_error(0);
	}
}

bool AleMain::check_resp_call_qual(int8s* data)
{
	return true;	//	IGNORING CALL QUAL DATA
}

void AleMain::link_set_tx_mgr()
{
    switch(ale_settings->phase)
	{
		case 0:	// resp_call_qual_rx
            if(temp_ale->pause_state)	// wait to start emit call
                ale_fxn->start_receive_msg(RESP_CALL_QUAL);
			else
			{
                if(!ale_fxn->check_msg(RESP_CALL_QUAL,true))
				{
                    ale_settings->phase=0;
                    ale_settings->repeat_counter++;
                    if(ale_settings->repeat_counter>=MAX_TRANSMODE_REPEAT)
                        ale_fxn->return_to_call();
					else
                        timer->set_timer(TRANS_MODE_SUPERPHASE_TIME-temp_ale->time[RESP_CALL_QUAL][RECEIVE_PERIOD]);
					break;
				}
                if(!check_resp_call_qual(temp_ale->received_msg.data))
                    ale_fxn->return_to_call();
                timer->set_timer(temp_ale->time[RESP_CALL_QUAL][RECEIVE_LAST_TIME]+temp_ale->time[TRANS_MODE][START_EMIT]);
                ale_settings->phase=1;
			}
			break;
		case 1:	// trans_mode_tx
            if(temp_ale->pause_state)	// wait to start receive
                ale_fxn->send_tx_msg(TRANS_MODE);
			else
			{
                ale_fxn->wait_end_tx(TRANS_MODE,HSHAKE,false);
                ale_settings->phase=2;
			}
			break;
		case 2:	// hshake wait
            if(temp_ale->pause_state)	// wait to start receive
                ale_fxn->start_receive_msg(HSHAKE);
			else
			{
                if(!ale_fxn->check_msg(HSHAKE,true))
				{	
                    ale_settings->phase=0;
                    ale_settings->repeat_counter++;
                    if(ale_settings->repeat_counter>=MAX_TRANSMODE_REPEAT)
                        ale_fxn->return_to_call();
                    timer->set_timer(temp_ale->time[HSHAKE][RECEIVE_LAST_TIME]+
                                     temp_ale->time[HSHAKE][PHASE_TIME]+
                                     temp_ale->time[RESP_CALL_QUAL][START_RECEIVE]);
					break;
				}
                timer->set_timer(temp_ale->time[HSHAKE][RECEIVE_LAST_TIME]+temp_ale->time[HSHAKE][START_EMIT]);
                ale_settings->phase=3;
			}
			break;
		case 3: // hshake tx
            if(temp_ale->pause_state)	// wait to start emit call
                ale_fxn->send_tx_msg(HSHAKE);
			else
			{
				//	GOTO NEXT SUPERPHASE
                if(ale_settings->probe_on)
				{
                    ale_fxn->set_next_superphase(3);
                    if(!ale_fxn->get_work_freq())
                        ale_fxn->return_to_call();
					else
                    {
                        ale_fxn->wait_end_tx(HSHAKE,SHORT_SOUND,true);
                        ale_fxn->set_freq(temp_ale->work_probe_freq[0]);
					}
				}
				else
				{
                    temp_ale->freq_num_now=0;
                    ale_fxn->set_freq(temp_ale->best_freq[temp_ale->freq_num_now]);
                    ale_fxn->set_next_superphase(7);
                    ale_fxn->wait_end_tx(HSHAKE,MSG_HEAD,true,ALE_DATA_PAUSE);
				}
			}
			break;
		default:
            ale_fxn->ale_logic_error(0);
	}
}

bool AleMain::check_call_rx(int8s* data, int8s snr)
{
    if(snr<call_snr_lim[AleCom::get_call_cycle_num(data)])
		return false;
    if(ale_settings->own_adress!=AleCom::get_call_resp_addr(data))
		return false;
    return AleCom::get_call_line_type(data);
}

void AleMain::call_rx_mgr()
{
    switch(ale_settings->phase)
	{
		case 0:	// call tx
            if(temp_ale->pause_state)	// wait to start emit call
                ale_fxn->start_receive_msg(CALL_MANUAL+ale_settings->gps_en);
			else
			{
                if(!ale_fxn->check_msg(CALL_MANUAL+ale_settings->gps_en,true))
                    ale_fxn->return_to_call();
				//	OTHER LOGIC IS IN COM RECEIVER
			}
			break;
		case 1:	// hshake tx
            if(temp_ale->pause_state)	// wait to start emit call
                ale_fxn->send_tx_msg(HSHAKE);
			else
			{
                ale_fxn->wait_end_tx(HSHAKE,HSHAKE,false);
                ale_settings->phase=2;
			}
			break;
		case 2: // hshake wait			
            if(temp_ale->pause_state)	// wait to start receive
                ale_fxn->start_receive_msg(HSHAKE);
			else
			{
                if(!ale_fxn->check_msg(HSHAKE,true))
				{
                    ale_fxn->return_to_call();
					break;
				}
                timer->set_timer(temp_ale->time[HSHAKE][RECEIVE_LAST_TIME]+
                                 temp_ale->time[RESP_CALL_QUAL][START_EMIT]);
                temp_ale->pause_state=true;
				//	GOTO NEXT SUPERPHASE
                ale_fxn->set_next_superphase(2);
			}
			break;
		default:
            ale_fxn->ale_logic_error(0);
	}
}

bool AleMain::check_trans_mode(int8s* data)
{	
    if(AleCom::get_trans_mode_work_mode(data)!=3)
		return false;
    if(AleCom::get_trans_mode_param_mode(data)!=1)
		return false;
    if(AleCom::get_trans_mode_sound_type(data)==3)
        ale_settings->probe_on=true;
    else if(AleCom::get_trans_mode_sound_type(data)==0)
        ale_settings->probe_on=false;
	else
		return false;
    ale_settings->caller_adress=AleCom::get_trans_mode_caller(data);
	return true;
}

void AleMain::link_set_rx_mgr()
{
    switch(ale_settings->phase)
	{
		case 0:	// resp_call_qual_tx
            if(temp_ale->pause_state)	// wait to start emit call
                ale_fxn->send_tx_msg(RESP_CALL_QUAL);
			else
			{
                ale_fxn->wait_end_tx(RESP_CALL_QUAL,TRANS_MODE,false);
                ale_settings->phase=1;
			}
			break;
		case 1:	// trans_mode_tx
            if(temp_ale->pause_state)	// wait to start receive
                ale_fxn->start_receive_msg(TRANS_MODE);
			else
			{				
                if(!ale_fxn->check_msg(TRANS_MODE,true))
				{
                    ale_settings->phase=0;
                    ale_settings->repeat_counter++;
                    if(ale_settings->repeat_counter>MAX_TRANSMODE_REPEAT)
                        ale_fxn->return_to_call();
					else
                        timer->set_timer(temp_ale->time[TRANS_MODE][RECEIVE_LAST_TIME]+
                                         temp_ale->time[HSHAKE][PHASE_TIME]+
                                         temp_ale->time[HSHAKE][PHASE_TIME]+
                                         temp_ale->time[RESP_CALL_QUAL][START_EMIT]);
					break;
				}
                else if(!check_trans_mode(temp_ale->received_msg.data))
                    ale_fxn->return_to_call();
				else
				{
                    timer->set_timer(temp_ale->time[TRANS_MODE][RECEIVE_LAST_TIME]+
                                     temp_ale->time[HSHAKE][START_EMIT]);
                    ale_settings->phase=2;
				}
			}
			break;			
		case 2: // hshake tx
            if(temp_ale->pause_state)	// wait to start emit call
                ale_fxn->send_tx_msg(HSHAKE);
			else
			{
                ale_fxn->wait_end_tx(HSHAKE,HSHAKE,false);
                ale_settings->phase=3;
			}				
			break;
		case 3:	// hshake wait
            if(temp_ale->pause_state)	// wait to start receive
                ale_fxn->start_receive_msg(HSHAKE);
			else
			{
                if(!ale_fxn->check_msg(HSHAKE,true))
				{	
                    ale_settings->phase=0;
                    ale_settings->repeat_counter++;
                    if(ale_settings->repeat_counter>MAX_TRANSMODE_REPEAT)
                        ale_fxn->return_to_call();
					else
                        timer->set_timer(temp_ale->time[HSHAKE][RECEIVE_LAST_TIME]+temp_ale->time[RESP_CALL_QUAL][START_EMIT]);
					break;
				}
				//	GOTO NEXT SUPERPHASE
                if(ale_settings->probe_on)
				{
                    ale_fxn->set_next_superphase(3);
                    if(!ale_fxn->get_work_freq())
                        ale_fxn->return_to_call();
					else
                    {
                        ale_fxn->wait_end_tx(HSHAKE,SHORT_SOUND,true);
                        ale_fxn->set_freq(temp_ale->work_probe_freq[0]);
					}
				}
				else
				{
                    temp_ale->freq_num_now=0;
                    temp_ale->best_freq[0]=temp_ale->call_freq;
                    ale_fxn->set_next_superphase(7);
                    ale_fxn->wait_end_tx(HSHAKE,MSG_HEAD,true,ALE_DATA_PAUSE);
                    ale_fxn->set_freq(temp_ale->best_freq[temp_ale->freq_num_now]);
				}
			}
			break;
		default:
            ale_fxn->ale_logic_error(0);
	}
}

void AleMain::short_probe_tx_mgr()
{
    if(temp_ale->pause_state)
        ale_fxn->send_tx_msg(SHORT_SOUND);
	else
	{
        ale_settings->phase++;
        if(ale_settings->phase<temp_ale->work_probe_freq_num)
		{
            ale_fxn->wait_end_tx(SHORT_SOUND,SHORT_SOUND,true);
            ale_fxn->set_freq(temp_ale->work_probe_freq[ale_settings->phase]);
		}
		else
		{
            ale_fxn->wait_end_tx(SHORT_SOUND,SOUND_QUAL,false,189);
            ale_fxn->set_freq(temp_ale->call_freq);
            ale_settings->phase=0;
            ale_settings->superphase=4;
            ale_settings->repeat_counter=0;
		}
	}
}

void AleMain::short_sound_qual_tx_mgr()
{
    switch(ale_settings->phase)
	{
		case 0:
            if(temp_ale->pause_state==true)
                ale_fxn->start_receive_msg(SOUND_QUAL);
			else
			{
                ale_fxn->set_rx_mode(0);
                temp_ale->pause_state=true;
                if(!ale_fxn->check_msg(SOUND_QUAL, true))
				{	
                    ale_settings->repeat_counter++;
                    if(ale_settings->repeat_counter>=MAX_SHORT_PROBE_REPEAT)
                        ale_fxn->return_to_call();
                    timer->set_timer(temp_ale->time[SOUND_QUAL][RECEIVE_LAST_TIME]+temp_ale->time[HSHAKE][PHASE_TIME]+temp_ale->time[SOUND_QUAL][START_RECEIVE]);
					break;
				}
                //temp_ale->best_freq_num=get_sound_qual_freq_info(temp_ale->received_msg.data,temp_ale->best_freq_index,temp_ale->best_freq_sign_form);
                for(int8s i=0;i<temp_ale->best_freq_num;i++)
                    temp_ale->best_freq[i]=temp_ale->work_probe_freq[temp_ale->best_freq_index[i]];
                if(temp_ale->best_freq_num==0)
                    ale_fxn->return_to_call();
                timer->set_timer(temp_ale->time[SOUND_QUAL][RECEIVE_LAST_TIME]+temp_ale->time[HSHAKE][START_EMIT]);
                ale_settings->phase=1;
			}
			break;
		case 1:
            if(temp_ale->pause_state)
                ale_fxn->send_tx_msg(HSHAKE);
			else
			{
                ale_fxn->wait_end_tx(HSHAKE,MSG_HEAD,true,100);
                ale_fxn->set_freq(temp_ale->best_freq[0]);
                temp_ale->sign_form=temp_ale->best_freq_sign_form[0];
                temp_ale->freq_num_now=0;
                ale_settings->phase=0;
                ale_settings->superphase=7;		// GOTO MSG_HEAD
                ale_settings->repeat_counter=0;
			}
			break;
	}
}

void AleMain::short_probe_rx_mgr()
{
    if(temp_ale->pause_state)
        ale_fxn->start_receive_msg(SHORT_SOUND);
	else
	{
        ale_settings->phase++;
        if(!ale_fxn->check_msg(SHORT_SOUND, true))
        if(ale_settings->phase<temp_ale->work_probe_freq_num)
		{
            ale_fxn->wait_end_tx(SHORT_SOUND,SHORT_SOUND,true);
            ale_fxn->set_freq(temp_ale->work_probe_freq[ale_settings->phase]);
		}
		else
		{
            ale_fxn->wait_end_tx(SHORT_SOUND,SOUND_QUAL,false,189);
            ale_settings->phase=0;
            ale_settings->superphase=4;
            ale_settings->repeat_counter=0;
		}
	}
}


void AleMain::fxn_1pps(int h, int m, int s)
{
	int32s real_time_sec;
	real_time_sec=3600*h+60*m+s;
    if(ale_settings->call_freq_num==0)
        ale_settings->superphase=0;	//	TURN OFF IF NO CALL FREQ FOUND
    if(ale_settings->superphase==0)
		return;						//	RETURN IF ALE TURN OFF
    if(((real_time_sec % ale_call_superphase_time[ale_settings->gps_en])!=0)||((24*60*60-real_time_sec)<ale_call_superphase_time[ale_settings->gps_en]))
		return;						//	RETURN IF NO DWELL START DETECTED	
    if(ale_settings->caller)
	{
        ale_settings->call_counter++;
        if(ale_settings->call_counter==ale_settings->call_freq_num)
		{
            ale_settings->call_counter=0;
            if(ale_settings->call_supercounter!=127)			ale_settings->call_supercounter++;
		}
	}
    if(timer->get_timer_state())
		return;						//	RETURN IF TIMER TURNED ON (IT MEANS THAT ALE PROCESS CONTINUES)
    if((temp_ale->rx==1)||(temp_ale->tx==1))
	{
        ale_fxn->set_tx_mode(0);
        ale_fxn->set_rx_mode(0);
        ale_fxn->error_detect(1);
	}
    if((ale_settings->caller)&&(ale_settings->call_supercounter>=ale_max_supercounter[ale_settings->gps_en]))
	{
        ale_settings->superphase=0;
		return;
	}
    if(ale_settings->caller)		//	CALLER
        timer->start_timer(temp_ale->time[CALL_MANUAL+ale_settings->gps_en][START_EMIT]);
	else			//	RESPONDER
        timer->start_timer(temp_ale->time[CALL_MANUAL+ale_settings->gps_en][START_RECEIVE]);
    temp_ale->pause_state=true;
    ale_fxn->set_rx_bw(3);
	//	IF WE WORK IN CALL FREQ WE MUST WRITE ONLY THIS FREQ TO BEST WORK FREQ TABLE
    temp_ale->best_freq_num = 1;
    temp_ale->best_freq[0]=ale_settings->call_freq[(real_time_sec/ale_call_superphase_time[ale_settings->gps_en])%ale_settings->call_freq_num];
    temp_ale->best_freq_sign_form[0]=6;	// DEFAULT START
    temp_ale->call_freq=temp_ale->best_freq[0];
#ifdef	OLD_DSP_VERSION
    ale_fxn->set_caller_mode(ale_settings->caller);		//	ONLY FOR OLD DSP VERSIONS
#endif	
    ale_fxn->set_freq(ale_settings->call_freq[(real_time_sec/ale_call_superphase_time[ale_settings->gps_en])%ale_settings->call_freq_num]);
}

void AleMain::start_fxn(int8s gps_en, bool caller, int8s adress_dst, bool probe_on)
{
    ale_settings->gps_en=gps_en;
    ale_settings->caller=caller;
    ale_settings->adress_dst=adress_dst;
    ale_settings->probe_on=probe_on;
    ale_settings->call_counter=0;
    ale_settings->call_supercounter=0;
    ale_settings->phase=0;
    ale_settings->superphase=1;
    ale_settings->repeat_counter=0;
}

void AleMain::stop_fxn()
{
    timer->stop_timer();
    ale_settings->superphase=0;
    ale_fxn->set_tx_mode(0);
    ale_fxn->set_rx_mode(0);
}

void AleMain::modem_packet_receiver(int8s type, int8s snr, int8s error, int8s bandwidth, int8s* data, int8s data_length)
{
    temp_ale->received_msg.snr=snr; // FOR CORRECT SNR IN RESP PACK QUAL, IF PACKET DATA NOT RECEIVED
    if(temp_ale->rx==0)
		return;
    if(!((type==PACK_HEAD)&&(data_length==2)))
        ale_fxn->set_rx_mode(0);
	if(error==100)
		return;
	if((type==CALL_MANUAL)||(type==CALL_GPS))
	{
		if(!check_call_rx(data,snr))
            timer->stop_timer();
		else
		{
            timer->start_timer( temp_ale->time[CALL_MANUAL+ale_settings->gps_en][RECEIVE_LAST_TIME]+
                                temp_ale->time[HSHAKE][START_EMIT]);
            temp_ale->call_err=temp_ale->received_msg.error;
            temp_ale->call_snr=temp_ale->received_msg.snr;
            ale_fxn->set_rx_bw(bandwidth);
            temp_ale->pause_state=true;
            ale_settings->phase=1;
		}
	}
    temp_ale->received_msg.time = timer->get_timer_counter();
    temp_ale->received_msg.type=type;
    if(temp_ale->received_msg.type==3)		//	FOR OLD VERSION
        temp_ale->received_msg.type=HSHAKE;
    temp_ale->received_msg.snr=snr;
    temp_ale->received_msg.error=error;
    temp_ale->received_msg.bandwidth=bandwidth;
    temp_ale->received_msg.data_length=data_length;
	for(int i=0;i<data_length;i++)
        temp_ale->received_msg.data[i]=data[i];

}

void AleMain::modem_packet_transmitter_complete()
{
    ale_fxn->set_tx_mode(0);
}

}
