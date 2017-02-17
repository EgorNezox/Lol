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
    bool check_resp_call_qual(int8s* data);
    bool check_call_rx(int8s* data, int8s snr);
    bool check_trans_mode(int8s* data);

    ale_data* temp_ale;
    ext_ale_settings* ale_settings;
    ContTimer* timer;
    AleFxn* ale_fxn;
    void calc_sign_forms();
public:
    AleMain(ContTimer* tmr, ale_data* tmp_ale, ext_ale_settings* ale_s, AleFxn* ale_f);
    void call_tx_mgr();
    void link_set_tx_mgr();
    void call_rx_mgr();
    void link_set_rx_mgr();
    void short_probe_tx_mgr();
    void short_sound_qual_tx_mgr();
    //	FOR VOICEMAIL.CPP
    void fxn_1pps(int h, int m, int s);
    void start_fxn(int8s gps_en, bool caller, int8s adress_dst, bool probe_on);
    void stop_fxn();
    void modem_packet_receiver(int8s type, int8s snr, int8s error, int8s bandwidth, int8s* data, int8s data_length);
    void modem_packet_transmitter_complete();
    void short_probe_rx_mgr();
    void short_sound_qual_rx_mgr();
};

}

#endif /* FIRMWARE_APP_MRD_ALEMAIN_H_ */
