/**
 ******************************************************************************
 * @file    smarttransport.cpp
 * @author  Petr Dmitriev
 * @date    28.01.2016
 *
 ******************************************************************************
 */

#define QMDEBUGDOMAIN	headset
#include "qmdebug.h"
#include "qmcrc.h"
#include "qmuart.h"

#include "smarttransport.h"

#define MAX_FRAME_SIZE	((208/*data*/ + 2/*crc*/) * 2/*ce*/ + 1/*cmd*/ + 1/*bof*/ + 1/*eof*/)

#define FRAME_START_DELIMITER	0xC0
#define FRAME_END_DELIMITER		0xC1

namespace Headset {

const int SmartTransport::MAX_FRAME_DATA_SIZE = 208;

SmartTransport::SmartTransport(int uart_resource, int max_tx_queue_size, QmObject *parent) :
	QmObject(parent),
	rx_state(rxstateNone), rx_frame_size(-1)
{
	rx_frame_buf = new uint8_t[MAX_FRAME_SIZE];
	QmUart::ConfigStruct uart_config;
	uart_config.baud_rate = 115200;
	uart_config.stop_bits = QmUart::StopBits_1;
	uart_config.parity = QmUart::Parity_None;
	uart_config.flow_control = QmUart::FlowControl_None;
	uart_config.rx_buffer_size = 1024;
	uart_config.tx_buffer_size = max_tx_queue_size * MAX_FRAME_SIZE;
	uart_config.io_pending_interval = 10;
	uart = new QmUart(uart_resource, &uart_config, this);
	uart->dataReceived.connect(sigc::mem_fun(this, &SmartTransport::processUartReceivedData));
	uart->rxError.connect(sigc::mem_fun(this, &SmartTransport::processUartReceivedErrors));
}

SmartTransport::~SmartTransport() {
	uart->close();
	delete[] rx_frame_buf;
}

void SmartTransport::enable() {
	qmDebugMessage(QmDebug::Info, "uart open");
	uart->open();
}

void SmartTransport::disable() {
	qmDebugMessage(QmDebug::Info, "uart close");
	uart->close();
	dropRxSync();
}

void SmartTransport::transmitFrame(uint8_t cmd, uint8_t *data, int data_len) {

}

void SmartTransport::processUartReceivedData() {
	uint8_t byte;
	qmDebugMessage(QmDebug::Dump, "uart rx data...");
	while (uart->readData(&byte, 1)) {
		switch (rx_state) {
		case rxstateNone: {
			if (byte == FRAME_START_DELIMITER) {
				qmDebugMessage(QmDebug::Info, "uart rx: - frame start synchronized");
				rx_state = rxstateFrame;
				rx_frame_size = 0;
			} else {
				qmDebugMessage(QmDebug::Dump, "uart rx: - ignoring out-of-sync byte 0x%02X", byte);
			}
			break;
		}
		case rxstateFrame: {
			if (rx_frame_size < MAX_FRAME_SIZE) {
				qmDebugMessage(QmDebug::Dump, "uart rx: - frame data 0x%02X", byte);
				rx_frame_buf[rx_frame_size++] = byte;
				if (byte == FRAME_END_DELIMITER) {
					qmDebugMessage(QmDebug::Info, "uart rx: - frame end");
					rx_state = rxstateNone;
				}
			} else {
				qmDebugMessage(QmDebug::Warning, "uart rx: - sync lost (unexpected frame end 0x%02X)", byte);
				rx_state = rxstateNone;
			}
		}
		}
	}
}

void SmartTransport::processUartReceivedErrors(bool data_errors, bool overflow) {
	QM_UNUSED(data_errors);
	QM_UNUSED(overflow);
	if (data_errors)
		qmDebugMessage(QmDebug::Info, "uart rx data errors");
	if (overflow)
		qmDebugMessage(QmDebug::Info, "uart rx overflow");
	uart->readData(0, uart->getRxDataAvailable()); // flush received chunks
	dropRxSync();
}

void SmartTransport::dropRxSync() {
	if (rx_state == rxstateFrame) {
		qmDebugMessage(QmDebug::Dump, "uart rx: dropping frame");
	}
	rx_state = rxstateNone;
}

} /* namespace Headset */
