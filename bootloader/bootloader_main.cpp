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

#include "qmconsolescreen.h"
#include "qmmatrixkeyboard.h"

static bool check_bootloader_condition() {
	QmMatrixKeyboard k(platformhwMatrixKeyboard);
	return (k.isKeyPressed(platformhwKeyEnter) && k.isKeyPressed(platformhwKeyBack));
}

void qmMain() {
	QmConsoleScreen::init(2, 2, 2, 2);

	QmConsoleScreen::oprintf("*** SazhenN bootloader ***\r\n");
#ifndef NDEBUG
	QmConsoleScreen::oprintf("DEBUG BUILD\r\n");
#endif
	QmConsoleScreen::oprintf("Press Enter+Back keys to start system bootloader\r\n\r\n");

	if (check_bootloader_condition()) {
		QmConsoleScreen::oprintf("*** SYSTEM BOOTLOADER\r\n");
		hwboot_jump_system_bootloader();
		/* (выполнение никогда не доходит до этого места) */
	}

	hwboot_test_result_t test_result = hwboot_test_board();
	switch (test_result) {
	case hwboottestOk:
		QmConsoleScreen::oprintf("Base hardware test: OK\r\n");
		break;
	case hwboottestErrorExtSram:
		QmConsoleScreen::oprintf("*** Ext. mem test FAILED, system terminated !!!\r\n");
		while(1); // cannot safely continue !
		break;
	case hwboottestErrorHseClock:
		QmConsoleScreen::oprintf("*** Ref. clock test FAILED, system terminated !!!\r\n");
		while(1); // cannot safely continue !
		break;
	}

	if (!hwboot_check_firmware()) {
		QmConsoleScreen::oprintf("Invalid/missing firmware\r\n");
		while(1);
	}

	QmConsoleScreen::oprintf("\r\n*** Starting firmware...\r\n");
	hwboot_jump_firmware(); // управление передается прошивке
	/* (выполнение никогда не доходит до этого места) */
}
