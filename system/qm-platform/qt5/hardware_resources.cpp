/**
  ******************************************************************************
  * @file    hardware_resources.cpp
  * @author  Artem Pisarenko, PMR dept. software team, ONIIP, PJSC
  * @date    05.11.2015
  *
  ******************************************************************************
  */

#include <qmetaobject.h>
#include <qmap.h>
#include <qmutex.h>

#include "hardware_resources.h"

#ifndef Q_MOC_RUN
namespace QtHwEmu
#else
class QtHwEmu
#endif
{
#ifdef Q_MOC_RUN
	Q_GADGET
	Q_ENUMS(platformhw_resource_t)
public:
#endif
#include "../../platform_hw_map.h"
	extern const QMetaObject staticMetaObject;
}

namespace QtHwEmu {

static QMap<int, QObject*> resources_registry;
static QMutex resources_registry_mutex;

int convertToPlatformHwResource(const QString &value) {
	bool cast_is_ok = false;
	int enum_index = staticMetaObject.indexOfEnumerator("platformhw_resource_t");
	QMetaEnum meta_enum = staticMetaObject.enumerator(enum_index);
	int enum_value = meta_enum.keyToValue(value.toLocal8Bit().data(), &cast_is_ok);
	Q_ASSERT(cast_is_ok);
	return enum_value;
}

QObject* getResourceInterface(int hw_resource) {
	QMutexLocker locker(&resources_registry_mutex);
	QObject* interface = resources_registry.value(hw_resource, 0);
	Q_ASSERT(interface);
	return interface;
}

void acquireResource(int hw_resource, QObject *interface) {
	QMutexLocker locker(&resources_registry_mutex);
	Q_ASSERT(hw_resource != -1);
	Q_ASSERT(resources_registry.contains(hw_resource) == 0);
	resources_registry.insert(hw_resource, interface);
}

void releaseResource(QObject *interface) {
	QMutexLocker locker(&resources_registry_mutex);
	int hw_resource = resources_registry.key(interface, -1);
	Q_ASSERT(hw_resource != -1);
	resources_registry.remove(hw_resource);
}

} /* namespace QtHwEmu */

#include "hardware_resources.moc"
