#ifndef FIRMWARE_APP_MRD_ALECOM_H_
#define FIRMWARE_APP_MRD_ALECOM_H_

#include "ale_const.h"

class AleCom
{
public:
    AleCom();
    static int8_t generate_call(int8s* data, bool line_type, int8s cycle_num, int8s resp_addr);
    static int8_t generate_resp_call_qual(int8s* data, int8s err_num, int8s snr);
    static int8_t generate_trans_mode(int8s* data, int8s sound_type, int8s work_mode, int8s param_mode, bool schedule, int8s call_addr);
    static int8s generate_sound_qual(int8s* data, int8s* indexes, int8s* snr, int8s* sign_forms);
    static int8_t generate_msg_head(int8s* data, int16s msg_size);
    static int8_t generate_resp_pack_qual(int8s* data, bool pack_result, int8s snr);
    static bool check_resp_pack_qual(int8s* data);
    static int8s get_resp_pack_qual_snr(int8s* data);
    static int8s get_resp_call_qual_err_num(int8s* data);
    static int8s get_resp_call_qual_snr(int8s* data);
    static int8s get_trans_mode_sound_type(int8s* data);
    static int8s get_trans_mode_work_mode(int8s* data);
    static int8s get_trans_mode_param_mode(int8s* data);
    static bool get_trans_mode_schedule(int8s* data);
    static int8s get_trans_mode_caller(int8s* data);
    static bool get_call_line_type(int8s* data);
    static int8s get_call_cycle_num(int8s* data);
    static int8s get_call_resp_addr(int8s* data);
    static int16s get_msg_head_msg_size(int8s* data);
    static int8s get_pack_head_sign_form(int8s* data);
    static int8s get_sound_qual_freq_info(int8s* data, int8s* out_indexes, int8s* out_sign_form);
    static int8s get_short_sound_snr(int8s* data);
    static int8s get_short_sound_sign_forms(int8s* data);
    static int16s get_sound_qual_crc16(int8s* data);
    static void resize_symbols(int8_t *data_in, int8_t *data_out, int16_t size_in, int16_t size_out, int16_t length_in);
};

#endif
