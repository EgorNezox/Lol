/**
  ******************************************************************************
  * @file    qmtimestamp_qt.cpp
  * @author  Artem Pisarenko, PMR dept. software team, ONIIP, PJSC
  * @date    27.05.2016
  *
  ******************************************************************************
  */

#include "qmdebug.h"

#include "qmtimestamp.h"
#include "qmtimestamp_p.h"

QmTimestamp::QmTimestamp() :
	impl(new QmTimestampPrivate())
{
	QM_ASSERT(QElapsedTimer::isMonotonic());
}

QmTimestamp::~QmTimestamp() {
	delete impl;
}

void QmTimestamp::set() {
	impl->reftimer.start();
}

void QmTimestamp::invalidate() {
	impl->reftimer.invalidate();
}
