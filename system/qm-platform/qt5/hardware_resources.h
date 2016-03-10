/**
  ******************************************************************************
  * @file    hardware_resources.h
  * @author  Artem Pisarenko, PMR dept. software team, ONIIP, PJSC
  * @date    17.02.2016
  *
  ******************************************************************************
 */

#ifndef HARDWARE_RESOURCES_H_
#define HARDWARE_RESOURCES_H_

#include <qobject.h>
#include <qstring.h>

namespace QtHwEmu {

int convertToPlatformHwResource(const QString &value);
QObject* getResourceInterface(int hw_resource);
void acquireResource(int hw_resource, QObject *interface);
void releaseResource(QObject *interface);

} /* namespace QtHwEmu */

#endif /* HARDWARE_RESOURCES_H_ */
