/**
  ******************************************************************************
  * @file    qmsmbushost_qt.cpp
  * @author  Artem Pisarenko, PMR dept. software team, ONIIP, PJSC
  * @date    08.12.2015
  *
  ******************************************************************************
  */

#include "qmsmbushost_p.h"

QmSMBusHostPrivateAdapter::QmSMBusHostPrivateAdapter(QmSMBusHostPrivate *qmsmbushostprivate) :
	qmsmbushostprivate(qmsmbushostprivate)
{
	interface = SMBusHostInterface::getInstance(qmsmbushostprivate->hw_resource);
}

QmSMBusHostPrivateAdapter::~QmSMBusHostPrivateAdapter()
{
}

QmSMBusHostPrivate::QmSMBusHostPrivate(QmSMBusHost *q) :
	QmObjectPrivate(q),
	hw_resource(-1), smbushost_adapter(0)
{
}

QmSMBusHostPrivate::~QmSMBusHostPrivate()
{
}

void QmSMBusHostPrivate::init() {
	smbushost_adapter = new QmSMBusHostPrivateAdapter(this);
}

void QmSMBusHostPrivate::deinit()
{
	delete smbushost_adapter;
}

bool QmSMBusHost::event(QmEvent* event) {
	return QmObject::event(event);
}
