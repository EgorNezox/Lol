/**
 ******************************************************************************
 * @file    iopininterface.cpp
 * @author  Artem Pisarenko, PMR dept. software team, ONIIP, PJSC
 * @date    06.11.2015
 *
 ******************************************************************************
 */

#include "iopininterface.h"

IopinInterface::IopinInterface() :
	output_level(Level_Low)
{
}

IopinInterface::~IopinInterface()
{
}

IopinInterface::Level IopinInterface::getOutputLevel() {
	return output_level;
}

void IopinInterface::setInputLevel(Level level) {
	Q_EMIT inputLevelAssigned(level);
}

void IopinInterface::assignOutputLevel(Level level) {
	if (level != output_level) {
		output_level = level;
		Q_EMIT outputLevelChanged(level);
	}
}
