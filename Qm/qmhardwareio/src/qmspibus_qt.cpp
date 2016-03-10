/**
  ******************************************************************************
  * @file    qmspibus_stm32f2xx.cpp
  * @author  Artem Pisarenko, PMR dept. software team, ONIIP, PJSC
  * @date    11.02.2016
  *
  ******************************************************************************
 */

#include <qglobal.h>
#include <qthread.h>
#include "port_hardwareio/spibus.h"
#include "qmspibus.h"

void QmSPIBus::enable(int hw_resource) {
	Q_ASSERT(SPIBus::getInstance(hw_resource)->thread() == QThread::currentThread());
	SPIBus::getInstance(hw_resource)->enabled = true;
}

void QmSPIBus::disable(int hw_resource) {
	Q_ASSERT(SPIBus::getInstance(hw_resource)->thread() == QThread::currentThread());
	SPIBus::getInstance(hw_resource)->enabled = false;
}
