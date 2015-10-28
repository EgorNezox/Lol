/**
  ******************************************************************************
  * @file    qmiopin.cpp
  * @author  Artem Pisarenko, PMR dept. software team, ONIIP, PJSC
  * @date    28.10.2015
  *
  * Dummy
  * TODO: implement QmIopin class
  *
  ******************************************************************************
  */

#include "qmiopin.h"
#include "qmiopin_p.h"

QmIopin::QmIopin(int hw_resource, QmObject* parent) :
	QmObject(*new QmIopinPrivate(this), parent)
{
	QM_UNUSED(hw_resource);
}

QmIopin::~QmIopin() {
}

QmIopin::Direction QmIopin::getDirection() {
	return Direction_Invalid;
}

bool QmIopin::setInputTriggerMode(LevelTriggerMode mode) {
	QM_UNUSED(mode);
	return false;
}

QmIopin::Level QmIopin::readInput() {
	return Level_Low;
}

void QmIopin::writeOutput(Level level) {
	QM_UNUSED(level);
}
