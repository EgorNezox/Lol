#ifndef FIRMWARE_APP_MRD_ALECONST_H_
#define FIRMWARE_APP_MRD_ALECONST_H_

#include <stdint.h>
#include "multiradio.h"

#define	int8s	int8_t
#define	int16s	int16_t
#define	int32s	int32_t

#define OLD_DSP_VERSION

typedef struct	{
	int8s type;
	int8s snr;
	int8s error;
	int8s bandwidth;
	int8s data[128];
	int8s data_length;
	int32s time;			// TIME WHEN MSG WAS RECEIVED
}	received_message;

typedef struct	{
    int32s time[27][7];                 // ALL_TIMINGS FOR TX AND RX
	int8s tx;
	int8s rx;
	//	WORK FREQ FOR PROBE, Fc/2<=Fw<=2*Fc
	int32s work_probe_freq[32];
	int8s work_probe_freq_index[32];
	int8s work_probe_sign_forms[32];	// bit6 - fts16 en, 5 - fts8en, 4 - fts4en, 3 - ft1800en ....
	int8s work_probe_snr[32];
	int8s work_probe_freq_num;
	//	BEST FREQ AFTER PROBE PHASE
	int32s best_freq[3];
	int8s best_freq_sign_form[3];
	int8s best_freq_snr[3];
	int8s best_freq_index[3];
	int8s best_freq_num;
	//
	int32s call_freq;
	int8s call_err;				//	only for responder, used in respcallqual
	int8s call_snr;				//	only for responder, used in respcallqual
	//
	bool pack_result;			//	only for responder, for resp pack qual
	int8s pack_snr;				//	only for responder, for resp pack qual
	//
	int8s freq_num_now;			//	freq num which used now
	int8s sign_form;				//	signal for of last pack_head
	int8s snr;					//	snr of last packet (used in call, hshake and pack_head)
	int8s packet_result;			//	result of data in last pack_head (0 - none, 1 - ok)
	//
	bool pause_state;	//	true if station waiting to the next
	bool last_msg;		//	used only when 2 msg can be received at the same time (CALLER - LR AND RPQ RECEIVE (RPQtime>LRtime), RESPONDER - RECEIVE PACK_HEAD)
	int pack_head_emit_time,pack_head_phase_time;
	received_message received_msg;
	received_message tx_msg;
}	ale_data;

typedef struct	{
	int8s gps_en;	// 0 - manual, 1 - gps
	bool caller;		// true - caller, false - responder
	int32s call_freq[19];
	int8s call_freq_num;
	int32s work_freq[32];
	int8s work_freq_num;
	int8s own_adress;
	int8s caller_adress;		// written from ale rx
	bool probe_on;			// used only in tx (when station is responder, this bit is given from TRANS_MODE)
	bool schedule;			// used only in tx
	int8s adress_dst;		// used only in tx (when station is responder, this is a caller station adress)
	int8s superphase;		// set 0 to turn off ALE, set 1 to start and check for GUI
							// 0 - OFF, 1 - CALL, 2 - LINK_SET, 3 - SHORT PROBES, 4 - SHORT_PROBES_QUAL, 5 - LONG PROBES,
							// 6 - LONG_PROBES_QUAL, 7 - MSG HEAD, 8 - MSG HEAD REPEAT, 9 - DATA, 10 - DATA_END
	int8s result;			// 0 - all ok, 1 - call error, 2 - trans error, 3 - probe error, 4 - data error
	int8s phase;				// set 0 when ALE starts
	int8s call_counter;		// set 0 when ALE starts
	int8s call_supercounter;	// set 0 when ALE starts
	int8s repeat_counter;	// repeats of phase TRANSMODE, of SOUND QUAL of of VM PACKETS
	int8s nres0;				// only for data tx/rx
	int8s nres1;				// only for data tx/rx
	int8s last_data_snr[3];	// only for data tx/rx
}	ext_ale_settings;

enum AleFunctionalState {
    alefunctionIdle,
    alefunctionRx,
    alefunctionTx
};

enum AleVmAdaptationType {
    alevmadaptationNone,
    alevmadaptationUp,
    alevmadaptationDown
};

enum AlePhase {
    ALE_STOPPED,
    ALE_TX_SETUP,
    ALE_TX_CYCLE_CALL,
    ALE_TX_CALL,
    ALE_TX_CALL_RX_HSHAKE,
    ALE_TX_CALL_TX_HSHAKE,
    ALE_TX_NEG_RX_QUAL,
    ALE_TX_NEG_TX_MODE,
    ALE_TX_NEG_RX_HSHAKE,
    ALE_TX_NEG_TX_HSHAKE,
    ALE_TX_NEG_IDLE,
    ALE_TX_VM_START,
    ALE_TX_VM_TX_MSGHEAD,
    ALE_TX_VM_RX_MSG_RESP,
    ALE_TX_VM_TX_MSG_HSHAKE,
    ALE_TX_VM_MSG_IDLE,
    ALE_TX_VM_TX_PACKET,
    ALE_TX_VM_RX_PACK_RESP,
    ALE_TX_VM_TX_PACK_HSHAKE,
    ALE_TX_VM_TX_LINK_RELEASE,
    ALE_TX_VM_PACK_IDLE,
    ALE_RX_SETUP,
    ALE_RX_SCAN,
    ALE_RX_CALL,
    ALE_RX_CALL_TX_HSHAKE,
    ALE_RX_CALL_RX_HSHAKE,
    ALE_RX_NEG_TX_QUAL,
    ALE_RX_NEG_RX_MODE,
    ALE_RX_NEG_TX_HSHAKE,
    ALE_RX_NEG_RX_HSHAKE,
    ALE_RX_NEG_IDLE,
    ALE_RX_VM_START,
    ALE_RX_VM_RX_MSGHEAD,
    ALE_RX_VM_TX_MSG_RESP,
    ALE_RX_VM_RX_MSG_HSHAKE,
    ALE_RX_VM_MSG_IDLE,
    ALE_RX_VM_RX_PACKET,
    ALE_RX_VM_TX_PACK_RESP,
    ALE_RX_VM_TX_LINK_RELEASE,
    ALE_RX_VM_RX_PACK_HSHAKE,
    ALE_RX_VM_PACK_IDLE
};

enum AleState {
    AleState_IDLE,
    AleState_FAIL_GNSS,
    AleState_FAIL_HW,
    AleState_FAIL_NO_FREQ,
    AleState_FAIL_NO_ADDR,
    AleState_FHSS_FAIL_CONFIG,
    AleState_RX_SCANNING,
    AleState_RX_CALL_NEGOTIATING,
    AleState_RX_CALL_FAIL_UNSUPPORTED,
    AleState_RX_VM_TRANSFER,
    AleState_TX_CALLING,
    AleState_TX_CALL_FAIL,
    AleState_TX_CALL_NEGOTIATING,
    AleState_TX_VM_TRANSFER,
    AleState_TX_VM_FAIL,
    AleState_TX_VM_COMPLETE_PARTIAL,
    AleState_TX_VM_COMPLETE_FULL,
    AleState_RX_VM_FAIL_UNSUPPORTED,
    AleState_RX_VM_FAIL,
    AleState_RX_VM_COMPLETE_PARTIAL,
    AleState_RX_VM_COMPLETE_FULL
};

enum AleResult {
    AleResultNone,
    AleResultVoiceMail
};

struct __attribute__ ((__packed__)) AleVmPacket {
    uint8_t num_data[62];
    uint32_t crc;
};

struct OldAleData{
    AleFunctionalState f_state;
    AleState state;
    AlePhase phase;
    AleResult result;
    uint8_t vm_progress;
    Multiradio::ale_call_freqs_t call_freqs;
    Multiradio::ale_call_freqs_t work_freqs;
    uint8_t station_address, address;
    int supercycle, cycle;
    uint8_t call_snr;
    int rcount;
    int vm_size;
    int vm_f_count;
    std::vector<AleVmPacket> vm_fragments;
    int vm_f_idx;
    int vm_msg_cycle;
    int vm_sform_c;
    int vm_sform_p;
    int vm_sform_n;
    int vm_ack_count;
    int vm_nack_count;
    int vm_snr_ack[3];
    int vm_snr_nack[3];
    AleVmAdaptationType vm_adaptation;
    bool vm_packet_result;
    bool vm_last_result;
    bool ManualTimeMode;
    bool probe_on;
    uint8_t vm_packet_snr;
};

#define	DT_GPS								250
#define	DT_MANUAL							5000
#define	DT_ALE								90

//	MAXIMUM TIMES
#define	DSP_LIGHT_MSG_TX_WAITING			30
#define	DSP_MSG_PACK_HEAD_TX_WAITING		50
#define	DSP_TX_STOP_TIME					50

#define	DSP_RX_TURN_ON_WAITING				20
#define	DSP_LIGHT_MSG_RX_WAITING			20
#define	DSP_MSG_PACK_HEAD_RX_WAITING		1000

//	MIDDLE TIME FOR TIME CORRECTION
#define	DSP_CALL_WAITING					11

#define	CALL_DSP_TIME						15

#define	CALL_SNR_LIM_HIGH					5
#define	CALL_SNR_LIM_LOW					-5

#define	MAX_TRANSMODE_REPEAT				3
#define	MAX_MSG_HEAD_REPEAT					3
#define	MAX_PACK_HEAD_REPEAT				3
#define	MAX_PACK_HEAD_NRES0					2
#define	MAX_PACK_HEAD_NRES1					3
#define	MAX_SHORT_PROBE_REPEAT				3

#define	CALL_MANUAL					0
#define	CALL_GPS					1
#define	HSHAKE						2
#define	TRANS_MODE					4
#define	RESP_CALL_QUAL				5
#define	LINK_RELEASE				6
#define	SHORT_SOUND					10
#define	LONG_SOUND					11
#define	SOUND_QUAL					12
#define	MSG_HEAD					21
#define	PACK_HEAD					22
#define	RESP_PACK_QUAL				23
#define	MSG_HEAD_PACK_HEAD			25
#define	RESP_PACK_QUAL_LINK_RELEASE	26

#define	PROBE_SOUND_QUAL_PAUSE		189		// pause between probes and sound_qual
#define	ALE_DATA_PAUSE				100		// pause between ALE and data tx/rx

#define	IDEAL_PHASE_TIME	0
#define	IDEAL_START_EMIT	1
#define	IDEAL_EMIT_PERIOD	2

#define	START_EMIT				0
#define	EMIT_PERIOD				1
#define	EMIT_LAST_TIME			2
#define	START_RECEIVE			3
#define	RECEIVE_PERIOD			4
#define	RECEIVE_LAST_TIME		5
#define	PHASE_TIME				6

#define	DATA_SIGNAL_FORM_NUM	8

const int call_snr_lim[]={CALL_SNR_LIM_HIGH,CALL_SNR_LIM_LOW,CALL_SNR_LIM_LOW};
const int ale_hshake_snr_lim[]={CALL_SNR_LIM_HIGH,CALL_SNR_LIM_LOW,CALL_SNR_LIM_LOW};

const int pack_head_lim_snr[DATA_SIGNAL_FORM_NUM]={10,8,5,5,-5,-8,-11,-14};

const int pack_head_data_time[DATA_SIGNAL_FORM_NUM]={ 2640, 3960, 7920, 7920, 7392, 14784, 29568, 59136 };

#define	PACK_HEAD_IDEAL_START_EMIT		279
#define	PACK_HEAD_IDEAL_EMIT_LAST_TIME	1977		//494

const int ideal_timings[][3]={
/* CALL_MANUAL - 0 */				11145,	5925,	4816,
/* CALL_GPS - 1 */					3301,	769,	2128,
/* HSHAKE (ACK)  - 2 */				1221,	361,	448,
/* NONE - 3 */						0,		0,		0,
/* TRANS_MODE - 4 */				3797,	279,	3024,
/* RESP_CALL_QUAL - 5 */			2901,	279,	2128,
/* LINK_RELEASE - 6 */				1547,	279,	784,
/* NONE - 7 */						0,		0,		0,
/* NONE - 8 */						0,		0,		0,
/* NONE - 9 */						0,		0,		0,
/* SHORT_SOUND - 10 */				1670,	468,	708,
/* LONG_SOUND - 11 */				32218,	468,	31256,
/* SOUND_QUAL - 12 */				5589,	279,	4816,
/* NONE - 13 */						0,		0,		0,
/* NONE - 14 */						0,		0,		0,
/* NONE - 15 */						0,		0,		0,
/* NONE - 16 */						0,		0,		0,
/* NONE - 17 */						0,		0,		0,
/* NONE - 18 */						0,		0,		0,
/* NONE - 19 */						0,		0,		0,
/* NONE - 20 */						0,		0,		0,
/* MSG_HEAD - 21 */					3349,	279,	2576,
/* PACK_HEAD - 22 */				3349,	279,	2576,	// AND TIMING FOR DATA
/* RESP_PACK_QUAL - 23 */			2565,	279,	1792,
/* NONE - 24 */						0,		0,		0,
/* MSG_HEAD+PACK_HEAD - 25 */		3349,	279,	2576,
/* PACK_QUAL+LINK_RELEASE - 26 */	1547,	279,	784,
};

#define	CALL_MANUAL_SUPERPHASE_TIME	14000
#define	CALL_MANUAL_END_TIME		(CALL_MANUAL_SUPERPHASE_TIME-ideal_timings[CALL_MANUAL][IDEAL_PHASE_TIME]-2*ideal_timings[HSHAKE][IDEAL_PHASE_TIME])
#define	CALL_GPS_SUPERPHASE_TIME	6000
#define	CALL_GPS_END_TIME			(CALL_GPS_SUPERPHASE_TIME-ideal_timings[CALL_GPS][IDEAL_PHASE_TIME]-2*ideal_timings[HSHAKE][IDEAL_PHASE_TIME])
#define	TRANS_MODE_SUPERPHASE_TIME	(ideal_timings[RESP_CALL_QUAL][IDEAL_PHASE_TIME]+ideal_timings[TRANS_MODE][IDEAL_PHASE_TIME]+2*ideal_timings[HSHAKE][IDEAL_PHASE_TIME])

const int8s call_dwell_time[]		=	{ 19, 6 };
const int32s call_superphase_time[]	=	{ CALL_MANUAL_SUPERPHASE_TIME, CALL_GPS_SUPERPHASE_TIME };
const int32s call_end_time[]		=	{ CALL_MANUAL_END_TIME, CALL_GPS_END_TIME };
const int32s call_end_temp_ale[]	=	{ CALL_MANUAL_END_TIME, CALL_GPS_END_TIME };

const int32s ale_call_superphase_time[]	=	{ CALL_MANUAL_SUPERPHASE_TIME / 1000, CALL_GPS_SUPERPHASE_TIME / 1000 };
const int32s ale_max_supercounter[]		=	{ 1, 3 };


//	CALC SNR:
//	MFT8 - snr = 9.5819 * ln(x+1) - 11.159
//	MFT4 - snr = 9.2724 * ln(x+1) - 15.794
//	FT500 - snr = 9.8928 * ln(x+1) - 23.178
//	FT1800 - snr = 9.6866 * ln(x+1) - 24.728
//	FTS13x7 - snr = 10.044 * ln(x+1) - ( 10 * log(16000/speed) + 2 ); speed=125 for 4ms, 62.5 for 8ms, 31.25 for 16ms

#endif
