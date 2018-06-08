/**
  ******************************************************************************
  * @file    bootloader_main.cpp
  * @author  Artem Pisarenko, PMR dept. software team, ONIIP, PJSC
  * @date    19.05.2016
  * @brief   Р вЂњРЎвЂ™Р вЂ™РІР‚вЂќР вЂњРЎвЂ™Р вЂ™Р’В°Р вЂњРЎвЂ™Р вЂ™РЎвЂ“Р вЂњРІР‚пїЅР вЂ™Р вЂљР вЂњРІР‚пїЅР вЂ™РЎвЂњР вЂњРЎвЂ™Р вЂ™Р’В·Р вЂњРЎвЂ™Р вЂ™РЎвЂўР вЂњРІР‚пїЅР вЂ™РІР‚РЋР вЂњРЎвЂ™Р вЂ™Р вЂ¦Р вЂњРІР‚пїЅР вЂ™РІР‚в„–Р вЂњРЎвЂ™Р вЂ™РІвЂћвЂ“ Р вЂњРЎвЂ™Р вЂ™РЎпїЅР вЂњРЎвЂ™Р вЂ™РЎвЂўР вЂњРЎвЂ™Р вЂ™РўвЂ�Р вЂњРІР‚пїЅР вЂ™РЎвЂњР вЂњРЎвЂ™Р вЂ™Р’В»Р вЂњРІР‚пїЅР вЂ™Р Р‰ Р вЂњРЎвЂ™Р вЂ™РЎвЂ”Р вЂњРІР‚пїЅР вЂ™Р вЂљР вЂњРЎвЂ™Р вЂ™РЎвЂ�Р вЂњРЎвЂ™Р вЂ™Р’В»Р вЂњРЎвЂ™Р вЂ™РЎвЂўР вЂњРЎвЂ™Р вЂ™Р’В¶Р вЂњРЎвЂ™Р вЂ™Р’ВµР вЂњРЎвЂ™Р вЂ™Р вЂ¦Р вЂњРЎвЂ™Р вЂ™РЎвЂ�Р вЂњРІР‚пїЅР вЂ™Р РЏ Р вЂњРЎвЂ™Р вЂ™Р’В·Р вЂњРЎвЂ™Р вЂ™Р’В°Р вЂњРЎвЂ™Р вЂ™РЎвЂ“Р вЂњРІР‚пїЅР вЂ™Р вЂљР вЂњРІР‚пїЅР вЂ™РЎвЂњР вЂњРЎвЂ™Р вЂ™Р’В·Р вЂњРІР‚пїЅР вЂ™РІР‚РЋР вЂњРЎвЂ™Р вЂ™РЎвЂ�Р вЂњРЎвЂ™Р вЂ™РЎвЂќР вЂњРЎвЂ™Р вЂ™Р’В°.
  *
  ******************************************************************************
 */

#include "../system/platform_hw_map.h"
#include "system/hardware_boot.h"

#include "qmconsolescreen.h"
#include "qmmatrixkeyboard.h"

#include <string>


#if NEW_BOOTLOADER

static const char* saghenTitleStr      = " ** ������-� **\0";
static const char* bootloaderTitleStr  = " - ��������� -\0";
static const char* usbComputerTitleStr = "  USB ���������\0";
static const char* sysBootTitleStr     = "  - ��������� -\r\n   - ��������� -\r\n\0";

static const char* spaceStr = "                ";
//static const char* spaceStr = "\r\n\0";

static const char* firmwareStr    = " ��������\r\n\0";
static const char* usbComputerStr = " USB �������.\r\n\0";
static const char* sysBootStr     = " ������.����.\r\n\0";

static const char* arrow = "->";
static const char* space = "  ";

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

static void drawNewLine(const char* s)
{
		QmConsoleScreen::oputstr(s);
}

static void drawChar(char c)
{
	QmConsoleScreen::oputc(c);
}

static void drawMenu()
{
	const char* lines[3] = {usbComputerStr, firmwareStr, sysBootStr};

	QmConsoleScreen::clearScreen();
	drawNewLine(saghenTitleStr);
	drawNewLine(bootloaderTitleStr);
	drawNewLine(spaceStr);

	for (uint8_t i = 0; i <= 2; i++)
	{
		if (i == focus)
			drawNewLine(arrow);
		else
			drawNewLine(space);

		drawNewLine(lines[i]);

	}
}

static void drawUsbHostScreen()
{
    QmConsoleScreen::clearScreen();
    drawLine(saghenTitleStr);
    drawLine("\n\n\n\0");
	QmConsoleScreen::oprintf(usbComputerTitleStr);
}

static void drawSystemBootScreen()
{
    QmConsoleScreen::clearScreen();
    drawLine(saghenTitleStr);
    drawLine("\n\n\n\0");
	QmConsoleScreen::oprintf(sysBootTitleStr);
}

static void drawFirmwareErrorScreen(uint8_t firmwareNum)
{
    QmConsoleScreen::clearScreen();
    drawLine(saghenTitleStr);
    drawLine("\n\n\0");
    switch (firmwareNum)
    {
    	case 0: QmConsoleScreen::oprintf("     Р вЂњР вЂЎР вЂ™РЎвЂ”Р вЂ™Р вЂ¦Р вЂњР вЂЎР вЂ™РЎвЂ”Р вЂ™Р вЂ¦Р вЂњР вЂЎР вЂ™РЎвЂ”Р вЂ™Р вЂ¦Р вЂњР вЂЎР вЂ™РЎвЂ”Р вЂ™Р вЂ¦Р вЂњР вЂЎР вЂ™РЎвЂ”Р вЂ™Р вЂ¦Р вЂњР вЂЎР вЂ™РЎвЂ”Р вЂ™Р вЂ¦Р вЂњР вЂЎР вЂ™РЎвЂ”Р вЂ™Р вЂ¦Р вЂњР вЂЎР вЂ™РЎвЂ”Р вЂ™Р вЂ¦Р вЂњР вЂЎР вЂ™РЎвЂ”Р вЂ™Р вЂ¦\r\n    USB Р вЂњР вЂЎР вЂ™РЎвЂ”Р вЂ™Р вЂ¦Р вЂњР вЂЎР вЂ™РЎвЂ”Р вЂ™Р вЂ¦Р вЂњР вЂЎР вЂ™РЎвЂ”Р вЂ™Р вЂ¦Р вЂњР вЂЎР вЂ™РЎвЂ”Р вЂ™Р вЂ¦Р вЂњР вЂЎР вЂ™РЎвЂ”Р вЂ™Р вЂ¦Р вЂњР вЂЎР вЂ™РЎвЂ”Р вЂ™Р вЂ¦Р вЂњР вЂЎР вЂ™РЎвЂ”Р вЂ™Р вЂ¦Р вЂњР вЂЎР вЂ™РЎвЂ”Р вЂ™Р вЂ¦Р вЂњР вЂЎР вЂ™РЎвЂ”Р вЂ™Р вЂ¦\r\n\r\n\     Р вЂњР вЂЎР вЂ™РЎвЂ”Р вЂ™Р вЂ¦Р вЂњР вЂЎР вЂ™РЎвЂ”Р вЂ™Р вЂ¦Р вЂњР вЂЎР вЂ™РЎвЂ”Р вЂ™Р вЂ¦Р вЂњР вЂЎР вЂ™РЎвЂ”Р вЂ™Р вЂ¦Р вЂњР вЂЎР вЂ™РЎвЂ”Р вЂ™Р вЂ¦Р вЂњР вЂЎР вЂ™РЎвЂ”Р вЂ™Р вЂ¦Р вЂњР вЂЎР вЂ™РЎвЂ”Р вЂ™Р вЂ¦Р вЂњР вЂЎР вЂ™РЎвЂ”Р вЂ™Р вЂ¦Р вЂњР вЂЎР вЂ™РЎвЂ”Р вЂ™Р вЂ¦Р вЂњР вЂЎР вЂ™РЎвЂ”Р вЂ™Р вЂ¦\r\n\        Р вЂњР вЂЎР вЂ™РЎвЂ”Р вЂ™Р вЂ¦Р вЂњР вЂЎР вЂ™РЎвЂ”Р вЂ™Р вЂ¦Р вЂњР вЂЎР вЂ™РЎвЂ”Р вЂ™Р вЂ¦    \r\n\    Р вЂњР вЂЎР вЂ™РЎвЂ”Р вЂ™Р вЂ¦Р вЂњР вЂЎР вЂ™РЎвЂ”Р вЂ™Р вЂ¦Р вЂњР вЂЎР вЂ™РЎвЂ”Р вЂ™Р вЂ¦Р вЂњР вЂЎР вЂ™РЎвЂ”Р вЂ™Р вЂ¦Р вЂњР вЂЎР вЂ™РЎвЂ”Р вЂ™Р вЂ¦Р вЂњР вЂЎР вЂ™РЎвЂ”Р вЂ™Р вЂ¦Р вЂњР вЂЎР вЂ™РЎвЂ”Р вЂ™Р вЂ¦Р вЂњР вЂЎР вЂ™РЎвЂ”Р вЂ™Р вЂ¦Р вЂњР вЂЎР вЂ™РЎвЂ”Р вЂ™Р вЂ¦Р вЂњР вЂЎР вЂ™РЎвЂ”Р вЂ™Р вЂ¦Р вЂњР вЂЎР вЂ™РЎвЂ”Р вЂ™Р вЂ¦ !\0"); break;
    	case 1: QmConsoleScreen::oprintf("     Р вЂњР вЂЎР вЂ™РЎвЂ”Р вЂ™Р вЂ¦Р вЂњР вЂЎР вЂ™РЎвЂ”Р вЂ™Р вЂ¦Р вЂњР вЂЎР вЂ™РЎвЂ”Р вЂ™Р вЂ¦Р вЂњР вЂЎР вЂ™РЎвЂ”Р вЂ™Р вЂ¦Р вЂњР вЂЎР вЂ™РЎвЂ”Р вЂ™Р вЂ¦Р вЂњР вЂЎР вЂ™РЎвЂ”Р вЂ™Р вЂ¦Р вЂњР вЂЎР вЂ™РЎвЂ”Р вЂ™Р вЂ¦Р вЂњР вЂЎР вЂ™РЎвЂ”Р вЂ™Р вЂ¦Р вЂњР вЂЎР вЂ™РЎвЂ”Р вЂ™Р вЂ¦\r\n      Р вЂњР вЂЎР вЂ™РЎвЂ”Р вЂ™Р вЂ¦Р вЂњР вЂЎР вЂ™РЎвЂ”Р вЂ™Р вЂ¦Р вЂњР вЂЎР вЂ™РЎвЂ”Р вЂ™Р вЂ¦Р вЂњР вЂЎР вЂ™РЎвЂ”Р вЂ™Р вЂ¦Р вЂњР вЂЎР вЂ™РЎвЂ”Р вЂ™Р вЂ¦Р вЂњР вЂЎР вЂ™РЎвЂ”Р вЂ™Р вЂ¦Р вЂњР вЂЎР вЂ™РЎвЂ”Р вЂ™Р вЂ¦Р вЂњР вЂЎР вЂ™РЎвЂ”Р вЂ™Р вЂ¦\r\n\r\n\     Р вЂњР вЂЎР вЂ™РЎвЂ”Р вЂ™Р вЂ¦Р вЂњР вЂЎР вЂ™РЎвЂ”Р вЂ™Р вЂ¦Р вЂњР вЂЎР вЂ™РЎвЂ”Р вЂ™Р вЂ¦Р вЂњР вЂЎР вЂ™РЎвЂ”Р вЂ™Р вЂ¦Р вЂњР вЂЎР вЂ™РЎвЂ”Р вЂ™Р вЂ¦Р вЂњР вЂЎР вЂ™РЎвЂ”Р вЂ™Р вЂ¦Р вЂњР вЂЎР вЂ™РЎвЂ”Р вЂ™Р вЂ¦Р вЂњР вЂЎР вЂ™РЎвЂ”Р вЂ™Р вЂ¦Р вЂњР вЂЎР вЂ™РЎвЂ”Р вЂ™Р вЂ¦Р вЂњР вЂЎР вЂ™РЎвЂ”Р вЂ™Р вЂ¦\r\n\        Р вЂњР вЂЎР вЂ™РЎвЂ”Р вЂ™Р вЂ¦Р вЂњР вЂЎР вЂ™РЎвЂ”Р вЂ™Р вЂ¦Р вЂњР вЂЎР вЂ™РЎвЂ”Р вЂ™Р вЂ¦    \r\n\    Р вЂњР вЂЎР вЂ™РЎвЂ”Р вЂ™Р вЂ¦Р вЂњР вЂЎР вЂ™РЎвЂ”Р вЂ™Р вЂ¦Р вЂњР вЂЎР вЂ™РЎвЂ”Р вЂ™Р вЂ¦Р вЂњР вЂЎР вЂ™РЎвЂ”Р вЂ™Р вЂ¦Р вЂњР вЂЎР вЂ™РЎвЂ”Р вЂ™Р вЂ¦Р вЂњР вЂЎР вЂ™РЎвЂ”Р вЂ™Р вЂ¦Р вЂњР вЂЎР вЂ™РЎвЂ”Р вЂ™Р вЂ¦Р вЂњР вЂЎР вЂ™РЎвЂ”Р вЂ™Р вЂ¦Р вЂњР вЂЎР вЂ™РЎвЂ”Р вЂ™Р вЂ¦Р вЂњР вЂЎР вЂ™РЎвЂ”Р вЂ™Р вЂ¦Р вЂњР вЂЎР вЂ™РЎвЂ”Р вЂ™Р вЂ¦ !\0"); break;
    }
}

//static void drawNewLine(std::string str)
//{
//	for (uint8_t i = 0; i < str.size(); i++)
//		QmConsoleScreen::oputc(str[i]);
//}



void qmMain()
{
	old_focus = FIRMWARE;
	focus = FIRMWARE;

	QmConsoleScreen::init(2, 2, 2, 2);
	drawMenu();

	//QmConsoleScreen::clearScreen();

	//drawNewLine("abcdefghjklmnopqrstuvwxyz\0");
	//std::string s("Р°Р±");
	//drawNewLine(s);
	//drawChar('Р°');
	//drawChar('Р±');
//	drawNewLine(symsStr);

//	QmConsoleScreen::oputc('Р В Р’В°');
//	QmConsoleScreen::oputc('Р В Р’В¶');
//	QmConsoleScreen::oputc('Р В Р’Вµ');
//	QmConsoleScreen::oputc('Р В Р вЂ¦');
//	QmConsoleScreen::oputc('Р РЋР Р‰');
//	QmConsoleScreen::oputc('-');
//	QmConsoleScreen::oputc('Р В РЎСљ');

//	drawLine(saghenTitleTestStr);




//	hwboot_test_result_t test_result = hwboot_test_board();
//
//	switch (test_result)
//	{
//		case hwboottestOk:
//		{
//			QmConsoleScreen::oprintf("\r\n Р вЂњР вЂЎР вЂ™РЎвЂ”Р вЂ™Р вЂ¦Р вЂњР вЂЎР вЂ™РЎвЂ”Р вЂ™Р вЂ¦Р вЂњР вЂЎР вЂ™РЎвЂ”Р вЂ™Р вЂ¦Р вЂњР вЂЎР вЂ™РЎвЂ”Р вЂ™Р вЂ¦Р вЂњР вЂЎР вЂ™РЎвЂ”Р вЂ™Р вЂ¦Р вЂњР вЂЎР вЂ™РЎвЂ”Р вЂ™Р вЂ¦Р вЂњР вЂЎР вЂ™РЎвЂ”Р вЂ™Р вЂ¦ Р вЂњР вЂЎР вЂ™РЎвЂ”Р вЂ™Р вЂ¦Р вЂњР вЂЎР вЂ™РЎвЂ”Р вЂ™Р вЂ¦Р вЂњР вЂЎР вЂ™РЎвЂ”Р вЂ™Р вЂ¦Р вЂњР вЂЎР вЂ™РЎвЂ”Р вЂ™Р вЂ¦Р вЂњР вЂЎР вЂ™РЎвЂ”Р вЂ™Р вЂ¦Р вЂњР вЂЎР вЂ™РЎвЂ”Р вЂ™Р вЂ¦Р вЂњР вЂЎР вЂ™РЎвЂ”Р вЂ™Р вЂ¦Р вЂњР вЂЎР вЂ™РЎвЂ”Р вЂ™Р вЂ¦Р вЂњР вЂЎР вЂ™РЎвЂ”Р вЂ™Р вЂ¦Р вЂњР вЂЎР вЂ™РЎвЂ”Р вЂ™Р вЂ¦\r\n    Р вЂњР вЂЎР вЂ™РЎвЂ”Р вЂ™Р вЂ¦Р вЂњР вЂЎР вЂ™РЎвЂ”Р вЂ™Р вЂ¦Р вЂњР вЂЎР вЂ™РЎвЂ”Р вЂ™Р вЂ¦Р вЂњР вЂЎР вЂ™РЎвЂ”Р вЂ™Р вЂ¦ Р вЂњР вЂЎР вЂ™РЎвЂ”Р вЂ™Р вЂ¦Р вЂњР вЂЎР вЂ™РЎвЂ”Р вЂ™Р вЂ¦Р вЂњР вЂЎР вЂ™РЎвЂ”Р вЂ™Р вЂ¦Р вЂњР вЂЎР вЂ™РЎвЂ”Р вЂ™Р вЂ¦Р вЂњР вЂЎР вЂ™РЎвЂ”Р вЂ™Р вЂ¦Р вЂњР вЂЎР вЂ™РЎвЂ”Р вЂ™Р вЂ¦Р вЂњР вЂЎР вЂ™РЎвЂ”Р вЂ™Р вЂ¦\0");
//			break;
//		}
//		case hwboottestErrorExtSram:
//		{
//			QmConsoleScreen::oprintf("\r\nР вЂњР вЂЎР вЂ™РЎвЂ”Р вЂ™Р вЂ¦Р вЂњР вЂЎР вЂ™РЎвЂ”Р вЂ™Р вЂ¦Р вЂњР вЂЎР вЂ™РЎвЂ”Р вЂ™Р вЂ¦Р вЂњР вЂЎР вЂ™РЎвЂ”Р вЂ™Р вЂ¦ Р вЂњР вЂЎР вЂ™РЎвЂ”Р вЂ™Р вЂ¦Р вЂњР вЂЎР вЂ™РЎвЂ”Р вЂ™Р вЂ¦Р вЂњР вЂЎР вЂ™РЎвЂ”Р вЂ™Р вЂ¦Р вЂњР вЂЎР вЂ™РЎвЂ”Р вЂ™Р вЂ¦Р вЂњР вЂЎР вЂ™РЎвЂ”Р вЂ™Р вЂ¦Р вЂњР вЂЎР вЂ™РЎвЂ”Р вЂ™Р вЂ¦Р вЂњР вЂЎР вЂ™РЎвЂ”Р вЂ™Р вЂ¦ Р вЂњР вЂЎР вЂ™РЎвЂ”Р вЂ™Р вЂ¦Р вЂњР вЂЎР вЂ™РЎвЂ”Р вЂ™Р вЂ¦Р вЂњР вЂЎР вЂ™РЎвЂ”Р вЂ™Р вЂ¦Р вЂњР вЂЎР вЂ™РЎвЂ”Р вЂ™Р вЂ¦Р вЂњР вЂЎР вЂ™РЎвЂ”Р вЂ™Р вЂ¦Р вЂњР вЂЎР вЂ™РЎвЂ”Р вЂ™Р вЂ¦\r\n    Р вЂњР вЂЎР вЂ™РЎвЂ”Р вЂ™Р вЂ¦Р вЂњР вЂЎР вЂ™РЎвЂ”Р вЂ™Р вЂ¦ Р вЂњР вЂЎР вЂ™РЎвЂ”Р вЂ™Р вЂ¦Р вЂњР вЂЎР вЂ™РЎвЂ”Р вЂ™Р вЂ¦Р вЂњР вЂЎР вЂ™РЎвЂ”Р вЂ™Р вЂ¦Р вЂњР вЂЎР вЂ™РЎвЂ”Р вЂ™Р вЂ¦Р вЂњР вЂЎР вЂ™РЎвЂ”Р вЂ™Р вЂ¦Р вЂњР вЂЎР вЂ™РЎвЂ”Р вЂ™Р вЂ¦Р вЂњР вЂЎР вЂ™РЎвЂ”Р вЂ™Р вЂ¦ !\0");
//			while(1); // cannot safely continue !
//			break;
//		}
//		case hwboottestErrorHseClock:
//		{
//			QmConsoleScreen::oprintf("\r\n Р вЂњР вЂЎР вЂ™РЎвЂ”Р вЂ™Р вЂ¦Р вЂњР вЂЎР вЂ™РЎвЂ”Р вЂ™Р вЂ¦Р вЂњР вЂЎР вЂ™РЎвЂ”Р вЂ™Р вЂ¦Р вЂњР вЂЎР вЂ™РЎвЂ”Р вЂ™Р вЂ¦ Р вЂњР вЂЎР вЂ™РЎвЂ”Р вЂ™Р вЂ¦Р вЂњР вЂЎР вЂ™РЎвЂ”Р вЂ™Р вЂ¦Р вЂњР вЂЎР вЂ™РЎвЂ”Р вЂ™Р вЂ¦Р вЂњР вЂЎР вЂ™РЎвЂ”Р вЂ™Р вЂ¦Р вЂњР вЂЎР вЂ™РЎвЂ”Р вЂ™Р вЂ¦Р вЂњР вЂЎР вЂ™РЎвЂ”Р вЂ™Р вЂ¦Р вЂњР вЂЎР вЂ™РЎвЂ”Р вЂ™Р вЂ¦Р вЂњР вЂЎР вЂ™РЎвЂ”Р вЂ™Р вЂ¦Р вЂњР вЂЎР вЂ™РЎвЂ”Р вЂ™Р вЂ¦Р вЂњР вЂЎР вЂ™РЎвЂ”Р вЂ™Р вЂ¦Р вЂњР вЂЎР вЂ™РЎвЂ”Р вЂ™Р вЂ¦Р вЂњР вЂЎР вЂ™РЎвЂ”Р вЂ™Р вЂ¦\r\n     Р вЂњР вЂЎР вЂ™РЎвЂ”Р вЂ™Р вЂ¦Р вЂњР вЂЎР вЂ™РЎвЂ”Р вЂ™Р вЂ¦ Р вЂњР вЂЎР вЂ™РЎвЂ”Р вЂ™Р вЂ¦Р вЂњР вЂЎР вЂ™РЎвЂ”Р вЂ™Р вЂ¦Р вЂњР вЂЎР вЂ™РЎвЂ”Р вЂ™Р вЂ¦Р вЂњР вЂЎР вЂ™РЎвЂ”Р вЂ™Р вЂ¦Р вЂњР вЂЎР вЂ™РЎвЂ”Р вЂ™Р вЂ¦Р вЂњР вЂЎР вЂ™РЎвЂ”Р вЂ™Р вЂ¦Р вЂњР вЂЎР вЂ™РЎвЂ”Р вЂ™Р вЂ¦ !\0");
//			while(1); // cannot safely continue !
//			break;
//		}
//	}
//
//	if (check_arrows_key_pressed(true, false, false, false))
//		focus = USB_COMPUTER;
//	if (check_arrows_key_pressed(false, true, false, false))
//		focus = SYSTEM_BOOT;
//
//	if (focus != old_focus)
//		drawMenu();
//
//	switch (focus)
//	{
//		case FIRMWARE:
//		{
//			if (!hwboot_check_firmware())
//			{
//				drawFirmwareErrorScreen(1);
//				while(1);
//			}
//			hwboot_jump_firmware();
//			break;
//		}
//		case USB_COMPUTER:
//		{
//			if (!hwboot_check_usbcdc())
//			{
//				drawFirmwareErrorScreen(0);
//				while(1);
//			}
//			drawUsbHostScreen();
//			hwboot_jump_cdc();
//			break;
//		}
//		case SYSTEM_BOOT:
//		{
//			drawSystemBootScreen();
//			hwboot_jump_system_bootloader();
//			break;
//		}
//	}
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
		/* (Р вЂњРЎвЂ™Р вЂ™Р вЂ Р вЂњРІР‚пїЅР вЂ™РІР‚в„–Р вЂњРЎвЂ™Р вЂ™РЎвЂ”Р вЂњРЎвЂ™Р вЂ™РЎвЂўР вЂњРЎвЂ™Р вЂ™Р’В»Р вЂњРЎвЂ™Р вЂ™Р вЂ¦Р вЂњРЎвЂ™Р вЂ™Р’ВµР вЂњРЎвЂ™Р вЂ™Р вЂ¦Р вЂњРЎвЂ™Р вЂ™РЎвЂ�Р вЂњРЎвЂ™Р вЂ™Р’Вµ Р вЂњРЎвЂ™Р вЂ™Р вЂ¦Р вЂњРЎвЂ™Р вЂ™РЎвЂ�Р вЂњРЎвЂ™Р вЂ™РЎвЂќР вЂњРЎвЂ™Р вЂ™РЎвЂўР вЂњРЎвЂ™Р вЂ™РЎвЂ“Р вЂњРЎвЂ™Р вЂ™РўвЂ�Р вЂњРЎвЂ™Р вЂ™Р’В° Р вЂњРЎвЂ™Р вЂ™Р вЂ¦Р вЂњРЎвЂ™Р вЂ™Р’Вµ Р вЂњРЎвЂ™Р вЂ™РўвЂ�Р вЂњРЎвЂ™Р вЂ™РЎвЂўР вЂњРІР‚пїЅР вЂ™РІР‚В¦Р вЂњРЎвЂ™Р вЂ™РЎвЂўР вЂњРЎвЂ™Р вЂ™РўвЂ�Р вЂњРЎвЂ™Р вЂ™РЎвЂ�Р вЂњРІР‚пїЅР вЂ™РІР‚С™ Р вЂњРЎвЂ™Р вЂ™РўвЂ�Р вЂњРЎвЂ™Р вЂ™РЎвЂў Р вЂњРІР‚пїЅР вЂ™Р РЉР вЂњРІР‚пїЅР вЂ™РІР‚С™Р вЂњРЎвЂ™Р вЂ™РЎвЂўР вЂњРЎвЂ™Р вЂ™РЎвЂ“Р вЂњРЎвЂ™Р вЂ™РЎвЂў Р вЂњРЎвЂ™Р вЂ™РЎпїЅР вЂњРЎвЂ™Р вЂ™Р’ВµР вЂњРІР‚пїЅР вЂ™Р С“Р вЂњРІР‚пїЅР вЂ™РІР‚С™Р вЂњРЎвЂ™Р вЂ™Р’В°) */
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
		/* (Р вЂњРЎвЂ™Р вЂ™Р вЂ Р вЂњРІР‚пїЅР вЂ™РІР‚в„–Р вЂњРЎвЂ™Р вЂ™РЎвЂ”Р вЂњРЎвЂ™Р вЂ™РЎвЂўР вЂњРЎвЂ™Р вЂ™Р’В»Р вЂњРЎвЂ™Р вЂ™Р вЂ¦Р вЂњРЎвЂ™Р вЂ™Р’ВµР вЂњРЎвЂ™Р вЂ™Р вЂ¦Р вЂњРЎвЂ™Р вЂ™РЎвЂ�Р вЂњРЎвЂ™Р вЂ™Р’Вµ Р вЂњРЎвЂ™Р вЂ™Р вЂ¦Р вЂњРЎвЂ™Р вЂ™РЎвЂ�Р вЂњРЎвЂ™Р вЂ™РЎвЂќР вЂњРЎвЂ™Р вЂ™РЎвЂўР вЂњРЎвЂ™Р вЂ™РЎвЂ“Р вЂњРЎвЂ™Р вЂ™РўвЂ�Р вЂњРЎвЂ™Р вЂ™Р’В° Р вЂњРЎвЂ™Р вЂ™Р вЂ¦Р вЂњРЎвЂ™Р вЂ™Р’Вµ Р вЂњРЎвЂ™Р вЂ™РўвЂ�Р вЂњРЎвЂ™Р вЂ™РЎвЂўР вЂњРІР‚пїЅР вЂ™РІР‚В¦Р вЂњРЎвЂ™Р вЂ™РЎвЂўР вЂњРЎвЂ™Р вЂ™РўвЂ�Р вЂњРЎвЂ™Р вЂ™РЎвЂ�Р вЂњРІР‚пїЅР вЂ™РІР‚С™ Р вЂњРЎвЂ™Р вЂ™РўвЂ�Р вЂњРЎвЂ™Р вЂ™РЎвЂў Р вЂњРІР‚пїЅР вЂ™Р РЉР вЂњРІР‚пїЅР вЂ™РІР‚С™Р вЂњРЎвЂ™Р вЂ™РЎвЂўР вЂњРЎвЂ™Р вЂ™РЎвЂ“Р вЂњРЎвЂ™Р вЂ™РЎвЂў Р вЂњРЎвЂ™Р вЂ™РЎпїЅР вЂњРЎвЂ™Р вЂ™Р’ВµР вЂњРІР‚пїЅР вЂ™Р С“Р вЂњРІР‚пїЅР вЂ™РІР‚С™Р вЂњРЎвЂ™Р вЂ™Р’В°) */
	}

	if (!hwboot_check_firmware()) {
		QmConsoleScreen::oprintf("Invalid/missing firmware\r\n");
		while(1);
	}

	QmConsoleScreen::oprintf("\r\n*** Starting firmware...\r\n");
	hwboot_jump_firmware(); // Р вЂњРІР‚пїЅР вЂ™РЎвЂњР вЂњРЎвЂ™Р вЂ™РЎвЂ”Р вЂњРІР‚пїЅР вЂ™Р вЂљР вЂњРЎвЂ™Р вЂ™Р’В°Р вЂњРЎвЂ™Р вЂ™Р вЂ Р вЂњРЎвЂ™Р вЂ™Р’В»Р вЂњРЎвЂ™Р вЂ™Р’ВµР вЂњРЎвЂ™Р вЂ™Р вЂ¦Р вЂњРЎвЂ™Р вЂ™РЎвЂ�Р вЂњРЎвЂ™Р вЂ™Р’Вµ Р вЂњРЎвЂ™Р вЂ™РЎвЂ”Р вЂњРЎвЂ™Р вЂ™Р’ВµР вЂњРІР‚пїЅР вЂ™Р вЂљР вЂњРЎвЂ™Р вЂ™Р’ВµР вЂњРЎвЂ™Р вЂ™РўвЂ�Р вЂњРЎвЂ™Р вЂ™Р’В°Р вЂњРЎвЂ™Р вЂ™Р’ВµР вЂњРІР‚пїЅР вЂ™РІР‚С™Р вЂњРІР‚пїЅР вЂ™Р С“Р вЂњРІР‚пїЅР вЂ™Р РЏ Р вЂњРЎвЂ™Р вЂ™РЎвЂ”Р вЂњРІР‚пїЅР вЂ™Р вЂљР вЂњРЎвЂ™Р вЂ™РЎвЂўР вЂњРІР‚пїЅР вЂ™РІвЂљВ¬Р вЂњРЎвЂ™Р вЂ™РЎвЂ�Р вЂњРЎвЂ™Р вЂ™Р вЂ Р вЂњРЎвЂ™Р вЂ™РЎвЂќР вЂњРЎвЂ™Р вЂ™Р’Вµ
	/* (Р вЂњРЎвЂ™Р вЂ™Р вЂ Р вЂњРІР‚пїЅР вЂ™РІР‚в„–Р вЂњРЎвЂ™Р вЂ™РЎвЂ”Р вЂњРЎвЂ™Р вЂ™РЎвЂўР вЂњРЎвЂ™Р вЂ™Р’В»Р вЂњРЎвЂ™Р вЂ™Р вЂ¦Р вЂњРЎвЂ™Р вЂ™Р’ВµР вЂњРЎвЂ™Р вЂ™Р вЂ¦Р вЂњРЎвЂ™Р вЂ™РЎвЂ�Р вЂњРЎвЂ™Р вЂ™Р’Вµ Р вЂњРЎвЂ™Р вЂ™Р вЂ¦Р вЂњРЎвЂ™Р вЂ™РЎвЂ�Р вЂњРЎвЂ™Р вЂ™РЎвЂќР вЂњРЎвЂ™Р вЂ™РЎвЂўР вЂњРЎвЂ™Р вЂ™РЎвЂ“Р вЂњРЎвЂ™Р вЂ™РўвЂ�Р вЂњРЎвЂ™Р вЂ™Р’В° Р вЂњРЎвЂ™Р вЂ™Р вЂ¦Р вЂњРЎвЂ™Р вЂ™Р’Вµ Р вЂњРЎвЂ™Р вЂ™РўвЂ�Р вЂњРЎвЂ™Р вЂ™РЎвЂўР вЂњРІР‚пїЅР вЂ™РІР‚В¦Р вЂњРЎвЂ™Р вЂ™РЎвЂўР вЂњРЎвЂ™Р вЂ™РўвЂ�Р вЂњРЎвЂ™Р вЂ™РЎвЂ�Р вЂњРІР‚пїЅР вЂ™РІР‚С™ Р вЂњРЎвЂ™Р вЂ™РўвЂ�Р вЂњРЎвЂ™Р вЂ™РЎвЂў Р вЂњРІР‚пїЅР вЂ™Р РЉР вЂњРІР‚пїЅР вЂ™РІР‚С™Р вЂњРЎвЂ™Р вЂ™РЎвЂўР вЂњРЎвЂ™Р вЂ™РЎвЂ“Р вЂњРЎвЂ™Р вЂ™РЎвЂў Р вЂњРЎвЂ™Р вЂ™РЎпїЅР вЂњРЎвЂ™Р вЂ™Р’ВµР вЂњРІР‚пїЅР вЂ™Р С“Р вЂњРІР‚пїЅР вЂ™РІР‚С™Р вЂњРЎвЂ™Р вЂ™Р’В°) */
}

#endif


