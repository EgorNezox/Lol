#ifndef FIRMWARE_APP_MRD_ALEMAIN_H_
#define FIRMWARE_APP_MRD_ALEMAIN_H_

#include "ale_data_transport.h"
#include "ale_const.h"
#include "aleservice.h"
#include "continious_timer.h"
#include "ale_fxn.h"

namespace Multiradio {

//class AleDataTransport;
class AleService;
class AleFxn;

class AleMain
{
private:
    //friend class AleService;
    void error_detect(int error_num);
    void tx_off_control();
    //	ALE MSG
    void gen_msg_call();
    void gen_trans_mode();
    void gen_call_qual();
    void gen_sound_qual();
    //	DATA MSG
    void gen_msg_head();
    void gen_pack_head();
    void gen_pack_qual();

    bool get_work_freq();
    bool check_resp_call_qual(int8s* data);
    bool check_call_rx(int8s* data, int8s snr);
    bool check_trans_mode(int8s* data);
    void short_probe_rx_mgr();

//    int16s ale_max_supercounter[2]={ALE_MAX_MANUAL_SUPERCOUNTER,ALE_MAX_GPS_SUPERCOUNTER};
//    int16s ale_hshake_snr_lim[ALE_MAX_GPS_SUPERCOUNTER]={CALL_SNR_LIM_HIGH,CALL_SNR_LIM_LOW,CALL_SNR_LIM_LOW};

    ale_data* temp_ale;
    ext_ale_settings* ale_settings;
    ContTimer* timer;
    AleFxn* ale_fxn;
public:
    AleMain(ContTimer* tmr, ale_data* tmp_ale, ext_ale_settings* ale_s, AleFxn* ale_f);
    void call_tx_mgr();
    void link_set_tx_mgr();
    void call_rx_mgr();
    void link_set_rx_mgr();
    void short_probe_tx_mgr();
    void short_sound_qual_tx_mgr();
    //	FOR ALE_DATA_MAIL.CPP
    void ale_logic_error(int8s error_num);
    void return_to_call();
    void start_receive_msg(int8s msg_num);
    void set_next_superphase(int8s superphase_num);
    bool check_msg(int8s msg_type, bool rx_stop);
    //	FOR VOICEMAIL.CPP
    void timer_fxn();
    void fxn_1pps(int h, int m, int s);
    void init_ale(int8s own_adress, int32s* call_freq, int8s call_freq_num, int32s* work_freq, int8s work_freq_num);
    void start_fxn(int8s gps_en, bool caller, int8s adress_dst, bool probe_on);
    void stop_fxn();
    void modem_packet_receiver(int8s type, int8s snr, int8s error, int8s bandwidth, int8s* data, int8s data_length);
    void modem_packet_transmitter_complete();

    void set_tx_mode(int mode);
    void set_rx_mode(int mode);
};

}

#endif /* FIRMWARE_APP_MRD_ALEMAIN_H_ */
