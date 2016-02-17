/**
  ******************************************************************************
  * @file    hardware_emulation.h
  * @author  Artem Pisarenko, PMR dept. software team, ONIIP, PJSC
  * @date    05.11.2015
  * @brief   Интерфейс управления эмуляцией аппаратных ресурсов платформы
  *
  ******************************************************************************
  */

#ifndef HARDWARE_EMULATION_H_
#define HARDWARE_EMULATION_H_

#include <qobject.h>
#include <qstring.h>

namespace QtHwEmu {

void init();
void deinit();
int convertToPlatformHwResource(const QString &value);
QObject* getResourceInterface(int hw_resource);
void acquireResource(int hw_resource, QObject *interface);
void releaseResource(QObject *interface);

} /* namespace QtHwEmu */

#endif /* HARDWARE_EMULATION_H_ */
