/**
  ******************************************************************************
  * @file    qmpushbuttonkey_stm32f2xx.cpp
  * @author  Artem Pisarenko, PMR dept. software team, ONIIP, PJSC
  * @date    28.10.2015
  *
  ******************************************************************************
  */

#include "qmpushbuttonkey.h"
#include "qmpushbuttonkey_p.h"
#include "qmevent.h"

QmPushButtonKeyPrivate::QmPushButtonKeyPrivate(QmPushButtonKey *q) :
	QmObjectPrivate(q)
{
}

QmPushButtonKeyPrivate::~QmPushButtonKeyPrivate()
{
}

bool QmPushButtonKey::event(QmEvent* event) {
	if (event->type() == QmEvent::None) {
		//TODO: ...
		return true;
	}
	return QmObject::event(event);
}
