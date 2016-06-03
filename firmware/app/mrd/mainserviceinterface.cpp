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
#define TIMER_VALUE_tDataStart_offset (ALE_TIME_TMaxEthX + ALE_TIME_TMaxOpenTuneX + ALE_TIME_dTInit)
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
	ale.state = AleState_IDLE;
	ale.phase = ALE_STOPPED;
	ale.timerRadioReady = new QmTimer(true, this);
	ale.timerRadioReady->timeout.connect(sigc::mem_fun(this, &MainServiceInterface::aleprocessRadioReady));
	ale.timerGnssSync = new QmTimer(true, this);
	ale.timerGnssSync->timeout.connect(sigc::mem_fun(this, &MainServiceInterface::aleprocessTimerGnssSyncExpired));
	ale.timerTxCall = new QmTimer(true, this);
	ale.timerTxCall->timeout.connect(sigc::mem_fun(this, &MainServiceInterface::aleprocessTimerTxCallExpired));
	ale.timerCallRonHshakeR = new QmTimer(true, this);
	ale.timerCallRonHshakeR->timeout.connect(sigc::mem_fun(this, &MainServiceInterface::aleprocessTimerCallRonHshakeRExpired));
	ale.timerCallRoffHshakeR = new QmTimer(true, this);
	ale.timerCallRoffHshakeR->timeout.connect(sigc::mem_fun(this, &MainServiceInterface::aleprocessTimerCallRoffHshakeRExpired));
	ale.timerCallTxHshakeT = new QmTimer(true, this);
	ale.timerCallTxHshakeT->timeout.connect(sigc::mem_fun(this, &MainServiceInterface::aleprocessTimerTxHshakeTransExpired));
	for (int i = 0; i < 3; i++) {
		ale.timerNegStart[i] = new QmTimer(true, this);
		ale.timerNegStart[i]->timeout.connect(sigc::mem_fun(this, &MainServiceInterface::aleprocessTimerNegStartExpired));
		ale.timerNegRoffRespCallQual[i] = new QmTimer(true, this);
		ale.timerNegRoffRespCallQual[i]->timeout.connect(sigc::mem_fun(this, &MainServiceInterface::aleprocessTimerNegRoffExpired));
		ale.timerNegTxHshakeTransMode[i] = new QmTimer(true, this);
		ale.timerNegTxHshakeTransMode[i]->timeout.connect(sigc::mem_fun(this, &MainServiceInterface::aleprocessTimerNegTxHshakeTransModeExpired));
		ale.timerNegRonHshakeReceiv[i] = new QmTimer(true, this);
		ale.timerNegRonHshakeReceiv[i]->timeout.connect(sigc::mem_fun(this, &MainServiceInterface::aleprocessTimerNegRonHshakeReceivExpired));
		ale.timerNegRoffHshakeReceiv[i] = new QmTimer(true, this);
		ale.timerNegRoffHshakeReceiv[i]->timeout.connect(sigc::mem_fun(this, &MainServiceInterface::aleprocessTimerNegRoffExpired));
		ale.timerNegTxHshakeTrans[i] = new QmTimer(true, this);
		ale.timerNegTxHshakeTrans[i]->timeout.connect(sigc::mem_fun(this, &MainServiceInterface::aleprocessTimerTxHshakeTransExpired));
	}
	ale.timerDataStart = new QmTimer(true, this);
	ale.timerDataStart->timeout.connect(sigc::mem_fun(this, &MainServiceInterface::aleprocessTimerDataStartExpired));
	for (int i = 0; i < 3; i++) {
		ale.timerMsgTxHead[i] = new QmTimer(true, this);
		ale.timerMsgTxHead[i]->timeout.connect(sigc::mem_fun(this, &MainServiceInterface::aleprocessTimerMsgTxHeadExpired));
		ale.timerMsgRonRespPackQual[i] = new QmTimer(true, this);
		ale.timerMsgRonRespPackQual[i]->timeout.connect(sigc::mem_fun(this, &MainServiceInterface::aleprocessTimerMsgRonRespPackQualExpired));
		ale.timerMsgRoffRespPackQual[i] = new QmTimer(true, this);
		ale.timerMsgRoffRespPackQual[i]->timeout.connect(sigc::mem_fun(this, &MainServiceInterface::aleprocessTimerMsgRoffRespPackQualExpired));
		ale.timerMsgTxHshakeT[i] = new QmTimer(true, this);
		ale.timerMsgTxHshakeT[i]->timeout.connect(sigc::mem_fun(this, &MainServiceInterface::aleprocessTimerMsgTxHshakeTExpired));
		ale.timerMsgCycle[i] = new QmTimer(true, this);
		ale.timerMsgCycle[i]->timeout.connect(sigc::mem_fun(this, &MainServiceInterface::aleprocessTimerMsgCycleExpired));
	}
	ale.timerPacketSync = new QmAbsTimer(this);
	ale.timerPacketSync->timeout.connect(sigc::mem_fun(this, &MainServiceInterface::aleprocessPacketSync));
	ale.timerPacketTxHeadData = new QmAbsTimer(this);
	ale.timerPacketTxHeadData->timeout.connect(sigc::mem_fun(this, &MainServiceInterface::aleprocessTimerPacketTxHeadDataExpired));
	ale.timerPacketRonRespPackQual = new QmAbsTimer(this);
	ale.timerPacketRonRespPackQual->timeout.connect(sigc::mem_fun(this, &MainServiceInterface::aleprocessTimerPacketRonRespPackQualExpired));
	ale.timerPacketRoffRespPackQual = new QmAbsTimer(this);
	ale.timerPacketRoffRespPackQual->timeout.connect(sigc::mem_fun(this, &MainServiceInterface::aleprocessTimerPacketRoffRespPackQualExpired));
	ale.timerPacketTxHshakeT = new QmAbsTimer(this);
	ale.timerPacketTxHshakeT->timeout.connect(sigc::mem_fun(this, &MainServiceInterface::aleprocessTimerPacketTxHshakeTExpired));
	ale.timerPacketTxLinkRelease = new QmAbsTimer(this);
	ale.timerPacketTxLinkRelease->timeout.connect(sigc::mem_fun(this, &MainServiceInterface::aleprocessTimerPacketTxLinkReleaseExpired));
	if (navigator != 0)
		navigator->syncPulse.connect(sigc::mem_fun(this, &MainServiceInterface::aleprocess1PPS));
	dsp_controller->transmittedModemPacket.connect(sigc::mem_fun(this, &MainServiceInterface::aleprocessModemPacketTransmitted));
	dsp_controller->failedTxModemPacket.connect(sigc::mem_fun(this, &MainServiceInterface::aleprocessModemPacketFailedTx));
	dsp_controller->receivedModemPacket.connect(sigc::mem_fun(this, &MainServiceInterface::aleprocessModemPacketReceived));
	dsp_controller->startedRxModemPacket.connect(sigc::mem_fun(this, &MainServiceInterface::aleprocessModemPacketStartedRx));
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
	if (!startAleSession())
		return;
	//...
}

void MainServiceInterface::startAleTxVoiceMail(uint8_t address) {
	qmDebugMessage(QmDebug::Info, "starting ALE tx voice mail (address = %u)", address);
	if (!startAleSession())
		return;
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
	return AleResultNone;
}

MainServiceInterface::AleState MainServiceInterface::getAleState() {
	return ale.state;
}

uint8_t Multiradio::MainServiceInterface::getAleVmProgress() {
	return ale.vm_progress;
}

uint8_t MainServiceInterface::getAleRxAddress() {
	//...
	return 0;
}

voice_message_t MainServiceInterface::getAleRxVmMessage() {
	//...
	return voice_message_t();
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
	stopAllTimers();
	if (ale.phase == ALE_STOPPED) {
		setAleState(AleState_IDLE);
		return;
	}
	dsp_controller->disableModemReceiver();
	dsp_controller->disableModemTransmitter();
}

void MainServiceInterface::stopAleTimers() {
	ale.timerTxCall->stop();
	ale.timerCallRonHshakeR->stop();
	ale.timerCallRoffHshakeR->stop();
	ale.timerCallTxHshakeT->stop();
	for (int i = 0; i < 3; i++) {
		ale.timerNegStart[i]->stop();
		ale.timerNegRoffRespCallQual[i]->stop();
		ale.timerNegTxHshakeTransMode[i]->stop();
		ale.timerNegRonHshakeReceiv[i]->stop();
		ale.timerNegRoffHshakeReceiv[i]->stop();
		ale.timerNegTxHshakeTrans[i]->stop();
	}
}

void MainServiceInterface::stopVmMsgTimers() {
	for (int i = 0; i < 3; i++) {
		ale.timerMsgTxHead[i]->stop();
		ale.timerMsgRonRespPackQual[i]->stop();
		ale.timerMsgRoffRespPackQual[i]->stop();
		ale.timerMsgTxHshakeT[i]->stop();
		ale.timerMsgCycle[i]->stop();
	}
}

void MainServiceInterface::stopAllTimers() {
	ale.timerRadioReady->stop();
	ale.timerGnssSync->stop();
	stopAleTimers();
	ale.timerDataStart->stop();
	stopVmMsgTimers();
	ale.timerPacketSync->stop();
	ale.timerPacketTxHeadData->stop();
	ale.timerPacketRonRespPackQual->stop();
	ale.timerPacketRoffRespPackQual->stop();
	ale.timerPacketTxHshakeT->stop();
	ale.timerPacketTxLinkRelease->stop();
}

void MainServiceInterface::startVmTx() {
	ale.timerDataStart->start(TIMER_VALUE_tDataStart_offset);
	setAlePhase(ALE_TX_VM_START);
	ale.vm_msg_cycle = 0;
	ale.vm_progress = 0;
	setAleState(AleState_TX_VM_TRANSFER);
	dsp_controller->setModemReceiverPhase(DspController::modemphaseLinkEstablished);
}

int MainServiceInterface::convertSnrFromPacket(uint8_t value) {
	if (value == 0)
		return -19;
	if (value == 63)
		return 42;
	return (-20 + ((value - 1)/(62-1))*(20 + 41));
}

void MainServiceInterface::proceedTxCalling() {
	stopAleTimers();
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

bool MainServiceInterface::evaluatePacketSNR(uint8_t snr) {
	uint8_t snr_threshold = (ale.supercycle == 1)?ALE_CALL_SNR_HIGH:ALE_CALL_SNR_LOW;
	return (snr >= snr_threshold);
}

void MainServiceInterface::aleprocessRadioReady() {
	switch (ale.phase) {
	case ALE_TX_SETUP:
		setAlePhase(ALE_TX_CYCLE_CALL);
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
			stopAleTimers();
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
		if (ale.phase != ALE_TX_VM_TX_LINK_RELEASE)
			break;
		setAleState(AleState_TX_VM_COMPLETE_FULL);
		stopAleSession();
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

void MainServiceInterface::aleprocessModemPacketReceived(DspController::ModemPacketType type, uint8_t snr, DspController::ModemBandwidth bandwidth, uint8_t* data, int data_len) {
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
		ale.timerPacketTxLinkRelease->start(ale.tPacketSync, TIMER_VALUE_tDataTxHshakeTDelay(ale.vm_sform_c));
		dsp_controller->enableModemTransmitter();
		setAlePhase(ALE_TX_VM_TX_LINK_RELEASE);
		break;
	}
	default:
		break;
	}
}

void MainServiceInterface::aleprocessModemPacketStartedRx(DspController::ModemPacketType type, uint8_t snr, DspController::ModemBandwidth bandwidth, uint8_t* data, int data_len) {
	switch (type) {
	case DspController::modempacket_Call:

		break;
	default:
		break;
	}
}

void MainServiceInterface::aleprocessModemPacketStartedRxPackHead(uint8_t snr, DspController::ModemBandwidth bandwidth, uint8_t param_signForm, uint8_t param_packCode, uint8_t* data, int data_len) {
	//...
}

void MainServiceInterface::aleprocessModemPacketFailedRx(DspController::ModemPacketType type) {
	//...
}

void MainServiceInterface::aleprocess1PPS() {
	if (ale.phase == ALE_STOPPED)
		return;
	ale.timerGnssSync->start(1500);
	switch (ale.phase) {
	case ALE_TX_CYCLE_CALL: {
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
			break;
		int msecs = sec*1000+min*(60*1000)+hrs*(60*60*1000);
		if ((msecs % ALE_TIME_Tdwell) != 0)
			break;
		qmDebugMessage(QmDebug::Info, "ale dwell start");
		ale.timerTxCall->start(TIMER_VALUE_tTxCall);
		ale.timerCallRonHshakeR->start(TIMER_VALUE_tCallRonHshakeR);
		ale.timerCallRoffHshakeR->start(TIMER_VALUE_tCallRoffHshakeR);
		ale.timerCallTxHshakeT->start(TIMER_VALUE_tCallTxHshakeT);
		for (int i = 0; i < 3; i++) {
			int neg_start = TIMER_VALUE_tNegStart(i);
			ale.timerNegStart[i]->start(neg_start);
			ale.timerNegRoffRespCallQual[i]->start(neg_start + TIMER_VALUE_tNegRoffRespCallQual_offset);
			ale.timerNegTxHshakeTransMode[i]->start(neg_start + TIMER_VALUE_tNegTxHshakeTransMode_offset);
			ale.timerNegRonHshakeReceiv[i]->start(neg_start + TIMER_VALUE_tNegRonHshakeReceiv_offset);
			ale.timerNegRoffHshakeReceiv[i]->start(neg_start + TIMER_VALUE_tNegRoffHshakeReceiv_offset);
			ale.timerNegTxHshakeTrans[i]->start(neg_start + TIMER_VALUE_tNegTxHshakeTrans_offset);
		}
		int freq_idx = (msecs/ALE_TIME_Tdwell) % ale.call_freqs.size();
		dsp_controller->setRadioParameters(DspController::RadioModeSazhenData, ale.call_freqs[freq_idx]);
		dsp_controller->enableModemTransmitter();
		setAlePhase(ALE_TX_CALL);
		ale.rcount = 0;
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

void MainServiceInterface::aleprocessTimerNegStartExpired() {
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
	ale.tPacketSync.set();
	for (int i = 0; i < 3; i++) {
		int cycle = i*TIMER_VALUE_tDataCycle(-1);
		ale.timerMsgTxHead[i]->start(cycle + TIMER_VALUE_tDataTxHeadDelay(-1));
		ale.timerMsgRonRespPackQual[i]->start(cycle + TIMER_VALUE_tDataRonRespPackQualDelay(-1));
		ale.timerMsgRoffRespPackQual[i]->start(cycle + TIMER_VALUE_tDataRoffRespPackQualDelay(-1));
		ale.timerMsgTxHshakeT[i]->start(cycle + TIMER_VALUE_tDataTxHshakeTDelay(-1));
		ale.timerMsgCycle[i]->start(cycle);
	}
	dsp_controller->enableModemTransmitter();
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

void MainServiceInterface::aleprocessTimerMsgCycleExpired() {
	if (ale.phase != ALE_TX_VM_TX_MSG_HSHAKE) {
		ale.vm_msg_cycle++;
		if (ale.vm_msg_cycle < 3) {
			dsp_controller->enableModemTransmitter();
		} else {
			setAleState(AleState_TX_VM_FAIL);
			stopAleSession();
		}
	} else {
		stopVmMsgTimers();
		ale.tPacketSync.shift((ale.vm_msg_cycle + 1)*TIMER_VALUE_tDataCycle(-1));
		ale.vm_sform_c = ALE_VM_INITIAL_SFORM;
		ale.vm_sform_p = ale.vm_sform_c;
		ale.vm_f_idx = 0;
		ale.rcount = 0;
		ale.vm_ack_count = 0;
		ale.vm_nack_count = 0;
		aleprocessPacketSync();
	}
}

void MainServiceInterface::aleprocessPacketSync() {
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
	ale.timerPacketSync->start(ale.tPacketSync, TIMER_VALUE_tDataCycle(sform));
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

void MainServiceInterface::aleprocessTimerPacketTxLinkReleaseExpired() {
	dsp_controller->sendModemPacket(DspController::modempacket_LinkRelease, DspController::modembw20kHz, 0, 0);
}

} /* namespace Multiradio */

#include "qmdebug_domains_start.h"
QMDEBUG_DEFINE_DOMAIN(mrd_mainservice, LevelInfo)
#include "qmdebug_domains_end.h"
