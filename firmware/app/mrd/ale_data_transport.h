#ifndef FIRMWARE_APP_MRD_ALEDATATRANSPORT_H_
#define FIRMWARE_APP_MRD_ALEDATATRANSPORT_H_

#include "ale_const.h"
#include "ale_com.h"
#include "ale_main.h"
#include "aleservice.h"
#include "ale_fxn.h"
#include "continious_timer.h"

namespace Multiradio {

class AleMain;
class AleService;
class AleFxn;

class AleDataTransport 
{
public:
    AleDataTransport(ContTimer* tmr, ale_data* tmp_ale, ext_ale_settings* ale_s, AleFxn* ale_f);
    void get_next_freq(bool set_freq);
    void msg_head_tx_mgr();
    void data_tx_mgr();
    void data_end_tx_mgr();
    void msg_head_rx_mgr();
    void msg_head_rx_error_mgr();
    void data_rx_mgr();

    ale_data* temp_ale;
    ext_ale_settings* ale_settings;
    ContTimer* timer;
    AleFxn* ale_fxn;

    //int32s time[27][7];
    //ale_data* temp_ale;
    //ext_ale_settings* ale_settings;
    //AleMain* ale_main_fxn;
    //long* time;
private:
    friend class AleMain;
    friend class AleService;

};

}

#endif
