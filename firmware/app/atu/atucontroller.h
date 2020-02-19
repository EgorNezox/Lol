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
	// перичисление набора состояний АНСУ
	enum Mode {
		modeNone,    // ansu not detect
		modeFault,   // ansu error
		modeBypass,  // ansu bypass mode
		modeTuning,  // ansu tuning
		modeActiveTx // ansu tx
	};

	AtuController(int uart_resource, int iopin_resource, QmObject *parent);
	~AtuController();

	// инициализация и запуск штатного режима опроса
	void startServicing();

	// пин готовности ансу
	bool isReady();

	// проверка подключения устройства
	bool isDeviceConnected();

	void ack();

	// получить режим устройства
	Mode getMode();

	// переход в режим обхода
	bool enterBypassMode(uint32_t frequency);

	// настройка передатчика с АНСУ
	bool tuneTxMode(uint32_t frequency);

	// установка параметров
	void setNextTuningParams(bool force_full);

	// включение - выключение работы с АНСУ
	void setMinimalActivityMode(bool enabled);

	sigc::signal<void, Mode/*new_mode*/> modeChanged;
	sigc::signal<void, bool/*enable*/>   requestTx;

	void executeTuneTxMode();

private:
	enum CommandId {
		commandInactive = 0,
		commandRequestState = 0x41,
		commandExitBypassMode = 0x58,
		commandEnterBypassMode = 0x59,
		commandEnterFullTuningMode = 0x46,
		commandEnterQuickTuningMode = 0x66,
		commandRequestTWF = 0x4B,
		commandSetAck = 0x6
	};
	enum FrameId {
		frameid_ack = 0x6,
		frameid_NAK = 0x15,
		frameid_A = 0x41,
		frameid_D = 0x44,
		frameid_f = 0x66,
		frameid_F = 0x46,
		frameid_K = 0x4B,
		frameid_U = 0x55,
		frameid_X = 0x58,
		frameid_Y = 0x59,
		frameid_V = 0x56
	};

	/* save mode ansu */
	void setMode(Mode mode);

	/* request state cadr - ping */
	void scan();

	/* send cmd for ansu with params */
	void startCommand(CommandId id, const uint8_t *data, int data_len, int repeat_count, int timeout = 10);

	void finishCommand();

	/* repeat not set cmd */
	void tryRepeatCommand();


//	void processReceivedTuningFrame(uint8_t id, uint8_t *data);

	/* timeout tune */
	void processTxTuneTimeout();

	/* check cadr state */
	void processReceivedStateMessage(uint8_t *data);

	/*  */
	void processReceivedBypassModeMessage();

	void processReceivedUnexpectedFrame(uint8_t id);

	void processReceivedFrame(uint8_t id, uint8_t *data);

	void sendFrame(uint8_t id, const uint8_t *data, int data_len);

	void sendNak();

	void processUartReceivedData();

	void setAntenna(uint32_t frequency);

	void processDeferred();

	void executeEnterBypassMode();

	void executeExitBypassMode();

	void setAnsuFreq();

	void requestKBW();

	void setForFastTune(uint8_t *data);

	void stateError(uint8_t error_code);


	uint8_t kbw_value = 0;

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
		int data_len, data_pos;
	} uart_rx_frame;

	uint8_t antenna;

	QmIopin *poff_iopin;
	QmIopin *pin_ready;

	uint32_t ansu_version;

	bool deferred_enterbypass_active, deferred_tunetx_active;
	bool tx_tuning_power_state, tx_quick_tuning_attempt;
	uint32_t tunetx_frequency;
	bool last_tune_setup_valid;

	// dataset for ansu state
	uint8_t last_tune_setup[6];

	bool minimal_activity_mode;
};

} /* namespace Multiradio */

#endif /* FIRMWARE_APP_ATU_ATUCONTROLLER_H_ */
