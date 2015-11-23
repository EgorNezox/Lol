/**
  ******************************************************************************
  * @file    qmuart.cpp
  * @author  Artem Pisarenko, PMR dept. software team, ONIIP, PJSC
  * @date    28.10.2015
  *
  ******************************************************************************
  */

#include "qmuart_p.h"

QmUart::QmUart(int hw_resource, ConfigStruct* config, QmObject* parent) :
	QmObject(*new QmUartPrivate(this), parent)
{
	QM_D(QmUart);
	d->hw_resource = hw_resource;
	d->config = *config;
	d->init();
}

QmUart::~QmUart() {
	QM_D(QmUart);
	d->deinit();
}
