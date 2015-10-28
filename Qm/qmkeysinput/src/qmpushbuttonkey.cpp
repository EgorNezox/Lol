/**
  ******************************************************************************
  * @file    qmpushbuttonkey.cpp
  * @author  Artem Pisarenko, PMR dept. software team, ONIIP, PJSC
  * @date    28.10.2015
  *
  * Dummy
  * TODO: implement QmPushButtonKey class
  *
  ******************************************************************************
  */

#include "qmpushbuttonkey.h"
#include "qmpushbuttonkey_p.h"

QmPushButtonKey::QmPushButtonKey(int hw_resource, QmObject* parent) :
	QmObject(*new QmPushButtonKeyPrivate(this), parent)
{
    QM_UNUSED(hw_resource);
}

QmPushButtonKey::~QmPushButtonKey() {
}

bool QmPushButtonKey::isPressed() {
	return false;
}
