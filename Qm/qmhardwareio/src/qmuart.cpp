/**
  ******************************************************************************
  * @file    qmuart.cpp
  * @author  Artem Pisarenko, PMR dept. software team, ONIIP, PJSC
  * @date    28.10.2015
  *
  * Dummy
  * TODO: implement QmUart class
  *
  ******************************************************************************
  */

#include "qmuart.h"
#include "qmuart_p.h"

QmUart::QmUart(int hw_resource, ConfigStruct* config, QmObject* parent) :
	QmObject(*new QmUartPrivate(this), parent)
{
    QM_UNUSED(hw_resource);
    QM_UNUSED(config);
}

QmUart::~QmUart() {
}

bool QmUart::isOpen() {
	return false;
}

bool QmUart::open() {
	return false;
}

bool QmUart::close() {
	return false;
}

int64_t QmUart::readData(uint8_t* buffer, uint32_t max_size) {
    QM_UNUSED(buffer);
    QM_UNUSED(max_size);
    return -1;
}

int64_t QmUart::writeData(uint8_t* data, uint32_t data_size) {
    QM_UNUSED(data);
    QM_UNUSED(data_size);
    return -1;
}

uint32_t QmUart::getRxDataAvailable() {
	return 0;
}

uint32_t QmUart::getTxSpaceAvailable() {
	return 0;
}
