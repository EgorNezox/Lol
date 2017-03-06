#ifndef ALEFXN_H
#define ALEFXN_H

#include "continious_timer.h"
#include "dispatcher.h"
#include "ale_const.h"
#include "../dsp/dspcontroller.h"

namespace Multiradio {

class Dispatcher;

class AleFxn
{
private:
    ContTimer* timer;
    Dispatcher* dispatcher;
    ale_data* temp_ale;
    ext_ale_settings* ale_settings;
    OldAleData* ale;
    int16s get_msg_size();
    void makeTable32();
    void makeTable16();
public:
    int8s get_min_value(int8s* data, int8s length);
    bool check_msg(int8s msg_type, bool rx_stop);
    void set_next_superphase(int8s superphase_num);
    void start_receive_msg(int8s msg_num);
    void wait_end_tx(int8s msg_num, int8s next_msg_num, bool next_mode, int32s inc);
    void wait_end_tx(int8s msg_num, int8s next_msg_num, bool next_mode);
    void send_tx_msg(int8s msg_num);
    bool get_work_freq();
    void gen_pack_qual();
    void gen_pack_head();
    void gen_msg_head();
    void gen_short_sound();
    void gen_sound_qual();
    void gen_call_qual();
    void gen_trans_mode();
    void gen_msg_call();
    void return_to_call();
    void tx_off_control();
    void ale_logic_error(int8s error_num);
    void error_detect(int error_num);
    void set_rx_mode(int mode);
    void set_wait_msg(int8_t msg);
    AleFxn(ContTimer* tmr, Dispatcher* disp, ale_data* tmp_ale, ext_ale_settings* ale_s, OldAleData *oldAle);
    void aleprocessTimerRxPacketTxLinkReleaseExpired();
    void set_freq(long freq);
    void set_tx(int8_t mode);
    void set_rx(int8_t mode);
    void set_caller_mode(bool mode);
    void set_tx_mode(int mode);
    int8s get_packet_num();
    int8s set_packet_num(int16s num_msg_head);
    void get_msg_fragment(int16s num, int8s* data);
    void aleprocessTX_modem(int8s packet_type, int8s* data, int8s length);
    void set_rx_bw(int8_t bandwidth);
    static unsigned short CRC16(int8_t *buf, int len);
    static unsigned int CRC32(int8_t *pData, int len);
    bool check_pack_head_crc(int8s *data);
    void ale_log(const char* text);
    void ale_log(const char* text, const char* arg);
    void ale_log(const char* text, int arg);
    void ale_log(const char* text, int arg1, int arg2);
    void ale_log(const char* text, int arg1, int arg2, int arg3);
    void ale_log(const char* text, int arg1, int arg2, int arg3, int arg4);
    void ale_log(const char* text, const char* arg1, int arg2, int arg3);
};

}

#endif // ALEFXN_H
