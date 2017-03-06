#ifndef ALE_SETTINGS_H
#define ALE_SETTINGS_H

#define	NEW_MSG_HEAD

//#define CALLER_IGNORE_RX
#define	NOT_TURN_OFF_RX_WHEN_MSG_RECEIVED
//#define	CALLER_DT_INCREMENT					(-300)
#define	TX_DATA_SUPERPHASE_INC_TIME			100
#define	RX_DATA_SUPERPHASE_INC_TIME			100

//#define	MIN_SIGN_FORM_TX					4
//#define	START_SIGN_FORM						0		//	STT SIGN FORM FOR CALL FREQ WORKING

//#define	PACKET_TEST							1

//	bit0 - mft8, bit1 - mft4, bit2 - ft500, bit3 - ft1800, bit4 - fts4, bit5 - fts8, bit6 - fts16
#define	PROBES_ENABLED	{ 0x23, 0x61, 0x7C, 0x13 }//, 0x52, 0x6F, 0x63, 0x7F, 0x7F }
//#define	NOT_TX_PROBE_MFT8
//#define	NOT_TX_PROBE_MFT4
//#define	NOT_TX_PROBE_FT500
//#define	NOT_TX_PROBE_FT1800
//#define	NOT_TX_PROBE_FTS4
//#define	NOT_TX_PROBE_FTS8
//#define	NOT_TX_PROBE_FTS16

#endif // ALE_SETTINGS_H

