/**
 ******************************************************************************
 * @file    flashm25pdevice.cpp
 * @author  Artem Pisarenko, PMR dept. software team, ONIIP, PJSC
 * @date    20.02.2016
 *
 ******************************************************************************
 */

#include <qglobal.h>
#include <qfile.h>
#include "port_hardwareio/spideviceinterface.h"

#include "flashm25pdevice.h"

static const int M25P_PAGE_SIZE = 256;

FlashM25PDevice::FlashM25PDevice(const QString &image_path, const Config &config, int spi_bus_resource, int cs_resource) :
	image(new QFile(image_path)), config(config), interface(new SPIDeviceInterface(spi_bus_resource, cs_resource)),
	bit_wel(false)
{
	if (image->exists()) {
		image->open(QIODevice::ReadWrite);
		image->resize(getConfigTotalSize());
	}
	QObject::connect(interface, &SPIDeviceInterface::transferFullDuplex8bit, this, &FlashM25PDevice::processInterfaceTransfer);
	QObject::connect(interface, &SPIDeviceInterface::transferFullDuplex16bit, this, &FlashM25PDevice::processInterfaceFD16Transfer);
}

FlashM25PDevice::~FlashM25PDevice()
{
	if (image->isOpen())
		image->close();
	delete interface;
	delete image;
}

void FlashM25PDevice::processInterfaceTransfer(quint8* rx_data, quint8* tx_data, int count) {
	if (!(image->isOpen() && (count > 0)))
		return;
	quint8 command_id = tx_data[0];
	quint8 *rx_bytes_data = rx_data + 1;
	quint8 *tx_bytes_data = tx_data + 1;
	int bytes_count = count - 1;
	switch (command_id) {
	case 0x06: {
		if (!(bytes_count == 0))
			break;
		processCommandWriteEnable();
		break;
	}
	case 0x04: {
		if (!(bytes_count == 0))
			break;
		processCommandWriteDisable();
		break;
	}
	case 0x9F: {
		processCommandReadIdentification(rx_bytes_data, bytes_count);
		break;
	}
	case 0x05: {
		processCommandReadStatusRegister(rx_bytes_data, bytes_count);
		break;
	}
	case 0x03: {
		if (!(bytes_count > 3))
			break;
		processCommandReadDataBytes(tx_bytes_data, rx_bytes_data+3, bytes_count-3);
		break;
	}
	case 0x0B: {
		if (!(bytes_count > 4))
			break;
		processCommandReadDataBytes(tx_bytes_data, rx_bytes_data+4, bytes_count-4);
		break;
	}
	case 0x02: {
		if (!(bytes_count > 3))
			break;
		processCommandPageProgram(tx_bytes_data, tx_bytes_data+3, bytes_count-3);
		break;
	}
	case 0xD8: {
		if (!(bytes_count == 3))
			break;
		processCommandSectorErase(tx_bytes_data);
		break;
	}
	case 0xC7: {
		if (!(bytes_count == 0))
			break;
		processCommandBulkErase();
		break;
	}
	default: break;
	}
}

void FlashM25PDevice::processInterfaceFD16Transfer(quint16* rx_data, quint16* tx_data, int count) {
	Q_UNUSED(rx_data);
	Q_UNUSED(tx_data);
	Q_UNUSED(count);
	Q_ASSERT(0); // not supported operation
}

quint32 FlashM25PDevice::getConfigTotalSize() {
	return (config.sector_size * config.sectors_count);
}

void FlashM25PDevice::processCommandWriteEnable() {
	bit_wel = true;
}

void FlashM25PDevice::processCommandWriteDisable() {
	bit_wel = false;
}

void FlashM25PDevice::processCommandReadIdentification(quint8* data_out, int count) {
	if (count >= 1)
		data_out[0] = 0x20;
	if (count >= 2)
		data_out[1] = 0x20;
	if (count >= 3)
		data_out[2] = config.memory_capacity_id;
}

void FlashM25PDevice::processCommandReadStatusRegister(quint8* data_out, int count) {
	quint8 status_reg_value = 0x00 | (bit_wel?0x02:0x00);
	for (int i = 0; i < count; i++)
		data_out[i] = status_reg_value;
}

void FlashM25PDevice::processCommandReadDataBytes(quint8* address_in, quint8* data_out, int data_count) {
	quint32 address = 0;
	address |= (address_in[0] & 0xFF) << 16;
	address |= (address_in[1] & 0xFF) << 8;
	address |= address_in[2] & 0xFF;
	if (!image->seek(address))
		return;
	image->read((char *)data_out, data_count);
}

void FlashM25PDevice::processCommandPageProgram(quint8* address_in, quint8* data_in, int data_count) {
	if (!bit_wel)
		return;
	quint32 address = 0;
	address |= (address_in[0] & 0xFF) << 16;
	address |= (address_in[1] & 0xFF) << 8;
	address |= address_in[2] & 0xFF;
	if (!((address + data_count) <= getConfigTotalSize()))
		return;
	if (!image->seek(address & ~(M25P_PAGE_SIZE-1)))
		return;
	QByteArray original_page_data = image->read(M25P_PAGE_SIZE);
	if (original_page_data.size() != M25P_PAGE_SIZE)
		return;
	QByteArray page_data = original_page_data;
	int page_offset = address & (M25P_PAGE_SIZE-1);
	while (data_count > 0) {
		int page_chunk_size = qMin(data_count, (M25P_PAGE_SIZE - page_offset));
		page_data.replace(page_offset, page_chunk_size, (char *)data_in, page_chunk_size);
		page_offset = 0;
		data_in += page_chunk_size;
		data_count -= page_chunk_size;
	}
	for (int i = 0; i < M25P_PAGE_SIZE; i++)
		page_data[i] = page_data.at(i) & original_page_data.at(i);
	if (!image->seek(address & ~(M25P_PAGE_SIZE-1)))
		return;
	if (image->write(page_data) != M25P_PAGE_SIZE)
		return;
	if (!image->flush())
		return;
	bit_wel = false;
}

void FlashM25PDevice::processCommandSectorErase(quint8 *address_in) {
	if (!bit_wel)
		return;
	quint32 address = 0;
	address |= (address_in[0] & 0xFF) << 16;
	address |= (address_in[1] & 0xFF) << 8;
	address |= address_in[2] & 0xFF;
	if (!(address < getConfigTotalSize()))
		return;
	address &= ~(config.sector_size-1);
	if (!image->seek(address))
		return;
	if (image->write(QByteArray(config.sector_size, 0xFF)) != config.sector_size)
		return;
	if (!image->flush())
		return;
	bit_wel = false;
}

void FlashM25PDevice::processCommandBulkErase() {
	if (!bit_wel)
		return;
	if (!image->seek(0))
		return;
	if (image->write(QByteArray(getConfigTotalSize(), 0xFF)) != getConfigTotalSize())
		return;
	if (!image->flush())
		return;
	bit_wel = false;
}
