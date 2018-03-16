/**
  ******************************************************************************
  * @file    qmrtc_p.h
  * @author  Petr Dmitriev
  * @date    23.11.2016
  *
  ******************************************************************************
 */

#ifndef QM_QMUSB_SRC_QMUSB_P_H_
#define QM_QMUSB_SRC_QMUSB_P_H_

#include "../../qmcore/src/qmobject_p.h"
#include "qmusb.h"

#ifdef QM_PLATFORM_STM32F2XX
#include "hal_exti.h"
#include "hal_rtc.h"
#include "../../qmcore/src/qm_core.h"
#endif /* QM_PLATFORM_STM32F2XX */
#ifdef QM_PLATFORM_QT
#include <qobject.h>
#include "port_rtc/rtcinterface.h"
#endif /* QM_PLATFORM_QT */

#ifdef QM_PLATFORM_STM32F2XX
class QmUsbWakeupEvent : public QmSystemEvent
{
public:
	QmUsbWakeupEvent(QmUsb *o);
private:
	QmUsb *o;
	void process();
};
#endif /* QM_PLATFORM_STM32F2XX */
#endif /* QM_PLATFORM_STM32F2XX */
#ifdef QM_PLATFORM_QT
class QmUsbPrivate;
class QmUsbPrivateAdapter : public QObject
{
    Q_OBJECT
public:
    QmUsbPrivateAdapter(QmRtcPrivate *qmrtcprivate);
    ~QmUsbPrivateAdapter();
    QmUsbPrivate *qmrtcprivate;
    UsbInterface *interface;
public Q_SLOTS:
    void processWakeup();
Q_SIGNALS:
//	void writeOutputLevel(RtcInterface::Level level);
};
#endif /* QM_PLATFORM_QT */

class QmUsbPrivate : public QmObjectPrivate {
	QM_DECLARE_PUBLIC(QmUsb)
public:
	QmUsbPrivate(QmUsb *q);
	virtual ~QmUsbPrivate();
private:
	void init();
	void deinit();
	int hw_resource;
//#ifdef QM_PLATFORM_STM32F2XX
	int exti_line;
	hal_exti_handle_t exti_handle;
	QmUsbWakeupEvent usb_wakeup_event;
//#endif /* QM_PLATFORM_STM32F2XX */
#ifdef QM_PLATFORM_QT
    friend class QmUsbPrivateAdapter;
    QmUsbPrivateAdapter *rtc_adapter;
    QTime *time;
#endif /* QM_PLATFORM_QT */
};

/* QM_QMRTC_SRC_QMRTC_P_H_ */
