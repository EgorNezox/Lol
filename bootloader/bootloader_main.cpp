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

#if NEW_BOOTLOADER

static const char* saghenTitleStr      = "********* SAZHEN *********\r\n\n\0";
static const char* bootloaderTitleStr  = "     -= Bootloader =-   \r\n\n\n\0";
static const char* usbComputerTitleStr = "    -= USB Computer =-   \r\n\0";
static const char* sysBootTitleStr     = "  -= System Bootloader =-  \r\n\0";

static const char* firmwareStr    = " Main\r\n\n\0";
static const char* usbComputerStr = " USB Computer\r\n\n\0";
static const char* sysBootStr     = " SysBoot\r\n\n\0";

static const char* arrow = " -> ";
static const char* space = "    ";

static int focus = 0;
static int old_focus = 0;

#define USB_COMPUTER 0
#define FIRMWARE 1
#define SYSTEM_BOOT 2

static bool check_arrows_key_pressed(bool isUp, bool isDown, bool isLeft, bool isRight) // up   -1, down 1
{
    if (isUp)
    {
    	QmMatrixKeyboard upBtn(platformhwMatrixKeyboard);
    	return upBtn.isKeyPressed(platformhwKeyUp);
    }
    else if (isDown)
    {
    	QmMatrixKeyboard downBtn(platformhwMatrixKeyboard);
    	return downBtn.isKeyPressed(platformhwKeyDown);
    }
    else if (isLeft)
    {
    	QmMatrixKeyboard leftBtn(platformhwMatrixKeyboard);
    	return leftBtn.isKeyPressed(platformhwKeyLeft);
    }
    else if (isRight)
    {
    	QmMatrixKeyboard rightBtn(platformhwMatrixKeyboard);
    	return rightBtn.isKeyPressed(platformhwKeyRight);
    }
}

static void drawMenuLine(const char* text)
{
	QmConsoleScreen::oprintf(text);
}

static void drawLine(const char* text)
{
	QmConsoleScreen::oprintf(text);
}

static void drawMenu()
{
	const char* lines[3] = {usbComputerStr, firmwareStr, sysBootStr};

	QmConsoleScreen::clearScreen();
	drawLine(saghenTitleStr);
	drawLine(bootloaderTitleStr);

	for (uint8_t i = 0; i <= 2; i++)
	{
		if (i == focus)
			drawLine(arrow);
		else
			drawLine(space);

		drawLine(lines[i]);
	}
}

static void drawUsbHostScreen()
{
    QmConsoleScreen::clearScreen();
    drawLine(saghenTitleStr);
    drawLine("\n\n\n\n\n\0");
	QmConsoleScreen::oprintf(usbComputerTitleStr);
}

static void drawSystemBootScreen()
{
    QmConsoleScreen::clearScreen();
    drawLine(saghenTitleStr);
    drawLine("\n\n\n\n\n\0");
	QmConsoleScreen::oprintf(sysBootTitleStr);
}

static void drawFirmwareErrorScreen(uint8_t firmwareNum)
{
    QmConsoleScreen::clearScreen();
    drawLine(saghenTitleStr);
    drawLine("\n\n\n\n\n\0");
    switch (firmwareNum)
    {
    	case 0: QmConsoleScreen::oprintf("\t\t\t\t\t\t Usb Computer\r\n\r\n\t\t\t\t\is invalid/missing "); break;
    	case 1: QmConsoleScreen::oprintf("\t\t\t\t\t\t\t\t\t\t  Main\r\n\r\n\t\t\t\t\is invalid/missing "); break;
    }
}

void qmMain()
{
	old_focus = FIRMWARE;
	focus = FIRMWARE;

	QmConsoleScreen::init(2, 2, 2, 2);
	drawMenu();

	hwboot_test_result_t test_result = hwboot_test_board();

	switch (test_result)
	{
		case hwboottestOk:
		{
			QmConsoleScreen::oprintf("\n Base hardware test: OK\r\n");
			break;
		}
		case hwboottestErrorExtSram:
		{
			QmConsoleScreen::oprintf("   Ext. mem test FAILED   ");
			QmConsoleScreen::oprintf("  ! System terminated !");
			while(1); // cannot safely continue !
			break;
		}
		case hwboottestErrorHseClock:
		{
			QmConsoleScreen::oprintf("  Ref. clock test FAILED   ");
	      	QmConsoleScreen::oprintf("  ! System terminated !");
			while(1); // cannot safely continue !
			break;
		}
	}

	if (check_arrows_key_pressed(true, false, false, false))
		focus = USB_COMPUTER;
	if (check_arrows_key_pressed(false, true, false, false))
		focus = SYSTEM_BOOT;

	if (focus != old_focus)
		drawMenu();

	switch (focus)
	{
		case FIRMWARE:
		{
			if (!hwboot_check_firmware())
			{
				drawFirmwareErrorScreen(1);
				while(1);
			}
			hwboot_jump_firmware();
			break;
		}
		case USB_COMPUTER:
		{
			if (!hwboot_check_usbcdc())
			{
				drawFirmwareErrorScreen(2);
				while(1);
			}
			drawUsbHostScreen();
			hwboot_jump_cdc();
			break;
		}
		case SYSTEM_BOOT:
		{
			drawSystemBootScreen();
			hwboot_jump_system_bootloader();
			break;
		}
	}
}



#else

static bool check_bootloader_condition() {
	QmMatrixKeyboard k(platformhwMatrixKeyboard);
	return (k.isKeyPressed(platformhwKeyEnter) && k.isKeyPressed(platformhwKeyBack));
}

static bool check_usbflasher_condition() {
	QmMatrixKeyboard k(platformhwMatrixKeyboard);
	return (k.isKeyPressed(platformhwKeyEnter) && k.isKeyPressed(platformhwKeyLeft));
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

	if (check_usbflasher_condition()) {
		QmConsoleScreen::oprintf("*** USB Flasher\r\n");
		hwboot_jump_usbflasher();
		/* (выполнение никогда не доходит до этого места) */
	}

	if (!hwboot_check_firmware()) {
		QmConsoleScreen::oprintf("Invalid/missing firmware\r\n");
		while(1);
	}

	QmConsoleScreen::oprintf("\r\n*** Starting firmware...\r\n");
	hwboot_jump_firmware(); // управление передается прошивке
	/* (выполнение никогда не доходит до этого места) */
}

#endif


