/**
  ******************************************************************************
  * @file    atudevice.h
  * @author  Artem Pisarenko, PMR dept. software team, ONIIP, PJSC
  * @date    18.01.2016
  * @brief   Интерфейс класса Qt, симулирующего устройство АСУ
  *
  ******************************************************************************
  */

#ifndef ATU_ATUDEVICE_H
#define ATU_ATUDEVICE_H

#include <qframe.h>
#include <qtimer.h>

namespace Ui {
class AtuDevice;
}

class UartInterface;

namespace QtHwEmu {

class AtuDevice : public QFrame
{
	Q_OBJECT

public:
	AtuDevice(int uart_resource, QWidget *parent = 0);
	~AtuDevice();

private Q_SLOTS:
	void on_btnDisconnect_clicked();
	void on_btnConnect_clicked();
	void cycleTuning();
	void transferRxNak(bool delayed = true);
	void transferRxErrorState(bool delayed = true);
	void processTransferedTxFrame(uint8_t id, uint8_t *data, int data_len);
	void transferRxFrame(uint8_t id, uint8_t *data, int data_len, bool delayed = true);
	void processTransferedTxData(const QByteArray &data);

private:
	enum State {
		stateDisconnected,
		stateIdle,
		stateBypass,
		stateTuning,
		stateTuned
	};
	enum FrameId {
		frameid_NAK = 0x15,
		frameid_A = 0x41,
		frameid_D = 0x44,
		frameid_F = 0x46,
		frameid_U = 0x55,
		frameid_Y = 0x59
	};

	Ui::AtuDevice *ui;
	bool connected;
	State state;
	int tune_cycle_counter;
	UartInterface *uart;
	QTimer tune_timer;
	enum {
		uarttxNone,
		uarttxFrame
	} uart_tx_state;
	struct {
		uint8_t id;
		uint8_t *data_buf;
		int data_len;
		bool truncated;
	} uart_tx_frame;

private:
	void setState(State state);
	void updateConnection(bool on);
};

} /* namespace QtHwEmu */

#endif // ATU_ATUDEVICE_H
