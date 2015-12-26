/**
  ******************************************************************************
  * @file    qmuart.h
  * @author  Artem Pisarenko, PMR dept. software team, ONIIP, PJSC
  * @date    26.10.2015
  *
  ******************************************************************************
  */

#ifndef QMUART_H_
#define QMUART_H_

#include "qmobject.h"

QM_FORWARD_PRIVATE(QmUart)

/*! The QmUart class provides functions to access UARTs peripherals.
 */
class QmUart: public QmObject {
public:
	enum StopBits {
		StopBits_1,
		StopBits_1_5,
		StopBits_2
	};
	enum Parity {
		Parity_None,
		Parity_Even,
		Parity_Odd
	};
	enum FlowControl {
		FlowControl_None,
		FlowControl_Hardware
	};
	struct ConfigStruct {
		uint32_t baud_rate;
		StopBits stop_bits;
		Parity parity;
		FlowControl flow_control;
		uint32_t rx_buffer_size;		/*!< (минимальный) размер приемного буфера (с учетом baud_rate и io_pending_interval) */
		uint32_t tx_buffer_size;		/*!< размер передающего буфера (с учетом baud_rate и io_pending_interval) */
		uint32_t io_pending_interval;	/*!< мин. интервал (в мс) задержки в обработке событий ввода/вывода (0 - обрабатывать немедленно) */
	};

	/*! Constructs an uart with the given \a parent.
	 *
	 * Parameter \a hw_resource specifies platform-identified instance of peripheral.
	 */
	QmUart(int hw_resource, ConfigStruct *config, QmObject *parent = 0);

	/*! Destroys the uart. */
	virtual ~QmUart();

	bool isOpen();

	bool open();

	bool close();

	int64_t readData(uint8_t *buffer, uint32_t max_size);

	int64_t writeData(const uint8_t *data, uint32_t data_size);

	uint32_t getRxDataAvailable();

	uint32_t getTxSpaceAvailable();

	sigc::signal<void> dataReceived;

	sigc::signal<void, bool/*data_errors*/, bool/*overflow*/> rxError;

	sigc::signal<void> dataTransmitted;

protected:
	virtual bool event(QmEvent *event);

private:
	QM_DECLARE_PRIVATE(QmUart)
	QM_DISABLE_COPY(QmUart)
};

#endif /* QMUART_H_ */
