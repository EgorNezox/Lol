/**
  ******************************************************************************
  * @file    qmspibus_stm32f2xx.cpp
  * @author  Artem Pisarenko, PMR dept. software team, ONIIP, PJSC
  * @date    11.02.2016
  *
  ******************************************************************************
 */

#include "system_hw_io.h"

#include "qmspibus.h"

void QmSPIBus::enable(int hw_resource) {
	stm32f2_get_spi_bus_instance(hw_resource);
	stm32f2_ext_pins_init(hw_resource);
}

void QmSPIBus::disable(int hw_resource) {
	stm32f2_get_spi_bus_instance(hw_resource);
	stm32f2_ext_pins_deinit(hw_resource);
}
