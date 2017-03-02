
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
                temp_ale->time[i][RECEIVE_PERIOD]=ideal_timings[i][IDEAL_EMIT_PERIOD]+2*DT_MANUAL+DSP_RX_TURN_ON_WAITING+DSP_LIGHT_MSG_RX_WAITING;
                temp_ale->time[i][RECEIVE_LAST_TIME]=ideal_timings[i][IDEAL_PHASE_TIME]-ideal_timings[i][IDEAL_START_EMIT]-ideal_timings[i][IDEAL_EMIT_PERIOD]-CALL_DSP_TIME;
			}
			else if(i==CALL_GPS)
			{
                temp_ale->time[i][START_RECEIVE]=ideal_timings[i][IDEAL_START_EMIT]-DT_GPS-DSP_RX_TURN_ON_WAITING;
                temp_ale->time[i][RECEIVE_PERIOD]=ideal_timings[i][IDEAL_EMIT_PERIOD]+2*DT_GPS+DSP_RX_TURN_ON_WAITING+DSP_LIGHT_MSG_RX_WAITING;
                temp_ale->time[i][RECEIVE_LAST_TIME]=ideal_timings[i][IDEAL_PHASE_TIME]-ideal_timings[i][IDEAL_START_EMIT]-ideal_timings[i][IDEAL_EMIT_PERIOD]-CALL_DSP_TIME;
			}
			else
			{
                temp_ale->time[i][START_RECEIVE]=ideal_timings[i][IDEAL_START_EMIT]-DT_ALE-DSP_RX_TURN_ON_WAITING;
                temp_ale->time[i][RECEIVE_PERIOD]=ideal_timings[i][IDEAL_EMIT_PERIOD]+2*DT_ALE+DSP_RX_TURN_ON_WAITING+DSP_LIGHT_MSG_RX_WAITING;
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
            temp_ale->time[i][RECEIVE_PERIOD]=ideal_timings[i][IDEAL_EMIT_PERIOD]+2*DT_ALE+DSP_RX_TURN_ON_WAITING+DSP_LIGHT_MSG_RX_WAITING;
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
#ifndef		CALLER_DT_INCREMENT
                ale_fxn->wait_end_tx(CALL_MANUAL+ale_settings->gps_en,HSHAKE,false);
#else
                ale_fxn->wait_end_tx(CALL_MANUAL+ale_settings->gps_en,HSHAKE,false,CALLER_DT_INCREMENT);
#endif
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
#ifndef CALLER_IGNORE_RX
                    ale_fxn->return_to_call();
					break;
#endif
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
                ale_fxn->wait_end_tx(HSHAKE,RESP_CALL_QUAL,false,call_end_time[ale_settings->gps_en]);
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
#ifndef CALLER_IGNORE_RX
                    ale_settings->phase=0;
                    ale_settings->neg_counter++;
                    if(ale_settings->neg_counter>=MAX_TRANSMODE_REPEAT)
                        ale_fxn->return_to_call();
					else
                        timer->set_timer(TRANS_MODE_SUPERPHASE_TIME-temp_ale->time[RESP_CALL_QUAL][RECEIVE_PERIOD]);
					break;
#endif
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
#ifndef CALLER_IGNORE_RX
                    ale_settings->phase=0;
                    ale_settings->neg_counter++;
                    if(ale_settings->neg_counter>=MAX_TRANSMODE_REPEAT)
                        ale_fxn->return_to_call();
                    timer->set_timer(temp_ale->time[HSHAKE][RECEIVE_LAST_TIME]+
                                     temp_ale->time[HSHAKE][PHASE_TIME]+
                                     temp_ale->time[RESP_CALL_QUAL][START_RECEIVE]);
					break;
#endif
				}
                timer->set_timer(temp_ale->time[HSHAKE][RECEIVE_LAST_TIME]+
                                 temp_ale->time[HSHAKE][START_EMIT]);
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
                        ale_fxn->set_freq(temp_ale->work_freq[0]);
					}
				}
				else
				{
                    temp_ale->freq_num_now=0;
                    temp_ale->sign_form=temp_ale->best_freq_sign_form[0];
                    ale_fxn->set_next_superphase(7);
                    ale_fxn->wait_end_tx(HSHAKE,MSG_HEAD,true,MSG_HEAD_START_TIME);
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
                                 temp_ale->time[RESP_CALL_QUAL][START_EMIT]+
								 call_end_time[ale_settings->gps_en]);
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
                    ale_settings->neg_counter++;
                    if(ale_settings->neg_counter>=MAX_TRANSMODE_REPEAT)
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
                    ale_settings->neg_counter++;
                    if(ale_settings->neg_counter>=MAX_TRANSMODE_REPEAT)
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
						timer->set_timer(temp_ale->time[HSHAKE][RECEIVE_LAST_TIME]+temp_ale->time[SHORT_SOUND][START_EMIT]);
                        //ale_fxn->wait_end_tx(HSHAKE,SHORT_SOUND,true);
                        ale_fxn->set_freq(temp_ale->work_freq[0]);
					}
				}
				else
				{
                    temp_ale->freq_num_now=0;
                    temp_ale->best_freq[0]=temp_ale->call_freq;
                    ale_fxn->set_next_superphase(7);
                    timer->set_timer(temp_ale->time[HSHAKE][RECEIVE_LAST_TIME]+temp_ale->time[MSG_HEAD][START_EMIT]+MSG_HEAD_START_TIME);
                    //ale_fxn->wait_end_tx(HSHAKE,MSG_HEAD,false,MSG_HEAD_START_TIME);
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
        if(ale_settings->phase<temp_ale->work_freq_num)
		{
            ale_fxn->wait_end_tx(SHORT_SOUND,SHORT_SOUND,true);
            ale_fxn->set_freq(temp_ale->work_freq[ale_settings->phase]);
		}
		else
		{
            ale_fxn->wait_end_tx(SHORT_SOUND,SOUND_QUAL,false,SOUND_QUAL_START_TIME);
            ale_fxn->set_freq(temp_ale->call_freq);
            ale_fxn->set_next_superphase(4);
		}
	}
}

void AleMain::calc_sign_forms()
{
    temp_ale->best_freq_num=0;
    for(int8s i=0;i<3;i++)
    {
        temp_ale->best_freq_num++;
        if((temp_ale->best_freq_sign_form[i]&0x0F)==0x0F)
            temp_ale->best_freq_sign_form[i]=0;
        else if((temp_ale->best_freq_sign_form[i]&0x0E)==0x0E)
            temp_ale->best_freq_sign_form[i]=1;
        else if((temp_ale->best_freq_sign_form[i]&0x04)==0x04)
            temp_ale->best_freq_sign_form[i]=2;
        else if((temp_ale->best_freq_sign_form[i]&0x08)==0x08)
            temp_ale->best_freq_sign_form[i]=3;
        else if((temp_ale->best_freq_sign_form[i]&0x70)==0x70)
            temp_ale->best_freq_sign_form[i]=4;
        else if((temp_ale->best_freq_sign_form[i]&0x60)==0x60)
            temp_ale->best_freq_sign_form[i]=5;
        else if((temp_ale->best_freq_sign_form[i]&0x40)==0x40)
            temp_ale->best_freq_sign_form[i]=6;
        else if(temp_ale->best_freq_sign_form[i]==0)
            temp_ale->best_freq_num--;
        else
            temp_ale->best_freq_sign_form[i]=7;
    }
}

void AleMain::short_sound_qual_tx_mgr()
{
    int16s crc16;
    switch(ale_settings->phase)
	{
		case 0:
            if(temp_ale->pause_state==true)
                ale_fxn->start_receive_msg(SOUND_QUAL);
			else
            {
                crc16=AleCom::get_sound_qual_crc16(temp_ale->received_msg.data);
#ifndef  NO_CRC16_CHECK
                if((!ale_fxn->check_msg(SOUND_QUAL, true))||(crc16!=ale_fxn->CRC16(temp_ale->received_msg.data,7)))
#else
                if(!ale_fxn->check_msg(SOUND_QUAL, true))
#endif
                {
                    ale_settings->neg_counter++;
                    if(ale_settings->neg_counter>=MAX_SHORT_PROBE_REPEAT)
                        ale_fxn->return_to_call();
                    timer->set_timer(temp_ale->time[SOUND_QUAL][RECEIVE_LAST_TIME]+
                                     temp_ale->time[HSHAKE][PHASE_TIME]+
                                     temp_ale->time[SOUND_QUAL][START_RECEIVE]);
					break;
                }
                temp_ale->best_freq_num=AleCom::get_sound_qual_freq_info(temp_ale->received_msg.data,temp_ale->best_freq_index,temp_ale->best_freq_sign_form);
                calc_sign_forms();
                for(int8s i=0;i<temp_ale->best_freq_num;i++)
                    temp_ale->best_freq[i]=temp_ale->work_freq[temp_ale->best_freq_index[i]];
                if(temp_ale->best_freq_num==0)
                    ale_fxn->return_to_call();
                timer->set_timer(temp_ale->time[SOUND_QUAL][RECEIVE_LAST_TIME]+
                                 temp_ale->time[HSHAKE][START_EMIT]);
                ale_settings->phase=1;
			}
			break;
		case 1:
            if(temp_ale->pause_state)
                ale_fxn->send_tx_msg(HSHAKE);
			else
			{
                ale_fxn->wait_end_tx(HSHAKE,MSG_HEAD,true,MSG_HEAD_START_TIME);
                ale_fxn->set_freq(temp_ale->best_freq[0]);
                temp_ale->sign_form=temp_ale->best_freq_sign_form[0];
                temp_ale->freq_num_now=0;
                ale_fxn->set_next_superphase(7);        //  NO LONG PROBES
			}
			break;
	}
}

void AleMain::short_probe_rx_mgr()
{
    int32s temp[32];
    if(temp_ale->pause_state)
        ale_fxn->start_receive_msg(SHORT_SOUND);
	else
	{
        if(!ale_fxn->check_msg(SHORT_SOUND, true))
        {
            temp_ale->received_msg.snr=0;
            temp_ale->received_msg.error=0;
        }
        temp_ale->work_freq_sign_forms[ale_settings->phase]=temp_ale->received_msg.error;
        temp_ale->work_freq_snr[ale_settings->phase]=temp_ale->received_msg.snr;
        temp_ale->work_freq_weights[ale_settings->phase]=temp_ale->received_msg.snr;
        temp_ale->work_freq_index[ale_settings->phase]=ale_settings->phase;
        for(int8s i=0;i<7;i++)     //  SEE WEIGHT DESCRIPTION IN STRUCT COMMENTS
            temp_ale->work_freq_weights[ale_settings->phase]=temp_ale->work_freq_weights[ale_settings->phase]+
                ((int32s)((temp_ale->received_msg.error>>i)&1))*(65536+(1<<(6-((int32s)i))*256));
        ale_settings->phase++;
        if(ale_settings->phase<temp_ale->work_freq_num)
		{
            ale_fxn->wait_end_tx(SHORT_SOUND,SHORT_SOUND,false);
            ale_fxn->set_freq(temp_ale->work_freq[ale_settings->phase]);
		}
		else
		{
            ale_fxn->wait_end_tx(SHORT_SOUND,SOUND_QUAL,true,SOUND_QUAL_START_TIME);            
            ale_fxn->set_freq(temp_ale->call_freq);
            ale_fxn->set_next_superphase(4);
            //  CALC FREQ PARAMS
            for(int8s i=0;i<temp_ale->work_freq_num;i++)
                temp_ale->work_freq_index[i]=i;
            sort_int32s_data_and_index(temp_ale->work_freq_weights, temp_ale->work_freq_index, temp_ale->work_freq_num);
            for(int8s i=0;i<temp_ale->work_freq_num;i++)
                temp[i]=temp_ale->work_freq_sign_forms[i];
            for(int8s i=0;i<temp_ale->work_freq_num;i++)
                temp_ale->work_freq_sign_forms[i]=temp[temp_ale->work_freq_index[i]];
            for(int8s i=0;i<temp_ale->work_freq_num;i++)
                temp[i]=temp_ale->work_freq_snr[i];
            for(int8s i=0;i<temp_ale->work_freq_num;i++)
                temp_ale->work_freq_snr[i]=temp[temp_ale->work_freq_index[i]];
            for(int8s i=0;i<temp_ale->work_freq_num;i++)
                temp[i]=temp_ale->work_freq[i];
            for(int8s i=0;i<temp_ale->work_freq_num;i++)
                temp_ale->work_freq[i]=temp[temp_ale->work_freq_index[i]];
            int8s freq_counter=0;
            for(int8s i=0;i<temp_ale->work_freq_num;i++)
            {
                if((temp_ale->work_freq_snr[0]-temp_ale->work_freq_snr[i])>20)
                    temp_ale->work_freq_sign_forms[i]=0;
                if(temp_ale->work_freq_sign_forms[i]!=0)
                    freq_counter++;
            }
            if(freq_counter<2)
                return;
            for(int8s i=0;i<temp_ale->work_freq_num;i++)
            {
                if(temp_ale->work_freq_snr[0]<15)
                    temp_ale->work_freq_sign_forms[i]=0;
            }
            freq_counter=0;
            for(int8s i=0;i<3;i++)
            {
                for(int8s j=i;j<temp_ale->work_freq_num;j++)
                {
                    if(temp_ale->work_freq_sign_forms[j]!=0)
                    {
                        //  REPLACE FREQ DATA
                        temp_ale->work_freq_sign_forms[i]=temp_ale->work_freq_sign_forms[j];
                        temp_ale->work_freq_snr[i]=temp_ale->work_freq_snr[j];
                        temp_ale->work_freq_index[i]=temp_ale->work_freq_index[j];
                        temp_ale->work_freq[i]=temp_ale->work_freq[j];
                        temp_ale->work_freq_weights[i]=temp_ale->work_freq_weights[j];
                        //  COPY BEST FREQ DATA
                        temp_ale->best_freq[i]=temp_ale->work_freq[i];
                        temp_ale->best_freq_sign_form[i]=temp_ale->work_freq_sign_forms[i];
                        temp_ale->best_freq_index[i]=temp_ale->work_freq_index[i];
                        temp_ale->best_freq_snr[i]=temp_ale->work_freq_snr[i];
                        if(i!=j)
                            temp_ale->work_freq_sign_forms[j]=0;
                        break;
                    }
                }
                if(temp_ale->work_freq_sign_forms[i]!=0)
                    freq_counter++;
            }
            temp_ale->work_freq_num=freq_counter;
            temp_ale->best_freq_num=temp_ale->work_freq_num;
		}
	}
}

void AleMain::short_sound_qual_rx_mgr()
{
    switch(ale_settings->phase)
    {
        case 0:
            if(temp_ale->pause_state)
                ale_fxn->send_tx_msg(SOUND_QUAL);
            else
            {
                ale_fxn->wait_end_tx(SOUND_QUAL,HSHAKE,false);
                ale_settings->phase++;
            }
            break;
        case 1:
            if(temp_ale->pause_state==true)
                ale_fxn->start_receive_msg(HSHAKE);
            else
            {
                if(!ale_fxn->check_msg(HSHAKE, true))
                {
                    ale_settings->neg_counter++;
                    if(ale_settings->neg_counter>=MAX_SHORT_PROBE_REPEAT)
                        ale_fxn->return_to_call();
                    timer->set_timer(temp_ale->time[HSHAKE][RECEIVE_LAST_TIME]+
                                     temp_ale->time[SOUND_QUAL][PHASE_TIME]+
                                     temp_ale->time[HSHAKE][START_RECEIVE]);
                    ale_settings->phase=0;
                    break;
                }
                timer->set_timer(temp_ale->time[SOUND_QUAL][RECEIVE_LAST_TIME]+MSG_HEAD_START_TIME+
                                 temp_ale->time[MSG_HEAD][START_RECEIVE]);
                calc_sign_forms();
                ale_fxn->set_freq(temp_ale->best_freq[0]);
                temp_ale->sign_form=temp_ale->best_freq_sign_form[0];
                temp_ale->freq_num_now=0;
                ale_fxn->set_next_superphase(7);        //  NO LONG PROBES
            }
            break;
    }
}

void AleMain::fxn_1pps(int h, int m, int s)
{
	int32s real_time_sec;
	real_time_sec=3600*h+60*m+s;
    //ale_fxn->ale_log("1 PPS, time %u : %u : %u",h,m,s);
    if(ale_settings->call_freq_num==0)
        ale_settings->superphase=0;	//	TURN OFF IF NO CALL FREQ FOUND
    if(ale_settings->superphase==0)
		return;						//	RETURN IF ALE TURN OFF
    if(((real_time_sec % call_dwell_time[ale_settings->gps_en])!=0)||((24*60*60-real_time_sec)<call_dwell_time[ale_settings->gps_en]))
		return;						//	RETURN IF NO DWELL START DETECTED	
    if(ale_settings->caller)
	{
        ale_settings->call_counter++;
        if(ale_settings->call_counter==ale_settings->call_freq_num)
		{
            ale_settings->call_counter=0;
            if(ale_settings->call_supercounter!=127)
                ale_settings->call_supercounter++;
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
        ale_fxn->ale_log("TX call limit reached, end TX");
		return;
	}
    if(ale_settings->caller)		//	CALLER
    {
        timer->start_timer(temp_ale->time[CALL_MANUAL+ale_settings->gps_en][START_EMIT]);
        ale_fxn->ale_log("TX call emit, counter %u, supercounter %u", ale_settings->call_counter, ale_settings->call_supercounter);
    }
	else			//	RESPONDER
        timer->start_timer(temp_ale->time[CALL_MANUAL+ale_settings->gps_en][START_RECEIVE]);
    temp_ale->pause_state=true;
    ale_fxn->set_rx_bw(3);
	//	IF WE WORK IN CALL FREQ WE MUST WRITE ONLY THIS FREQ TO BEST WORK FREQ TABLE
    temp_ale->best_freq_num = 1;
    temp_ale->best_freq[0]=ale_settings->call_freq[(real_time_sec/call_dwell_time[ale_settings->gps_en])%ale_settings->call_freq_num];
#ifndef	START_SIGN_FORM
    temp_ale->best_freq_sign_form[0]=6;	// DEFAULT START
#else
    temp_ale->best_freq_sign_form[0]=START_SIGN_FORM;	// DEFAULT START
#endif
    temp_ale->call_freq=temp_ale->best_freq[0];
#ifdef	OLD_DSP_VERSION
    ale_fxn->set_caller_mode(ale_settings->caller);		//	ONLY FOR OLD DSP VERSIONS
#endif	
    ale_fxn->set_freq(ale_settings->call_freq[(real_time_sec/call_dwell_time[ale_settings->gps_en])%ale_settings->call_freq_num]);
}

void AleMain::start_fxn(int8s gps_en, bool caller, int8s adress_dst, bool probe_on)
{
    ale_settings->gps_en=gps_en;
    ale_settings->caller=caller;
    ale_settings->adress_dst=adress_dst;
    ale_settings->probe_on=probe_on;
    ale_settings->call_counter=0;
    ale_settings->call_supercounter=0;
    //ale_settings->phase=0;
    //ale_settings->superphase=1;
    //ale_settings->neg_counter=0;
    ale_fxn->set_next_superphase(1);
}

void AleMain::stop_fxn()
{
    timer->stop_timer();
    //ale_settings->superphase=0;
    ale_fxn->set_next_superphase(0);
    ale_fxn->set_tx_mode(0);
    ale_fxn->set_rx_mode(0);
}

extern const char* msg_names[];

void AleMain::modem_packet_receiver(int8s type, int8s snr, int8s error, int8s bandwidth, int8s* data, int8s data_length)
{
    temp_ale->received_msg.snr=snr; // FOR CORRECT SNR IN RESP PACK QUAL, IF PACKET DATA NOT RECEIVED
    if(temp_ale->rx==0)
		return;
    if(error==100)
        return;
#ifndef	NOT_TURN_OFF_RX_WHEN_MSG_RECEIVED
    if(!((type==PACK_HEAD)&&(data_length==2)))
        ale_fxn->set_rx_mode(0);
#endif
    if((type==CALL_MANUAL)||(type==CALL_GPS))
	{
		if(!check_call_rx(data,snr))
        {
            timer->stop_timer();
            return;
        }
        timer->start_timer( temp_ale->time[CALL_MANUAL+ale_settings->gps_en][RECEIVE_LAST_TIME]+
                            temp_ale->time[HSHAKE][START_EMIT]);
        temp_ale->call_err=temp_ale->received_msg.error;
        temp_ale->call_snr=temp_ale->received_msg.snr;
        ale_fxn->set_rx_bw(bandwidth);
        temp_ale->pause_state=true;
        ale_settings->phase=1;
#ifdef	NOT_TURN_OFF_RX_WHEN_MSG_RECEIVED
        ale_fxn->set_rx_mode(0);
#endif
	}
    temp_ale->received_msg.time = timer->get_timer_counter();
    temp_ale->received_msg.type=type;
    if(temp_ale->received_msg.type==3)		//	FOR OLD VERSION
        temp_ale->received_msg.type=HSHAKE;
    temp_ale->received_msg.snr=snr;
    temp_ale->received_msg.error=error;
    temp_ale->received_msg.bandwidth=bandwidth;
    temp_ale->received_msg.data_length=data_length;
    for(int8s i=0;i<data_length;i++)
        temp_ale->received_msg.data[i]=data[i];
    if((type!=RESP_PACK_QUAL)&&(type!=PACK_HEAD))
    	ale_fxn->ale_log("Received msg, type %s, SNR %u, ERROR_RATE %u", msg_names[type],snr, error);
}

void AleMain::modem_packet_transmitter_complete()
{
    ale_fxn->set_tx_mode(0);
}

}
