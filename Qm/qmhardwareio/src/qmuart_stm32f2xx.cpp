/**
  ******************************************************************************
  * @file    qmuart_stm32f2xx.cpp
  * @author  Artem Pisarenko, PMR dept. software team, ONIIP, PJSC
  * @date    26.10.2015
  *
  ******************************************************************************
  */

#include "qmuart.h"
#include "qmuart_p.h"
#include "qmevent.h"

QmUartPrivate::QmUartPrivate(QmUart *q) :
	QmObjectPrivate(q)
{
}

QmUartPrivate::~QmUartPrivate()
{
}

bool QmUart::event(QmEvent* event) {
	if (event->type() == QmEvent::None) {
		//TODO: ...
		return true;
	}
	return QmObject::event(event);
}
