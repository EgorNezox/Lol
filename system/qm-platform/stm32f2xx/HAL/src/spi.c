/**
  ******************************************************************************
  * @file    spi.c
  * @author  Artem Pisarenko, PMR dept. software team, ONIIP, PJSC
  * @date    10.02.2016
  * @brief   Реализация аппаратной абстракции доступа к SPI на STM32F2xx
  *
  ******************************************************************************
  */

#include <math.h>
#include "stm32f2xx.h"
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

#include "sys_internal.h"
#include "hal_rcc.h"
#include "hal_spi.h"

#define SPI_INSTANCES_COUNT 3

/* структура конфигурации spi шины */
static struct s_spibus_pcb
{
	/* указатель на структуру spi */
	SPI_TypeDef* spi;

	/* тип прерывания */
	IRQn_Type  irq_n;

	/* маска для предделителя  */
	uint32_t rcc_apb_mask;

	/* семафоры для синхронизации работы по spi */
	SemaphoreHandle_t mutex;
	SemaphoreHandle_t smphr_transfer_sync;

	/* размер данных по spi */
	hal_spi_data_size_t mt_data_size;

	/* указатель на приемный буфер и размер буфера */
	uint8_t *mt_rx_ptr;
	int      mt_rx_count;

	/* указатель на передающий буфер и размер буфера */
	uint8_t *mt_tx_ptr;
	int      mt_tx_count;

} /*создаем 3 spi шины для stm32 */
spibus_pcbs[SPI_INSTANCES_COUNT] =
{
	{SPI1, SPI1_IRQn, RCC_APB2ENR_SPI1EN, 0, 0, 0, 0, 0, 0, 0},
	{SPI2, SPI2_IRQn, RCC_APB1ENR_SPI2EN, 0, 0, 0, 0, 0, 0, 0},
	{SPI3, SPI3_IRQn, RCC_APB1ENR_SPI3EN, 0, 0, 0, 0, 0, 0, 0}
};

/* значение битрейта для шины spi */
static uint32_t spi_max_baud_rate_value;

/*вектор прерываний, размещенный по адресу стуктуры */
static inline void spi_irq_handler(struct s_spibus_pcb *spibus) __attribute__((optimize("-O0"),always_inline));

/* функция для инициализации шины spi */
void halinternal_spi_init(void)
{
	/* установка значения spi битрейта */
	spi_max_baud_rate_value = SPI_CR1_BR >> POSITION_VAL(SPI_CR1_BR);

	/* установка шины APB2 */
	RCC->APB2RSTR |= RCC_APB2RSTR_SPI1RST;
	RCC->APB2RSTR &= ~RCC_APB2RSTR_SPI1RST;
	RCC->APB2ENR  &= ~RCC_APB2ENR_SPI1EN;

	/* установка шины APB1 */
	RCC->APB1RSTR |=  (RCC_APB1RSTR_SPI2RST | RCC_APB1RSTR_SPI3RST);
	RCC->APB1RSTR &= ~(RCC_APB1RSTR_SPI2RST | RCC_APB1RSTR_SPI3RST);
	RCC->APB1ENR  &= ~(RCC_APB1ENR_SPI2EN   | RCC_APB1ENR_SPI3EN);

	/* обеспечение завершения доступа к памяти */
	__DSB();

	/* пройтись по всем шинам spi, определить прерывания и семафоры с мьютексами */
	for (int i = 0; i < sizeof(spibus_pcbs)/sizeof(spibus_pcbs[0]); i++)
	{
		struct s_spibus_pcb *spibus = &(spibus_pcbs[i]);
		spibus->mutex = xSemaphoreCreateMutex();

		spibus->smphr_transfer_sync = xSemaphoreCreateBinary();
		halinternal_set_nvic_priority(spibus->irq_n);

		NVIC_ClearPendingIRQ(spibus->irq_n);
		NVIC_EnableIRQ      (spibus->irq_n);
	}
}

/* первичная инициализация структуры spi */
void hal_spi_init_master_transfer_struct(struct hal_spi_master_transfer_t *t)
{
	t->max_baud_rate = 0;
	t->cpha = hspiCPHA0;
	t->cpol = hspiCPOL0;
	t->data_size = hspiDataSize8bit;
	t->first_bit = hspiFirstMSB;
	t->rx_buffer = 0;
	t->tx_buffer = 0;
	t->buffer_size = 0;
}

/* функция отправки по spi */
bool hal_spi_master_fd_transfer(int bus_instance, struct hal_spi_master_transfer_t *t)
{

	struct s_spibus_pcb *spibus = &(spibus_pcbs[bus_instance-1]);

	/* проверка номера шины, по которой идет общение */
	SYS_ASSERT((1 <= bus_instance) && (bus_instance <= SPI_INSTANCES_COUNT));

	/* проверка на положительный размер буфера */
	SYS_ASSERT(t->buffer_size > 0);

	/* проверка на состояние планировщика  */
	SYS_ASSERT(xTaskGetSchedulerState() == taskSCHEDULER_RUNNING); // due to freertos mutex usage

	/*структура для инициализации spi */
	struct
	{
		/* регистр записи-чтения spi  */
		uint32_t CR1;
		/* битрейт */
		uint32_t br_value;
	} init_struct;

	/* размер отправки */
	int transfer_count;

	/* ставим роль spi: мастер, управляемый или изменяемый */
	init_struct.CR1 = SPI_CR1_MSTR | SPI_CR1_SSI | SPI_CR1_SSM;

	/* получаем размер посылки - 8/16 бит */
	switch (t->data_size)
	{
	case hspiDataSize8bit:
		transfer_count = t->buffer_size;
		break;
	case hspiDataSize16bit:
		SYS_ASSERT((t->buffer_size % 2) == 0);
		transfer_count = t->buffer_size / 2;
		init_struct.CR1 |= SPI_CR1_DFF;
		break;
	default:
		transfer_count = 0;
		SYS_ASSERT(0);
	}

	/* ставим фазу сигнала spi: передний или задний фронт */
	switch (t->cpha)
	{
	case hspiCPHA0:
		break;
	case hspiCPHA1:
		init_struct.CR1 |= SPI_CR1_CPHA;
		break;
	default: SYS_ASSERT(0);
	}

	/* ставим синхронизацию по низкому или высокому уровню */
	switch (t->cpol)
	{
	case hspiCPOL0:
		break;
	case hspiCPOL1:
		init_struct.CR1 |= SPI_CR1_CPOL;
		break;
	default: SYS_ASSERT(0);
	}

	/* устанавливаем порядок бит */
	switch (t->first_bit)
	{
	case hspiFirstMSB:
		break;
	case hspiFirstLSB:
		init_struct.CR1 |= SPI_CR1_LSBFIRST;
		break;
	default: SYS_ASSERT(0);
	}

	/* если битрейт не нулевой  */
	if (t->max_baud_rate != 0)
	{
		/* получаем  */
		hal_rcc_clocks_t rcc_clocks;
		uint32_t    *pclk_frequency;

		uint32_t doubled_max_baud_rate, divider;

		/* получаем рабочую частоту */
		hal_rcc_get_clocks(&rcc_clocks);

		/* если работаем по 12 spi берем одну частоту, по другим иную */
		if (bus_instance == 1)
			pclk_frequency = &rcc_clocks.pclk2_frequency;
		else
			pclk_frequency = &rcc_clocks.pclk1_frequency;

		/*  получаем предделитель */
		doubled_max_baud_rate = 2 * t->max_baud_rate;
		divider = (*pclk_frequency)/doubled_max_baud_rate;

		/* если делитель больше нуля */
		if (divider > 0)
		{
			init_struct.br_value = 31 - __CLZ(divider);

			if (((divider & ((1 << init_struct.br_value) - 1)) != 0) ||
				((divider * doubled_max_baud_rate) < (*pclk_frequency)))
				init_struct.br_value++;

			init_struct.br_value = min(init_struct.br_value, spi_max_baud_rate_value);
		}
		else
		{
			init_struct.br_value = 0;
		}
	}
	else
	{
		init_struct.br_value = spi_max_baud_rate_value;
	}

	init_struct.CR1 |= init_struct.br_value << POSITION_VAL(SPI_CR1_BR);

	/* забираем время на установку действий*/
	xSemaphoreTake(spibus->mutex, portMAX_DELAY);

	/* приемный и передающий буфер устанавливаем */
	spibus->mt_data_size = t->data_size;
	spibus->mt_rx_ptr    = t->rx_buffer;
	spibus->mt_rx_count  = transfer_count;

	spibus->mt_tx_ptr    = t->tx_buffer;
	spibus->mt_tx_count  = transfer_count;

	/* включаем предделитель  */
	if (bus_instance == 1)
		RCC->APB2ENR |= (spibus->rcc_apb_mask);
	else
		RCC->APB1ENR |= (spibus->rcc_apb_mask);

	/* записать данные и послать прерывание */
	spibus->spi->CR1 = init_struct.CR1;
	spibus->spi->CR2 = (SPI_CR2_TXEIE | SPI_CR2_RXNEIE);

	spibus->spi->CR1 |= SPI_CR1_SPE;
	xSemaphoreTake(spibus->smphr_transfer_sync, portMAX_DELAY);

	SYS_ASSERT((spibus->spi->SR & SPI_SR_BSY) == 0);
	spibus->spi->CR1 &= ~SPI_CR1_SPE;

	if (bus_instance == 1)
		RCC->APB2ENR &= ~(spibus->rcc_apb_mask);
	else
		RCC->APB1ENR &= ~(spibus->rcc_apb_mask);

	/* отдать управление */
	xSemaphoreGive(spibus->mutex);

	return true;
}

static void spi_irq_handler(struct s_spibus_pcb *spibus)
{
	portBASE_TYPE xHigherPriorityTaskWoken = pdFALSE;
	/*получить стату spi*/
	uint32_t SR_value = spibus->spi->SR;
	uint16_t data_reg_value;

	/* если ошибок приема нет и размер обнаруженных данных больше нуля */
	if (((SR_value & SPI_SR_RXNE) != 0) && (spibus->mt_rx_count > 0))
	{
		/* получить регистр с данными  */
		data_reg_value = spibus->spi->DR;

		/*если приемный буфер определен */
		if (spibus->mt_rx_ptr)
		{
			/* если размер данных равен 8 бит */
			if (spibus->mt_data_size == hspiDataSize8bit)
			{
				/* записать данные по указателю и накопить указатель */
				*((uint8_t *)spibus->mt_rx_ptr) = data_reg_value;
				spibus->mt_rx_ptr++;
			}
			else
			{
				/* записать данные по указателю и накопить указатель на 2 */
				*((uint16_t *)spibus->mt_rx_ptr) = data_reg_value;
				spibus->mt_rx_ptr += 2;
			}
		}

		/* уменьшить размер буфера */
		spibus->mt_rx_count--;

		/* если буфер заполнен */
		if (spibus->mt_rx_count == 0)
		{
			/* генерируем прерывание и запускаем сообщение с данными */
			spibus->spi->CR2 &= ~SPI_CR2_RXNEIE;
			xSemaphoreGiveFromISR(spibus->smphr_transfer_sync, &xHigherPriorityTaskWoken);
		}
	}

	/* если передача прошла без ошибок */
	if (((SR_value & SPI_SR_TXE) != 0)  &&
	(spibus->mt_tx_count >= spibus->mt_rx_count) && (spibus->mt_tx_count > 0))
	{
		/*проверяем буфер передачи */
		if (spibus->mt_tx_ptr)
		{
			/* если размер данных равен 8 бит */
			if (spibus->mt_data_size == hspiDataSize8bit)
			{
				/* записать данные по указателю и накопить указатель */
				data_reg_value = *((uint8_t *)spibus->mt_tx_ptr);
				spibus->mt_tx_ptr++;
			}
			else
			{
				/* записать данные по указателю и накопить указатель на 2 */
				data_reg_value = *((uint16_t *)spibus->mt_tx_ptr);
				spibus->mt_tx_ptr += 2;
			}
		}
		else
		{
			/* установить регистр с данными с ошибочной маской */
			data_reg_value = 0xFFFF;
		}

		/* уменьшить размер передающего буфера */
		spibus->mt_tx_count--;

	    /* записать в регистр данных значение данных */
		spibus->spi->DR = data_reg_value;

		/* если передатчик заполнен, то генерируем прерывание передачи */
		if (spibus->mt_tx_count == 0)
			spibus->spi->CR2 &= ~SPI_CR2_TXEIE;
	}

	/* переключение контекста на приоритетную задачу приложения */
	portEND_SWITCHING_ISR(xHigherPriorityTaskWoken);
}


/* обработка прерывания от 1 spi */
void SPI1_IRQHandler(void)
{
	spi_irq_handler(&spibus_pcbs[0]);
}

/* обработка прерывания от 2 spi */
void SPI2_IRQHandler(void)
{
	spi_irq_handler(&spibus_pcbs[1]);
}

/* обработка прерывания от 3 spi */
void SPI3_IRQHandler(void)
{
	spi_irq_handler(&spibus_pcbs[2]);
}
