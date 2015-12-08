/**
  ******************************************************************************
  * @file    qmsmbushost_p.h
  * @author  Artem Pisarenko, PMR dept. software team, ONIIP, PJSC
  * @date    08.12.2015
  *
  ******************************************************************************
  */

#ifndef QMSMBUSHOST_P_H_
#define QMSMBUSHOST_P_H_

#include "../../qmcore/src/qmobject_p.h"
#include "qmsmbushost.h"

#ifdef QMHARDWAREIO_PLATFORM_STM32F2XX
#include "hal_i2c.h"
#include "../../qmcore/src/qm_core.h"
#endif
#ifdef QMHARDWAREIO_PLATFORM_QT
#include <QObject>
#include "port_hardwareio/smbushostinterface.h"
#endif /* QMHARDWAREIO_PLATFORM_QT */

#ifdef QMHARDWAREIO_PLATFORM_STM32F2XX
class QmSMBusHostIOEvent : public QmSystemEvent
{
public:
	QmSMBusHostIOEvent(QmSMBusHost *o);
private:
	QmSMBusHost *o;
	void process();
};
#endif
#ifdef QMHARDWAREIO_PLATFORM_QT
class QmSMBusHostPrivate;
class QmSMBusHostPrivateAdapter : public QObject
{
	Q_OBJECT
public:
	QmSMBusHostPrivateAdapter(QmSMBusHostPrivate *qmsmbhostprivate);
	~QmSMBusHostPrivateAdapter();
	QmSMBusHostPrivate *qmsmbushostprivate;
	SMBusHostInterface *interface;
public Q_SLOTS:

Q_SIGNALS:

};
#endif /* QMHARDWAREIO_PLATFORM_QT */

class QmSMBusHostPrivate : public QmObjectPrivate {
	QM_DECLARE_PUBLIC(QmSMBusHost)
public:
	QmSMBusHostPrivate(QmSMBusHost *q);
	virtual ~QmSMBusHostPrivate();
private:
	void init();
	void deinit();
	int hw_resource;
#ifdef QMHARDWAREIO_PLATFORM_STM32F2XX
	void processEventHardwareIO();
	int i2c_bus_instance;
	hal_i2c_smbus_handle_t i2c_smbus_handle;
	QmSMBusHostIOEvent io_event;
#endif
#ifdef QMHARDWAREIO_PLATFORM_QT
	friend class QmSMBusHostPrivateAdapter;
	QmSMBusHostPrivateAdapter *smbushost_adapter;
#endif /* QMHARDWAREIO_PLATFORM_QT */
};

#endif /* QMSMBUSHOST_P_H_ */
