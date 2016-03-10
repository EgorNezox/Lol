/**
  ******************************************************************************
  * @file    qmspidevice.h
  * @author  Artem Pisarenko, PMR dept. software team, ONIIP, PJSC
  * @date    11.02.2016
  *
  ******************************************************************************
  */

#ifndef QMSPIDEVICE_H_
#define QMSPIDEVICE_H_

#include "qmobject.h"

QM_FORWARD_PRIVATE(QmSPIDevice)

/*! The QmSPIDevice class provides functions to access SPI bus devices(slaves).
 */
class QmSPIDevice: public QmObject {
public:
	enum BusCPHA {
		CPHA_0,
		CPHA_1
	};
	enum BusCPOL {
		CPOL_0,
		CPOL_1
	};
	enum BusFirstBit {
		FirstBit_MSB,
		FirstBit_LSB
	};
	struct BusConfigStruct {
		uint32_t max_baud_rate;	/*!< макс. битовая скорость (0 - минимально возможная) */
		BusCPHA cpha; 			/*!< CPHA */
		BusCPOL cpol; 			/*!< CPOL */
		BusFirstBit first_bit;	/*!< порядок бит в данных */
	};
	struct FD8Burst {
		uint8_t *rx_data;
		uint8_t *tx_data;
		int count;
	};
	struct FD16Burst {
		uint16_t *rx_data;
		uint16_t *tx_data;
		int count;
	};

	/*! Constructs a device with the given \a parent.
	 *
	 * Parameter \a bus_hw_resource specifies platform-identified instance of a bus device connected to.
	 * Parameter \a cs_hw_resource specifies platform-identified instance of device nCS (chip select) pin (optional).
	 */
	QmSPIDevice(int bus_hw_resource, BusConfigStruct *bus_config, int cs_hw_resource = -1, QmObject *parent = 0);

	/*! Destroys the device. */
	virtual ~QmSPIDevice();

	bool transferFullDuplex8bit(uint8_t *rx_data, uint8_t *tx_data, int count);
	bool transferBurstFullDuplex8bit(FD8Burst *bursts, int count);

	bool transferFullDuplex16bit(uint16_t *rx_data, uint16_t *tx_data, int count);
	bool transferBurstFullDuplex16bit(FD16Burst *bursts, int count);

private:
	QM_DECLARE_PRIVATE(QmSPIDevice)
	QM_DISABLE_COPY(QmSPIDevice)
};

#endif /* QMSPIDEVICE_H_ */
