/**
  ******************************************************************************
  * @file    uart.c
  * @author  Artem Pisarenko, PMR dept. software team, ONIIP, PJSC
  * @date    28.08.2015
  * @brief   Реализация аппаратной абстракции доступа к UART на STM32F2xx
  *
  * UART работает в полнодуплексном режиме.
  *
  * Внимание! Доступ к внутренним буферам может осуществляться только из критической секции
  * или из контекста обработчика прерывания с приоритетом не выше SYS_IRQ_CHANNEL_PREEMPTION_PRIORITY.
  * (См. порядок работы и синхронизацию доступа в документации hal_ringbuffer.)
  * Callback'и c префиксом isr означают, что они выполняются из контекста обработчика прерывания. Все вызываемые из них функции FreeRTOS должны использовать аргумент pxHigherPriorityTaskWoken.
  * Все callback'и должны выполняться очень быстро и не содержать блокирующих вызовов
  * (правильная реализация должна выполнять в них только установку флагов и неблокирующие вызовы функций синхронизации FreeRTOS).
  *
  * Прием(Rx):
  * Работает через DMA с использованием внутреннего коммуникационного кольцевого буфера приема.
  * Фактический размер буфера расширяется (наиболее оптимальным способом) с учетом макс. задержки обработки прерываний заданного приоритета в системе,
  * значения мин. интервала задержки непрочитанных данных, конфигурации аппаратного управления потоком и скорости UART.
  * Ресурсы DMA статически выделены для каждого экземпляра UART и недоступны для использования в других модулях.
  * DMA-поток функционирует в режиме Circular direct mode (peripheral-to-memory) и пишет напрямую в память кольцевого буфера.
  * При запуске приема включается DMA-передача, при обнаружении переполнения буфера DMA-передача выключается.
  * При запуске кольцевой буфер сбрасывается на начало, т.к. DMA не поддерживает запись с произвольного места в кольцевом буфере.
  * Флаги DMA "Half-transfer reached" и "Transfer complete" используются для отслеживания продвижения указателя записи по отношению к текущему указателю чтения
  * (поочередно снимаются, а если оба обнаружены выставленными, то считается что произошло критическое переполнение буфера, но такое переполнение исключено в нормальных условиях
  *  благодаря дополнительному расширению буфера).
  * По прерываниям RXNE от UART (а также по таймауту задержки индикации непрочитанных данных, если таковой интервал используется) обновляется указатель записи (см. выше)
  * и отслеживается обычное переполнение буфера. Буфер считается переполненным, если свободное место меньше дополнительного(расширенного) пространства.
  * Приложение может считывать данные из буфера путем вызова функции hal_uart_receive()
  * или напрямую читать из приемного буфера, возвращаенного при открытии UART, выполняя роль потребителя данных.
  * Оптимизировать обработку приема можно путем задания ненулевого интервала задержки индикации непрочитанных данных в буфере.
  * Размер буфера приема должен задаваться с учетом этой задержки и скорости uart.
  * isrcallbackRxDataPending: аргумент unread_bytes_count указывает размер непрочитанных на момент вызова данных в буфере.
  * isrcallbackRxDataErrors: аргумент error_bytes_count указывает размер данных, содержащихся в начале непрочитанных на момент вызова данных в буфере,
  *  корректность которых не гарантируется.
  * isrcallbackRxOverflowSuspended: на момент вызова в буфере отсутствует свободное место,
  *  новые данные не принимаются и ошибки не обнаруживаются до тех пор пока прием не будет возобновлен вызовом hal_uart_start_rx.
  *
  * Передача(Tx):
  * Работает через прерывания USART (посимвольно) с использованием буфера данных.
  * Приложению доступны три способа:
  * - асинхронно из внутреннего кольцевого буфера передачи, выполняя роль производителя данных
  *   (запускается/возобновляется вызовами hal_uart_start_tx(), приостанавливается при завершении отправки последнего байта из буфера)
  *   (с использованием callback-функций isrcallbackTxSpacePending, isrcallbackTxCompleted)
  * - асинхронно из выделяемого приложением буфера данных
  *   (с использованием callback-функции isrcallbackTxCompleted)
  * - блокирующий, из выделяемого приложением буфера данных.
  * isrcallbackTxSpacePending: вызывается только при использовании передачи из внутреннего буфера,
  *                            аргумент available_bytes_count указывает размер свободного места в буфере на момент вызова
  * isrcallbackTxCompleted: вызывается только при ииспользовании асинхронной передачи и обозначает ее завершение (не обязательно успешное)
  *
  * Заняты следующие аппаратные ресурсы:
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

#include <stdbool.h>
#include <string.h>
#include <math.h>
#include "stm32f2xx.h"
#include "FreeRTOS.h"
#include "semphr.h"
#include "timers.h"

#include "sys_internal.h"
#include "sys_internal_freertos_timers.h"
#include "hal_uart.h"

#if configUSE_TIMERS == 0
#error "FreeRTOS must be configured to use software timers"
#endif

#define UART_INSTANCES_COUNT 6

#define DEFINE_PCB_FROM_HANDLE(var_pcb, handle) \
	struct s_uart_pcb *var_pcb = (struct s_uart_pcb *)handle; \
	SYS_ASSERT(var_pcb != 0);

static struct s_uart_pcb {
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
	uint32_t dma_rx_flag_tcif;
	uint32_t dma_rx_flag_htif;
	bool is_open;
	void *userid;
	hal_ringbuffer_t *rx_buffer;
	size_t rx_buffer_extra_space;
	bool rx_data_defer_indication; // use deferred rx unread data indication
	TimerHandle_t rx_data_timer; // (optional) timer used to defer rx unread data indication (data "flush")
	enum {
		rxdatatimerIdle,
		rxdatatimerActive,
		rxdatatimerExpired
	} rx_data_timer_state; // (optional) rx_data_timer state used for deferred indication of rx unread data
	void (*isrcallbackRxDataPending)(hal_uart_handle_t handle, void *userid, size_t unread_bytes_count, signed portBASE_TYPE *pxHigherPriorityTaskWoken);
	void (*isrcallbackRxDataErrors)(hal_uart_handle_t handle, void *userid, size_t error_bytes_count, signed portBASE_TYPE *pxHigherPriorityTaskWoken);
	void (*isrcallbackRxOverflowSuspended)(hal_uart_handle_t handle, void *userid, signed portBASE_TYPE *pxHigherPriorityTaskWoken);
	void (*isrcallbackTxSpacePending)(hal_uart_handle_t handle, void *userid, size_t available_bytes_count, signed portBASE_TYPE *pxHigherPriorityTaskWoken);
	void (*isrcallbackTxCompleted)(hal_uart_handle_t handle, void *userid, signed portBASE_TYPE *pxHigherPriorityTaskWoken);
	hal_ringbuffer_t *tx_buffer;
	enum {
		txtransferNone, // no transmission active
		txtransferActiveInternal, // asynchronous transmission from internal tx_buffer is active (uses isrcallbackTxSpacePending, isrcallbackTxCompleted)
		txtransferActiveExternalAsync, // asynchronous transmission of external data is active (uses isrcallbackTxCompleted)
		txtransferActiveExternalSync, // synchronous transmission of external data is active (uses smphr_tx_complete)
	} tx_transfer;
	uint8_t *tx_transfer_data_ptr; // incrementing pointer to external data buffer (active transmission)
	size_t tx_transfer_pending_size; // remaining size of data from external buffer (active transmission)
	SemaphoreHandle_t smphr_tx_complete; // used for transmission completion synchronization
} uart_pcbs[UART_INSTANCES_COUNT] = {
		{USART1, USART1_IRQn, 2, RCC_APB2Periph_USART1, DMA2, 2, DMA2_Stream2, DMA2_Stream2_IRQn, RCC_AHB1Periph_DMA2, DMA_Channel_4, DMA_IT_TEIF2, DMA_FLAG_TCIF2, DMA_FLAG_HTIF2, false, 0, 0, 0, false, 0, rxdatatimerIdle, NULL, NULL, NULL, NULL, NULL, 0, txtransferNone, 0, 0, 0},
		{USART2, USART2_IRQn, 1, RCC_APB1Periph_USART2, DMA1, 5, DMA1_Stream5, DMA1_Stream5_IRQn, RCC_AHB1Periph_DMA1, DMA_Channel_4, DMA_IT_TEIF5, DMA_FLAG_TCIF5, DMA_FLAG_HTIF5, false, 0, 0, 0, false, 0, rxdatatimerIdle, NULL, NULL, NULL, NULL, NULL, 0, txtransferNone, 0, 0, 0},
		{USART3, USART3_IRQn, 1, RCC_APB1Periph_USART3, DMA1, 1, DMA1_Stream1, DMA1_Stream1_IRQn, RCC_AHB1Periph_DMA1, DMA_Channel_4, DMA_IT_TEIF1, DMA_FLAG_TCIF1, DMA_FLAG_HTIF1, false, 0, 0, 0, false, 0, rxdatatimerIdle, NULL, NULL, NULL, NULL, NULL, 0, txtransferNone, 0, 0, 0},
		{UART4, UART4_IRQn, 1, RCC_APB1Periph_UART4, DMA1, 2, DMA1_Stream2, DMA1_Stream2_IRQn, RCC_AHB1Periph_DMA1, DMA_Channel_4, DMA_IT_TEIF2, DMA_FLAG_TCIF2, DMA_FLAG_HTIF2, false, 0, 0, 0, false, 0, rxdatatimerIdle, NULL, NULL, NULL, NULL, NULL, 0, txtransferNone, 0, 0, 0},
		{UART5, UART5_IRQn, 1, RCC_APB1Periph_UART5, DMA1, 0, DMA1_Stream0, DMA1_Stream0_IRQn, RCC_AHB1Periph_DMA1, DMA_Channel_4, DMA_IT_TEIF0, DMA_FLAG_TCIF0, DMA_FLAG_HTIF0, false, 0, 0, 0, false, 0, rxdatatimerIdle, NULL, NULL, NULL, NULL, NULL, 0, txtransferNone, 0, 0, 0},
		{USART6, USART6_IRQn, 2, RCC_APB2Periph_USART6, DMA2, 1, DMA2_Stream1, DMA2_Stream1_IRQn, RCC_AHB1Periph_DMA2, DMA_Channel_5, DMA_IT_TEIF1, DMA_FLAG_TCIF1, DMA_FLAG_HTIF1, false, 0, 0, 0, false, 0, rxdatatimerIdle, NULL, NULL, NULL, NULL, NULL, 0, txtransferNone, 0, 0, 0},
};

static double uart_character_rate_from_baud_rate(uint32_t rate, hal_uart_stop_bits_t stop_bits, hal_uart_parity_t parity);
static void uart_set_rx_interrupts(struct s_uart_pcb *uart, FunctionalState state);
static void uart_set_tx_interrupts(struct s_uart_pcb *uart, FunctionalState state);
static void uart_start_dma_rx(struct s_uart_pcb *uart);
static void uart_stop_dma_rx(struct s_uart_pcb *uart);
static bool uart_is_dma_rx_running(struct s_uart_pcb *uart);
static size_t uart_update_rx_buffer_from_isr(struct s_uart_pcb *uart, bool dma_tc, uint16_t dma_ndt);
static void uart_suspend_rx_from_isr(struct s_uart_pcb *uart, signed portBASE_TYPE *pxHigherPriorityTaskWoken);
static bool uart_update_rx_data_indication_from_isr(struct s_uart_pcb *uart, size_t bytes_transfered, bool rx_active, signed portBASE_TYPE *pxHigherPriorityTaskWoken);
static void uart_timer_rx_data_callback(TimerHandle_t xTimer);
static void uart_usart_irq_handler(struct s_uart_pcb *uart);
static void uart_usart_irq_handler_rx(struct s_uart_pcb *uart, uint16_t USART_SR_value, signed portBASE_TYPE *pxHigherPriorityTaskWoken);
static void uart_usart_irq_handler_tx(struct s_uart_pcb *uart, uint16_t USART_SR_value, signed portBASE_TYPE *pxHigherPriorityTaskWoken);
static void uart_dma_rx_irq_handler(struct s_uart_pcb *uart);

void halinternal_uart_init(void) {
	for (int i = 0; i < sizeof(uart_pcbs)/sizeof(uart_pcbs[0]); i++) {
		uart_pcbs[i].rx_data_timer = xTimerCreate("HAL_UART_rx_data_X", 1/*doesn't matter*/, pdFALSE, (void *)&(uart_pcbs[i]), uart_timer_rx_data_callback);
		halinternal_freertos_timer_queue_length += 3; // must match usage scenario (xTimerXXX calls, calls contexts, worst case of queuing)
		uart_pcbs[i].smphr_tx_complete = xSemaphoreCreateCounting(1, 0);
	}
}

void hal_uart_set_default_params(hal_uart_params_t *params) {
	SYS_ASSERT(params);
	params->baud_rate = 115200;
	params->stop_bits = huartStopBits_1;
	params->parity = huartParity_None;
	params->hw_flow_control = huartHwFlowControl_None;
	params->rx_buffer_size = 0;
	params->tx_buffer_size = 0;
	params->rx_data_pending_interval = 0;
	params->userid = 0;
	params->isrcallbackRxDataPending = NULL;
	params->isrcallbackRxDataErrors = NULL;
	params->isrcallbackRxOverflowSuspended = NULL;
	params->isrcallbackTxSpacePending = NULL;
	params->isrcallbackTxCompleted = NULL;
}

hal_uart_handle_t hal_uart_open(int instance, hal_uart_params_t *params, hal_ringbuffer_t **rx_buffer, hal_ringbuffer_t **tx_buffer) {
	struct s_uart_pcb *uart = &(uart_pcbs[instance-1]);
	size_t total_rx_buffer_size;
	hal_ringbuffer_t *new_rx_buffer = 0;
	hal_ringbuffer_t *new_tx_buffer = 0;

	SYS_ASSERT((1 <= instance) && (instance <= UART_INSTANCES_COUNT));
	SYS_ASSERT(params);
	SYS_ASSERT(params->isrcallbackRxOverflowSuspended); // required in order to support hal_uart_start_rx()
	SYS_ASSERT(params->rx_buffer_size > 0); // rx buffer required because reception is always enabled
	SYS_ASSERT(xTaskGetSchedulerState() == taskSCHEDULER_RUNNING); // due to freertos semaphores and timers usage

	int max_irq_service_delay = 0;
	if (!((params->hw_flow_control == huartHwFlowControl_Rx) || (params->hw_flow_control == huartHwFlowControl_Rx_Tx)))
		max_irq_service_delay = SYS_MAX_IRQ_LATENCY_MS;
	double character_rate = uart_character_rate_from_baud_rate(params->baud_rate, params->stop_bits, params->parity);
	size_t extra_rx_buffer_space = ceil(character_rate*((double)(max(max_irq_service_delay, (params->rx_data_pending_interval + 1)))/1000));
	total_rx_buffer_size = params->rx_buffer_size + extra_rx_buffer_space; // extra space required to mitigate system-level processing delays
	total_rx_buffer_size += (total_rx_buffer_size % 2); // round up to nearest even number (required to determine DMA half-transfer condition properly)
	SYS_ASSERT(total_rx_buffer_size <= MAX_DMA_TRANSFER_SIZE); // incompatible uart baudrate and (system characteristics and/or uart parameters)

	USART_InitTypeDef usart_init_struct;
	DMA_InitTypeDef dma_init_struct;
	NVIC_InitTypeDef nvic_init_struct;
	USART_StructInit(&usart_init_struct);
	usart_init_struct.USART_BaudRate = params->baud_rate;
	if (params->parity == huartParity_None)
		usart_init_struct.USART_WordLength = USART_WordLength_8b;
	else
		usart_init_struct.USART_WordLength = USART_WordLength_9b;
	switch (params->stop_bits) {
	case huartStopBits_0_5: usart_init_struct.USART_StopBits = USART_StopBits_0_5; break;
	case huartStopBits_1: usart_init_struct.USART_StopBits = USART_StopBits_1; break;
	case huartStopBits_1_5: usart_init_struct.USART_StopBits = USART_StopBits_1_5; break;
	case huartStopBits_2: usart_init_struct.USART_StopBits = USART_StopBits_2; break;
	default: SYS_ASSERT(0); break;
	}
	switch (params->parity) {
	case huartParity_None: usart_init_struct.USART_Parity = USART_Parity_No; break;
	case huartParity_Even: usart_init_struct.USART_Parity = USART_Parity_Even; break;
	case huartParity_Odd: usart_init_struct.USART_Parity = USART_Parity_Odd; break;
	default: SYS_ASSERT(0); break;
	}
	switch (params->hw_flow_control) {
	case huartHwFlowControl_None: usart_init_struct.USART_HardwareFlowControl = USART_HardwareFlowControl_None; break;
	case huartHwFlowControl_Rx: usart_init_struct.USART_HardwareFlowControl = USART_HardwareFlowControl_RTS; break;
	case huartHwFlowControl_Tx: usart_init_struct.USART_HardwareFlowControl = USART_HardwareFlowControl_CTS; break;
	case huartHwFlowControl_Rx_Tx: usart_init_struct.USART_HardwareFlowControl = USART_HardwareFlowControl_RTS_CTS; break;
	default: SYS_ASSERT(0); break;
	}
	usart_init_struct.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
	DMA_StructInit(&dma_init_struct);
	dma_init_struct.DMA_Channel = uart->dma_rx_channel;
	dma_init_struct.DMA_PeripheralBaseAddr = (uint32_t)&(uart->usart->DR);
	dma_init_struct.DMA_DIR = DMA_DIR_PeripheralToMemory;
	dma_init_struct.DMA_BufferSize = 1; // no matter, it's to pass driver library assertion
	dma_init_struct.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
	dma_init_struct.DMA_MemoryInc = DMA_MemoryInc_Enable;
	dma_init_struct.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;
	dma_init_struct.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;
	dma_init_struct.DMA_Mode = DMA_Mode_Circular;
	dma_init_struct.DMA_Priority = DMA_Priority_VeryHigh;
	dma_init_struct.DMA_FIFOMode = DMA_FIFOMode_Disable;
	dma_init_struct.DMA_MemoryBurst = DMA_MemoryBurst_Single;
	dma_init_struct.DMA_PeripheralBurst = DMA_PeripheralBurst_Single;
	nvic_init_struct.NVIC_IRQChannelPreemptionPriority = SYS_IRQ_CHANNEL_PREEMPTION_PRIORITY;
	nvic_init_struct.NVIC_IRQChannelSubPriority = 0;
	nvic_init_struct.NVIC_IRQChannelCmd = ENABLE;

	new_rx_buffer = hal_ringbuffer_create(total_rx_buffer_size, 0);
	if (rx_buffer)
		*rx_buffer = new_rx_buffer;
	if (params->tx_buffer_size > 0)
		new_tx_buffer = hal_ringbuffer_create(params->tx_buffer_size, 0);
	if (tx_buffer)
		*tx_buffer = new_tx_buffer;

	portENTER_CRITICAL();
	/* Mark peripheral instance as busy */
	SYS_ASSERT(uart->is_open == false);
	uart->is_open = true;
	/* Enable USART and DMA peripherals clock */
	if (uart->usart_apb == 1)
		RCC_APB1PeriphClockCmd(uart->usart_rcc_periph, ENABLE);
	else if (uart->usart_apb == 2)
		RCC_APB2PeriphClockCmd(uart->usart_rcc_periph, ENABLE);
	RCC_AHB1PeriphClockCmd(uart->dma_rx_stream_rcc_periph, ENABLE);
	/* Init USART peripheral */
	USART_Init(uart->usart, &usart_init_struct);
	/* Init DMA rx stream peripheral */
	DMA_Init(uart->dma_rx_stream, &dma_init_struct);
	/* Init USART and DMA interrupts */
	nvic_init_struct.NVIC_IRQChannel = uart->usart_irq_n;
	NVIC_Init(&nvic_init_struct);
	nvic_init_struct.NVIC_IRQChannel = uart->dma_rx_irq_n;
	NVIC_Init(&nvic_init_struct);
	DMA_ITConfig(uart->dma_rx_stream, DMA_IT_TE, ENABLE);
	portEXIT_CRITICAL();

	uart->userid = params->userid;
	uart->rx_buffer = new_rx_buffer;
	uart->tx_buffer = new_tx_buffer;
	uart->rx_buffer_extra_space = extra_rx_buffer_space;
	if (params->rx_data_pending_interval > 0) {
		uart->rx_data_defer_indication = true;
		xTimerChangePeriod(uart->rx_data_timer, (params->rx_data_pending_interval/portTICK_PERIOD_MS), portMAX_DELAY);
		xTimerStop(uart->rx_data_timer, portMAX_DELAY); // because previous xTimerChangePeriod() stupidly starts timer
	}
	uart->isrcallbackRxDataPending = params->isrcallbackRxDataPending;
	uart->isrcallbackRxDataErrors = params->isrcallbackRxDataErrors;
	uart->isrcallbackRxOverflowSuspended = params->isrcallbackRxOverflowSuspended;
	uart->isrcallbackTxSpacePending = params->isrcallbackTxSpacePending;
	uart->isrcallbackTxCompleted = params->isrcallbackTxCompleted;
	hal_uart_start_rx((hal_uart_handle_t)uart);
	xSemaphoreTake(uart->smphr_tx_complete, 0);
	USART_Cmd(uart->usart, ENABLE);

	return (hal_uart_handle_t)uart;
}

void hal_uart_close(hal_uart_handle_t handle) {
	DEFINE_PCB_FROM_HANDLE(uart, handle)
	SYS_ASSERT(xTaskGetSchedulerState() == taskSCHEDULER_RUNNING); // due to freertos semaphores and timers usage

	uart->rx_data_timer_state = rxdatatimerIdle;
	if (uart->rx_data_defer_indication)
		xTimerStop(uart->rx_data_timer, portMAX_DELAY);
	portENTER_CRITICAL();
	USART_Cmd(uart->usart, DISABLE);
	uart_stop_dma_rx(uart);
	uart_set_rx_interrupts(uart, DISABLE);
	if (uart->tx_transfer != txtransferNone)
		NVIC_SetPendingIRQ(uart->usart_irq_n); // force processing of transmission completion (interruption)
	uart->tx_transfer_data_ptr = 0;
	uart->tx_transfer_pending_size = 0;
	if (uart->tx_buffer)
		hal_ringbuffer_flush_read(uart->tx_buffer);
	uart_set_tx_interrupts(uart, DISABLE);
	portEXIT_CRITICAL();
	// usart tx interrupt (requested transmission interruption) activates right here (just after exit from critical section)

	if (uart->tx_buffer)
		hal_ringbuffer_delete(uart->tx_buffer);
	hal_ringbuffer_delete(uart->rx_buffer);
	uart->userid = 0;
	uart->rx_buffer = 0;
	uart->tx_buffer = 0;
	uart->rx_buffer_extra_space = 0;
	uart->rx_data_defer_indication = false;
	uart->isrcallbackRxDataPending = NULL;
	uart->isrcallbackRxDataErrors = NULL;
	uart->isrcallbackRxOverflowSuspended = NULL;
	uart->isrcallbackTxSpacePending = NULL;
	uart->isrcallbackTxCompleted = NULL;

	NVIC_InitTypeDef nvic_init_struct;
	nvic_init_struct.NVIC_IRQChannelPreemptionPriority = SYS_IRQ_CHANNEL_PREEMPTION_PRIORITY;
	nvic_init_struct.NVIC_IRQChannelSubPriority = 0;
	nvic_init_struct.NVIC_IRQChannelCmd = DISABLE;

	portENTER_CRITICAL();
	/* Mark peripheral instance as not busy */
	SYS_ASSERT(uart->is_open == true);
	uart->is_open = false;
	/* Deinit DMA and USART interrupts */
	DMA_ITConfig(uart->dma_rx_stream, DMA_IT_TE, DISABLE);
	nvic_init_struct.NVIC_IRQChannel = uart->dma_rx_irq_n;
	NVIC_Init(&nvic_init_struct);
	nvic_init_struct.NVIC_IRQChannel = uart->usart_irq_n;
	NVIC_Init(&nvic_init_struct);
	/* Reset DMA rx stream peripheral */
	DMA_DeInit(uart->dma_rx_stream);
	/* Reset USART peripheral */
	USART_DeInit(uart->usart);
	/* Disable DMA and USART peripherals clock */
	RCC_AHB1PeriphClockCmd(uart->dma_rx_stream_rcc_periph, DISABLE);
	if (uart->usart_apb == 1)
		RCC_APB1PeriphClockCmd(uart->usart_rcc_periph, DISABLE);
	else if (uart->usart_apb == 2)
		RCC_APB2PeriphClockCmd(uart->usart_rcc_periph, DISABLE);
	portEXIT_CRITICAL();
}

static double uart_character_rate_from_baud_rate(uint32_t rate, hal_uart_stop_bits_t stop_bits, hal_uart_parity_t parity) {
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

static void uart_set_rx_interrupts(struct s_uart_pcb *uart, FunctionalState state) {
	USART_ITConfig(uart->usart, USART_IT_RXNE, state);
	USART_ITConfig(uart->usart, USART_IT_ERR, state);
	USART_ITConfig(uart->usart, USART_IT_PE, state);
}

static void uart_set_tx_interrupts(struct s_uart_pcb *uart, FunctionalState state) {
	USART_ITConfig(uart->usart, USART_IT_TXE, state);
	USART_ITConfig(uart->usart, USART_IT_TC, state);
}

static void uart_start_dma_rx(struct s_uart_pcb *uart) {
	DMA_ClearFlag(uart->dma_rx_stream, uart->dma_rx_flag_htif);
	DMA_ClearFlag(uart->dma_rx_stream, uart->dma_rx_flag_tcif);
	DMA_Cmd(uart->dma_rx_stream, ENABLE);
	USART_DMACmd(uart->usart, USART_DMAReq_Rx, ENABLE);
}

static void uart_stop_dma_rx(struct s_uart_pcb *uart) {
	USART_DMACmd(uart->usart, USART_DMAReq_Rx, DISABLE);
	DMA_Cmd(uart->dma_rx_stream, DISABLE);
	while (DMA_GetCmdStatus(uart->dma_rx_stream) != DISABLE);
}

static bool uart_is_dma_rx_running(struct s_uart_pcb *uart) {
	return (DMA_GetCmdStatus(uart->dma_rx_stream) == ENABLE);
}

bool hal_uart_start_rx(hal_uart_handle_t handle) {
	DEFINE_PCB_FROM_HANDLE(uart, handle)
	bool success = false;
	portENTER_CRITICAL();
	if (!uart_is_dma_rx_running(uart)) {
		hal_ringbuffer_reset(uart->rx_buffer);
		/* Prepare memory address and size for DMA transfer */
		uint8_t *buffer_ptr;
		size_t buffer_size;
		hal_ringbuffer_get_write_ptr(uart->rx_buffer, &buffer_ptr, &buffer_size); // expected to be whole ringbuffer
		hal_ringbuffer_write_next(uart->rx_buffer, 0);
		DMA_MemoryTargetConfig(uart->dma_rx_stream, (uint32_t)buffer_ptr, DMA_Memory_0);
		DMA_SetCurrDataCounter(uart->dma_rx_stream, (uint16_t)buffer_size);
		/* Reset USART rx state */
		(void)uart->usart->SR;
		(void)uart->usart->DR;
		uart->rx_data_timer_state = rxdatatimerIdle;
		/* Start DMA rx transfer and be ready to accept stream */
		uart_start_dma_rx(uart);
		uart_set_rx_interrupts(uart, ENABLE);
		/* Mark success */
		success = true;
	}
	portEXIT_CRITICAL();
	return success;
}

bool hal_uart_start_tx(hal_uart_handle_t handle) {
	DEFINE_PCB_FROM_HANDLE(uart, handle)
	if (!((uart->tx_buffer) && (uart->tx_transfer == txtransferNone)))
		return false;
	bool success = false;
	portENTER_CRITICAL();
	if (!hal_ringbuffer_is_empty(uart->tx_buffer)) {
		uart->tx_transfer = txtransferActiveInternal;
		uart_set_tx_interrupts(uart, ENABLE);
		success = true;
	}
	portEXIT_CRITICAL();
	return success;
}

int hal_uart_receive(hal_uart_handle_t handle, uint8_t *buffer, int max_size) {
	DEFINE_PCB_FROM_HANDLE(uart, handle)
	SYS_ASSERT(buffer);
	SYS_ASSERT(max_size > 0);
	int result = 0;
	hal_ringbuffer_ctrl_t buffer_ctrl;
	uint8_t *chunk_ptr;
	size_t chunk_size;
	portENTER_CRITICAL();
	buffer_ctrl = hal_ringbuffer_get_ctrl(uart->rx_buffer);
	portEXIT_CRITICAL();
	while (max_size > 0) {
		hal_ringbuffer_extctrl_get_read_ptr(&buffer_ctrl, &chunk_ptr, &chunk_size);
		if (chunk_size == 0)
			break;
		chunk_size = min(max_size, chunk_size);
		memcpy(buffer, chunk_ptr, chunk_size);
		buffer += chunk_size;
		max_size -= chunk_size;
		hal_ringbuffer_extctrl_read_next(&buffer_ctrl, chunk_size);
		result += chunk_size;
	}
	portENTER_CRITICAL();
	hal_ringbuffer_update_read_ctrl(uart->rx_buffer, &buffer_ctrl);
	portEXIT_CRITICAL();
	return result;
}

bool hal_uart_transmit_blocked(hal_uart_handle_t handle, uint8_t *data, int size, TickType_t block_time) {
	DEFINE_PCB_FROM_HANDLE(uart, handle)
	SYS_ASSERT(data);
	SYS_ASSERT(size > 0);
	if (!(uart->tx_transfer == txtransferNone))
		return false;
	portENTER_CRITICAL();
	uart->tx_transfer = txtransferActiveExternalSync;
	uart->tx_transfer_data_ptr = data;
	uart->tx_transfer_pending_size = size;
	uart_set_tx_interrupts(uart, ENABLE);
	portEXIT_CRITICAL();
	xSemaphoreTake(uart->smphr_tx_complete, block_time);
	return (uart->tx_transfer == txtransferNone);
}

bool hal_uart_start_transmit(hal_uart_handle_t handle, uint8_t *data, int size) {
	DEFINE_PCB_FROM_HANDLE(uart, handle)
	SYS_ASSERT(data);
	SYS_ASSERT(size > 0);
	if (!(uart->tx_transfer == txtransferNone))
		return false;
	portENTER_CRITICAL();
	uart->tx_transfer = txtransferActiveExternalAsync;
	uart->tx_transfer_data_ptr = data;
	uart->tx_transfer_pending_size = size;
	uart_set_tx_interrupts(uart, ENABLE);
	portEXIT_CRITICAL();
	return true;
}

static size_t uart_update_rx_buffer_from_isr(struct s_uart_pcb *uart, bool dma_tc, uint16_t dma_ndt) {
	size_t bytes_transfered;
	uint8_t *buffer_write_ptr;
	size_t dummy;
	int buffer_size = hal_ringbuffer_get_size(uart->rx_buffer);
	uint8_t *buffer_start_addr = (uint8_t *)uart->dma_rx_stream->M0AR;
	hal_ringbuffer_get_write_ptr(uart->rx_buffer, &buffer_write_ptr, &dummy);
	bytes_transfered = (int)((buffer_size - dma_ndt) - (buffer_write_ptr - buffer_start_addr) + dma_tc*buffer_size);
	hal_ringbuffer_write_next(uart->rx_buffer, bytes_transfered);
	return bytes_transfered;
}

static void uart_suspend_rx_from_isr(struct s_uart_pcb *uart, signed portBASE_TYPE *pxHigherPriorityTaskWoken) {
	uart_set_rx_interrupts(uart, DISABLE);
	uart->rx_data_timer_state = rxdatatimerIdle;
	if (uart->rx_data_defer_indication)
		xTimerStopFromISR(uart->rx_data_timer, pxHigherPriorityTaskWoken);
	if (uart_is_dma_rx_running(uart))
		uart_stop_dma_rx(uart);
}

/* For non-zero interval it controls timer scheduling, timeout processing and managing USART RXNE interrupt */
static bool uart_update_rx_data_indication_from_isr(struct s_uart_pcb *uart, size_t bytes_transfered, bool rx_active, signed portBASE_TYPE *pxHigherPriorityTaskWoken) {
	bool result = false;
	if ((uart->rx_data_defer_indication) && rx_active) {
		if (uart->rx_data_timer_state == rxdatatimerExpired) {
			uart->rx_data_timer_state = rxdatatimerIdle;
			result = !hal_ringbuffer_is_empty(uart->rx_buffer);
		}
		if ((bytes_transfered > 0) && (uart->rx_data_timer_state != rxdatatimerActive)) {
			uart->usart->CR1 &= ~USART_CR1_RXNEIE; // disable rx interrupt at least for timer period and don't process active streaming
			uart->rx_data_timer_state = rxdatatimerActive;
			xTimerStartFromISR(uart->rx_data_timer, pxHigherPriorityTaskWoken); // when timer expires it will trigger this processing again
		}
	} else {
		result = (bytes_transfered > 0);
	}
	return result;
}

static void uart_timer_rx_data_callback(TimerHandle_t xTimer) {
	DEFINE_PCB_FROM_HANDLE(uart, pvTimerGetTimerID(xTimer))
	portENTER_CRITICAL();
	if ((uart->rx_data_timer_state == rxdatatimerActive) && (uart->usart->CR1 & USART_CR1_UE)) {
		uart->usart->CR1 |= USART_CR1_RXNEIE; // enable uart rx interrupt back again
		uart->rx_data_timer_state = rxdatatimerExpired;
		/* invoke rx handler to process rx data indication (with updated flag)
		 * this solution isn't perfect, but unified with timer-less method of indication
		 */
		NVIC_SetPendingIRQ(uart->usart_irq_n);
	}
	portEXIT_CRITICAL();
}

/* Note, that this handler may be triggered not only by USART hardware interrupt request, but also in software.
 * In particularly, it DOES NOT mean there are any hardware flag pending.
 */
static void uart_usart_irq_handler(struct s_uart_pcb *uart) {
	portBASE_TYPE xHigherPriorityTaskWoken = pdFALSE;
	/* USART status register must be read once (instead of calling USART_GetITStatus() for each flag) in order to:
	 * - prevent loss of pending flags (such as known issue with PE);
	 * - prevent loss of events occured while irq handler is active;
	 * - provide guaranteed TC bit clearing sequence (in sync with write access to DR register).
	 */
	uint16_t USART_SR = uart->usart->SR;
	if (uart->usart->CR1 & USART_CR1_UE) // uart enabled ?
		uart_usart_irq_handler_rx(uart, USART_SR, &xHigherPriorityTaskWoken);
	uart_usart_irq_handler_tx(uart, USART_SR, &xHigherPriorityTaskWoken);
	portEND_SWITCHING_ISR(xHigherPriorityTaskWoken);
}

static void uart_usart_irq_handler_rx(struct s_uart_pcb *uart, uint16_t USART_SR_value, signed portBASE_TYPE *pxHigherPriorityTaskWoken) {
	bool errors_detected = false;
	bool overflow_detected = false;
	size_t bytes_transfered = 0;
	bool data_indication_required = false;
	uint16_t buffer_size = hal_ringbuffer_get_size(uart->rx_buffer);
	bool flag_dma_ht, flag_dma_tc;
	uint16_t current_dma_ndt;

	/* Capture DMA buffer state */
	flag_dma_ht = DMA_GetFlagStatus(uart->dma_rx_stream, uart->dma_rx_flag_htif);
	flag_dma_tc = DMA_GetFlagStatus(uart->dma_rx_stream, uart->dma_rx_flag_tcif);
	current_dma_ndt = DMA_GetCurrDataCounter(uart->dma_rx_stream); // must be captured after HT,TC flags

	/* Process streaming if it's active */
	// warning: dependency on RXNEIE requires special handling in uart_update_rx_data_indication_from_isr() and uart_timer_rx_data_callback() !
	if ((uart->usart->CR1 & USART_CR1_RXNEIE) && (uart_is_dma_rx_running(uart))) {
		if (!(flag_dma_ht && flag_dma_tc)) {
			if (flag_dma_ht && (current_dma_ndt >= (buffer_size>>1))) // did NDT crossed half-buffer mark since previous capture ?
				DMA_ClearFlag(uart->dma_rx_stream, uart->dma_rx_flag_htif); // condition above ensures that we will not lose flag which may appear at once
			if (flag_dma_tc && (current_dma_ndt < (buffer_size>>1))) // did NDT wrapped from end to start of buffer since previous capture ?
				DMA_ClearFlag(uart->dma_rx_stream, uart->dma_rx_flag_tcif); // condition above ensures that we will not lose flag which may appear at once
			bytes_transfered += uart_update_rx_buffer_from_isr(uart, flag_dma_tc, current_dma_ndt);
			if (hal_ringbuffer_get_free_space_size(uart->rx_buffer) <= uart->rx_buffer_extra_space) // normal buffer overflow
				overflow_detected = true;
		} else { // critical buffer overflow
			overflow_detected = true;
			uart_stop_dma_rx(uart);
		}
	}

	/* Process possible failures in streaming if it's active */
	if (uart->usart->CR3 & USART_CR3_EIE) {
		errors_detected = USART_SR_value & (USART_SR_PE | USART_SR_FE | USART_SR_NE);
		overflow_detected |= (USART_SR_value & USART_SR_ORE) || (!uart_is_dma_rx_running(uart)); // both checks required (otherwise RXNE will cause interrupt remain pending)
		if (errors_detected)
			(void)uart->usart->DR; // dummy read required to reset flags (yes, there are races with DMA, but we are agree to loss one byte in such conditions)
		/* Process possible overflow condition in stream */
		if (overflow_detected) {
			if (uart_is_dma_rx_running(uart)) { // was it forced to stop ?
				/* write transferred chunk to buffer (if any) */
				bytes_transfered += uart_update_rx_buffer_from_isr(uart, flag_dma_tc, current_dma_ndt);
			}
			uart_suspend_rx_from_isr(uart, pxHigherPriorityTaskWoken);
		}
	}

	/* Process rx data indication conditions and state */
	data_indication_required = uart_update_rx_data_indication_from_isr(uart, bytes_transfered, (uart_is_dma_rx_running(uart)), pxHigherPriorityTaskWoken);

	/* Indicate user pending data errors in stream (must be after last transfer completed in order to account all possible error bytes) */
	if (errors_detected && uart->isrcallbackRxDataErrors)
		uart->isrcallbackRxDataErrors((hal_uart_handle_t)uart, uart->userid, hal_ringbuffer_get_pending_data_size(uart->rx_buffer), pxHigherPriorityTaskWoken);
	/* Indicate user transferred data (written in buffer) and possible stream suspension (overflow status) */
	if (data_indication_required && uart->isrcallbackRxDataPending)
		uart->isrcallbackRxDataPending((hal_uart_handle_t)uart, uart->userid, hal_ringbuffer_get_pending_data_size(uart->rx_buffer), pxHigherPriorityTaskWoken);
	if (overflow_detected && uart->isrcallbackRxOverflowSuspended)
		uart->isrcallbackRxOverflowSuspended((hal_uart_handle_t)uart, uart->userid, pxHigherPriorityTaskWoken);
}

static void uart_usart_irq_handler_tx(struct s_uart_pcb *uart, uint16_t USART_SR_value, signed portBASE_TYPE *pxHigherPriorityTaskWoken) {
	/* Process active transmission (from any buffer) and transfer completion (optimized, see warnings) */
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
		// warning: here is the only place to write DR register (see uart_usart_irq_handler)
		bool disable_txe_interrupt; // last byte transferred ?
		switch (uart->tx_transfer) {
		case txtransferActiveInternal: {
			int pending_size;
			hal_ringbuffer_read_byte(uart->tx_buffer, (uint8_t *)&(uart->usart->DR));
			pending_size = hal_ringbuffer_get_pending_data_size(uart->tx_buffer);
			if (uart->isrcallbackTxSpacePending)
				uart->isrcallbackTxSpacePending((hal_uart_handle_t)uart, uart->userid, hal_ringbuffer_get_free_space_size(uart->tx_buffer), pxHigherPriorityTaskWoken);
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
		// 			(see uart_usart_irq_handler and hal_uart_close)
		uart->usart->CR1 &= ~(USART_CR1_TCIE | USART_CR1_TXEIE);
		switch (uart->tx_transfer) {
		case txtransferActiveInternal:
		case txtransferActiveExternalAsync:
			if (uart->isrcallbackTxCompleted)
				uart->isrcallbackTxCompleted((hal_uart_handle_t)uart, uart->userid, pxHigherPriorityTaskWoken);
			break;
		case txtransferActiveExternalSync:
			xSemaphoreGiveFromISR(uart->smphr_tx_complete, pxHigherPriorityTaskWoken);
			break;
		default: break;
		}
		uart->tx_transfer = txtransferNone;
	}
}

static void uart_dma_rx_irq_handler(struct s_uart_pcb *uart) {
	portBASE_TYPE xHigherPriorityTaskWoken = pdFALSE;
	/* DMA transfer error (bus error ?) */
	if (DMA_GetITStatus(uart->dma_rx_stream, uart->dma_rx_it_teif) == SET) {
		uart_suspend_rx_from_isr(uart, &xHigherPriorityTaskWoken);
		DMA_ClearITPendingBit(uart->dma_rx_stream, uart->dma_rx_it_teif);
		halinternal_system_fault_handler();
	}
	portEND_SWITCHING_ISR(xHigherPriorityTaskWoken);
}

void USART1_IRQHandler(void) {
	uart_usart_irq_handler(&uart_pcbs[0]);
}
void USART2_IRQHandler(void) {
	uart_usart_irq_handler(&uart_pcbs[1]);
}
void USART3_IRQHandler(void) {
	uart_usart_irq_handler(&uart_pcbs[2]);
}
void UART4_IRQHandler(void) {
	uart_usart_irq_handler(&uart_pcbs[3]);
}
void UART5_IRQHandler(void) {
	uart_usart_irq_handler(&uart_pcbs[4]);
}
void USART6_IRQHandler(void) {
	uart_usart_irq_handler(&uart_pcbs[5]);
}
void DMA2_Stream2_IRQHandler(void) {
	uart_dma_rx_irq_handler(&uart_pcbs[0]);
}
void DMA1_Stream5_IRQHandler(void) {
	uart_dma_rx_irq_handler(&uart_pcbs[1]);
}
void DMA1_Stream1_IRQHandler(void) {
	uart_dma_rx_irq_handler(&uart_pcbs[2]);
}
void DMA1_Stream2_IRQHandler(void) {
	uart_dma_rx_irq_handler(&uart_pcbs[3]);
}
void DMA1_Stream0_IRQHandler(void) {
	uart_dma_rx_irq_handler(&uart_pcbs[4]);
}
void DMA2_Stream1_IRQHandler(void) {
	uart_dma_rx_irq_handler(&uart_pcbs[5]);
}
