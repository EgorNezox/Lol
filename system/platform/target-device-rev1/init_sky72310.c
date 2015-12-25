/**
  ******************************************************************************
  * @file    init_sky72310.c
  * @author  Artem Pisarenko, PMR dept. software team, ONIIP, PJSC
  * @date    25.12.2015
  *
  ******************************************************************************
 */

#include "stm32f2xx_hal.h"

#define SPIx                             SPI2
#define SPIx_CLK_ENABLE()                __HAL_RCC_SPI2_CLK_ENABLE()
#define SPIx_SCK_GPIO_CLK_ENABLE()       __HAL_RCC_GPIOI_CLK_ENABLE()
#define SPIx_MOSI_GPIO_CLK_ENABLE()      __HAL_RCC_GPIOI_CLK_ENABLE()
#define SPIx_FORCE_RESET()               __HAL_RCC_SPI2_FORCE_RESET()
#define SPIx_RELEASE_RESET()             __HAL_RCC_SPI2_RELEASE_RESET()

/* Definition for SPIx Pins */
#define SPIx_SCK_PIN                     GPIO_PIN_1
#define SPIx_SCK_GPIO_PORT               GPIOI
#define SPIx_SCK_AF                      GPIO_AF5_SPI2
#define SPIx_MOSI_PIN                    GPIO_PIN_3
#define SPIx_MOSI_GPIO_PORT              GPIOI
#define SPIx_MOSI_AF                     GPIO_AF5_SPI2

#define CS_PLL_GPIO_CLK_ENABLE()      __HAL_RCC_GPIOH_CLK_ENABLE()
#define CS_PLL_PIN                     GPIO_PIN_15
#define CS_PLL_GPIO_PORT               GPIOH

extern char sky72310_stm32f2cube_active;
static SPI_HandleTypeDef SpiHandle;

static void write_reg(uint8_t address, uint16_t value) {
	uint16_t command = ((address & 0x0F) << 12) | (value & 0x0FFF);
	HAL_GPIO_WritePin(CS_PLL_GPIO_PORT, CS_PLL_PIN, GPIO_PIN_RESET);
	HAL_Delay(1);
	HAL_SPI_TransmitReceive(&SpiHandle, (uint8_t *)&command, (uint8_t *)&command, 1, 1000);
	HAL_Delay(1);
	HAL_GPIO_WritePin(CS_PLL_GPIO_PORT, CS_PLL_PIN, GPIO_PIN_SET);
}

void init_sky72310(void) {
	sky72310_stm32f2cube_active = 1;

	HAL_Init();
	SpiHandle.Instance               = SPIx;
	SpiHandle.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_256;
	SpiHandle.Init.Direction         = SPI_DIRECTION_2LINES;
	SpiHandle.Init.CLKPhase          = SPI_PHASE_1EDGE;
	SpiHandle.Init.CLKPolarity       = SPI_POLARITY_LOW;
	SpiHandle.Init.CRCCalculation    = SPI_CRCCALCULATION_DISABLE;
	SpiHandle.Init.CRCPolynomial     = 7;
	SpiHandle.Init.DataSize          = SPI_DATASIZE_16BIT;
	SpiHandle.Init.FirstBit          = SPI_FIRSTBIT_MSB;
	SpiHandle.Init.NSS               = SPI_NSS_SOFT;
	SpiHandle.Init.TIMode            = SPI_TIMODE_DISABLE;
	SpiHandle.Init.Mode = SPI_MODE_MASTER;
	HAL_SPI_Init(&SpiHandle);

	write_reg(0x07, 0x0088);
	write_reg(0x05, 0x0019);
	write_reg(0x06, 0x0000);
	write_reg(0x08, 0x0200);
	write_reg(0x00, 0x0160);
	write_reg(0x01, 0x0000);
	write_reg(0x02, 0x0000);

	HAL_SPI_DeInit(&SpiHandle);
	HAL_DeInit();

	sky72310_stm32f2cube_active = 0;
}

void HAL_SPI_MspInit(SPI_HandleTypeDef *hspi)
{
GPIO_InitTypeDef  GPIO_InitStruct;

  if (hspi->Instance == SPIx)
  {
    /*##-1- Enable peripherals and GPIO Clocks #################################*/
    /* Enable GPIO TX/RX clock */
    SPIx_SCK_GPIO_CLK_ENABLE();
    SPIx_MOSI_GPIO_CLK_ENABLE();
    /* Enable SPI clock */
    SPIx_CLK_ENABLE();

    CS_PLL_GPIO_CLK_ENABLE();

    /*##-2- Configure peripheral GPIO ##########################################*/
    /* SPI SCK GPIO pin configuration  */
    GPIO_InitStruct.Pin       = SPIx_SCK_PIN;
    GPIO_InitStruct.Mode      = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull      = GPIO_PULLUP;
    GPIO_InitStruct.Speed     = GPIO_SPEED_FAST;
    GPIO_InitStruct.Alternate = SPIx_SCK_AF;

    HAL_GPIO_Init(SPIx_SCK_GPIO_PORT, &GPIO_InitStruct);

    /* SPI MOSI GPIO pin configuration  */
    GPIO_InitStruct.Pin = SPIx_MOSI_PIN;
    GPIO_InitStruct.Alternate = SPIx_MOSI_AF;

    HAL_GPIO_Init(SPIx_MOSI_GPIO_PORT, &GPIO_InitStruct);

    /* CS_PLL GPIO pin configuration */
    HAL_GPIO_WritePin(CS_PLL_GPIO_PORT, CS_PLL_PIN, GPIO_PIN_SET);

    GPIO_InitStruct.Pin       = CS_PLL_PIN;
    GPIO_InitStruct.Mode      = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull      = GPIO_PULLUP;
    GPIO_InitStruct.Speed     = GPIO_SPEED_FAST;

    HAL_GPIO_Init(CS_PLL_GPIO_PORT, &GPIO_InitStruct);
  }
}

void HAL_SPI_MspDeInit(SPI_HandleTypeDef *hspi)
{
  if (hspi->Instance == SPIx)
  {
    /*##-1- Reset peripherals ##################################################*/
    SPIx_FORCE_RESET();
    SPIx_RELEASE_RESET();

    /*##-2- Disable peripherals and GPIO Clocks ################################*/
    /* Configure SPI SCK as alternate function  */
    HAL_GPIO_DeInit(SPIx_SCK_GPIO_PORT, SPIx_SCK_PIN);
    /* Configure SPI MOSI as alternate function  */
    HAL_GPIO_DeInit(SPIx_MOSI_GPIO_PORT, SPIx_MOSI_PIN);
    /* Configure CS_PLL as input with pull-up */
    GPIO_InitTypeDef  GPIO_InitStruct;
    GPIO_InitStruct.Pin       = CS_PLL_PIN;
    GPIO_InitStruct.Mode      = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull      = GPIO_PULLUP;
    GPIO_InitStruct.Speed     = GPIO_SPEED_FAST;
    HAL_GPIO_Init(CS_PLL_GPIO_PORT, &GPIO_InitStruct);
  }
}
