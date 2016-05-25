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

namespace Multiradio {

class Dispatcher;

class MainServiceInterface : QmObject
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
		AleState_PREPARING,
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
	uint8_t getAleRxAddress();

	sigc::signal<void, Status/*new_status*/> statusChanged;
	sigc::signal<void, AleState/*new_state*/> aleStateChanged;

private:
	friend class Dispatcher;

	MainServiceInterface(Dispatcher *dispatcher);
	~MainServiceInterface();
	void setStatus(Status value);

	Status current_status;
};

} /* namespace Multiradio */

#endif /* FIRMWARE_APP_MRD_MAINSERVICEINTERFACE_H_ */
