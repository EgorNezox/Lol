/**
  ******************************************************************************
  * @file    qmrtc_stm32f2xx.cpp
  * @author  Petr Dmitriev
  * @date    23.11.2016
  *
  ******************************************************************************
 */

#include "system.h"
#include "system_hw_io.h"

#define QMDEBUGDOMAIN	QmUsb
#include "qmdebug.h"
#include "qmusb_p.h"
#include "qmevent.h"
#include "qmapplication.h"
#include "hal_usb.h"
#include <cstring>

#include "../usb_cdc.h"

#include "usb_cdc/Inc/usb_device.h"
#include "usb_cdc/Inc/usbd_cdc_if.h"

static void qmusbExtiTriggerIsrCallback(hal_exti_handle_t handle, signed portBASE_TYPE *pxHigherPriorityTaskWoken)
{
	/* генерация события в общий пул событий для передачи в dsp controller */
	QmUsbWakeupEvent *system_event = static_cast<QmUsbWakeupEvent *>(hal_exti_get_userid(handle));
	system_event->setPendingFromISR(pxHigherPriorityTaskWoken);
}

static void *id;

/* получаем приемнный буфер и программно генерируем прерывание для того, чтобы потом запустить событие */
 int8_t CDC_Receive_FS(uint8_t* Buf, uint32_t *Len)
{
  USBD_CDC_SetRxBuffer(&hUsbDeviceFS, &Buf[0]);
  USBD_CDC_ReceivePacket(&hUsbDeviceFS);

 // скопировать буффер

  __HAL_GPIO_EXTI_GENERATE_SWIT(EXTI_SWIER_SWIER18);

  return (USBD_OK);
}

QmUsbWakeupEvent::QmUsbWakeupEvent(QmUsb *o) :
	o(o)
{
}

void QmUsbWakeupEvent::process() {
	QmApplication::postEvent(o, new QmEvent(QmEvent::UsbWakeUp));
}


QmUsbPrivate::QmUsbPrivate(QmUsb *q) :
	QmObjectPrivate(q),
	hw_resource(-1), exti_line(-1), exti_handle(0),
	usb_wakeup_event(q)
{
	id  = (void *)q;
}

QmUsbPrivate::~QmUsbPrivate()
{
}

void QmUsbPrivate::init() {
	hal_exti_params_t exti_params;
	exti_line = stm32f2_get_exti_line(hw_resource);
	exti_params.mode = hextiMode_Rising_Falling;
	exti_params.isrcallbackTrigger = qmusbExtiTriggerIsrCallback;
	exti_params.userid = static_cast<void *>(&usb_wakeup_event);

	exti_handle = hal_exti_open(exti_line, &exti_params);
}

void QmUsbPrivate::deinit() {
	hal_exti_close(exti_handle);
}


bool QmUsb::event(QmEvent* event) {
	if (event->type() == QmEvent::UsbWakeUp) {
		qmDebugMessage(QmDebug::Dump, " usb wakeup event");
		usbwakeup();
		return true;
	}
	return QmObject::event(event);
}

uint8_t* QmUsb::getbuffer()
{
    return buffer;
}

#include "qmdebug_domains_start.h"
QMDEBUG_DEFINE_DOMAIN(QmUsb, LevelDefault)
#include "qmdebug_domains_end.h"
