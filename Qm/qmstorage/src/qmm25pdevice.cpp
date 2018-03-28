/**
 ******************************************************************************
 * @file    qmm25pdevice.cpp
 * @author  Artem Pisarenko, PMR dept. software team, ONIIP, PJSC
 * @date    19.02.2016
 *
 ******************************************************************************
 */

#include "qm.h"
#include "qmelapsedtimer.h"
#include "qmthread.h"
#include "qmspidevice.h"

#include "qmm25pdevice.h"

class QmM25PDevicePrivate {
public:
	bool commandWriteEnable();
	bool pollWIPStatus(int timeout_ms);

	QmM25PDevice::Config config;
	QmSPIDevice *interface;
	QmElapsedTimer poll_timer;
};

static const int M25P_PAGE_SIZE = 256;
static const int M25P_MAX_PAGE_PROGRAM_CYCLE_TIME = 5;
static const int M25P_MAX_SECTOR_ERASE_CYCLE_TIME = 3000;

QmM25PDevice::QmM25PDevice(const Config &config, int spi_bus_resource, int cs_resource) :
	d_ptr(new QmM25PDevicePrivate())
{
	d_ptr->config = config;
	QmSPIDevice::BusConfigStruct spi_interface_config;
	spi_interface_config.max_baud_rate = config.speed;

	if (config.idle_clock_low)
	{
		spi_interface_config.cpha = QmSPIDevice::CPHA_0;
		spi_interface_config.cpol = QmSPIDevice::CPOL_0;
	}
	else
	{
		spi_interface_config.cpha = QmSPIDevice::CPHA_1;
		spi_interface_config.cpol = QmSPIDevice::CPOL_1;
	}

	spi_interface_config.first_bit = QmSPIDevice::FirstBit_MSB;
	d_ptr->interface = new QmSPIDevice(spi_bus_resource, &spi_interface_config, cs_resource);
}

QmM25PDevice::~QmM25PDevice()
{
	delete d_ptr->interface;
	delete d_ptr;
}

bool QmM25PDevicePrivate::commandWriteEnable()
{
	uint8_t command = 0x06;

	if (!interface->transferFullDuplex8bit(0, &command, sizeof(command)))
		return false;

	return true;
}

bool QmM25PDevicePrivate::pollWIPStatus(int timeout_ms)
{
	uint32_t poll_delay = qmMin(1, timeout_ms);
	uint8_t command_data[2];
	bool wip;

	poll_timer.start();

	do
	{
		QmThread::msleep(poll_delay);
		command_data[0] = 0x05;

		if (!interface->transferFullDuplex8bit(command_data, command_data, sizeof(command_data)))
			return false;

		wip = ((command_data[1] & 0x01) != 0);
	} while (wip && !poll_timer.hasExpired(timeout_ms));
	return (!wip);
}

bool QmM25PDevice::checkIdentification()
{
	uint8_t command_data[4];
	command_data[0] = 0x9F;

	if (!d_ptr->interface->transferFullDuplex8bit(command_data, command_data, sizeof(command_data)))
		return false;

	uint8_t expected_capacity_value = (15 - __builtin_clz(getTotalSize())) + 0x10;

	if (!((command_data[1] == 0x20) && (command_data[2] == 0x20)
	 &&   (command_data[3] == expected_capacity_value)))
		return false;
	return true;
}

uint32_t QmM25PDevice::getPageSize()
{
	return M25P_PAGE_SIZE;
}

uint32_t QmM25PDevice::getTotalSize()
{
	return (d_ptr->config.sectors_count * d_ptr->config.sector_size);
}

std::list<QmSerialNORFlashDevice::Block> QmM25PDevice::getBlocks()
{
	std::list<Block> blocks;
	for (uint32_t i = 0; i < d_ptr->config.sectors_count; i++)
		blocks.push_back((Block){i*d_ptr->config.sector_size, d_ptr->config.sector_size});
	return blocks;
}

bool QmM25PDevice::read(uint32_t address, uint8_t* data, uint32_t size) {
	uint8_t command[] = {
			0x0B,
			(uint8_t)((address >> 16) & 0xFF),
			(uint8_t)((address >> 8) & 0xFF),
			(uint8_t)(address & 0xFF),
			0
	};
	QmSPIDevice::FD8Burst spi_bursts[2] = {
			{0, command, sizeof(command)},
			{data, 0, (int)size}
	};
	if (!d_ptr->interface->transferBurstFullDuplex8bit(spi_bursts, sizeof(spi_bursts)/sizeof(spi_bursts[0])))
		return false;
	return true;
}


uint32_t QmM25PDevice::readCrc32(uint32_t address, uint32_t size)
{
	uint8_t command[] =
	    {
				0x0B,
				(uint8_t)((address >> 16) & 0xFF),
				(uint8_t)((address >> 8)  & 0xFF),
				(uint8_t)(address         & 0xFF),
				0xFF
		};

		QmSPIDevice::FD8Burst spi_bursts[1] =
		{
				{0, command, sizeof(command)}
		};

		if (!d_ptr->interface->transferBurstFullDuplex8bit(spi_bursts, sizeof(spi_bursts)/sizeof(spi_bursts[0])))
			return false;

		uint8_t data;


		for(uint32_t i = 0; i < size; i++)
		{
			if (!d_ptr->interface->transferFullDuplex8bit(&data,0,1))
				return false;

			crcUsb.update(&data,1);
		}

		return crcUsb.result();
}

bool QmM25PDevice::write(uint32_t address, uint8_t* data, uint32_t size) {
	while (size > 0)
	{
		/* если запись запрещена то возвращаемся */
		if (!d_ptr->commandWriteEnable())
			return false;

		/* получаем количество байт, которое войдет в текущую страницу  */
		int chunk_size = qmMin(size, (M25P_PAGE_SIZE - (address & (M25P_PAGE_SIZE-1))));

		/* формируем команду на запись */
		uint8_t page_program_command[] =
		{
				0x02,
				(uint8_t) ((address >> 16) & 0xFF),
				(uint8_t) ((address >> 8)  & 0xFF),
				(uint8_t) ( address        & 0xFF)
		};

		/* в буфере две команды на запись - (0) команда записи и (1) сами данные */
		QmSPIDevice::FD8Burst spi_bursts[2] =
		{
				{0, page_program_command, sizeof(page_program_command)},
				{0, data, chunk_size}
		};

		/* если запись первой команды успешна, то  */
		if (!d_ptr->interface->transferBurstFullDuplex8bit(spi_bursts, sizeof(spi_bursts)/sizeof(spi_bursts[0])))
			return false;

		/* если через 5 мс spi не доступен, то считаем, что ошибка */
		if (!d_ptr->pollWIPStatus(M25P_MAX_PAGE_PROGRAM_CYCLE_TIME))
			return false;

		/* прибавить адрес, прибавить данные, отнять размер */
		address += chunk_size;
		data    += chunk_size;
		size    -= chunk_size;
	}
	return true;
}

bool QmM25PDevice::erase(uint32_t address, uint32_t size)
{
	/* если превысили размер флэшки, то выходим */
	if (!( ((address & (d_ptr->config.sector_size - 1)) == 0)
		&& ((size    & (d_ptr->config.sector_size - 1)) == 0) ))
		return false;

	/* если запись запрещена то возвращаемся */
	if (!d_ptr->commandWriteEnable())
		return false;

	/* если байты записаны не до конца  */
	while (size > 0)
	{
		/* если запись запрещена то возвращаемся */
		if (!d_ptr->commandWriteEnable())
			return false;

		/* формируем массив из команды на очистку */
		uint8_t sector_erase_command[] =
		{
				0xD8,
				(uint8_t)((address >> 16) & 0xFF),
				(uint8_t)((address >> 8)  & 0xFF),
				(uint8_t)(address         & 0xFF)
		};

		/* отправляем команду на стирание */
		if (!d_ptr->interface->transferFullDuplex8bit(0, sector_erase_command, sizeof(sector_erase_command)))
			return false;

		/* ждем ответа от флэшки  */
		if (!d_ptr->pollWIPStatus(M25P_MAX_SECTOR_ERASE_CYCLE_TIME))
			return false;

		/* к адресу добавляем размер сектора, от размера отнимаем размер сектора */
		address += d_ptr->config.sector_size;
		size    -= d_ptr->config.sector_size;
	}

	return true;
}
