/**
  ******************************************************************************
  * @file    qmpushbuttonkey_p.h
  * @author  Artem Pisarenko, PMR dept. software team, ONIIP, PJSC
  * @author  Petr Dmitriev
  * @date    28.10.2015
  *
  ******************************************************************************
  */

#ifndef QMPUSHBUTTONKEY_P_H_
#define QMPUSHBUTTONKEY_P_H_

#include "../../qmcore/src/qmobject_p.h"
#include "qmpushbuttonkey.h"

#ifdef QM_PLATFORM_STM32F2XX
#include "hal_gpio.h"
#include "hal_exti.h"
#include "../../qmcore/src/qm_core.h"
#include "FreeRTOS.h"
#include "timers.h"
#endif /* QM_PLATFORM_STM32F2XX */
#ifdef QMKEYSINPUT_PLATFORM_QT
#include <QObject>
#include "port_keysinput/pushbuttonkeyinterface.h"
#endif /* QMKEYSINPUT_PLATFORM_QT */

#ifdef QMKEYSINPUT_PLATFORM_QT
class QmPushButtonKeyPrivate;
class QmPushButtonKeyPrivateAdapter : public QObject
{
    Q_OBJECT
public:
    QmPushButtonKeyPrivateAdapter(QmPushButtonKeyPrivate *qmpushbuttonkeyprivate);
    ~QmPushButtonKeyPrivateAdapter();
    QmPushButtonKeyPrivate *qmpushbuttonkeyprivate;
    PushbuttonkeyInterface *interface;
public Q_SLOTS:
    void processStateChanged();
};
#endif /* QMKEYSINPUT_PLATFORM_QT */

class QmPushButtonKeyPrivate : public QmObjectPrivate {
	QM_DECLARE_PUBLIC(QmPushButtonKey)
public:
	QmPushButtonKeyPrivate(QmPushButtonKey *q);
    virtual ~QmPushButtonKeyPrivate();
#ifdef QM_PLATFORM_STM32F2XX
    void postPbStateChangedEvent();
	void extiEnable();
	xTimerHandle* getDebounceTimer();
#endif /* QM_PLATFORM_STM32F2XX */
private:
	void init();
    void deinit();
    bool isGpioPressed();
	int hw_resource;
	bool updated_state;	
#ifdef QM_PLATFORM_STM32F2XX
    hal_gpio_pin_t gpio_pin;
	int exti_line;
	hal_exti_params_t exti_params;
	hal_exti_handle_t exti_handle;
	xTimerHandle debounce_timer;
    bool event_posting_available;
#endif /* QM_PLATFORM_STM32F2XX */
#ifdef QMKEYSINPUT_PLATFORM_QT
    friend class QmPushButtonKeyPrivateAdapter;
    QmPushButtonKeyPrivateAdapter *pbkey_adapter;
#endif /* QMKEYSINPUT_PLATFORM_QT */
};

#endif /* QMPUSHBUTTONKEY_P_H_ */
