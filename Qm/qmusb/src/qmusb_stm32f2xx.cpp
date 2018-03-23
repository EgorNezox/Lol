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

static uint8_t buffer[255];
static bool dtr;
static bool rts;
static uint16_t len;

/* получаем приемнный буфер и программно генерируем прерывание для того, чтобы потом запустить событие */
 int8_t CDC_Receive_FS(uint8_t* Buf, uint32_t *Len)
{
  USBD_CDC_SetRxBuffer(&hUsbDeviceFS, &Buf[0]);
  USBD_CDC_ReceivePacket(&hUsbDeviceFS);

  len = *Len;
 // скопировать буффер
  strncpy((char*)buffer,(char*)Buf,*Len);
  buffer[*Len]=0;

  __HAL_GPIO_EXTI_GENERATE_SWIT(EXTI_SWIER_SWIER18);

  return (USBD_OK);
}


 /**
   * @brief  Manage the CDC class requests
   * @param  cmd: Command code
   * @param  pbuf: Buffer containing command data (request parameters)
   * @param  length: Number of data to be sent (in bytes)
   * @retval Result of the operation: USBD_OK if all operations are OK else USBD_FAIL
   */


 /* Сигнал DTR и RST пердаются при запросе 0x22 в 3 байте
  * На нулевой позиции лежит DTR, на первой RST
  * https://electronix.ru/forum/lofiversion/index.php/t144877.html */
 int8_t CDC_Control_FS(uint8_t cmd, uint8_t* pbuf, uint16_t length)
 {
   /* USER CODE BEGIN 5 */
   switch(cmd)
   {
     case CDC_SEND_ENCAPSULATED_COMMAND:

     break;

     case CDC_GET_ENCAPSULATED_RESPONSE:

     break;

     case CDC_SET_COMM_FEATURE:

     break;

     case CDC_GET_COMM_FEATURE:

     break;

     case CDC_CLEAR_COMM_FEATURE:

     break;

   /*******************************************************************************/
   /* Line Coding Structure                                                       */
   /*-----------------------------------------------------------------------------*/
   /* Offset | Field       | Size | Value  | Description                          */
   /* 0      | dwDTERate   |   4  | Number |Data terminal rate, in bits per second*/
   /* 4      | bCharFormat |   1  | Number | Stop bits                            */
   /*                                        0 - 1 Stop bit                       */
   /*                                        1 - 1.5 Stop bits                    */
   /*                                        2 - 2 Stop bits                      */
   /* 5      | bParityType |  1   | Number | Parity                               */
   /*                                        0 - None                             */
   /*                                        1 - Odd                              */
   /*                                        2 - Even                             */
   /*                                        3 - Mark                             */
   /*                                        4 - Space                            */
   /* 6      | bDataBits  |   1   | Number Data bits (5, 6, 7, 8 or 16).          */
   /*******************************************************************************/
     case CDC_SET_LINE_CODING:

     break;

     case CDC_GET_LINE_CODING:

     break;

     case CDC_SET_CONTROL_LINE_STATE:
    	 if ( pbuf[2] & 1) dtr = 1; else dtr = 0;
    	 if ( pbuf[2] & 2) rts = 1; else rts = 0;
     break;

     case CDC_SEND_BREAK:

     break;

   default:
     break;
   }

   return (USBD_OK);
   /* USER CODE END 5 */
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

bool QmUsb::getdtr()
{
	return dtr;
}

bool QmUsb::getrtc()
{
	return rts;
}

uint16_t QmUsb::getLen()
{
	return len;
}

#include "qmdebug_domains_start.h"
QMDEBUG_DEFINE_DOMAIN(QmUsb, LevelDefault)
#include "qmdebug_domains_end.h"
