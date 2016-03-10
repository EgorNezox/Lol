/**
 ******************************************************************************
 * @file    qmserialnorflashdevice.h
 * @author  Artem Pisarenko, PMR dept. software team, ONIIP, PJSC
 * @date    18.02.2016
 *
 ******************************************************************************
 */

#ifndef QMSERIALNORFLASHDEVICE_H_
#define QMSERIALNORFLASHDEVICE_H_

#include <stdint.h>
#include <list>

class QmSerialNORFlashDevice {
public:
	struct Block {
		uint32_t address;
		uint32_t size;
	};

	virtual uint32_t getPageSize() = 0;
	virtual uint32_t getTotalSize() = 0;
	virtual std::list<Block> getBlocks() = 0;
	virtual bool read(uint32_t address, uint8_t *data, uint32_t size) = 0;
	virtual bool write(uint32_t address, uint8_t *data, uint32_t size) = 0;
	virtual bool erase(uint32_t address, uint32_t size) = 0;

protected:
	inline QmSerialNORFlashDevice() {};
	inline virtual ~QmSerialNORFlashDevice() {};
};

#endif /* QMSERIALNORFLASHDEVICE_H_ */
