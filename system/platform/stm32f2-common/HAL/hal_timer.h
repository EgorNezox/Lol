/**
  ******************************************************************************
  * @file    hal_timer.h
  * @author  Artem Pisarenko, PMR dept. software team, ONIIP, PJSC
  * @date    03.09.2015
  * @brief   Интерфейс аппаратной абстракции доступа к таймеру
  *
  * Системный таймер для отсчета небольших периодов (сотни миллисекунд) и формирования задержек
  *
  ******************************************************************************
  */

#ifndef HAL_TIMER_H_
#define HAL_TIMER_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "FreeRTOS.h"
#include "task.h"

typedef struct {
	enum {htimerSysTick, htimerFreeRTOS} type;
	TickType_t timestamp_start, timestamp_end;
	signed char timestamp_ovf;
} hal_timer_t;

/*! Выдерживает задержку в мс */
void hal_timer_delay(unsigned int ms);

/*! Создает таймер с отсчетом и запускает его
 *
 * Используется обязательно в паре с hal_timer_check_expired()
 * \param[in]	ms	значение отсчета в мс (0 - максимальное значение)
 * \return таймер
 */
hal_timer_t hal_timer_start(unsigned int ms);

/*! Возвращает состояние запущенного таймера и прошедшее время
 *
 * Используется обязательно в паре с hal_timer_start()
 * \param[in]	timer		таймер, возвращенный вызовом hal_timer_start()
 * \param[out]	elapsed_ms	прошедшее с момента запуска время (валидно при неистеченном таймере, м/б NULL)
 * \return		0 - таймер не истек (значение elapsed_ms корректное), 1 - таймер истек
 */
char hal_timer_check_expired(hal_timer_t timer, unsigned int *elapsed_ms);

#ifdef __cplusplus
}
#endif
#endif /* HAL_TIMER_H_ */
