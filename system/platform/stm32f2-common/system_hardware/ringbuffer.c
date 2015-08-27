/**
  ******************************************************************************
  * @file    ringbuffer.c
  * @author  Artem Pisarenko, PMR dept. software team, ONIIP, PJSC
  * @date    29.09.2015
  * @brief   Реализация средств работы с коммуникационными кольцевыми буферами данных
  *
  * Расширенный кольцевой буфер с встроенным механизмом экстра-накопительного пространства для применения в коммуникации (передаче данных).
  * Может использоваться как обычный классический кольцевой буфер, если при создании объекта указать extra_accumulation_size = 0.
  *
  * Объект буфера создается функцией hal_ringbuffer_create и состоит из:
  *  - управляющей структуры с параметрами и индексами (указателями);
  *  - указателя на начало фактического буфера данных.
  * Параметр size определяет минимальный размер буферизации между производителем данных (producer) и потребителем данных (consumer).
  * Параметр extra_accumulation_size позволяет реализовать сценарий буферизации, где производителем данных является "капризный" аппаратный DMA.
  * (В буфере выделяется дополнительное пространство и поддержание двойной непрерывной области памяти для приема extra_accumulation_size байт
  *  при наличии места.)
  * Таким образом, extra_accumulation_size должен быть как можно меньше, чтобы эффективнее использовать доступную память.
  * Фактический размер данных равен: MAX(size, extra_accumulation_size) + extra_accumulation_size.
  *
  * Функции hal_ringbuffer_* не являются потокобезопасными (но являются реентерабельными).
  * Интерфейс позволяет организовавать гибкие сценарии буферизации,
  *  где производитель и потребитель данных могут записывать/читать данные из разных контекстов ОС (в т.ч. из прерываний).
  * Производитель и потребитель используют hal_ringbuffer_get_ctrl (синхронизируя доступ) для получения копии управляющей структуры
  *  ("замороженного состояния" буфера) и последующей асинхронной работы
  *  с функциями hal_ringbuffer_extctrl_* (симметричный функционал вариантам без "extctrl") с прямым доступом к данным.
  * Обновление сделанных асинхронно изменений производится функциями
  * hal_ringbuffer_update_write_ctrl (производителем)
  * и hal_ringbuffer_update_read_ctrl (потребителем)
  * с синхронизацией доступа.
  * Под синхронизацией доступа подразумевается вызов функций с использованием мьютекса (для исключения доступа из разных потоков ОС)
  * или в критической секции (для исключения доступа между потоками ОС и обработчиками прерываний).
  * Следует иметь также в виду, что копия буфера между явным получением и обновлением не обновляется другим участником,
  *  даже если тот парраллельно записывает или читает новые данные, поэтому обработка каждой копии буфера должна согласовываться
  *  с размером буфера, производительностью системы и интенсивности записи/чтения данных.
  *
  * Запись/чтение данных может осуществляться как побайтно, так и частями. При записи контролируется переполнение буфера.
  * Работа с частями осуществляется следующим образом:
  * - производитель использует попарно вызовы hal_ringbuffer_*_get_write_ptr (для получения указателя и размера следующей доступной для записи части)
  *   и hal_ringbuffer_*_write_next (для продвижения записи с фактически записанным размером);
  * - потребитель использует попарно вызовы hal_ringbuffer_*_get_read_ptr (для получения указателя и размера следующей доступной для чтения части)
  *   и hal_ringbuffer_*_read_next (для продвижения чтения с фактически прочитанным размером), и/или очищает буфер вызовами hal_ringbuffer_*_flush_read.
  *
  ******************************************************************************
  */

#include <stdlib.h>
#include <string.h>
#include "sys_internal.h"
#include "hal_ringbuffer.h"

#ifndef max
#define max(a,b)	(((a) > (b)) ? (a) : (b))
#endif

static void align_next_write(HALRingBufferCtrl_t *ctrl);
static void align_next_read(HALRingBufferCtrl_t *ctrl);

HALRingBuffer_t* hal_ringbuffer_create(int size, int extra_accumulation_size) {
	HALRingBuffer_t *buffer = malloc(sizeof(HALRingBuffer_t));
	if (buffer) {
		buffer->ctrl.read_idx = 0;
		buffer->ctrl.write_idx = 0;
		buffer->ctrl.read_msb = 0;
		buffer->ctrl.write_msb = 0;
		buffer->ctrl.read_ptr_acquired = false;
		buffer->ctrl.write_ptr_acquired = false;
		buffer->ctrl.size = size;
		buffer->ctrl.extra_accumulation_size = extra_accumulation_size;
		buffer->ctrl.tail = max(size, extra_accumulation_size);
		buffer->ctrl.total_size = max(size, extra_accumulation_size) + extra_accumulation_size;
		buffer->ctrl.pData = malloc(buffer->ctrl.total_size);
		if (buffer->ctrl.pData == NULL) {
			SYS_ASSERT(0);
			free(buffer);
			buffer = NULL;
		}
	} else {
		SYS_ASSERT(0);
	}
	return buffer;
}

int hal_ringbuffer_get_size(HALRingBuffer_t *buffer) {
	return buffer->ctrl.size;
}

int hal_ringbuffer_get_pending_data_size(HALRingBuffer_t *buffer) {
	return hal_ringbuffer_extctrl_get_pending_data_size(&(buffer->ctrl));
}

int hal_ringbuffer_extctrl_get_pending_data_size(HALRingBufferCtrl_t *ctrl) {
	if (hal_ringbuffer_extctrl_is_empty(ctrl))
		return 0;
	return (ctrl->write_idx - ctrl->read_idx + ((int)(ctrl->write_idx <= ctrl->read_idx))*ctrl->tail);
}

int hal_ringbuffer_get_free_space_size(HALRingBuffer_t *buffer) {
	return hal_ringbuffer_extctrl_get_free_space_size(&(buffer->ctrl));
}

int hal_ringbuffer_extctrl_get_free_space_size(HALRingBufferCtrl_t *ctrl) {
	int size;
	if (hal_ringbuffer_extctrl_is_full(ctrl))
		return 0;
	if (ctrl->write_idx < ctrl->read_idx) {
		size = ctrl->read_idx - ctrl->write_idx;
	} else {
		int wrap_threshold = max(ctrl->size, ctrl->extra_accumulation_size);
		if (ctrl->write_idx < wrap_threshold)
			size = wrap_threshold - ctrl->write_idx + max(ctrl->extra_accumulation_size, ctrl->read_idx);
		else
			size = max(ctrl->total_size - ctrl->write_idx, ctrl->read_idx);
	}
	return size;
}

bool hal_ringbuffer_is_full(HALRingBuffer_t *buffer) {
	return hal_ringbuffer_extctrl_is_full(&(buffer->ctrl));
}

bool hal_ringbuffer_extctrl_is_full(HALRingBufferCtrl_t *ctrl) {
	return (ctrl->write_idx == ctrl->read_idx && ctrl->write_msb != ctrl->read_msb);
}

bool hal_ringbuffer_is_empty(HALRingBuffer_t *buffer) {
	return hal_ringbuffer_extctrl_is_empty(&(buffer->ctrl));
}

bool hal_ringbuffer_extctrl_is_empty(HALRingBufferCtrl_t *ctrl) {
	return (ctrl->write_idx == ctrl->read_idx && ctrl->write_msb == ctrl->read_msb);
}

bool hal_ringbuffer_write_byte(HALRingBuffer_t *buffer, uint8_t data) {
	return hal_ringbuffer_extctrl_write_byte(&(buffer->ctrl), data);
}

bool hal_ringbuffer_extctrl_write_byte(HALRingBufferCtrl_t *ctrl, uint8_t data) {
	SYS_ASSERT(ctrl->write_ptr_acquired == false);
	if (hal_ringbuffer_extctrl_is_full(ctrl))
		return false;
	ctrl->pData[ctrl->write_idx++] = data;
	align_next_write(ctrl);
	return true;
}

bool hal_ringbuffer_read_byte(HALRingBuffer_t *buffer, uint8_t *data) {
	return hal_ringbuffer_extctrl_read_byte(&(buffer->ctrl), data);
}

bool hal_ringbuffer_extctrl_read_byte(HALRingBufferCtrl_t *ctrl, uint8_t *data) {
	SYS_ASSERT(ctrl->read_ptr_acquired == false);
	if (hal_ringbuffer_extctrl_is_empty(ctrl))
		return false;
	*data = ctrl->pData[ctrl->read_idx++];
	align_next_read(ctrl);
	return true;
}

void hal_ringbuffer_get_read_ptr(HALRingBuffer_t *buffer, uint8_t **ptr, size_t *size) {
	hal_ringbuffer_extctrl_get_read_ptr(&(buffer->ctrl), ptr, size);
}

void hal_ringbuffer_extctrl_get_read_ptr(HALRingBufferCtrl_t *ctrl, uint8_t **ptr, size_t *size) {
	SYS_ASSERT(ctrl->read_ptr_acquired == false);
	if (hal_ringbuffer_extctrl_is_empty(ctrl)) {
		*ptr = NULL;
		*size = 0;
		return;
	}
	*ptr = (uint8_t *)(ctrl->pData) + ctrl->read_idx;
	if (ctrl->write_idx > ctrl->read_idx) {
		*size = ctrl->write_idx - ctrl->read_idx;
	} else {
		*size = ctrl->tail - ctrl->read_idx;
	}
	ctrl->read_ptr_acquired = true;
}

void hal_ringbuffer_read_next(HALRingBuffer_t *buffer, size_t consumed) {
	hal_ringbuffer_extctrl_read_next(&(buffer->ctrl), consumed);
}

void hal_ringbuffer_extctrl_read_next(HALRingBufferCtrl_t *ctrl, size_t consumed) {
	SYS_ASSERT(ctrl->read_ptr_acquired == true);
	ctrl->read_ptr_acquired = false;
	if (consumed == 0)
		return;
	if (ctrl->write_idx > ctrl->read_idx) {
		SYS_ASSERT(consumed <= (ctrl->write_idx - ctrl->read_idx));
	} else {
		SYS_ASSERT((consumed <= (ctrl->tail - ctrl->read_idx)) && (ctrl->write_msb != ctrl->read_msb));
	}
	ctrl->read_idx += consumed;
	align_next_read(ctrl);
}

static void align_next_read(HALRingBufferCtrl_t *ctrl) {
	if ((ctrl->read_idx > ctrl->write_idx) && (ctrl->read_idx == ctrl->tail)) {
		ctrl->read_msb ^= 1;
		ctrl->read_idx = 0;
	}
}

void hal_ringbuffer_get_write_ptr(HALRingBuffer_t *buffer, uint8_t **ptr, size_t *size) {
	hal_ringbuffer_extctrl_get_write_ptr(&(buffer->ctrl), ptr, size);
}

void hal_ringbuffer_extctrl_get_write_ptr(HALRingBufferCtrl_t *ctrl, uint8_t **ptr, size_t *size) {
	SYS_ASSERT(ctrl->write_ptr_acquired == false);
	if (hal_ringbuffer_extctrl_is_full(ctrl)) {
		*ptr = NULL;
		*size = 0;
		return;
	}
	*ptr = (uint8_t *)(ctrl->pData) + ctrl->write_idx;
	if (ctrl->read_idx > ctrl->write_idx) {
		*size = ctrl->read_idx - ctrl->write_idx;
	} else {
		*size = ctrl->total_size - ctrl->write_idx;
	}
	ctrl->write_ptr_acquired = true;
}

void hal_ringbuffer_write_next(HALRingBuffer_t *buffer, size_t produced) {
	hal_ringbuffer_extctrl_write_next(&(buffer->ctrl), produced);
}

void hal_ringbuffer_extctrl_write_next(HALRingBufferCtrl_t *ctrl, size_t produced) {
	SYS_ASSERT(ctrl->write_ptr_acquired == true);
	ctrl->write_ptr_acquired = false;
	if (produced == 0)
		return;
	if (ctrl->read_idx > ctrl->write_idx) {
		SYS_ASSERT(produced <= (ctrl->read_idx - ctrl->write_idx));
	} else {
		SYS_ASSERT((produced <= (ctrl->total_size - ctrl->write_idx)) && (ctrl->write_msb == ctrl->read_msb));
	}
	ctrl->write_idx += produced;
	align_next_write(ctrl);
}

static void align_next_write(HALRingBufferCtrl_t *ctrl) {
	if ((ctrl->write_idx > ctrl->read_idx) && (ctrl->write_idx >= max(ctrl->size, ctrl->extra_accumulation_size))) {
		ctrl->tail = ctrl->write_idx;
		int next_chunk_size = ctrl->total_size - ctrl->write_idx;
		if ((next_chunk_size == 0) || (ctrl->read_idx > next_chunk_size)) {
			ctrl->write_msb ^= 1;
			ctrl->write_idx = 0;
		}
	}
}

void hal_ringbuffer_flush_read(HALRingBuffer_t *buffer) {
	hal_ringbuffer_extctrl_flush_read(&(buffer->ctrl));
}

void hal_ringbuffer_extctrl_flush_read(HALRingBufferCtrl_t *ctrl) {
	SYS_ASSERT(ctrl->read_ptr_acquired == false);
	ctrl->read_idx = ctrl->write_idx;
	ctrl->read_msb = ctrl->write_msb;
}

HALRingBufferCtrl_t hal_ringbuffer_get_ctrl(HALRingBuffer_t *buffer) {
	return buffer->ctrl;
}

void hal_ringbuffer_update_read_ctrl(HALRingBuffer_t *buffer, HALRingBufferCtrl_t *ctrl) {
	buffer->ctrl.read_ptr_acquired = ctrl->read_ptr_acquired;
	buffer->ctrl.read_idx = ctrl->read_idx;
	buffer->ctrl.read_msb = ctrl->read_msb;
}

void hal_ringbuffer_update_write_ctrl(HALRingBuffer_t *buffer, HALRingBufferCtrl_t *ctrl) {
	buffer->ctrl.write_ptr_acquired = ctrl->write_ptr_acquired;
	buffer->ctrl.write_idx = ctrl->write_idx;
	buffer->ctrl.write_msb = ctrl->write_msb;
	buffer->ctrl.tail = ctrl->tail;
}
