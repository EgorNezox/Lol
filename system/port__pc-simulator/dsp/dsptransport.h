/**
 ******************************************************************************
 * @file    dsptransport.h
 * @author  Artem Pisarenko, PMR dept. software team, ONIIP, PJSC
 * @date    23.12.2015
 *
 ******************************************************************************
 */

#ifndef DSP_DSPTRANSPORT_H_
#define DSP_DSPTRANSPORT_H_

#include <qobject.h>

class UartInterface;

namespace QtHwEmu {

class DspTransport : public QObject
{
	Q_OBJECT

public:
	DspTransport(int uart_resource, QObject *parent = 0);
	~DspTransport();

	static const int MAX_FRAME_DATA_LEN;

public Q_SLOTS:
	void reset();
	void transferRxFrame(uint8_t address, uint8_t *data, int data_size);

Q_SIGNALS:
	void transferedTxFrame(uint8_t address, uint8_t *data, int data_size);

private:
	void processTxData(const QByteArray &data);

	UartInterface *uart;
	enum {
		txstateNoSync,
		txstateSync,
		txstateFrame
	} tx_state;
	uint8_t *tx_frame_buf;
	int tx_frame_expected_size, tx_frame_size;
};

} /* namespace QtHwEmu */

#endif /* DSP_DSPTRANSPORT_H_ */
