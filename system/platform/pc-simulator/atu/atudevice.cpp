/**
  ******************************************************************************
  * @file    atudevice.cpp
  * @author  Artem Pisarenko, PMR dept. software team, ONIIP, PJSC
  * @date    18.01.2016
  * @brief   Реализация класса Qt, симулирующего устройство АСУ
  *
  ******************************************************************************
  */

#include "port_hardwareio/uartinterface.h"
#include "atudevice.h"
#include "ui_atudevice.h"

#define MAX_FRAME_DATA_SIZE		5
#define FRAME_SYMBOL_EOT		0x03

namespace QtHwEmu {

AtuDevice::AtuDevice(int uart_resource, QWidget *parent) :
    QFrame(parent),
	ui(new Ui::AtuDevice)
{
	ui->setupUi(this);
	setWindowFlags(Qt::Tool | Qt::CustomizeWindowHint | Qt::WindowTitleHint);
	uart_tx_frame.data_buf = new uint8_t[MAX_FRAME_DATA_SIZE];
	uart = UartInterface::createInstance(uart_resource);
	QObject::connect(uart, &UartInterface::txTransferred, this, &AtuDevice::processTransferedTxData);
	tune_timer.setSingleShot(true);
	tune_timer.setInterval(20);
	QObject::connect(&tune_timer, &QTimer::timeout, this, &AtuDevice::cycleTuning);
	updateConnection(false);
}

AtuDevice::~AtuDevice()
{
	UartInterface::destroyInstance(uart);
	delete[] uart_tx_frame.data_buf;
	delete ui;
}

void AtuDevice::on_btnConnect_clicked() {
	updateConnection(true);
}

void AtuDevice::on_btnDisconnect_clicked() {
	updateConnection(false);
}

void AtuDevice::setState(State state) {
	this->state = state;
	switch (state) {
	case stateDisconnected: ui->lblState->setText("State: DISCONNECTED"); break;
	case stateIdle: ui->lblState->setText("State: IDLE"); break;
	case stateBypass: ui->lblState->setText("State: BYPASS"); break;
	case stateTuning: ui->lblState->setText("State: TUNING"); break;
	case stateTuned: ui->lblState->setText("State: TUNED"); break;
	}
}

void AtuDevice::cycleTuning() {
	Q_ASSERT(state == stateTuning);
	if (tune_cycle_counter == 0) {
		uint8_t tune_data[5];
		for (unsigned int i = 0; i < sizeof(tune_data); i++)
			tune_data[i] = 0;
		transferRxFrame(frameid_F, tune_data, sizeof(tune_data), false);
		setState(stateTuned);
		return;
	}
	transferRxFrame(((tune_cycle_counter % 2) == 0)?frameid_D:frameid_U, 0, 0, false);
	tune_cycle_counter--;
}

void AtuDevice::updateConnection(bool on) {
	connected = on;
	ui->btnConnect->setEnabled(!on);
	ui->btnDisconnect->setEnabled(on);
	if (on) {
		setState(stateIdle);
	} else {
		setState(stateDisconnected);
		tune_timer.stop();
		uart_tx_state = uarttxNone;
		ui->lblTunedFrequency->clear();
	}
}

void AtuDevice::transferRxNak(bool delayed) {
	transferRxFrame(frameid_NAK, 0, 0, delayed);
}

void AtuDevice::transferRxErrorState(bool delayed) {
	uint8_t error_code = ui->sbErrorCode->value();
	transferRxFrame(frameid_A, &error_code, 1, delayed);
}

void AtuDevice::processTransferedTxFrame(uint8_t id, uint8_t* data, int data_len) {
	switch (state) {
	case stateDisconnected: {
		Q_ASSERT(0);
		break;
	}
	case stateIdle:
	case stateBypass:
	case stateTuned: {
		switch (id) {
		case frameid_A: {
			transferRxErrorState();
			break;
		}
		case frameid_F: {
			if (!(data_len >= 4)) {
				transferRxNak();
				break;
			}
			uint32_t frequency = 0;
			frequency |= (data[0] << 16) & 0xFF0000;
			frequency |= (data[1] << 8) & 0xFF00;
			frequency |= data[2] & 0xFF;
			frequency *= 10;
			if (ui->sbErrorCode->value() == 0) {
				setState(stateTuning);
				ui->lblTunedFrequency->setText(QString("Frequency: %1").arg(frequency));
				tune_cycle_counter = 50;
				transferRxFrame(frameid_U, 0, 0);
			} else {
				transferRxErrorState();
			}
			break;
		}
		case frameid_Y: {
			if (!(data_len >= 1)) {
				transferRxNak();
				break;
			}
			setState(stateBypass);
			transferRxFrame(frameid_Y, 0, 0);
			break;
		}
		case frameid_NAK:
			break;
		default:
			transferRxNak();
			break;
		}
		break;
	}
	case stateTuning: {
		switch (id) {
		case frameid_D:
		case frameid_U: {
			if (!(((tune_cycle_counter % 2) == 0) == (id == frameid_U))) {
				transferRxNak();
				break;
			}
			tune_timer.start();
			break;
		}
		case frameid_Y: {
			if (!(data_len >= 1)) {
				transferRxNak();
				break;
			}
			setState(stateBypass);
			tune_timer.stop();
			transferRxFrame(frameid_Y, 0, 0);
			break;
		}
		case frameid_NAK:
			break;
		default:
			transferRxNak();
			break;
		}
		break;
	}
	}
}

void AtuDevice::transferRxFrame(uint8_t id, uint8_t* data, int data_len, bool delayed) {
	uint8_t eot = FRAME_SYMBOL_EOT;
	QByteArray frame;
	frame.append((char *)&id, 1);
	if (data_len > 0)
		frame.append((char *)data, data_len);
	frame.append((char *)&eot, 1);
	if (!delayed)
		uart->transferRx(frame);
	else
		QMetaObject::invokeMethod(uart, "transferRx", Qt::QueuedConnection, Q_ARG(QByteArray, frame));
}

void AtuDevice::processTransferedTxData(const QByteArray& data) {
	for (int i = 0; i < data.size(); i++) {
		if (state == stateDisconnected)
			break;
		uint8_t byte = data.at(i);
		switch (uart_tx_state) {
		case uarttxNone:
			if (byte == FRAME_SYMBOL_EOT)
				break;
			uart_tx_frame.id = byte;
			uart_tx_frame.data_len = 0;
			uart_tx_frame.truncated = false;
			uart_tx_state = uarttxFrame;
			break;
		case uarttxFrame:
			if (byte != FRAME_SYMBOL_EOT) {
				if (uart_tx_frame.data_len < MAX_FRAME_DATA_SIZE) {
					uart_tx_frame.data_buf[uart_tx_frame.data_len++] = byte;
				} else {
					if (!uart_tx_frame.truncated)
						uart_tx_frame.truncated = true;
				}
			} else {
				uart_tx_state = uarttxNone;
				processTransferedTxFrame(uart_tx_frame.id, uart_tx_frame.data_buf, uart_tx_frame.data_len);
			}
			break;
		}
	}
}

} /* namespace QtHwEmu */
