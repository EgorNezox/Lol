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
	void *userid;								/*!< (опциональный) пользовательский идентификатор */
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

/*! Открывает (занимает) UART и включает приемопередачу
 *
 * Запускается прием во внутренний буфер.
 * Становятся доступны функции управления приемом и передачей данных.
 * См. предостережение насчет внутренних буферов в описании реализации.
 * (Внутренний буфер передачи не создается, если tx_buffer_size = 0.)
 * \param[in] instance		номер экземпляра аппратного UART в диапазоне [1..6]
 * \param[in] params		указатель на параметры инициализации UART
 * \param[out] rx_buffer	указатель на возвращаемый хэндл внутреннего кольцевого буфера для приема (м/б NULL)
 * \param[out] tx_buffer	указатель на возвращаемый хэндл внутреннего кольцевого буфера для передачи (м/б NULL)
 * \retval handle		хэндл для использования в последующих вызовах (NULL в случае ошибки)
 */
hal_uart_handle_t hal_uart_open(int instance, hal_uart_params_t *params, hal_ringbuffer_t **rx_buffer, hal_ringbuffer_t **tx_buffer);

/*! Закрывает (освобождает) UART и выключает приемопередачу
 *
 * Текущий прием и передача данных прерывается.
 * Функции управления приемом и передачей данных становятся недоступны.
 * Хэндлы внутренних кольцевых буферов становятся недействительными.
 */
void hal_uart_close(hal_uart_handle_t handle);

/*! Возвращает значение пользовательского идентификатора UART
 *
 * \return значение поля userid параметров инициализации
 */
void* hal_uart_get_userid(hal_uart_handle_t handle);

/*! Запускает прием во внутренний буфер приема
 *
 * Если прием был приостановлен в результате переполнения буфера,
 * то данная функция возобновляет его.
 * Буфер приема опустошается.
 * Внимание! 	Внутренний буфер приема сбрасывается (это необходимо учитывать,
 * 				если доступ к буферу осуществляется напрямую, см. hal_ringbuffer_reset()).
 * \return true - прием запущен, false - отказ
 */
bool hal_uart_start_rx(hal_uart_handle_t handle);

/*! Запускает передачу из внутреннего буфера передачи
 *
 * Если передача была приостановлена в результате опустошения буфера,
 * то данная функция возобновляет его (при наличии данных в буфере).
 * \return true - передача запущена, false - отказ
 */
bool hal_uart_start_tx(hal_uart_handle_t handle);

/*! Читает данные из внутреннего буфера приема
 *
 * Не рекомендуется использовать эту функцию совместно с использованием прямого доступа к приемному буферу.
 * \param[out] buffer	указатель на буфер для извлекаемых данных
 * \param[in]  max_size	максимальный размер извлекаемых данных
 * \return размер фактически прочитанных данных в buffer (отрицательное значение означает ошибку)
 */
int hal_uart_receive(hal_uart_handle_t handle, uint8_t *buffer, int max_size);

/*! Передает внешний буфер данных (синхронная/блокирующая передача)
 *
 * Вызов запускает передачу данных data размером size, ожидает завершения передачи и возвращает результат.
 * \param[in] data			указатель на буфер
 * \param[in] size			размер буфера
 * \param[in] block_time	таймаут всей передачи (если UART настроен на аппаратное управление потоком передачи, то не рекомендуется использовать portMAX_DELAY)
 * \return true - передача успешно выполнена, false - сбой передачи или истек таймаут
 */
bool hal_uart_transmit_blocked(hal_uart_handle_t handle, uint8_t *data, int size, TickType_t block_time);

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
