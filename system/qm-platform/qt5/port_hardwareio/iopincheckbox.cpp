/**
 ******************************************************************************
 * @file    iopincheckbox.cpp
 * @author  Artem Pisarenko, PMR dept. software team, ONIIP, PJSC
 * @date    06.11.2015
 *
 ******************************************************************************
 */

#include <qmetaobject.h>

#include "iopincheckbox.h"
#include "hardware_resources.h"

IopinCheckBox::IopinCheckBox(QWidget *parent, int hw_resource) :
	QCheckBox(parent), pin_interface(0)
{
	if (hw_resource != -1)
		assignHwResource(hw_resource);
	setDirection(Direction_Input);
}

IopinCheckBox::~IopinCheckBox() {
	if (pin_interface)
		IopinInterface::destroyInstance(pin_interface);
}

QString IopinCheckBox::getHwResource() {
	return QString();
}

void IopinCheckBox::assignHwResource(const QString &value) {
	assignHwResource(QtHwEmu::convertToPlatformHwResource(value));
}

void IopinCheckBox::assignHwResource(int value) {
	Q_ASSERT(pin_interface == 0);
	pin_interface = IopinInterface::createInstance(value);
	QObject::connect(pin_interface, &IopinInterface::outputLevelChanged, this, &IopinCheckBox::processPinOutputLevelChanged);
	QObject::connect(this, &QCheckBox::stateChanged, this, &IopinCheckBox::processCBStateChanged);
	synchronizeInterface();
}

QString IopinCheckBox::getDirection() {
	return QString();
}

void IopinCheckBox::setDirection(const QString& value) {
	bool cast_is_ok = false;
	int enum_index = staticMetaObject.indexOfEnumerator("Direction");
	QMetaEnum meta_enum = staticMetaObject.enumerator(enum_index);
	Direction enum_value = static_cast<Direction>(meta_enum.keyToValue(QString("Direction_"+value).toLocal8Bit().data(), &cast_is_ok));
	Q_ASSERT(cast_is_ok);
	setDirection(enum_value);
}

void IopinCheckBox::setDirection(Direction value) {
	setEnabled(value == Direction_Input);
	synchronizeInterface();
}

void IopinCheckBox::processPinOutputLevelChanged(IopinInterface::Level level) {
	switch (level) {
	case IopinInterface::Level_Low: setCheckState(Qt::Unchecked); break;
	case IopinInterface::Level_HiZ: setCheckState(Qt::PartiallyChecked); break;
	case IopinInterface::Level_High: setCheckState(Qt::Checked); break;
	}
}

void IopinCheckBox::synchronizeInterface() {
	if (pin_interface == 0)
		return;
	if (isEnabled()) {
		processCBStateChanged(checkState());
	} else {
		processPinOutputLevelChanged(pin_interface->getOutputLevel());
	}
}

void IopinCheckBox::processCBStateChanged(int state) {
	IopinInterface::Level level;
	bool emu_trigger_ovf = false;
	switch (state) {
	case Qt::Unchecked:
		level = IopinInterface::Level_Low;
		break;
	case Qt::PartiallyChecked:
		level = IopinInterface::Level_HiZ;
		emu_trigger_ovf = true;
		break;
	case Qt::Checked:
		level = IopinInterface::Level_High;
		break;
	default:
		Q_ASSERT(0);
		return;
	}
	pin_interface->setInputLevel(level, emu_trigger_ovf);
}
