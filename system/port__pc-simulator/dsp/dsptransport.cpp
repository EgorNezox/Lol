/**
 ******************************************************************************
 * @file    dsptransport.cpp
 * @author  Artem Pisarenko, PMR dept. software team, ONIIP, PJSC
 * @date    23.12.2015
 *
 ******************************************************************************
 */

#include <qendian.h>
#include "qmcrc.h"
#include "port_hardwareio/uartinterface.h"
#include "dsptransport.h"

#define MAX_FRAME_SIZE	255

#define FRAME_START_DELIMITER	0x02
#define FRAME_END_DELIMITER		0x03

typedef QmCrc<uint16_t, 16, 0x8005, 0x0000, true, 0x0000> CRC16arc;

namespace QtHwEmu {

const int DspTransport::MAX_FRAME_DATA_LEN = (MAX_FRAME_SIZE - 2/*маркеры*/ - 1/*размер кадра*/ - 1/*адрес назначения*/ - 2/*контрольная сумма*/);

DspTransport::DspTransport(int uart_resource, QObject *parent) :
	QObject(parent),
	tx_state(txstateNoSync), tx_frame_expected_size(-1), tx_frame_size(-1)
{
	tx_frame_buf = new uint8_t[MAX_FRAME_SIZE];
    uart = UartInterface::createInstance(uart_resource);
    QObject::connect(uart, &UartInterface::txTransferred, this, &DspTransport::processTxData);
}

DspTransport::~DspTransport()
{
	UartInterface::destroyInstance(uart);
	delete[] tx_frame_buf;
}

void DspTransport::reset() {
	tx_state = txstateNoSync;
}

void DspTransport::processTxData(const QByteArray& data) {
	for (int i = 0; i < data.size(); i++) {
		uint8_t byte = data.at(i);
		switch (tx_state) {
		case txstateNoSync: {
			if (byte == FRAME_START_DELIMITER) {
				tx_state = txstateFrame;
				tx_frame_size = 0;
			} else if (byte == FRAME_END_DELIMITER) {
				tx_state = txstateSync;
			}
			break;
		}
		case txstateSync: {
			if (byte == FRAME_START_DELIMITER) {
				tx_state = txstateFrame;
				tx_frame_size = 0;
			} else {
				tx_state = txstateNoSync;
			}
			break;
		}
		case txstateFrame: {
			if (tx_frame_size == 0) {
				tx_frame_buf[tx_frame_size++] = byte;
				tx_frame_expected_size = byte;
				if (!((4 <= tx_frame_expected_size) && (tx_frame_expected_size <= MAX_FRAME_SIZE)))
					tx_state = txstateNoSync;
			} else if (tx_frame_size < tx_frame_expected_size) {
				tx_frame_buf[tx_frame_size++] = byte;
			} else {
				if (byte == FRAME_END_DELIMITER) {
					tx_state = txstateSync;
					uint8_t rx_address = qFromBigEndian<quint8>(tx_frame_buf+1);
					uint8_t *rx_data = tx_frame_buf + 2;
					int rx_data_len = tx_frame_size - 4;
					uint16_t __attribute__((unused)) crc_value = qFromBigEndian<quint16>(tx_frame_buf+(tx_frame_size - 2));
					CRC16arc crc;
					crc.update(tx_frame_buf, (tx_frame_size - 2));
					if (!((rx_data_len > 0)/* && (crc.result() == crc_value)*/))
						break;
					transferedTxFrame(rx_address, rx_data, rx_data_len);
				} else {
					tx_state = txstateNoSync;
				}
			}
			break;
		}
		}
	}
}

void DspTransport::transferRxFrame(uint8_t address, uint8_t* data, int data_len) {
	Q_ASSERT(data_len <= MAX_FRAME_DATA_LEN);
	uint8_t frame_header[3];
	CRC16arc crc;
	uint8_t frame_footer[3];
	qToBigEndian((quint8)FRAME_START_DELIMITER, frame_header+0);
	qToBigEndian((quint8)(4 + data_len), frame_header+1);
	qToBigEndian((quint8)address, frame_header+2);
	crc.update(frame_header+1, (sizeof(frame_header) - 1));
	crc.update(data, data_len);
//	qToBigEndian(crc.result(), frame_footer+0);
	qToBigEndian((uint16_t)0, frame_footer+0);
	qToBigEndian((quint8)FRAME_END_DELIMITER, frame_footer+2);
	QByteArray frame_with_markers;
	frame_with_markers.append((char *)frame_header, sizeof(frame_header));
	frame_with_markers.append((char *)data, data_len);
	frame_with_markers.append((char *)frame_footer, sizeof(frame_footer));
	uart->transferRx(frame_with_markers);
}

} /* namespace QtHwEmu */
