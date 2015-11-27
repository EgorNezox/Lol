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
	QM_D(QmPushButtonKey);
	d->hw_resource = hw_resource;
	d->init();
}

QmPushButtonKey::~QmPushButtonKey() {
	QM_D(QmPushButtonKey);
	d->deinit();
}

bool QmPushButtonKey::isPressed() {
	QM_D(QmPushButtonKey);
	return d->updated_state;
}
