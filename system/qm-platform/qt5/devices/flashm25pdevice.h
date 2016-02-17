/**
 ******************************************************************************
 * @file    flashm25pdevice.h
 * @author  Artem Pisarenko, PMR dept. software team, ONIIP, PJSC
 * @date    20.02.2016
 *
 ******************************************************************************
 */

#ifndef FLASHM25PDEVICE_H_
#define FLASHM25PDEVICE_H_

#include <qobject.h>
#include <qstring.h>

class QFile;
class SPIDeviceInterface;

class FlashM25PDevice : public QObject {
	Q_OBJECT

public:
	struct Config {
		quint8 memory_capacity_id;
		quint32 sector_size;
		quint32 sectors_count;
	};

	FlashM25PDevice(const QString &image_path, const Config &config, int spi_bus_resource, int cs_resource = -1);
	~FlashM25PDevice();

private Q_SLOTS:
	void processInterfaceTransfer(quint8 *rx_data, quint8 *tx_data, int count);
	void processInterfaceFD16Transfer(quint16 *rx_data, quint16 *tx_data, int count);

private:
	quint32 getConfigTotalSize();
	void processCommandWriteEnable();
	void processCommandWriteDisable();
	void processCommandReadIdentification(quint8 *data_out, int count);
	void processCommandReadStatusRegister(quint8 *data_out, int count);
	void processCommandReadDataBytes(quint8 *address_in, quint8 *data_out, int data_count);
	void processCommandPageProgram(quint8 *address_in, quint8 *data_in, int data_count);
	void processCommandSectorErase(quint8 *address_in);
	void processCommandBulkErase();

	QFile *image;
	Config config;
	SPIDeviceInterface *interface;
	bool bit_wel;
};

#endif /* FLASHM25PDEVICE_H_ */
