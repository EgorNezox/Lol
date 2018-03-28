/**
 ******************************************************************************
 * @file    qmm25pdevice.h
 * @author  Artem Pisarenko, PMR dept. software team, ONIIP, PJSC
 * @date    19.02.2016
 *
 ******************************************************************************
 */

#ifndef QMM25PDEVICE_H_
#define QMM25PDEVICE_H_

#include "qmserialnorflashdevice.h"
#include "qmcrc.h"

class QmM25PDevicePrivate;

class QmM25PDevice: public QmSerialNORFlashDevice {
public:
	struct Config {
		uint32_t sector_size;
		uint32_t sectors_count;
		uint32_t speed;
		bool idle_clock_low;
	};

	/* создать объект класса флэшки с конфигурацией и пинами spi */
	QmM25PDevice(const Config &config, int spi_bus_resource, int cs_resource = -1);
	~QmM25PDevice();

	/* создать crc класс для сверки */
	QmCrc<uint32_t,32,0x04c11db7,0xffffffff,true,0xffffffff> crcUsb;

	/* проверить, что флэшка является m25 */
	bool checkIdentification();

    /* получить размер страницы */
	uint32_t getPageSize();

	/* получить размер флэшки */
	uint32_t getTotalSize();

	/* получить адреса блоков для чтения */
	std::list<Block> getBlocks();

	/* команда считывания в поле data с адреса address количества size */
	bool read(uint32_t address, uint8_t *data, uint32_t size);

	/* команда считывания Crc по принятым байтам */
	uint32_t readCrc32(uint32_t address,uint32_t size);

	/* команда записи  */
	bool write(uint32_t address, uint8_t *data, uint32_t size);

	/* команда очистки сектора */
	bool erase(uint32_t address, uint32_t size);

private:
	QmM25PDevicePrivate *d_ptr;
};

#endif /* QMM25PDEVICE_H_ */
