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
#define QMDEBUGDOMAIN mrd_mainservice
#include "qmdebug.h"
#include "qmcrc.h"

#include "mainserviceinterface.h"
#include "dispatcher.h"

#include "ale_param_defs.h"

/* Расчет значений временных параметров */
#define TIMER_VALUE_tTxCall (ceil(0.5*ALE_TIME_dTdwell) + ALE_TIME_TEthTx + ALE_TIME_TTuneTx + ALE_TIME_TEthRx + ALE_TIME_TTuneRx + ALE_TIME_dTSyn + ALE_TIME_DTMistiming + ALE_TIME_TEthTx + ALE_TIME_TOpenTx)
#define TIMER_VALUE_tToffCall (TIMER_VALUE_tTxCall + ALE_TIME_TEthTx + ALE_TIME_TCall)
#define TIMER_VALUE_tRoffCall (TIMER_VALUE_tToffCall + ALE_TIME_TRChan + ALE_TIME_TEthRx)
#define TIMER_VALUE_tCallRonHshakeR (TIMER_VALUE_tRoffCall + ALE_TIME_TMaxEthX + ALE_TIME_TMaxOpenTuneX)
#define TIMER_VALUE_tCallTxHshakeR (TIMER_VALUE_tCallRonHshakeR + ALE_TIME_TMaxEthX + ALE_TIME_TMaxOpenTuneX)
#define TIMER_VALUE_tCallRoffHshakeR (TIMER_VALUE_tCallTxHshakeR + ALE_TIME_TEthTx + ALE_TIME_THshakeReceiv + ALE_TIME_TRChan + ALE_TIME_TEthRx)
#define TIMER_VALUE_tCallRonHshakeT (TIMER_VALUE_tCallRoffHshakeR + ALE_TIME_TMaxEthX + ALE_TIME_TMaxOpenTuneX)
#define TIMER_VALUE_tCallTxHshakeT (TIMER_VALUE_tCallRonHshakeT + ALE_TIME_TMaxEthX + ALE_TIME_TMaxOpenTuneX)
#define TIMER_VALUE_tCallRoffHshakeT (TIMER_VALUE_tCallTxHshakeT + ALE_TIME_TEthTx + ALE_TIME_THshakeTrans + ALE_TIME_TRChan + ALE_TIME_TEthRx)
#define TIMER_VALUE_tNegTxRespCallQual_offset (ALE_TIME_TMaxEthX + ALE_TIME_TMaxOpenTuneX)
#define TIMER_VALUE_tNegRoffRespCallQual_offset (TIMER_VALUE_tNegTxRespCallQual_offset + ALE_TIME_TEthTx + ALE_TIME_TRespCallQual + ALE_TIME_TRChan + ALE_TIME_TEthRx)
#define TIMER_VALUE_tNegRonHshakeTransMode_offset (TIMER_VALUE_tNegRoffRespCallQual_offset + ALE_TIME_TMaxEthX + ALE_TIME_TMaxOpenTuneX)
#define TIMER_VALUE_tNegTxHshakeTransMode_offset (TIMER_VALUE_tNegRonHshakeTransMode_offset + ALE_TIME_TMaxEthX + ALE_TIME_TMaxOpenTuneX)
#define TIMER_VALUE_tNegRoffHshakeTransMode_offset (TIMER_VALUE_tNegTxHshakeTransMode_offset + ALE_TIME_TEthTx + ALE_TIME_THshakeTransMode + ALE_TIME_TRChan + ALE_TIME_TEthRx)
#define TIMER_VALUE_tNegRonHshakeReceiv_offset (TIMER_VALUE_tNegRoffHshakeTransMode_offset + ALE_TIME_TMaxEthX + ALE_TIME_TMaxOpenTuneX)
#define TIMER_VALUE_tNegTxHshakeReceiv_offset (TIMER_VALUE_tNegRonHshakeReceiv_offset + ALE_TIME_TMaxEthX + ALE_TIME_TMaxOpenTuneX)
#define TIMER_VALUE_tNegRoffHshakeReceiv_offset (TIMER_VALUE_tNegTxHshakeReceiv_offset + ALE_TIME_TEthTx + ALE_TIME_THshakeReceiv + ALE_TIME_TRChan + ALE_TIME_TEthRx)
#define TIMER_VALUE_tNegRonHshakeTrans_offset (TIMER_VALUE_tNegRoffHshakeReceiv_offset + ALE_TIME_TMaxEthX + ALE_TIME_TMaxOpenTuneX)
#define TIMER_VALUE_tNegTxHshakeTrans_offset (TIMER_VALUE_tNegRonHshakeTrans_offset + ALE_TIME_TMaxEthX + ALE_TIME_TMaxOpenTuneX)
#define TIMER_VALUE_tNegRoffHshakeTrans_offset (TIMER_VALUE_tNegTxHshakeTrans_offset + ALE_TIME_TEthTx + ALE_TIME_THshakeTrans + ALE_TIME_TRChan + ALE_TIME_TEthRx)
#define TIMER_VALUE_tNegCycle (TIMER_VALUE_tNegRoffHshakeTrans_offset + ALE_TIME_TMaxEthX + ALE_TIME_TMaxOpenTuneX + ALE_TIME_DTMistiming)
#define TIMER_VALUE_tNegStart(n) (TIMER_VALUE_tCallRoffHshakeT + ALE_TIME_TMaxEthX + ALE_TIME_TMaxOpenTuneX + ALE_TIME_DTMistiming + n*TIMER_VALUE_tNegCycle)
#define TIMER_VALUE_tDataStart_offset (ALE_TIME_TRChan + ALE_TIME_TEthRx + ALE_TIME_TMaxEthX + ALE_TIME_TMaxOpenTuneX + ALE_TIME_dTInit)
#define TIMER_VALUE_tDataTxHeadDelay(sform) (ALE_TIME_TMaxEthX + ALE_TIME_TMaxOpenTuneX + ALE_TIME_dTSynPacket(sform) + ALE_TIME_DTMistiming)
#define TIMER_VALUE_tDataRoffHeadDelay(sform) (TIMER_VALUE_tDataTxHeadDelay(sform) + ALE_TIME_TEthTx + ALE_TIME_THeadL(sform) + ALE_TIME_TRChan + ALE_TIME_TEthRx)
#define TIMER_VALUE_tDataRonRespPackQualDelay(sform) (TIMER_VALUE_tDataRoffHeadDelay(sform) + ALE_TIME_TMaxEthX + ALE_TIME_TMaxOpenTuneX + ALE_TIME_TDataL(sform))
#define TIMER_VALUE_tDataTxRespPackQualDelay(sform) (TIMER_VALUE_tDataRonRespPackQualDelay(sform) + ALE_TIME_TMaxEthX + ALE_TIME_TMaxOpenTuneX)
#define TIMER_VALUE_tDataRoffRespPackQualDelay(sform) (TIMER_VALUE_tDataTxRespPackQualDelay(sform) + ALE_TIME_TEthTx + ALE_TIME_TRespPackQualL + ALE_TIME_TRChan + ALE_TIME_TEthRx)
#define TIMER_VALUE_tDataRonHshakeTDelay(sform) (TIMER_VALUE_tDataRoffRespPackQualDelay(sform) + ALE_TIME_TMaxEthX + ALE_TIME_TMaxOpenTuneX)
#define TIMER_VALUE_tDataTxHshakeTDelay(sform) (TIMER_VALUE_tDataRonHshakeTDelay(sform) + ALE_TIME_TMaxEthX + ALE_TIME_TMaxOpenTuneX)
#define TIMER_VALUE_tDataRoffHshakeTDelay(sform) (TIMER_VALUE_tDataTxHshakeTDelay(sform) + ALE_TIME_TEthTx + ALE_TIME_THshakeTrans + ALE_TIME_TRChan + ALE_TIME_TEthRx)
#define TIMER_VALUE_tDataCycle(sform) (TIMER_VALUE_tDataRoffHshakeTDelay(sform) + ALE_TIME_TMaxEthX + ALE_TIME_TMaxOpenTuneX + ALE_TIME_dTSynPacket(sform) + ALE_TIME_DTMistiming)

struct __attribute__ ((__packed__)) call_packet_t {
	unsigned int lineType : 1;
	unsigned int cycleNum : 4;
	unsigned int respAddr : 5;
	unsigned int reserved : 2;
};
struct __attribute__ ((__packed__)) respcallqual_packet_t {
	unsigned int errSignal : 5;
	unsigned int SNR : 6;
	unsigned int reserved : 1;
};
struct __attribute__ ((__packed__)) hshaketransmode_packet_t {
	unsigned int soundType : 2;
	unsigned int workMode : 4;
	unsigned int paramMode : 6;
	unsigned int schedule : 1;
	unsigned int callAddr : 5;
	unsigned int reserved : 6;
};
struct __attribute__ ((__packed__)) msghead_packet_t {
	unsigned int msgSize : 11;
	unsigned int symCode : 1;
};
struct __attribute__ ((__packed__)) resppackqual_packet_t {
	unsigned int packResult : 1;
	unsigned int SNR : 6;
	unsigned int reserved : 5;
};

typedef QmCrc<uint32_t, 32, 0x04c11db7, 0xffffffff, true, 0xffffffff> CRC32;

static int ale_vm_snr_table[] = ALE_VM_SNR_TABLE_VALUES;

namespace Multiradio {

MainServiceInterface::MainServiceInterface(Dispatcher *dispatcher, Navigation::Navigator *navigator) :
	QmObject(dispatcher),
	current_status(StatusNotReady),
	data_storage_fs(dispatcher->data_storage_fs), dsp_controller(dispatcher->dsp_controller), navigator(navigator), headset_controller(dispatcher->headset_controller)
{
	ale.f_state = alefunctionIdle;
	ale.state = AleState_IDLE;
	ale.phase = ALE_STOPPED;
	ale.timerRadioReady = new QmTimer(true, this);
	ale.timerRadioReady->timeout.connect(sigc::mem_fun(this, &MainServiceInterface::aleprocessRadioReady));
	ale.timerGnssSync = new QmTimer(true, this);
	ale.timerGnssSync->timeout.connect(sigc::mem_fun(this, &MainServiceInterface::aleprocessTimerGnssSyncExpired));
	ale.timerTxCall = new QmTimer(true, this);
	ale.timerTxCall->timeout.connect(sigc::mem_fun(this, &MainServiceInterface::aleprocessTimerTxCallExpired));
	ale.timerRoffCall = new QmTimer(true, this);
	ale.timerRoffCall->timeout.connect(sigc::mem_fun(this, &MainServiceInterface::aleprocessTimerRoffCallExpired));
	ale.timerCallRonHshakeR = new QmTimer(true, this);
	ale.timerCallRonHshakeR->timeout.connect(sigc::mem_fun(this, &MainServiceInterface::aleprocessTimerCallRonHshakeRExpired));
	ale.timerCallTxHshakeR = new QmTimer(true, this);
	ale.timerCallTxHshakeR->timeout.connect(sigc::mem_fun(this, &MainServiceInterface::aleprocessTimerCallTxHshakeRExpired));
	ale.timerCallRoffHshakeR = new QmTimer(true, this);
	ale.timerCallRoffHshakeR->timeout.connect(sigc::mem_fun(this, &MainServiceInterface::aleprocessTimerCallRoffHshakeRExpired));
	ale.timerCallRonHshakeT = new QmTimer(true, this);
	ale.timerCallRonHshakeT->timeout.connect(sigc::mem_fun(this, &MainServiceInterface::aleprocessTimerCallRonHshakeTExpired));
	ale.timerCallTxHshakeT = new QmTimer(true, this);
	ale.timerCallTxHshakeT->timeout.connect(sigc::mem_fun(this, &MainServiceInterface::aleprocessTimerTxHshakeTransExpired));
	ale.timerCallRoffHshakeT = new QmTimer(true, this);
	ale.timerCallRoffHshakeT->timeout.connect(sigc::mem_fun(this, &MainServiceInterface::aleprocessTimerCallRoffHshakeTExpired));
	for (int i = 0; i < 3; i++) {
		ale.timerRxNegStart[i] = new QmTimer(true, this);
		ale.timerRxNegStart[i]->timeout.connect(sigc::mem_fun(this, &MainServiceInterface::aleprocessTimerRxNegStartExpired));
		ale.timerTxNegStart[i] = new QmTimer(true, this);
		ale.timerTxNegStart[i]->timeout.connect(sigc::mem_fun(this, &MainServiceInterface::aleprocessTimerTxNegStartExpired));
		ale.timerNegTxRespCallQual[i] = new QmTimer(true, this);
		ale.timerNegTxRespCallQual[i]->timeout.connect(sigc::mem_fun(this, &MainServiceInterface::aleprocessTimerNegTxRespCallQualExpired));
		ale.timerNegRoffRespCallQual[i] = new QmTimer(true, this);
		ale.timerNegRoffRespCallQual[i]->timeout.connect(sigc::mem_fun(this, &MainServiceInterface::aleprocessTimerNegRoffExpired));
		ale.timerNegRonHshakeTransMode[i] = new QmTimer(true, this);
		ale.timerNegRonHshakeTransMode[i]->timeout.connect(sigc::mem_fun(this, &MainServiceInterface::aleprocessTimerNegRonHshakeTransModeExpired));
		ale.timerNegTxHshakeTransMode[i] = new QmTimer(true, this);
		ale.timerNegTxHshakeTransMode[i]->timeout.connect(sigc::mem_fun(this, &MainServiceInterface::aleprocessTimerNegTxHshakeTransModeExpired));
		ale.timerNegRoffHshakeTransMode[i] = new QmTimer(true, this);
		ale.timerNegRoffHshakeTransMode[i]->timeout.connect(sigc::mem_fun(this, &MainServiceInterface::aleprocessTimerNegRoffHshakeTransModeExpired));
		ale.timerNegRonHshakeReceiv[i] = new QmTimer(true, this);
		ale.timerNegRonHshakeReceiv[i]->timeout.connect(sigc::mem_fun(this, &MainServiceInterface::aleprocessTimerNegRonHshakeReceivExpired));
		ale.timerNegTxHshakeReceiv[i] = new QmTimer(true, this);
		ale.timerNegTxHshakeReceiv[i]->timeout.connect(sigc::mem_fun(this, &MainServiceInterface::aleprocessTimerNegTxHshakeReceivExpired));
		ale.timerNegRoffHshakeReceiv[i] = new QmTimer(true, this);
		ale.timerNegRoffHshakeReceiv[i]->timeout.connect(sigc::mem_fun(this, &MainServiceInterface::aleprocessTimerNegRoffExpired));
		ale.timerNegRonHshakeTrans[i] = new QmTimer(true, this);
		ale.timerNegRonHshakeTrans[i]->timeout.connect(sigc::mem_fun(this, &MainServiceInterface::aleprocessTimerNegRonHshakeTransExpired));
		ale.timerNegTxHshakeTrans[i] = new QmTimer(true, this);
		ale.timerNegTxHshakeTrans[i]->timeout.connect(sigc::mem_fun(this, &MainServiceInterface::aleprocessTimerTxHshakeTransExpired));
		ale.timerNegRoffHshakeTrans[i] = new QmTimer(true, this);
		ale.timerNegRoffHshakeTrans[i]->timeout.connect(sigc::mem_fun(this, &MainServiceInterface::aleprocessTimerNegRoffHshakeTransExpired));
	}
	ale.timerDataStart = new QmTimer(true, this);
	ale.timerDataStart->timeout.connect(sigc::mem_fun(this, &MainServiceInterface::aleprocessTimerDataStartExpired));
	for (int i = 0; i < 3; i++) {
		ale.timerMsgTxHead[i] = new QmTimer(true, this);
		ale.timerMsgTxHead[i]->timeout.connect(sigc::mem_fun(this, &MainServiceInterface::aleprocessTimerMsgTxHeadExpired));
		ale.timerMsgRoffHead[i] = new QmTimer(true, this);
		ale.timerMsgRoffHead[i]->timeout.connect(sigc::mem_fun(this, &MainServiceInterface::aleprocessTimerMsgRoffHeadExpired));
		ale.timerMsgRonRespPackQual[i] = new QmTimer(true, this);
		ale.timerMsgRonRespPackQual[i]->timeout.connect(sigc::mem_fun(this, &MainServiceInterface::aleprocessTimerMsgRonRespPackQualExpired));
		ale.timerMsgTxRespPackQual[i] = new QmTimer(true, this);
		ale.timerMsgTxRespPackQual[i]->timeout.connect(sigc::mem_fun(this, &MainServiceInterface::aleprocessTimerMsgTxRespPackQualExpired));
		ale.timerMsgRoffRespPackQual[i] = new QmTimer(true, this);
		ale.timerMsgRoffRespPackQual[i]->timeout.connect(sigc::mem_fun(this, &MainServiceInterface::aleprocessTimerMsgRoffRespPackQualExpired));
		ale.timerMsgRonHshakeT[i] = new QmTimer(true, this);
		ale.timerMsgRonHshakeT[i]->timeout.connect(sigc::mem_fun(this, &MainServiceInterface::aleprocessTimerMsgRonHshakeTExpired));
		ale.timerMsgTxHshakeT[i] = new QmTimer(true, this);
		ale.timerMsgTxHshakeT[i]->timeout.connect(sigc::mem_fun(this, &MainServiceInterface::aleprocessTimerMsgTxHshakeTExpired));
		ale.timerMsgRoffHshakeT[i] = new QmTimer(true, this);
		ale.timerMsgRoffHshakeT[i]->timeout.connect(sigc::mem_fun(this, &MainServiceInterface::aleprocessTimerMsgRoffHshakeTExpired));
		ale.timerRxMsgCycle[i] = new QmTimer(true, this);
		ale.timerRxMsgCycle[i]->timeout.connect(sigc::mem_fun(this, &MainServiceInterface::aleprocessTimerRxMsgCycleExpired));
		ale.timerTxMsgCycle[i] = new QmTimer(true, this);
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
	if (navigator != 0)
		navigator->syncPulse.connect(sigc::mem_fun(this, &MainServiceInterface::aleprocess1PPS));
	dsp_controller->transmittedModemPacket.connect(sigc::mem_fun(this, &MainServiceInterface::aleprocessModemPacketTransmitted));
	dsp_controller->failedTxModemPacket.connect(sigc::mem_fun(this, &MainServiceInterface::aleprocessModemPacketFailedTx));
	dsp_controller->receivedModemPacket.connect(sigc::mem_fun(this, &MainServiceInterface::aleprocessModemPacketReceived));
	dsp_controller->startedRxModemPacket_packHead.connect(sigc::mem_fun(this, &MainServiceInterface::aleprocessModemPacketStartedRxPackHead));
	dsp_controller->failedRxModemPacket.connect(sigc::mem_fun(this, &MainServiceInterface::aleprocessModemPacketFailedRx));
}

MainServiceInterface::~MainServiceInterface()
{
}

MainServiceInterface::Status MainServiceInterface::getStatus() {
	return current_status;
}

void MainServiceInterface::setStatus(Status value) {
	if (current_status != value) {
		current_status = value;
		statusChanged(value);
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
	dsp_controller->setModemReceiverBandwidth(DspController::modembwAll);
	dsp_controller->setModemReceiverTimeSyncMode(DspController::modemtimesyncGPS);
	dsp_controller->setModemReceiverPhase(DspController::modemphaseWaitingCall);
	dsp_controller->setModemReceiverRole(DspController::modemroleResponder);
	ale.timerRadioReady->start(100);
}

void MainServiceInterface::startAleTxVoiceMail(uint8_t address) {
	qmDebugMessage(QmDebug::Info, "starting ALE tx voice mail (address = %u)", address);
	if (!startAleSession())
		return;
	ale.f_state = alefunctionTx;
	ale.result = AleResultNone;
	ale.address = address;
	setAlePhase(ALE_TX_SETUP);
	setAleState(AleState_TX_CALLING);
	ale.supercycle = 1;
	ale.cycle = 1;
	Multiradio::voice_message_t message = headset_controller->getRecordedSmartMessage();
	int message_bits_size = message.size()*8;
	ale.vm_size = message_bits_size/72;
	ale.vm_f_count = ceil(message_bits_size/490);
	ale.vm_fragments.resize(ale.vm_f_count);
	for (unsigned int i = 0; i < ale.vm_fragments.size(); i++) {
		ale.vm_fragments[i].num_data[0] = i;
		for (unsigned int j = 1; j < sizeof(ale.vm_fragments[0].num_data); j++)
			ale.vm_fragments[i].num_data[j] = 0;
	}
	for (int m_bit_i = 0; m_bit_i < message_bits_size; m_bit_i++) {
		int f_i = m_bit_i / 490;
		int f_bit_i = 6 + (m_bit_i % 490);
		int f_byte_i = f_bit_i / 8;
		int f_byte_bit = f_bit_i % 8;
		if ((message[m_bit_i/8] & (1 << (m_bit_i % 8))) != 0)
			ale.vm_fragments[f_i].num_data[f_byte_i] |= (1 << f_byte_bit);
	}
	for (unsigned int i = 0; i < ale.vm_fragments.size(); i++) {
		CRC32 f_crc;
		f_crc.update(&(ale.vm_fragments[i].num_data[0]), sizeof(ale.vm_fragments[0].num_data));
		ale.vm_fragments[i].crc = f_crc.result();
	}
	dsp_controller->setModemReceiverBandwidth(DspController::modembw20kHz);
	dsp_controller->setModemReceiverTimeSyncMode(DspController::modemtimesyncGPS);
	dsp_controller->setModemReceiverPhase(DspController::modemphaseALE);
	dsp_controller->setModemReceiverRole(DspController::modemroleCaller);
	ale.timerRadioReady->start(100);
}

MainServiceInterface::AleResult MainServiceInterface::stopAle() {
	setAlePhase(ALE_STOPPED);
	stopAleSession();
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
	Multiradio::voice_message_t vm_rx_message(ale.vm_size*72/8, 0);
	for (int f_i = 0; f_i < ale.vm_f_idx; f_i++) {
		int f_bits_size = (f_i == (ale.vm_f_idx - 1))?(ale.vm_size*72 - (ale.vm_f_idx - 1)*490):(490);
		int message_bits_offset = f_i*490;
		for (int f_bit_i = 6; f_bit_i < (6 + f_bits_size); f_bit_i++) {
			int f_byte_i = f_bit_i / 8;
			int f_byte_bit = f_bit_i % 8;
			int m_byte_i = (message_bits_offset + f_bit_i) / 8;
			int m_byte_bit = (message_bits_offset + f_bit_i) % 8;
			if ((ale.vm_fragments[f_i].num_data[f_byte_i] & (1 << f_byte_bit)) != 0)
				vm_rx_message[m_byte_i] |= (1 << m_byte_bit);
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
	ALE_PHASE_SWITCH_CASE(TX_VM_START)
	ALE_PHASE_SWITCH_CASE(TX_VM_TX_MSGHEAD)
	ALE_PHASE_SWITCH_CASE(TX_VM_RX_MSG_RESP)
	ALE_PHASE_SWITCH_CASE(TX_VM_TX_MSG_HSHAKE)
	ALE_PHASE_SWITCH_CASE(TX_VM_TX_PACKET)
	ALE_PHASE_SWITCH_CASE(TX_VM_RX_PACK_RESP)
	ALE_PHASE_SWITCH_CASE(TX_VM_TX_PACK_HSHAKE)
	ALE_PHASE_SWITCH_CASE(TX_VM_TX_LINK_RELEASE)
	ALE_PHASE_SWITCH_CASE(RX_SETUP)
	ALE_PHASE_SWITCH_CASE(RX_SCAN)
	ALE_PHASE_SWITCH_CASE(RX_CALL)
	ALE_PHASE_SWITCH_CASE(RX_CALL_TX_HSHAKE)
	ALE_PHASE_SWITCH_CASE(RX_CALL_RX_HSHAKE)
	ALE_PHASE_SWITCH_CASE(RX_NEG_TX_QUAL)
	ALE_PHASE_SWITCH_CASE(RX_NEG_RX_MODE)
	ALE_PHASE_SWITCH_CASE(RX_NEG_TX_HSHAKE)
	ALE_PHASE_SWITCH_CASE(RX_NEG_RX_HSHAKE)
	ALE_PHASE_SWITCH_CASE(RX_VM_START)
	ALE_PHASE_SWITCH_CASE(RX_VM_RX_MSGHEAD)
	ALE_PHASE_SWITCH_CASE(RX_VM_TX_MSG_RESP)
	ALE_PHASE_SWITCH_CASE(RX_VM_RX_MSG_HSHAKE)
	ALE_PHASE_SWITCH_CASE(RX_VM_RX_PACKET)
	ALE_PHASE_SWITCH_CASE(RX_VM_TX_PACK_RESP)
	ALE_PHASE_SWITCH_CASE(RX_VM_TX_LINK_RELEASE)
	ALE_PHASE_SWITCH_CASE(RX_VM_RX_PACK_HSHAKE)
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
	if (!data_storage_fs->getAleDefaultCallFreqs(ale.call_freqs)) {
		setAleState(AleState_FAIL_NO_FREQ);
		setAlePhase(ALE_STOPPED);
		return false;
	}
	if (!data_storage_fs->getAleStationAddress(ale.station_address)) {
		setAleState(AleState_FAIL_NO_ADDR);
		setAlePhase(ALE_STOPPED);
		return false;
	}
	dsp_controller->setRadioOperation(DspController::RadioOperationOff);
	return true;
}

void MainServiceInterface::stopAleSession() {
	qmDebugMessage(QmDebug::Info, "ale session stop");
	switch (ale.f_state) {
	case alefunctionRx:
		stopAllRxTimers();
		break;
	case alefunctionTx:
		ale.vm_fragments.clear();
		stopAllTxTimers();
		break;
	default:
		QM_ASSERT(0);
	}
	if (ale.phase == ALE_STOPPED) {
		setAleState(AleState_IDLE);
		return;
	}
	dsp_controller->disableModemReceiver();
	dsp_controller->disableModemTransmitter();
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
	ale.timerDataStart->start(TIMER_VALUE_tDataStart_offset);
	setAlePhase(ALE_RX_VM_START);
	ale.vm_msg_cycle = 0;
	ale.vm_progress = 0;
	setAleState(AleState_RX_VM_TRANSFER);
	dsp_controller->setModemReceiverPhase(DspController::modemphaseLinkEstablished);
}

void MainServiceInterface::startVmTx() {
	ale.timerDataStart->start(TIMER_VALUE_tDataStart_offset);
	setAlePhase(ALE_TX_VM_START);
	ale.vm_msg_cycle = 0;
	ale.vm_progress = 0;
	setAleState(AleState_TX_VM_TRANSFER);
	dsp_controller->setModemReceiverPhase(DspController::modemphaseLinkEstablished);
}

bool MainServiceInterface::checkDwellStart(int &freq_idx) {
	Navigation::Coord_Date date = navigator->getCoordDate();
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
		return -19;
	if (value == 63)
		return 42;
	return (-20 + ((value - 1)/(62-1))*(20 + 41));
}

uint8_t MainServiceInterface::convertSnrToPacket(int value) {
	if (value < -20)
		return 0;
	if (value > 41)
		return 63;
	return (1 + ((value + 20)/(20 + 41))*(62 - 1));
}

void MainServiceInterface::proceedRxScanning() {
	stopAleRxTimers();
	dsp_controller->setModemReceiverBandwidth(DspController::modembwAll);
	dsp_controller->setModemReceiverPhase(DspController::modemphaseWaitingCall);
	ale.timerRadioReady->start(100);
	setAlePhase(ALE_RX_SETUP);
	setAleState(AleState_RX_SCANNING);
}

void MainServiceInterface::proceedTxCalling() {
	stopAleTxTimers();
	ale.cycle++;
	if (!(ale.cycle <= 12)) {
		ale.supercycle++;
		ale.cycle = 1;
	}
	if (ale.supercycle <= 3) {
		setAlePhase(ALE_TX_CYCLE_CALL);
		setAleState(AleState_TX_CALLING);
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
		dsp_controller->disableModemTransmitter();
		break;
	case DspController::modempacket_HshakeTrans: {
		switch (ale.phase) {
		case ALE_TX_CALL_TX_HSHAKE:
			dsp_controller->disableModemTransmitter();
			break;
		case ALE_TX_NEG_TX_HSHAKE:
			dsp_controller->disableModemTransmitter();
			stopAleTxTimers();
			startVmTx();
			break;
		case ALE_TX_VM_TX_PACK_HSHAKE: {
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
		dsp_controller->disableModemTransmitter();
		break;
	}
	case DspController::modempacket_packHead: {
		dsp_controller->disableModemTransmitter();
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
			stopAleSession();
			setAleState(AleState_RX_VM_COMPLETE_FULL);
			break;
		}
		default:
			break;
		}
		break;
	}
	case DspController::modempacket_HshakeReceiv: {
		dsp_controller->disableModemTransmitter();
		break;
	}
	case DspController::modempacket_RespCallQual: {
		dsp_controller->disableModemTransmitter();
		break;
	}
	case DspController::modempacket_RespPackQual: {
		dsp_controller->disableModemTransmitter();
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

void MainServiceInterface::aleprocessModemPacketReceived(DspController::ModemPacketType type, int8_t snr, DspController::ModemBandwidth bandwidth, uint8_t* data, int data_len) {
	switch (type) {
	case DspController::modempacket_HshakeReceiv: {
		switch (ale.phase) {
		case ALE_TX_CALL_RX_HSHAKE: {
			ale.timerCallRoffHshakeR->stop();
			if (evaluatePacketSNR(snr)) {
				dsp_controller->enableModemTransmitter();
				setAlePhase(ALE_TX_CALL_TX_HSHAKE);
			} else {
				dsp_controller->disableModemReceiver();
				proceedTxCalling();
			}
			break;
		}
		case ALE_TX_NEG_RX_HSHAKE: {
			ale.timerNegRoffHshakeReceiv[ale.rcount]->stop();
			dsp_controller->enableModemTransmitter();
			setAlePhase(ALE_TX_NEG_TX_HSHAKE);
			break;
		}
		default:
			break;
		}
		break;
	}
	case DspController::modempacket_RespCallQual: {
		if (!(data_len >= 2))
			break;
		if (ale.phase != ALE_TX_NEG_RX_QUAL)
			break;
		ale.timerNegRoffRespCallQual[ale.rcount]->stop();
		dsp_controller->enableModemTransmitter();
		setAlePhase(ALE_TX_NEG_TX_MODE);
		break;
	}
	case DspController::modempacket_RespPackQual: {
		switch (ale.phase) {
		case ALE_TX_VM_RX_MSG_RESP: {
			ale.timerMsgRoffRespPackQual[ale.vm_msg_cycle]->stop();
			setAlePhase(ALE_TX_VM_TX_MSG_HSHAKE);
			dsp_controller->enableModemTransmitter();
			break;
		}
		case ALE_TX_VM_RX_PACK_RESP: {
			if (!(data_len >= 2))
				break;
			resppackqual_packet_t *resppackqual_packet = (resppackqual_packet_t *)data;
			processPacketTxResponse(resppackqual_packet->packResult, resppackqual_packet->SNR);
			break;
		}
		default:
			break;
		}
		break;
	}
	case DspController::modempacket_LinkRelease: {
		if (!((ale.phase == ALE_TX_VM_RX_PACK_RESP) && (ale.vm_f_idx == (ale.vm_f_count - 1))))
			break;
		ale.timerTxPacketTxLinkRelease->start(ale.tPacketSync, TIMER_VALUE_tDataTxHshakeTDelay(ale.vm_sform_c));
		dsp_controller->enableModemTransmitter();
		setAlePhase(ALE_TX_VM_TX_LINK_RELEASE);
		break;
	}
	case DspController::modempacket_Call: {
		if (!(data_len >= 2))
			break;
		if (ale.phase != ALE_RX_CALL)
			break;
		ale.timerRoffCall->stop();
		call_packet_t *call_packet = (call_packet_t *)data;
		if (!((call_packet->lineType == 1) && (call_packet->respAddr == ale.station_address))) {
			dsp_controller->disableModemReceiver();
			proceedRxScanning();
			break;
		}
		ale.supercycle = call_packet->cycleNum;
		if (!evaluatePacketSNR(snr)) {
			dsp_controller->disableModemReceiver();
			proceedRxScanning();
			break;
		}
		ale.call_bw = bandwidth;
		ale.call_snr = convertSnrToPacket(snr);
		dsp_controller->enableModemTransmitter();
		dsp_controller->setModemReceiverBandwidth(bandwidth);
		dsp_controller->setModemReceiverPhase(DspController::modemphaseALE);
		setAlePhase(ALE_RX_CALL_TX_HSHAKE);
		break;
	}
	case DspController::modempacket_HshakeTrans: {
		switch (ale.phase) {
		case ALE_RX_CALL_RX_HSHAKE: {
			ale.timerCallRoffHshakeT->stop();
			dsp_controller->disableModemReceiver();
			setAleState(AleState_RX_CALL_NEGOTIATING);
			ale.rcount = 0;
			break;
		}
		case ALE_RX_NEG_RX_HSHAKE: {
			stopAleRxTimers();
			dsp_controller->disableModemReceiver();
			startVmRx();
			break;
		}
		case ALE_RX_VM_RX_MSG_HSHAKE: {
			ale.timerMsgRoffHshakeT[ale.vm_msg_cycle]->stop();
			break;
		}
		case ALE_RX_VM_RX_PACK_HSHAKE: {
			ale.timerPacketRoffHshakeT->stop();
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
		if (ale.phase != ALE_RX_NEG_RX_MODE)
			break;
		ale.timerNegRoffHshakeTransMode[ale.rcount]->stop();
		hshaketransmode_packet_t *hshaketransmode_packet = (hshaketransmode_packet_t *)data;
		if (!((hshaketransmode_packet->soundType == 0) && (hshaketransmode_packet->schedule == 1)
				&& (hshaketransmode_packet->workMode == 3) && (hshaketransmode_packet->paramMode == 1))) {
			setAleState(AleState_RX_CALL_FAIL_UNSUPPORTED);
			stopAleSession();
			break;
		}
		ale.address = hshaketransmode_packet->callAddr;
		setAlePhase(ALE_RX_NEG_TX_HSHAKE);
		dsp_controller->enableModemTransmitter();
		break;
	}
	case DspController::modempacket_msgHead: {
		if (!(data_len >= 2))
			break;
		if (ale.phase != ALE_RX_VM_RX_MSGHEAD)
			break;
		msghead_packet_t *msghead_packet = (msghead_packet_t *)data;
		if (!((msghead_packet->msgSize >= 1) && (msghead_packet->msgSize <= 250) && (msghead_packet->symCode == 0))) {
			setAleState(AleState_RX_VM_FAIL_UNSUPPORTED);
			stopAleSession();
			break;
		}
		ale.vm_size = msghead_packet->msgSize;
		ale.vm_f_count = ceil(ale.vm_size*72/490);
		ale.vm_fragments.resize(ale.vm_f_count);
		ale.timerMsgRoffHead[ale.vm_msg_cycle]->stop();
		setAlePhase(ALE_RX_VM_TX_MSG_RESP);
		dsp_controller->enableModemTransmitter();
		break;
	}
	case DspController::modempacket_packHead: {
		if (!(data_len == 66))
			break;
		if (ale.phase != ALE_RX_VM_RX_PACKET)
			break;
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

void MainServiceInterface::aleprocessModemPacketStartedRxPackHead(int8_t snr, DspController::ModemBandwidth bandwidth, uint8_t param_signForm, uint8_t param_packCode, uint8_t* data, int data_len) {
	if (!(param_packCode = 0)) {
		dsp_controller->disableModemReceiver();
		return;
	}
	switch (ale.phase) {
	case ALE_RX_VM_RX_MSGHEAD: {
		if (!(ale.vm_msg_cycle > 0)) {
			dsp_controller->disableModemReceiver();
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
		dsp_controller->disableModemReceiver();
		return;
	}
	ale.vm_sform_n = param_signForm;
	ale.rcount = 0;
	ale.vm_packet_snr = convertSnrToPacket(snr);
}

void MainServiceInterface::aleprocessModemPacketFailedRx(DspController::ModemPacketType type) {
	switch (type) {
	case DspController::modempacket_packHead: {
		if (ale.phase != ALE_RX_VM_RX_PACKET)
			break;
		ale.vm_packet_result = false;
		startRxPacketResponse();
		break;
	}
	default:
		break;
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
		qmDebugMessage(QmDebug::Info, "ale dwell start (tx call)");
		ale.timerTxCall->start(TIMER_VALUE_tTxCall);
		ale.timerCallRonHshakeR->start(TIMER_VALUE_tCallRonHshakeR);
		ale.timerCallRoffHshakeR->start(TIMER_VALUE_tCallRoffHshakeR);
		ale.timerCallTxHshakeT->start(TIMER_VALUE_tCallTxHshakeT);
		for (int i = 0; i < 3; i++) {
			int neg_start = TIMER_VALUE_tNegStart(i);
			ale.timerTxNegStart[i]->start(neg_start);
			ale.timerNegRoffRespCallQual[i]->start(neg_start + TIMER_VALUE_tNegRoffRespCallQual_offset);
			ale.timerNegTxHshakeTransMode[i]->start(neg_start + TIMER_VALUE_tNegTxHshakeTransMode_offset);
			ale.timerNegRonHshakeReceiv[i]->start(neg_start + TIMER_VALUE_tNegRonHshakeReceiv_offset);
			ale.timerNegRoffHshakeReceiv[i]->start(neg_start + TIMER_VALUE_tNegRoffHshakeReceiv_offset);
			ale.timerNegTxHshakeTrans[i]->start(neg_start + TIMER_VALUE_tNegTxHshakeTrans_offset);
		}
		dsp_controller->setRadioParameters(DspController::RadioModeSazhenData, ale.call_freqs[freq_idx]);
		dsp_controller->enableModemTransmitter();
		setAlePhase(ALE_TX_CALL);
		ale.rcount = 0;
		break;
	}
	case ALE_RX_SCAN: {
		int freq_idx;
		if (!checkDwellStart(freq_idx))
			break;
		qmDebugMessage(QmDebug::Info, "ale dwell start (rx scan)");
		ale.timerRoffCall->start(TIMER_VALUE_tRoffCall);
		ale.timerCallTxHshakeR->start(TIMER_VALUE_tCallTxHshakeR);
		ale.timerCallRonHshakeT->start(TIMER_VALUE_tCallRonHshakeT);
		ale.timerCallRoffHshakeT->start(TIMER_VALUE_tCallRoffHshakeT);
		for (int i = 0; i < 3; i++) {
			int neg_start = TIMER_VALUE_tNegStart(i);
			ale.timerRxNegStart[i]->start(neg_start);
			ale.timerNegTxRespCallQual[i]->start(neg_start + TIMER_VALUE_tNegTxRespCallQual_offset);
			ale.timerNegRonHshakeTransMode[i]->start(neg_start + TIMER_VALUE_tNegRonHshakeTransMode_offset);
			ale.timerNegRoffHshakeTransMode[i]->start(neg_start + TIMER_VALUE_tNegRoffHshakeTransMode_offset);
			ale.timerNegTxHshakeReceiv[i]->start(neg_start + TIMER_VALUE_tNegTxHshakeReceiv_offset);
			ale.timerNegRonHshakeTrans[i]->start(neg_start + TIMER_VALUE_tNegRonHshakeTrans_offset);
			ale.timerNegRoffHshakeTrans[i]->start(neg_start + TIMER_VALUE_tNegRoffHshakeTrans_offset);
		}
		dsp_controller->setRadioParameters(DspController::RadioModeSazhenData, ale.call_freqs[freq_idx]);
		dsp_controller->enableModemReceiver();
		setAlePhase(ALE_RX_CALL);
		break;
	}
	default:
		break;
	}
}

void MainServiceInterface::aleprocessTimerGnssSyncExpired() {
	setAleState(AleState_FAIL_GNSS);
	stopAleSession();
}

void MainServiceInterface::aleprocessTimerTxCallExpired() {
	call_packet_t call_packet;
	call_packet.lineType = 1;
	call_packet.cycleNum = ale.supercycle;
	call_packet.respAddr = ale.address;
	call_packet.reserved = 0;
	dsp_controller->sendModemPacket(DspController::modempacket_Call, DspController::modembw20kHz, (uint8_t *)&call_packet, sizeof(call_packet));
}

void MainServiceInterface::aleprocessTimerCallRonHshakeRExpired() {
	dsp_controller->enableModemReceiver();
	setAlePhase(ALE_TX_CALL_RX_HSHAKE);
}

void MainServiceInterface::aleprocessTimerCallRoffHshakeRExpired() {
	dsp_controller->disableModemReceiver();
	proceedTxCalling();
}

void MainServiceInterface::aleprocessTimerTxNegStartExpired() {
	setAleState(AleState_TX_CALL_NEGOTIATING);
	setAlePhase(ALE_TX_NEG_RX_QUAL);
	dsp_controller->enableModemReceiver();
}

void MainServiceInterface::aleprocessTimerNegRoffExpired() {
	switch (ale.phase) {
	case ALE_TX_NEG_RX_QUAL:
	case ALE_TX_NEG_RX_HSHAKE: {
		dsp_controller->disableModemReceiver();
		if (ale.rcount < 3) {
			ale.timerNegTxHshakeTransMode[ale.rcount]->stop();
			ale.timerNegRonHshakeReceiv[ale.rcount]->stop();
			ale.timerNegRoffHshakeReceiv[ale.rcount]->stop();
			ale.timerNegTxHshakeTrans[ale.rcount]->stop();
			ale.rcount++;
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
	hshaketransmode_packet.reserved = 0;
	dsp_controller->sendModemPacket(DspController::modempacket_HshakeTransMode, DspController::modembw20kHz, (uint8_t *)&hshaketransmode_packet, sizeof(hshaketransmode_packet));
}

void MainServiceInterface::aleprocessTimerNegRonHshakeReceivExpired() {
	dsp_controller->enableModemReceiver();
	setAlePhase(ALE_TX_NEG_RX_HSHAKE);
}

void MainServiceInterface::aleprocessTimerTxHshakeTransExpired() {
	dsp_controller->sendModemPacket(DspController::modempacket_HshakeTrans, DspController::modembw20kHz, 0, 0);
}

void MainServiceInterface::aleprocessTimerDataStartExpired() {
	switch (ale.phase) {
	case ALE_RX_VM_START: {
		ale.tPacketSync.set();
		for (int i = 0; i < 3; i++) {
			int cycle = i*TIMER_VALUE_tDataCycle(-1);
			ale.timerMsgRoffHead[i]->start(cycle + TIMER_VALUE_tDataRoffHeadDelay(-1));
			ale.timerMsgTxRespPackQual[i]->start(cycle + TIMER_VALUE_tDataTxRespPackQualDelay(-1));
			ale.timerMsgRonHshakeT[i]->start(cycle + TIMER_VALUE_tDataRonHshakeTDelay(-1));
			ale.timerMsgRoffHshakeT[i]->start(cycle + TIMER_VALUE_tDataRoffHshakeTDelay(-1));
			ale.timerTxMsgCycle[i]->start(cycle);
		}
		dsp_controller->enableModemReceiver();
		setAlePhase(ALE_RX_VM_RX_MSGHEAD);
		break;
	}
	case ALE_TX_VM_START: {
		ale.tPacketSync.set();
		for (int i = 0; i < 3; i++) {
			int cycle = i*TIMER_VALUE_tDataCycle(-1);
			ale.timerMsgTxHead[i]->start(cycle + TIMER_VALUE_tDataTxHeadDelay(-1));
			ale.timerMsgRonRespPackQual[i]->start(cycle + TIMER_VALUE_tDataRonRespPackQualDelay(-1));
			ale.timerMsgRoffRespPackQual[i]->start(cycle + TIMER_VALUE_tDataRoffRespPackQualDelay(-1));
			ale.timerMsgTxHshakeT[i]->start(cycle + TIMER_VALUE_tDataTxHshakeTDelay(-1));
			ale.timerTxMsgCycle[i]->start(cycle);
		}
		dsp_controller->enableModemTransmitter();
		break;
	}
	default:
		QM_ASSERT(0);
	}
}

void MainServiceInterface::aleprocessTimerMsgTxHeadExpired() {
	setAlePhase(ALE_TX_VM_TX_MSGHEAD);
	msghead_packet_t msghead_packet;
	msghead_packet.msgSize = ale.vm_size;
	msghead_packet.symCode = 0;
	dsp_controller->sendModemPacket(DspController::modempacket_msgHead, DspController::modembw20kHz, (uint8_t *)&msghead_packet, sizeof(msghead_packet));
}

void MainServiceInterface::aleprocessTimerMsgRonRespPackQualExpired() {
	dsp_controller->enableModemReceiver();
	setAlePhase(ALE_TX_VM_RX_MSG_RESP);
}

void MainServiceInterface::aleprocessTimerMsgRoffRespPackQualExpired() {
	dsp_controller->disableModemReceiver();
	ale.timerMsgTxHshakeT[ale.vm_msg_cycle]->stop();
}

void MainServiceInterface::aleprocessTimerMsgTxHshakeTExpired() {
	dsp_controller->sendModemPacket(DspController::modempacket_HshakeTrans, DspController::modembw20kHz, 0, 0);
}

void MainServiceInterface::aleprocessTimerTxMsgCycleExpired() {
	if (ale.phase != ALE_TX_VM_TX_MSG_HSHAKE) {
		ale.vm_msg_cycle++;
		if (ale.vm_msg_cycle < 3) {
			dsp_controller->enableModemTransmitter();
		} else {
			setAleState(AleState_TX_VM_FAIL);
			stopAleSession();
		}
	} else {
		stopVmMsgTxTimers();
		ale.tPacketSync.shift((ale.vm_msg_cycle + 1)*TIMER_VALUE_tDataCycle(-1));
		ale.vm_sform_c = ALE_VM_INITIAL_SFORM;
		ale.vm_sform_p = ale.vm_sform_c;
		ale.vm_f_idx = 0;
		ale.rcount = 0;
		ale.vm_ack_count = 0;
		ale.vm_nack_count = 0;
		aleprocessTxPacketSync();
	}
}

void MainServiceInterface::aleprocessTxPacketSync() {
	dsp_controller->enableModemTransmitter();
	ale.vm_adaptation = alevmadaptationNone;
	setAlePhase(ALE_TX_VM_TX_PACKET);
	ale.timerPacketTxHeadData->start(ale.tPacketSync, TIMER_VALUE_tDataTxHeadDelay(ale.vm_sform_c));
}

void MainServiceInterface::processPacketTxResponse(bool p_result, uint8_t p_snr) {
	ale.timerPacketRoffRespPackQual->stop();
	int snr = convertSnrFromPacket(p_snr);
	if ((ale.vm_f_idx == (ale.vm_f_count - 1)) && (p_result != false)) {
		dsp_controller->disableModemReceiver();
		processFailedPacketTxCycle();
		return;
	}
	if (p_result) {
		ale.vm_f_idx++;
		setAleVmProgress(ale.vm_f_idx/ale.vm_f_count);
		ale.vm_snr_ack[ale.vm_ack_count] = snr;
		ale.vm_nack_count = 0;
		ale.vm_ack_count++;
		if (ale.vm_ack_count == 3)
			ale.vm_adaptation = alevmadaptationUp;
	} else {
		ale.vm_snr_nack[ale.vm_nack_count] = snr;
		ale.vm_ack_count = 0;
		ale.vm_nack_count++;
		if (ale.vm_nack_count == 2)
			ale.vm_adaptation = alevmadaptationDown;
	}
	ale.timerPacketTxHshakeT->start(ale.tPacketSync, TIMER_VALUE_tDataTxHshakeTDelay(ale.vm_sform_c));
	dsp_controller->enableModemTransmitter();
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
	int sform = (ale.vm_sform_p > ale.vm_sform_c)?ale.vm_sform_p:ale.vm_sform_c;
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
	dsp_controller->sendModemPacket_packHead(DspController::modembw20kHz, ale.vm_sform_c, 0, (uint8_t *)&(ale.vm_fragments[ale.vm_f_idx]), sizeof(ale.vm_fragments[0]));
}

void MainServiceInterface::aleprocessTimerPacketRonRespPackQualExpired() {
	dsp_controller->enableModemReceiver();
	ale.timerPacketRoffRespPackQual->start(ale.tPacketSync, TIMER_VALUE_tDataRoffRespPackQualDelay(ale.vm_sform_c));
	setAlePhase(ALE_TX_VM_RX_PACK_RESP);
}

void MainServiceInterface::aleprocessTimerPacketRoffRespPackQualExpired() {
	dsp_controller->disableModemReceiver();
	processFailedPacketTxCycle();
}

void MainServiceInterface::aleprocessTimerPacketTxHshakeTExpired() {
	dsp_controller->sendModemPacket(DspController::modempacket_HshakeTrans, DspController::modembw20kHz, 0, 0);
}

void MainServiceInterface::aleprocessTimerTxPacketTxLinkReleaseExpired() {
	dsp_controller->sendModemPacket(DspController::modempacket_LinkRelease, DspController::modembw20kHz, 0, 0);
}

void MainServiceInterface::aleprocessTimerRoffCallExpired() {
	dsp_controller->disableModemReceiver();
	proceedRxScanning();
}

void MainServiceInterface::aleprocessTimerCallTxHshakeRExpired() {
	dsp_controller->sendModemPacket(DspController::modempacket_HshakeReceiv, ale.call_bw, 0, 0);
}

void MainServiceInterface::aleprocessTimerCallRonHshakeTExpired() {
	dsp_controller->enableModemReceiver();
	setAlePhase(ALE_RX_CALL_RX_HSHAKE);
}

void MainServiceInterface::aleprocessTimerCallRoffHshakeTExpired() {
	dsp_controller->disableModemReceiver();
	proceedRxScanning();
}

void MainServiceInterface::aleprocessTimerRxNegStartExpired() {
	setAlePhase(ALE_RX_NEG_TX_QUAL);
	dsp_controller->enableModemTransmitter();
}

void MainServiceInterface::aleprocessTimerNegTxRespCallQualExpired() {
	respcallqual_packet_t respcallqual_packet;
	respcallqual_packet.errSignal = 0;
	respcallqual_packet.SNR = ale.call_snr;
	respcallqual_packet.reserved = 0;
	dsp_controller->sendModemPacket(DspController::modempacket_RespCallQual, ale.call_bw, (uint8_t *)&respcallqual_packet, sizeof(respcallqual_packet));
}

void MainServiceInterface::aleprocessTimerNegRonHshakeTransModeExpired() {
	dsp_controller->enableModemReceiver();
	setAlePhase(ALE_RX_NEG_RX_MODE);
}

void MainServiceInterface::aleprocessTimerNegRoffHshakeTransModeExpired() {
	dsp_controller->disableModemReceiver();
	if (ale.rcount < 3) {
		ale.timerNegTxHshakeReceiv[ale.rcount]->stop();
		ale.timerNegRonHshakeTrans[ale.rcount]->stop();
		ale.timerNegRoffHshakeTrans[ale.rcount]->stop();
		ale.rcount++;
	} else {
		proceedRxScanning();
	}
}

void MainServiceInterface::aleprocessTimerNegTxHshakeReceivExpired() {
	dsp_controller->sendModemPacket(DspController::modempacket_HshakeReceiv, ale.call_bw, 0, 0);
}

void MainServiceInterface::aleprocessTimerNegRonHshakeTransExpired() {
	dsp_controller->enableModemReceiver();
	setAlePhase(ALE_RX_NEG_RX_HSHAKE);
}

void MainServiceInterface::aleprocessTimerNegRoffHshakeTransExpired() {
	dsp_controller->disableModemReceiver();
	if (ale.rcount < 3) {
		ale.rcount++;
	} else {
		proceedRxScanning();
	}
}

void MainServiceInterface::aleprocessTimerMsgRoffHeadExpired() {
	dsp_controller->disableModemReceiver();
	ale.timerMsgTxRespPackQual[ale.vm_msg_cycle]->stop();
	ale.timerMsgRonHshakeT[ale.vm_msg_cycle]->stop();
	ale.timerMsgRoffHshakeT[ale.vm_msg_cycle]->stop();
}

void MainServiceInterface::aleprocessTimerMsgTxRespPackQualExpired() {
	resppackqual_packet_t resppackqual_packet;
	resppackqual_packet.packResult = 1;
	resppackqual_packet.SNR = 0;
	resppackqual_packet.reserved = 0;
	dsp_controller->sendModemPacket(DspController::modempacket_RespPackQual, ale.call_bw, (uint8_t *)&resppackqual_packet, sizeof(resppackqual_packet));
}

void MainServiceInterface::aleprocessTimerMsgRonHshakeTExpired() {
	setAlePhase(ALE_RX_VM_RX_MSG_HSHAKE);
	dsp_controller->enableModemReceiver();
}

void MainServiceInterface::aleprocessTimerMsgRoffHshakeTExpired() {
	setAlePhase(ALE_RX_VM_RX_MSGHEAD);
}

void MainServiceInterface::aleprocessTimerRxMsgCycleExpired() {
	if (ale.phase == ALE_RX_VM_RX_MSG_HSHAKE) {
		setPacketRxPhase();
		ale.timerPacketRoffHead->start(ale.tPacketSync, TIMER_VALUE_tDataRoffHeadDelay(ale.vm_sform_c));
		setAlePhase(ALE_RX_VM_RX_PACKET);
	} else {
		ale.vm_msg_cycle++;
		if (ale.vm_msg_cycle < 3) {
			dsp_controller->enableModemReceiver();
		} else {
			setAleState(AleState_RX_VM_FAIL);
			stopAleSession();
		}
	}
}

void MainServiceInterface::setPacketRxPhase() {
	ale.tPacketSync.shift((ale.vm_msg_cycle + 1)*TIMER_VALUE_tDataCycle(-1));
	ale.vm_sform_c = ALE_VM_INITIAL_SFORM;
	ale.vm_f_idx = 0;
	ale.vm_last_result = true;
	setAlePhase(ALE_RX_VM_RX_PACKET);
	ale.result = AleResultVoiceMail;
	stopVmMsgRxTimers();
}

bool MainServiceInterface::processPacketReceivedPacket(uint8_t *data) {
	AleVmPacket *packet = (AleVmPacket *)data;
	CRC32 crc;
	crc.update(packet->num_data, sizeof(packet->num_data));
	if (!(crc.result() == packet->crc))
		return false;
	uint8_t num = packet->num_data[0] & 0x3F;
	if(!(num == ale.vm_f_idx)) {
		if (num == (ale.vm_f_idx - 1))
			return true;
		else
			return false;
	}
	std::copy(packet->num_data, packet->num_data + sizeof(packet->num_data), ale.vm_fragments[ale.vm_f_idx].num_data);
	ale.vm_f_idx++;
	setAleVmProgress(ale.vm_f_idx/ale.vm_f_count);
	return true;
}

void MainServiceInterface::processPacketMissedPacket() {
	qmDebugMessage(QmDebug::Warning, "ale vm packHead missed (oh shit... SESSION HAS BEEN BROKEN !!!)");
	//...
}

void MainServiceInterface::startRxPacketResponse() {
	dsp_controller->enableModemTransmitter();
	ale.vm_last_result = ale.vm_packet_result;
	setAlePhase(ALE_RX_VM_TX_PACK_RESP);
	ale.timerPacketTxRespPackQual->start(ale.tPacketSync, TIMER_VALUE_tDataTxRespPackQualDelay(ale.vm_sform_n));
}

void MainServiceInterface::startRxPacketLinkRelease() {
	ale.timerRxPacketTxLinkRelease->start(ale.tPacketSync, TIMER_VALUE_tDataTxRespPackQualDelay(ale.vm_sform_n));
	dsp_controller->enableModemTransmitter();
	setAlePhase(ALE_RX_VM_TX_LINK_RELEASE);
}

void MainServiceInterface::processPacketReceivedAck() {
	ale.vm_sform_c = ale.vm_sform_n;
	ale.timerRxPacketSync->start(ale.tPacketSync, TIMER_VALUE_tDataCycle(ale.vm_sform_c));
	ale.tPacketSync.shift(TIMER_VALUE_tDataCycle(ale.vm_sform_c));
}

void MainServiceInterface::processPacketMissedAck() {
	qmDebugMessage(QmDebug::Warning, "ale vm HshakeTrans missed (oh shit... SESSION HAS BEEN BROKEN !!!)");
	//...
}

void MainServiceInterface::aleprocessRxPacketSync() {
	ale.timerPacketRoffHead->start(ale.tPacketSync, TIMER_VALUE_tDataRoffHeadDelay(ale.vm_sform_c));
	setAlePhase(ALE_RX_VM_RX_PACKET);
}

void MainServiceInterface::aleprocessPacketRoffHeadExpired() {
	processPacketMissedPacket();
}

void MainServiceInterface::aleprocessPacketTxRespPackQualExpired() {
	resppackqual_packet_t resppackqual_packet;
	resppackqual_packet.packResult = ale.vm_packet_result;
	resppackqual_packet.SNR = ale.vm_packet_snr;
	resppackqual_packet.reserved = 0;
	dsp_controller->sendModemPacket(DspController::modempacket_RespPackQual, ale.call_bw, (uint8_t *)&resppackqual_packet, sizeof(resppackqual_packet));
}

void MainServiceInterface::aleprocessTimerRxPacketTxLinkReleaseExpired() {
	dsp_controller->sendModemPacket(DspController::modempacket_LinkRelease, ale.call_bw, 0, 0);
}

void MainServiceInterface::aleprocessPacketRonHshakeTExpired() {
	ale.timerPacketRoffHshakeT->start(ale.tPacketSync, TIMER_VALUE_tDataRoffHshakeTDelay(ale.vm_sform_n));
	dsp_controller->enableModemReceiver();
	setAlePhase(ALE_RX_VM_RX_PACK_HSHAKE);
}

void MainServiceInterface::aleprocessPacketRoffHshakeTExpired() {
	processPacketMissedAck();
}

} /* namespace Multiradio */

#include "qmdebug_domains_start.h"
QMDEBUG_DEFINE_DOMAIN(mrd_mainservice, LevelInfo)
#include "qmdebug_domains_end.h"
