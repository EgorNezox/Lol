/**
  ******************************************************************************
  * @file    qmapplication.cpp
  * @author  Artem Pisarenko, PMR dept. software team, ONIIP, PJSC
  * @date    11.09.2015
  *
  ******************************************************************************
  */

#include "qmdebug.h"
#include "qmthread.h"
#include "qmapplication.h"
#include "qmapplication_p.h"

QmApplication* QmApplication::self = 0;

QmApplication::QmApplication() :
	QmObject(*new QmApplicationPrivate(this), 0)
{
	QM_ASSERT(QmThread::currentThread());
	QM_ASSERT(self == 0); // only one application object allowed
	self = this;
}

QmApplication::~QmApplication() {
	self = 0;
}
