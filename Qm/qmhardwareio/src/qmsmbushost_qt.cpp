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
	bus = I2CBus::getInstance(qmsmbushostprivate->hw_resource);
	QObject::connect(bus, &I2CBus::messageHostNotify, this, &QmSMBusHostPrivateAdapter::processMessageHostNotify);
}

QmSMBusHostPrivateAdapter::~QmSMBusHostPrivateAdapter()
{
}

void QmSMBusHostPrivateAdapter::processMessageHostNotify(uint8_t address, uint16_t status) {
	QmSMBusHost * const q = qmsmbushostprivate->q_func();
	q->messageReceived(address, status);
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
