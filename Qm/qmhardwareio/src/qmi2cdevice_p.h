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
#include "hal_i2c.h"
#include "../../qmcore/src/qm_core.h"
#endif
#ifdef QMHARDWAREIO_PLATFORM_QT
#include <QObject>
#include "port_hardwareio/i2cdeviceinterface.h"
#endif /* QMHARDWAREIO_PLATFORM_QT */

#ifdef QMHARDWAREIO_PLATFORM_STM32F2XX
class QmI2CDeviceIOEvent : public QmSystemEvent
{
public:
	QmI2CDeviceIOEvent(QmI2CDevice *o);
private:
	QmI2CDevice *o;
	void process();
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
	I2CDeviceInterface *interface;
public Q_SLOTS:

Q_SIGNALS:

};
#endif /* QMHARDWAREIO_PLATFORM_QT */

class QmI2CDevicePrivate : public QmObjectPrivate {
	QM_DECLARE_PUBLIC(QmI2CDevice)
public:
	QmI2CDevicePrivate(QmI2CDevice *q);
	virtual ~QmI2CDevicePrivate();
private:
	void init();
	void deinit();
	int hw_resource;
#ifdef QMHARDWAREIO_PLATFORM_STM32F2XX
	void processEventHardwareIO();
	hal_i2c_device_t i2c_device_descriptor;
	hal_i2c_master_transfer_t i2c_transfer;
	QmI2CDeviceIOEvent io_event;
#endif
#ifdef QMHARDWAREIO_PLATFORM_QT
	friend class QmI2CDevicePrivateAdapter;
	QmI2CDevicePrivateAdapter *i2cdevice_adapter;
#endif /* QMHARDWAREIO_PLATFORM_QT */
};

#endif /* QMI2CDEVICE_P_H_ */
