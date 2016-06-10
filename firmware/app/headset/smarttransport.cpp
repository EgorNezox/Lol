/**
 ******************************************************************************
 * @file    smarttransport.cpp
 * @author  Petr Dmitriev
 * @date    28.01.2016
 *
 ******************************************************************************
 */

#define QMDEBUGDOMAIN	hstransport
#include "qmdebug.h"
#include "qmendian.h"
#include "qmuart.h"

#include "smarttransport.h"
#include "headsetcrc.h"


#define MAX_FRAME_PAYLOAD_SIZE		(208 + 1/*cmd*/ + 2/*crc*/)
#define MAX_FRAME_CORRECTED_SIZE	(MAX_FRAME_PAYLOAD_SIZE * 2/*ce*/)
#define MAX_FRAME_TOTAL_SIZE		(MAX_FRAME_CORRECTED_SIZE + 1/*bof*/ + 1/*eof*/)

#define FRAME_START_DELIMITER	0xC0
#define FRAME_END_DELIMITER		0xC1
#define FRAME_SPEC_SYMBOL		0x7D

namespace Headset {

const int SmartTransport::MAX_FRAME_DATA_SIZE = MAX_FRAME_PAYLOAD_SIZE - 1/*cmd*/ - 2/*crc*/;

SmartTransport::SmartTransport(int uart_resource, int max_tx_queue_size, QmObject *parent) :
	QmObject(parent),
	rx_state(rxstateNone), rx_frame_size(-1)
{
	rx_frame_buf = new uint8_t[MAX_FRAME_TOTAL_SIZE];
	QmUart::ConfigStruct uart_config;
	uart_config.baud_rate = 115200;
	uart_config.stop_bits = QmUart::StopBits_1;
	uart_config.parity = QmUart::Parity_None;
	uart_config.flow_control = QmUart::FlowControl_None;
	uart_config.rx_buffer_size = 1024;
	uart_config.tx_buffer_size = max_tx_queue_size * MAX_FRAME_TOTAL_SIZE;
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

void SmartTransport::transmitCmd(uint8_t cmd, uint8_t *data, int data_len) {
	QM_ASSERT(data_len <= MAX_FRAME_DATA_SIZE);
	qmDebugMessage(QmDebug::Info, "transmitting frame (cmd=0x%02X, data_len=%d)", cmd, data_len);
	uint8_t frame_crc[2];
	qmToLittleEndian(calcFrameCRC(cmd, data, data_len), frame_crc);
	uint8_t frame_start = FRAME_START_DELIMITER;
	uint8_t frame_stop = FRAME_END_DELIMITER;

	uint8_t frame[MAX_FRAME_TOTAL_SIZE];
	int frame_len = 0;
	frame_len += encodeFrameData(&cmd, (uint8_t*)(frame + frame_len), 1);
	frame_len += encodeFrameData(data, (uint8_t*)(frame + frame_len), data_len);
	frame_len += encodeFrameData(frame_crc, (uint8_t*)(frame + frame_len), 2);
	QM_ASSERT(frame_len <= MAX_FRAME_TOTAL_SIZE);

	int64_t written = 0;
	written += uart->writeData(&frame_start, 1);
	written += uart->writeData(frame, frame_len);
	written += uart->writeData(&frame_stop, 1);
	QM_ASSERT(written == (2 + frame_len));
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
//				qmDebugMessage(QmDebug::Dump, "uart rx: - ignoring out-of-sync byte 0x%02X", byte);
			}
			break;
		}
		case rxstateFrame: {
			if (rx_frame_size < MAX_FRAME_TOTAL_SIZE) {
				if (byte == FRAME_END_DELIMITER) {
					qmDebugMessage(QmDebug::Info, "uart rx: - frame end");
					rx_state = rxstateNone;
					uint8_t frame_data[MAX_FRAME_PAYLOAD_SIZE];
					int frame_data_size = decodeFrameData(rx_frame_buf, frame_data, rx_frame_size);
					HeadsetCRC crc;
					crc.update(frame_data, (frame_data_size - 2));
					if (crc.result() != extractFrameCRC(frame_data, frame_data_size)) {
						qmDebugMessage(QmDebug::Warning, "uart rx: - bad frame (crc mismatch), dropping");
						break;
					}
					uint8_t cmd = frame_data[0];
					uint8_t* rx_data = (uint8_t*)(frame_data + 1);
					int rx_data_len = frame_data_size - 1/*cmd*/ - 2/*crc*/;
					qmDebugMessage(QmDebug::Info, "received frame (cmd=0x%02X, data_len=%u)", cmd, rx_data_len);
					receivedCmd(cmd, rx_data, rx_data_len);
				} else {
//					qmDebugMessage(QmDebug::Dump, "uart rx: - frame data 0x%02X", byte);
					rx_frame_buf[rx_frame_size++] = byte;
				}
			} else {
				qmDebugMessage(QmDebug::Warning, "uart rx: - sync lost (frame buffer overflow)");
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

uint16_t SmartTransport::calcFrameCRC(uint8_t cmd, uint8_t *data, int data_len) {
	HeadsetCRC crc;
	crc.update(&cmd, 1);
	crc.update(data, data_len);
	return (uint16_t)~crc.result();
}

uint16_t SmartTransport::extractFrameCRC(uint8_t *data, int data_len) {
	return (uint16_t)~qmFromLittleEndian<uint16_t>( (uint8_t*)(data+(data_len - 2)) );
}

int SmartTransport::encodeFrameData(uint8_t* input_data, uint8_t* output_data, int data_len) {
	QM_ASSERT(data_len <= MAX_FRAME_PAYLOAD_SIZE);
	int oindex = 0;
	for (int i = 0; i < data_len; ++i) {
		QM_ASSERT(oindex < MAX_FRAME_CORRECTED_SIZE);
		if (input_data[i] == FRAME_START_DELIMITER || input_data[i] == FRAME_END_DELIMITER || input_data[i] == FRAME_SPEC_SYMBOL) {
			output_data[oindex++] = FRAME_SPEC_SYMBOL;
			output_data[oindex++] = input_data[i] ^ 0x02;
		} else {
			output_data[oindex++] = input_data[i];
		}
	}
	return oindex;
}

int SmartTransport::decodeFrameData(uint8_t* input_data, uint8_t* output_data, int data_len) {
	QM_ASSERT(data_len <= MAX_FRAME_CORRECTED_SIZE);
	int oindex = 0;
	int i = 0;
	while (i < data_len) {
		QM_ASSERT(oindex < MAX_FRAME_PAYLOAD_SIZE);
		if (input_data[i] == FRAME_SPEC_SYMBOL) {
			output_data[oindex++] = input_data[i + 1] ^ 0x02;
			++i;
		} else {
			output_data[oindex++] = input_data[i];
		}
		++i;
	}
	return oindex;
}

} /* namespace Headset */

#include "qmdebug_domains_start.h"
QMDEBUG_DEFINE_DOMAIN(hstransport, LevelDefault)
#include "qmdebug_domains_end.h"
