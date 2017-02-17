/**
  ******************************************************************************
  * @file    qmdebug.cpp
  * @author  Artem Pisarenko, PMR dept. software team, ONIIP, PJSC
  * @date    11.09.2015
  *
  ******************************************************************************
  */

#ifdef QM_PLATFORM_QT
#include <qglobal.h>
#include <qdatetime.h>
#endif
#ifdef QM_PLATFORM_STM32F2XX
#include "FreeRTOS.h"
#include "task.h"
#endif
#include <stdio.h>
#include <stdarg.h>

#include "qmmutex.h"
#include "qmdebug.h"

static QmMutex qmdebug_mutex;

void QmDebug::message(const char * domain_name, msg_type_t type, const char * format, ...) {
	va_list args;
	va_start(args, format);
	const char *type_str;

	switch (type) {
	case Warning: type_str = "Warn "; break;
	case Error: type_str = "ERROR"; break;
	case Info: type_str = "info "; break;
	case Dump: type_str = "dump "; break;
	default: type_str = "???"; break;
	}

	qmdebug_mutex.lock();

#ifdef QM_PLATFORM_QT
	{
		QTime t = QTime::currentTime();
		char *buffer = 0;
		int buffer_len = 0;
		do {
			buffer_len = vsnprintf(buffer, buffer_len, format, args) + 1;
		} while ((buffer == 0) && (buffer = new char[buffer_len]));
		qDebug("[%02u:%02u:%02u.%03u, %s] %s: %s", t.hour(), t.minute(), t.second(), t.msec(), type_str, domain_name, buffer);
		delete []buffer;
	}
#endif /* QM_PLATFORM_QT */
#ifdef QM_PLATFORM_STM32F2XX
	printf("[%010lut, %s] %s: ", xTaskGetTickCount(), type_str, domain_name);
	vprintf(format, args);
	putchar('\n');
#endif /* QM_PLATFORM_STM32F2XX */

	qmdebug_mutex.unlock();

	va_end(args);
}

unsigned long QmDebug::getTicks()
{
	//qmdebug_mutex.lock();

#ifdef QM_PLATFORM_STM32F2XX
	return	xTaskGetTickCount();
#endif

	//qmdebug_mutex.unlock();
}

#include "qmdebug_domains_start.h"
QMDEBUG_DEFINE_DOMAIN(QmCore, LevelDefault)
#include "qmdebug_domains_end.h"
