/**
 ******************************************************************************
 * @file    uartinterface.h
 * @author  Artem Pisarenko, PMR dept. software team, ONIIP, PJSC
 * @date    13.11.2015
 *
 * TODO: implement baud rate emulation
 *
 ******************************************************************************
 */

#ifndef UARTINTERFACE_H_
#define UARTINTERFACE_H_

#include <QObject>
#include <QByteArray>
#include <QMutex>

struct QmUart_ConfigStruct;

class UartInterface: public QObject {
	Q_OBJECT

public:
	struct RxErrorsDescriptor {
		bool data;
		bool overflow;
	};

	static UartInterface* getInstance(int hw_resource);
	static UartInterface* createInstance(int hw_resource);
	static void destroyInstance(UartInterface *instance);

public Q_SLOTS:
	void transferRx(const QByteArray &data);
	void injectRxDataError();

Q_SIGNALS:
	void txTransferred(const QByteArray &data);

private:
	friend class QmUartPrivateAdapter;

	UartInterface();
	virtual ~UartInterface();

	QMutex mutex;
	struct QmUart_ConfigStruct *config;
	QByteArray rx_buffer;
	bool rx_data_error, rx_overflow;

#ifndef Q_MOC_RUN
private:
#else
Q_SIGNALS:
#endif
	void rxStatusUpdated();
	void txStatusUpdated();
private Q_SLOTS:
	void setConfig(struct QmUart_ConfigStruct *config);
	quint32 getRxDataAvailable();
	QByteArray popRxData(quint32 size);
	bool popRxErrors(RxErrorsDescriptor &descriptor);
	quint32 getTxSpaceAvailable();
	void pushTxData(const QByteArray &data);
	void flushTx();
};

#endif /* UARTINTERFACE_H_ */
