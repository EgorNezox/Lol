/**
  ******************************************************************************
  * @file    boot.cpp
  * @author  Artem Pisarenko, PMR dept. software team, ONIIP, PJSC
  * @date    23.03.2016
  *
  ******************************************************************************
 */

#ifndef PORT__PCSIMULATOR
#include "system.h"
#else
#include "../system/qm-platform/qt5/hardware_emulation.h"
#endif

void boot_enter_bootloader() {
#ifndef PORT__PCSIMULATOR
	stm32f2_enter_bootloader();
#else
	QtHwEmu::show_message("Bootloader activated");
#endif
}
