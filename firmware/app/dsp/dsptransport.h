/**
 ******************************************************************************
 * @file    dsptransport.h
 * @author  Artem Pisarenko, PMR dept. software team, ONIIP, PJSC
 * @date    21.12.2015
 *
 ******************************************************************************
 */

#ifndef FIRMWARE_APP_DSP_DSPTRANSPORT_H_
#define FIRMWARE_APP_DSP_DSPTRANSPORT_H_

#include "qmobject.h"

class QmUart;

namespace Multiradio {

class DspTransport : public QmObject
{
public:
	DspTransport(int uart_resource, int max_tx_queue_size, QmObject *parent);
	~DspTransport();
	void enable();
	void disable();
	void transmitFrame(uint8_t address, uint8_t *data, int data_len);

	sigc::signal<void, uint8_t/*address*/, uint8_t*/*data*/, int/*data_len*/> receivedFrame;

	static const int MAX_FRAME_DATA_SIZE;

private:
	void processUartReceivedData();
	void processUartReceivedErrors(bool data_errors, bool overflow);
	void dropRxSync();

	QmUart *uart;
	enum {
		rxstateNoSync,
		rxstateSync,
		rxstateFrame
	} rx_state;
	uint8_t *rx_frame_buf;
	int rx_frame_expected_size, rx_frame_size;
};

} /* namespace Multiradio */

#endif /* FIRMWARE_APP_DSP_DSPTRANSPORT_H_ */
