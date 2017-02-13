#include "ale_fxn.h"

namespace Multiradio {

AleFxn::AleFxn(ContTimer *tmr, Dispatcher *disp, ale_data* tmp_ale, ext_ale_settings* ale_s,  OldAleData* oldAle )
{
    timer=tmr;
    dispatcher=disp;
    temp_ale=tmp_ale;
    ale_settings=ale_s;
}

void AleFxn::aleprocessTimerRxPacketTxLinkReleaseExpired() {
    dispatcher->dsp_controller->sendModemPacket(DspController::modempacket_LinkRelease, DspController::modembw20kHz, 0, 0);
}

void AleFxn::aleprocessTX_modem(int8s packet_type, int8s* data, int8s length) {
    DspController::ModemPacketType type;
    type=(DspController::ModemPacketType)packet_type;
    dispatcher->dsp_controller->sendModemPacket(type, DspController::modembw20kHz, (uint8_t*)data, length);
}

void AleFxn::set_freq(long freq)
{
    dispatcher->dsp_controller->tuneModemFrequency(freq);
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
    if(temp_ale->tx!=0)
    {
        set_tx_mode(0);
        error_detect(2);
    }
}

void AleFxn::return_to_call()
{
    timer->stop_timer();
    ale_settings->superphase=1;
    ale_settings->phase=0;
    ale_settings->repeat_counter=0;
    set_tx_mode(0);
    set_rx_mode(0);
}

void AleFxn::gen_msg_call()
{
    AleCom::generate_call(temp_ale->tx_msg.data, true, ale_settings->call_supercounter, ale_settings->adress_dst);
    temp_ale->tx_msg.data_length=2;
}

void AleFxn::gen_trans_mode()
{
    if(ale_settings->probe_on)
        AleCom::generate_trans_mode(temp_ale->tx_msg.data, 3, 3, 1, ale_settings->schedule, ale_settings->own_adress);
    else
        AleCom::generate_trans_mode(temp_ale->tx_msg.data, 0, 3, 1, ale_settings->schedule, ale_settings->own_adress);
    temp_ale->tx_msg.data_length=3;
}

void AleFxn::gen_call_qual()
{
    AleCom::generate_resp_call_qual(temp_ale->tx_msg.data, (int8s)(((int16s)temp_ale->call_err)*19/100), temp_ale->call_snr);
    temp_ale->tx_msg.data_length=2;
}

void AleFxn::gen_sound_qual()
{
    //	FOR PROBES
}

void AleFxn::gen_msg_head()
{
    AleCom::generate_msg_head(temp_ale->tx_msg.data, get_msg_size());
    temp_ale->tx_msg.data_length=2;
}

int16s AleFxn::get_msg_size()
{
    return ale->vm_fragments.size();
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
    AleCom::generate_resp_pack_qual(temp_ale->tx_msg.data, temp_ale->pack_result,temp_ale->pack_snr);
    temp_ale->tx_msg.data_length=2;
}

bool AleFxn::get_work_freq()
{
    int8s counter;
    counter=0;
    for(int8s i=0;i<ale_settings->work_freq_num;i++)
    {
        if((ale_settings->work_freq[i]>=(temp_ale->call_freq/2))&&(ale_settings->work_freq[i]<=(temp_ale->call_freq*2)))
        {
            temp_ale->work_probe_freq[counter]=ale_settings->work_freq[i];
            counter++;
        }
    }
    temp_ale->work_probe_freq_num=counter;
    if(counter==0)
        return false;
    return true;
}

void AleFxn::send_tx_msg(int8s msg_num)
{
    set_tx_mode(11);
    temp_ale->tx_msg.data_length=0;
    if(msg_fxn[msg_num]!=0)
        msg_fxn[msg_num];
    aleprocessTX_modem(msg_num,temp_ale->tx_msg.data,temp_ale->received_msg.data_length);
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
    int8s msg_num_to_old_dsp_version[]={ 0, 0, 0, 0, 1,		1, 2, 0, 0, 0,		3, 4, 1, 0, 0,		0, 0, 0, 0, 0,		0, 2, 2, 2, 0,		2, 2 };
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

void AleFxn::set_next_superphase(int8s superphase_num)
//	DO NOT USE IN MIX MSG AND PACK HEAD !!!
{
    ale_settings->phase=0;
    ale_settings->superphase=superphase_num;
    ale_settings->repeat_counter=0;
    ale_settings->nres0=0;
    ale_settings->nres1=0;
}

bool AleFxn::check_msg(int8s msg_type, bool rx_stop)
{
    if((temp_ale->received_msg.data_length!=-1)&&(temp_ale->received_msg.type==msg_type)&&(temp_ale->received_msg.error!=100))
        return true;
    if(rx_stop)
    {
        set_rx_mode(0);
        temp_ale->pause_state=true;
    }
    else
    {
        temp_ale->last_msg=true;
        if(temp_ale->received_msg.data_length!=0)
            return true;
    }
    return false;
}

int8s AleFxn::get_packet_num()
{
    return ale->vm_f_count;
}

int8s AleFxn::set_packet_num(int8s num_msg_head)
{
    ale->vm_size = num_msg_head;
    ale->vm_f_count = ceilf((float)ale->vm_size*72/490);
    ale->vm_fragments.resize(ale->vm_f_count);
    return ale->vm_f_count;
}

void AleFxn::get_msg_fragment(int8s num, int8s* data)
{
    for(int8s i = 0; i < 66; i++)
        data[i]=ale->vm_fragments[num].num_data[i];
}



}
