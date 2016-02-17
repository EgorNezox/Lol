/**
  ******************************************************************************
  * @file    firmware_main.cpp
  * @author  Artem Pisarenko, PMR dept. software team, ONIIP, PJSC
  * @date    18.08.2015
  * @brief   Загрузочный модуль приложения прошивки.
  *
  ******************************************************************************
 */

#include "../system/platform_hw_map.h"
#include "qmapplication.h"

void qmMain() {
	QmApplication a;

	a.exec();
}
