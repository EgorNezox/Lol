/**
  ******************************************************************************
  * @file    qmuart_stm32f2xx.cpp
  * @author  Artem Pisarenko, PMR dept. software team, ONIIP, PJSC
  * @date    26.10.2015
  *
  ******************************************************************************
  */

#include <string.h>
#include "system_hw_io.h"

#include "qmdebug.h"
#include "qmuart.h"
#include "qmuart_p.h"
#include "qmevent.h"
#include "qmapplication.h"

static void qmuartRxDataPendingIsrCallback(hal_uart_handle_t handle, void *userid, size_t unread_bytes_count, signed portBASE_TYPE *pxHigherPriorityTaskWoken) {
	QM_UNUSED(handle);
	QM_UNUSED(unread_bytes_count);
	QmUartIOEvent *system_event = static_cast<QmUartIOEvent *>(userid);
	if (!system_event->rx_data_pending) {
		system_event->rx_data_pending = true;
		system_event->setPendingFromISR(pxHigherPriorityTaskWoken);
	}
}
static void qmuartRxDataErrorsIsrCallback(hal_uart_handle_t handle, void *userid, size_t error_bytes_count, signed portBASE_TYPE *pxHigherPriorityTaskWoken) {
	QM_UNUSED(handle);
	QM_UNUSED(error_bytes_count);
	QmUartIOEvent *system_event = static_cast<QmUartIOEvent *>(userid);
	system_event->rx_data_errors = true;
	system_event->setPendingFromISR(pxHigherPriorityTaskWoken);
}
static void qmuartRxOverflowSuspendedIsrCallback(hal_uart_handle_t handle, void *userid, signed portBASE_TYPE *pxHigherPriorityTaskWoken) {
	QM_UNUSED(handle);
	QmUartIOEvent *system_event = static_cast<QmUartIOEvent *>(userid);
	system_event->rx_overflow_suspended = true;
	system_event->setPendingFromISR(pxHigherPriorityTaskWoken);
}
static void qmuartTxCompletedIsrCallback(hal_uart_handle_t handle, void *userid, signed portBASE_TYPE *pxHigherPriorityTaskWoken) {
	QM_UNUSED(handle);
	QmUartIOEvent *system_event = static_cast<QmUartIOEvent *>(userid);
	if (!system_event->tx_completed) {
		system_event->tx_completed = true;
		system_event->setPendingFromISR(pxHigherPriorityTaskWoken);
	}
}

QmUartIOEvent::QmUartIOEvent(QmUart *o) :
	o(o),
	rx_data_pending(false), rx_data_errors(false), rx_overflow_suspended(false),
	tx_completed(false)
{
}

void QmUartIOEvent::process() {
	QmApplication::postEvent(o, new QmEvent(QmEvent::HardwareIO));
}

QmUartPrivate::QmUartPrivate(QmUart *q) :
	QmObjectPrivate(q),
	hw_resource(-1),
	uart_instance(-1), uart_handle(0), uart_rx_buffer(0), uart_tx_buffer(0), rx_active(false), trigger_event(q)
{
}

QmUartPrivate::~QmUartPrivate()
{
}

void QmUartPrivate::init() {
	uart_instance = stm32f2_get_uart_instance(hw_resource);
	stm32f2_ext_pins_init(hw_resource);
}

void QmUartPrivate::deinit() {
	QM_Q(QmUart);
	q->close();
	stm32f2_ext_pins_deinit(hw_resource);
}

void QmUartPrivate::processEventHardwareIO() {
	QM_Q(QmUart);
	QM_ASSERT(uart_handle != 0);
	if (q->isOpen()) {
		bool rx_data_errors = false;
		bool rx_overflow = false;
		if (trigger_event.rx_data_errors) {
			trigger_event.rx_data_errors = false;
			rx_data_errors = true;
		}
		if (trigger_event.rx_overflow_suspended) {
			trigger_event.rx_overflow_suspended = false;
			if (q->getRxDataAvailable() > 0) {
				rx_active = false;
			} else {
				hal_uart_start_rx(uart_handle);
			}
			rx_overflow = true;
		}
		if (rx_data_errors || rx_overflow)
			q->rxError(rx_data_errors, rx_overflow);
	}
	if (q->isOpen() && (trigger_event.rx_data_pending))
		q->dataReceived();
	if (q->isOpen() && (trigger_event.tx_completed))
		q->dataTransmitted();
}

bool QmUart::isOpen() {
	QM_D(QmUart);
	return (d->uart_handle != 0);
}

bool QmUart::open() {
	QM_D(QmUart);
	hal_uart_params_t params;
	if (isOpen())
		return false;
	hal_uart_set_default_params(&params);
	params.baud_rate = d->config.baud_rate;
	switch (d->config.stop_bits) {
	case StopBits_1: params.stop_bits = huartStopBits_1; break;
	case StopBits_1_5: params.stop_bits = huartStopBits_1_5; break;
	case StopBits_2: params.stop_bits = huartStopBits_2; break;
	}
	switch (d->config.parity) {
	case Parity_None: params.parity = huartParity_None; break;
	case Parity_Even: params.parity = huartParity_Even; break;
	case Parity_Odd: params.parity = huartParity_Odd; break;
	}
	switch (d->config.flow_control) {
	case FlowControl_None: params.hw_flow_control = huartHwFlowControl_None; break;
	case FlowControl_Hardware: params.hw_flow_control = huartHwFlowControl_Rx_Tx; break;
	}
	params.rx_buffer_size = d->config.rx_buffer_size;
	params.tx_buffer_size = d->config.tx_buffer_size;
	params.rx_data_pending_interval = d->config.io_pending_interval;
	params.userid = static_cast<void *>(&d->trigger_event);
	params.isrcallbackRxDataPending = qmuartRxDataPendingIsrCallback;
	params.isrcallbackRxDataErrors = qmuartRxDataErrorsIsrCallback;
	params.isrcallbackRxOverflowSuspended = qmuartRxOverflowSuspendedIsrCallback;
	params.isrcallbackTxCompleted = qmuartTxCompletedIsrCallback;
	d->uart_handle = hal_uart_open(d->uart_instance, &params, &d->uart_rx_buffer, &d->uart_tx_buffer);
	d->rx_active = true;
	return true;
}

bool QmUart::close() {
	QM_D(QmUart);
	if (!isOpen())
		return false;
	hal_uart_close(d->uart_handle);
	d->uart_handle = 0;
	d->uart_rx_buffer = 0;
	d->uart_tx_buffer = 0;
	d->rx_active = false;
	d->trigger_event.rx_data_pending = false;
	d->trigger_event.rx_data_errors = false;
	d->trigger_event.rx_overflow_suspended = false;
	d->trigger_event.tx_completed = false;
	QmApplication::removePostedEvents(this, QmEvent::HardwareIO);
	return true;
}

int64_t QmUart::readData(uint8_t* buffer, uint32_t max_size) {
	QM_D(QmUart);
	if (!isOpen())
		return -1;
	if (max_size == 0)
		return 0;
	int64_t read;
	hal_ringbuffer_ctrl_t buffer_ctrl;
	uint8_t *buffer_ptr;
	size_t buffer_data_size;
	portENTER_CRITICAL();
	d->trigger_event.rx_data_pending = false;
	buffer_ctrl = hal_ringbuffer_get_ctrl(d->uart_rx_buffer);
	portEXIT_CRITICAL();
	hal_ringbuffer_extctrl_get_read_ptr(&buffer_ctrl, &buffer_ptr, &buffer_data_size);
	read = qmMin((size_t)max_size, buffer_data_size);
	if (read > 0) {
		memcpy(buffer, buffer_ptr, read);
		hal_ringbuffer_extctrl_read_next(&buffer_ctrl, read);
	}
	portENTER_CRITICAL();
	hal_ringbuffer_update_read_ctrl(d->uart_rx_buffer, &buffer_ctrl);
	portEXIT_CRITICAL();
	if (!(d->rx_active) && hal_ringbuffer_extctrl_is_empty(&buffer_ctrl)) {
		hal_uart_start_rx(d->uart_handle);
		d->rx_active = true;
	}
	return read;
}

int64_t QmUart::writeData(const uint8_t* data, uint32_t data_size) {
	QM_D(QmUart);
	if (!isOpen())
		return -1;
	if (data_size == 0)
		return 0;
	int64_t written;
	hal_ringbuffer_ctrl_t buffer_ctrl;
	uint8_t *buffer_ptr;
	size_t buffer_space_size;
	portENTER_CRITICAL();
	buffer_ctrl = hal_ringbuffer_get_ctrl(d->uart_tx_buffer);
	portEXIT_CRITICAL();
	hal_ringbuffer_extctrl_get_write_ptr(&buffer_ctrl, &buffer_ptr, &buffer_space_size);
	written = qmMin((size_t)data_size, buffer_space_size);
	if (written > 0) {
		memcpy(buffer_ptr, data, written);
		hal_ringbuffer_extctrl_write_next(&buffer_ctrl, written);
	}
	portENTER_CRITICAL();
	hal_ringbuffer_update_write_ctrl(d->uart_tx_buffer, &buffer_ctrl);
	if (written > 0)
		d->trigger_event.tx_completed = false;
	portEXIT_CRITICAL();
	hal_uart_start_tx(d->uart_handle);
	return written;
}

uint32_t QmUart::getRxDataAvailable() {
	QM_D(QmUart);
	if (!isOpen())
		return 0;
	uint32_t value;
	portENTER_CRITICAL();
	value = hal_ringbuffer_get_pending_data_size(d->uart_rx_buffer);
	portEXIT_CRITICAL();
	return value;
}

uint32_t QmUart::getTxSpaceAvailable() {
	QM_D(QmUart);
	if (!isOpen())
		return 0;
	uint32_t value;
	portENTER_CRITICAL();
	value = hal_ringbuffer_get_free_space_size(d->uart_tx_buffer);
	portEXIT_CRITICAL();
	return value;
}

bool QmUart::event(QmEvent* event) {
	QM_D(QmUart);
	if (event->type() == QmEvent::HardwareIO) {
		d->processEventHardwareIO();
		return true;
	}
	return QmObject::event(event);
}
