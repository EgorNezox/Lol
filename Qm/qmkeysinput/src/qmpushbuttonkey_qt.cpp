/**
  ******************************************************************************
  * @file    qmpushbuttonkey_qt.cpp
  * @author  Artem Pisarenko, PMR dept. software team, ONIIP, PJSC
  * @author  Petr Dmitriev
  * @date    28.10.2015
  *
  ******************************************************************************
  */

#include "qmpushbuttonkey_p.h"

QmPushButtonKeyPrivateAdapter::QmPushButtonKeyPrivateAdapter(QmPushButtonKeyPrivate *qmpushbuttonkeyprivate) :
    qmpushbuttonkeyprivate(qmpushbuttonkeyprivate)
{
    interface = PushbuttonkeyInterface::getInstance(qmpushbuttonkeyprivate->hw_resource);
    QObject::connect(interface, &PushbuttonkeyInterface::stateChanged, this, &QmPushButtonKeyPrivateAdapter::processStateChanged);
}

QmPushButtonKeyPrivateAdapter::~QmPushButtonKeyPrivateAdapter()
{
}

void QmPushButtonKeyPrivateAdapter::processStateChanged() {
    qmpushbuttonkeyprivate->updated_state = !(qmpushbuttonkeyprivate->updated_state);
    QmPushButtonKey * const q = qmpushbuttonkeyprivate->q_func();
    q->stateChanged.emit();
}

QmPushButtonKeyPrivate::QmPushButtonKeyPrivate(QmPushButtonKey *q) :
    QmObjectPrivate(q),
    hw_resource(-1), updated_state(false), pbkey_adapter(0)
{
}

QmPushButtonKeyPrivate::~QmPushButtonKeyPrivate()
{    
}

void QmPushButtonKeyPrivate::init() {
    pbkey_adapter = new QmPushButtonKeyPrivateAdapter(this);
}

void QmPushButtonKeyPrivate::deinit() {
    delete pbkey_adapter;
}

bool QmPushButtonKey::event(QmEvent* event) {
	return QmObject::event(event);
}
