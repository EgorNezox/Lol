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

bool QmTimestampPrivate::isValid() {
	return reftimer.isValid();
}

TimestampValueType QmTimestampPrivate::getValue() {
	return (reftimer.elapsed() - offset);
}

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
	impl->offset = 0;
}

void QmTimestamp::invalidate() {
	impl->reftimer.invalidate();
}

void QmTimestamp::shift(long int msec) {
	impl->offset += msec;
}

QmTimestamp& QmTimestamp::operator =(const QmTimestamp& other) {
	impl->reftimer = other.impl->reftimer;
	impl->offset = other.impl->offset;
	return *this;
}
