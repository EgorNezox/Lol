/**
  ******************************************************************************
  * @file    qmspidevice_p.h
  * @author  Artem Pisarenko, PMR dept. software team, ONIIP, PJSC
  * @date    11.02.2016
  *
  ******************************************************************************
  */

#ifndef QMSPIDEVICE_P_H_
#define QMSPIDEVICE_P_H_

#include "../../qmcore/src/qmobject_p.h"
#include "qmspidevice.h"

#ifdef QM_PLATFORM_STM32F2XX
#include "hal_gpio.h"
#include "hal_spi.h"
#endif /* QM_PLATFORM_STM32F2XX */
#ifdef QM_PLATFORM_QT
#include <qobject.h>
#include <qbytearray.h>
#include "port_hardwareio/spibus.h"
#endif /* QM_PLATFORM_QT */

#ifdef QM_PLATFORM_QT
class QmSPIDevicePrivate;
class QmSPIDevicePrivateAdapter : public QObject
{
	Q_OBJECT
public:
	QmSPIDevicePrivateAdapter(QmSPIDevicePrivate *qmspideviceprivate);
	~QmSPIDevicePrivateAdapter();
	Q_INVOKABLE void connectBus();
	QmSPIDevicePrivate *qmspideviceprivate;
	SPIBus *bus;
Q_SIGNALS:
	void transferFullDuplex8bit(int cs_hw_resource, quint8 *rx_data, quint8 *tx_data, int count);
	void transferFullDuplex16bit(int cs_hw_resource, quint16 *rx_data, quint16 *tx_data, int count);
private:
	bool event(QEvent *e);
};
#endif /* QM_PLATFORM_QT */

class QmSPIDevicePrivate : public QmObjectPrivate {
	QM_DECLARE_PUBLIC(QmSPIDevice)
public:
	QmSPIDevicePrivate(QmSPIDevice *q);
	virtual ~QmSPIDevicePrivate();
private:
	void init(int bus_hw_resource, QmSPIDevice::BusConfigStruct *bus_config, int cs_hw_resource);
	void deinit();
	int cs_hw_resource;
#ifdef QM_PLATFORM_STM32F2XX
	inline void chipSelect() __attribute__((always_inline));
	inline void chipDeselect() __attribute__((always_inline));
	hal_gpio_pin_t cs_pin;
	int bus_instance;
	struct hal_spi_master_transfer_t spi_transfer;
#endif /* QM_PLATFORM_STM32F2XX */
#ifdef QM_PLATFORM_QT
	friend class QmSPIDevicePrivateAdapter;
	QmSPIDevicePrivateAdapter *spidevice_adapter;
	int bus_hw_resource;
#endif /* QM_PLATFORM_QT */
};

#endif /* QMSPIDEVICE_P_H_ */
