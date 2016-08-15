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
    sigc::signal<void, uint8_t/*subdevice_code*/, uint8_t/*error_code*/> dspHardwareFailed;
	sigc::signal<void, AleState/*new_state*/> aleStateChanged;
	sigc::signal<void, uint8_t/*new_value*/> aleVmProgressUpdated;

private:
	friend class Dispatcher;

	enum AleFunctionalState {
		alefunctionIdle,
		alefunctionRx,
		alefunctionTx
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
		ALE_TX_VM_START,
		ALE_TX_VM_TX_MSGHEAD,
		ALE_TX_VM_RX_MSG_RESP,
		ALE_TX_VM_TX_MSG_HSHAKE,
		ALE_TX_VM_TX_PACKET,
		ALE_TX_VM_RX_PACK_RESP,
		ALE_TX_VM_TX_PACK_HSHAKE,
		ALE_TX_VM_TX_LINK_RELEASE,
		ALE_RX_SETUP,
		ALE_RX_SCAN,
		ALE_RX_CALL,
		ALE_RX_CALL_TX_HSHAKE,
		ALE_RX_CALL_RX_HSHAKE,
		ALE_RX_NEG_TX_QUAL,
		ALE_RX_NEG_RX_MODE,
		ALE_RX_NEG_TX_HSHAKE,
		ALE_RX_NEG_RX_HSHAKE,
		ALE_RX_VM_START,
		ALE_RX_VM_RX_MSGHEAD,
		ALE_RX_VM_TX_MSG_RESP,
		ALE_RX_VM_RX_MSG_HSHAKE,
		ALE_RX_VM_RX_PACKET,
		ALE_RX_VM_TX_PACK_RESP,
		ALE_RX_VM_TX_LINK_RELEASE,
		ALE_RX_VM_RX_PACK_HSHAKE
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



	MainServiceInterface(Dispatcher *dispatcher);
	~MainServiceInterface();
	void setStatus(Status value);
	void forwardDspHardwareFailure(uint8_t subdevice_code, uint8_t error_code);
	void printDebugAleTimings();
	void printDebugVmMessage(int groups, int packets, voice_message_t &message);
	void setAleState(AleState value);
	void setAlePhase(AlePhase value);
	void setAleVmProgress(uint8_t value);
	bool startAleSession();
	void stopAleSession();
	void stopAleRxTimers();
	void stopAleTxTimers();
	void stopVmMsgRxTimers();
	void stopVmMsgTxTimers();
	void stopAllRxTimers();
	void stopAllTxTimers();
	void startVmRx();
	void startVmTx();
	bool checkDwellStart(int &freq_idx);
	int convertSnrFromPacket(uint8_t value);
	uint8_t convertSnrToPacket(int value);
	void proceedRxScanning();
	void proceedTxCalling();
	bool evaluatePacketSNR(int8_t snr);
	void aleprocessRadioReady();
	void aleprocessModemPacketTransmitted(DspController::ModemPacketType type);
	void aleprocessModemPacketFailedTx();
	void aleprocessModemPacketReceived(DspController::ModemPacketType type, uint8_t snr, DspController::ModemBandwidth bandwidth, uint8_t* data, int data_len);
	void aleprocessModemPacketStartedRxPackHead(uint8_t snr, DspController::ModemBandwidth bandwidth, uint8_t param_signForm, uint8_t param_packCode, uint8_t* data, int data_len);
	void aleprocessModemPacketFailedRx(DspController::ModemPacketType type);
	void aleprocess1PPS();
	void aleprocessTimerDataStartExpired();
	void aleprocessTimerGnssSyncExpired();
	void aleprocessTimerTxCallExpired();
	void aleprocessTimerCallRonHshakeRExpired();
	void aleprocessTimerCallRoffHshakeRExpired();
	void aleprocessTimerTxNegStartExpired();
	void aleprocessTimerNegRoffExpired();
	void aleprocessTimerNegTxHshakeTransModeExpired();
	void aleprocessTimerNegRonHshakeReceivExpired();
	void aleprocessTimerTxHshakeTransExpired();
	void aleprocessTimerMsgTxHeadExpired();
	void aleprocessTimerMsgRonRespPackQualExpired();
	void aleprocessTimerMsgRoffRespPackQualExpired();
	void aleprocessTimerMsgTxHshakeTExpired();
	void aleprocessTimerTxMsgCycleExpired();
	void aleprocessTxPacketSync();
	void processPacketTxResponse(bool p_result, uint8_t p_snr);
	void processFailedPacketTxCycle();
	void startNextPacketTxCycle();
	void adaptPacketTxUp();
	bool adaptPacketTxDown();
	void aleprocessTimerPacketTxHeadDataExpired();
	void aleprocessTimerPacketRonRespPackQualExpired();
	void aleprocessTimerPacketRoffRespPackQualExpired();
	void aleprocessTimerPacketTxHshakeTExpired();
	void aleprocessTimerTxPacketTxLinkReleaseExpired();
	void aleprocessTimerRoffCallExpired();
	void aleprocessTimerCallTxHshakeRExpired();
	void aleprocessTimerCallRonHshakeTExpired();
	void aleprocessTimerCallRoffHshakeTExpired();
	void aleprocessTimerRxNegStartExpired();
	void aleprocessTimerNegTxRespCallQualExpired();
	void aleprocessTimerNegRonHshakeTransModeExpired();
	void aleprocessTimerNegRoffHshakeTransModeExpired();
	void aleprocessTimerNegTxHshakeReceivExpired();
	void aleprocessTimerNegRonHshakeTransExpired();
	void aleprocessTimerNegRoffHshakeTransExpired();
	void aleprocessTimerMsgRoffHeadExpired();
	void aleprocessTimerMsgTxRespPackQualExpired();
	void aleprocessTimerMsgRonHshakeTExpired();
	void aleprocessTimerMsgRoffHshakeTExpired();
	void aleprocessTimerRxMsgCycleExpired();
	void setPacketRxPhase();
	void processFailedPacketRxCycle();
	bool processPacketReceivedPacket(uint8_t *data);
	void processPacketMissedPacket();
	void startRxPacketResponse();
	void startRxPacketLinkRelease();
	void processPacketReceivedAck();
	void processPacketMissedAck();
	void aleprocessRxPacketSync();
	void aleprocessPacketRoffHeadExpired();
	void aleprocessPacketTxRespPackQualExpired();
	void aleprocessTimerRxPacketTxLinkReleaseExpired();
	void aleprocessPacketRonHshakeTExpired();
	void aleprocessPacketRoffHshakeTExpired();

	Status current_status;
	Dispatcher *dispatcher;
	struct {
		AleFunctionalState f_state;
		AleState state;
		AlePhase phase;
		AleResult result;
		uint8_t vm_progress;
		ale_call_freqs_t call_freqs;
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
		uint8_t vm_packet_snr;
		QmTimer *timerRadioReady;
		QmTimer *timerGnssSync;
		QmTimestamp tCallStartSync;
		QmAbsTimer *timerTxCall;
		QmTimer *timerRoffCall;
		QmAbsTimer *timerCallRonHshakeR;
		QmAbsTimer *timerCallTxHshakeR;
		QmAbsTimer *timerCallRoffHshakeR;
		QmAbsTimer *timerCallRonHshakeT;
		QmAbsTimer *timerCallTxHshakeT;
		QmAbsTimer *timerCallRoffHshakeT;
		QmAbsTimer *timerRxNegStart[3];
		QmAbsTimer *timerTxNegStart[3];
		QmAbsTimer *timerNegTxRespCallQual[3];
		QmAbsTimer *timerNegRoffRespCallQual[3];
		QmAbsTimer *timerNegRonHshakeTransMode[3];
		QmAbsTimer *timerNegTxHshakeTransMode[3];
		QmAbsTimer *timerNegRoffHshakeTransMode[3];
		QmAbsTimer *timerNegRonHshakeReceiv[3];
		QmAbsTimer *timerNegTxHshakeReceiv[3];
		QmAbsTimer *timerNegRoffHshakeReceiv[3];
		QmAbsTimer *timerNegRonHshakeTrans[3];
		QmAbsTimer *timerNegTxHshakeTrans[3];
		QmAbsTimer *timerNegRoffHshakeTrans[3];
		QmAbsTimer *timerDataStart;
		QmTimestamp tPacketSync;
		QmAbsTimer *timerMsgTxHead[3];
		QmAbsTimer *timerMsgRoffHead[3];
		QmAbsTimer *timerMsgRonRespPackQual[3];
		QmAbsTimer *timerMsgTxRespPackQual[3];
		QmAbsTimer *timerMsgRoffRespPackQual[3];
		QmAbsTimer *timerMsgRonHshakeT[3];
		QmAbsTimer *timerMsgTxHshakeT[3];
		QmAbsTimer *timerMsgRoffHshakeT[3];
		QmAbsTimer *timerRxMsgCycle[3];
		QmAbsTimer *timerTxMsgCycle[3];
		QmAbsTimer *timerTxPacketSync;
		QmAbsTimer *timerPacketTxHeadData;
		QmAbsTimer *timerPacketRonRespPackQual;
		QmAbsTimer *timerPacketRoffRespPackQual;
		QmAbsTimer *timerPacketTxHshakeT;
		QmAbsTimer *timerTxPacketTxLinkRelease;
		QmAbsTimer *timerRxPacketSync;
		QmAbsTimer *timerPacketRoffHead;
		QmAbsTimer *timerPacketTxRespPackQual;
		QmAbsTimer *timerPacketRonHshakeT;
		QmAbsTimer *timerPacketRoffHshakeT;
		QmAbsTimer *timerRxPacketTxLinkRelease;
	} ale;
};

} /* namespace Multiradio */

#endif /* FIRMWARE_APP_MRD_MAINSERVICEINTERFACE_H_ */
