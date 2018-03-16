/**
  ******************************************************************************
  * @file    qmrtc.cpp
  * @author  Petr Dmitriev
  * @date    23.11.2016
  *
  ******************************************************************************
 */

#include "qmusb_p.h"

QmUsb::QmUsb(int hw_resource, QmObject* parent) :
	QmObject(*new QmUsbPrivate(this), parent)
{
	QM_D(QmUsb);
	d->hw_resource = hw_resource;
	d->init();
}

QmUsb::~QmUsb() {
	QM_D(QmUsb);
	d->deinit();
}
