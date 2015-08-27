/**
  ******************************************************************************
  * @file    uart.c
  * @author  Artem Pisarenko, PMR dept. software team, ONIIP, PJSC
  * @date    28.08.2015
  * @brief   Реализация аппаратной абстракции доступа к UART на STM32F2xx
  *
  * UART работает в полнодуплексном режиме.
  * Отсутствует защита от исключения взаимного доступа (ответственность многопоточного приложения).
  *
  * Внимание! Доступ к внутренним буферам может осуществляться только из критической секции (при доступе из задачи FreeRTOS)
  * или из прерывания с приоритетом не выше irq_channel_preemption_priority.
  * (См. порядок работы и синхронизацию доступа в документации hal_ringbuffer.)
  * Callback'и c префиксом isr означают, что они выполняются из контекста прерывания. Все вызываемые из них функции FreeRTOS должны использовать аргумент pxHigherPriorityTaskWoken.
  * Все callback'и должны выполняться очень быстро и не содержать блокирующих вызовов
  * (правильная реализация должна выполнять в них только установку флагов и неблокирующие вызовы функций синхронизации FreeRTOS).
  *
  * Прием(Rx):
  * Работает через DMA с использованием коммуникационного кольцевого буфера.
  * Размер буфера расширяется (наиболее оптимальным способом) с учетом макс. задержки обработки прерываний заданного приоритета в системе,
  * значения мин. интервала задержки непрочитанных данных, конфигурации аппаратного управления потоком и скорости UART.
  * (См. подробности в функции hal_uart_open()).
  * Обработка приема и переполнения буфера (уведомление приложения о состоянии приема) ведется по прерываниям от USART.
  * В связи с неполноценной реализацией DMA-контроллера в серии STM32F2 (в отличие от STM32F1) прием ведется фрагментами с
  * приостановкой DMA-потока в прерывании для того, чтобы считывать фактический размер уже принятых данных.
  * Приложение может принимать данные посимвольно (путем вызова соответствующей функции)
  * или напрямую читать из приемного буфера, возвращаенного при открытии UART, выполняя роль потребителя данных.
  * Оптимизировать обработку приема можно путем задания ненулевого интервала задержки индикации непрочитанных данных в буфере.
  * Размер буфера приема должен задаваться с учетом этой задержки и скорости uart.
  * isrcallbackRxDataPending: аргумент unread_bytes_count указывает размер непрочитанных на момент вызова данных в буфере.
  * isrcallbackRxDataErrors: аргумент error_bytes_count указывает размер данных, содержащихся в начале непрочитанных на момент вызова данных в буфере,
  *  корректность которых не гарантируется.
  * isrcallbackRxOverflowSuspended: на момент вызова в буфере отсутствует свободное место,
  *  новые данные не принимаются и ошибки не обнаруживаются до тех пор пока прием не будет возобновлен вызовом hal_uart_resume_rx.
  *
  * Передача(Tx):
  * Работает через прерывания USART (посимвольно) с использованием буфера данных.
  * Приложению доступны три способа:
  * - асинхронно из внутреннего кольцевого буфера передачи, выполняя роль производителя данных
  *   (запускается при включении, приостанавливается при завершении отправки последнего байта из буфера, возобновляется вызовами hal_uart_resume_tx())
  *   (с использованием callback-функций isrcallbackTxSpacePending, isrcallbackTxCompleted)
  * - асинхронно из выделяемого приложением буфера данных
  *   (с использованием callback-функции isrcallbackTxCompleted)
  * - блокирующий, из выделяемого приложением буфера данных.
  * isrcallbackTxSpacePending: вызывается только при использовании передачи из внутреннего буфера,
  *                            аргумент available_bytes_count указывает размер свободного места в буфере на момент вызова
  * isrcallbackTxCompleted: вызывается только при ииспользовании асинхронной передачи и обозначает ее завершение (не обязательно успешное)
  *
  * Заняты (недоступны для использования в системе) следующие аппаратные ресурсы:
  * - все UART и USART
  * - DMA1 Stream 0
  * - DMA1 Stream 1
  * - DMA1 Stream 2
  * - DMA1 Stream 5
  * - DMA2 Stream 1
  * - DMA2 Stream 2
  *
  ******************************************************************************
  */

#include <stdint.h>
#include <stdbool.h>
#include <math.h>
#include "stm32f2xx.h"
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "timers.h"

#include "sys_internal.h"
#include "hal_uart.h"

#ifndef max
#define max(a,b)	(((a) > (b)) ? (a) : (b))
#endif
#ifndef min
#define min(a,b)	(((a) < (b)) ? (a) : (b))
#endif

#define MAX_DMA_TRANSFER_SIZE	0xFFFF // see STM32F2 reference manual

#define DEFINE_PCB_FROM_HANDLE(var_pcb, handle) struct s_uart_pcb *var_pcb = (struct s_uart_pcb *)handle

struct s_uart_pcb {
	USART_TypeDef* usart;
	IRQn_Type usart_irq_n;
	uint8_t usart_apb;
	uint32_t usart_rcc_periph;
	DMA_TypeDef* dma_rx;
	uint8_t dma_rx_stream_number;
	DMA_Stream_TypeDef* dma_rx_stream;
	IRQn_Type dma_rx_irq_n;
	uint32_t dma_rx_stream_rcc_periph;
	uint32_t dma_rx_channel;
	uint32_t dma_rx_it_teif;
	uint32_t dma_rx_it_flags;
	uint32_t dma_rx_it_raw_ifcr_flags;
	bool is_open;
	HALRingBuffer_t *rx_buffer;
	xTimerHandle rx_data_timer; // (optional) timer used to defer rx unread data indication (data "flush")
	bool rx_data_timer_expired; // (optional) flag indicating rx_data_timer expired
	void (*isrcallbackRxDataPending)(hal_uart_handle_t handle, size_t unread_bytes_count, signed portBASE_TYPE *pxHigherPriorityTaskWoken);
	void (*isrcallbackRxDataErrors)(hal_uart_handle_t handle, size_t error_bytes_count, signed portBASE_TYPE *pxHigherPriorityTaskWoken);
	void (*isrcallbackRxOverflowSuspended)(hal_uart_handle_t handle, signed portBASE_TYPE *pxHigherPriorityTaskWoken);
	void (*isrcallbackTxSpacePending)(hal_uart_handle_t handle, size_t available_bytes_count, signed portBASE_TYPE *pxHigherPriorityTaskWoken);
	void (*isrcallbackTxCompleted)(hal_uart_handle_t handle, signed portBASE_TYPE *pxHigherPriorityTaskWoken);
	size_t rx_active_transfer_size; // size of last scheduled DMA rx transfer (zero means DMA transfer isn't running)
	HALRingBuffer_t *tx_buffer;
	enum {
		txtransferNone, // no transmission active
		txtransferActiveInternal, // asynchronous transmission from internal tx_buffer is active (uses isrcallbackTxSpacePending, isrcallbackTxCompleted)
		txtransferActiveExternalAsync, // asynchronous transmission of external data is active (uses isrcallbackTxCompleted)
		txtransferActiveExternalSync, // synchronous transmission of external data is active (uses smphr_tx_complete)
	} tx_transfer;
	uint8_t *tx_transfer_data_ptr; // incrementing pointer to external data buffer (active transmission)
	size_t tx_transfer_pending_size; // remaining size of data from external buffer (active transmission)
	xSemaphoreHandle smphr_tx_complete; // used for transmission completition synchronization
} uart_pcbs[6] = {
		{USART1, USART1_IRQn, 2, RCC_APB2Periph_USART1, DMA2, 2, DMA2_Stream2, DMA2_Stream2_IRQn, RCC_AHB1Periph_DMA2, DMA_Channel_4, DMA_IT_TEIF2, (DMA_FLAG_TCIF2 | DMA_FLAG_HTIF2), (DMA_LIFCR_CTCIF2 | DMA_LIFCR_CHTIF2), false, NULL, NULL, false, NULL, NULL, NULL, NULL, NULL, 0, NULL, txtransferNone, NULL, 0, NULL},
		{USART2, USART2_IRQn, 1, RCC_APB1Periph_USART2, DMA1, 5, DMA1_Stream5, DMA1_Stream5_IRQn, RCC_AHB1Periph_DMA1, DMA_Channel_4, DMA_IT_TEIF5, (DMA_FLAG_TCIF5 | DMA_FLAG_HTIF5), (DMA_HIFCR_CTCIF5 | DMA_HIFCR_CHTIF5), false, NULL, NULL, false, NULL, NULL, NULL, NULL, NULL, 0, NULL, txtransferNone, NULL, 0, NULL},
		{USART3, USART3_IRQn, 1, RCC_APB1Periph_USART3, DMA1, 1, DMA1_Stream1, DMA1_Stream1_IRQn, RCC_AHB1Periph_DMA1, DMA_Channel_4, DMA_IT_TEIF1, (DMA_FLAG_TCIF1 | DMA_FLAG_HTIF1), (DMA_LIFCR_CTCIF1 | DMA_LIFCR_CHTIF1), false, NULL, NULL, false, NULL, NULL, NULL, NULL, NULL, 0, NULL, txtransferNone, NULL, 0, NULL},
		{UART4, UART4_IRQn, 1, RCC_APB1Periph_UART4, DMA1, 2, DMA1_Stream2, DMA1_Stream2_IRQn, RCC_AHB1Periph_DMA1, DMA_Channel_4, DMA_IT_TEIF2, (DMA_FLAG_TCIF2 | DMA_FLAG_HTIF2), (DMA_LIFCR_CTCIF2 | DMA_LIFCR_CHTIF2), false, NULL, NULL, false, NULL, NULL, NULL, NULL, NULL, 0, NULL, txtransferNone, NULL, 0, NULL},
		{UART5, UART5_IRQn, 1, RCC_APB1Periph_UART5, DMA1, 0, DMA1_Stream0, DMA1_Stream0_IRQn, RCC_AHB1Periph_DMA1, DMA_Channel_4, DMA_IT_TEIF0, (DMA_FLAG_TCIF0 | DMA_FLAG_HTIF0), (DMA_LIFCR_CTCIF0 | DMA_LIFCR_CHTIF0), false, NULL, NULL, false, NULL, NULL, NULL, NULL, NULL, 0, NULL, txtransferNone, NULL, 0, NULL},
		{USART6, USART6_IRQn, 2, RCC_APB2Periph_USART6, DMA2, 1, DMA2_Stream1, DMA2_Stream1_IRQn, RCC_AHB1Periph_DMA2, DMA_Channel_5, DMA_IT_TEIF1, (DMA_FLAG_TCIF1 | DMA_FLAG_HTIF1), (DMA_LIFCR_CTCIF1 | DMA_LIFCR_CHTIF1), false, NULL, NULL, false, NULL, NULL, NULL, NULL, NULL, 0, NULL, txtransferNone, NULL, 0, NULL},
};

static double get_character_rate_from_baud_rate(uint32_t rate, hal_uart_stop_bits_t stop_bits, hal_uart_parity_t parity);
static void set_usart_rx_interrupts(struct s_uart_pcb *uart, FunctionalState state);
static void set_usart_tx_interrupts(struct s_uart_pcb *uart, FunctionalState state);
static void start_dma_rx(struct s_uart_pcb *uart);
static void stop_dma_rx(struct s_uart_pcb *uart);
static bool update_rx_data_indication_from_isr(struct s_uart_pcb *uart, size_t bytes_transfered, bool rx_active, signed portBASE_TYPE *pxHigherPriorityTaskWoken);
static void vTimerRxDataCallback(xTimerHandle pxTimer);
static void usart_irq_handler(struct s_uart_pcb *uart);
static void usart_irq_handler_rx(struct s_uart_pcb *uart, uint16_t USART_SR_value, signed portBASE_TYPE *pxHigherPriorityTaskWoken);
static int get_max_dma_rx_pause_length(void);
static void usart_irq_handler_tx(struct s_uart_pcb *uart, uint16_t USART_SR_value, signed portBASE_TYPE *pxHigherPriorityTaskWoken);
static void dma_rx_irq_handler(struct s_uart_pcb *uart);

void halinternal_uart_init(void) {
	for (int i = 0; i < sizeof(uart_pcbs)/sizeof(uart_pcbs[0]); i++)
		uart_pcbs[i].smphr_tx_complete = xSemaphoreCreateCounting(1, 0);
}

void hal_uart_set_default_params(hal_uart_params_t *params) {
	params->baud_rate = 115200;
	params->stop_bits = huartStopBits_1;
	params->parity = huartParity_None;
	params->hw_flow_control = huartHwFlowControl_None;
	params->rx_buffer_size = 0;
	params->tx_buffer_size = 0;
	params->rx_data_pending_interval = 0;
	params->isrcallbackRxDataPending = NULL;
	params->isrcallbackRxDataErrors = NULL;
	params->isrcallbackRxOverflowSuspended = NULL;
	params->isrcallbackTxSpacePending = NULL;
	params->isrcallbackTxCompleted = NULL;
}

hal_uart_handle_t hal_uart_open(int hw_instance_number, hal_uart_params_t *params, HALRingBuffer_t **rx_buffer, HALRingBuffer_t **tx_buffer) {
	struct s_uart_pcb *uart = &(uart_pcbs[hw_instance_number-1]);
	SYS_ASSERT((1 <= hw_instance_number) && (hw_instance_number <= sizeof(uart_pcbs)/sizeof(uart_pcbs[0])));
	SYS_ASSERT(params);
	SYS_ASSERT(params->isrcallbackRxOverflowSuspended);
	SYS_ASSERT(params->rx_buffer_size > 0);
	SYS_ASSERT(uart->is_open == false);
	SYS_ASSERT(xTaskGetSchedulerState() != taskSCHEDULER_RUNNING);

	int max_irq_service_delay = 0;
	if (!((params->hw_flow_control == huartHwFlowControl_Rx) || (params->hw_flow_control == huartHwFlowControl_Rx_Tx)))
		max_irq_service_delay = SYS_MAX_IRQ_LATENCY_MS;
	double character_rate = get_character_rate_from_baud_rate(params->baud_rate, params->stop_bits, params->parity);

	RCC_ClocksTypeDef current_rcc_clocks;
	RCC_GetClocksFreq(&current_rcc_clocks);
	SYS_ASSERT(character_rate < ((double)(current_rcc_clocks.HCLK_Frequency)/((double)get_max_dma_rx_pause_length()))); // incompatible uart baudrate and system bus clock

	int extra_rx_accumulation_size = ceil(character_rate*((double)(max(max_irq_service_delay, (params->rx_data_pending_interval + 1)))/1000));
	SYS_ASSERT(extra_rx_accumulation_size <= MAX_DMA_TRANSFER_SIZE); // incompatible uart baudrate and (system characteristics and/or uart parameters)

	uart->rx_buffer = hal_ringbuffer_create(params->rx_buffer_size, extra_rx_accumulation_size);
	SYS_ASSERT(uart->rx_buffer);
	if (params->rx_data_pending_interval > 0)
		uart->rx_data_timer = xTimerCreate("HAL_UART_rx_data_X", (params->rx_data_pending_interval/portTICK_RATE_MS), pdFALSE, (void *)uart, vTimerRxDataCallback);
	if (rx_buffer)
		*rx_buffer = uart->rx_buffer;
	if (params->tx_buffer_size > 0) {
		uart->tx_buffer = hal_ringbuffer_create(params->tx_buffer_size, 0);
	}
	if (tx_buffer)
		*tx_buffer = uart->tx_buffer;
	uart->isrcallbackRxDataPending = params->isrcallbackRxDataPending;
	uart->isrcallbackRxDataErrors = params->isrcallbackRxDataErrors;
	uart->isrcallbackRxOverflowSuspended = params->isrcallbackRxOverflowSuspended;
	uart->isrcallbackTxSpacePending = params->isrcallbackTxSpacePending;
	uart->isrcallbackTxCompleted = params->isrcallbackTxCompleted;

	/* Enable USART and DMA peripherals clock */
	if (uart->usart_apb == 1)
		RCC_APB1PeriphClockCmd(uart->usart_rcc_periph, ENABLE);
	else if (uart->usart_apb == 2)
		RCC_APB2PeriphClockCmd(uart->usart_rcc_periph, ENABLE);
	RCC_AHB1PeriphClockCmd(uart->dma_rx_stream_rcc_periph, ENABLE);

	/* Init USART peripheral */
	USART_InitTypeDef USART_InitStructure;
	USART_InitStructure.USART_BaudRate = params->baud_rate;
	if (params->parity == huartParity_None)
		USART_InitStructure.USART_WordLength = USART_WordLength_8b;
	else
		USART_InitStructure.USART_WordLength = USART_WordLength_9b;
	switch (params->stop_bits) {
	case huartStopBits_0_5: USART_InitStructure.USART_StopBits = USART_StopBits_0_5; break;
	case huartStopBits_1: USART_InitStructure.USART_StopBits = USART_StopBits_1; break;
	case huartStopBits_1_5: USART_InitStructure.USART_StopBits = USART_StopBits_1_5; break;
	case huartStopBits_2: USART_InitStructure.USART_StopBits = USART_StopBits_2; break;
	default: SYS_ASSERT(0); break;
	}
	switch (params->parity) {
	case huartParity_None: USART_InitStructure.USART_Parity = USART_Parity_No; break;
	case huartParity_Even: USART_InitStructure.USART_Parity = USART_Parity_Even; break;
	case huartParity_Odd: USART_InitStructure.USART_Parity = USART_Parity_Odd; break;
	default: SYS_ASSERT(0); break;
	}
	switch (params->hw_flow_control) {
	case huartHwFlowControl_None: USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None; break;
	case huartHwFlowControl_Rx: USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_RTS; break;
	case huartHwFlowControl_Tx: USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_CTS; break;
	case huartHwFlowControl_Rx_Tx: USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_RTS_CTS; break;
	default: SYS_ASSERT(0); break;
	}
	USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
	USART_Init(uart->usart, &USART_InitStructure);
	USART_DMACmd(uart->usart, USART_DMAReq_Rx, ENABLE);

	/* Init DMA rx stream peripheral */
	DMA_InitTypeDef DMA_InitStructure;
	DMA_StructInit(&DMA_InitStructure);
	DMA_InitStructure.DMA_Channel = uart->dma_rx_channel;
	DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t)&(uart->usart->DR);
	DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralToMemory;
	DMA_InitStructure.DMA_BufferSize = 1; // no matter, it's to pass driver library assertion
	DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
	DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
	DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;
	DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;
	DMA_InitStructure.DMA_Mode = DMA_Mode_Normal;
	DMA_InitStructure.DMA_Priority = DMA_Priority_VeryHigh;
	DMA_InitStructure.DMA_FIFOMode = DMA_FIFOMode_Disable;
	DMA_InitStructure.DMA_MemoryBurst = DMA_MemoryBurst_Single;
	DMA_InitStructure.DMA_PeripheralBurst = DMA_PeripheralBurst_Single;
	DMA_Init(uart->dma_rx_stream, &DMA_InitStructure);

	/* Init USART and DMA interrupts */
	NVIC_InitTypeDef NVIC_InitStructure;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = SYS_IRQ_CHANNEL_PREEMPTION_PRIORITY;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_InitStructure.NVIC_IRQChannel = uart->usart_irq_n;
	NVIC_Init(&NVIC_InitStructure);
	NVIC_InitStructure.NVIC_IRQChannel = uart->dma_rx_irq_n;
	NVIC_Init(&NVIC_InitStructure);
	DMA_ITConfig(uart->dma_rx_stream, DMA_IT_TE, ENABLE);

	uart->is_open = true;

	return (hal_uart_handle_t)uart;
}

static double get_character_rate_from_baud_rate(uint32_t rate, hal_uart_stop_bits_t stop_bits, hal_uart_parity_t parity) {
	double character_bits = 1.0 + 8.0; /*start bit + data word bits*/
	if (parity != huartParity_None)
		character_bits += 1.0; /* + optional parity bit*/
	switch (stop_bits) {
	case huartStopBits_0_5: character_bits += 0.5; break;
	case huartStopBits_1: character_bits += 1.0; break;
	case huartStopBits_1_5: character_bits += 1.5; break;
	case huartStopBits_2: character_bits += 2.0; break;
	default: SYS_ASSERT(0); break;
	}
	return (double)(rate)/character_bits;
}

static void set_usart_rx_interrupts(struct s_uart_pcb *uart, FunctionalState state) {
	USART_ITConfig(uart->usart, USART_IT_RXNE, state);
	USART_ITConfig(uart->usart, USART_IT_ERR, state);
	USART_ITConfig(uart->usart, USART_IT_PE, state);
}

static void set_usart_tx_interrupts(struct s_uart_pcb *uart, FunctionalState state) {
	USART_ITConfig(uart->usart, USART_IT_TXE, state);
	USART_ITConfig(uart->usart, USART_IT_TC, state);
}

static void start_dma_rx(struct s_uart_pcb *uart) {
	DMA_ClearFlag(uart->dma_rx_stream, uart->dma_rx_it_flags);
	DMA_Cmd(uart->dma_rx_stream, ENABLE);
	USART_DMACmd(uart->usart, USART_DMAReq_Rx, ENABLE);
}

static void stop_dma_rx(struct s_uart_pcb *uart) {
	USART_DMACmd(uart->usart, USART_DMAReq_Rx, DISABLE);
	DMA_Cmd(uart->dma_rx_stream, DISABLE);
	while (DMA_GetCmdStatus(uart->dma_rx_stream) != DISABLE);
}

void hal_uart_enable(hal_uart_handle_t handle) {
	DEFINE_PCB_FROM_HANDLE(uart, handle);
	if (!(uart->is_open && !(uart->usart->CR1 & USART_CR1_UE)))
		return;
	taskENTER_CRITICAL();
	hal_ringbuffer_flush_read(uart->rx_buffer);
	taskEXIT_CRITICAL();
	hal_uart_resume_rx((hal_uart_handle_t)uart);
	xSemaphoreTake(uart->smphr_tx_complete, 0);
	USART_Cmd(uart->usart, ENABLE);
	hal_uart_resume_tx((hal_uart_handle_t)uart);
}

void hal_uart_disable(hal_uart_handle_t handle) {
	DEFINE_PCB_FROM_HANDLE(uart, handle);
	if (!(uart->is_open && (uart->usart->CR1 & USART_CR1_UE)))
		return;
	if (uart->rx_data_timer)
		xTimerStop(uart->rx_data_timer, (portTickType)0);
	taskENTER_CRITICAL();
	USART_Cmd(uart->usart, DISABLE);
	stop_dma_rx(uart);
	if (uart->rx_active_transfer_size > 0)
		hal_ringbuffer_write_next(uart->rx_buffer, 0);
	uart->rx_active_transfer_size = 0;
	set_usart_rx_interrupts(uart, DISABLE);
	if (uart->tx_transfer != txtransferNone)
		NVIC_SetPendingIRQ(uart->usart_irq_n); // force processing of transmission completition (interruption)
	uart->tx_transfer_data_ptr = NULL;
	uart->tx_transfer_pending_size = 0;
	if (uart->tx_buffer)
		hal_ringbuffer_flush_read(uart->tx_buffer);
	set_usart_tx_interrupts(uart, DISABLE);
	taskEXIT_CRITICAL();
	// usart tx interrupt (requested transmission interruption) activates right here (just after exit from critical section)
}

void hal_uart_resume_rx(hal_uart_handle_t handle) {
	DEFINE_PCB_FROM_HANDLE(uart, handle);
	taskENTER_CRITICAL();
	if (uart->is_open && (DMA_GetCmdStatus(uart->dma_rx_stream) == DISABLE)) {
		SYS_ASSERT(uart->rx_active_transfer_size == 0);
		/* Prepare memory address and size for DMA transfer */
		uint8_t *chunk;
		hal_ringbuffer_get_write_ptr(uart->rx_buffer, &chunk, &(uart->rx_active_transfer_size));
		uart->rx_active_transfer_size = min(uart->rx_active_transfer_size, 0xFFFF);
		DMA_MemoryTargetConfig(uart->dma_rx_stream, (uint32_t)chunk, DMA_Memory_0);
		DMA_SetCurrDataCounter(uart->dma_rx_stream, (uint16_t)uart->rx_active_transfer_size);
		/* Reset USART rx state */
		(void)uart->usart->SR;
		(void)uart->usart->DR;
		if (uart->rx_data_timer)
			uart->rx_data_timer_expired = false;
		/* Start DMA rx transfer and be ready to accept stream */
		start_dma_rx(uart);
		set_usart_rx_interrupts(uart, ENABLE);
	}
	taskEXIT_CRITICAL();
}

void hal_uart_resume_tx(hal_uart_handle_t handle) {
	DEFINE_PCB_FROM_HANDLE(uart, handle);
	if (!(uart->is_open && (uart->tx_buffer) && (uart->usart->CR1 & USART_CR1_UE) && (uart->tx_transfer == txtransferNone)))
		return;
	taskENTER_CRITICAL();
	if (!hal_ringbuffer_is_empty(uart->tx_buffer)) {
		uart->tx_transfer = txtransferActiveInternal;
		set_usart_tx_interrupts(uart, ENABLE);
	}
	taskEXIT_CRITICAL();
}

bool hal_uart_receive_byte(hal_uart_handle_t handle, uint8_t *data) {
	DEFINE_PCB_FROM_HANDLE(uart, handle);
	if (!uart->is_open)
		return false;
	bool received;
	taskENTER_CRITICAL();
	received = hal_ringbuffer_read_byte(uart->rx_buffer, data);
	taskEXIT_CRITICAL();
	return received;
}

bool hal_uart_transmit_blocked(hal_uart_handle_t handle, uint8_t *data, int size, portTickType block_time) {
	DEFINE_PCB_FROM_HANDLE(uart, handle);
	if (!(uart->is_open && (uart->usart->CR1 & USART_CR1_UE) && (uart->tx_transfer == txtransferNone)))
		return false;
	taskENTER_CRITICAL();
	uart->tx_transfer = txtransferActiveExternalSync;
	uart->tx_transfer_data_ptr = data;
	uart->tx_transfer_pending_size = size;
	set_usart_tx_interrupts(uart, ENABLE);
	taskEXIT_CRITICAL();
	xSemaphoreTake(uart->smphr_tx_complete, block_time);
	return (uart->tx_transfer == txtransferNone);
}

bool hal_uart_start_transmit(hal_uart_handle_t handle, uint8_t *data, int size) {
	DEFINE_PCB_FROM_HANDLE(uart, handle);
	if (!(uart->is_open && (uart->usart->CR1 & USART_CR1_UE) && (uart->tx_transfer == txtransferNone)))
		return false;
	taskENTER_CRITICAL();
	uart->tx_transfer = txtransferActiveExternalAsync;
	uart->tx_transfer_data_ptr = data;
	uart->tx_transfer_pending_size = size;
	set_usart_tx_interrupts(uart, ENABLE);
	taskEXIT_CRITICAL();
	return true;
}

/* For non-zero interval it controls timer scheduling, timeout processing and managing USART RXNE interrupt */
static bool update_rx_data_indication_from_isr(struct s_uart_pcb *uart, size_t bytes_transfered, bool rx_active, signed portBASE_TYPE *pxHigherPriorityTaskWoken) {
	bool result = false;
	if ((uart->rx_data_timer) && rx_active) {
		if (uart->rx_data_timer_expired) {
			uart->rx_data_timer_expired = false;
			result = true;
		}
		if ((bytes_transfered > 0) && (xTimerIsTimerActive(uart->rx_data_timer) != pdTRUE)) {
			uart->usart->CR1 &= ~USART_CR1_RXNEIE; // disable rx interrupt at least for timer period and don't process active streaming
			xTimerStartFromISR(uart->rx_data_timer, pxHigherPriorityTaskWoken); // when timer expires it will trigger this processing again
		}
	} else {
		result = (bytes_transfered > 0);
	}
	return result;
}

static void vTimerRxDataCallback(xTimerHandle pxTimer) {
	DEFINE_PCB_FROM_HANDLE(uart, pvTimerGetTimerID(pxTimer));
	taskENTER_CRITICAL();
	if (uart->usart->CR1 & USART_CR1_UE) {
		uart->usart->CR1 |= USART_CR1_RXNEIE; // enable uart rx interrupt back again
		uart->rx_data_timer_expired = true;
		/* invoke rx handler to process rx data indication (with updated flag)
		 * this solution isn't perfect, but unified with timer-less method of indication
		 */
		NVIC_SetPendingIRQ(uart->usart_irq_n);
	}
	taskEXIT_CRITICAL();
}

/* Note, that this handler may be triggered not only by USART hardware interrupt request, but also in software.
 * In particularly, it DOES NOT mean there are any hardware flag pending.
 */
static void usart_irq_handler(struct s_uart_pcb *uart) {
	portBASE_TYPE xHigherPriorityTaskWoken = pdFALSE;
	/* USART status register must be read once (instead of calling USART_GetITStatus() for each flag) in order to:
	 * - prevent loss of pending flags (such as known issue with PE);
	 * - prevent loss of events occured while irq handler is active;
	 * - provide guaranteed TC bit clearing sequence (in sync with write access to DR register).
	 */
	uint16_t USART_SR = uart->usart->SR;
	usart_irq_handler_rx(uart, USART_SR, &xHigherPriorityTaskWoken);
	usart_irq_handler_tx(uart, USART_SR, &xHigherPriorityTaskWoken);
	portEND_SWITCHING_ISR(xHigherPriorityTaskWoken);
}

static void usart_irq_handler_rx(struct s_uart_pcb *uart, uint16_t USART_SR_value, signed portBASE_TYPE *pxHigherPriorityTaskWoken) {
	bool errors_detected = false;
	bool overflow_detected = false;
	size_t bytes_transfered = 0;
	bool data_indication_required = false;

	if (!(uart->usart->CR1 & USART_CR1_UE))
		return; // no processing required if uart disabled

	if (uart->usart->CR3 & USART_CR3_EIE) {
		errors_detected = USART_SR_value & (USART_SR_PE | USART_SR_FE | USART_SR_NE);
		overflow_detected = (USART_SR_value & USART_SR_ORE) || (uart->rx_active_transfer_size == 0); // both checks required (otherwise RXNE will cause interrupt remain pending)
		if (errors_detected)
			(void)uart->usart->DR; // dummy read required to reset flags (yes, there are races with DMA, but we are agree to loss one byte in such conditions)
		/* Process possible overflow condition in stream */
		if (overflow_detected) {
			set_usart_rx_interrupts(uart, DISABLE);
			stop_dma_rx(uart);
			if (uart->rx_data_timer)
				xTimerStopFromISR(uart->rx_data_timer, pxHigherPriorityTaskWoken);
			/* write transferred chunk to buffer (if any) */
			if (uart->rx_active_transfer_size > 0) {
				bytes_transfered = (uart->rx_active_transfer_size - (size_t)DMA_GetCurrDataCounter(uart->dma_rx_stream));
				hal_ringbuffer_write_next(uart->rx_buffer, bytes_transfered);
			}
			uart->rx_active_transfer_size = 0; // marks streaming as inactive
		}
	}

	/* Process active streaming (should be done after processing overflow condition) */
	if ((uart->usart->CR1 & USART_CR1_RXNEIE) && (uart->rx_active_transfer_size > 0)) { // warning: dependency on RXNEIE requires special handling in update_rx_data_indication_from_isr() and vTimerRxDataCallback() !
		/* Following variables must be local in order to get them placed in stack (located in internal SRAM) */
		__IO uint32_t *DMAyStreamx_CR = &(uart->dma_rx_stream->CR);
		__IO uint32_t *DMAyStreamx_NDTR = &(uart->dma_rx_stream->NDTR);
		__IO uint32_t *DMAyStreamx_M0AR = &(uart->dma_rx_stream->M0AR);
		__IO uint32_t *DMAy_IFCR = (uart->dma_rx_stream_number > 3)?(&(uart->dma_rx->HIFCR)):(&(uart->dma_rx->LIFCR)); // select high or low register
		uint32_t DMA_IFCR_clear_flags = uart->dma_rx_it_raw_ifcr_flags; // select CTCIFx and CHTIFx flags
		size_t active_transfer_size = uart->rx_active_transfer_size;
		HALRingBufferCtrl_t cached_buf_ctrl = hal_ringbuffer_get_ctrl(uart->rx_buffer);
		uint8_t *next_chunk;
		__disable_irq(); // protect against higher-level interrupts preemption
		(*DMAyStreamx_CR) &= ~DMA_SxCR_EN; // pause DMA stream
		/* Following section of code must execute as fast as possible (to not miss incoming USART data)
		 * Its execution speed is limiting factor of maximum uart baudrate supported.
		 * Dependies reduced down to HCLK (Flash and internal SRAM used), hal_ringbuffer execution branches, AHB matrix activity and C compiler.
		 * Estimated max(worst) execution length defined in system HCLK cycles.
		 * (Don't beat me, say thanks to STMicroelectronics for their DMA controller, not allowing to get accurate value from NDTR during transfer.)
		 */
#define MAX_USART_IRQ_DMA_PAUSE_LENGTH_HCLKS	300
		while ((*DMAyStreamx_CR) & DMA_SxCR_EN); // wait it paused
		bytes_transfered = active_transfer_size - (*DMAyStreamx_NDTR); // get actual bytes count transferred after previous DMA start/resume
		hal_ringbuffer_extctrl_write_next(&cached_buf_ctrl, bytes_transfered); // forward buffer write state
		hal_ringbuffer_extctrl_get_write_ptr(&cached_buf_ctrl, &next_chunk, &active_transfer_size); // get next available write space (may be zero size)
		active_transfer_size = min(active_transfer_size, MAX_DMA_TRANSFER_SIZE); // limit next chunk size with DMA capability
		(*DMAyStreamx_M0AR) = (uint32_t)next_chunk; // set start address of next chunk to DMA
		(*DMAyStreamx_NDTR) = (uint16_t)active_transfer_size; // set size of next chunk to DMA
		(*DMAy_IFCR) = DMA_IFCR_clear_flags;
		(*DMAyStreamx_CR) |= DMA_SxCR_EN; // resume DMA stream (but don't check it actually resumed)
		// end of fast section of code
		__enable_irq(); // release protection from higher-level interrupts
		/* Update state */
		uart->rx_active_transfer_size = active_transfer_size;
		hal_ringbuffer_update_write_ctrl(uart->rx_buffer, &cached_buf_ctrl);
	}

	/* Process rx data indication conditions and state */
	data_indication_required = update_rx_data_indication_from_isr(uart, bytes_transfered, (uart->rx_active_transfer_size > 0), pxHigherPriorityTaskWoken);

	/* Process pending data errors in stream (must be after last transfer completed in order to account all possible error bytes) */
	if (errors_detected && uart->isrcallbackRxDataErrors)
		uart->isrcallbackRxDataErrors((hal_uart_handle_t)uart, hal_ringbuffer_get_pending_data_size(uart->rx_buffer), pxHigherPriorityTaskWoken);
	/* Process transferred data (written in buffer) and possible stream suspension (overflow status) */
	if (data_indication_required && uart->isrcallbackRxDataPending)
		uart->isrcallbackRxDataPending((hal_uart_handle_t)uart, hal_ringbuffer_get_pending_data_size(uart->rx_buffer), pxHigherPriorityTaskWoken);
	if (overflow_detected && uart->isrcallbackRxOverflowSuspended)
		uart->isrcallbackRxOverflowSuspended((hal_uart_handle_t)uart, pxHigherPriorityTaskWoken);
}

static int get_max_dma_rx_pause_length(void) {
	return MAX_USART_IRQ_DMA_PAUSE_LENGTH_HCLKS;
}

static void usart_irq_handler_tx(struct s_uart_pcb *uart, uint16_t USART_SR_value, signed portBASE_TYPE *pxHigherPriorityTaskWoken) {
	/* Process active transmission (from any buffer) and transfer completition (optimized, see warnings) */
	bool more_data_pending;
	switch (uart->tx_transfer) {
	case txtransferActiveInternal:
		more_data_pending = !hal_ringbuffer_is_empty(uart->tx_buffer);
		break;
	case txtransferActiveExternalAsync:
	case txtransferActiveExternalSync:
		more_data_pending = (uart->tx_transfer_pending_size > 0);
		break;
	default:
		more_data_pending = false;
		break;
	}
	if (more_data_pending && (USART_SR_value & USART_SR_TXE)) { // warning: pending data in buffer must be flushed when uart disabled
		// warning: here is the only place to write DR register (see usart_irq_handler)
		bool disable_txe_interrupt; // last byte transferred ?
		switch (uart->tx_transfer) {
		case txtransferActiveInternal: {
			int pending_size;
			hal_ringbuffer_read_byte(uart->tx_buffer, (uint8_t *)&(uart->usart->DR));
			pending_size = hal_ringbuffer_get_pending_data_size(uart->tx_buffer);
			if (uart->isrcallbackTxSpacePending)
				uart->isrcallbackTxSpacePending((hal_uart_handle_t)uart, hal_ringbuffer_get_free_space_size(uart->tx_buffer), pxHigherPriorityTaskWoken);
			disable_txe_interrupt = (pending_size == 0);
			break;
		}
		case txtransferActiveExternalAsync:
		case txtransferActiveExternalSync: {
			uart->usart->DR = *((uart->tx_transfer_data_ptr)++);
			uart->tx_transfer_pending_size--;
			disable_txe_interrupt = (uart->tx_transfer_pending_size == 0);
			break;
		}
		default:
			disable_txe_interrupt = true;
			break;
		}
		if (disable_txe_interrupt)
			uart->usart->CR1 &= ~USART_CR1_TXEIE; // disable TXE interrupt (since it cannot be cleared and will remain pending)
	} else if ((uart->tx_transfer != txtransferNone) && ((USART_SR_value & USART_SR_TC) || !(uart->usart->CR1 & USART_CR1_UE))) {
		// warning:	such conditions and checks above are sensible to TC bit clearing sequence and transmission interruption
		// 			(see usart_irq_handler and hal_uart_disable)
		uart->usart->CR1 &= ~(USART_CR1_TCIE | USART_CR1_TXEIE);
		switch (uart->tx_transfer) {
		case txtransferActiveInternal:
		case txtransferActiveExternalAsync:
			if (uart->isrcallbackTxCompleted)
				uart->isrcallbackTxCompleted((hal_uart_handle_t)uart, pxHigherPriorityTaskWoken);
			break;
		case txtransferActiveExternalSync:
			xSemaphoreGiveFromISR(uart->smphr_tx_complete, pxHigherPriorityTaskWoken);
			break;
		default: break;
		}
		uart->tx_transfer = txtransferNone;
	}
}

static void dma_rx_irq_handler(struct s_uart_pcb *uart) {
	/* DMA transfer error (bus error ?) */
	if ((CoreDebug->DHCSR & CoreDebug_DHCSR_C_DEBUGEN_Msk)) { // check whether the processor is running under debugger control
		__asm volatile("bkpt");
	}
	if (DMA_GetITStatus(uart->dma_rx_stream, uart->dma_rx_it_teif) == SET) {
		DMA_ClearITPendingBit(uart->dma_rx_stream, uart->dma_rx_it_teif);
		stop_dma_rx(uart);
		if (uart->rx_active_transfer_size > 0)
			hal_ringbuffer_write_next(uart->rx_buffer, 0);
		uart->rx_active_transfer_size = 0;
		set_usart_rx_interrupts(uart, DISABLE);
		halinternal_system_fault_handler();
	}
}

void USART1_IRQHandler(void) {
	usart_irq_handler(&uart_pcbs[0]);
}
void USART2_IRQHandler(void) {
	usart_irq_handler(&uart_pcbs[1]);
}
void USART3_IRQHandler(void) {
	usart_irq_handler(&uart_pcbs[2]);
}
void UART4_IRQHandler(void) {
	usart_irq_handler(&uart_pcbs[3]);
}
void UART5_IRQHandler(void) {
	usart_irq_handler(&uart_pcbs[4]);
}
void USART6_IRQHandler(void) {
	usart_irq_handler(&uart_pcbs[5]);
}
void DMA2_Stream2_IRQHandler(void) {
	dma_rx_irq_handler(&uart_pcbs[0]);
}
void DMA1_Stream5_IRQHandler(void) {
	dma_rx_irq_handler(&uart_pcbs[1]);
}
void DMA1_Stream1_IRQHandler(void) {
	dma_rx_irq_handler(&uart_pcbs[2]);
}
void DMA1_Stream2_IRQHandler(void) {
	dma_rx_irq_handler(&uart_pcbs[3]);
}
void DMA1_Stream0_IRQHandler(void) {
	dma_rx_irq_handler(&uart_pcbs[4]);
}
void DMA2_Stream1_IRQHandler(void) {
	dma_rx_irq_handler(&uart_pcbs[5]);
}
