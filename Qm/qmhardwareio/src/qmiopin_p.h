/**
  ******************************************************************************
  * @file    qmiopin_p.h
  * @author  Artem Pisarenko, PMR dept. software team, ONIIP, PJSC
  * @date    28.10.2015
  *
  ******************************************************************************
  */

#ifndef QMIOPIN_P_H_
#define QMIOPIN_P_H_

#include "qm.h"
#include "../../qmcore/src/qmobject_p.h"

#ifdef QMHARDWAREIO_PLATFORM_STM32F2XX
#include "hal_gpio.h"
#include "hal_exti.h"
#include "../../qmcore/src/qm_core.h"
#endif
#ifdef QMHARDWAREIO_PLATFORM_QT
#include <QObject>
#include "port_hardwareio/iopininterface.h"
#endif /* QMHARDWAREIO_PLATFORM_QT */

#ifdef QMHARDWAREIO_PLATFORM_STM32F2XX
class QmIoPinTriggerEvent : public QmSystemEvent
{
public:
	QmIoPinTriggerEvent(QmIopin *o);
private:
	QmIopin *o;
	void process();
};
#endif
#ifdef QMHARDWAREIO_PLATFORM_QT
class QmIopinPrivateAdapter : public QObject
{
public:
	QmIopinPrivateAdapter(QmIopinPrivate *qmiopinprivate);
	~QmIopinPrivateAdapter();
	QmIopinPrivate *qmiopinprivate;
	IopinInterface *interface;
public Q_SLOTS:
	void assignOutputLevel(QmIopin::Level level);
	void processInputLevelAssigned(IopinInterface::Level level);
};
#endif /* QMHARDWAREIO_PLATFORM_QT */

class QmIopinPrivate : public QmObjectPrivate {
	QM_DECLARE_PUBLIC(QmIopin)
public:
	QmIopinPrivate(QmIopin *q);
	virtual ~QmIopinPrivate();
private:
	void init();
	void deinit();
	int hw_resource;
	QmIopin::LevelTriggerMode input_trigger_mode;
#ifdef QMHARDWAREIO_PLATFORM_STM32F2XX
	hal_gpio_pin_t gpio_pin;
	int exti_line;
	hal_exti_handle_t exti_handle;
	QmIoPinTriggerEvent trigger_event;
#endif
#ifdef QMHARDWAREIO_PLATFORM_QT
	friend class QmIopinPrivateAdapter;
	QmIopinPrivateAdapter *iopin_adapter;
	QmIopin::Level input_level;
#endif /* QMHARDWAREIO_PLATFORM_QT */
};

#endif /* QMIOPIN_P_H_ */
