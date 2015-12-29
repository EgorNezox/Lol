/**
  ******************************************************************************
  * @file    hardware_emulation.cpp
  * @author  Artem Pisarenko, PMR dept. software team, ONIIP, PJSC
  * @date    05.11.2015
  * @brief   Реализация эмуляциии аппаратных ресурсов для Qt
  *
  ******************************************************************************
  */

#include <QMetaEnum>
#include <QMetaObject>
#include <QMap>
#include <QMutex>

#include "hardware_emulation.h"
#include "mainwidget.h"
#include "dsp/dspdevice.h"
#include "port_hardwareio/iopininterface.h"
#include "port_hardwareio/uartinterface.h"
#include "port_hardwareio/i2cbus.h"
#include "port_keysinput/pushbuttonkeyinterface.h"
#include "port_keysinput/matrixkeyboardinterface.h"

#ifndef Q_MOC_RUN
namespace QtHwEmu
#else
class QtHwEmu
#endif
{
#if defined(Q_MOC_RUN)
	Q_GADGET
	Q_ENUMS(platformhw_resource_t)
public:
#endif
#include "../platform_hw_map.h"
	extern const QMetaObject staticMetaObject;
}

namespace QtHwEmu {

static MainWidget *main_widget = 0;
static DspDevice *dsp_device = 0;
static I2CBus *battery_smbus = 0;
static QMap<int, QObject*> resources_registry;
static QMutex resources_registry_mutex;

void init() {
	main_widget = new MainWidget(platformhwMatrixKeyboard);
	main_widget->show();
	dsp_device = new DspDevice(platformhwDspUart, platformhwDspResetIopin);
	dsp_device->move(main_widget->frameGeometry().topRight() + QPoint(10,0));
	dsp_device->show();
	main_widget->activateWindow();
	main_widget->raise();
	battery_smbus = I2CBus::openInstance(platformhwBatterySmbusI2c);
	PushbuttonkeyInterface::createInstance(platformhwHeadsetPttIopin); // TODO: emulate HeadsetPttIopin
	IopinInterface::createInstance(platformhwKeyboardsLightIopin); // TODO: emulate KeyboardsLightIopin
	IopinInterface::createInstance(platformhwEnRxRs232Iopin); // TODO: emulate EnRxRs232Iopin
	IopinInterface::createInstance(platformhwEnTxRs232Iopin); // TODO: emulate EnTxRs232Iopin
	UartInterface::createInstance(platformhwHeadsetUart); //TODO: emulate HeadsetUart
	UartInterface::createInstance(platformhwAtuUart); //TODO: emulate AtuUart
}

void deinit() {
	delete main_widget;
	delete dsp_device;
	I2CBus::closeInstance(battery_smbus);
}

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

#include "hardware_emulation.moc"
