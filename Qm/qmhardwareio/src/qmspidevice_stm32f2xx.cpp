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
	cs_hw_resource(-1), bus_instance(-1)
{
}

QmSPIDevicePrivate::~QmSPIDevicePrivate()
{
}

void QmSPIDevicePrivate::init(int bus_hw_resource, QmSPIDevice::BusConfigStruct *bus_config, int cs_hw_resource) {
	this->cs_hw_resource = cs_hw_resource;
	if (cs_hw_resource != -1) {
		stm32f2_ext_pins_init(cs_hw_resource);
		cs_pin = stm32f2_get_gpio_pin(cs_hw_resource);
		hal_gpio_set_output(cs_pin, hgpioHigh);
	}
	bus_instance = stm32f2_get_spi_bus_instance(bus_hw_resource);
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

void QmSPIDevicePrivate::chipSelect() {
	if (cs_hw_resource != -1)
		hal_gpio_set_output(cs_pin, hgpioLow);
}

void QmSPIDevicePrivate::chipDeselect() {
	if (cs_hw_resource != -1)
		hal_gpio_set_output(cs_pin, hgpioHigh);
}

bool QmSPIDevice::transferBurstFullDuplex8bit(FD8Burst *bursts, int count) {
	QM_D(QmSPIDevice);
	bool result;
	d->chipSelect();

	d->spi_transfer.data_size = hspiDataSize8bit;
	for (int i = 0; i < count; i++)
	{
		d->spi_transfer.rx_buffer   = bursts[i].rx_data;
		d->spi_transfer.tx_buffer   = bursts[i].tx_data;
		d->spi_transfer.buffer_size = bursts[i].count;

		result = hal_spi_master_fd_transfer(d->bus_instance, &(d->spi_transfer));
		if (!result)
			break;
	}
	d->chipDeselect();
	return result;
}

bool QmSPIDevice::transferBurstFullDuplex16bit(FD16Burst *bursts, int count) {
	QM_D(QmSPIDevice);
	bool result;
	d->chipSelect();

	d->spi_transfer.data_size = hspiDataSize16bit;

	for (int i = 0; i < count; i++)
	{
		d->spi_transfer.rx_buffer   = (uint8_t *)(bursts[i].rx_data);
		d->spi_transfer.tx_buffer   = (uint8_t *)(bursts[i].tx_data);
		d->spi_transfer.buffer_size = 2 * (bursts[i].count);

		result = hal_spi_master_fd_transfer(d->bus_instance, &(d->spi_transfer));

		if (!result)
			break;
	}
	d->chipDeselect();

	return result;
}
