
#include "ale_data_transport.h"
//#include "ale_settings.h"

namespace Multiradio {

AleDataTransport::AleDataTransport(ContTimer *tmr, ale_data* tmp_ale, ext_ale_settings* ale_s, AleFxn *ale_f)
{
    temp_ale=tmp_ale;
    ale_settings=ale_s;
    timer=tmr;
    ale_fxn=ale_f;
}

void AleDataTransport::msg_head_tx_mgr()
{
	switch(ale_settings->phase)
	{
		case 0:	// msg head tx
			if(temp_ale->pause_state)	// wait to start emit call
                ale_fxn->send_tx_msg(MSG_HEAD);
			else
			{
                ale_fxn->wait_end_tx(MSG_HEAD,RESP_PACK_QUAL,false);
				ale_settings->phase=1;
				ale_fxn->ale_log("Number of data blocks: %u, bytes: %u", ale_settings->data490bit_length, ale_settings->data72bit_length);
			}
			break;
		case 1:	// resp pack qual wait
			if(temp_ale->pause_state)	// wait to start receive
                ale_fxn->start_receive_msg(RESP_PACK_QUAL);
			else
			{
                if(!ale_fxn->check_msg(RESP_PACK_QUAL,true))
                {
#ifndef CALLER_IGNORE_RX
					ale_settings->phase--;
                    ale_settings->neg_counter++;
                    timer->set_timer(temp_ale->time[RESP_PACK_QUAL][RECEIVE_LAST_TIME]+
                                     temp_ale->time[HSHAKE][PHASE_TIME]+
                                     temp_ale->time[MSG_HEAD][START_EMIT]);
                    if(ale_settings->neg_counter>MAX_MSG_HEAD_REPEAT)
					{
                        ale_settings->neg_counter=0;
						if(temp_ale->freq_num_now==(temp_ale->best_freq_num-1))
                            ale_fxn->return_to_call();
						else
						{
							temp_ale->freq_num_now++;
							temp_ale->sign_form=temp_ale->best_freq_sign_form[temp_ale->freq_num_now];
                            ale_fxn->set_freq(temp_ale->best_freq[temp_ale->freq_num_now]);
						}
					}
                    ale_fxn->ale_log("Msg resp pack qual NOT received");
					break;
#endif
				}				
                timer->set_timer(temp_ale->time[RESP_PACK_QUAL][RECEIVE_LAST_TIME]+
                                 temp_ale->time[HSHAKE][START_EMIT]);
				ale_settings->phase=2;
                ale_fxn->ale_log("Msg resp pack qual received OK");
			}
			break;
		case 2: // hshake tx
			if(temp_ale->pause_state)	// wait to start emit call
                ale_fxn->send_tx_msg(HSHAKE);
			else
			{
                ale_fxn->wait_end_tx(HSHAKE,PACK_HEAD,true);
                ale_fxn->set_next_superphase(9);
			}
			break;
		default:
            ale_fxn->ale_logic_error(0);
	}
}

void AleDataTransport::data_tx_mgr()
{
	switch((ale_settings->phase+3)%3)
	{
		case 0:
			if(temp_ale->pause_state)
			{
#ifdef	MIN_SIGN_FORM_TX
				if(temp_ale->sign_form<MIN_SIGN_FORM_TX)
					temp_ale->sign_form=MIN_SIGN_FORM_TX;
#endif
				ale_fxn->send_tx_msg(PACK_HEAD);
			}
			else
			{
                ale_fxn->wait_end_tx(PACK_HEAD,RESP_PACK_QUAL,false);
                //ale_fxn->ale_log("Sending packet, phase %u, sign form %u", ale_settings->phase, temp_ale->sign_form);
				ale_settings->phase++;
			}	
			break;
		case 1:
			if(temp_ale->pause_state)	// wait to start receive
			{
                if((ale_settings->phase/3)!=(ale_fxn->get_packet_num()-1))
				{
                    ale_fxn->start_receive_msg(RESP_PACK_QUAL);
                    temp_ale->last_msg=true;    //  FOR CHECK ONLY PACK_QUAL
				}
				else
                    ale_fxn->start_receive_msg(RESP_PACK_QUAL_LINK_RELEASE);	// LINK RELEASE WAIT
			}
			else
			{
				if(!temp_ale->last_msg)
                {
                    if(!ale_fxn->check_msg(LINK_RELEASE, false))	// NO LINK_RELEASE
                        timer->set_timer(temp_ale->time[RESP_PACK_QUAL][RECEIVE_PERIOD]-
                                         temp_ale->time[LINK_RELEASE][RECEIVE_PERIOD]);
					else
					{
                        ale_fxn->set_next_superphase(10);
                        ale_fxn->set_rx_mode(0);
                        temp_ale->pause_state=true;
                        timer->set_timer(temp_ale->time[LINK_RELEASE][RECEIVE_LAST_TIME]+temp_ale->time[LINK_RELEASE][START_EMIT]);
					}
				}
				else
                {
#ifdef CALLER_IGNORE_RX
                    temp_ale->received_msg.data_length=68;
                    ale_fxn->set_rx_mode(0);
                    temp_ale->received_msg.type=PACK_HEAD;
                    temp_ale->received_msg.error=0;
                    temp_ale->received_msg.snr=12;   //
#endif
                    //	IF NO PACK QUAL RECEIVED
                    if(!ale_fxn->check_msg(RESP_PACK_QUAL, true))	// NO RESP PACK QUAL
					{	
                        ale_settings->phase--;	ale_settings->neg_counter++;		ale_settings->nres1=0;
#ifndef	TX_DATA_SUPERPHASE_INC_TIME
                        timer->set_timer(temp_ale->time[RESP_PACK_QUAL][RECEIVE_LAST_TIME]+
                                         temp_ale->time[HSHAKE][PHASE_TIME]+
                                         temp_ale->time[PACK_HEAD][START_EMIT]);
#else
                        timer->set_timer(temp_ale->time[RESP_PACK_QUAL][RECEIVE_LAST_TIME]+
                                         temp_ale->time[HSHAKE][PHASE_TIME]+
                                         temp_ale->time[PACK_HEAD][START_EMIT]+TX_DATA_SUPERPHASE_INC_TIME);
#endif
                        if(ale_settings->neg_counter>=MAX_PACK_HEAD_REPEAT)
						{
							ale_settings->nres0=0;
                            ale_settings->neg_counter=0;
							if(temp_ale->freq_num_now==(temp_ale->best_freq_num-1))	//	IF NONE FREQ FOR FREQ ADAPT
                                ale_fxn->return_to_call();
							else
							{
								temp_ale->freq_num_now++;
								temp_ale->sign_form=temp_ale->best_freq_sign_form[temp_ale->freq_num_now];
                                ale_fxn->set_freq(temp_ale->best_freq[temp_ale->freq_num_now]);
							}
						}
                        ale_fxn->ale_log("No pack qual, phase %u", ale_settings->phase);
					}
					// IF RESP PACK QUAL RESULT = 0
                    else if(!AleCom::check_resp_pack_qual(temp_ale->received_msg.data))
					{
						ale_settings->phase=ale_settings->phase-2;
						ale_settings->nres0++;	ale_settings->nres1=0;
                        ale_settings->last_data_snr[ale_settings->nres0-1]=AleCom::get_resp_pack_qual_snr(temp_ale->received_msg.data);
                        timer->set_timer(temp_ale->time[RESP_PACK_QUAL][RECEIVE_LAST_TIME]+
                                         temp_ale->time[HSHAKE][START_EMIT]);
                        if((ale_settings->last_data_snr[ale_settings->nres0-1]<pack_head_lim_snr[DATA_SIGNAL_FORM_NUM-1])&&(temp_ale->best_freq_num!=(temp_ale->freq_num_now+1)))
						{
                            ale_settings->nres0=0;	ale_settings->neg_counter=0;
							temp_ale->freq_num_now++;
							temp_ale->sign_form=temp_ale->best_freq_sign_form[temp_ale->freq_num_now];
						}
						else if(ale_settings->nres0>=MAX_PACK_HEAD_NRES0)
						{
                            ale_settings->nres0=0;	ale_settings->neg_counter=0;
							if(temp_ale->sign_form==(DATA_SIGNAL_FORM_NUM-1))
							{
								if(temp_ale->best_freq_num==(temp_ale->freq_num_now+1))
                                    ale_fxn->return_to_call();
								else
								{
									temp_ale->freq_num_now++;
									temp_ale->sign_form=temp_ale->best_freq_sign_form[temp_ale->freq_num_now];
								}
							}
							else
							{
                                temp_ale->snr=ale_fxn->get_min_value(ale_settings->last_data_snr,MAX_PACK_HEAD_NRES0);
								temp_ale->sign_form++;
								for(temp_ale->sign_form=temp_ale->sign_form;temp_ale->sign_form<DATA_SIGNAL_FORM_NUM;temp_ale->sign_form++)
								{
									if(temp_ale->snr>=pack_head_lim_snr[temp_ale->sign_form])
										break;
								}
							}
						}
                        ale_fxn->ale_log("Bad pack, phase %u, snr %u", ale_settings->phase, AleCom::get_resp_pack_qual_snr(temp_ale->received_msg.data));
					}
					//	IF RESULT OK
					else																			
					{
						ale_settings->phase++;
                        ale_settings->nres0=0;	ale_settings->neg_counter=0;		ale_settings->nres1++;
                        ale_settings->last_data_snr[ale_settings->nres1-1]=AleCom::get_resp_pack_qual_snr(temp_ale->received_msg.data);
						if(ale_settings->nres1>=MAX_PACK_HEAD_NRES1)
						{
                            temp_ale->snr=ale_fxn->get_min_value(ale_settings->last_data_snr,MAX_PACK_HEAD_NRES1);
							if(temp_ale->snr<=pack_head_lim_snr[temp_ale->sign_form])		//	CANNOT SPPED SHIFT
							{
								for(int i=0;i<(MAX_PACK_HEAD_NRES1-1);i++)
									ale_settings->last_data_snr[i]=ale_settings->last_data_snr[i+1];
								ale_settings->nres1--;
							}
							else
							{
								ale_settings->nres1=0;
								for(temp_ale->sign_form=0;temp_ale->sign_form<DATA_SIGNAL_FORM_NUM;temp_ale->sign_form++)
								{
									if(temp_ale->snr>=pack_head_lim_snr[temp_ale->sign_form])
										break;
								}
                                if(temp_ale->sign_form==2)		temp_ale->sign_form=3;
							}							
						}
                        timer->set_timer(temp_ale->time[RESP_PACK_QUAL][RECEIVE_LAST_TIME]+
                                         temp_ale->time[HSHAKE][START_EMIT]);
                        ale_fxn->ale_log("Good pack, phase %u, snr %u", ale_settings->phase, AleCom::get_resp_pack_qual_snr(temp_ale->received_msg.data));
					}	
				}		
			}
			break;
		case 2:
			if(temp_ale->pause_state)	// wait to start emit call
                ale_fxn->send_tx_msg(HSHAKE);
			else
			{
#ifndef	TX_DATA_SUPERPHASE_INC_TIME
                ale_fxn->wait_end_tx(HSHAKE,PACK_HEAD,true);
#else
                ale_fxn->wait_end_tx(HSHAKE,PACK_HEAD,true,TX_DATA_SUPERPHASE_INC_TIME);
#endif
				ale_settings->phase++;
                ale_settings->neg_counter=0;
                ale_fxn->set_freq(temp_ale->best_freq[temp_ale->freq_num_now]);
			}
			break;
	}
}

void AleDataTransport::data_end_tx_mgr()
{
	if(temp_ale->pause_state)
        ale_fxn->send_tx_msg(LINK_RELEASE);
	else
	{
        ale_fxn->wait_end_tx(LINK_RELEASE,LINK_RELEASE,false);
        ale_fxn->set_next_superphase(0);
		ale_settings->result=0;
		//	THE END OF TX CALLER
	}
}


void AleDataTransport::msg_head_rx_mgr()
{
	switch(ale_settings->phase)
	{
		case 0:
			if(temp_ale->pause_state)
            {
                if(ale_settings->neg_counter>0)
                    ale_fxn->start_receive_msg(MSG_HEAD_PACK_HEAD);
                else
                    ale_fxn->start_receive_msg(MSG_HEAD);
            }
			else
            {
                if(temp_ale->received_msg.data_length==-1)   //  RECEIVED NOTHING
                {
                    ale_settings->phase=0;
                    ale_settings->neg_counter++;
                    if(ale_settings->neg_counter>MAX_MSG_HEAD_REPEAT)
                    {
                        ale_settings->neg_counter=0;
                        if(temp_ale->freq_num_now==(temp_ale->best_freq_num-1))
                            ale_fxn->return_to_call();
                        else
                        {
                            timer->set_timer(temp_ale->time[MSG_HEAD][RECEIVE_LAST_TIME]+
                                             temp_ale->time[RESP_PACK_QUAL][PHASE_TIME]+
                                             temp_ale->time[HSHAKE][PHASE_TIME]+
                                             temp_ale->time[MSG_HEAD][START_RECEIVE]);
                            temp_ale->freq_num_now++;
                            temp_ale->sign_form=temp_ale->best_freq_sign_form[temp_ale->freq_num_now];
                            ale_fxn->set_freq(temp_ale->best_freq[temp_ale->freq_num_now]);
                        }
                    }
                    else
                        timer->set_timer(temp_ale->time[MSG_HEAD][RECEIVE_LAST_TIME]+
                                         temp_ale->time[RESP_PACK_QUAL][PHASE_TIME]+
                                         temp_ale->time[HSHAKE][PHASE_TIME]+
                                         temp_ale->time[MSG_HEAD][START_RECEIVE]);
                    ale_fxn->set_rx_mode(0);
                    temp_ale->pause_state=true;
                    break;
                }
                else if(temp_ale->received_msg.type==MSG_HEAD)
                {
                    ale_fxn->set_rx_mode(0);
                    temp_ale->pause_state=true;
                    ale_settings->phase++;
                    ale_fxn->set_packet_num(AleCom::get_msg_head_msg_size(temp_ale->received_msg.data));
                    timer->set_timer(temp_ale->time[MSG_HEAD][RECEIVE_LAST_TIME]+
                                     temp_ale->time[RESP_PACK_QUAL][START_EMIT]);
                    ale_fxn->ale_log("Number of data blocks: %u, bytes: %u", ale_settings->data490bit_length, ale_settings->data72bit_length);
                    temp_ale->pack_result=true;
                    temp_ale->pack_snr=temp_ale->received_msg.snr;
                }
                else	// PACK_HEAD
                {
                    temp_ale->last_msg=true;
                    if(temp_ale->received_msg.data_length!=2)
                        ale_fxn->return_to_call();
                    else if((temp_ale->received_msg.data[0]<0)||(temp_ale->received_msg.data[0]>7))
                        ale_fxn->return_to_call();
                    else
                    {
                        temp_ale->sign_form=temp_ale->received_msg.data[0];
                        timer->set_timer(   pack_head_data_time[temp_ale->sign_form]+
                                            DSP_MSG_PACK_HEAD_RX_WAITING-DSP_LIGHT_MSG_RX_WAITING   );
                        temp_ale->received_msg.data_length=-1;
                    }
                    ale_settings->superphase=9; //  GOTO DATA RECEIVING
                }
			}
			break;
		case 1:
			if(temp_ale->pause_state)	// wait to start emit call
                ale_fxn->send_tx_msg(RESP_PACK_QUAL);
			else
			{
                ale_fxn->wait_end_tx(RESP_PACK_QUAL,HSHAKE,false);
				ale_settings->phase++;
			}				
			break;	
		case 2:
			if(temp_ale->pause_state)
                ale_fxn->start_receive_msg(HSHAKE);
			else
			{
                ale_fxn->set_rx_mode(0);
				temp_ale->pause_state=true;
                ale_settings->phase=0;
                if(!ale_fxn->check_msg(HSHAKE,true))
                {
                    ale_settings->neg_counter++;
                    if(ale_settings->neg_counter>MAX_MSG_HEAD_REPEAT)
					{
                        ale_settings->neg_counter=0;
						if(temp_ale->freq_num_now==(temp_ale->best_freq_num-1))
                            ale_fxn->return_to_call();
						else
						{
                            timer->set_timer(temp_ale->time[HSHAKE][RECEIVE_LAST_TIME]+
                                             temp_ale->time[MSG_HEAD][START_RECEIVE]);
							temp_ale->freq_num_now++;
							temp_ale->sign_form=temp_ale->best_freq_sign_form[temp_ale->freq_num_now];
                            ale_fxn->set_freq(temp_ale->best_freq[temp_ale->freq_num_now]);
						}
					}
					else
                        timer->set_timer(temp_ale->time[HSHAKE][RECEIVE_LAST_TIME]+
                                         temp_ale->time[MSG_HEAD][START_RECEIVE]);
					break;
				}
				//	NEXT SUPERPHASE
                ale_fxn->set_next_superphase(9);
                timer->set_timer(temp_ale->time[HSHAKE][RECEIVE_LAST_TIME]+
                                 temp_ale->time[PACK_HEAD][START_RECEIVE]);
			}
			break;
	}
}

void AleDataTransport::data_rx_mgr()
{
	switch((ale_settings->phase+3)%3)
	{
		case 0:
			if(temp_ale->pause_state)
                ale_fxn->start_receive_msg(PACK_HEAD);
			else
			{                
				if(!temp_ale->last_msg)
                {
                    if(!ale_fxn->check_msg(PACK_HEAD,false))
                    {
                        ale_settings->neg_counter++;
                        if(ale_settings->neg_counter>MAX_MSG_HEAD_REPEAT)
                        {
                            ale_settings->neg_counter=0;	ale_settings->nres0=0;
                            if(temp_ale->freq_num_now==(temp_ale->best_freq_num-1))
                                ale_fxn->return_to_call();
                            else
                            {
                                temp_ale->freq_num_now++;
                                temp_ale->sign_form=temp_ale->best_freq_sign_form[temp_ale->freq_num_now];
                                ale_fxn->set_freq(temp_ale->best_freq[temp_ale->freq_num_now]);
                            }
                        }
#ifndef		RX_DATA_SUPERPHASE_INC_TIME
                        timer->set_timer(   pack_head_data_time[temp_ale->sign_form]
                                            +DSP_MSG_PACK_HEAD_RX_WAITING
                                            -DSP_LIGHT_MSG_RX_WAITING
                                            +temp_ale->time[PACK_HEAD][RECEIVE_LAST_TIME]
                                            +temp_ale->time[RESP_PACK_QUAL][PHASE_TIME]
                                            +temp_ale->time[HSHAKE][PHASE_TIME]
                                            +temp_ale->time[PACK_HEAD][START_RECEIVE]   );
#else
                        timer->set_timer(   pack_head_data_time[temp_ale->sign_form]
                                            +DSP_MSG_PACK_HEAD_RX_WAITING
                                            -DSP_LIGHT_MSG_RX_WAITING
                                            +temp_ale->time[PACK_HEAD][RECEIVE_LAST_TIME]
                                            +temp_ale->time[RESP_PACK_QUAL][PHASE_TIME]
                                            +temp_ale->time[HSHAKE][PHASE_TIME]
                                            +temp_ale->time[PACK_HEAD][START_RECEIVE]+RX_DATA_SUPERPHASE_INC_TIME   );
#endif
                        temp_ale->pause_state=true;
                        ale_fxn->set_rx_mode(0);
                        ale_fxn->ale_log("PACK_HEAD NOT RECEIVED");
                        break;
                    }
                    if(temp_ale->sign_form!=temp_ale->received_msg.data[0])
                        ale_settings->nres0=0;
                    temp_ale->sign_form=temp_ale->received_msg.data[0];
                    ale_fxn->ale_log("PACK_HEAD RECEIVED, SPEED %u",temp_ale->sign_form);
                    timer->set_timer(   pack_head_data_time[temp_ale->sign_form]+
                                        DSP_MSG_PACK_HEAD_RX_WAITING-DSP_LIGHT_MSG_RX_WAITING   );
				}
				else
				{					
                    temp_ale->pack_snr=temp_ale->received_msg.snr;
                    if((!ale_fxn->check_msg(PACK_HEAD,true))||(!ale_fxn->check_pack_head_crc(temp_ale->received_msg.data)))
					{
						ale_settings->phase=ale_settings->phase-2;
						ale_settings->nres0++;
						if((temp_ale->received_msg.snr<pack_head_lim_snr[DATA_SIGNAL_FORM_NUM-1])&&(temp_ale->best_freq_num!=(temp_ale->freq_num_now+1)))
						{
                            ale_settings->nres0=0;	ale_settings->neg_counter=0;
							temp_ale->freq_num_now++;
						}
						else if(ale_settings->nres0>MAX_PACK_HEAD_NRES0)
						{
                            ale_settings->nres0=0;	ale_settings->neg_counter=0;
							if(temp_ale->sign_form==(DATA_SIGNAL_FORM_NUM-1))
							{
								if(temp_ale->best_freq_num==(temp_ale->freq_num_now+1))
                                    ale_fxn->return_to_call();
								else
									temp_ale->freq_num_now++;
							}
						}
						temp_ale->pack_result=0;
                        temp_ale->sign_form=temp_ale->best_freq_sign_form[temp_ale->freq_num_now];
                        ale_fxn->ale_log("PACK_DATA NOT RECEIVED, SNR %u",temp_ale->pack_snr);
					}
					else
					{
                        ale_settings->nres0=0;	ale_settings->neg_counter=0;
						temp_ale->pack_result=1;
                        ale_settings->phase=AleCom::get_packet_num(temp_ale->received_msg.data)*3+1;
                        for(int8s i=0;i<66;i++)
                        	ale_settings->data_packs[ale_settings->phase/3][i]=temp_ale->received_msg.data[i];
                        //std::copy(packet->num_data, packet->num_data + sizeof(packet->num_data), ale.vm_fragments[ale.vm_f_idx].num_data);

                        ale_fxn->ale_log("PACK_DATA RECEIVED OK, SNR %u, NUM %u, PHASE %u",temp_ale->pack_snr, AleCom::get_packet_num(temp_ale->received_msg.data), ale_settings->phase);
					}
                    timer->set_timer(temp_ale->time[PACK_HEAD][RECEIVE_LAST_TIME]+
                                     temp_ale->time[RESP_PACK_QUAL][START_EMIT]);
				}
			}
			break;
		case 1:
            if((ale_settings->phase/3)==(ale_fxn->get_packet_num()-1))
			{
				if(temp_ale->pause_state)	// wait to start emit call
                    ale_fxn->send_tx_msg(LINK_RELEASE);
				else
				{
                    ale_fxn->wait_end_tx(LINK_RELEASE,LINK_RELEASE,false);
					ale_settings->phase++;
				}
			}		
			else	
			{	
				if(temp_ale->pause_state)	// wait to start emit call
                    ale_fxn->send_tx_msg(RESP_PACK_QUAL);
				else
				{
                    ale_fxn->wait_end_tx(RESP_PACK_QUAL,HSHAKE,false);
					ale_settings->phase++;
				}
			}
			break;	
		case 2:
            if((ale_settings->phase/3)==(ale_fxn->get_packet_num()-1))
			{
				if(temp_ale->pause_state)
                    ale_fxn->start_receive_msg(LINK_RELEASE);
				else
				{
                    ale_fxn->set_rx_mode(0);
					temp_ale->pause_state=true;
                    ale_fxn->set_next_superphase(0);
					//	END OF DATA
					ale_settings->result=0;
				}
			}
			else
			{
				if(temp_ale->pause_state)
                    ale_fxn->start_receive_msg(HSHAKE);
				else
				{
                    ale_fxn->set_rx_mode(0);
					temp_ale->pause_state=true;
					ale_settings->phase++;
#ifndef		RX_DATA_SUPERPHASE_INC_TIME
                    timer->set_timer(temp_ale->time[HSHAKE][RECEIVE_LAST_TIME]+
                                     temp_ale->time[PACK_HEAD][START_RECEIVE]);
#else
                    timer->set_timer(temp_ale->time[HSHAKE][RECEIVE_LAST_TIME]+
                                     temp_ale->time[PACK_HEAD][START_RECEIVE]+RX_DATA_SUPERPHASE_INC_TIME);
#endif
                    ale_fxn->set_freq(temp_ale->best_freq[temp_ale->freq_num_now]);
				}
			}
			break;
	}
}

}
