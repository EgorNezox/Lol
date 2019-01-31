/**
  ******************************************************************************
  * @file    privenumdspcontroller.h
  * @author  Artem Pisarenko, PMR dept. software team, ONIIP, PJSC
  * @date    22.09.2017
  *
  ******************************************************************************
 */

#ifndef FIRMWARE_APP_DSP_PRIVENUMDSPCONTROLLER_H_
#define FIRMWARE_APP_DSP_PRIVENUMDSPCONTROLLER_H_

enum  Module {
	RxRadiopath,
	TxRadiopath,
	Audiopath,
	PSWFReceiver,		//0x60
	PSWFTransmitter,    //0x72
	RadioLineNotPswf,   //0x68
	GucPath,            //0x7A
	ModemReceiver,
	VirtualPps         	// 0x64
};

enum RxParameterCode {
	RxFrequency 		= 1,
	RxRadioMode 		= 2,
	RxSquelch 			= 4,
	AGCRX 				= 8
};

enum TxParameterCode
{
	TxFrequency         = 1,
	TxRadioMode         = 2,
	TxPower             = 4,
	AGCTX               = 7
};

enum AudioParameterCode
{
	AudioModeParameter  = 0,
	AudioVolumeLevel    = 2,
	AudioMicAmplify     = 3,
	AudioSignalNumber   = 4,
	AudioSignalDuration = 5,
	AudioSignalMicLevel = 6,
	AudioTypeGarnityre  = 7
};

enum PSWF
{
	PSWF_RX 			= 0,
	PSWF_TX 			= 1
};

enum PswfRxParameterCode
{
	PswfRxState          = 0,
	PswfRxRAdr           = 1,
	PswfRxFrequency      = 2,
	PswfRxFreqSignal     = 3,
	PswfRxMode           = 4
};

enum ModemRxParameterCode
{
	ModemRxState         = 0,
	ModemRxBandwidth     = 1,
	ModemRxTimeSyncMode  = 2,
	ModemRxPhase 		 = 3,
	ModemRxRole 		 = 4
};

enum ModemState
{
	ModemRxOff 			 = 0,
	ModemRxDetectingStart= 3,
	ModemRxReceiving     = 5
};

union ParameterValue
{
	uint32_t frequency;
	uint8_t power;
	RadioMode radio_mode;
	AudioMode audio_mode;
	uint8_t squelch;
	uint8_t volume_level;
	uint8_t mic_amplify;
	uint8_t signal_number;
	uint8_t signal_duration;
	uint8_t signal_mic_level;
	uint8_t signal_type_garn;
	uint8_t agc_mode;
	uint8_t pswf_indicator;
	uint8_t pswf_r_adr;
	uint8_t swf_mode;
	uint8_t guc_mode;
	uint8_t guc_adr;
	uint8_t param;
	uint8_t voltage;
	uint8_t headsetType;
	ModemState modem_rx_state;
	ModemBandwidth modem_rx_bandwidth;
	ModemTimeSyncMode modem_rx_time_sync_mode;
	ModemPhase modem_rx_phase;
	ModemRole modem_rx_role;
};

struct PswfContent
{
	uint8_t indicator;
	uint8_t TYPE;
	uint32_t Frequency;
	uint8_t SNR;
	uint8_t R_ADR;
	uint8_t S_ADR;
	uint8_t COM_N;
	uint8_t L_CODE;
	int RN_KEY;
	uint8_t Conditional_Command;
	uint8_t RET_end_adr;
} ContentPSWF;

struct SmsContent
{
	uint8_t indicator;
	uint8_t TYPE;
	uint32_t Frequency;
	uint8_t SNR;
	uint8_t R_ADR;
	uint8_t S_ADR;
	uint8_t CYC_N;
	uint8_t L_CODE;
	int RN_KEY;
	SmsStage stage;
	uint8_t message[259];
} ContentSms;

struct GucContent
{
	uint8_t indicator;
	uint8_t type;
	uint8_t chip_time;
	uint8_t WIDTH_SIGNAL;
	uint8_t S_ADR;
	uint8_t R_ADR;
	uint8_t NUM_com;
	uint8_t ckk;
	uint8_t uin;
	uint8_t Coord;
	uint8_t stage;
	uint8_t command[120];

} ContentGuc;

enum
{
	radiostateSync,
	radiostateCmdModeOffRx,
	radiostateCmdModeOffTx,
	radiostateCmdRxFreq,
	radiostateCmdTxFreq,
	radiostateCmdRxOff,
	radiostateCmdTxOff,
	radiostateCmdRxMode,
	radiostateCmdTxPower,
	radiostateCmdTxMode,
	radiostateCmdCarrierTx,
	radiostatePswfTxPrepare,
	radiostatePswfTx,
	radiostatePswfRxPrepare,
	radiostatePswf,
	radiostatePswfRx,
	radiostateSmsTx,
	radiostateSmsRx,
	radiostateSms,
	radiostateSmsRxPrepare,
	radiostateSmsTxPrepare,
	radiostateSmsTxRxSwitch,
	radiostateSmsRxTxSwitch,
	radiostateGucTxPrepare,
	radiostateGucRxPrepare,
	radiostateGucSwitch,
	radiostateGucTx,
	radiostateGucRx,
} radio_state;

#endif /* FIRMWARE_APP_DSP_PRIVENUMDSPCONTROLLER_H_ */
