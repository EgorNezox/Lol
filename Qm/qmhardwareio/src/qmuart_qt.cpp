/**
  ******************************************************************************
  * @file    qmuart_qt.cpp
  * @author  Artem Pisarenko, PMR dept. software team, ONIIP, PJSC
  * @date    26.10.2015
  *
  ******************************************************************************
  */

#include "qmuart.h"
#include "qmuart_p.h"

QmUartPrivate::QmUartPrivate(QmUart *q) :
	QmObjectPrivate(q)
{
}

QmUartPrivate::~QmUartPrivate()
{
}

bool QmUart::event(QmEvent* event) {
	return QmObject::event(event);
}
