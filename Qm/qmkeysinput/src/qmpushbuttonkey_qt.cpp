/**
  ******************************************************************************
  * @file    qmpushbuttonkey_qt.cpp
  * @author  Artem Pisarenko, PMR dept. software team, ONIIP, PJSC
  * @date    28.10.2015
  *
  ******************************************************************************
  */

#include "qmpushbuttonkey.h"
#include "qmpushbuttonkey_p.h"

QmPushButtonKeyPrivate::QmPushButtonKeyPrivate(QmPushButtonKey *q) :
	QmObjectPrivate(q)
{
}

QmPushButtonKeyPrivate::~QmPushButtonKeyPrivate()
{
}

bool QmPushButtonKey::event(QmEvent* event) {
	return QmObject::event(event);
}
