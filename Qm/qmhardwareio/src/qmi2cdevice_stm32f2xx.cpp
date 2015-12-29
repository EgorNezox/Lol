/**
  ******************************************************************************
  * @file    qmi2cdevice_stm32f2xx.cpp
  * @author  Artem Pisarenko, PMR dept. software team, ONIIP, PJSC
  * @date    23.11.2015
  *
  ******************************************************************************
  */

#include <string.h>
#include "system_hw_io.h"

#include "qmdebug.h"
#include "qmi2cdevice_p.h"
#include "qmevent.h"
#include "qmapplication.h"

static void qmi2cdeviceTransferCompletedIsrCallback(struct hal_i2c_master_transfer_t *t, hal_i2c_transfer_result_t result, signed portBASE_TYPE *pxHigherPriorityTaskWoken) {
	QmI2CDeviceIOEvent *system_event = static_cast<QmI2CDeviceIOEvent *>(t->userid);
	system_event->transfer_result = result;
	system_event->transfer_completed = true;
	system_event->setPendingFromISR(pxHigherPriorityTaskWoken);
	xSemaphoreGiveFromISR(system_event->sync_semaphore, pxHigherPriorityTaskWoken);
}

QmI2CDeviceIOEvent::QmI2CDeviceIOEvent(QmI2CDevice *o) :
	o(o),
	transfer_completed(false), transfer_result(hi2cSuccess)
{
	sync_semaphore = xSemaphoreCreateBinary();
}

QmI2CDeviceIOEvent::~QmI2CDeviceIOEvent()
{
	vSemaphoreDelete(sync_semaphore);
}

void QmI2CDeviceIOEvent::process() {
	QmApplication::postEvent(o, new QmEvent(QmEvent::HardwareIO));
}

void QmI2CDeviceIOEvent::resetTransferState() {
	transfer_completed = false;
	xSemaphoreTake(sync_semaphore, 0);
}

void QmI2CDeviceIOEvent::syncTransferComplete() {
	while (!transfer_completed)
		xSemaphoreTake(sync_semaphore, portMAX_DELAY);
}

QmI2CDevicePrivate::QmI2CDevicePrivate(QmI2CDevice *q) :
	QmObjectPrivate(q),
	bus_hw_resource(-1), transfer_timeout(0), transfer_in_progress(false),
	transfer_rx_size(0), io_event(q), transfer_timer(true)
{
	transfer_timer.timeout.connect(sigc::mem_fun(this, &QmI2CDevicePrivate::abortTransferInProgress));
}

QmI2CDevicePrivate::~QmI2CDevicePrivate()
{
}

void QmI2CDevicePrivate::init(uint8_t address) {
	i2c_transfer.device.bus_instance = stm32f2_get_i2c_bus_instance(bus_hw_resource);
	i2c_transfer.device.address = address;
	i2c_transfer.use_pec = false;
	i2c_transfer.dirs = 0;
	i2c_transfer.data = 0;
	i2c_transfer.size = 0;
	i2c_transfer.userid = static_cast<void *>(&io_event);
	i2c_transfer.isrcallbackTransferCompleted = qmi2cdeviceTransferCompletedIsrCallback;
}

void QmI2CDevicePrivate::deinit() {
	if (transfer_in_progress) {
		transfer_timer.stop();
		abortTransferInProgress();
		io_event.syncTransferComplete();
		transfer_in_progress = false;
	}
	if (i2c_transfer.dirs)
		delete[] i2c_transfer.dirs;
	if (i2c_transfer.data)
		delete[] i2c_transfer.data;
}

void QmI2CDevicePrivate::processEventHardwareIO() {
	QM_Q(QmI2CDevice);
	if (!transfer_in_progress)
		return;
	if (io_event.transfer_completed) {
		QmI2CDevice::TransferResult result;
		switch (io_event.transfer_result) {
		case hi2cSuccess: result = QmI2CDevice::transferSuccess; break;
		case hi2cErrorAborted: result = QmI2CDevice::transferErrorAborted; break;
		case hi2cErrorPEC: result = QmI2CDevice::transferErrorPEC; break;
		case hi2cErrorBus: result = QmI2CDevice::transferErrorBus; break;
		case hi2cErrorAddressNACK: result = QmI2CDevice::transferErrorAddressNACK; break;
		case hi2cErrorDataNACK: result = QmI2CDevice::transferErrorDataNACK; break;
		}
		transfer_in_progress = false;
		transfer_timer.stop();
		q->transferCompleted(result);
	}
}

bool QmI2CDevicePrivate::prepareTransfer() {
	if (transfer_in_progress)
		return false;
	i2c_transfer.use_pec = false;
	if (i2c_transfer.dirs) {
		delete[] i2c_transfer.dirs;
		i2c_transfer.dirs = 0;
	}
	if (i2c_transfer.data) {
		delete[] i2c_transfer.data;
		i2c_transfer.data = 0;
	}
	i2c_transfer.size = 0;
	transfer_rx_size = 0;
	io_event.resetTransferState();
	return true;
}

bool QmI2CDevicePrivate::startTransfer() {
	if (!hal_i2c_start_master_transfer(&i2c_transfer))
		return false;
	if (transfer_timeout > 0)
		transfer_timer.start(transfer_timeout);
	transfer_in_progress = true;
	return true;
}

void QmI2CDevicePrivate::abortTransferInProgress() {
	QM_ASSERT(transfer_in_progress);
	hal_i2c_abort_master_transfer(&i2c_transfer);
}

bool QmI2CDevice::startEmptyTransfer() {
	QM_D(QmI2CDevice);
	if (!d->prepareTransfer())
		return false;
	return d->startTransfer();
}

bool QmI2CDevice::startRxTransfer(bool use_pec, uint32_t size) {
	QM_D(QmI2CDevice);
	QM_ASSERT(size > 0);
	if (!d->prepareTransfer())
		return false;
	d->i2c_transfer.use_pec = use_pec;
	d->i2c_transfer.dirs = new hal_i2c_direction_t[size];
	d->i2c_transfer.data = new uint8_t[size];
	for (unsigned int i = 0; i < size; i++)
		d->i2c_transfer.dirs[i] = hi2cDirectionRx;
	d->i2c_transfer.size = size;
	d->transfer_rx_size = size;
	return d->startTransfer();
}

bool QmI2CDevice::startTxTransfer(bool use_pec, uint8_t* data, uint32_t size) {
	QM_D(QmI2CDevice);
	QM_ASSERT(size > 0);
	if (!d->prepareTransfer())
		return false;
	d->i2c_transfer.use_pec = use_pec;
	d->i2c_transfer.dirs = new hal_i2c_direction_t[size];
	d->i2c_transfer.data = new uint8_t[size];
	for (unsigned int i = 0; i < size; i++) {
		d->i2c_transfer.dirs[i] = hi2cDirectionTx;
		d->i2c_transfer.data[i] = data[i];
	}
	d->i2c_transfer.size = size;
	return d->startTransfer();
}

bool QmI2CDevice::startTxRxTransfer(bool use_pec, uint8_t* tx_data, uint32_t tx_size, uint32_t rx_size) {
	QM_D(QmI2CDevice);
	QM_ASSERT((tx_size > 0) && (rx_size > 0));
	if (!d->prepareTransfer())
		return false;
	d->i2c_transfer.use_pec = use_pec;
	d->i2c_transfer.dirs = new hal_i2c_direction_t[tx_size + rx_size];
	d->i2c_transfer.data = new uint8_t[tx_size + rx_size];
	for (unsigned int i = 0; i < tx_size; i++) {
		d->i2c_transfer.dirs[i] = hi2cDirectionTx;
		d->i2c_transfer.data[i] = tx_data[i];
	}
	for (unsigned int i = 0; i < rx_size; i++)
		d->i2c_transfer.dirs[tx_size+i] = hi2cDirectionRx;
	d->i2c_transfer.size = tx_size + rx_size;
	d->transfer_rx_size = rx_size;
	return d->startTransfer();
}

int64_t QmI2CDevice::readRxData(uint8_t* buffer, uint32_t size, int offset) {
	QM_D(QmI2CDevice);
	if (!(!d->transfer_in_progress && (d->io_event.transfer_result == hi2cSuccess) && (d->transfer_rx_size != 0)))
		return -1;
	int64_t read = qmMax(0, qmMin((d->transfer_rx_size - offset), (int)size));
	memcpy(buffer, (d->i2c_transfer.data + (d->i2c_transfer.size - d->transfer_rx_size) + offset), read);
	d->transfer_rx_size = 0;
	return read;
}

bool QmI2CDevice::event(QmEvent* event) {
	QM_D(QmI2CDevice);
	if (event->type() == QmEvent::HardwareIO) {
		d->processEventHardwareIO();
		return true;
	}
	return QmObject::event(event);
}
