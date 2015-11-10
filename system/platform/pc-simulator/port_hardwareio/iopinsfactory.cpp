/**
 ******************************************************************************
 * @file    iopinsfactory.cpp
 * @author  Artem Pisarenko, PMR dept. software team, ONIIP, PJSC
 * @date    06.11.2015
 *
 ******************************************************************************
 */

#include "iopinsfactory.h"
#include "iopininterface.h"
#include "hardware_emulation.h"

IopinInterface* IopinsFactory::getInstance(int hw_resource) {
	IopinInterface *instance = qobject_cast<IopinInterface *>(QtHwEmu::getResourceInterface(hw_resource));
	Q_ASSERT(instance);
	return instance;
}

IopinInterface* IopinsFactory::createInstance(int hw_resource) {
	IopinInterface *instance = new IopinInterface();
	QtHwEmu::acquireResource(hw_resource, instance);
	return instance;
}

void IopinsFactory::destroyInstance(IopinInterface* instance) {
	QtHwEmu::releaseResource(instance);
	delete instance;
}
