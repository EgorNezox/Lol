/**
  ******************************************************************************
  * @file    hal_ringbuffer.h
  * @author  Artem Pisarenko, PMR dept. software team, ONIIP, PJSC
  * @date    29.09.2015
  * @brief   Интерфейс средств работы с коммуникационными кольцевыми буферами данных
  *
  ******************************************************************************
  */

#ifndef HAL_RINGBUFFER_H_
#define HAL_RINGBUFFER_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

/*! Тип управляющей структуры объекта буфера
 *
 * Используется также совместно с объектом буфера для асинхронной работы через функции extctrl_*
 */
typedef struct {
	volatile uint8_t *pData;
	int size, extra_accumulation_size, total_size, read_idx, write_idx, read_msb, write_msb, tail;
	char read_ptr_acquired, write_ptr_acquired;
} HALRingBufferCtrl_t;

/*! Тип объекта буфера, используемый для управления им и синхронизации доступа к данным */
typedef struct {
	HALRingBufferCtrl_t ctrl;
} HALRingBuffer_t;


//////////////////////////////////////////////////////////////////////////////////////////////////////////////
/* Общее управление буфером */

/*! Создает новый объект буфера
 *
 * \param[in] size	минимальный размер буфера
 * \param[in] extra_accumulation_size дополнительный размер накопления
 * \return объект буфера
 */
HALRingBuffer_t* hal_ringbuffer_create(int size, int extra_accumulation_size);

/*! Получает минимальный размер буфера, заданный при создании */
int hal_ringbuffer_get_size(HALRingBuffer_t *buffer);


//////////////////////////////////////////////////////////////////////////////////////////////////////////////
/* Прямой доступ к буферу */

/* Возвращает размер имеющихся данных для чтения в буфере */
int hal_ringbuffer_get_pending_data_size(HALRingBuffer_t *buffer);

/* Возвращает минимально доступный для последующей записи размер пространства в буфере */
int hal_ringbuffer_get_free_space_size(HALRingBuffer_t *buffer);

/* Возвращает состояние переполненности буфера (overflow condition)
 *
 * \return 1 - буфер полностью заполнен (отсутствует пространство для записи), 0 - в буфере имеется пространство для записи
 */
bool hal_ringbuffer_is_full(HALRingBuffer_t *buffer);

/*! Возвращает состояние незаполненности буфера (underrun condition)
 *
 * \return 1 - буфер пуст (отсутствуют данные для чтения), 0 - в буфере имеются данные для чтения
 */
bool hal_ringbuffer_is_empty(HALRingBuffer_t *buffer);

/* Записывает один байт данных в буфер
 *
 * \param[in] data - данные для записи
 * \return true - данные успешно записаны, false - буфер переполнен
 */
bool hal_ringbuffer_write_byte(HALRingBuffer_t *buffer, uint8_t data);

/* Читает один байт данных из буфера
 *
 * \param[out] data - указатель на данные для чтения
 * \return true - данные успешно прочитаны, false - буфер пуст
 */
bool hal_ringbuffer_read_byte(HALRingBuffer_t *buffer, uint8_t *data);

/* Получает указатель и размер следующей доступной для чтения части данных в буфере
 *
 * Возвращенный фрагмент может быть прочитан полностью или частично.
 * Если вызов данной функции вернул ненулевой размер, то необходимо вызвать функцию hal_ringbuffer_read_next()
 * до следующего чтения из буфера и/или вызова hal_ringbuffer_flush_read().
 * \param[out] ptr	указатель на возвращаемый указатель на данные (NULL, если буфер пуст)
 * \param[out] size	указатель на возвращаемые размер данных (0, если буфер пуст)
 */
void hal_ringbuffer_get_read_ptr(HALRingBuffer_t *buffer, uint8_t **ptr, size_t *size);

/* Продвигает чтение буфера в размере фактически прочитанных (потребленных) из него данных
 *
 * Функцию необходимо вызывать после вызова hal_ringbuffer_get_read_ptr(), вернувшего ненулевой размер (даже если ничего не было прочитано),
 * до следующего чтения из буфера и/или вызова hal_ringbuffer_flush_read().
 * Нельзя прочитать больше, чем размер, возвращенный ранее функцией hal_ringbuffer_get_read_ptr().
 * \param[in] consumed	количество прочитанных байт
 */
void hal_ringbuffer_read_next(HALRingBuffer_t *buffer, size_t consumed);

/* Получает указатель и размер следующей доступного для записи пространства данных в буфере
 *
 * Возвращенный фрагмент может быть записан полностью или частично.
 * Если вызов данной функции вернул ненулевой размер, то необходимо вызвать функцию hal_ringbuffer_write_next()
 * до следующей записи в буфер.
 * \param[out] ptr	указатель на возвращаемый указатель на пространство (NULL, если буфер переполнен)
 * \param[out] size	указатель на возвращаемый размер пространства (0, если буфер переполнен)
 */
void hal_ringbuffer_get_write_ptr(HALRingBuffer_t *buffer, uint8_t **ptr, size_t *size);

/* Продвигает запись буфера в размере фактически записанных (произведенных) в него данных
 *
 * Функцию необходимо вызывать после вызова hal_ringbuffer_get_write_ptr(), вернувшего ненулевой размер (даже если ничего не было записано),
 * до следующей записи в буфер.
 * Нельзя записать больше, чем размер, возвращенный ранее функцией hal_ringbuffer_get_write_ptr().
 * \param[in] produced	количество записанных байт
 */
void hal_ringbuffer_write_next(HALRingBuffer_t *buffer, size_t produced);

/* Опустошает буфер для чтения
 *
 * Имеющиеся в буфере данные уничтожаются и буфер становится пустым.
 */
void hal_ringbuffer_flush_read(HALRingBuffer_t *buffer);


//////////////////////////////////////////////////////////////////////////////////////////////////////////////
/* Доступ к буферу через копию управляющей структуры (хранимую в контексте участника доступа).
 * (Обычно применяется совместно с исключением взаимного доступа при чтении/записи из разных контекстов.)
 * Последовательность доступа: get_ctrl(защищенный), доступы через extctrl_* (аналогично прямому доступу), update_read_ctrl/update_write_ctrl(защищенный).
 */
HALRingBufferCtrl_t hal_ringbuffer_get_ctrl(HALRingBuffer_t *buffer);
void hal_ringbuffer_update_read_ctrl(HALRingBuffer_t *buffer, HALRingBufferCtrl_t *ctrl);
void hal_ringbuffer_update_write_ctrl(HALRingBuffer_t *buffer, HALRingBufferCtrl_t *ctrl);
int hal_ringbuffer_extctrl_get_pending_data_size(HALRingBufferCtrl_t *ctrl);
int hal_ringbuffer_extctrl_get_free_space_size(HALRingBufferCtrl_t *ctrl);
bool hal_ringbuffer_extctrl_is_full(HALRingBufferCtrl_t *ctrl);
bool hal_ringbuffer_extctrl_is_empty(HALRingBufferCtrl_t *ctrl);
bool hal_ringbuffer_extctrl_write_byte(HALRingBufferCtrl_t *ctrl, uint8_t data);
bool hal_ringbuffer_extctrl_read_byte(HALRingBufferCtrl_t *ctrl, uint8_t *data);
void hal_ringbuffer_extctrl_get_read_ptr(HALRingBufferCtrl_t *ctrl, uint8_t **ptr, size_t *size);
void hal_ringbuffer_extctrl_read_next(HALRingBufferCtrl_t *ctrl, size_t consumed);
void hal_ringbuffer_extctrl_get_write_ptr(HALRingBufferCtrl_t *ctrl, uint8_t **ptr, size_t *size);
void hal_ringbuffer_extctrl_write_next(HALRingBufferCtrl_t *ctrl, size_t produced);
void hal_ringbuffer_extctrl_flush_read(HALRingBufferCtrl_t *ctrl);

#ifdef __cplusplus
}
#endif
#endif /* HAL_RINGBUFFER_H_ */
