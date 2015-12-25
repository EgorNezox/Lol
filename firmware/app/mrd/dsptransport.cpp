/**
 ******************************************************************************
 * @file    dsptransport.cpp
 * @author  Artem Pisarenko, PMR dept. software team, ONIIP, PJSC
 * @date    21.12.2015
 *
 * TODO: реализовать CRC в Qm Core (шаблон)
 ******************************************************************************
 */

#include "qmdebug.h"
#include "qmendian.h"
#include "qmuart.h"

#include "dsptransport.h"

#define MAX_FRAME_SIZE	255

#define FRAME_START_DELIMITER	0x02
#define FRAME_END_DELIMITER		0x03

namespace Multiradio {

const int DspTransport::MAX_FRAME_DATA_LEN = (MAX_FRAME_SIZE - 2/*маркеры*/ - 1/*размер кадра*/ - 1/*адрес назначения*/ - 2/*контрольная сумма*/);

class CRC16arc
{
public:
	CRC16arc() : value((uint16_t)precalc_init_reflected)
	{
	}
	void update(const unsigned char *data, unsigned long size) {
		while (size--)
			value = precalc_table[(value ^ *data++) & 0xFFL] ^ (value >> 8);
	}
	uint16_t result() {
		return value;
	}
private:
	static void __attribute__((constructor)) initPrecalc() {
		precalc_init_reflected = reflect(0x0000, 16);
		for (uint32_t i = 0; i < 256; i++) {
			uint32_t value = reflect(i, 8) << 8;
			for (int j = 0; j < 8; j++)
				if (value & 0x8000)
					value = (value << 1) ^ 0x8005;
				else
					value <<= 1;
			value = reflect(value, 16);
			precalc_table[i] = value & 0xFFFF;
		}
	}
	static uint32_t reflect(uint32_t value, int bitscount) {
		uint32_t t = value;
		for (int i = 0; i < bitscount; i++) {
			if (t & 1L)
				value |= (1L << ((bitscount-1)-i));
			else
				value &= ~(1L << ((bitscount-1)-i));
			t >>= 1;
		}
		return value;
	}

	uint16_t value;
	static uint32_t precalc_init_reflected;
	static uint32_t precalc_table[];
};
uint32_t CRC16arc::precalc_init_reflected;
uint32_t CRC16arc::precalc_table[256];

DspTransport::DspTransport(int uart_resource, int max_tx_queue_size, QmObject *parent) :
	QmObject(parent),
	rx_state(rxstateNoSync), rx_frame_expected_size(-1), rx_frame_size(-1)
{
	rx_frame_buf = new uint8_t[MAX_FRAME_SIZE];
	QmUart::ConfigStruct uart_config;
	uart_config.baud_rate = 57600;
	uart_config.stop_bits = QmUart::StopBits_1;
	uart_config.parity = QmUart::Parity_None;
	uart_config.flow_control = QmUart::FlowControl_None;
	uart_config.rx_buffer_size = 512;
	uart_config.tx_buffer_size = max_tx_queue_size*(MAX_FRAME_SIZE + 2/*маркеры*/);
	uart_config.io_pending_interval = 10;
	uart = new QmUart(uart_resource, &uart_config, this);
	uart->dataReceived.connect(sigc::mem_fun(this, &DspTransport::processUartReceivedData));
	uart->rxError.connect(sigc::mem_fun(this, &DspTransport::processUartReceivedErrors));
	uart->open();
}

DspTransport::~DspTransport()
{
	uart->close();
	delete[] rx_frame_buf;
}

void DspTransport::flush() {
	uart->close();
	uart->open();
}

void DspTransport::transmitFrame(uint8_t address, uint8_t* data, int data_len) {
	QM_ASSERT(data_len <= MAX_FRAME_DATA_LEN);
	uint8_t frame_header[3];
	CRC16arc crc;
	uint8_t frame_footer[3];
	qmToBigEndian((uint8_t)FRAME_START_DELIMITER, frame_header+0);
	qmToBigEndian((uint8_t)(4 + data_len), frame_header+1);
	qmToBigEndian((uint8_t)address, frame_header+2);
	crc.update(frame_header+1, (sizeof(frame_header) - 1));
	crc.update(data, data_len);
//	qmToBigEndian(crc.result(), frame_footer+0);
	qmToBigEndian((uint16_t)0, frame_footer+0);
	qmToBigEndian((uint8_t)FRAME_END_DELIMITER, frame_footer+2);
	int64_t written = 0;
	written += uart->writeData(frame_header, sizeof(frame_header));
	written += uart->writeData(data, data_len);
	written += uart->writeData(frame_footer, sizeof(frame_footer));
	QM_ASSERT(written == (6 + data_len));
}

void DspTransport::processUartReceivedData() {
	uint8_t byte;
	while (uart->readData(&byte, 1)) {
		switch (rx_state) {
		case rxstateNoSync: {
			if (byte == FRAME_START_DELIMITER) {
				rx_state = rxstateFrame;
				rx_frame_size = 0;
			} else if (byte == FRAME_END_DELIMITER) {
				rx_state = rxstateSync;
			}
			break;
		}
		case rxstateSync: {
			if (byte == FRAME_START_DELIMITER) {
				rx_state = rxstateFrame;
				rx_frame_size = 0;
			} else {
				rx_state = rxstateNoSync;
			}
			break;
		}
		case rxstateFrame: {
			if (rx_frame_size == 0) {
				rx_frame_buf[rx_frame_size++] = byte;
				rx_frame_expected_size = byte;
				if (!((4 <= rx_frame_expected_size) && (rx_frame_expected_size <= MAX_FRAME_SIZE)))
					rx_state = rxstateNoSync;
			} else if (rx_frame_size < rx_frame_expected_size) {
				rx_frame_buf[rx_frame_size++] = byte;
			} else {
				if (byte == FRAME_END_DELIMITER) {
					rx_state = rxstateSync;
					uint8_t rx_address = qmFromBigEndian<uint8_t>(rx_frame_buf+1);
					uint8_t *rx_data = rx_frame_buf + 2;
					int rx_data_len = rx_frame_size - 4;
					uint16_t __attribute__((unused)) crc_value = qmFromBigEndian<uint16_t>(rx_frame_buf+(rx_frame_size - 2));
					CRC16arc crc;
					crc.update(rx_frame_buf, (rx_frame_size - 2));
					if (!((rx_data_len > 0)/* && (crc.result() == crc_value)*/))
						break;
					receivedFrame(rx_address, rx_data, rx_data_len);
				} else {
					rx_state = rxstateNoSync;
				}
			}
			break;
		}
		}
	}
}

void DspTransport::processUartReceivedErrors(bool data_errors, bool overflow) {
	QM_UNUSED(data_errors);
	QM_UNUSED(overflow);
	rx_state = rxstateNoSync;
	uart->readData(0, uart->getRxDataAvailable()); // flush received chunks
}

} /* namespace Multiradio */
