/**
 ******************************************************************************
 * @file    iopininterface.cpp
 * @author  Artem Pisarenko, PMR dept. software team, ONIIP, PJSC
 * @date    06.11.2015
 *
 ******************************************************************************
 */

#include "iopininterface.h"

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

IopinInterface::Level IopinInterface::getOutputLevel() {
	return output_level;
}

void IopinInterface::setInputLevel(Level level) {
	input_level = level;
	Q_EMIT inputLevelAssigned(level);
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
