/**
  ******************************************************************************
  * @file    hardware_emulation.cpp
  * @author  Artem Pisarenko, PMR dept. software team, ONIIP, PJSC
  * @date    05.11.2015
  * @brief   Реализация эмуляциии аппаратных ресурсов для Qt
  *
  ******************************************************************************
  */

#include "qt_hw_emu.h"
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
