/**
  ******************************************************************************
  * @file    qmspibus.h
  * @author  Artem Pisarenko, PMR dept. software team, ONIIP, PJSC
  * @date    11.02.2016
  *
  ******************************************************************************
  */

#ifndef QMSPIBUS_H_
#define QMSPIBUS_H_

/*! The QmSPIBus class provides functions to control SPI bus operation.
 */
class QmSPIBus {
public:
	static void enable(int hw_resource);
	static void disable(int hw_resource);
};

#endif /* QMSPIBUS_H_ */
