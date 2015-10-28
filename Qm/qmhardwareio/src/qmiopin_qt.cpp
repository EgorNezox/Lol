/**
  ******************************************************************************
  * @file    qmiopin_qt.cpp
  * @author  Artem Pisarenko, PMR dept. software team, ONIIP, PJSC
  * @date    28.10.2015
  *
  ******************************************************************************
  */

#include "qmiopin.h"
#include "qmiopin_p.h"

QmIopinPrivate::QmIopinPrivate(QmIopin *q) :
	QmObjectPrivate(q)
{
}

QmIopinPrivate::~QmIopinPrivate()
{
}

bool QmIopin::event(QmEvent* event) {
	return QmObject::event(event);
}
