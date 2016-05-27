/**
  ******************************************************************************
  * @file    qmtimestamp_p.h
  * @author  Artem Pisarenko, PMR dept. software team, ONIIP, PJSC
  * @date    27.05.2016
  *
  ******************************************************************************
  */

#ifndef QMTIMESTAMP_P_H_
#define QMTIMESTAMP_P_H_

#ifdef QM_PLATFORM_STM32F2XX
#include "FreeRTOS.h"
#endif /* QM_PLATFORM_STM32F2XX */

#ifdef QM_PLATFORM_QT
#include <qelapsedtimer.h>
#endif /* QM_PLATFORM_QT */

class QmTimestampPrivate {
public:
#ifdef QM_PLATFORM_STM32F2XX
	bool valid;
	TickType_t value;
#endif /* QM_PLATFORM_STM32F2XX */
#ifdef QM_PLATFORM_QT
	QElapsedTimer reftimer;
#endif /* QM_PLATFORM_QT */
};

#endif /* QMTIMESTAMP_P_H_ */
