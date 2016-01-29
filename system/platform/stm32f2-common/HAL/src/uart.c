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
  * (поочередно снимаются, а если хоть один обнаружен выставленным после снятия другого, то считается что произошло критическое переполнение буфера,
  * но такое переполнение исключено в нормальных условиях благодаря дополнительному расширению буфера в размере двойного интервала задержки обработки прерываний).
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
#include "task.h"
#include "semphr.h"

#include "sys_internal.h"
#include "hal_timer.h"
#include "hal_uart.h"

#define UART_INSTANCES_COUNT 6

#define DEFINE_PCB_FROM_HANDLE(var_pcb, handle) \
	struct s_uart_pcb *var_pcb = (struct s_uart_pcb *)handle; \
	SYS_ASSERT(var_pcb != 0);

static struct s_uart_pcb {
	USART_TypeDef* usart;
	IRQn_Type usart_irq_n;
	uint8_t usart_apb;
	uint32_t usart_rcc_apb_mask;
	DMA_TypeDef* dma_rx;
	uint8_t dma_rx_stream_number;
	DMA_Stream_TypeDef* dma_rx_stream;
	IRQn_Type dma_rx_stream_irq_n;
	uint32_t dma_rx_rcc_ahb1enr;
	uint8_t dma_rx_channel;
	uint32_t dma_rx_teif_mask;
	uint32_t dma_rx_tcif_mask;
	uint32_t dma_rx_htif_mask;
	bool is_open;
	void *userid;
	hal_ringbuffer_t *rx_buffer;
	size_t rx_buffer_extra_space;
	unsigned int rx_data_pending_interval;
	hal_timer_handle_t rx_data_timer; // (optional) timer used to defer rx unread data indication (data "flush")
	enum {
		rxdatatimerIdle,
		rxdatatimerActive,
		rxdatatimerExpired
	} rx_data_timer_state; // (optional) rx_data_timer state used for deferred indication of rx unread data
	void (*isrcallbackRxDataPending)(hal_uart_handle_t handle, size_t unread_bytes_count, signed portBASE_TYPE *pxHigherPriorityTaskWoken);
	void (*isrcallbackRxDataErrors)(hal_uart_handle_t handle, size_t error_bytes_count, signed portBASE_TYPE *pxHigherPriorityTaskWoken);
	void (*isrcallbackRxOverflowSuspended)(hal_uart_handle_t handle, signed portBASE_TYPE *pxHigherPriorityTaskWoken);
	void (*isrcallbackTxSpacePending)(hal_uart_handle_t handle, size_t available_bytes_count, signed portBASE_TYPE *pxHigherPriorityTaskWoken);
	void (*isrcallbackTxCompleted)(hal_uart_handle_t handle, signed portBASE_TYPE *pxHigherPriorityTaskWoken);
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
		{USART1, USART1_IRQn, 2, RCC_APB2ENR_USART1EN, DMA2, 2, DMA2_Stream2, DMA2_Stream2_IRQn, RCC_AHB1ENR_DMA2EN, 4, DMA_LISR_TEIF2, DMA_LISR_TCIF2, DMA_LISR_HTIF2, false, 0, 0, 0, 0, 0, rxdatatimerIdle, NULL, NULL, NULL, NULL, NULL, 0, txtransferNone, 0, 0, 0},
		{USART2, USART2_IRQn, 1, RCC_APB1ENR_USART2EN, DMA1, 5, DMA1_Stream5, DMA1_Stream5_IRQn, RCC_AHB1ENR_DMA1EN, 4, DMA_HISR_TEIF5, DMA_HISR_TCIF5, DMA_HISR_HTIF5, false, 0, 0, 0, 0, 0, rxdatatimerIdle, NULL, NULL, NULL, NULL, NULL, 0, txtransferNone, 0, 0, 0},
		{USART3, USART3_IRQn, 1, RCC_APB1ENR_USART3EN, DMA1, 1, DMA1_Stream1, DMA1_Stream1_IRQn, RCC_AHB1ENR_DMA1EN, 4, DMA_LISR_TEIF1, DMA_LISR_TCIF1, DMA_LISR_HTIF1, false, 0, 0, 0, 0, 0, rxdatatimerIdle, NULL, NULL, NULL, NULL, NULL, 0, txtransferNone, 0, 0, 0},
		{UART4, UART4_IRQn, 1, RCC_APB1ENR_UART4EN, DMA1, 2, DMA1_Stream2, DMA1_Stream2_IRQn, RCC_AHB1ENR_DMA1EN, 4, DMA_LISR_TEIF2, DMA_LISR_TCIF2, DMA_LISR_HTIF2, false, 0, 0, 0, 0, 0, rxdatatimerIdle, NULL, NULL, NULL, NULL, NULL, 0, txtransferNone, 0, 0, 0},
		{UART5, UART5_IRQn, 1, RCC_APB1ENR_UART5EN, DMA1, 0, DMA1_Stream0, DMA1_Stream0_IRQn, RCC_AHB1ENR_DMA1EN, 4, DMA_LISR_TEIF0, DMA_LISR_TCIF0, DMA_LISR_HTIF0, false, 0, 0, 0, 0, 0, rxdatatimerIdle, NULL, NULL, NULL, NULL, NULL, 0, txtransferNone, 0, 0, 0},
		{USART6, USART6_IRQn, 2, RCC_APB2ENR_USART6EN, DMA2, 1, DMA2_Stream1, DMA2_Stream1_IRQn, RCC_AHB1ENR_DMA2EN, 5, DMA_LISR_TEIF1, DMA_LISR_TCIF1, DMA_LISR_HTIF1, false, 0, 0, 0, 0, 0, rxdatatimerIdle, NULL, NULL, NULL, NULL, NULL, 0, txtransferNone, 0, 0, 0},
};

static double uart_character_rate_from_baud_rate(uint32_t rate, hal_uart_stop_bits_t stop_bits, hal_uart_parity_t parity);
static void uart_set_rx_interrupts(struct s_uart_pcb *uart, FunctionalState state);
static void uart_set_tx_interrupts(struct s_uart_pcb *uart, FunctionalState state);
static bool uart_get_dma_te_flag(struct s_uart_pcb *uart);
static bool uart_get_dma_ht_flag(struct s_uart_pcb *uart);
static bool uart_get_dma_tc_flag(struct s_uart_pcb *uart);
static void uart_clear_dma_te_flag(struct s_uart_pcb *uart);
static void uart_clear_dma_ht_flag(struct s_uart_pcb *uart);
static void uart_clear_dma_tc_flag(struct s_uart_pcb *uart);
static void uart_start_dma_rx(struct s_uart_pcb *uart);
static void uart_stop_dma_rx(struct s_uart_pcb *uart);
static bool uart_is_dma_rx_running(struct s_uart_pcb *uart);
static int uart_update_rx_buffer_from_isr(struct s_uart_pcb *uart, uint16_t dma_ndt);
static void uart_suspend_rx_from_isr(struct s_uart_pcb *uart, signed portBASE_TYPE *pxHigherPriorityTaskWoken);
static bool uart_update_rx_data_indication_from_isr(struct s_uart_pcb *uart, int bytes_transfered, bool rx_active, signed portBASE_TYPE *pxHigherPriorityTaskWoken);
static void uart_timer_rx_data_callback(hal_timer_handle_t handle);
static void uart_usart_irq_handler(struct s_uart_pcb *uart);
static void uart_usart_irq_handler_rx(struct s_uart_pcb *uart, uint16_t usart_SR, signed portBASE_TYPE *pxHigherPriorityTaskWoken);
static void uart_usart_irq_handler_tx(struct s_uart_pcb *uart, uint16_t usart_SR, signed portBASE_TYPE *pxHigherPriorityTaskWoken);
static void uart_dma_rx_irq_handler(struct s_uart_pcb *uart);

void halinternal_uart_init(void) {
	for (int i = 0; i < sizeof(uart_pcbs)/sizeof(uart_pcbs[0]); i++) {
		struct s_uart_pcb *uart = &(uart_pcbs[i]);
		/* Init hal timer for defered rx data indication */
		hal_timer_params_t rx_data_timer_params;
		rx_data_timer_params.userid = (void *)uart;
		rx_data_timer_params.callbackTimeout = uart_timer_rx_data_callback;
		uart->rx_data_timer = hal_timer_create(&rx_data_timer_params);
		/* Init semaphore used for tx transfer completion synchronization */
		uart->smphr_tx_complete = xSemaphoreCreateCounting(1, 0);
		/* Init DMA stream used for rx */
		RCC->AHB1ENR |= (uart->dma_rx_rcc_ahb1enr);
		uart->dma_rx_stream->CR = 0;
		uart->dma_rx_stream->CR |= uart->dma_rx_channel << POSITION_VAL(DMA_SxCR_CHSEL);
		uart->dma_rx_stream->CR |= DMA_SxCR_MINC;
		uart->dma_rx_stream->CR |= DMA_SxCR_CIRC;
		uart->dma_rx_stream->FCR = 0;
		uart->dma_rx_stream->PAR = (uint32_t)&(uart->usart->DR);
		/* Init NVIC interrupts of USART and rx DMA stream */
		halinternal_set_nvic_priority(uart->usart_irq_n);
		halinternal_set_nvic_priority(uart->dma_rx_stream_irq_n);
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
	SYS_ASSERT(xTaskGetSchedulerState() == taskSCHEDULER_RUNNING); // due to freertos semaphores usage

	int max_irq_service_delay = 0;
	if (!((params->hw_flow_control == huartHwFlowControl_Rx) || (params->hw_flow_control == huartHwFlowControl_Rx_Tx)))
		max_irq_service_delay = SYS_MAX_IRQ_LATENCY_MS;
	double character_rate = uart_character_rate_from_baud_rate(params->baud_rate, params->stop_bits, params->parity);
	// irq processing delay converted to bytes (space required to distinguish consecutive DMA HT and DMA TC flags, see irq handler)
	size_t extra_rx_buffer_space = ceil(character_rate*((double)(max(max_irq_service_delay, (params->rx_data_pending_interval + 1)))/1000));
	// extend half-buffer with extra space (required to mitigate system-level processing delays)
	total_rx_buffer_size = max(params->rx_buffer_size, extra_rx_buffer_space) + extra_rx_buffer_space;
	// round up to nearest even number (required to determine DMA half-transfer condition properly)
	total_rx_buffer_size += (total_rx_buffer_size % 2);

	SYS_ASSERT(total_rx_buffer_size <= MAX_DMA_TRANSFER_SIZE); // incompatible system characteristics and uart character rate

	struct {
		uint16_t BRR;
		uint16_t CR1;
		uint16_t CR2;
		uint16_t CR3;
		uint32_t tmpreg;
		uint32_t apbclock;
		uint32_t integerdivider;
		uint32_t fractionaldivider;
	} usart_init_struct = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
	halinternal_rcc_clocks_t rcc_clocks;
	SYS_ASSERT((params->baud_rate > 0) && (params->baud_rate < 7500001));
	if (params->hw_flow_control != huartHwFlowControl_None)
		SYS_ASSERT(IS_UART_HWFLOW_INSTANCE(uart->usart));
	switch (params->stop_bits) {
	case huartStopBits_0_5: usart_init_struct.CR2 |= USART_CR2_STOP_0; break;
	case huartStopBits_1: usart_init_struct.CR2 |= 0; break;
	case huartStopBits_1_5: usart_init_struct.CR2 |= USART_CR2_STOP_0 | USART_CR2_STOP_1; break;
	case huartStopBits_2: usart_init_struct.CR2 |= USART_CR2_STOP_1; break;
	default: SYS_ASSERT(0); break;
	}
	if (params->parity != huartParity_None)
		usart_init_struct.CR1 |= USART_CR1_M;
	switch (params->parity) {
	case huartParity_None: usart_init_struct.CR1 |= 0; break;
	case huartParity_Even: usart_init_struct.CR1 |= USART_CR1_PCE; break;
	case huartParity_Odd: usart_init_struct.CR1 |= USART_CR1_PCE | USART_CR1_PS; break;
	default: SYS_ASSERT(0); break;
	}
	usart_init_struct.CR1 |= USART_CR1_RE | USART_CR1_TE;
	switch (params->hw_flow_control) {
	case huartHwFlowControl_None: usart_init_struct.CR3 |= 0; break;
	case huartHwFlowControl_Rx: usart_init_struct.CR3 |= USART_CR3_RTSE; break;
	case huartHwFlowControl_Tx: usart_init_struct.CR3 |= USART_CR3_CTSE; break;
	case huartHwFlowControl_Rx_Tx: usart_init_struct.CR3 |= USART_CR3_RTSE | USART_CR3_CTSE; break;
	default: SYS_ASSERT(0); break;
	}
	halinternal_get_rcc_clocks(&rcc_clocks);
	if ((instance == 1) || (instance == 6))
		usart_init_struct.apbclock = rcc_clocks.pclk2_frequency;
	else
		usart_init_struct.apbclock = rcc_clocks.pclk1_frequency;
	if ((usart_init_struct.CR1 & USART_CR1_OVER8) != 0)
		usart_init_struct.integerdivider = ((25 * usart_init_struct.apbclock) / (2 * (params->baud_rate)));
	else
		usart_init_struct.integerdivider = ((25 * usart_init_struct.apbclock) / (4 * (params->baud_rate)));
	usart_init_struct.tmpreg = (usart_init_struct.integerdivider / 100) << 4;
	usart_init_struct.fractionaldivider = usart_init_struct.integerdivider - (100 * (usart_init_struct.tmpreg >> 4));
	if ((usart_init_struct.CR1 & USART_CR1_OVER8) != 0)
		usart_init_struct.tmpreg |= ((((usart_init_struct.fractionaldivider * 8) + 50) / 100)) & ((uint8_t)0x07);
	else
		usart_init_struct.tmpreg |= ((((usart_init_struct.fractionaldivider * 16) + 50) / 100)) & ((uint8_t)0x0F);
	usart_init_struct.BRR = (uint16_t)usart_init_struct.tmpreg;

	new_rx_buffer = hal_ringbuffer_create(total_rx_buffer_size, 0);
	if (rx_buffer)
		*rx_buffer = new_rx_buffer;
	if (params->tx_buffer_size > 0)
		new_tx_buffer = hal_ringbuffer_create(params->tx_buffer_size, 0);
	if (tx_buffer)
		*tx_buffer = new_tx_buffer;

	portDISABLE_INTERRUPTS();
	/* Mark peripheral instance as busy */
	SYS_ASSERT(uart->is_open == false);
	uart->is_open = true;
	/* Enable USART peripheral clock */
	if (uart->usart_apb == 1)
		RCC->APB1ENR |= (uart->usart_rcc_apb_mask);
	else if (uart->usart_apb == 2)
		RCC->APB2ENR |= (uart->usart_rcc_apb_mask);
	/* Init USART peripheral */
	uart->usart->CR2 = usart_init_struct.CR2;
	uart->usart->CR1 = usart_init_struct.CR1;
	uart->usart->CR3 = usart_init_struct.CR3;
	uart->usart->BRR = usart_init_struct.BRR;
	/* Init USART and DMA interrupts */
	NVIC_EnableIRQ(uart->usart_irq_n);
	NVIC_EnableIRQ(uart->dma_rx_stream_irq_n);
	uart->dma_rx_stream->CR |= DMA_SxCR_TEIE;
	portENABLE_INTERRUPTS();

	uart->userid = params->userid;
	uart->rx_buffer = new_rx_buffer;
	uart->tx_buffer = new_tx_buffer;
	uart->rx_buffer_extra_space = extra_rx_buffer_space;
	uart->rx_data_pending_interval = params->rx_data_pending_interval;
	uart->isrcallbackRxDataPending = params->isrcallbackRxDataPending;
	uart->isrcallbackRxDataErrors = params->isrcallbackRxDataErrors;
	uart->isrcallbackRxOverflowSuspended = params->isrcallbackRxOverflowSuspended;
	uart->isrcallbackTxSpacePending = params->isrcallbackTxSpacePending;
	uart->isrcallbackTxCompleted = params->isrcallbackTxCompleted;
	hal_uart_start_rx((hal_uart_handle_t)uart);
	xSemaphoreTake(uart->smphr_tx_complete, 0);
	uart->usart->CR1 |= USART_CR1_UE; // enable USART peripheral

	return (hal_uart_handle_t)uart;
}

void hal_uart_close(hal_uart_handle_t handle) {
	DEFINE_PCB_FROM_HANDLE(uart, handle)
	SYS_ASSERT(xTaskGetSchedulerState() == taskSCHEDULER_RUNNING); // due to freertos semaphores usage

	uart->rx_data_timer_state = rxdatatimerIdle;
	if (uart->rx_data_pending_interval > 0)
		hal_timer_stop(uart->rx_data_timer);
	portDISABLE_INTERRUPTS();
	uart->usart->CR1 &= ~USART_CR1_UE; // disable USART peripheral
	uart_stop_dma_rx(uart);
	uart_set_rx_interrupts(uart, DISABLE);
	if (uart->tx_transfer != txtransferNone)
		NVIC_SetPendingIRQ(uart->usart_irq_n); // force processing of transmission completion (interruption)
	uart->tx_transfer_data_ptr = 0;
	uart->tx_transfer_pending_size = 0;
	if (uart->tx_buffer)
		hal_ringbuffer_flush_read(uart->tx_buffer);
	uart_set_tx_interrupts(uart, DISABLE);
	portENABLE_INTERRUPTS();
	// usart tx interrupt (requested transmission interruption) activates right here (just after exit from critical section)

	if (uart->tx_buffer)
		hal_ringbuffer_delete(uart->tx_buffer);
	hal_ringbuffer_delete(uart->rx_buffer);
	uart->userid = 0;
	uart->rx_buffer = 0;
	uart->tx_buffer = 0;
	uart->rx_buffer_extra_space = 0;
	uart->rx_data_pending_interval = 0;
	uart->isrcallbackRxDataPending = NULL;
	uart->isrcallbackRxDataErrors = NULL;
	uart->isrcallbackRxOverflowSuspended = NULL;
	uart->isrcallbackTxSpacePending = NULL;
	uart->isrcallbackTxCompleted = NULL;

	portDISABLE_INTERRUPTS();
	/* Mark peripheral instance as not busy */
	SYS_ASSERT(uart->is_open == true);
	uart->is_open = false;
	/* Deinit DMA and USART interrupts */
	uart->dma_rx_stream->CR &= ~DMA_SxCR_TEIE;
	NVIC_DisableIRQ(uart->dma_rx_stream_irq_n);
	NVIC_DisableIRQ(uart->usart_irq_n);
	/* Reset USART peripheral */
	if (uart->usart_apb == 1) {
		RCC->APB1RSTR |= (uart->usart_rcc_apb_mask);
		RCC->APB1RSTR &= ~(uart->usart_rcc_apb_mask);
	} else if (uart->usart_apb == 2) {
		RCC->APB2RSTR |= (uart->usart_rcc_apb_mask);
		RCC->APB2RSTR &= ~(uart->usart_rcc_apb_mask);
	}
	/* Disable USART peripheral clock */
	if (uart->usart_apb == 1)
		RCC->APB1ENR &= ~(uart->usart_rcc_apb_mask);
	else if (uart->usart_apb == 2)
		RCC->APB2ENR &= ~(uart->usart_rcc_apb_mask);
	portENABLE_INTERRUPTS();
}

void* hal_uart_get_userid(hal_uart_handle_t handle) {
	DEFINE_PCB_FROM_HANDLE(uart, handle)
	return uart->userid;
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
	if (state == ENABLE) {
		uart->usart->CR1 |= USART_CR1_RXNEIE;
		uart->usart->CR1 |= USART_CR1_PEIE;
		uart->usart->CR3 |= USART_CR3_EIE;
	} else {
		uart->usart->CR1 &= ~USART_CR1_RXNEIE;
		uart->usart->CR1 &= ~USART_CR1_PEIE;
		uart->usart->CR3 &= ~USART_CR3_EIE;
	}
}

static void uart_set_tx_interrupts(struct s_uart_pcb *uart, FunctionalState state) {
	if (state == ENABLE) {
		uart->usart->CR1 |= USART_CR1_TXEIE;
		uart->usart->CR1 |= USART_CR1_TCIE;
	} else {
		uart->usart->CR1 &= ~USART_CR1_TXEIE;
		uart->usart->CR1 &= ~USART_CR1_TCIE;
	}
}

static bool uart_get_dma_te_flag(struct s_uart_pcb *uart) {
	bool status;
	if (uart->dma_rx_stream_number > 3)
		status = (uart->dma_rx->HISR & uart->dma_rx_teif_mask);
	else
		status = (uart->dma_rx->LISR & uart->dma_rx_teif_mask);
	return status;
}

static bool uart_get_dma_ht_flag(struct s_uart_pcb *uart) {
	bool status;
	if (uart->dma_rx_stream_number > 3)
		status = (uart->dma_rx->HISR & uart->dma_rx_htif_mask);
	else
		status = (uart->dma_rx->LISR & uart->dma_rx_htif_mask);
	return status;
}

static bool uart_get_dma_tc_flag(struct s_uart_pcb *uart) {
	bool status;
	if (uart->dma_rx_stream_number > 3)
		status = (uart->dma_rx->HISR & uart->dma_rx_tcif_mask);
	else
		status = (uart->dma_rx->LISR & uart->dma_rx_tcif_mask);
	return status;
}

static void uart_clear_dma_te_flag(struct s_uart_pcb *uart) {
	if (uart->dma_rx_stream_number > 3)
		uart->dma_rx->HIFCR = uart->dma_rx_teif_mask;
	else
		uart->dma_rx->LIFCR = uart->dma_rx_teif_mask;
}

static void uart_clear_dma_ht_flag(struct s_uart_pcb *uart) {
	if (uart->dma_rx_stream_number > 3)
		uart->dma_rx->HIFCR = uart->dma_rx_htif_mask;
	else
		uart->dma_rx->LIFCR = uart->dma_rx_htif_mask;
}

static void uart_clear_dma_tc_flag(struct s_uart_pcb *uart) {
	if (uart->dma_rx_stream_number > 3)
		uart->dma_rx->HIFCR = uart->dma_rx_tcif_mask;
	else
		uart->dma_rx->LIFCR = uart->dma_rx_tcif_mask;
}

static void uart_start_dma_rx(struct s_uart_pcb *uart) {
	uart_clear_dma_ht_flag(uart);
	uart_clear_dma_tc_flag(uart);
	uart->dma_rx_stream->CR |= DMA_SxCR_EN;
	uart->usart->CR3 |= USART_CR3_DMAR;
}

static void uart_stop_dma_rx(struct s_uart_pcb *uart) {
	uart->usart->CR3 &= ~USART_CR3_DMAR;
	uart->dma_rx_stream->CR &= ~DMA_SxCR_EN;
	while ((uart->dma_rx_stream->CR & DMA_SxCR_EN) != 0);
}

static bool uart_is_dma_rx_running(struct s_uart_pcb *uart) {
	return ((uart->dma_rx_stream->CR & DMA_SxCR_EN) != 0);
}

bool hal_uart_start_rx(hal_uart_handle_t handle) {
	DEFINE_PCB_FROM_HANDLE(uart, handle)
	bool success = false;
	portDISABLE_INTERRUPTS();
	if (!uart_is_dma_rx_running(uart)) {
		hal_ringbuffer_reset(uart->rx_buffer);
		/* Prepare memory address and size for DMA transfer */
		uint8_t *buffer_ptr;
		size_t buffer_size;
		hal_ringbuffer_get_write_ptr(uart->rx_buffer, &buffer_ptr, &buffer_size); // expected to be whole ringbuffer
		hal_ringbuffer_write_next(uart->rx_buffer, 0);
		uart->dma_rx_stream->M0AR = (uint32_t)buffer_ptr;
		uart->dma_rx_stream->NDTR = (uint16_t)buffer_size;
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
	portENABLE_INTERRUPTS();
	return success;
}

bool hal_uart_start_tx(hal_uart_handle_t handle) {
	DEFINE_PCB_FROM_HANDLE(uart, handle)
	if (!((uart->tx_buffer) && (uart->tx_transfer == txtransferNone)))
		return false;
	bool success = false;
	portDISABLE_INTERRUPTS();
	if (!hal_ringbuffer_is_empty(uart->tx_buffer)) {
		uart->tx_transfer = txtransferActiveInternal;
		uart_set_tx_interrupts(uart, ENABLE);
		success = true;
	}
	portENABLE_INTERRUPTS();
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
	portDISABLE_INTERRUPTS();
	buffer_ctrl = hal_ringbuffer_get_ctrl(uart->rx_buffer);
	portENABLE_INTERRUPTS();
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
	portDISABLE_INTERRUPTS();
	hal_ringbuffer_update_read_ctrl(uart->rx_buffer, &buffer_ctrl);
	portENABLE_INTERRUPTS();
	return result;
}

bool hal_uart_transmit_blocked(hal_uart_handle_t handle, uint8_t *data, int size, TickType_t block_time) {
	DEFINE_PCB_FROM_HANDLE(uart, handle)
	SYS_ASSERT(data);
	SYS_ASSERT(size > 0);
	if (!(uart->tx_transfer == txtransferNone))
		return false;
	portDISABLE_INTERRUPTS();
	uart->tx_transfer = txtransferActiveExternalSync;
	uart->tx_transfer_data_ptr = data;
	uart->tx_transfer_pending_size = size;
	uart_set_tx_interrupts(uart, ENABLE);
	portENABLE_INTERRUPTS();
	xSemaphoreTake(uart->smphr_tx_complete, block_time);
	return (uart->tx_transfer == txtransferNone);
}

bool hal_uart_start_transmit(hal_uart_handle_t handle, uint8_t *data, int size) {
	DEFINE_PCB_FROM_HANDLE(uart, handle)
	SYS_ASSERT(data);
	SYS_ASSERT(size > 0);
	if (!(uart->tx_transfer == txtransferNone))
		return false;
	portDISABLE_INTERRUPTS();
	uart->tx_transfer = txtransferActiveExternalAsync;
	uart->tx_transfer_data_ptr = data;
	uart->tx_transfer_pending_size = size;
	uart_set_tx_interrupts(uart, ENABLE);
	portENABLE_INTERRUPTS();
	return true;
}

static int uart_update_rx_buffer_from_isr(struct s_uart_pcb *uart, uint16_t dma_ndt) {
	int bytes_transfered;
	uint8_t *buffer_write_ptr;
	size_t buffer_chunk_size;
	int buffer_size = hal_ringbuffer_get_size(uart->rx_buffer);
	int buffer_space_bytes = hal_ringbuffer_get_free_space_size(uart->rx_buffer);
	uint8_t *buffer_start_addr = (uint8_t *)uart->dma_rx_stream->M0AR;

	/* Calculating bytes offset between advanced DMA position and current buffer write position.
	 * Handling of buffer wrapping is simplified because there are no sense to avoid "over-reading" due to possible critical buffer overflow.
	 * (So we assume that offset is always less than buffer size and it's not required to check borderline cases.)
	 */
	hal_ringbuffer_get_write_ptr(uart->rx_buffer, &buffer_write_ptr, &buffer_chunk_size);
	if (buffer_chunk_size > 0)
		hal_ringbuffer_write_next(uart->rx_buffer, 0);
	bytes_transfered = (int)((buffer_size - dma_ndt) - (buffer_write_ptr - buffer_start_addr));
	if (bytes_transfered < 0) // buffer end wrapped ?
		bytes_transfered += buffer_size;

	/* Update buffer write position according to transferred amount, but no more than space available (normal buffer overflow is possible here) */
	buffer_space_bytes = min(buffer_space_bytes, bytes_transfered);
	while (buffer_space_bytes > 0) {
		hal_ringbuffer_get_write_ptr(uart->rx_buffer, &buffer_write_ptr, &buffer_chunk_size);
		buffer_chunk_size = min(buffer_chunk_size, buffer_space_bytes);
		hal_ringbuffer_write_next(uart->rx_buffer, buffer_chunk_size);
		buffer_space_bytes -= buffer_chunk_size;
	}

	return bytes_transfered;
}

static void uart_suspend_rx_from_isr(struct s_uart_pcb *uart, signed portBASE_TYPE *pxHigherPriorityTaskWoken) {
	uart_set_rx_interrupts(uart, DISABLE);
	uart->rx_data_timer_state = rxdatatimerIdle;
	if (uart->rx_data_pending_interval > 0)
		hal_timer_stop(uart->rx_data_timer);
	if (uart_is_dma_rx_running(uart))
		uart_stop_dma_rx(uart);
}

/* For non-zero interval it controls timer scheduling, timeout processing and managing USART RXNE interrupt */
static bool uart_update_rx_data_indication_from_isr(struct s_uart_pcb *uart, int bytes_transfered, bool rx_active, signed portBASE_TYPE *pxHigherPriorityTaskWoken) {
	bool result = false;
	if ((uart->rx_data_pending_interval > 0) && rx_active) {
		if (uart->rx_data_timer_state == rxdatatimerExpired) {
			uart->rx_data_timer_state = rxdatatimerIdle;
			result = !hal_ringbuffer_is_empty(uart->rx_buffer);
		}
		if ((bytes_transfered > 0) && (uart->rx_data_timer_state != rxdatatimerActive)) {
			uart->usart->CR1 &= ~USART_CR1_RXNEIE; // disable rx interrupt at least for timer period and don't process active streaming
			uart->rx_data_timer_state = rxdatatimerActive;
			hal_timer_start(uart->rx_data_timer, uart->rx_data_pending_interval, pxHigherPriorityTaskWoken); // when timer expires it will trigger this processing again
		}
	} else {
		result = (bytes_transfered > 0);
	}
	return result;
}

static void uart_timer_rx_data_callback(hal_timer_handle_t handle) {
	DEFINE_PCB_FROM_HANDLE(uart, hal_timer_get_userid(handle))
	portDISABLE_INTERRUPTS();
	if ((uart->rx_data_timer_state == rxdatatimerActive) && (uart->usart->CR1 & USART_CR1_UE)) {
		uart->usart->CR1 |= USART_CR1_RXNEIE; // enable uart rx interrupt back again
		uart->rx_data_timer_state = rxdatatimerExpired;
		/* invoke rx handler to process rx data indication (with updated flag)
		 * this solution isn't perfect, but unified with timer-less method of indication
		 */
		NVIC_SetPendingIRQ(uart->usart_irq_n);
	}
	portENABLE_INTERRUPTS();
}

/* Note, that this handler may be triggered not only by USART hardware interrupt request, but also in software.
 * In particularly, it DOES NOT mean there are any hardware flag pending.
 */
static void uart_usart_irq_handler(struct s_uart_pcb *uart) {
	portBASE_TYPE xHigherPriorityTaskWoken = pdFALSE;
	/* USART status register must be read once in order to:
	 * - prevent loss of pending flags (such as known issue with PE);
	 * - prevent loss of events occurred while irq handler is active;
	 * - provide guaranteed TC bit clearing sequence (in sync with write access to DR register).
	 */
	uint16_t usart_SR = uart->usart->SR;
	if (uart->usart->CR1 & USART_CR1_UE) // uart enabled ?
		uart_usart_irq_handler_rx(uart, usart_SR, &xHigherPriorityTaskWoken);
	uart_usart_irq_handler_tx(uart, usart_SR, &xHigherPriorityTaskWoken);
	portEND_SWITCHING_ISR(xHigherPriorityTaskWoken);
}

static void uart_usart_irq_handler_rx(struct s_uart_pcb *uart, uint16_t usart_SR, signed portBASE_TYPE *pxHigherPriorityTaskWoken) {
	int bytes_transfered = 0;
	bool data_indication_required = false;
	bool errors_detected = false;
	bool overflow_detected = false;

	/* Process streaming if it's active */
	// warning: dependency on RXNEIE requires special handling in uart_update_rx_data_indication_from_isr() and uart_timer_rx_data_callback() !
	if ((uart->usart->CR1 & USART_CR1_RXNEIE) && (uart_is_dma_rx_running(uart))) {
		/* Capture DMA buffer state */
		bool flag_dma_ht = uart_get_dma_ht_flag(uart);
		bool flag_dma_tc = uart_get_dma_tc_flag(uart);
		uint16_t current_dma_ndt = (uint16_t)uart->dma_rx_stream->NDTR; // must be read after HT,TC flags
		if (flag_dma_ht)
			uart_clear_dma_ht_flag(uart);
		if (flag_dma_tc)
			uart_clear_dma_tc_flag(uart);
		/* Update(move) buffer write pointer (it's ok to do before check for any overflow) */
		bytes_transfered += uart_update_rx_buffer_from_isr(uart, current_dma_ndt);
		/* Check for buffer overflow (must be done after we already updated buffer write pointer due to racing with HT,TC flags) */
		if (hal_ringbuffer_get_free_space_size(uart->rx_buffer) <= uart->rx_buffer_extra_space) // at least normal buffer overflow
			overflow_detected = true;
		if ((flag_dma_ht && flag_dma_tc) || uart_get_dma_ht_flag(uart) || uart_get_dma_tc_flag(uart)) { // critical buffer overflow
			SYS_ASSERT(0); // critical overflow caused by system-level malfunction or configuration mismatch, buffer is corrupted (overwritten)
			overflow_detected = true;
			uart_stop_dma_rx(uart);
		}
	}

	/* Process possible failures in streaming if it's active */
	if (uart->usart->CR3 & USART_CR3_EIE) {
		errors_detected = usart_SR & (USART_SR_PE | USART_SR_FE | USART_SR_NE);
		overflow_detected |= (usart_SR & USART_SR_ORE) || (!uart_is_dma_rx_running(uart)); // both checks required (otherwise RXNE will cause interrupt remain pending)
		if (errors_detected)
			(void)uart->usart->DR; // dummy read required to reset flags (yes, there are races with DMA, but we are agree to loss one byte in such conditions)
		/* Process possible overflow condition in stream */
		if (overflow_detected) {
			if (uart_is_dma_rx_running(uart)) { // wasn't it forced to stop already ?
				/* update buffer with transferred chunk (if any) */
				bytes_transfered += uart_update_rx_buffer_from_isr(uart, (uint16_t)uart->dma_rx_stream->NDTR);
			}
			uart_suspend_rx_from_isr(uart, pxHigherPriorityTaskWoken);
		}
	}

	/* Process rx data indication conditions and state */
	data_indication_required = uart_update_rx_data_indication_from_isr(uart, bytes_transfered, (uart_is_dma_rx_running(uart)), pxHigherPriorityTaskWoken);

	/* Indicate user pending data errors in stream (must be after last transfer completed in order to account all possible error bytes) */
	if (errors_detected && uart->isrcallbackRxDataErrors)
		uart->isrcallbackRxDataErrors((hal_uart_handle_t)uart, hal_ringbuffer_get_pending_data_size(uart->rx_buffer), pxHigherPriorityTaskWoken);
	/* Indicate user transferred data (written in buffer) and possible stream suspension (overflow status) */
	if (data_indication_required && uart->isrcallbackRxDataPending)
		uart->isrcallbackRxDataPending((hal_uart_handle_t)uart, hal_ringbuffer_get_pending_data_size(uart->rx_buffer), pxHigherPriorityTaskWoken);
	if (overflow_detected && uart->isrcallbackRxOverflowSuspended)
		uart->isrcallbackRxOverflowSuspended((hal_uart_handle_t)uart, pxHigherPriorityTaskWoken);
}

static void uart_usart_irq_handler_tx(struct s_uart_pcb *uart, uint16_t usart_SR, signed portBASE_TYPE *pxHigherPriorityTaskWoken) {
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
	if (more_data_pending && (usart_SR & USART_SR_TXE)) { // warning: pending data in buffer must be flushed when uart disabled
		// warning: here is the only place to write DR register (see uart_usart_irq_handler)
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
	} else if ((uart->tx_transfer != txtransferNone) && ((usart_SR & USART_SR_TC) || !(uart->usart->CR1 & USART_CR1_UE))) {
		// warning:	such conditions and checks above are sensible to TC bit clearing sequence and transmission interruption
		// 			(see uart_usart_irq_handler and hal_uart_close)
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

static void uart_dma_rx_irq_handler(struct s_uart_pcb *uart) {
	portBASE_TYPE xHigherPriorityTaskWoken = pdFALSE;
	/* DMA transfer error (bus error ?) */
	if (uart_get_dma_te_flag(uart)) {
		uart_suspend_rx_from_isr(uart, &xHigherPriorityTaskWoken);
		uart_clear_dma_te_flag(uart);
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
