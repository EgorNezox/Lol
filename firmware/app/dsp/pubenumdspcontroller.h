/**
  ******************************************************************************
  * @file    incdspcontroller.h
  * @author  Artem Pisarenko, PMR dept. software team, ONIIP, PJSC
  * @date    22.09.2017
  *
  ******************************************************************************
 */

#ifndef FIRMWARE_APP_DSP_PUBENUMDSPCONTROLLER_H_
#define FIRMWARE_APP_DSP_PUBENUMDSPCONTROLLER_H_

enum RadioMode {
	RadioModeOff = 0,
	RadioModeCarrierTx = 1,
	RadioModeUSB = 7, // J3E
	RadioModeFM = 9,  // F3E
	RadioModeSazhenData = 11,
	RadioModePSWF = 20,
	RadioModeVirtualPpps = 5,
	RadioModeVirtualRvv = 4
};

enum RadioOperation {
	RadioOperationOff,
	RadioOperationRxMode,
	RadioOperationTxMode,
	RadioOperationCarrierTx
};
enum AudioMode {
	AudioModeOff = 0,
	AudioModeRadiopath = 1,
	AudioModeMicTest = 2,
	AudioModePlayLongSignal = 3,
	AudioModePlayShortSignal = 4
};

enum SmsStage
{
	StageNone = -1,
	StageTx_call = 0,
	StageTx_call_ack  = 1,
	StageTx_data = 2,
	StageTx_quit = 3,
	StageRx_call = 4,
	StageRx_call_ack = 5,
	StageRx_data = 6,
	StageRx_quit = 7
};

enum GucStage
{
	GucNone = 0,
	GucTx = 1,
	GucRx = 2,
	GucTxQuit = 3,
	GucRxQuit = 4
};

enum ModemTimeSyncMode {
	modemtimesyncManual = 0,
	modemtimesyncGPS = 1
};
enum ModemPhase {
	modemphaseWaitingCall = 0,
	modemphaseALE = 1,
	modemphaseLinkEstablished = 2,
	mm1 = 4,
	mm2 = 5,
	mm3 = 6,
	mm4 = 10,
	mm5 = 11,
	mm6 = 12,
	mm7 = 21,
	mm8 = 22,
	mm9 = 23,
	mm10 = 25,
	mm11 = 26,
};
enum ModemRole {
	modemroleResponder = 0,
	modemroleCaller = 1
};
enum ModemBandwidth {
	modembw3100Hz = 1,
	modembw20kHz = 2,
	modembwAll = 3
};
enum ModemPacketType {
	modempacket_Call = 1,
	modempacket_HshakeReceiv = 2,
	modempacket_HshakeTrans = 3,
	modempacket_HshakeTransMode = 4,
	modempacket_RespCallQual = 5,
	modempacket_LinkRelease = 6,
	modempacket_msgHead = 21,
	modempacket_packHead = 22,
	modempacket_RespPackQual = 23
};

enum SmsRole
{
	SmsRoleIdle = 2,
	SmsRoleTx = 0,
	SmsRoleRx = 1
};

enum CondComRole
{
	CondComTx = 0,
	CondComRx = 1
};

typedef struct trFrame
{
	int address;
	uint8_t* data;
	int len;
};

#endif /* FIRMWARE_APP_DSP_PUBENUMDSPCONTROLLER_H_ */
