/**
  ******************************************************************************
  * @file    hal_uart.h
  * @author  Artem Pisarenko, PMR dept. software team, ONIIP, PJSC
  * @date    28.08.2015
  * @brief   Интерфейс аппаратной абстракции доступа к UART
  *
  ******************************************************************************
  */

#ifndef HAL_UART_H_
#define HAL_UART_H_

#include <stdint.h>
#include <stdbool.h>
#include "FreeRTOS.h"
#include "hal_ringbuffer.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef void* hal_uart_handle_t;

/*! Тип количества стоп-бит в конфигурации UART */
typedef enum {
	huartStopBits_0_5,	/*!< 0.5 бита */
	huartStopBits_1,	/*!< 1 бит */
	huartStopBits_1_5,	/*!< 1.5 бита */
	huartStopBits_2		/*!< 2 бита */
} hal_uart_stop_bits_t;

/*! Тип бита четности UART в конфигурации UART */
typedef enum {
	huartParity_None,	/*!< отсутствует */
	huartParity_Even,	/*!< бит нечетности */
	huartParity_Odd		/*!< бит четности */
} hal_uart_parity_t;

/*! Тип аппаратного управления потоком в конфигурации UART */
typedef enum {
	huartHwFlowControl_None,	/*!< отсутствует */
	huartHwFlowControl_Rx,		/*!< прием (RTS out) */
	huartHwFlowControl_Tx,		/*!< передача (CTS in) */
	huartHwFlowControl_Rx_Tx,	/*!< прием (RTS out) и передача (CTS in) */
} hal_uart_hw_flow_control_t;

/*! Тип параметров для инициализации открываемого UART */
typedef struct {
	uint32_t baud_rate;							/*!< конфигурация: скорость */
	hal_uart_stop_bits_t stop_bits; 			/*!< конфигурация: количество стоп-битов */
	hal_uart_parity_t parity;					/*!< конфигурация: четность */
	hal_uart_hw_flow_control_t hw_flow_control;	/*!< конфигурация: аппаратное управление потоком */
	uint32_t rx_buffer_size;					/*!< (минимальный) размер внутреннего буфера приема в байтах */
	uint32_t tx_buffer_size;					/*!< размер (опционального) внутреннего буфера передачи в байтах */
	unsigned int rx_data_pending_interval;		/*!< мин. интервал (в мс) задержки индикации непрочитанных данных в буфере приема (0 - индицировать немедленно) */
	/*! callback, вызываемый из ISR, индицирующий наличие непрочитанных данных в буфере приема */
	void (*isrcallbackRxDataPending)(hal_uart_handle_t handle, size_t unread_bytes_count, signed portBASE_TYPE *pxHigherPriorityTaskWoken);
	/*! callback, вызываемый из ISR, индицирующий наличие ошибок в принимаемом потоке */
	void (*isrcallbackRxDataErrors)(hal_uart_handle_t handle, size_t error_bytes_count, signed portBASE_TYPE *pxHigherPriorityTaskWoken);
	/*! callback, вызываемый из ISR, индицирующий приостановку приема из-за переполнения буфера */
	void (*isrcallbackRxOverflowSuspended)(hal_uart_handle_t handle, signed portBASE_TYPE *pxHigherPriorityTaskWoken);
	/*! callback, вызываемый из ISR, индицирующий наличие свободного места в буфере передачи */
	void (*isrcallbackTxSpacePending)(hal_uart_handle_t handle, size_t available_bytes_count, signed portBASE_TYPE *pxHigherPriorityTaskWoken);
	/*! callback, вызываемый из ISR, индицирующий завершение асинхронной передачи (из внутреннего буфера, или начатой вызовом hal_uart_start_transmit() */
	void (*isrcallbackTxCompleted)(hal_uart_handle_t handle, signed portBASE_TYPE *pxHigherPriorityTaskWoken);
} hal_uart_params_t;

/*! Инициализирует структуру с параметрами UART значениями по умолчанию
 *
 * \param[in] params		указатель на параметры
 */
void hal_uart_set_default_params(hal_uart_params_t *params);

/*! Открывает (занимает) UART
 *
 * См. предостережение насчет внутренних буферов в описании реализации.
 * Внутренний буфер передачи не создается, если tx_buffer_size = 0.
 * \param[in] hw_instance_number	номер экземпляра аппратного UART в диапазоне [1..6]
 * \param[in] params				указатель на параметры инициализации UART
 * \param[out] rx_buffer			указатель на возвращаемый хэндл внутреннего кольцевого буфера для приема (м/б NULL)
 * \param[out] tx_buffer			указатель на возвращаемый хэндл внутреннего кольцевого буфера для передачи (м/б NULL)
 * \retval handle		хэндл для использования в последующих вызовах (NULL в случае ошибки)
 */
hal_uart_handle_t hal_uart_open(int hw_instance_number, hal_uart_params_t *params, HALRingBuffer_t **rx_buffer, HALRingBuffer_t **tx_buffer);

/*! Включает приемопередачу
 *
 * После включения UART запускается прием и передача из внутреннего буфера (если он создан и не пуст),
 * становятся доступны функции управления приемом и передачей данных.
 * Внимание: внутренний буфер приема опустошается !
 */
void hal_uart_enable(hal_uart_handle_t handle);

/*! Выключает приемопередачу
 *
 * После выключения UART функции возобновления приема и передачи данных недоступны.
 * Оставшиеся во внутреннем буфере приема данные доступны для чтения.
 * Текущая передача данных прерывается и завершается, внутренний буфер передачи очищается.
 */
void hal_uart_disable(hal_uart_handle_t handle);

/*! Возобновляет приостановленный прием
 *
 * Если прием был приостановлен в результате переполнения внутреннего буфера,
 * то данная функция возобновляет его (при наличии места в буфере).
 */
void hal_uart_resume_rx(hal_uart_handle_t handle);

/*! Возобновляет приостановленную передачу
 *
 * Если передача была приостановлена в результате опустошения внутреннего буфера,
 * то данная функция возобновляет его (при наличии данных в буфере).
 */
void hal_uart_resume_tx(hal_uart_handle_t handle);

/*! Читает байт данных из внутреннего буфера приема
 *
 * Не рекомендуется использовать эту функцию совместно с использованием доступа к приемному буферу (возвращенному при открытии UART).
 * \param[out] data	указатель на возвращаемые данные
 * \return true - данные успешно прочитаны (data содержит валидные данные), false - данные непрочитаны (буфер пуст)
 */
bool hal_uart_receive_byte(hal_uart_handle_t handle, uint8_t *data);

/*! Передает внешний буфер данных (синхронная/блокирующая передача)
 *
 * Вызов запускает передачу данных data размером size, ожидает завершения передачи и возвращает результат.
 * \param[in] data			указатель на буфер
 * \param[in] size			размер буфера
 * \param[in] block_time	таймаут всей передачи (если UART настроен на аппаратное управление потоком передачи, то не рекомендуется использовать portMAX_DELAY)
 * \return true - передача успешно выполнена, false - сбой передачи или истек таймаут
 */
bool hal_uart_transmit_blocked(hal_uart_handle_t handle, uint8_t *data, int size, portTickType block_time);

/*! Запускает передачу внешнего буфера данных (асинхронно с завершением)
 *
 * Вызов запускает передачу данных data размером size и возвращает результат запуска.
 * После успешного запуска данные в буфере должны оставаться валидными до окончания передачи.
 * При завершении передачи (а также при выключении приемопередачи) выполняется isrcallbackTxCompleted, после чего буфер свободен.
 * \param[in] data	указатель на буфер
 * \param[in] size	размер буфера
 * \return true - передача успешно запущена, false - передача не запущена (например, приемопередача выключена или предыдущая передача не завершена)
 */
bool hal_uart_start_transmit(hal_uart_handle_t handle, uint8_t *data, int size);

#ifdef __cplusplus
}
#endif

#endif /* HAL_UART_H_ */
