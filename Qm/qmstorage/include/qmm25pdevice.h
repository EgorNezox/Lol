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

	QmM25PDevice(const Config &config, int spi_bus_resource, int cs_resource = -1);
	~QmM25PDevice();

	QmCrc<uint32_t,32,0x04c11db7,0xffffffff,true,0xffffffff> crcUsb;

	bool checkIdentification();

	uint32_t getPageSize();
	uint32_t getTotalSize();
	std::list<Block> getBlocks();
	bool read(uint32_t address, uint8_t *data, uint32_t size);

	uint32_t readCrc32(uint32_t address, uint8_t *data, uint32_t size);

	bool write(uint32_t address, uint8_t *data, uint32_t size);
	bool erase(uint32_t address, uint32_t size);

private:
	QmM25PDevicePrivate *d_ptr;
};

#endif /* QMM25PDEVICE_H_ */
