/**
 ******************************************************************************
 * @file    mainserviceinterface.cpp
 * @author  Artem Pisarenko, PMR dept. software team, ONIIP, PJSC
 * @date    30.10.2015
 *
 ******************************************************************************
 */

#include <math.h>
#include <string.h>
#include <stdio.h>
#define QMDEBUGDOMAIN mrd_mainservice
#include "qmdebug.h"
#include "qmcrc.h"
#include "qmendian.h"

#include "mainserviceinterface.h"
#include "dispatcher.h"
#include "voiceserviceinterface.h"

#include "ale_param_defs.h"

static_assert(ALE_TIME_TLinkReleaseL <= ALE_TIME_TRespPackQualL, "");
static_assert(ALE_TIME_dTCodec >= ALE_TIME_dTCommand, "");

/* Расчет значений временных параметров */
#define ALE_TIME_Tdwell0 (ALE_TIME_tTxCall_offset + ALE_TIME_tRoffCall_offset + TIMER_VALUE_tCallToffHshakeT_offset + ALE_TIME_dTSyn)
#define ALE_TIME_Tdwell ((unsigned int)(ceilf((float)(ALE_TIME_Tdwell0)/1000)*1000))
#define ALE_TIME_dTDwellLeft ((unsigned int)(ceilf((float)(ALE_TIME_Tdwell - ALE_TIME_Tdwell0)/2)))
#define ALE_TIME_tTxCall_offset (ALE_TIME_TEthRx + ALE_TIME_TTuneRx + ALE_TIME_TEthTx + ALE_TIME_TTuneTx + ALE_TIME_TMaxEthX + ALE_TIME_TMaxOpenTuneX + ALE_TIME_dTSyn)
#define TIMER_VALUE_tTxCall (ALE_TIME_dTDwellLeft + ALE_TIME_tTxCall_offset)
#define ALE_TIME_tRoffCall_offset (ALE_TIME_TEthTx + ALE_TIME_TCall + ALE_TIME_TRChan + ALE_TIME_TEthRx + ALE_TIME_dTCommand)
#define TIMER_VALUE_tRoffSyncCall (TIMER_VALUE_tTxCall + ALE_TIME_tRoffCall_offset)
#define TIMER_VALUE_tCallRonHshakeR_offset (ALE_TIME_TMaxEthX + ALE_TIME_TMaxOpenTuneX)
#define TIMER_VALUE_tCallTxHshakeR_offset (TIMER_VALUE_tCallRonHshakeR_offset + ALE_TIME_TMaxEthX + ALE_TIME_TMaxOpenTuneX + ALE_TIME_DTMistiming)
#define TIMER_VALUE_tCallRoffHshakeR_offset (TIMER_VALUE_tCallTxHshakeR_offset + ALE_TIME_TEthTx + ALE_TIME_THshakeReceiv + ALE_TIME_TRChan + ALE_TIME_TEthRx + ALE_TIME_dTCommand + ALE_TIME_DTMistiming)
#define TIMER_VALUE_tCallRonHshakeT_offset (TIMER_VALUE_tCallRoffHshakeR_offset + ALE_TIME_TMaxEthX + ALE_TIME_TMaxOpenTuneX)
#define TIMER_VALUE_tCallTxHshakeT_offset (TIMER_VALUE_tCallRonHshakeT_offset + ALE_TIME_TMaxEthX + ALE_TIME_TMaxOpenTuneX + ALE_TIME_DTMistiming)
#define TIMER_VALUE_tCallRoffHshakeT_offset (TIMER_VALUE_tCallTxHshakeT_offset + ALE_TIME_TEthTx + ALE_TIME_THshakeTrans + ALE_TIME_TRChan + ALE_TIME_TEthRx + ALE_TIME_dTCommand + ALE_TIME_DTMistiming)
#define TIMER_VALUE_tCallToffHshakeT_offset (TIMER_VALUE_tCallRoffHshakeT_offset + ALE_TIME_TMaxEthX + ALE_TIME_TMaxOpenTuneX)
#define TIMER_VALUE_tNegTxRespCallQual_offset (ALE_TIME_TMaxEthX + ALE_TIME_TMaxOpenTuneX + ALE_TIME_DTMistiming)
#define TIMER_VALUE_tNegRoffRespCallQual_offset (TIMER_VALUE_tNegTxRespCallQual_offset + ALE_TIME_TEthTx + ALE_TIME_TRespCallQual + ALE_TIME_TRChan + ALE_TIME_TEthRx + ALE_TIME_dTCommand + ALE_TIME_DTMistiming)
#define TIMER_VALUE_tNegRonHshakeTransMode_offset (TIMER_VALUE_tNegRoffRespCallQual_offset + ALE_TIME_TMaxEthX + ALE_TIME_TMaxOpenTuneX)
#define TIMER_VALUE_tNegTxHshakeTransMode_offset (TIMER_VALUE_tNegRonHshakeTransMode_offset + ALE_TIME_TMaxEthX + ALE_TIME_TMaxOpenTuneX + ALE_TIME_DTMistiming)
#define TIMER_VALUE_tNegRoffHshakeTransMode_offset (TIMER_VALUE_tNegTxHshakeTransMode_offset + ALE_TIME_TEthTx + ALE_TIME_THshakeTransMode + ALE_TIME_TRChan + ALE_TIME_TEthRx + ALE_TIME_dTCommand + ALE_TIME_DTMistiming)
#define TIMER_VALUE_tNegRonHshakeReceiv_offset (TIMER_VALUE_tNegRoffHshakeTransMode_offset + ALE_TIME_TMaxEthX + ALE_TIME_TMaxOpenTuneX)
#define TIMER_VALUE_tNegTxHshakeReceiv_offset (TIMER_VALUE_tNegRonHshakeReceiv_offset + ALE_TIME_TMaxEthX + ALE_TIME_TMaxOpenTuneX + ALE_TIME_DTMistiming)
#define TIMER_VALUE_tNegRoffHshakeReceiv_offset (TIMER_VALUE_tNegTxHshakeReceiv_offset + ALE_TIME_TEthTx + ALE_TIME_THshakeReceiv + ALE_TIME_TRChan + ALE_TIME_TEthRx + ALE_TIME_dTCommand + ALE_TIME_DTMistiming)
#define TIMER_VALUE_tNegRonHshakeTrans_offset (TIMER_VALUE_tNegRoffHshakeReceiv_offset + ALE_TIME_TMaxEthX + ALE_TIME_TMaxOpenTuneX)
#define TIMER_VALUE_tNegTxHshakeTrans_offset (TIMER_VALUE_tNegRonHshakeTrans_offset + ALE_TIME_TMaxEthX + ALE_TIME_TMaxOpenTuneX + ALE_TIME_DTMistiming)
#define TIMER_VALUE_tNegRoffHshakeTrans_offset (TIMER_VALUE_tNegTxHshakeTrans_offset + ALE_TIME_TEthTx + ALE_TIME_THshakeTrans + ALE_TIME_TRChan + ALE_TIME_TEthRx + ALE_TIME_dTCommand + ALE_TIME_DTMistiming)
#define TIMER_VALUE_tNegCycle (TIMER_VALUE_tNegRoffHshakeTrans_offset + ALE_TIME_TMaxEthX + ALE_TIME_TMaxOpenTuneX)
#define TIMER_VALUE_tNegStart(n) (TIMER_VALUE_tCallToffHshakeT_offset + (n)*TIMER_VALUE_tNegCycle)
#define TIMER_VALUE_tDataStart_offset(n) (TIMER_VALUE_tNegStart(n+1) + ALE_TIME_dTInit + ALE_TIME_TEthTx + ALE_TIME_TTuneTx + ALE_TIME_TEthRx + ALE_TIME_TTuneRx)
#define TIMER_VALUE_tDataTxHeadDelay(sform) (ALE_TIME_TMaxEthX + ALE_TIME_TMaxOpenTuneX + ALE_TIME_DTMistiming + ALE_TIME_dTSynPacket(sform))
#define TIMER_VALUE_tDataRoffSyncHeadDelay(sform) (TIMER_VALUE_tDataTxHeadDelay(sform) + ALE_TIME_TEthTx + ALE_TIME_THeadL(sform) + ALE_TIME_TRChan + ALE_TIME_TEthRx + ALE_TIME_dTCommand + ALE_TIME_DTMistiming)
#define TIMER_VALUE_tDataRonRespPackQualDelay(sform) (TIMER_VALUE_tDataRoffSyncHeadDelay(sform) + ALE_TIME_TDataL(sform) + (((sform) == -1)?(0):(ALE_TIME_dTCodec - ALE_TIME_dTCommand)) + ALE_TIME_TMaxEthX + ALE_TIME_TMaxOpenTuneX)
#define TIMER_VALUE_tDataTxRespPackQualDelay(sform) (TIMER_VALUE_tDataRonRespPackQualDelay(sform) + ALE_TIME_TMaxEthX + ALE_TIME_TMaxOpenTuneX + ALE_TIME_DTMistiming)
#define TIMER_VALUE_tDataRoffRespPackQualDelay(sform) (TIMER_VALUE_tDataTxRespPackQualDelay(sform) + ALE_TIME_TEthTx + ALE_TIME_TRespPackQualL + ALE_TIME_TRChan + ALE_TIME_TEthRx + ALE_TIME_dTCommand + ALE_TIME_DTMistiming)
#define TIMER_VALUE_tDataRonHshakeTDelay(sform) (TIMER_VALUE_tDataRoffRespPackQualDelay(sform) + ALE_TIME_TMaxEthX + ALE_TIME_TMaxOpenTuneX)
#define TIMER_VALUE_tDataTxHshakeTDelay(sform) (TIMER_VALUE_tDataRonHshakeTDelay(sform) + ALE_TIME_TMaxEthX + ALE_TIME_TMaxOpenTuneX + ALE_TIME_DTMistiming)
#define TIMER_VALUE_tDataRoffHshakeTDelay(sform) (TIMER_VALUE_tDataTxHshakeTDelay(sform) + ALE_TIME_TEthTx + ALE_TIME_THshakeTrans + ALE_TIME_TRChan + ALE_TIME_TEthRx + ALE_TIME_dTCommand + ALE_TIME_DTMistiming)
#define TIMER_VALUE_tDataCycle(sform) (TIMER_VALUE_tDataRoffHshakeTDelay(sform) + ALE_TIME_TMaxEthX + ALE_TIME_TMaxOpenTuneX + ALE_TIME_dTSynPacket(sform))
#define TIMER_VALUE_tDataTxLinkReleaseDelay(sform) (TIMER_VALUE_tDataTxHshakeTDelay(sform) - ALE_TIME_TRespPackQualL + ALE_TIME_TLinkReleaseL)

struct call_packet_t {
	unsigned int lineType;
	unsigned int cycleNum;
	unsigned int respAddr;
};
struct respcallqual_packet_t {
	unsigned int errSignal;
	unsigned int SNR;
};
struct hshaketransmode_packet_t {
	unsigned int soundType;
	unsigned int workMode;
	unsigned int paramMode;
	unsigned int schedule;
	unsigned int callAddr;
};
struct msghead_packet_t {
	unsigned int msgSize;
	unsigned int symCode;
};
struct resppackqual_packet_t {
	unsigned int packResult;
	unsigned int SNR;
};

typedef QmCrc<uint32_t, 32, 0x04c11db7, 0xffffffff, true, 0xffffffff> CRC32;

static int ale_vm_snr_table[] = ALE_VM_SNR_TABLE_VALUES;

namespace Multiradio {

MainServiceInterface::MainServiceInterface(Dispatcher *dispatcher) :
	QmObject(dispatcher),
	current_status(StatusNotReady),
	current_mode(VoiceModeManual),
	dispatcher(dispatcher)
{
	ale.f_state = alefunctionIdle;
	ale.state = AleState_IDLE;
	ale.phase = ALE_STOPPED;
	dispatcher->dsp_controller->hardwareFailed.connect(sigc::mem_fun(this, &MainServiceInterface::forwardDspHardwareFailure));
	ale.timerRadioReady = new QmTimer(true, this);
	ale.timerRadioReady->setInterval(500);
	ale.timerRadioReady->timeout.connect(sigc::mem_fun(this, &MainServiceInterface::aleprocessRadioReady));
	ale.timerGnssSync = new QmTimer(true, this);
	ale.timerGnssSync->timeout.connect(sigc::mem_fun(this, &MainServiceInterface::aleprocessTimerGnssSyncExpired));
	ale.timerTxCall = new QmAbsTimer(this);
	ale.timerTxCall->timeout.connect(sigc::mem_fun(this, &MainServiceInterface::aleprocessTimerTxCallExpired));
	ale.timerRoffCall = new QmTimer(true, this);
	ale.timerRoffCall->timeout.connect(sigc::mem_fun(this, &MainServiceInterface::aleprocessTimerRoffCallExpired));
	ale.timerCallRonHshakeR = new QmAbsTimer(this);
	ale.timerCallRonHshakeR->timeout.connect(sigc::mem_fun(this, &MainServiceInterface::aleprocessTimerCallRonHshakeRExpired));
	ale.timerCallTxHshakeR = new QmAbsTimer(this);
	ale.timerCallTxHshakeR->timeout.connect(sigc::mem_fun(this, &MainServiceInterface::aleprocessTimerCallTxHshakeRExpired));
	ale.timerCallRoffHshakeR = new QmAbsTimer(this);
	ale.timerCallRoffHshakeR->timeout.connect(sigc::mem_fun(this, &MainServiceInterface::aleprocessTimerCallRoffHshakeRExpired));
	ale.timerCallRonHshakeT = new QmAbsTimer(this);
	ale.timerCallRonHshakeT->timeout.connect(sigc::mem_fun(this, &MainServiceInterface::aleprocessTimerCallRonHshakeTExpired));
	ale.timerCallTxHshakeT = new QmAbsTimer(this);
	ale.timerCallTxHshakeT->timeout.connect(sigc::mem_fun(this, &MainServiceInterface::aleprocessTimerTxHshakeTransExpired));
	ale.timerCallRoffHshakeT = new QmAbsTimer(this);
	ale.timerCallRoffHshakeT->timeout.connect(sigc::mem_fun(this, &MainServiceInterface::aleprocessTimerCallRoffHshakeTExpired));
	for (int i = 0; i < 3; i++) {
		ale.timerRxNegStart[i] = new QmAbsTimer(this);
		ale.timerRxNegStart[i]->timeout.connect(sigc::mem_fun(this, &MainServiceInterface::aleprocessTimerRxNegStartExpired));
		ale.timerTxNegStart[i] = new QmAbsTimer(this);
		ale.timerTxNegStart[i]->timeout.connect(sigc::mem_fun(this, &MainServiceInterface::aleprocessTimerTxNegStartExpired));
		ale.timerNegTxRespCallQual[i] = new QmAbsTimer(this);
		ale.timerNegTxRespCallQual[i]->timeout.connect(sigc::mem_fun(this, &MainServiceInterface::aleprocessTimerNegTxRespCallQualExpired));
		ale.timerNegRoffRespCallQual[i] = new QmAbsTimer(this);
		ale.timerNegRoffRespCallQual[i]->timeout.connect(sigc::mem_fun(this, &MainServiceInterface::aleprocessTimerNegRoffExpired));
		ale.timerNegRonHshakeTransMode[i] = new QmAbsTimer(this);
		ale.timerNegRonHshakeTransMode[i]->timeout.connect(sigc::mem_fun(this, &MainServiceInterface::aleprocessTimerNegRonHshakeTransModeExpired));
		ale.timerNegTxHshakeTransMode[i] = new QmAbsTimer(this);
		ale.timerNegTxHshakeTransMode[i]->timeout.connect(sigc::mem_fun(this, &MainServiceInterface::aleprocessTimerNegTxHshakeTransModeExpired));
		ale.timerNegRoffHshakeTransMode[i] = new QmAbsTimer(this);
		ale.timerNegRoffHshakeTransMode[i]->timeout.connect(sigc::mem_fun(this, &MainServiceInterface::aleprocessTimerNegRoffHshakeTransModeExpired));
		ale.timerNegRonHshakeReceiv[i] = new QmAbsTimer(this);
		ale.timerNegRonHshakeReceiv[i]->timeout.connect(sigc::mem_fun(this, &MainServiceInterface::aleprocessTimerNegRonHshakeReceivExpired));
		ale.timerNegTxHshakeReceiv[i] = new QmAbsTimer(this);
		ale.timerNegTxHshakeReceiv[i]->timeout.connect(sigc::mem_fun(this, &MainServiceInterface::aleprocessTimerNegTxHshakeReceivExpired));
		ale.timerNegRoffHshakeReceiv[i] = new QmAbsTimer(this);
		ale.timerNegRoffHshakeReceiv[i]->timeout.connect(sigc::mem_fun(this, &MainServiceInterface::aleprocessTimerNegRoffExpired));
		ale.timerNegRonHshakeTrans[i] = new QmAbsTimer(this);
		ale.timerNegRonHshakeTrans[i]->timeout.connect(sigc::mem_fun(this, &MainServiceInterface::aleprocessTimerNegRonHshakeTransExpired));
		ale.timerNegTxHshakeTrans[i] = new QmAbsTimer(this);
		ale.timerNegTxHshakeTrans[i]->timeout.connect(sigc::mem_fun(this, &MainServiceInterface::aleprocessTimerTxHshakeTransExpired));
		ale.timerNegRoffHshakeTrans[i] = new QmAbsTimer(this);
		ale.timerNegRoffHshakeTrans[i]->timeout.connect(sigc::mem_fun(this, &MainServiceInterface::aleprocessTimerNegRoffHshakeTransExpired));
	}
	ale.timerDataStart = new QmAbsTimer(this);
	ale.timerDataStart->timeout.connect(sigc::mem_fun(this, &MainServiceInterface::aleprocessTimerDataStartExpired));
	for (int i = 0; i < 3; i++) {
		ale.timerMsgTxHead[i] = new QmAbsTimer(this);
		ale.timerMsgTxHead[i]->timeout.connect(sigc::mem_fun(this, &MainServiceInterface::aleprocessTimerMsgTxHeadExpired));
		ale.timerMsgRoffHead[i] = new QmAbsTimer(this);
		ale.timerMsgRoffHead[i]->timeout.connect(sigc::mem_fun(this, &MainServiceInterface::aleprocessTimerMsgRoffHeadExpired));
		ale.timerMsgRonRespPackQual[i] = new QmAbsTimer(this);
		ale.timerMsgRonRespPackQual[i]->timeout.connect(sigc::mem_fun(this, &MainServiceInterface::aleprocessTimerMsgRonRespPackQualExpired));
		ale.timerMsgTxRespPackQual[i] = new QmAbsTimer(this);
		ale.timerMsgTxRespPackQual[i]->timeout.connect(sigc::mem_fun(this, &MainServiceInterface::aleprocessTimerMsgTxRespPackQualExpired));
		ale.timerMsgRoffRespPackQual[i] = new QmAbsTimer(this);
		ale.timerMsgRoffRespPackQual[i]->timeout.connect(sigc::mem_fun(this, &MainServiceInterface::aleprocessTimerMsgRoffRespPackQualExpired));
		ale.timerMsgRonHshakeT[i] = new QmAbsTimer(this);
		ale.timerMsgRonHshakeT[i]->timeout.connect(sigc::mem_fun(this, &MainServiceInterface::aleprocessTimerMsgRonHshakeTExpired));
		ale.timerMsgTxHshakeT[i] = new QmAbsTimer(this);
		ale.timerMsgTxHshakeT[i]->timeout.connect(sigc::mem_fun(this, &MainServiceInterface::aleprocessTimerMsgTxHshakeTExpired));
		ale.timerMsgRoffHshakeT[i] = new QmAbsTimer(this);
		ale.timerMsgRoffHshakeT[i]->timeout.connect(sigc::mem_fun(this, &MainServiceInterface::aleprocessTimerMsgRoffHshakeTExpired));
		ale.timerRxMsgCycle[i] = new QmAbsTimer(this);
		ale.timerRxMsgCycle[i]->timeout.connect(sigc::mem_fun(this, &MainServiceInterface::aleprocessTimerRxMsgCycleExpired));
		ale.timerTxMsgCycle[i] = new QmAbsTimer(this);
		ale.timerTxMsgCycle[i]->timeout.connect(sigc::mem_fun(this, &MainServiceInterface::aleprocessTimerTxMsgCycleExpired));
	}
	ale.timerTxPacketSync = new QmAbsTimer(this);
	ale.timerTxPacketSync->timeout.connect(sigc::mem_fun(this, &MainServiceInterface::aleprocessTxPacketSync));
	ale.timerPacketTxHeadData = new QmAbsTimer(this);
	ale.timerPacketTxHeadData->timeout.connect(sigc::mem_fun(this, &MainServiceInterface::aleprocessTimerPacketTxHeadDataExpired));
	ale.timerPacketRonRespPackQual = new QmAbsTimer(this);
	ale.timerPacketRonRespPackQual->timeout.connect(sigc::mem_fun(this, &MainServiceInterface::aleprocessTimerPacketRonRespPackQualExpired));
	ale.timerPacketRoffRespPackQual = new QmAbsTimer(this);
	ale.timerPacketRoffRespPackQual->timeout.connect(sigc::mem_fun(this, &MainServiceInterface::aleprocessTimerPacketRoffRespPackQualExpired));
	ale.timerPacketTxHshakeT = new QmAbsTimer(this);
	ale.timerPacketTxHshakeT->timeout.connect(sigc::mem_fun(this, &MainServiceInterface::aleprocessTimerPacketTxHshakeTExpired));
	ale.timerTxPacketTxLinkRelease = new QmAbsTimer(this);
	ale.timerTxPacketTxLinkRelease->timeout.connect(sigc::mem_fun(this, &MainServiceInterface::aleprocessTimerTxPacketTxLinkReleaseExpired));
	ale.timerRxPacketSync = new QmAbsTimer(this);
	ale.timerRxPacketSync->timeout.connect(sigc::mem_fun(this, &MainServiceInterface::aleprocessRxPacketSync));
	ale.timerPacketRoffHead = new QmAbsTimer(this);
	ale.timerPacketRoffHead->timeout.connect(sigc::mem_fun(this, &MainServiceInterface::aleprocessPacketRoffHeadExpired));
	ale.timerPacketTxRespPackQual = new QmAbsTimer(this);
	ale.timerPacketTxRespPackQual->timeout.connect(sigc::mem_fun(this, &MainServiceInterface::aleprocessPacketTxRespPackQualExpired));
	ale.timerRxPacketTxLinkRelease = new QmAbsTimer(this);
	ale.timerRxPacketTxLinkRelease->timeout.connect(sigc::mem_fun(this, &MainServiceInterface::aleprocessTimerRxPacketTxLinkReleaseExpired));
	ale.timerPacketRonHshakeT = new QmAbsTimer(this);
	ale.timerPacketRonHshakeT->timeout.connect(sigc::mem_fun(this, &MainServiceInterface::aleprocessPacketRonHshakeTExpired));
	ale.timerPacketRoffHshakeT = new QmAbsTimer(this);
	ale.timerPacketRoffHshakeT->timeout.connect(sigc::mem_fun(this, &MainServiceInterface::aleprocessPacketRoffHshakeTExpired));
	if (dispatcher->navigator != 0)
		dispatcher->navigator->syncPulse.connect(sigc::mem_fun(this, &MainServiceInterface::aleprocess1PPS));
	dispatcher->dsp_controller->transmittedModemPacket.connect(sigc::mem_fun(this, &MainServiceInterface::aleprocessModemPacketTransmitted));
	dispatcher->dsp_controller->failedTxModemPacket.connect(sigc::mem_fun(this, &MainServiceInterface::aleprocessModemPacketFailedTx));
	dispatcher->dsp_controller->receivedModemPacket.connect(sigc::mem_fun(this, &MainServiceInterface::aleprocessModemPacketReceived));
	dispatcher->dsp_controller->startedRxModemPacket_packHead.connect(sigc::mem_fun(this, &MainServiceInterface::aleprocessModemPacketStartedRxPackHead));
	dispatcher->dsp_controller->failedRxModemPacket.connect(sigc::mem_fun(this, &MainServiceInterface::aleprocessModemPacketFailedRx));
}

MainServiceInterface::~MainServiceInterface()
{
}

void MainServiceInterface::setFS(DataStorage::FS *fs)
{
    storageFs = fs;
}

MainServiceInterface::Status MainServiceInterface::getStatus() {
	return current_status;
}

void MainServiceInterface::setVoiceMode(VoiceMode mode) {
	if (mode == current_mode)
		return;
	current_mode = mode;
	if ((mode == VoiceModeAuto) && !dispatcher->isVoiceMode())
		return;
	dispatcher->updateVoiceChannel(true);
	dispatcher->headset_controller->setSmartCurrentChannelSpeed(dispatcher->voice_service->getCurrentChannelSpeed());
}

MainServiceInterface::VoiceMode MainServiceInterface::getVoiceMode() {
	return current_mode;
}

void MainServiceInterface::setStatus(Status value) {
	if (current_status != value) {
		current_status = value;
		statusChanged(value);
	}
}

void MainServiceInterface::forwardDspHardwareFailure(uint8_t subdevice_code, uint8_t error_code) {
	dspHardwareFailed.emit(subdevice_code, error_code);
}

void MainServiceInterface::printDebugVmMessage(int groups, int packets, voice_message_t &message) {
	qmDebugMessage(QmDebug::Info, "voice message: %d groups, %d packets", groups, packets);
	if (qmDebugIsVerbose()) {
		char *message_data_dump = new char[message.size()*3+1];
		message_data_dump[0] = 0;
		for (unsigned int i = 0; i < message.size(); i++)
			sprintf(message_data_dump + i*3, " %02X", message[i]);
		qmDebugMessage(QmDebug::Dump, "voice message data: %s", message_data_dump);
		delete[] message_data_dump;
	}
}

void MainServiceInterface::startAleRx() {
	qmDebugMessage(QmDebug::Info, "starting ALE rx");
	if (!startAleSession())
		return;
	ale.f_state = alefunctionRx;
	ale.result = AleResultNone;
	setAlePhase(ALE_RX_SETUP);
	setAleState(AleState_RX_SCANNING);
	ale.vm_size = 0;
	ale.vm_fragments.clear();
	dispatcher->dsp_controller->setModemReceiverBandwidth(DspController::modembwAll);
	dispatcher->dsp_controller->setModemReceiverTimeSyncMode(DspController::modemtimesyncGPS);
	dispatcher->dsp_controller->setModemReceiverPhase(DspController::modemphaseWaitingCall);
	dispatcher->dsp_controller->setModemReceiverRole(DspController::modemroleResponder);
	ale.timerRadioReady->start();
}

void MainServiceInterface::startAleTxVoiceMail(uint8_t address) {
	qmDebugMessage(QmDebug::Info, "starting ALE tx voice mail (address = %02u)", address);
	if (!startAleSession())
		return;
	ale.f_state = alefunctionTx;
	ale.result = AleResultNone;
	ale.address = address;
	setAlePhase(ALE_TX_SETUP);
	setAleState(AleState_TX_CALLING);
	ale.supercycle = 1;
	ale.cycle = 1;
	voice_message_t message = dispatcher->headset_controller->getRecordedSmartMessage();
	int message_size_mismatch = message.size() % 9;
	if (message_size_mismatch > 0)
		message.resize((message.size() + (9 - message_size_mismatch)), 0);
	int message_bits_size = message.size()*8;
	ale.vm_size = message_bits_size/72;
	ale.vm_f_count = ceilf((float)message_bits_size/490);
	ale.vm_fragments.resize(ale.vm_f_count);
	for (unsigned int i = 0; i < ale.vm_fragments.size(); i++) {
		ale.vm_fragments[i].num_data[0] = (i & 0x3F) << 2;
		for (unsigned int j = 1; j < sizeof(ale.vm_fragments[0].num_data); j++)
			ale.vm_fragments[i].num_data[j] = 0;
	}
	for (int m_bit_i = 0; m_bit_i < message_bits_size; m_bit_i++) {
		int f_i = m_bit_i / 490;
		int f_bit_i = 6 + (m_bit_i % 490);
		int f_byte_i = f_bit_i / 8;
		int f_byte_bit = 7 - (f_bit_i % 8);
		int m_byte_bit = 7 - (m_bit_i % 8);
		if ((message[m_bit_i/8] & (1 << m_byte_bit)) != 0)
			ale.vm_fragments[f_i].num_data[f_byte_i] |= (1 << f_byte_bit);
	}
	for (unsigned int i = 0; i < ale.vm_fragments.size(); i++) {
		CRC32 f_crc;
		f_crc.update(&(ale.vm_fragments[i].num_data[0]), sizeof(ale.vm_fragments[0].num_data));
		qmToBigEndian(f_crc.result(), (uint8_t *)&(ale.vm_fragments[i].crc));
	}
	printDebugVmMessage(ale.vm_size, ale.vm_f_count, message);
	dispatcher->dsp_controller->setModemReceiverBandwidth(DspController::modembw20kHz);
	dispatcher->dsp_controller->setModemReceiverTimeSyncMode(DspController::modemtimesyncGPS);
	dispatcher->dsp_controller->setModemReceiverPhase(DspController::modemphaseALE);
	dispatcher->dsp_controller->setModemReceiverRole(DspController::modemroleCaller);
	ale.timerRadioReady->start();
}

MainServiceInterface::AleResult MainServiceInterface::stopAle() {
	stopAleSession();
	setAlePhase(ALE_STOPPED);
	ale.f_state = alefunctionIdle;
	return AleResultNone;
}

MainServiceInterface::AleState MainServiceInterface::getAleState() {
	return ale.state;
}

uint8_t Multiradio::MainServiceInterface::getAleVmProgress() {
	return ale.vm_progress;
}

uint8_t MainServiceInterface::getAleRxAddress() {
	return ale.address;
}

voice_message_t MainServiceInterface::getAleRxVmMessage() {
	if (ale.vm_size == 0)
		return voice_message_t();
	int message_bits_size;
	if (ale.vm_f_idx == ale.vm_f_count) {
		message_bits_size = ale.vm_size*72;
	} else {
		message_bits_size = ale.vm_f_idx*490;
		if (!((message_bits_size % 72) == 0))
			message_bits_size += 72 - (message_bits_size % 72);
	}
	voice_message_t vm_rx_message(message_bits_size/8, 0);
	int message_bits_offset = 0;
	for (int f_i = 0; f_i < ale.vm_f_idx; f_i++) {
		int f_bits_size = (f_i == (ale.vm_f_count - 1))?(ale.vm_size*72 - (ale.vm_f_count - 1)*490):(490);
		for (int f_bit_i = 6; f_bit_i < (6 + f_bits_size); f_bit_i++) {
			int f_byte_i = f_bit_i / 8;
			int f_byte_bit = 7 - (f_bit_i % 8);
			int m_byte_i = message_bits_offset / 8;
			int m_byte_bit = 7 - (message_bits_offset % 8);
			if ((ale.vm_fragments[f_i].num_data[f_byte_i] & (1 << f_byte_bit)) != 0)
				vm_rx_message[m_byte_i] |= (1 << m_byte_bit);
			message_bits_offset++;
		}
	}
	return vm_rx_message;
}

void MainServiceInterface::setAleState(AleState value) {
	if (ale.state != value) {
		const char * str = 0;
#define ALE_STATE_SWITCH_CASE(option)	case AleState_ ## option: str = #option; break;
		switch (value) {
		ALE_STATE_SWITCH_CASE(IDLE)
		ALE_STATE_SWITCH_CASE(FAIL_GNSS)
		ALE_STATE_SWITCH_CASE(FAIL_HW)
		ALE_STATE_SWITCH_CASE(FAIL_NO_FREQ)
		ALE_STATE_SWITCH_CASE(FAIL_NO_ADDR)
		ALE_STATE_SWITCH_CASE(FHSS_FAIL_CONFIG)
		ALE_STATE_SWITCH_CASE(RX_SCANNING)
		ALE_STATE_SWITCH_CASE(RX_CALL_NEGOTIATING)
		ALE_STATE_SWITCH_CASE(RX_CALL_FAIL_UNSUPPORTED)
		ALE_STATE_SWITCH_CASE(RX_VM_TRANSFER)
		ALE_STATE_SWITCH_CASE(TX_CALLING)
		ALE_STATE_SWITCH_CASE(TX_CALL_FAIL)
		ALE_STATE_SWITCH_CASE(TX_CALL_NEGOTIATING)
		ALE_STATE_SWITCH_CASE(TX_VM_TRANSFER)
		ALE_STATE_SWITCH_CASE(TX_VM_FAIL)
		ALE_STATE_SWITCH_CASE(TX_VM_COMPLETE_PARTIAL)
		ALE_STATE_SWITCH_CASE(TX_VM_COMPLETE_FULL)
		ALE_STATE_SWITCH_CASE(RX_VM_FAIL_UNSUPPORTED)
		ALE_STATE_SWITCH_CASE(RX_VM_FAIL)
		ALE_STATE_SWITCH_CASE(RX_VM_COMPLETE_PARTIAL)
		ALE_STATE_SWITCH_CASE(RX_VM_COMPLETE_FULL)
		}
		QM_ASSERT(str);
		qmDebugMessage(QmDebug::Info, "ale state = %s", str);
		ale.state = value;
		aleStateChanged(value);
	}
}

void MainServiceInterface::setAlePhase(AlePhase value) {
	const char * str = 0;
#define ALE_PHASE_SWITCH_CASE(option)	case ALE_ ## option: str = #option; break;
	switch (value) {
	ALE_PHASE_SWITCH_CASE(STOPPED)
	ALE_PHASE_SWITCH_CASE(TX_SETUP)
	ALE_PHASE_SWITCH_CASE(TX_CYCLE_CALL)
	ALE_PHASE_SWITCH_CASE(TX_CALL)
	ALE_PHASE_SWITCH_CASE(TX_CALL_RX_HSHAKE)
	ALE_PHASE_SWITCH_CASE(TX_CALL_TX_HSHAKE)
	ALE_PHASE_SWITCH_CASE(TX_NEG_RX_QUAL)
	ALE_PHASE_SWITCH_CASE(TX_NEG_TX_MODE)
	ALE_PHASE_SWITCH_CASE(TX_NEG_RX_HSHAKE)
	ALE_PHASE_SWITCH_CASE(TX_NEG_TX_HSHAKE)
	ALE_PHASE_SWITCH_CASE(TX_NEG_IDLE)
	ALE_PHASE_SWITCH_CASE(TX_VM_START)
	ALE_PHASE_SWITCH_CASE(TX_VM_TX_MSGHEAD)
	ALE_PHASE_SWITCH_CASE(TX_VM_RX_MSG_RESP)
	ALE_PHASE_SWITCH_CASE(TX_VM_TX_MSG_HSHAKE)
	ALE_PHASE_SWITCH_CASE(TX_VM_MSG_IDLE)
	ALE_PHASE_SWITCH_CASE(TX_VM_TX_PACKET)
	ALE_PHASE_SWITCH_CASE(TX_VM_RX_PACK_RESP)
	ALE_PHASE_SWITCH_CASE(TX_VM_TX_PACK_HSHAKE)
	ALE_PHASE_SWITCH_CASE(TX_VM_TX_LINK_RELEASE)
	ALE_PHASE_SWITCH_CASE(TX_VM_PACK_IDLE)
	ALE_PHASE_SWITCH_CASE(RX_SETUP)
	ALE_PHASE_SWITCH_CASE(RX_SCAN)
	ALE_PHASE_SWITCH_CASE(RX_CALL)
	ALE_PHASE_SWITCH_CASE(RX_CALL_TX_HSHAKE)
	ALE_PHASE_SWITCH_CASE(RX_CALL_RX_HSHAKE)
	ALE_PHASE_SWITCH_CASE(RX_NEG_TX_QUAL)
	ALE_PHASE_SWITCH_CASE(RX_NEG_RX_MODE)
	ALE_PHASE_SWITCH_CASE(RX_NEG_TX_HSHAKE)
	ALE_PHASE_SWITCH_CASE(RX_NEG_RX_HSHAKE)
	ALE_PHASE_SWITCH_CASE(RX_NEG_IDLE)
	ALE_PHASE_SWITCH_CASE(RX_VM_START)
	ALE_PHASE_SWITCH_CASE(RX_VM_RX_MSGHEAD)
	ALE_PHASE_SWITCH_CASE(RX_VM_TX_MSG_RESP)
	ALE_PHASE_SWITCH_CASE(RX_VM_RX_MSG_HSHAKE)
	ALE_PHASE_SWITCH_CASE(RX_VM_MSG_IDLE)
	ALE_PHASE_SWITCH_CASE(RX_VM_RX_PACKET)
	ALE_PHASE_SWITCH_CASE(RX_VM_TX_PACK_RESP)
	ALE_PHASE_SWITCH_CASE(RX_VM_TX_LINK_RELEASE)
	ALE_PHASE_SWITCH_CASE(RX_VM_RX_PACK_HSHAKE)
	ALE_PHASE_SWITCH_CASE(RX_VM_PACK_IDLE)
	}
	QM_ASSERT(str);
	qmDebugMessage(QmDebug::Info, "ale phase = %s", str);
	ale.phase = value;
}

void MainServiceInterface::setAleVmProgress(uint8_t value) {
	if (ale.vm_progress != value) {
		qmDebugMessage(QmDebug::Info, "ale vm progress = %u", value);
		ale.vm_progress = value;
		aleVmProgressUpdated(value);
	}
}

bool MainServiceInterface::startAleSession() {
	if (!dispatcher->data_storage_fs->getAleDefaultCallFreqs(ale.call_freqs)) {
		setAleState(AleState_FAIL_NO_FREQ);
		setAlePhase(ALE_STOPPED);
		return false;
	}
	for (unsigned int i = 0; i < ale.call_freqs.size(); i++)
		qmDebugMessage(QmDebug::Info, "ale call freq [%u] = %lu", i, ale.call_freqs[i]);
	if (!dispatcher->data_storage_fs->getAleStationAddress(ale.station_address)) {
		setAleState(AleState_FAIL_NO_ADDR);
		setAlePhase(ALE_STOPPED);
		return false;
	}
	qmDebugMessage(QmDebug::Info, "ale station address = %02u", ale.station_address);
	dispatcher->dsp_controller->setRadioOperation(DspController::RadioOperationOff);
	dispatcher->atu_controller->setMinimalActivityMode(true);
	dispatcher->power_battery->setMinimalActivityMode(true);
	return true;
}

void MainServiceInterface::stopAleSession() {
	qmDebugMessage(QmDebug::Info, "ale session stop");
	switch (ale.f_state) {
	case alefunctionRx: {
		stopAllRxTimers();
		voice_message_t message = getAleRxVmMessage();
		printDebugVmMessage(ale.vm_size, ale.vm_f_count, message);
		dispatcher->headset_controller->setSmartMessageToPlay(message);
        if (storageFs > 0)
            storageFs->setVoiceMail(&message);
		break;
	}
	case alefunctionTx: {
		ale.vm_fragments.clear();
		stopAllTxTimers();
		break;
	}
	default:
		break;
	}
	if (ale.phase == ALE_STOPPED) {
		setAleState(AleState_IDLE);
		return;
	}
	dispatcher->dsp_controller->disableModemReceiver();
	dispatcher->dsp_controller->disableModemTransmitter();
	dispatcher->atu_controller->setMinimalActivityMode(false);
	dispatcher->power_battery->setMinimalActivityMode(false);
	dispatcher->navigator->setMinimalActivityMode(false);
}

void MainServiceInterface::stopAleRxTimers() {
	ale.timerRoffCall->stop();
	ale.timerCallTxHshakeR->stop();
	ale.timerCallRonHshakeT->stop();
	ale.timerCallRoffHshakeT->stop();
	for (int i = 0; i < 3; i++) {
		ale.timerRxNegStart[i]->stop();
		ale.timerNegTxRespCallQual[i]->stop();
		ale.timerNegRonHshakeTransMode[i]->stop();
		ale.timerNegRoffHshakeTransMode[i]->stop();
		ale.timerNegTxHshakeReceiv[i]->stop();
		ale.timerNegRonHshakeTrans[i]->stop();
		ale.timerNegRoffHshakeTrans[i]->stop();
	}
}

void MainServiceInterface::stopAleTxTimers() {
	ale.timerTxCall->stop();
	ale.timerCallRonHshakeR->stop();
	ale.timerCallRoffHshakeR->stop();
	ale.timerCallTxHshakeT->stop();
	for (int i = 0; i < 3; i++) {
		ale.timerTxNegStart[i]->stop();
		ale.timerNegRoffRespCallQual[i]->stop();
		ale.timerNegTxHshakeTransMode[i]->stop();
		ale.timerNegRonHshakeReceiv[i]->stop();
		ale.timerNegRoffHshakeReceiv[i]->stop();
		ale.timerNegTxHshakeTrans[i]->stop();
	}
}

void MainServiceInterface::stopVmMsgRxTimers() {
	for (int i = 0; i < 3; i++) {
		ale.timerMsgRoffHead[i]->stop();
		ale.timerMsgTxRespPackQual[i]->stop();
		ale.timerMsgRonHshakeT[i]->stop();
		ale.timerMsgRoffHshakeT[i]->stop();
		ale.timerRxMsgCycle[i]->stop();
	}
}

void MainServiceInterface::stopVmMsgTxTimers() {
	for (int i = 0; i < 3; i++) {
		ale.timerMsgTxHead[i]->stop();
		ale.timerMsgRonRespPackQual[i]->stop();
		ale.timerMsgRoffRespPackQual[i]->stop();
		ale.timerMsgTxHshakeT[i]->stop();
		ale.timerTxMsgCycle[i]->stop();
	}
}

void MainServiceInterface::stopAllRxTimers() {
	ale.timerRadioReady->stop();
	ale.timerGnssSync->stop();
	stopAleRxTimers();
	ale.timerDataStart->stop();
	stopVmMsgRxTimers();
	ale.timerRxPacketSync->stop();
	ale.timerPacketRoffHead->stop();
	ale.timerPacketTxRespPackQual->stop();
	ale.timerPacketRonHshakeT->stop();
	ale.timerPacketRoffHshakeT->stop();
	ale.timerRxPacketTxLinkRelease->stop();
}

void MainServiceInterface::stopAllTxTimers() {
	ale.timerRadioReady->stop();
	ale.timerGnssSync->stop();
	stopAleTxTimers();
	ale.timerDataStart->stop();
	stopVmMsgTxTimers();
	ale.timerTxPacketSync->stop();
	ale.timerPacketTxHeadData->stop();
	ale.timerPacketRonRespPackQual->stop();
	ale.timerPacketRoffRespPackQual->stop();
	ale.timerPacketTxHshakeT->stop();
	ale.timerTxPacketTxLinkRelease->stop();
}

void MainServiceInterface::startVmRx() {
	int data_start_offset = TIMER_VALUE_tDataStart_offset(ale.rcount);
	ale.timerDataStart->start(ale.tCallStartSync, data_start_offset);
	ale.tPacketSync = ale.tCallStartSync;
	ale.tPacketSync.shift(data_start_offset);
	setAlePhase(ALE_RX_VM_START);
	ale.vm_msg_cycle = 0;
	ale.vm_progress = 0;
	setAleState(AleState_RX_VM_TRANSFER);
	dispatcher->dsp_controller->setModemReceiverPhase(DspController::modemphaseLinkEstablished);
}

void MainServiceInterface::startVmTx() {
	int data_start_offset = TIMER_VALUE_tRoffSyncCall + TIMER_VALUE_tDataStart_offset(ale.rcount);
	ale.timerDataStart->start(ale.tCallStartSync, data_start_offset);
	ale.tPacketSync = ale.tCallStartSync;
	ale.tPacketSync.shift(data_start_offset);
	setAlePhase(ALE_TX_VM_START);
	ale.vm_msg_cycle = 0;
	ale.vm_progress = 0;
	setAleState(AleState_TX_VM_TRANSFER);
	dispatcher->dsp_controller->setModemReceiverPhase(DspController::modemphaseLinkEstablished);
}

bool MainServiceInterface::checkDwellStart(int &freq_idx) {
	Navigation::Coord_Date date = dispatcher->navigator->getCoordDate();
	char hr_ch[3] = {0,0,0};
	char mn_ch[3] = {0,0,0};
	char sec_ch[3] = {0,0,0};
	memcpy(hr_ch,&date.time[0],2);
	memcpy(mn_ch,&date.time[2],2);
	memcpy(sec_ch,&date.time[4],2);
	int hrs = atoi(hr_ch);
	int min = atoi(mn_ch);
	int sec = atoi(sec_ch);
	sec += 1;
	if (sec >= 60) {
		sec %= 60;
		min++;
		if (min >= 60) {
			min %= 60;
			hrs++;
			if (hrs >= 24) {
				hrs %= 24;
			}
		}
	}
	if ((hrs == 23) && (min == 59) && (sec == 60))
		return false;
	int msecs = sec*1000+min*(60*1000)+hrs*(60*60*1000);
	if ((msecs % ALE_TIME_Tdwell) != 0)
		return false;
	freq_idx = (msecs/ALE_TIME_Tdwell) % ale.call_freqs.size();
	return true;
}

int MainServiceInterface::convertSnrFromPacket(uint8_t value) {
	if (value == 0)
		return -21;
	if (value == 63)
		return 42;
	return (-20 + ((float)(value - 1)/(62-1))*(20 + 41));
}

uint8_t MainServiceInterface::convertSnrToPacket(int value) {
	if (value < -20)
		return 0;
	if (value > 41)
		return 63;
	return (1 + ((value + 20)/(20 + 41))*(62 - 1));
}

void MainServiceInterface::proceedRxScanning() {
	qmDebugMessage(QmDebug::Info, "ale proceed scanning");
	stopAleRxTimers();
	dispatcher->dsp_controller->setModemReceiverBandwidth(DspController::modembwAll);
	dispatcher->dsp_controller->setModemReceiverPhase(DspController::modemphaseWaitingCall);
	ale.timerRadioReady->start();
	setAlePhase(ALE_RX_SETUP);
	setAleState(AleState_RX_SCANNING);
	dispatcher->navigator->setMinimalActivityMode(false);
}

void MainServiceInterface::proceedTxCalling() {
	stopAleTxTimers();
	ale.cycle++;
	if (!(ale.cycle <= (int)ale.call_freqs.size())) {
		ale.supercycle++;
		ale.cycle = 1;
	}
	qmDebugMessage(QmDebug::Info, "ale proceed calling (cycle = %d, supercycle = %d)", ale.cycle, ale.supercycle);
	if (ale.supercycle <= 3) {
		setAlePhase(ALE_TX_CYCLE_CALL);
		setAleState(AleState_TX_CALLING);
		dispatcher->navigator->setMinimalActivityMode(false);
	} else {
		setAleState(AleState_TX_CALL_FAIL);
		stopAleSession();
	}
}

bool MainServiceInterface::evaluatePacketSNR(int8_t snr) {
	int8_t snr_threshold = (ale.supercycle == 1)?ALE_CALL_SNR_HIGH:ALE_CALL_SNR_LOW;
	return (snr >= snr_threshold);
}

void MainServiceInterface::aleprocessRadioReady() {
	switch (ale.phase) {
	case ALE_TX_SETUP:
		setAlePhase(ALE_TX_CYCLE_CALL);
		break;
	case ALE_RX_SETUP:
		setAlePhase(ALE_RX_SCAN);
		break;
	default:
		break;
	}
}

void MainServiceInterface::aleprocessModemPacketTransmitted(DspController::ModemPacketType type) {
	switch (type) {
	case DspController::modempacket_Call:
	case DspController::modempacket_HshakeTransMode:
		dispatcher->dsp_controller->disableModemTransmitter();
		break;
	case DspController::modempacket_HshakeTrans: {
		switch (ale.phase) {
		case ALE_TX_CALL_TX_HSHAKE:
			dispatcher->dsp_controller->disableModemTransmitter();
			setAleState(AleState_TX_CALL_NEGOTIATING);
			break;
		case ALE_TX_NEG_TX_HSHAKE:
			dispatcher->dsp_controller->disableModemTransmitter();
			stopAleTxTimers();
			startVmTx();
			break;
		case ALE_TX_VM_TX_MSG_HSHAKE:
			dispatcher->dsp_controller->disableModemTransmitter();
			stopVmMsgTxTimers();
			ale.tPacketSync.shift(ale.vm_msg_cycle*TIMER_VALUE_tDataCycle(-1));
			ale.vm_sform_c = ALE_VM_INITIAL_SFORM;
			ale.vm_sform_p = ale.vm_sform_c;
			ale.vm_f_idx = 0;
			ale.rcount = 0;
			ale.vm_ack_count = 0;
			ale.vm_nack_count = 0;
			aleprocessTxPacketSync();
			break;
		case ALE_TX_VM_TX_PACK_HSHAKE: {
			dispatcher->dsp_controller->disableModemTransmitter();
			ale.vm_sform_p = ale.vm_sform_c;
			AleVmAdaptationType adaptation_required = ale.vm_adaptation;
			if (adaptation_required == alevmadaptationDown) {
				if (!adaptPacketTxDown()) {
					processFailedPacketTxCycle();
					break;
				}
			}
			if (adaptation_required == alevmadaptationUp)
				adaptPacketTxUp();
			ale.rcount = 0;
			startNextPacketTxCycle();
			break;
		}
		default:
			break;
		}
		break;
	}
	case DspController::modempacket_msgHead: {
		dispatcher->dsp_controller->disableModemTransmitter();
		break;
	}
	case DspController::modempacket_packHead: {
		dispatcher->dsp_controller->disableModemTransmitter();
		ale.timerPacketRonRespPackQual->start(ale.tPacketSync, TIMER_VALUE_tDataRonRespPackQualDelay(ale.vm_sform_c));
		break;
	}
	case DspController::modempacket_LinkRelease: {
		switch (ale.phase) {
		case ALE_TX_VM_TX_LINK_RELEASE: {
			setAleState(AleState_TX_VM_COMPLETE_FULL);
			stopAleSession();
			break;
		}
		case ALE_RX_VM_TX_LINK_RELEASE: {
			setAleState(AleState_RX_VM_COMPLETE_FULL);
			stopAleSession();
			break;
		}
		default:
			break;
		}
		break;
	}
	case DspController::modempacket_HshakeReceiv: {
		dispatcher->dsp_controller->disableModemTransmitter();
		break;
	}
	case DspController::modempacket_RespCallQual: {
		dispatcher->dsp_controller->disableModemTransmitter();
		break;
	}
	case DspController::modempacket_RespPackQual: {
		dispatcher->dsp_controller->disableModemTransmitter();
		if (ale.phase != ALE_RX_VM_TX_PACK_RESP)
			break;
		ale.timerPacketRonHshakeT->start(ale.tPacketSync, TIMER_VALUE_tDataRonHshakeTDelay(ale.vm_sform_n));
		break;
	}
	default:
		break;
	}
}

void MainServiceInterface::aleprocessModemPacketFailedTx() {
	setAleState(AleState_FAIL_HW);
	stopAleSession();
}

void MainServiceInterface::aleprocessModemPacketReceived(DspController::ModemPacketType type, uint8_t snr, uint8_t errors, DspController::ModemBandwidth bandwidth, uint8_t* data, int data_len) {
	int8_t snr_db_value = convertSnrFromPacket(snr);
	qmDebugMessage(QmDebug::Info, "ale received modem packet (type = %u, bandwidth = %u) with SNR = %d dB and errors %u%%", type, bandwidth, snr_db_value, errors);
	switch (type) {
	case DspController::modempacket_HshakeReceiv: {
		switch (ale.phase) {
		case ALE_TX_CALL_RX_HSHAKE: {
			ale.timerCallRoffHshakeR->stop();
			if (evaluatePacketSNR(snr_db_value)) {
				dispatcher->dsp_controller->enableModemTransmitter();
				setAlePhase(ALE_TX_CALL_TX_HSHAKE);
			} else {
				dispatcher->dsp_controller->disableModemReceiver();
				proceedTxCalling();
			}
			break;
		}
		case ALE_TX_NEG_RX_HSHAKE: {
			ale.timerNegRoffHshakeReceiv[ale.rcount]->stop();
			dispatcher->dsp_controller->enableModemTransmitter();
			setAlePhase(ALE_TX_NEG_TX_HSHAKE);
			break;
		}
		default:
			qmDebugMessage(QmDebug::Info, "ale received unexpected modempacket_HshakeReceiv");
			break;
		}
		break;
	}
	case DspController::modempacket_RespCallQual: {
		if (!(data_len >= 2))
			break;
		if (ale.phase != ALE_TX_NEG_RX_QUAL) {
			qmDebugMessage(QmDebug::Info, "ale received unexpected modempacket_RespCallQual");
			break;
		}
		ale.timerNegRoffRespCallQual[ale.rcount]->stop();
		dispatcher->dsp_controller->enableModemTransmitter();
		setAlePhase(ALE_TX_NEG_TX_MODE);
		break;
	}
	case DspController::modempacket_RespPackQual: {
		switch (ale.phase) {
		case ALE_TX_VM_RX_MSG_RESP: {
			ale.timerMsgRoffRespPackQual[ale.vm_msg_cycle-1]->stop();
			setAlePhase(ALE_TX_VM_TX_MSG_HSHAKE);
			dispatcher->dsp_controller->enableModemTransmitter();
			break;
		}
		case ALE_TX_VM_RX_PACK_RESP: {
			if (!(data_len >= 2))
				break;
			resppackqual_packet_t resppackqual_packet;
			resppackqual_packet.packResult = (data[0] >> 7) & 0x01;
			resppackqual_packet.SNR = (data[0] >> 1) & 0x3F;
			processPacketTxResponse(resppackqual_packet.packResult, resppackqual_packet.SNR);
			break;
		}
		default:
			qmDebugMessage(QmDebug::Info, "ale received unexpected modempacket_RespPackQual");
			break;
		}
		break;
	}
	case DspController::modempacket_LinkRelease: {
		if (!((ale.phase == ALE_TX_VM_RX_PACK_RESP) && (ale.vm_f_idx == (ale.vm_f_count - 1)))) {
			qmDebugMessage(QmDebug::Info, "ale received unexpected modempacket_LinkRelease");
			break;
		}
		ale.timerPacketRoffRespPackQual->stop();
		ale.timerTxPacketTxLinkRelease->start(ale.tPacketSync, TIMER_VALUE_tDataTxLinkReleaseDelay(ale.vm_sform_c));
		dispatcher->dsp_controller->enableModemTransmitter();
		setAlePhase(ALE_TX_VM_TX_LINK_RELEASE);
		break;
	}
	case DspController::modempacket_Call: {
		if (!(data_len >= 2))
			break;
		if (ale.phase != ALE_RX_CALL) {
			qmDebugMessage(QmDebug::Info, "ale received unexpected modempacket_Call");
			break;
		}
		ale.tCallStartSync.set();
		ale.timerRoffCall->stop();
		ale.timerCallTxHshakeR->start(ale.tCallStartSync, TIMER_VALUE_tCallTxHshakeR_offset);
		ale.timerCallRonHshakeT->start(ale.tCallStartSync, TIMER_VALUE_tCallRonHshakeT_offset);
		ale.timerCallRoffHshakeT->start(ale.tCallStartSync, TIMER_VALUE_tCallRoffHshakeT_offset);
		for (int i = 0; i < 3; i++) {
			int neg_start = TIMER_VALUE_tNegStart(i);
			ale.timerRxNegStart[i]->start(ale.tCallStartSync, neg_start);
			ale.timerNegTxRespCallQual[i]->start(ale.tCallStartSync, (neg_start + TIMER_VALUE_tNegTxRespCallQual_offset));
			ale.timerNegRonHshakeTransMode[i]->start(ale.tCallStartSync, (neg_start + TIMER_VALUE_tNegRonHshakeTransMode_offset));
			ale.timerNegRoffHshakeTransMode[i]->start(ale.tCallStartSync, (neg_start + TIMER_VALUE_tNegRoffHshakeTransMode_offset));
			ale.timerNegTxHshakeReceiv[i]->start(ale.tCallStartSync, (neg_start + TIMER_VALUE_tNegTxHshakeReceiv_offset));
			ale.timerNegRonHshakeTrans[i]->start(ale.tCallStartSync, (neg_start + TIMER_VALUE_tNegRonHshakeTrans_offset));
			ale.timerNegRoffHshakeTrans[i]->start(ale.tCallStartSync, (neg_start + TIMER_VALUE_tNegRoffHshakeTrans_offset));
		}
		call_packet_t call_packet;
		call_packet.lineType = (data[0] >> 7) & 0x01;
		call_packet.cycleNum = (data[0] >> 3) & 0x0F;
		call_packet.respAddr = ((data[0] & 0x07) << 2) | ((data[1] >> 6) & 0x03);
		if (!((call_packet.lineType == 1) && (call_packet.cycleNum >= 1) && (call_packet.cycleNum <= 3) && (call_packet.respAddr == ale.station_address))) {
			qmDebugMessage(QmDebug::Info, "ale rejecting Call due to parameters mismatch (lineType = %u, cycleNum = %u, respAddr = %u)", call_packet.lineType, call_packet.cycleNum, call_packet.respAddr);
			dispatcher->dsp_controller->disableModemReceiver();
			proceedRxScanning();
			break;
		}
		ale.supercycle = call_packet.cycleNum;
		if (!evaluatePacketSNR(snr_db_value)) {
			qmDebugMessage(QmDebug::Info, "ale rejecting Call due to bad SNR = %d dB", snr_db_value);
			dispatcher->dsp_controller->disableModemReceiver();
			proceedRxScanning();
			break;
		}
		ale.call_snr = snr;
		dispatcher->dsp_controller->enableModemTransmitter();
		dispatcher->dsp_controller->setModemReceiverBandwidth(bandwidth);
		dispatcher->dsp_controller->setModemReceiverPhase(DspController::modemphaseALE);
		setAlePhase(ALE_RX_CALL_TX_HSHAKE);
		break;
	}
	case DspController::modempacket_HshakeTrans: {
		switch (ale.phase) {
		case ALE_RX_CALL_RX_HSHAKE: {
			ale.timerCallRoffHshakeT->stop();
			dispatcher->dsp_controller->disableModemReceiver();
			setAleState(AleState_RX_CALL_NEGOTIATING);
			ale.rcount = 0;
			break;
		}
		case ALE_RX_NEG_RX_HSHAKE: {
			stopAleRxTimers();
			dispatcher->dsp_controller->disableModemReceiver();
			startVmRx();
			break;
		}
		case ALE_RX_VM_RX_MSG_HSHAKE: {
			dispatcher->dsp_controller->disableModemReceiver();
			setPacketRxPhase();
			ale.timerPacketRoffHead->start(ale.tPacketSync, TIMER_VALUE_tDataRoffSyncHeadDelay(ale.vm_sform_c));
			dispatcher->dsp_controller->enableModemReceiver();
			break;
		}
		case ALE_RX_VM_RX_PACK_HSHAKE: {
			ale.timerPacketRoffHshakeT->stop();
			dispatcher->dsp_controller->disableModemReceiver();
			processPacketReceivedAck();
			break;
		}
		default:
			break;
		}
		break;
	}
	case DspController::modempacket_HshakeTransMode: {
		if (!(data_len >= 3))
			break;
		if (ale.phase != ALE_RX_NEG_RX_MODE) {
			qmDebugMessage(QmDebug::Info, "ale received unexpected modempacket_HshakeTransMode");
			break;
		}
		ale.timerNegRoffHshakeTransMode[ale.rcount]->stop();
		hshaketransmode_packet_t hshaketransmode_packet;
		hshaketransmode_packet.soundType = (data[0] >> 6) & 0x03;
		hshaketransmode_packet.workMode = (data[0] >> 2) & 0x0F;
		hshaketransmode_packet.paramMode = ((data[0] & 0x03) << 4) | ((data[1] >> 4) & 0x0F);
		hshaketransmode_packet.schedule = (data[1] >> 3) & 0x01;
		hshaketransmode_packet.callAddr = ((data[1] & 0x07) << 2) | ((data[2] >> 6) & 0x03);
		if (!(((hshaketransmode_packet.soundType == 0) || (hshaketransmode_packet.soundType == 3))
				&& (hshaketransmode_packet.schedule == 1)
				&& (hshaketransmode_packet.workMode == 3) && (hshaketransmode_packet.paramMode == 1))) {
			qmDebugMessage(QmDebug::Info, "ale rejecting unsupported HshakeTransMode (soundType = %u, workMode = %u, paramMode = %u, schedule = %u)", hshaketransmode_packet.soundType, hshaketransmode_packet.workMode, hshaketransmode_packet.paramMode, hshaketransmode_packet.schedule);
			setAleState(AleState_RX_CALL_FAIL_UNSUPPORTED);
			stopAleSession();
			break;
		}
		ale.address = hshaketransmode_packet.callAddr;
		setAlePhase(ALE_RX_NEG_TX_HSHAKE);
		dispatcher->dsp_controller->enableModemTransmitter();
		break;
	}
	case DspController::modempacket_msgHead: {
		if (!(data_len >= 2))
			break;
		if (ale.phase != ALE_RX_VM_RX_MSGHEAD) {
			qmDebugMessage(QmDebug::Info, "ale received unexpected modempacket_msgHead");
			break;
		}
		msghead_packet_t msghead_packet;
		msghead_packet.msgSize = ((data[0] & 0xFF) << 3) | ((data[1] >> 5) & 0x07);
		msghead_packet.symCode = (data[1] >> 4) & 0x01;
		if (!((msghead_packet.msgSize >= 1) && (msghead_packet.msgSize <= 254) && (msghead_packet.symCode == 0))) {
			qmDebugMessage(QmDebug::Info, "ale rejecting unsupported msgHead (msgSize = %u, symCode = %u)", msghead_packet.msgSize, msghead_packet.symCode);
			setAleState(AleState_RX_VM_FAIL_UNSUPPORTED);
			stopAleSession();
			break;
		}
		ale.vm_size = msghead_packet.msgSize;
		ale.vm_f_count = ceilf((float)ale.vm_size*72/490);
		ale.vm_fragments.resize(ale.vm_f_count);
		qmDebugMessage(QmDebug::Info, "ale expected voice message parameters: %d groups, %d packets", ale.vm_size, ale.vm_f_count);
		ale.timerMsgRoffHead[ale.vm_msg_cycle-1]->stop();
		setAlePhase(ALE_RX_VM_TX_MSG_RESP);
		dispatcher->dsp_controller->enableModemTransmitter();
		break;
	}
	case DspController::modempacket_packHead: {
		if (!(data_len == 66))
			break;
		if (ale.phase != ALE_RX_VM_RX_PACKET) {
			qmDebugMessage(QmDebug::Info, "ale received unexpected VM packet data");
			break;
		}
		if (processPacketReceivedPacket(data)) {
			if (ale.vm_f_idx == ale.vm_f_count) {
				startRxPacketLinkRelease();
				break;
			}
			ale.vm_packet_result = true;
		} else {
			ale.vm_packet_result = false;
		}
		startRxPacketResponse();
		break;
	}
	default:
		break;
	}
}

void MainServiceInterface::aleprocessModemPacketStartedRxPackHead(uint8_t snr, uint8_t errors, DspController::ModemBandwidth bandwidth, uint8_t param_signForm, uint8_t param_packCode, uint8_t* data, int data_len) {
	int8_t snr_db_value = convertSnrFromPacket(snr);
	qmDebugMessage(QmDebug::Info, "ale received modem VM packHead (bandwidth = %u, signForm = %u, param_packCode = %u) with SNR = %d dB and errors %u%%", bandwidth, param_signForm, param_packCode, snr_db_value, errors);
	if (!(param_packCode == 0)) {
		qmDebugMessage(QmDebug::Info, "...rejecting due to unsupported packCode");
		dispatcher->dsp_controller->disableModemReceiver();
		return;
	}
	switch (ale.phase) {
	case ALE_RX_VM_RX_MSGHEAD: {
		if (!(ale.vm_msg_cycle > 1)) {
			qmDebugMessage(QmDebug::Info, "...rejecting impossible case (unexpected during first cycle)");
			dispatcher->dsp_controller->disableModemReceiver();
			return;
		}
		ale.vm_msg_cycle--;
		setPacketRxPhase();
		break;
	}
	case ALE_RX_VM_RX_PACKET: {
		ale.timerPacketRoffHead->stop();
		break;
	}
	default:
		dispatcher->dsp_controller->disableModemReceiver();
		return;
	}
	ale.vm_sform_n = param_signForm;
	ale.rcount = 0;
	ale.vm_packet_snr = snr;
}

void MainServiceInterface::aleprocessModemPacketFailedRx(DspController::ModemPacketType type) {
	if (ale.phase == ALE_RX_VM_RX_PACKET) {
		QM_ASSERT(type == DspController::modempacket_packHead);
		qmDebugMessage(QmDebug::Info, "ale VM packet data rx failed");
		ale.vm_packet_result = false;
		startRxPacketResponse();
	}
}

void MainServiceInterface::aleprocess1PPS() {
	if (ale.phase == ALE_STOPPED)
		return;
	ale.timerGnssSync->start(1500);
	switch (ale.phase) {
	case ALE_TX_CYCLE_CALL: {
		int freq_idx;
		if (!checkDwellStart(freq_idx))
			break;
		ale.tCallStartSync.set();
		dispatcher->navigator->setMinimalActivityMode(true);
		qmDebugMessage(QmDebug::Info, "ale dwell start (tx call, frequency %lu Hz)", ale.call_freqs[freq_idx]);
		ale.timerTxCall->start(ale.tCallStartSync, TIMER_VALUE_tTxCall);
		ale.timerCallRonHshakeR->start(ale.tCallStartSync, TIMER_VALUE_tRoffSyncCall + TIMER_VALUE_tCallRonHshakeR_offset);
		ale.timerCallRoffHshakeR->start(ale.tCallStartSync, TIMER_VALUE_tRoffSyncCall + TIMER_VALUE_tCallRoffHshakeR_offset);
		ale.timerCallTxHshakeT->start(ale.tCallStartSync, TIMER_VALUE_tRoffSyncCall + TIMER_VALUE_tCallTxHshakeT_offset);
		for (int i = 0; i < 3; i++) {
			int neg_start = TIMER_VALUE_tRoffSyncCall + TIMER_VALUE_tNegStart(i);
			ale.timerTxNegStart[i]->start(ale.tCallStartSync, neg_start);
			ale.timerNegRoffRespCallQual[i]->start(ale.tCallStartSync, (neg_start + TIMER_VALUE_tNegRoffRespCallQual_offset));
			ale.timerNegTxHshakeTransMode[i]->start(ale.tCallStartSync, (neg_start + TIMER_VALUE_tNegTxHshakeTransMode_offset));
			ale.timerNegRonHshakeReceiv[i]->start(ale.tCallStartSync, (neg_start + TIMER_VALUE_tNegRonHshakeReceiv_offset));
			ale.timerNegRoffHshakeReceiv[i]->start(ale.tCallStartSync, (neg_start + TIMER_VALUE_tNegRoffHshakeReceiv_offset));
			ale.timerNegTxHshakeTrans[i]->start(ale.tCallStartSync, (neg_start + TIMER_VALUE_tNegTxHshakeTrans_offset));
		}
		dispatcher->dsp_controller->tuneModemFrequency(ale.call_freqs[freq_idx]);
		dispatcher->dsp_controller->enableModemTransmitter();
		setAlePhase(ALE_TX_CALL);
		ale.rcount = 0;
		break;
	}
	case ALE_RX_SCAN: {
		int freq_idx;
		if (!checkDwellStart(freq_idx))
			break;
		dispatcher->navigator->setMinimalActivityMode(true);
		qmDebugMessage(QmDebug::Info, "ale dwell start (rx scan, frequency %lu Hz)", ale.call_freqs[freq_idx]);
		ale.timerRoffCall->start(ALE_TIME_dTSyn + TIMER_VALUE_tRoffSyncCall);
		dispatcher->dsp_controller->tuneModemFrequency(ale.call_freqs[freq_idx]);
		dispatcher->dsp_controller->enableModemReceiver();
		setAlePhase(ALE_RX_CALL);
		break;
	}
	default:
		break;
	}
}

void MainServiceInterface::aleprocessTimerDataStartExpired() {
	switch (ale.phase) {
	case ALE_RX_VM_START: {
		for (int i = 0; i < 3; i++) {
			int cycle = i*TIMER_VALUE_tDataCycle(-1);
			ale.timerMsgRoffHead[i]->start(ale.tPacketSync, (cycle + TIMER_VALUE_tDataRoffSyncHeadDelay(-1)));
			ale.timerMsgTxRespPackQual[i]->start(ale.tPacketSync, (cycle + TIMER_VALUE_tDataTxRespPackQualDelay(-1)));
			ale.timerMsgRonHshakeT[i]->start(ale.tPacketSync, (cycle + TIMER_VALUE_tDataRonHshakeTDelay(-1)));
			ale.timerMsgRoffHshakeT[i]->start(ale.tPacketSync, (cycle + TIMER_VALUE_tDataRoffHshakeTDelay(-1)));
			ale.timerRxMsgCycle[i]->start(ale.tPacketSync, cycle);
		}
		break;
	}
	case ALE_TX_VM_START: {
		for (int i = 0; i < 3; i++) {
			int cycle = i*TIMER_VALUE_tDataCycle(-1);
			ale.timerMsgTxHead[i]->start(ale.tPacketSync, (cycle + TIMER_VALUE_tDataTxHeadDelay(-1)));
			ale.timerMsgRonRespPackQual[i]->start(ale.tPacketSync, (cycle + TIMER_VALUE_tDataRonRespPackQualDelay(-1)));
			ale.timerMsgRoffRespPackQual[i]->start(ale.tPacketSync, (cycle + TIMER_VALUE_tDataRoffRespPackQualDelay(-1)));
			ale.timerMsgTxHshakeT[i]->start(ale.tPacketSync, (cycle + TIMER_VALUE_tDataTxHshakeTDelay(-1)));
			ale.timerTxMsgCycle[i]->start(ale.tPacketSync, cycle);
		}
		break;
	}
	default:
		QM_ASSERT(0);
	}
}

void MainServiceInterface::aleprocessTimerGnssSyncExpired() {
	if ((ale.phase != ALE_TX_CYCLE_CALL) && (ale.phase != ALE_RX_SCAN))
		return;
	setAleState(AleState_FAIL_GNSS);
	stopAleSession();
}

void MainServiceInterface::aleprocessTimerTxCallExpired() {
	call_packet_t call_packet;
	call_packet.lineType = 1;
	call_packet.cycleNum = ale.supercycle;
	call_packet.respAddr = ale.address;
	uint8_t data[2] = {0, 0};
	data[0] |= (call_packet.lineType & 0x01) << 7;
	data[0] |= (call_packet.cycleNum & 0x0F) << 3;
	data[0] |= (call_packet.respAddr & 0x1F) >> 2;
	data[1] |= (call_packet.respAddr & 0x1F) << 6;
	dispatcher->dsp_controller->sendModemPacket(DspController::modempacket_Call, DspController::modembw20kHz, data, sizeof(data));
}

void MainServiceInterface::aleprocessTimerCallRonHshakeRExpired() {
	dispatcher->dsp_controller->enableModemReceiver();
	setAlePhase(ALE_TX_CALL_RX_HSHAKE);
}

void MainServiceInterface::aleprocessTimerCallRoffHshakeRExpired() {
	qmDebugMessage(QmDebug::Info, "ale rx HshakeReceiv timeout");
	dispatcher->dsp_controller->disableModemReceiver();
	proceedTxCalling();
}

void MainServiceInterface::aleprocessTimerTxNegStartExpired() {
	setAlePhase(ALE_TX_NEG_RX_QUAL);
	dispatcher->dsp_controller->enableModemReceiver();
}

void MainServiceInterface::aleprocessTimerNegRoffExpired() {
	qmDebugMessage(QmDebug::Info, "ale rx RespCallQual/HshakeTrans timeout");
	switch (ale.phase) {
	case ALE_TX_NEG_RX_QUAL:
	case ALE_TX_NEG_RX_HSHAKE: {
		dispatcher->dsp_controller->disableModemReceiver();
		ale.rcount++;
		if (ale.rcount < 3) {
			setAlePhase(ALE_TX_NEG_IDLE);
			ale.timerNegTxHshakeTransMode[ale.rcount-1]->stop();
			ale.timerNegRonHshakeReceiv[ale.rcount-1]->stop();
			ale.timerNegRoffHshakeReceiv[ale.rcount-1]->stop();
			ale.timerNegTxHshakeTrans[ale.rcount-1]->stop();
		} else {
			proceedTxCalling();
		}
		break;
	}
	default:
		break;
	}
}

void MainServiceInterface::aleprocessTimerNegTxHshakeTransModeExpired() {
	hshaketransmode_packet_t hshaketransmode_packet;
	hshaketransmode_packet.soundType = 0;
	hshaketransmode_packet.workMode = 3;
	hshaketransmode_packet.paramMode = 1;
	hshaketransmode_packet.schedule = 1;
	hshaketransmode_packet.callAddr = ale.station_address;
	uint8_t data[3] = {0, 0, 0};
	data[0] |= (hshaketransmode_packet.soundType & 0x03) << 6;
	data[0] |= (hshaketransmode_packet.workMode & 0x0F) << 2;
	data[0] |= (hshaketransmode_packet.paramMode & 0x3F) >> 4;
	data[1] |= (hshaketransmode_packet.paramMode & 0x3F) << 4;
	data[1] |= (hshaketransmode_packet.schedule & 0x01) << 3;
	data[1] |= (hshaketransmode_packet.callAddr & 0x1F) >> 2;
	data[2] |= (hshaketransmode_packet.callAddr & 0x1F) << 6;
	dispatcher->dsp_controller->sendModemPacket(DspController::modempacket_HshakeTransMode, DspController::modembw20kHz, data, sizeof(data));
}

void MainServiceInterface::aleprocessTimerNegRonHshakeReceivExpired() {
	dispatcher->dsp_controller->enableModemReceiver();
	setAlePhase(ALE_TX_NEG_RX_HSHAKE);
}

void MainServiceInterface::aleprocessTimerTxHshakeTransExpired() {
	dispatcher->dsp_controller->sendModemPacket(DspController::modempacket_HshakeTrans, DspController::modembw20kHz, 0, 0);
}

void MainServiceInterface::aleprocessTimerMsgTxHeadExpired() {
	msghead_packet_t msghead_packet;
	msghead_packet.msgSize = ale.vm_size;
	msghead_packet.symCode = 0;
	uint8_t data[2] = {0, 0};
	data[0] |= (msghead_packet.msgSize & 0x7FF) >> 3;
	data[1] |= (msghead_packet.msgSize & 0x7FF) << 5;
	data[1] |= (msghead_packet.symCode & 0x1) << 4;
	dispatcher->dsp_controller->sendModemPacket(DspController::modempacket_msgHead, DspController::modembw20kHz, data, sizeof(data));
}

void MainServiceInterface::aleprocessTimerMsgRonRespPackQualExpired() {
	dispatcher->dsp_controller->enableModemReceiver();
	setAlePhase(ALE_TX_VM_RX_MSG_RESP);
}

void MainServiceInterface::aleprocessTimerMsgRoffRespPackQualExpired() {
	qmDebugMessage(QmDebug::Info, "ale rx RespPackQual timeout");
	dispatcher->dsp_controller->disableModemReceiver();
	ale.timerMsgTxHshakeT[ale.vm_msg_cycle-1]->stop();
	if (ale.vm_msg_cycle < 3) {
		setAlePhase(ALE_TX_VM_MSG_IDLE);
	} else {
		setAleState(AleState_TX_VM_FAIL);
		stopAleSession();
	}
}

void MainServiceInterface::aleprocessTimerMsgTxHshakeTExpired() {
	dispatcher->dsp_controller->sendModemPacket(DspController::modempacket_HshakeTrans, DspController::modembw20kHz, 0, 0);
}

void MainServiceInterface::aleprocessTimerTxMsgCycleExpired() {
	qmDebugMessage(QmDebug::Info, "ale vm msg cycle start");
	ale.vm_msg_cycle++;
	setAlePhase(ALE_TX_VM_TX_MSGHEAD);
	dispatcher->dsp_controller->enableModemTransmitter();
}

void MainServiceInterface::aleprocessTxPacketSync() {
	qmDebugMessage(QmDebug::Info, "ale vm packet cycle start");
	dispatcher->dsp_controller->enableModemTransmitter();
	ale.vm_adaptation = alevmadaptationNone;
	setAlePhase(ALE_TX_VM_TX_PACKET);
	ale.timerPacketTxHeadData->start(ale.tPacketSync, TIMER_VALUE_tDataTxHeadDelay(ale.vm_sform_c));
}

void MainServiceInterface::processPacketTxResponse(bool p_result, uint8_t p_snr) {
	int snr_db_value = convertSnrFromPacket(p_snr);
	qmDebugMessage(QmDebug::Info, "processing ale packet response (RespPackQual) with result = %s and SNR = %d dB", (p_result)?"OK":"FAIL", snr_db_value);
	ale.timerPacketRoffRespPackQual->stop();
	if ((ale.vm_f_idx == (ale.vm_f_count - 1)) && (p_result != false)) {
		qmDebugMessage(QmDebug::Info, "ale vm last packet response is bad");
		dispatcher->dsp_controller->disableModemReceiver();
		processFailedPacketTxCycle();
		return;
	}
	if (p_result) {
		ale.vm_f_idx++;
		setAleVmProgress((int)(100*ale.vm_f_idx/ale.vm_f_count));
		ale.vm_snr_ack[ale.vm_ack_count] = snr_db_value;
		ale.vm_nack_count = 0;
		ale.vm_ack_count++;
		if (ale.vm_ack_count == 3)
			ale.vm_adaptation = alevmadaptationUp;
	} else {
		ale.vm_snr_nack[ale.vm_nack_count] = snr_db_value;
		ale.vm_ack_count = 0;
		ale.vm_nack_count++;
		if (ale.vm_nack_count == 2)
			ale.vm_adaptation = alevmadaptationDown;
	}
	ale.timerPacketTxHshakeT->start(ale.tPacketSync, TIMER_VALUE_tDataTxHshakeTDelay(ale.vm_sform_c));
	dispatcher->dsp_controller->enableModemTransmitter();
	setAlePhase(ALE_TX_VM_TX_PACK_HSHAKE);
}

void MainServiceInterface::processFailedPacketTxCycle() {
	ale.rcount++;
	if (ale.rcount < 3) {
		ale.vm_ack_count = 0;
		startNextPacketTxCycle();
	} else {
		setAleState(AleState_TX_VM_COMPLETE_PARTIAL);
		stopAleSession();
	}
}

void MainServiceInterface::startNextPacketTxCycle() {
//	int sform = (ale.vm_sform_p > ale.vm_sform_c)?ale.vm_sform_p:ale.vm_sform_c;
	int sform = ale.vm_sform_p;
	ale.timerTxPacketSync->start(ale.tPacketSync, TIMER_VALUE_tDataCycle(sform));
	ale.tPacketSync.shift(TIMER_VALUE_tDataCycle(sform));
}

void MainServiceInterface::adaptPacketTxUp() {
	int snr_e = qmMin(qmMin(ale.vm_snr_ack[0], ale.vm_snr_ack[1]), ale.vm_snr_ack[2]);
	if (!(snr_e <= ale_vm_snr_table[ale.vm_sform_c])) {
		int sform = (ale.vm_sform_c < 4)?0:4;
		while (!(ale_vm_snr_table[sform] <= snr_e))
			sform++;
		if (sform < ale.vm_sform_c) {
			ale.vm_sform_c = sform;
			ale.vm_ack_count = 0;
			return;
		}
	}
	ale.vm_ack_count--;
	ale.vm_snr_ack[0] = ale.vm_snr_ack[1];
	ale.vm_snr_ack[1] = ale.vm_snr_ack[2];
}

bool MainServiceInterface::adaptPacketTxDown() {
	if (ale.vm_sform_c == 7) {
		ale.vm_nack_count--;
		return false;
	}
	int snr_e = qmMin(ale.vm_snr_nack[0], ale.vm_snr_nack[1]);
	ale.vm_nack_count = 0;
	ale.vm_sform_c++;
	while ((ale_vm_snr_table[ale.vm_sform_c] > snr_e) && (ale.vm_sform_c < 7)) {
		ale.vm_sform_c++;
		if (ale.vm_sform_c == 2)
			ale.vm_sform_c = 3;
	}
	return true;
}

void MainServiceInterface::aleprocessTimerPacketTxHeadDataExpired() {
	dispatcher->dsp_controller->sendModemPacket_packHead(DspController::modembw20kHz, ale.vm_sform_c, 0, (uint8_t *)&(ale.vm_fragments[ale.vm_f_idx]), sizeof(ale.vm_fragments[0]));
}

void MainServiceInterface::aleprocessTimerPacketRonRespPackQualExpired() {
	dispatcher->dsp_controller->enableModemReceiver();
	ale.timerPacketRoffRespPackQual->start(ale.tPacketSync, TIMER_VALUE_tDataRoffRespPackQualDelay(ale.vm_sform_c));
	setAlePhase(ALE_TX_VM_RX_PACK_RESP);
}

void MainServiceInterface::aleprocessTimerPacketRoffRespPackQualExpired() {
	qmDebugMessage(QmDebug::Info, "ale rx RespPackQual timeout");
	setAlePhase(ALE_TX_VM_PACK_IDLE);
	dispatcher->dsp_controller->disableModemReceiver();
	processFailedPacketTxCycle();
}

void MainServiceInterface::aleprocessTimerPacketTxHshakeTExpired() {
	dispatcher->dsp_controller->sendModemPacket(DspController::modempacket_HshakeTrans, DspController::modembw20kHz, 0, 0);
}

void MainServiceInterface::aleprocessTimerTxPacketTxLinkReleaseExpired() {
	dispatcher->dsp_controller->sendModemPacket(DspController::modempacket_LinkRelease, DspController::modembw20kHz, 0, 0);
}

void MainServiceInterface::aleprocessTimerRoffCallExpired() {
	qmDebugMessage(QmDebug::Info, "ale rx Call timeout");
	dispatcher->dsp_controller->disableModemReceiver();
	proceedRxScanning();
}

void MainServiceInterface::aleprocessTimerCallTxHshakeRExpired() {
	dispatcher->dsp_controller->sendModemPacket(DspController::modempacket_HshakeReceiv, DspController::modembw20kHz, 0, 0);
}

void MainServiceInterface::aleprocessTimerCallRonHshakeTExpired() {
	dispatcher->dsp_controller->enableModemReceiver();
	setAlePhase(ALE_RX_CALL_RX_HSHAKE);
}

void MainServiceInterface::aleprocessTimerCallRoffHshakeTExpired() {
	qmDebugMessage(QmDebug::Info, "ale rx HshakeTrans timeout");
	dispatcher->dsp_controller->disableModemReceiver();
	proceedRxScanning();
}

void MainServiceInterface::aleprocessTimerRxNegStartExpired() {
	setAlePhase(ALE_RX_NEG_TX_QUAL);
	dispatcher->dsp_controller->enableModemTransmitter();
}

void MainServiceInterface::aleprocessTimerNegTxRespCallQualExpired() {
	respcallqual_packet_t respcallqual_packet;
	respcallqual_packet.errSignal = 0;
	respcallqual_packet.SNR = ale.call_snr;
	uint8_t data[2] = {0, 0};
	data[0] |= (respcallqual_packet.errSignal & 0x1F) << 3;
	data[0] |= (respcallqual_packet.SNR & 0x3F) >> 3;
	data[1] |= (respcallqual_packet.SNR & 0x3F) << 5;
	dispatcher->dsp_controller->sendModemPacket(DspController::modempacket_RespCallQual, DspController::modembw20kHz, data, sizeof(data));
}

void MainServiceInterface::aleprocessTimerNegRonHshakeTransModeExpired() {
	dispatcher->dsp_controller->enableModemReceiver();
	setAlePhase(ALE_RX_NEG_RX_MODE);
}

void MainServiceInterface::aleprocessTimerNegRoffHshakeTransModeExpired() {
	qmDebugMessage(QmDebug::Info, "ale rx HshakeTransMode timeout");
	dispatcher->dsp_controller->disableModemReceiver();
	ale.rcount++;
	if (ale.rcount < 3) {
		setAlePhase(ALE_RX_NEG_IDLE);
		ale.timerNegTxHshakeReceiv[ale.rcount-1]->stop();
		ale.timerNegRonHshakeTrans[ale.rcount-1]->stop();
		ale.timerNegRoffHshakeTrans[ale.rcount-1]->stop();
	} else {
		proceedRxScanning();
	}
}

void MainServiceInterface::aleprocessTimerNegTxHshakeReceivExpired() {
	dispatcher->dsp_controller->sendModemPacket(DspController::modempacket_HshakeReceiv, DspController::modembw20kHz, 0, 0);
}

void MainServiceInterface::aleprocessTimerNegRonHshakeTransExpired() {
	dispatcher->dsp_controller->enableModemReceiver();
	setAlePhase(ALE_RX_NEG_RX_HSHAKE);
}

void MainServiceInterface::aleprocessTimerNegRoffHshakeTransExpired() {
	qmDebugMessage(QmDebug::Info, "ale rx HshakeTrans timeout");
	dispatcher->dsp_controller->disableModemReceiver();
	ale.rcount++;
	if (ale.rcount < 3) {
		setAlePhase(ALE_RX_NEG_IDLE);
	} else {
		proceedRxScanning();
	}
}

void MainServiceInterface::aleprocessTimerMsgRoffHeadExpired() {
	qmDebugMessage(QmDebug::Info, "ale rx msgHead timeout");
	dispatcher->dsp_controller->disableModemReceiver();
	ale.timerMsgTxRespPackQual[ale.vm_msg_cycle-1]->stop();
	ale.timerMsgRonHshakeT[ale.vm_msg_cycle-1]->stop();
	ale.timerMsgRoffHshakeT[ale.vm_msg_cycle-1]->stop();
	if (ale.vm_msg_cycle < 3) {
		setAlePhase(ALE_RX_VM_MSG_IDLE);
	} else {
		setAleState(AleState_RX_VM_FAIL);
		stopAleSession();
	}
}

void MainServiceInterface::aleprocessTimerMsgTxRespPackQualExpired() {
	resppackqual_packet_t resppackqual_packet;
	resppackqual_packet.packResult = 1;
	resppackqual_packet.SNR = 0;
	uint8_t data[2] = {0, 0};
	data[0] |= (resppackqual_packet.packResult & 0x1) << 7;
	data[0] |= (resppackqual_packet.SNR & 0x3F) << 1;
	dispatcher->dsp_controller->sendModemPacket(DspController::modempacket_RespPackQual, DspController::modembw20kHz, data, sizeof(data));
}

void MainServiceInterface::aleprocessTimerMsgRonHshakeTExpired() {
	setAlePhase(ALE_RX_VM_RX_MSG_HSHAKE);
	dispatcher->dsp_controller->enableModemReceiver();
}

void MainServiceInterface::aleprocessTimerMsgRoffHshakeTExpired() {
	qmDebugMessage(QmDebug::Info, "ale rx HshakeTrans timeout");
	if (ale.vm_msg_cycle < 3) {
		setAlePhase(ALE_RX_VM_MSG_IDLE);
		dispatcher->dsp_controller->disableModemReceiver();
	} else {
		setAleState(AleState_RX_VM_FAIL);
		stopAleSession();
	}
}

void MainServiceInterface::aleprocessTimerRxMsgCycleExpired() {
	qmDebugMessage(QmDebug::Info, "ale vm msg cycle start");
	ale.vm_msg_cycle++;
	setAlePhase(ALE_RX_VM_RX_MSGHEAD);
	dispatcher->dsp_controller->enableModemReceiver();
}

void MainServiceInterface::setPacketRxPhase() {
	qmDebugMessage(QmDebug::Info, "ale vm set packet phase");
	ale.tPacketSync.shift(ale.vm_msg_cycle*TIMER_VALUE_tDataCycle(-1));
	ale.vm_sform_c = ALE_VM_INITIAL_SFORM;
	ale.vm_sform_p = ale.vm_sform_c;
	ale.vm_f_idx = 0;
	ale.vm_last_result = true;
	ale.vm_ack_count = 0;
	ale.vm_nack_count = 0;
	setAlePhase(ALE_RX_VM_RX_PACKET);
	ale.result = AleResultVoiceMail;
	stopVmMsgRxTimers();
}

void MainServiceInterface::processFailedPacketRxCycle() {
	ale.rcount++;
	if (ale.rcount < 3) {
		ale.vm_ack_count = 0;
	} else {
		setAleState(AleState_RX_VM_COMPLETE_PARTIAL);
		stopAleSession();
	}
}

bool MainServiceInterface::processPacketReceivedPacket(uint8_t *data) {
	AleVmPacket *packet = (AleVmPacket *)data;
	CRC32 crc;
	crc.update(packet->num_data, sizeof(packet->num_data));
	if (!(crc.result() == qmFromBigEndian<uint32_t>((uint8_t *)&(packet->crc)))) {
		qmDebugMessage(QmDebug::Info, "ale rx vm packet with bad CRC");
		return false;
	}
	uint8_t num = (packet->num_data[0] >> 2) & 0x3F;
	if(!(num == ale.vm_f_idx)) {
		if (num == (ale.vm_f_idx - 1)) {
			qmDebugMessage(QmDebug::Info, "ale rx vm packet repeated");
			return true;
		} else {
			qmDebugMessage(QmDebug::Info, "ale rx vm packet with bad number");
			return false;
		}
	}
	std::copy(packet->num_data, packet->num_data + sizeof(packet->num_data), ale.vm_fragments[ale.vm_f_idx].num_data);
	ale.vm_f_idx++;
	setAleVmProgress((int)(100*ale.vm_f_idx/ale.vm_f_count));
	return true;
}

void MainServiceInterface::processPacketMissedPacket() {
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	ale.timerRxPacketSync->start(ale.tPacketSync, TIMER_VALUE_tDataCycle(ale.vm_sform_c));
	ale.tPacketSync.shift(TIMER_VALUE_tDataCycle(ale.vm_sform_c));

	ale.vm_last_result = false;
	processFailedPacketRxCycle();
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
}

void MainServiceInterface::startRxPacketResponse() {
	dispatcher->dsp_controller->enableModemTransmitter();
	ale.vm_last_result = ale.vm_packet_result;
	setAlePhase(ALE_RX_VM_TX_PACK_RESP);
	ale.timerPacketTxRespPackQual->start(ale.tPacketSync, TIMER_VALUE_tDataTxRespPackQualDelay(ale.vm_sform_n));
}

void MainServiceInterface::startRxPacketLinkRelease() {
	ale.timerRxPacketTxLinkRelease->start(ale.tPacketSync, TIMER_VALUE_tDataTxRespPackQualDelay(ale.vm_sform_n));
	dispatcher->dsp_controller->enableModemTransmitter();
	setAlePhase(ALE_RX_VM_TX_LINK_RELEASE);
}

void MainServiceInterface::processPacketReceivedAck() {
	ale.vm_sform_c = ale.vm_sform_n;
	ale.timerRxPacketSync->start(ale.tPacketSync, TIMER_VALUE_tDataCycle(ale.vm_sform_c));
	ale.tPacketSync.shift(TIMER_VALUE_tDataCycle(ale.vm_sform_c));

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	ale.vm_adaptation = alevmadaptationNone;
	int snr_db_value = convertSnrFromPacket(ale.vm_packet_snr);
	if (ale.vm_packet_result) {
		ale.vm_snr_ack[ale.vm_ack_count] = snr_db_value;
		ale.vm_nack_count = 0;
		ale.vm_ack_count++;
		if (ale.vm_ack_count == 3)
			ale.vm_adaptation = alevmadaptationUp;
	} else {
		ale.vm_snr_nack[ale.vm_nack_count] = snr_db_value;
		ale.vm_ack_count = 0;
		ale.vm_nack_count++;
		if (ale.vm_nack_count == 2)
			ale.vm_adaptation = alevmadaptationDown;
	}
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	ale.vm_sform_p = ale.vm_sform_c;
	AleVmAdaptationType adaptation_required = ale.vm_adaptation;
	if (adaptation_required == alevmadaptationDown) {
		if (!adaptPacketTxDown()) {
			processFailedPacketRxCycle();
			return;
		}
	}
	if (adaptation_required == alevmadaptationUp)
		adaptPacketTxUp();
	ale.rcount = 0;
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
}

void MainServiceInterface::processPacketMissedAck() {
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	ale.timerRxPacketSync->start(ale.tPacketSync, TIMER_VALUE_tDataCycle(ale.vm_sform_c));
	ale.tPacketSync.shift(TIMER_VALUE_tDataCycle(ale.vm_sform_c));

	processFailedPacketRxCycle();
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
}

void MainServiceInterface::aleprocessRxPacketSync() {
	qmDebugMessage(QmDebug::Info, "ale vm packet cycle start");
	dispatcher->dsp_controller->enableModemReceiver();
	ale.timerPacketRoffHead->start(ale.tPacketSync, TIMER_VALUE_tDataRoffSyncHeadDelay(ale.vm_sform_c));
	setAlePhase(ALE_RX_VM_RX_PACKET);
}

void MainServiceInterface::aleprocessPacketRoffHeadExpired() {
	qmDebugMessage(QmDebug::Info, "ale rx packHead timeout");
	setAlePhase(ALE_RX_VM_PACK_IDLE);
	dispatcher->dsp_controller->disableModemReceiver();
	processPacketMissedPacket();
}

void MainServiceInterface::aleprocessPacketTxRespPackQualExpired() {
	resppackqual_packet_t resppackqual_packet;
	resppackqual_packet.packResult = ale.vm_packet_result;
	resppackqual_packet.SNR = ale.vm_packet_snr;
	uint8_t data[2] = {0, 0};
	data[0] |= (resppackqual_packet.packResult & 0x1) << 7;
	data[0] |= (resppackqual_packet.SNR & 0x3F) << 1;
	dispatcher->dsp_controller->sendModemPacket(DspController::modempacket_RespPackQual, DspController::modembw20kHz, data, sizeof(data));
}

void MainServiceInterface::aleprocessTimerRxPacketTxLinkReleaseExpired() {
	dispatcher->dsp_controller->sendModemPacket(DspController::modempacket_LinkRelease, DspController::modembw20kHz, 0, 0);
}

void MainServiceInterface::aleprocessPacketRonHshakeTExpired() {
	ale.timerPacketRoffHshakeT->start(ale.tPacketSync, TIMER_VALUE_tDataRoffHshakeTDelay(ale.vm_sform_n));
	dispatcher->dsp_controller->enableModemReceiver();
	setAlePhase(ALE_RX_VM_RX_PACK_HSHAKE);
}

void MainServiceInterface::aleprocessPacketRoffHshakeTExpired() {
	qmDebugMessage(QmDebug::Info, "ale rx HshakeTrans timeout");
	setAlePhase(ALE_RX_VM_PACK_IDLE);
	dispatcher->dsp_controller->disableModemReceiver();
	processPacketMissedAck();
}

} /* namespace Multiradio */

#include "qmdebug_domains_start.h"
QMDEBUG_DEFINE_DOMAIN(mrd_mainservice, LevelDefault)
#include "qmdebug_domains_end.h"
