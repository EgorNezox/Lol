/**
  ******************************************************************************
  * @file    qmuart_p.h
  * @author  Artem Pisarenko, PMR dept. software team, ONIIP, PJSC
  * @date    26.10.2015
  *
  ******************************************************************************
  */

#ifndef QMUART_P_H_
#define QMUART_P_H_

#include "../../qmcore/src/qmobject_p.h"
#include "qmuart.h"

#ifdef QMHARDWAREIO_PLATFORM_STM32F2XX
#include "hal_uart.h"
#include "../../qmcore/src/qm_core.h"
#endif
#ifdef QMHARDWAREIO_PLATFORM_QT
#include <QObject>
#include "port_hardwareio/uartinterface.h"
#endif /* QMHARDWAREIO_PLATFORM_QT */

#ifdef QMHARDWAREIO_PLATFORM_STM32F2XX
class QmUartIOEvent : public QmSystemEvent
{
public:
	QmUartIOEvent(QmUart *o);
private:
	QmUart *o;
	void process();
public:
	bool rx_data_pending, rx_data_errors, rx_overflow_suspended;
	bool tx_completed;
};
#endif
#ifdef QMHARDWAREIO_PLATFORM_QT
class QmUartPrivate;
class QmUartPrivateAdapter : public QObject
{
	Q_OBJECT
public:
	QmUartPrivateAdapter(QmUartPrivate *qmuartprivate);
	~QmUartPrivateAdapter();
	QmUartPrivate *qmuartprivate;
	UartInterface *interface;
public Q_SLOTS:
	void processRxStatus();
	void processTxStatus();
	quint32 getInterfaceRxDataAvailable();
	quint32 getInterfaceTxSpaceAvailable();
	QByteArray popInterfaceRxData(quint32 size);
	void flushInterfaceTx();
Q_SIGNALS:
	void writeTx(const QByteArray &data);
};
#endif /* QMHARDWAREIO_PLATFORM_QT */

class QmUartPrivate : public QmObjectPrivate {
	QM_DECLARE_PUBLIC(QmUart)
public:
	QmUartPrivate(QmUart *q);
	virtual ~QmUartPrivate();
private:
	void init();
	void deinit();
	int hw_resource;
	QmUart::ConfigStruct config;
#ifdef QMHARDWAREIO_PLATFORM_STM32F2XX
	void processEventHardwareIO();
	int uart_instance;
	hal_uart_handle_t uart_handle;
	hal_ringbuffer_t *uart_rx_buffer, *uart_tx_buffer;
	bool rx_active;
	QmUartIOEvent io_event;
#endif
#ifdef QMHARDWAREIO_PLATFORM_QT
	friend class QmUartPrivateAdapter;
	bool active;
	QmUartPrivateAdapter *uart_adapter;
#endif /* QMHARDWAREIO_PLATFORM_QT */
};

#endif /* QMUART_P_H_ */
