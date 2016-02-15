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
#include "hal_spi.h"

#define SPI_INSTANCES_COUNT 3

static struct s_spibus_pcb {
	SPI_TypeDef* spi;
	IRQn_Type irq_n;
	uint32_t rcc_apb_mask;
	SemaphoreHandle_t mutex;
	SemaphoreHandle_t smphr_transfer_sync;
	hal_spi_data_size_t mt_data_size;
	uint8_t *mt_rx_ptr;
	int mt_rx_count;
	uint8_t *mt_tx_ptr;
	int mt_tx_count;
} spibus_pcbs[SPI_INSTANCES_COUNT] = {
	{SPI1, SPI1_IRQn, RCC_APB2ENR_SPI1EN, 0, 0, 0, 0, 0, 0, 0},
	{SPI2, SPI2_IRQn, RCC_APB1ENR_SPI2EN, 0, 0, 0, 0, 0, 0, 0},
	{SPI3, SPI3_IRQn, RCC_APB1ENR_SPI3EN, 0, 0, 0, 0, 0, 0, 0}
};
static uint32_t spi_max_baud_rate_value;

static void spi_irq_handler(struct s_spibus_pcb *spibus);

void halinternal_spi_init(void) {
	spi_max_baud_rate_value = SPI_CR1_BR >> POSITION_VAL(SPI_CR1_BR);
	for (int i = 0; i < sizeof(spibus_pcbs)/sizeof(spibus_pcbs[0]); i++) {
		struct s_spibus_pcb *spibus = &(spibus_pcbs[i]);
		spibus->mutex = xSemaphoreCreateMutex();
		spibus->smphr_transfer_sync = xSemaphoreCreateBinary();
		halinternal_set_nvic_priority(spibus->irq_n);
		NVIC_EnableIRQ(spibus->irq_n);
	}
}

void hal_spi_init_master_transfer_struct(struct hal_spi_master_transfer_t *t) {
	t->max_baud_rate = 0;
	t->cpha = hspiCPHA0;
	t->cpol = hspiCPOL0;
	t->data_size = hspiDataSize8bit;
	t->first_bit = hspiFirstMSB;
	t->rx_buffer = 0;
	t->tx_buffer = 0;
	t->buffer_size = 0;
}

bool hal_spi_master_fd_transfer(int bus_instance, struct hal_spi_master_transfer_t *t) {
	struct s_spibus_pcb *spibus = &(spibus_pcbs[bus_instance-1]);

	SYS_ASSERT((1 <= bus_instance) && (bus_instance <= SPI_INSTANCES_COUNT));
	SYS_ASSERT(t->buffer_size > 0);
	SYS_ASSERT(xTaskGetSchedulerState() == taskSCHEDULER_RUNNING); // due to freertos mutex usage

	struct {
		uint32_t CR1;
		uint32_t CR2;
		uint32_t br_value;
	} init_struct = {0x00, 0x00, 0};
	int transfer_count = 0;
	init_struct.CR1 |= SPI_CR1_MSTR | SPI_CR1_SSI | SPI_CR1_SSM;
	switch (t->data_size) {
	case hspiDataSize8bit:
		transfer_count = t->buffer_size;
		break;
	case hspiDataSize16bit:
		SYS_ASSERT((t->buffer_size % 2) == 0);
		transfer_count = t->buffer_size / 2;
		init_struct.CR1 |= SPI_CR1_DFF;
		break;
	default: SYS_ASSERT(0);
	}
	switch (t->cpha) {
	case hspiCPHA0:
		break;
	case hspiCPHA1:
		init_struct.CR1 |= SPI_CR1_CPHA;
		break;
	default: SYS_ASSERT(0);
	}
	switch (t->cpol) {
	case hspiCPOL0:
		break;
	case hspiCPOL1:
		init_struct.CR1 |= SPI_CR1_CPOL;
		break;
	default: SYS_ASSERT(0);
	}
	switch (t->first_bit) {
	case hspiFirstMSB:
		break;
	case hspiFirstLSB:
		init_struct.CR1 |= SPI_CR1_LSBFIRST;
		break;
	default: SYS_ASSERT(0);
	}
	if (t->max_baud_rate != 0) {
		halinternal_rcc_clocks_t rcc_clocks;
		uint32_t *pclk_frequency;
		uint32_t divider;
		halinternal_get_rcc_clocks(&rcc_clocks);
		if (bus_instance == 1)
			pclk_frequency = &rcc_clocks.pclk2_frequency;
		else
			pclk_frequency = &rcc_clocks.pclk1_frequency;
		divider = ceil((double)(*pclk_frequency)/(double)(2 * t->max_baud_rate));
		init_struct.br_value = 31 - __CLZ(divider);
		if ((divider & ((1 << init_struct.br_value) - 1)) != 0)
			init_struct.br_value++;
		init_struct.br_value = min(init_struct.br_value, spi_max_baud_rate_value);
	} else {
		init_struct.br_value = spi_max_baud_rate_value;
	}
	init_struct.CR1 |= init_struct.br_value << POSITION_VAL(SPI_CR1_BR);
	init_struct.CR2 |= (SPI_CR2_TXEIE | SPI_CR2_RXNEIE);

	xSemaphoreTake(spibus->mutex, portMAX_DELAY);

	xSemaphoreTake(spibus->smphr_transfer_sync, 0);
	spibus->mt_data_size = t->data_size;
	spibus->mt_rx_ptr = t->rx_buffer;
	spibus->mt_rx_count = transfer_count;
	spibus->mt_tx_ptr = t->tx_buffer;
	spibus->mt_tx_count = transfer_count;

	if (bus_instance == 1)
		RCC->APB2ENR |= (spibus->rcc_apb_mask);
	else
		RCC->APB1ENR |= (spibus->rcc_apb_mask);
	spibus->spi->CR1 = init_struct.CR1;
	spibus->spi->CR2 = init_struct.CR2;

	spibus->spi->CR1 |= SPI_CR1_SPE;
	xSemaphoreTake(spibus->smphr_transfer_sync, portMAX_DELAY);
	SYS_ASSERT((spibus->spi->SR & SPI_SR_BSY) == 0);
	spibus->spi->CR1 &= ~SPI_CR1_SPE;

	if (bus_instance == 1)
		RCC->APB2ENR &= ~(spibus->rcc_apb_mask);
	else
		RCC->APB1ENR &= ~(spibus->rcc_apb_mask);

	xSemaphoreGive(spibus->mutex);

	return true;
}

static void spi_irq_handler(struct s_spibus_pcb *spibus) {
	portBASE_TYPE xHigherPriorityTaskWoken = pdFALSE;
	uint16_t data_reg_value;
	if (((spibus->spi->SR & SPI_SR_RXNE) != 0) && (spibus->mt_rx_count > 0)) {
		data_reg_value = spibus->spi->DR;
		if (spibus->mt_rx_ptr) {
			switch (spibus->mt_data_size) {
			case hspiDataSize8bit:
				*((uint8_t *)spibus->mt_rx_ptr) = data_reg_value;
				spibus->mt_rx_ptr++;
				break;
			case hspiDataSize16bit:
				*((uint16_t *)spibus->mt_rx_ptr) = data_reg_value;
				spibus->mt_rx_ptr += 2;
				break;
			}
		}
		spibus->mt_rx_count--;
		if (spibus->mt_rx_count == 0) {
			spibus->spi->CR2 &= ~SPI_CR2_RXNEIE;
			xSemaphoreGiveFromISR(spibus->smphr_transfer_sync, &xHigherPriorityTaskWoken);
		}
	}
	if (((spibus->spi->SR & SPI_SR_TXE) != 0) && (spibus->mt_tx_count > 0)) {
		if (spibus->mt_tx_ptr) {
			switch (spibus->mt_data_size) {
			case hspiDataSize8bit:
				data_reg_value = *((uint8_t *)spibus->mt_tx_ptr);
				spibus->mt_tx_ptr++;
				break;
			case hspiDataSize16bit:
				data_reg_value = *((uint16_t *)spibus->mt_tx_ptr);
				spibus->mt_tx_ptr += 2;
				break;
			}
		} else {
			data_reg_value = 0xFFFF;
		}
		spibus->mt_tx_count--;
		spibus->spi->DR = data_reg_value;
		if (spibus->mt_tx_count == 0)
			spibus->spi->CR2 &= ~SPI_CR2_TXEIE;
	}
	portEND_SWITCHING_ISR(xHigherPriorityTaskWoken);
}

void SPI1_IRQHandler(void) {
	spi_irq_handler(&spibus_pcbs[0]);
}
void SPI2_IRQHandler(void) {
	spi_irq_handler(&spibus_pcbs[1]);
}
void SPI3_IRQHandler(void) {
	spi_irq_handler(&spibus_pcbs[2]);
}
