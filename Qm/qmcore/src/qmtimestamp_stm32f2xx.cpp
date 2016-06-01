/**
  ******************************************************************************
  * @file    qmtimestamp_stm32f2xx.cpp
  * @author  Artem Pisarenko, PMR dept. software team, ONIIP, PJSC
  * @date    27.05.2016
  *
  ******************************************************************************
  */

#include "FreeRTOS.h"
#include "task.h"

#include "qmtimestamp.h"
#include "qmtimestamp_p.h"

bool QmTimestampPrivate::isValid() {
	return valid;
}

TimestampValueType QmTimestampPrivate::getValue() {
	return value;
}

QmTimestamp::QmTimestamp() :
	impl(new QmTimestampPrivate())
{
	invalidate();
}

QmTimestamp::~QmTimestamp() {
	delete impl;
}

void QmTimestamp::set() {
	impl->value = xTaskGetTickCount();
	impl->valid = true;
}

void QmTimestamp::invalidate() {
	impl->valid = false;
}

void QmTimestamp::shift(long int msec) {
	impl->value += (TickType_t)(msec/portTICK_PERIOD_MS);
}
