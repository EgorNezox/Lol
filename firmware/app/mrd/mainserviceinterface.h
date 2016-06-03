/**
 ******************************************************************************
 * @file    mainserviceinterface.h
 * @author  Artem Pisarenko, PMR dept. software team, ONIIP, PJSC
 * @date    30.10.2015
 *
 ******************************************************************************
 */

#ifndef FIRMWARE_APP_MRD_MAINSERVICEINTERFACE_H_
#define FIRMWARE_APP_MRD_MAINSERVICEINTERFACE_H_

#include "qmobject.h"
#include "qmtimestamp.h"
#include "qmtimer.h"
#include "qmabstimer.h"
#include "multiradio.h"
#include "../datastorage/fs.h"
#include "../dsp/dspcontroller.h"
#include "../navigation/navigator.h"
#include "../headset/controller.h"

namespace Multiradio {

class Dispatcher;

class MainServiceInterface : public QmObject
{
public:
	enum Status {
		StatusNotReady,
		StatusIdle,
		StatusVoiceRx,
		StatusVoiceTx,
		StatusTuningTx
	};
	enum AleResult {
		AleResultNone,
		AleResultVoiceMail
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

	Status getStatus();

	void startAleRx();
	void startAleTxVoiceMail(uint8_t address);
	AleResult stopAle();
	AleState getAleState();
	uint8_t getAleVmProgress();
	uint8_t getAleRxAddress();
	voice_message_t getAleRxVmMessage();

	sigc::signal<void, Status/*new_status*/> statusChanged;
	sigc::signal<void, AleState/*new_state*/> aleStateChanged;
	sigc::signal<void, uint8_t/*new_value*/> aleVmProgressUpdated;

private:
	friend class Dispatcher;

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
		ALE_TX_VM_START,
		ALE_TX_VM_TX_MSGHEAD,
		ALE_TX_VM_RX_MSG_RESP,
		ALE_TX_VM_TX_MSG_HSHAKE,
		ALE_TX_VM_TX_PACKET,
		ALE_TX_VM_RX_PACK_RESP,
		ALE_TX_VM_TX_PACK_HSHAKE,
		ALE_TX_VM_TX_LINK_RELEASE
	};
	struct __attribute__ ((__packed__)) AleVmPacket {
		uint8_t num_data[62];
		uint32_t crc;
	};
	enum AleVmAdaptationType {
		alevmadaptationNone,
		alevmadaptationUp,
		alevmadaptationDown
	};

	MainServiceInterface(Dispatcher *dispatcher, Navigation::Navigator *navigator);
	~MainServiceInterface();
	void setStatus(Status value);
	void setAleState(AleState value);
	void setAlePhase(AlePhase value);
	void setAleVmProgress(uint8_t value);
	bool startAleSession();
	void stopAleSession();
	void stopAleTimers();
	void stopVmMsgTimers();
	void stopAllTimers();
	void startVmTx();
	int convertSnrFromPacket(uint8_t value);
	void proceedTxCalling();
	bool evaluatePacketSNR(uint8_t snr);
	void aleprocessRadioReady();
	void aleprocessModemPacketTransmitted(DspController::ModemPacketType type);
	void aleprocessModemPacketFailedTx();
	void aleprocessModemPacketReceived(DspController::ModemPacketType type, uint8_t snr, DspController::ModemBandwidth bandwidth, uint8_t* data, int data_len);
	void aleprocessModemPacketStartedRx(DspController::ModemPacketType type, uint8_t snr, DspController::ModemBandwidth bandwidth, uint8_t* data, int data_len);
	void aleprocessModemPacketStartedRxPackHead(uint8_t snr, DspController::ModemBandwidth bandwidth, uint8_t param_signForm, uint8_t param_packCode, uint8_t* data, int data_len);
	void aleprocessModemPacketFailedRx(DspController::ModemPacketType type);
	void aleprocess1PPS();
	void aleprocessTimerGnssSyncExpired();
	void aleprocessTimerTxCallExpired();
	void aleprocessTimerCallRonHshakeRExpired();
	void aleprocessTimerCallRoffHshakeRExpired();
	void aleprocessTimerNegStartExpired();
	void aleprocessTimerNegRoffExpired();
	void aleprocessTimerNegTxHshakeTransModeExpired();
	void aleprocessTimerNegRonHshakeReceivExpired();
	void aleprocessTimerTxHshakeTransExpired();
	void aleprocessTimerDataStartExpired();
	void aleprocessTimerMsgTxHeadExpired();
	void aleprocessTimerMsgRonRespPackQualExpired();
	void aleprocessTimerMsgRoffRespPackQualExpired();
	void aleprocessTimerMsgTxHshakeTExpired();
	void aleprocessTimerMsgCycleExpired();
	void aleprocessPacketSync();
	void processPacketTxResponse(bool p_result, uint8_t p_snr);
	void processFailedPacketTxCycle();
	void startNextPacketTxCycle();
	void adaptPacketTxUp();
	bool adaptPacketTxDown();
	void aleprocessTimerPacketTxHeadDataExpired();
	void aleprocessTimerPacketRonRespPackQualExpired();
	void aleprocessTimerPacketRoffRespPackQualExpired();
	void aleprocessTimerPacketTxHshakeTExpired();
	void aleprocessTimerPacketTxLinkReleaseExpired();

	Status current_status;
	DataStorage::FS *data_storage_fs;
	DspController *dsp_controller;
	Navigation::Navigator *navigator;
	Headset::Controller *headset_controller;
	struct {
		AleState state;
		AlePhase phase;
		uint8_t vm_progress;
		ale_call_freqs_t call_freqs;
		uint8_t station_address, address;
		int supercycle, cycle;
		int rcount;
		int vm_size;
		int vm_f_count;
		std::vector<AleVmPacket> vm_fragments;
		int vm_f_idx;
		int vm_msg_cycle;
		int vm_sform_c;
		int vm_sform_p;
		int vm_ack_count;
		int vm_nack_count;
		int vm_snr_ack[3];
		int vm_snr_nack[3];
		AleVmAdaptationType vm_adaptation;
		QmTimer *timerRadioReady;
		QmTimer *timerGnssSync;
		QmTimer *timerTxCall;
		QmTimer *timerCallRonHshakeR;
		QmTimer *timerCallRoffHshakeR;
		QmTimer *timerCallTxHshakeT;
		QmTimer *timerNegStart[3];
		QmTimer *timerNegRoffRespCallQual[3];
		QmTimer *timerNegTxHshakeTransMode[3];
		QmTimer *timerNegRonHshakeReceiv[3];
		QmTimer *timerNegRoffHshakeReceiv[3];
		QmTimer *timerNegTxHshakeTrans[3];
		QmTimer *timerDataStart;
		QmTimestamp tPacketSync;
		QmTimer *timerMsgTxHead[3];
		QmTimer *timerMsgRonRespPackQual[3];
		QmTimer *timerMsgRoffRespPackQual[3];
		QmTimer *timerMsgTxHshakeT[3];
		QmTimer *timerMsgCycle[3];
		QmAbsTimer *timerPacketSync;
		QmAbsTimer *timerPacketTxHeadData;
		QmAbsTimer *timerPacketRonRespPackQual;
		QmAbsTimer *timerPacketRoffRespPackQual;
		QmAbsTimer *timerPacketTxHshakeT;
		QmAbsTimer *timerPacketTxLinkRelease;
	} ale;
};

} /* namespace Multiradio */

#endif /* FIRMWARE_APP_MRD_MAINSERVICEINTERFACE_H_ */
