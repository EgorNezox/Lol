/**
  ******************************************************************************
  * @file    hardware_emulation.cpp
  * @author  Artem Pisarenko, PMR dept. software team, ONIIP, PJSC
  * @date    17.02.2016
  *
  ******************************************************************************
  */

#include "hardware_emulation.h"
#include "mainwidget.h"

namespace QtHwEmu {

static MainWidget *main_widget = 0;

void init() {
	Q_ASSERT(main_widget == 0);
	main_widget = new MainWidget();
	main_widget->show();
}

void deinit() {
	Q_ASSERT(main_widget != 0);
	delete main_widget;
}

} /* namespace QtHwEmu */
