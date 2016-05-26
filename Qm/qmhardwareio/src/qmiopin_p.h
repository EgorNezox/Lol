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

#include "../../qmcore/src/qmobject_p.h"
#include "qmiopin.h"

#ifdef QM_PLATFORM_STM32F2XX
#include "hal_gpio.h"
#include "hal_exti.h"
#include "../../qmcore/src/qm_core.h"
#endif /* QM_PLATFORM_STM32F2XX */
#ifdef QM_PLATFORM_QT
#include <qobject.h>
#include "port_hardwareio/iopininterface.h"
#endif /* QM_PLATFORM_QT */

#ifdef QM_PLATFORM_STM32F2XX
class QmIoPinTriggerEvent : public QmSystemEvent
{
public:
	QmIoPinTriggerEvent(QmIopin *o);
private:
	QmIopin *o;
	void process();
public:
	bool active, overflow, limit_once;
};
#endif /* QM_PLATFORM_STM32F2XX */
#ifdef QM_PLATFORM_QT
class QmIopinPrivate;
class QmIopinPrivateAdapter : public QObject
{
	Q_OBJECT
public:
	QmIopinPrivateAdapter(QmIopinPrivate *qmiopinprivate);
	~QmIopinPrivateAdapter();
	static QmIopin::Level convertInputLevelToQm(IopinInterface::Level value);
	QmIopinPrivate *qmiopinprivate;
	IopinInterface *interface;
public Q_SLOTS:
	void processInputLevelAssigned(IopinInterface::Level level, bool overflow_trigger);
Q_SIGNALS:
	void writeOutputLevel(IopinInterface::Level level);
};
#endif /* QM_PLATFORM_QT */

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
#ifdef QM_PLATFORM_STM32F2XX
	hal_gpio_pin_t gpio_pin;
	int exti_line;
	hal_exti_handle_t exti_handle;
	QmIoPinTriggerEvent trigger_event;
#endif /* QM_PLATFORM_STM32F2XX */
#ifdef QM_PLATFORM_QT
	friend class QmIopinPrivateAdapter;
	QmIopinPrivateAdapter *iopin_adapter;
	QmIopin::TriggerProcessingType input_trigger_type;
	QmIopin::Level input_level;
#endif /* QM_PLATFORM_QT */
};

#endif /* QMIOPIN_P_H_ */
