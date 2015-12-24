/**
 ******************************************************************************
 * @file    dspcontroller.h
 * @author  Artem Pisarenko, PMR dept. software team, ONIIP, PJSC
 * @date    22.12.2015
 *
 ******************************************************************************
 */

#ifndef FIRMWARE_APP_MRD_DSPCONTROLLER_H_
#define FIRMWARE_APP_MRD_DSPCONTROLLER_H_

#include "qmobject.h"

class QmTimer;
class QmIopin;

namespace Multiradio {

class DspTransport;
struct DspCommand;

class DspController : public QmObject
{
public:
	enum RadioMode {
		RadioModeOff = 0,
		RadioModeCarrierTx = 1,
		RadioModeUSB = 7,
		RadioModeFM = 9,
		RadioModeSazhenData = 11
	};

	DspController(int uart_resource, int reset_iopin_resource, QmObject *parent);
	~DspController();
	void reset();
	void setRadioParameters(RadioMode mode, uint32_t frequency);
	void setRadioRx();
	void setRadioTx();

	sigc::signal<void> started;

private:
	friend struct DspCommand;

	enum Module {
		RxRadiopath,
		TxRadiopath
	};
	enum RxParameterCode {
		RxFrequency = 1,
		RxRadioMode = 2
	};
	enum TxParameterCode {
		TxFrequency = 1,
		TxRadioMode = 2
	};
	union ParameterValue {
		uint32_t frequency;
		RadioMode radio_mode;
	};
	enum RadioDirection {
		RadioDirectionInvalid,
		RadioDirectionRx,
		RadioDirectionTx
	};

	void initResetState();
	void processStartup(uint16_t id, uint16_t major_version, uint16_t minor_version);
	void processStartupTimeout();
	void processCommandTimeout();
	void processCommandResponse(bool success, Module module, int code, ParameterValue value);
	void syncPendingCommand();
	void processRadioState();
	void syncRadioState();
	void sendCommand(Module module, int code, ParameterValue value);
	void processReceivedFrame(uint8_t address, uint8_t *data, int data_size);

	bool is_ready;
	QmIopin *reset_iopin;
	DspTransport *transport;
	QmTimer *startup_timer, *command_timer;
	enum {
		radiostateSync,
		radiostateCmdModeOff,
		radiostateCmdRxFreq,
		radiostateCmdTxFreq,
		radiostateCmdLastModeOff,
		radiostateCmdRxMode,
		radiostateCmdTxMode
	} radio_state;
	RadioMode current_radio_mode;
	RadioDirection current_radio_direction;
	uint32_t current_radio_frequency;
	DspCommand *pending_command;
};

} /* namespace Multiradio */

#endif /* FIRMWARE_APP_MRD_DSPCONTROLLER_H_ */
