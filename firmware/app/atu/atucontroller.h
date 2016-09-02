/**
 ******************************************************************************
 * @file    atucontroller.h
 * @author  Artem Pisarenko, PMR dept. software team, ONIIP, PJSC
 * @date    11.01.2016
 *
 ******************************************************************************
 */

#ifndef FIRMWARE_APP_ATU_ATUCONTROLLER_H_
#define FIRMWARE_APP_ATU_ATUCONTROLLER_H_

#include "qmobject.h"
#include <sigc++config.h>

class QmTimer;
class QmUart;
class QmIopin;

namespace Multiradio {

class AtuController : public QmObject
{
public:
	enum Mode {
		modeNone,
		modeMalfunction,
		modeStartingBypass,
		modeBypass,
		modePrepareTuning,
		modeStartFullTuning,
		modeTuning,
		modeActiveTx
	};

	AtuController(int uart_resource, int iopin_resource, QmObject *parent);
	~AtuController();
	void startServicing();
	bool isDeviceConnected();
	Mode getMode();
	bool enterBypassMode(uint32_t frequency);
	bool tuneTxMode(uint32_t frequency);
	void setNextTuningParams(bool force_full);
	void acknowledgeTxRequest();
	void setRadioPowerOff(bool enable);

	void setMinimalActivityMode(bool enabled);

	sigc::signal<void, Mode/*new_mode*/> modeChanged;
	sigc::signal<void, bool/*enable*/> requestTx;

private:
	enum CommandId {
		commandInactive = 0,
		commandRequestState = 0x41,
		commandEnterBypassMode = 0x59,
		commandEnterFullTuningMode = 0x46,
		commandEnterQuickTuningMode = 0x66,
		commandRequestTWF = 0x4B
	};
	enum FrameId {
		frameid_NAK = 0x15,
		frameid_A = 0x41,
		frameid_D = 0x44,
		frameid_f = 0x66,
		frameid_F = 0x46,
		frameid_K = 0x4B,
		frameid_U = 0x55,
		frameid_Y = 0x59
	};

	void setMode(Mode mode);
	void scan();
	void startCommand(CommandId id, const uint8_t *data, int data_len, int repeat_count, int timeout = 10);
	void finishCommand();
	void tryRepeatCommand();
	void processReceivedTuningFrame(uint8_t id, uint8_t *data, int data_len);
	void processReceivedPrepareTuningFrame(uint8_t id, uint8_t *data, int data_len);
	void processTxTuneTimeout();
	void processReceivedStateMessage(uint8_t *data, int data_len);
	void processReceivedBypassModeMessage();
	void processReceivedTWFMessage(uint8_t *data, int data_len);
	void processReceivedUnexpectedFrame(uint8_t id);
	void processReceivedFrame(uint8_t id, uint8_t *data, int data_len);
	void sendFrame(uint8_t id, const uint8_t *data, int data_len);
	void sendNak();
	void processUartReceivedData();
	void setAntenna(uint32_t frequency);
	void processDeferred();
	void executeEnterBypassMode();
	void executeTuneTxMode();


	Mode mode;
	bool force_next_tunetx_full;
	struct {
		CommandId id;
		uint8_t *data_buf;
		int data_len;
		int repeat_count;
	} command;
	QmUart *uart;
	QmTimer *scan_timer, *command_timeout_timer, *tx_tune_timer;
	enum {
		uartrxNone,
		uartrxFrame
	} uart_rx_state;
	struct {
		uint8_t id;
		uint8_t *data_buf;
		int data_len;
		bool truncated;
	} uart_rx_frame;
	uint8_t antenna;
	QmIopin *poff_iopin;
	bool deferred_enterbypass_active, deferred_tunetx_active;
	bool tx_tuning_state;
	uint32_t tunetx_frequency;
	bool last_tune_setup_valid;
	uint8_t last_tune_setup[5];

	bool minimal_activity_mode;
};

} /* namespace Multiradio */

#endif /* FIRMWARE_APP_ATU_ATUCONTROLLER_H_ */
