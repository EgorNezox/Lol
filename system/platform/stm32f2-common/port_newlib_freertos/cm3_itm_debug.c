/**
  ******************************************************************************
  * @file    cm3_itm_debug.c
  * @author  Artem Pisarenko, PMR dept. software team, ONIIP, PJSC
  * @version V2.0
  * @date    14-March-2012
  * @brief   Cortex-M3 ITM debug support source file.
  *
  * Файл содержит функции для вывода данных из системы с процессором Cortex-M3 в терминал отладчика по интерфейсу SWD.
  * Для передачи используется ITM-модуль подсистемы CoreSight процессора.
  * Поддерживается 32 независимых порта(канала) передачи (обычно нулевой канал используется для printf-функций).
  * Функции передачи данных блокируют вызывающего в случае, если система находится под отладкой и предыдущие данные еще не отправлены.
  * ВНИМАНИЕ. Дополнительно необходима инициализация микроконтроллера для работы трассировочной системы ввода/вывода.
  *
  * Требуется библиотека CMSIS.
  *
  * Тестировалось и поддерживается на компиляторе GNU GCC при использовании адаптера J-Link.
  ******************************************************************************
  */

#ifndef __ARM_ARCH_7M__
#error "ARMv7-M architecture supported only"
#endif

// для некоторых CMSIS необходимы следующие определения (несмотря на то, что в данном случае они не используются)
#ifndef __NVIC_PRIO_BITS
#define __NVIC_PRIO_BITS	0
#endif
// необходимо определить тип IRQn_Type, т.к. он требуется хидеру core_cm3.h
typedef enum IRQn
{
/******  Cortex-M3 Processor Exceptions Numbers ****************************************************************/
  NonMaskableInt_IRQn         = -14,
  MemoryManagement_IRQn       = -12,
  BusFault_IRQn               = -11,
  UsageFault_IRQn             = -10,
  SVCall_IRQn                 = -5,
  DebugMonitor_IRQn           = -4,
  PendSV_IRQn                 = -2,
  SysTick_IRQn                = -1,
} IRQn_Type;

#include <core_cm3.h>

/** @addtogroup Debug instrumentation
  * @{
  */
/** @addtogroup Cortex-M3 ITM debug target sources
  * @{
  */

static inline char check_conditions(uint8_t channel_num) {
	return ((ITM->TCR & ITM_TCR_ITMENA_Msk)              	   &&   /* ITM enabled */
			(ITM->TER & (1ul << channel_num))                  );   /* ITM Port channel_num enabled */
}

/**
  * @brief  Передача байта
  * @param  channel_num: номер канала от 0 до 31
  * @param	data: байт
  * @retval None
  */
void CM3_ITM_SendByte(uint8_t channel_num, uint8_t data) {
	channel_num &= 0x1F;
	if (check_conditions(channel_num)) {
		while (ITM->PORT[channel_num].u32 == 0);
		ITM->PORT[channel_num].u8 = data;
	}
}

/**
  * @brief  Передача полуслова (16 бит)
  * @param  channel_num: номер канала от 0 до 31
  * @param	data: полуслово
  * @retval None
  */
void CM3_ITM_SendHalfWord(uint8_t channel_num, uint16_t data) {
	channel_num &= 0x1F;
	if (check_conditions(channel_num)) {
		while (ITM->PORT[channel_num].u32 == 0);
		ITM->PORT[channel_num].u16 = data;
	}
}

/**
  * @brief  Передача слова (32 бита)
  * @param  channel_num: номер канала от 0 до 31
  * @param	data: слово
  * @retval None
  */
void CM3_ITM_SendWord(uint8_t channel_num, uint32_t data) {
	channel_num &= 0x1F;
	if (check_conditions(channel_num)) {
		while (ITM->PORT[channel_num].u32 == 0);
		ITM->PORT[channel_num].u32 = data;
	}
}

/**
  * @}
  */
/**
  * @}
  */
