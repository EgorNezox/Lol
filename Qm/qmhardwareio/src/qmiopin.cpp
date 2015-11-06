/**
  ******************************************************************************
  * @file    qmiopin.cpp
  * @author  Artem Pisarenko, PMR dept. software team, ONIIP, PJSC
  * @date    28.10.2015
  *
  ******************************************************************************
  */

#include "qmiopin.h"
#include "qmiopin_p.h"

QmIopin::QmIopin(int hw_resource, QmObject* parent) :
	QmObject(*new QmIopinPrivate(this), parent)
{
	QM_D(QmIopin);
	d->hw_resource = hw_resource;
	d->init();
}

QmIopin::~QmIopin() {
	QM_D(QmIopin);
	d->deinit();
}
