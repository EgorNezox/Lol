/**
  ******************************************************************************
  * @file    qmiopin_qt.cpp
  * @author  Artem Pisarenko, PMR dept. software team, ONIIP, PJSC
  * @date    28.10.2015
  *
  ******************************************************************************
  */

#include "port_hardwareio/iopinsfactory.h"

#include "qmiopin_p.h"

QmIopinPrivateAdapter::QmIopinPrivateAdapter(QmIopinPrivate *qmiopinprivate) :
	qmiopinprivate(qmiopinprivate)
{
	interface = IopinsFactory::getInstance(qmiopinprivate->hw_resource);
	qmiopinprivate->input_level = convertInputLevelToQm(interface->getInputLevel());
	QObject::connect(interface, &IopinInterface::inputLevelAssigned, this, &QmIopinPrivateAdapter::processInputLevelAssigned);
	QObject::connect(this, &QmIopinPrivateAdapter::writeOutputLevel, interface, &IopinInterface::assignOutputLevel);
}

QmIopinPrivateAdapter::~QmIopinPrivateAdapter()
{
}

QmIopin::Level QmIopinPrivateAdapter::convertInputLevelToQm(IopinInterface::Level value) {
	QmIopin::Level result;
	switch (value) {
	case IopinInterface::Level_Low:
		result = QmIopin::Level_Low;
		break;
	case IopinInterface::Level_High:
		result = QmIopin::Level_High;
		break;
	case IopinInterface::Level_HiZ:
		result = (qrand() % 2)?(QmIopin::Level_Low):(QmIopin::Level_High);
		break;
	}
	return result;
}

void QmIopinPrivateAdapter::processInputLevelAssigned(IopinInterface::Level level) {
	QmIopin * const q = qmiopinprivate->q_func();
	QmIopin::Level new_level = convertInputLevelToQm(level);
	bool do_trigger;
	switch (qmiopinprivate->input_trigger_mode) {
	case QmIopin::InputTrigger_Disabled:
		do_trigger = false;
		break;
	case QmIopin::InputTrigger_Rising:
		do_trigger = ((qmiopinprivate->input_level == QmIopin::Level_Low) && (new_level == QmIopin::Level_High));
		break;
	case QmIopin::InputTrigger_Falling:
		do_trigger = ((qmiopinprivate->input_level == QmIopin::Level_High) && (new_level == QmIopin::Level_Low));
		break;
	case QmIopin::InputTrigger_Both:
		do_trigger = (qmiopinprivate->input_level != new_level);
		break;
	}
	qmiopinprivate->input_level = new_level;
	if (do_trigger)
		q->inputTrigger.emit();
}

QmIopinPrivate::QmIopinPrivate(QmIopin *q) :
	QmObjectPrivate(q),
	hw_resource(-1), input_trigger_mode(QmIopin::InputTrigger_Disabled),
	iopin_adapter(0), input_level(QmIopin::Level_Low)
{
}

QmIopinPrivate::~QmIopinPrivate()
{
}

void QmIopinPrivate::init() {
	iopin_adapter = new QmIopinPrivateAdapter(this);
}

void QmIopinPrivate::deinit()
{
	delete iopin_adapter;
}

bool QmIopin::setInputTriggerMode(LevelTriggerMode mode) {
	QM_D(QmIopin);
	d->input_trigger_mode = mode;
	return true;
}

QmIopin::Level QmIopin::readInput() {
	QM_D(QmIopin);
	return d->input_level;
}

void QmIopin::writeOutput(Level level) {
	QM_D(QmIopin);
	IopinInterface::Level value;
	switch (level) {
	case QmIopin::Level_Low: value = IopinInterface::Level_Low; break;
	case QmIopin::Level_High: value = IopinInterface::Level_High; break;
	}
	Q_EMIT d->iopin_adapter->writeOutputLevel(value);
}

bool QmIopin::event(QmEvent* event) {
	return QmObject::event(event);
}
