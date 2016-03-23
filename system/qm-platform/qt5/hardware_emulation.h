/**
  ******************************************************************************
  * @file    hardware_emulation.h
  * @author  Artem Pisarenko, PMR dept. software team, ONIIP, PJSC
  * @date    05.11.2015
  *
  ******************************************************************************
  */

#ifndef HARDWARE_EMULATION_H_
#define HARDWARE_EMULATION_H_

#include "qstring.h"

namespace QtHwEmu {

void init();
void deinit();

void show_message(const QString &text);

} /* namespace QtHwEmu */

#endif /* HARDWARE_EMULATION_H_ */
