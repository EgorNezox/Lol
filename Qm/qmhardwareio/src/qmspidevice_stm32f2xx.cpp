/**
  ******************************************************************************
  * @file    qmspidevice_stm32f2xx.cpp
  * @author  Artem Pisarenko, PMR dept. software team, ONIIP, PJSC
  * @date    11.02.2016
  *
  ******************************************************************************
  */

#include "system_hw_io.h"

#include "qmdebug.h"
#include "qmspidevice_p.h"

QmSPIDevicePrivate::QmSPIDevicePrivate(QmSPIDevice *q) :
	QmObjectPrivate(q),
	bus_hw_resource(-1), cs_hw_resource(-1)
{
}

QmSPIDevicePrivate::~QmSPIDevicePrivate()
{
}

void QmSPIDevicePrivate::init(int bus_hw_resource, QmSPIDevice::BusConfigStruct *bus_config, int cs_hw_resource) {
	this->cs_hw_resource = cs_hw_resource;
	if (cs_hw_resource != -1) {
		stm32f2_ext_pins_init(cs_hw_resource);
		hal_gpio_set_output(stm32f2_get_gpio_pin(cs_hw_resource), hgpioHigh);
	}
	this->bus_hw_resource = bus_hw_resource;
	hal_spi_init_master_transfer_struct(&spi_transfer);
	spi_transfer.max_baud_rate = bus_config->max_baud_rate;
	switch (bus_config->cpha) {
	case QmSPIDevice::CPHA_0: spi_transfer.cpha = hspiCPHA0; break;
	case QmSPIDevice::CPHA_1: spi_transfer.cpha = hspiCPHA1; break;
	}
	switch (bus_config->cpol) {
	case QmSPIDevice::CPOL_0: spi_transfer.cpol = hspiCPOL0; break;
	case QmSPIDevice::CPOL_1: spi_transfer.cpol = hspiCPOL1; break;
	}
	switch (bus_config->first_bit) {
	case QmSPIDevice::FirstBit_MSB: spi_transfer.first_bit = hspiFirstMSB; break;
	case QmSPIDevice::FirstBit_LSB: spi_transfer.first_bit = hspiFirstLSB; break;
	}
}

void QmSPIDevicePrivate::deinit() {
	if (cs_hw_resource != -1)
		stm32f2_ext_pins_deinit(cs_hw_resource);
}

bool QmSPIDevicePrivate::transferFullDuplex() {
	bool result;
	if (cs_hw_resource != -1)
		hal_gpio_set_output(stm32f2_get_gpio_pin(cs_hw_resource), hgpioLow);
	result = hal_spi_master_fd_transfer(stm32f2_get_spi_bus_instance(bus_hw_resource), &spi_transfer);
	if (cs_hw_resource != -1)
		hal_gpio_set_output(stm32f2_get_gpio_pin(cs_hw_resource), hgpioHigh);
	return result;
}

bool QmSPIDevice::transferFullDuplex8bit(uint8_t *rx_data, uint8_t *tx_data, int count) {
	QM_D(QmSPIDevice);
	d->spi_transfer.data_size = hspiDataSize8bit;
	d->spi_transfer.rx_buffer = rx_data;
	d->spi_transfer.tx_buffer = tx_data;
	d->spi_transfer.buffer_size = count;
	return d->transferFullDuplex();
}

bool QmSPIDevice::transferFullDuplex16bit(uint16_t *rx_data, uint16_t *tx_data, int count) {
	QM_D(QmSPIDevice);
	d->spi_transfer.data_size = hspiDataSize16bit;
	d->spi_transfer.rx_buffer = (uint8_t *)rx_data;
	d->spi_transfer.tx_buffer = (uint8_t *)tx_data;
	d->spi_transfer.buffer_size = 2*count;
	return d->transferFullDuplex();
}
