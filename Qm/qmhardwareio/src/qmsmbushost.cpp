/**
  ******************************************************************************
  * @file    qmsmbushost.cpp
  * @author  Artem Pisarenko, PMR dept. software team, ONIIP, PJSC
  * @date    08.12.2015
  *
  ******************************************************************************
  */

#include "qmsmbushost_p.h"

QmSMBusHost::QmSMBusHost(int hw_resource, QmObject* parent) :
	QmObject(*new QmSMBusHostPrivate(this), parent)
{
	QM_D(QmSMBusHost);
	d->hw_resource = hw_resource;
	d->init();
}

QmSMBusHost::~QmSMBusHost() {
	QM_D(QmSMBusHost);
	d->deinit();
}
