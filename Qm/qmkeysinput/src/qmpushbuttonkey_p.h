/**
  ******************************************************************************
  * @file    qmpushbuttonkey_p.h
  * @author  Artem Pisarenko, PMR dept. software team, ONIIP, PJSC
  * @date    28.10.2015
  *
  ******************************************************************************
  */

#ifndef QMPUSHBUTTONKEY_P_H_
#define QMPUSHBUTTONKEY_P_H_

#include "../../qmcore/src/qmobject_p.h"
#include "qmpushbuttonkey.h"

#ifdef QMKEYSINPUT_PLATFORM_STM32F2XX
#include "hal_gpio.h"
#include "hal_exti.h"
#include "../../qmcore/src/qm_core.h"
#include "FreeRTOS.h"
#include "timers.h"
#endif

class QmPushButtonKeyPrivate : public QmObjectPrivate {
	QM_DECLARE_PUBLIC(QmPushButtonKey)
public:
	QmPushButtonKeyPrivate(QmPushButtonKey *q);
	virtual ~QmPushButtonKeyPrivate();
	void postPbStateChangedEvent();
#ifdef QMKEYSINPUT_PLATFORM_STM32F2XX
	void extiEnable();
	xTimerHandle* getDebounceTimer();
#endif
private:
	void init();
	void deinit();
#ifdef QMKEYSINPUT_PLATFORM_STM32F2XX
	bool isGpioPressed();
#endif
	int hw_resource;
	bool updated_state;
	bool event_posting_available;
#ifdef QMKEYSINPUT_PLATFORM_STM32F2XX
	hal_gpio_pin_t gpio_pin;
	int exti_line;
	hal_exti_params_t exti_params;
	hal_exti_handle_t exti_handle;
	xTimerHandle debounce_timer;
#endif
};

#endif /* QMPUSHBUTTONKEY_P_H_ */
