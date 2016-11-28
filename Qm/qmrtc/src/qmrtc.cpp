/**
  ******************************************************************************
  * @file    qmrtc.cpp
  * @author  Petr Dmitriev
  * @date    23.11.2016
  *
  ******************************************************************************
 */

#include "qmrtc_p.h"

QmRtc::QmRtc(int hw_resource, QmObject* parent) :
	QmObject(*new QmRtcPrivate(this), parent)
{
	QM_D(QmRtc);
	d->hw_resource = hw_resource;
	d->init();
}

QmRtc::~QmRtc() {
	QM_D(QmRtc);
	d->deinit();
}
