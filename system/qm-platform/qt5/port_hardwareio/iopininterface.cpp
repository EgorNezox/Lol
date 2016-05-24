/**
 ******************************************************************************
 * @file    iopininterface.cpp
 * @author  Artem Pisarenko, PMR dept. software team, ONIIP, PJSC
 * @date    06.11.2015
 *
 ******************************************************************************
 */

#include "iopininterface.h"
#include "hardware_resources.h"

void IopinInterface::init() {
	qRegisterMetaType<Level>();
}

IopinInterface::IopinInterface() :
	input_level(Level_Low), output_level(Level_Low)
{
}

IopinInterface::~IopinInterface()
{
}

IopinInterface* IopinInterface::getInstance(int hw_resource) {
	IopinInterface *instance = qobject_cast<IopinInterface *>(QtHwEmu::getResourceInterface(hw_resource));
	Q_ASSERT(instance);
	return instance;
}

IopinInterface* IopinInterface::createInstance(int hw_resource) {
	IopinInterface *instance = new IopinInterface();
	QtHwEmu::acquireResource(hw_resource, instance);
	return instance;
}

void IopinInterface::destroyInstance(IopinInterface* instance) {
	QtHwEmu::releaseResource(instance);
	delete instance;
}

IopinInterface::Level IopinInterface::getOutputLevel() {
	return output_level;
}

void IopinInterface::setInputLevel(Level level, bool emu_trigger_ovf) {
	input_level = level;
	Q_EMIT inputLevelAssigned(level, emu_trigger_ovf);
}

IopinInterface::Level IopinInterface::getInputLevel() {
	return input_level;
}

void IopinInterface::assignOutputLevel(Level level) {
	if (level != output_level) {
		output_level = level;
		Q_EMIT outputLevelChanged(level);
	}
}
