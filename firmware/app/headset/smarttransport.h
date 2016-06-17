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
	void transmitCmd(uint8_t cmd, uint8_t *data, int data_len);
	void repeatLastCmd();

	sigc::signal<void, uint8_t/*cmd*/, uint8_t*/*data*/, int/*data_len*/> receivedCmd;

	static const int MAX_FRAME_DATA_SIZE;

private:
	void processUartReceivedData();
	void processUartReceivedErrors(bool data_errors, bool overflow);
	void dropRxSync();
	uint16_t calcFrameCRC(uint8_t cmd, uint8_t *data, int data_len);
	uint16_t extractFrameCRC(uint8_t *data, int data_len);
	int encodeFrameData(uint8_t* input_data, uint8_t* output_data, int data_len);
	int decodeFrameData(uint8_t* input_data, uint8_t* output_data, int data_len);

	enum {
		rxstateNone,
		rxstateFrame
	} rx_state;
	QmUart *uart;
	uint8_t *rx_frame_buf;
	int rx_frame_size;

	uint8_t last_cmd;
	uint8_t* last_cmd_data;
	int last_cmd_data_size;
};

} /* namespace Headset */

#endif /* FIRMWARE_APP_HEADSET_SMARTTRANSPORT_H_ */
