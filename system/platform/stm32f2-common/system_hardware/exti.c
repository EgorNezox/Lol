/**
  ******************************************************************************
  * @file    exti.c
  * @author  Artem Pisarenko, PMR dept. software team, ONIIP, PJSC
  * @date    05.11.2015
  * @brief   Реализация аппаратной абстракции доступа к линиям внешних прерываний на STM32F2xx
  *
  ******************************************************************************
  */

#include "stm32f2xx.h"
#include "FreeRTOS.h"

#include "sys_internal.h"
#include "hal_exti.h"

#define EXTI_LINES_COUNT 23

#define DEFINE_PCB_FROM_HANDLE(var_pcb, handle) struct s_exti_pcb *var_pcb = (struct s_exti_pcb *)handle

struct s_exti_pcb;
typedef struct {
	uint32_t line_reg_mask;
	struct s_exti_pcb *pcb;
	bool mask;
	bool pending;
} s_exti_mux_irq_descr;

struct s_exti_pcb {
	int line;
	bool is_busy;
	void *userid;
	void (*isrcallbackTrigger)(hal_exti_handle_t handle, void *userid, signed portBASE_TYPE *pxHigherPriorityTaskWoken);
} exti_pcbs[EXTI_LINES_COUNT];

static void exti_irq_handler(struct s_exti_pcb *exti);
static void exti_process_mux_interrupts(s_exti_mux_irq_descr descriptors[], int count);

void halinternal_exti_init(void) {
	NVIC_InitTypeDef nvic_init_struct;
	const IRQn_Type nvic_irq_numbers[] = {
			EXTI0_IRQn,
			EXTI1_IRQn,
			EXTI2_IRQn,
			EXTI3_IRQn,
			EXTI4_IRQn,
			EXTI9_5_IRQn,
			EXTI15_10_IRQn,
			PVD_IRQn,
			RTC_Alarm_IRQn,
			OTG_FS_WKUP_IRQn,
			ETH_WKUP_IRQn,
			OTG_HS_WKUP_IRQn,
			TAMP_STAMP_IRQn,
			RTC_WKUP_IRQn
	};
	for (int i = 0; i < sizeof(exti_pcbs)/sizeof(exti_pcbs[0]); i++) {
		exti_pcbs[i].line = i;
		exti_pcbs[i].is_busy = false;
		exti_pcbs[i].userid = 0;
		exti_pcbs[i].isrcallbackTrigger = 0;
	}
	nvic_init_struct.NVIC_IRQChannelPreemptionPriority = SYS_IRQ_CHANNEL_PREEMPTION_PRIORITY;
	nvic_init_struct.NVIC_IRQChannelSubPriority = 0;
	nvic_init_struct.NVIC_IRQChannelCmd = ENABLE;
	for (int i = 0; i < sizeof(nvic_irq_numbers)/sizeof(nvic_irq_numbers[0]); i++) {
		nvic_init_struct.NVIC_IRQChannel = nvic_irq_numbers[i];
		NVIC_Init(&nvic_init_struct);
	}
}

hal_exti_handle_t hal_exti_init(int line, hal_exti_params_t *params) {
	struct s_exti_pcb *exti = &(exti_pcbs[line]);
	EXTI_InitTypeDef init_struct;
	EXTI_StructInit(&init_struct);
	init_struct.EXTI_Line = 1 << exti->line;
	init_struct.EXTI_Mode = EXTI_Mode_Interrupt;
	switch (params->mode) {
	case hextiMode_Rising: init_struct.EXTI_Trigger = EXTI_Trigger_Rising; break;
	case hextiMode_Falling: init_struct.EXTI_Trigger = EXTI_Trigger_Falling; break;
	case hextiMode_Rising_Falling: init_struct.EXTI_Trigger = EXTI_Trigger_Rising_Falling; break;
	}
	init_struct.EXTI_LineCmd = ENABLE;
	portENTER_CRITICAL();
	SYS_ASSERT(exti->is_busy == false);
	exti->is_busy = true;
	exti->userid = params->userid;
	exti->isrcallbackTrigger = params->isrcallbackTrigger;
	EXTI_ClearFlag(1 << exti->line);
	EXTI_Init(&init_struct);
	portEXIT_CRITICAL();
	return (hal_exti_handle_t)exti;
}

void hal_exti_deinit(hal_exti_handle_t handle) {
	DEFINE_PCB_FROM_HANDLE(exti, handle);
	EXTI_InitTypeDef init_struct;
	EXTI_StructInit(&init_struct);
	init_struct.EXTI_Line = 1 << exti->line;
	init_struct.EXTI_LineCmd = DISABLE;
	portENTER_CRITICAL();
	SYS_ASSERT(exti->is_busy == true);
	exti->is_busy = false;
	exti->userid = 0;
	exti->isrcallbackTrigger = 0;
	EXTI_Init(&init_struct);
	EXTI_ClearFlag(1 << exti->line);
	portEXIT_CRITICAL();
}

static void exti_irq_handler(struct s_exti_pcb *exti) {
	portBASE_TYPE xHigherPriorityTaskWoken = pdFALSE;
	if (exti->isrcallbackTrigger)
		exti->isrcallbackTrigger((hal_exti_handle_t)exti, exti->userid, &xHigherPriorityTaskWoken);
	EXTI_ClearFlag(1 << exti->line);
	portEND_SWITCHING_ISR(xHigherPriorityTaskWoken);
}

/* Обработчик прерываний от EXTI[5..15].
 * Программно демультиплексирует совмещенные в один вектор прерывания в отдельные обработчики.
 * Решение максимально близко эмулирует модель прерываний ARMv7-M.
 */
static void exti_process_mux_interrupts(s_exti_mux_irq_descr descriptors[], int count) {
	/* Assign pending statuses (detection) */
	__disable_irq();
	for (int i = 0; i < count; i++) {
		descriptors[i].mask = EXTI->IMR & descriptors[i].line_reg_mask;
		descriptors[i].pending = (descriptors[i].mask && (EXTI->PR & descriptors[i].line_reg_mask));
	}
	__enable_irq();
	/* Enter active states */
	for (int i = 0; i < count; i++)
		if (descriptors[i].pending)
			exti_irq_handler(descriptors[i].pcb);
	/* Check if interrupt masks was disabled, and clear pending bits
	 * (otherwise interrupt will remain pending and detection will ignore it, causing processor being stuck in this handler !)
	 */
	for (int i = 0; i < count; i++)
		if (descriptors[i].mask && !(EXTI->IMR & descriptors[i].line_reg_mask))
			EXTI->PR = descriptors[i].line_reg_mask;
	// End with NVIC hardware behavior...
}

void EXTI0_IRQHandler(void) {
	exti_irq_handler(&exti_pcbs[0]);
}
void EXTI1_IRQHandler(void) {
	exti_irq_handler(&exti_pcbs[1]);
}
void EXTI2_IRQHandler(void) {
	exti_irq_handler(&exti_pcbs[2]);
}
void EXTI3_IRQHandler(void) {
	exti_irq_handler(&exti_pcbs[3]);
}
void EXTI4_IRQHandler(void) {
	exti_irq_handler(&exti_pcbs[4]);
}
void EXTI9_5_IRQHandler(void) {
	static s_exti_mux_irq_descr descriptors[] = {
			{0x00020, &exti_pcbs[5], false, false},
			{0x00040, &exti_pcbs[6], false, false},
			{0x00080, &exti_pcbs[7], false, false},
			{0x00100, &exti_pcbs[8], false, false},
			{0x00200, &exti_pcbs[9], false, false},
	};
	exti_process_mux_interrupts(descriptors, sizeof(descriptors)/sizeof(descriptors[0]));
}
void EXTI15_10_IRQHandler(void) {
	static s_exti_mux_irq_descr descriptors[] = {
			{0x00400, &exti_pcbs[10], false, false},
			{0x00800, &exti_pcbs[11], false, false},
			{0x01000, &exti_pcbs[12], false, false},
			{0x02000, &exti_pcbs[13], false, false},
			{0x04000, &exti_pcbs[14], false, false},
			{0x08000, &exti_pcbs[15], false, false},
	};
	exti_process_mux_interrupts(descriptors, sizeof(descriptors)/sizeof(descriptors[0]));
}
void PVD_IRQHandler(void) {
	exti_irq_handler(&exti_pcbs[16]);
}
void RTC_Alarm_IRQHandler(void) {
	exti_irq_handler(&exti_pcbs[17]);
}
void OTG_FS_WKUP_IRQHandler(void) {
	exti_irq_handler(&exti_pcbs[18]);
}
void ETH_WKUP_IRQHandler(void) {
	exti_irq_handler(&exti_pcbs[19]);
}
void OTG_HS_WKUP_IRQHandler(void) {
	exti_irq_handler(&exti_pcbs[20]);
}
void TAMP_STAMP_IRQHandler(void) {
	exti_irq_handler(&exti_pcbs[21]);
}
void RTC_WKUP_IRQHandler(void) {
	exti_irq_handler(&exti_pcbs[22]);
}
