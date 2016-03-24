/**
  ******************************************************************************
  * @file    bootloader_main.cpp
  * @author  Artem Pisarenko, PMR dept. software team, ONIIP, PJSC
  * @date    19.05.2016
  * @brief   Загрузочный модуль приложения загрузчика.
  *
  ******************************************************************************
 */

#include "../system/platform_hw_map.h"
#include "system/hardware_boot.h"

void qmMain() {
	hwboot_test_result_t test_result = hwboot_test_board();
	if (test_result != hwboottestOk)
		while(1); // test failed, cannot safely continue !
	hwboot_jump_firmware(); // управление передается прошивке
	/* (выполнение никогда не доходит до этого места) */
}
