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
#include "qmendian.h"
#include "qmcrc.h"
#include "qmuart.h"

#include "smarttransport.h"


#define MAX_FRAME_PAYLOAD_SIZE		(208 + 1/*cmd*/ + 2/*crc*/)
#define MAX_FRAME_CORRECTED_SIZE	(MAX_FRAME_PAYLOAD_SIZE * 2/*ce*/)
#define MAX_FRAME_TOTAL_SIZE		(MAX_FRAME_CORRECTED_SIZE + 1/*bof*/ + 1/*eof*/)

#define FRAME_START_DELIMITER	0xC0
#define FRAME_END_DELIMITER		0xC1
#define FRAME_SPEC_SYMBOL		0x7D

typedef QmCrc<uint16_t, 16, 0x1189, 0xFFFF, false, 0x0000> CRC16arc;

namespace Headset {

/*
 * CRC16 calculating algorithm from smart headset manufacturer (concern "Granit")
 * (incapsulated into custom class)
 **/
class GranitCRC
{
public:
	GranitCRC() : value(0xFFFF) {}
	void update(const unsigned char *data, unsigned short size) {
		value = crc16_calc(value, data, size);
	}
	uint16_t result() {
		return value;
	}
private:
	unsigned short crc16_calc(unsigned short crc16_start, const unsigned char *data, unsigned short len) {
		while(len--)
			crc16_start = (crc16_start >> 8) ^ crc16_tab[(crc16_start ^ *data++) & 0xFF];
		return crc16_start;
	}

	static const unsigned short crc16_tab[256];
	uint16_t value;
};

const unsigned short GranitCRC::crc16_tab[256] = {
		0x0000, 0x1189, 0x2312, 0x329b, 0x4624, 0x57ad, 0x6536, 0x74bf,
		0x8c48, 0x9dc1, 0xaf5a, 0xbed3, 0xca6c, 0xdbe5, 0xe97e, 0xf8f7,
		0x1081, 0x0108, 0x3393, 0x221a, 0x56a5, 0x472c, 0x75b7, 0x643e,
		0x9cc9, 0x8d40, 0xbfdb, 0xae52, 0xdaed, 0xcb64, 0xf9ff, 0xe876,
		0x2102, 0x308b, 0x0210, 0x1399, 0x6726, 0x76af, 0x4434, 0x55bd,
		0xad4a, 0xbcc3, 0x8e58, 0x9fd1, 0xeb6e, 0xfae7, 0xc87c, 0xd9f5,
		0x3183, 0x200a, 0x1291, 0x0318, 0x77a7, 0x662e, 0x54b5, 0x453c,
		0xbdcb, 0xac42, 0x9ed9, 0x8f50, 0xfbef, 0xea66, 0xd8fd, 0xc974,
		0x4204, 0x538d, 0x6116, 0x709f, 0x0420, 0x15a9, 0x2732, 0x36bb,
		0xce4c, 0xdfc5, 0xed5e, 0xfcd7, 0x8868, 0x99e1, 0xab7a, 0xbaf3,
		0x5285, 0x430c, 0x7197, 0x601e, 0x14a1, 0x0528, 0x37b3, 0x263a,
		0xdecd, 0xcf44, 0xfddf, 0xec56, 0x98e9, 0x8960, 0xbbfb, 0xaa72,
		0x6306, 0x728f, 0x4014, 0x519d, 0x2522, 0x34ab, 0x0630, 0x17b9,
		0xef4e, 0xfec7, 0xcc5c, 0xddd5, 0xa96a, 0xb8e3, 0x8a78, 0x9bf1,
		0x7387, 0x620e, 0x5095, 0x411c, 0x35a3, 0x242a, 0x16b1, 0x0738,
		0xffcf, 0xee46, 0xdcdd, 0xcd54, 0xb9eb, 0xa862, 0x9af9, 0x8b70,
		0x8408, 0x9581, 0xa71a, 0xb693, 0xc22c, 0xd3a5, 0xe13e, 0xf0b7,
		0x0840, 0x19c9, 0x2b52, 0x3adb, 0x4e64, 0x5fed, 0x6d76, 0x7cff,
		0x9489, 0x8500, 0xb79b, 0xa612, 0xd2ad, 0xc324, 0xf1bf, 0xe036,
		0x18c1, 0x0948, 0x3bd3, 0x2a5a, 0x5ee5, 0x4f6c, 0x7df7, 0x6c7e,
		0xa50a, 0xb483, 0x8618, 0x9791, 0xe32e, 0xf2a7, 0xc03c, 0xd1b5,
		0x2942, 0x38cb, 0x0a50, 0x1bd9, 0x6f66, 0x7eef, 0x4c74, 0x5dfd,
		0xb58b, 0xa402, 0x9699, 0x8710, 0xf3af, 0xe226, 0xd0bd, 0xc134,
		0x39c3, 0x284a, 0x1ad1, 0x0b58, 0x7fe7, 0x6e6e, 0x5cf5, 0x4d7c,
		0xc60c, 0xd785, 0xe51e, 0xf497, 0x8028, 0x91a1, 0xa33a, 0xb2b3,
		0x4a44, 0x5bcd, 0x6956, 0x78df, 0x0c60, 0x1de9, 0x2f72, 0x3efb,
		0xd68d, 0xc704, 0xf59f, 0xe416, 0x90a9, 0x8120, 0xb3bb, 0xa232,
		0x5ac5, 0x4b4c, 0x79d7, 0x685e, 0x1ce1, 0x0d68, 0x3ff3, 0x2e7a,
		0xe70e, 0xf687, 0xc41c, 0xd595, 0xa12a, 0xb0a3, 0x8238, 0x93b1,
		0x6b46, 0x7acf, 0x4854, 0x59dd, 0x2d62, 0x3ceb, 0x0e70, 0x1ff9,
		0xf78f, 0xe606, 0xd49d, 0xc514, 0xb1ab, 0xa022, 0x92b9, 0x8330,
		0x7bc7, 0x6a4e, 0x58d5, 0x495c, 0x3de3, 0x2c6a, 0x1ef1, 0x0f78
};

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

void SmartTransport::transmitFrame(uint8_t cmd, uint8_t *data, int data_len) {
	QM_ASSERT(data_len <= MAX_FRAME_DATA_SIZE);
	GranitCRC crc;
	qmDebugMessage(QmDebug::Info, "transmitting frame (cmd=0x%02X, data_len=%d)", cmd, data_len);
	crc.update(&cmd, 1);
	crc.update(data, data_len);
	uint8_t frame_crc[2];
	qmToBigEndian(crc.result(), frame_crc);
	uint8_t frame_start = FRAME_START_DELIMITER;
	uint8_t frame_stop = FRAME_END_DELIMITER;

	uint8_t frame[MAX_FRAME_TOTAL_SIZE];
	int frame_len = 0;
	frame_len += bytestuff(&cmd, (uint8_t*)(frame + frame_len), 1);
	frame_len += bytestuff(data, (uint8_t*)(frame + frame_len), data_len);
	frame_len += bytestuff(frame_crc, (uint8_t*)(frame + frame_len), sizeof(crc.result()));
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
				qmDebugMessage(QmDebug::Dump, "uart rx: - ignoring out-of-sync byte 0x%02X", byte);
			}
			break;
		}
		case rxstateFrame: {
			if (rx_frame_size < MAX_FRAME_TOTAL_SIZE) {
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

int SmartTransport::bytestuff(uint8_t* input_data, uint8_t* output_data, int data_len) {
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

} /* namespace Headset */
