/**
  ******************************************************************************
  * @file    qmi2cdevice_p.h
  * @author  Artem Pisarenko, PMR dept. software team, ONIIP, PJSC
  * @date    23.11.2015
  *
  ******************************************************************************
  */

#ifndef QMI2CDEVICE_P_H_
#define QMI2CDEVICE_P_H_

#include "../../qmcore/src/qmobject_p.h"
#include "qmi2cdevice.h"

#ifdef QMHARDWAREIO_PLATFORM_STM32F2XX
#include "FreeRTOS.h"
#include "semphr.h"
#include "hal_i2c.h"
#include "../../qmcore/src/qm_core.h"
#include "qmtimer.h"
#endif
#ifdef QMHARDWAREIO_PLATFORM_QT
#include <QObject>
#include <QByteArray>
#include "port_hardwareio/i2cbus.h"
#endif /* QMHARDWAREIO_PLATFORM_QT */

#ifdef QMHARDWAREIO_PLATFORM_STM32F2XX
class QmI2CDeviceIOEvent : public QmSystemEvent
{
public:
	QmI2CDeviceIOEvent(QmI2CDevice *o);
	~QmI2CDeviceIOEvent();
private:
	QmI2CDevice *o;
	void process();
public:
	bool transfer_completed;
	hal_i2c_transfer_result_t transfer_result;
	SemaphoreHandle_t sync_semaphore;
	void resetTransferState();
	void syncTransferComplete();
};
#endif
#ifdef QMHARDWAREIO_PLATFORM_QT
class QmI2CDevicePrivate;
class QmI2CDevicePrivateAdapter : public QObject
{
	Q_OBJECT
public:
	QmI2CDevicePrivateAdapter(QmI2CDevicePrivate *qmi2cdeviceprivate);
	~QmI2CDevicePrivateAdapter();
	QmI2CDevicePrivate *qmi2cdeviceprivate;
	I2CBus *bus;
public Q_SLOTS:
	void processTransferResponse(uint8_t address, bool ack, bool pec_present, const QByteArray &rx_data);
Q_SIGNALS:
	void transferRequested(uint8_t address, bool use_pec, const QByteArray &tx_data, uint32_t rx_size);
};
#endif /* QMHARDWAREIO_PLATFORM_QT */

class QmI2CDevicePrivate : public QmObjectPrivate {
	QM_DECLARE_PUBLIC(QmI2CDevice)
public:
	QmI2CDevicePrivate(QmI2CDevice *q);
	virtual ~QmI2CDevicePrivate();
private:
	void init(uint8_t address);
	void deinit();
	bool startTransfer();
	int bus_hw_resource;
	int transfer_timeout;
	bool transfer_in_progress;
#ifdef QMHARDWAREIO_PLATFORM_STM32F2XX
	void processEventHardwareIO();
	bool prepareTransfer();
	void abortTransferInProgress();
	hal_i2c_master_transfer_t i2c_transfer;
	int transfer_rx_size;
	QmI2CDeviceIOEvent io_event;
	QmTimer transfer_timer;
#endif
#ifdef QMHARDWAREIO_PLATFORM_QT
	friend class QmI2CDevicePrivateAdapter;
	void finishTransfer(bool ack, bool pec_present, const QByteArray &rx_data);
	QmI2CDevicePrivateAdapter *i2cdevice_adapter;
	uint8_t address;
	bool transfer_uses_pec;
	QByteArray transfer_rx_data;
#endif /* QMHARDWAREIO_PLATFORM_QT */
};

#endif /* QMI2CDEVICE_P_H_ */
