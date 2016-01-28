/**
 ******************************************************************************
 * @file    smarttransport.h
 * @author  Petr Dmitriev
 * @date    28.01.2016
 *
 ******************************************************************************
 */

#ifndef FIRMWARE_APP_HEADSET_SMARTTRANSPORT_H_
#define FIRMWARE_APP_HEADSET_SMARTTRANSPORT_H_

#include "qmobject.h"

class QmUart;

namespace Headset {

class SmartTransport : public QmObject
{
public:
	SmartTransport(int uart_resource, int max_tx_queue_size, QmObject *parent);
	~SmartTransport();
	void enable();
	void disable();
	void transmitFrame(uint8_t cmd, uint8_t *data, int data_len);

	sigc::signal<void, uint8_t/*cmd*/, uint8_t*/*data*/, int/*data_len*/> receivedFrame;

	static const int MAX_FRAME_DATA_SIZE;

private:
	void processUartReceivedData();
	void processUartReceivedErrors(bool data_errors, bool overflow);
	void dropRxSync();

	enum {
		rxstateNone,
		rxstateFrame
	} rx_state;
	QmUart *uart;
	uint8_t *rx_frame_buf;
	int rx_frame_size;
};

} /* namespace Headset */

#endif /* FIRMWARE_APP_HEADSET_SMARTTRANSPORT_H_ */
