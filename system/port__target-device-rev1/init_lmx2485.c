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

static SPI_HandleTypeDef SpiHandle;

static void write_data(uint8_t value1, uint8_t value2, uint8_t value3)
{
	uint8_t command1 = value1;
	uint8_t command2 = value2;
	uint8_t command3 = value3;

	HAL_GPIO_WritePin(CS_PLL_GPIO_PORT, CS_PLL_PIN, GPIO_PIN_RESET);

	HAL_Delay(1);
	HAL_SPI_TransmitReceive(&SpiHandle, (uint8_t *)&command1, (uint8_t *)&command1, 1, 1000);
	HAL_Delay(1);
	HAL_SPI_TransmitReceive(&SpiHandle, (uint8_t *)&command2, (uint8_t *)&command2, 1, 1000);
	HAL_Delay(1);
	HAL_SPI_TransmitReceive(&SpiHandle, (uint8_t *)&command3, (uint8_t *)&command3, 1, 1000);
	HAL_Delay(1);

	HAL_GPIO_WritePin(CS_PLL_GPIO_PORT, CS_PLL_PIN, GPIO_PIN_SET);
}

void init_lmx2485(void)
{
	SpiHandle.Instance               = SPIx;
	SpiHandle.State                  = HAL_SPI_STATE_RESET;
	SpiHandle.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_256;
	SpiHandle.Init.Direction         = SPI_DIRECTION_2LINES;
	SpiHandle.Init.CLKPhase          = SPI_PHASE_1EDGE;
	SpiHandle.Init.CLKPolarity       = SPI_POLARITY_LOW;
	SpiHandle.Init.CRCCalculation    = SPI_CRCCALCULATION_DISABLE;
	SpiHandle.Init.CRCPolynomial     = 7;
	SpiHandle.Init.DataSize          = SPI_DATASIZE_8BIT;
	SpiHandle.Init.FirstBit          = SPI_FIRSTBIT_MSB;
	SpiHandle.Init.NSS               = SPI_NSS_SOFT;
	SpiHandle.Init.TIMode            = SPI_TIMODE_DISABLE;
	SpiHandle.Init.Mode              = SPI_MODE_MASTER;

	HAL_SPI_Init(&SpiHandle);

	write_data(0x30, 0x00, 0x00); // R0 	RF_N[10:0] RF_FN[11:0]                                                               0
	write_data(0x0D, 0x10, 0x03); // R1 	RF_PD RF_P RF_R[5:0] RF_FD[11:0]   											   0 0 1 1
	write_data(0x80, 0xF0, 0xF5); // R2 	IF_PD IF_N[18:0]                   											   0 1 0 1
	write_data(0x1F, 0xF0, 0x07); // R3 	ACCESS[3:0] RF_CPG[3:0] IF_R[11:0] 											   0 1 1 1
	write_data(0x20, 0xC7, 0xF9); // R4 	ATPU 0 1 0 0 0 DITH[1:0] FM[1:0] 0 OSC_2x OSC_OUT IF_CPP RF_CPP IF_P MUX [3:0] 1 0 0 1

	HAL_SPI_DeInit(&SpiHandle);
}

// --------------- R0 -----------------

// RF_N[10:0]—RF N Counter Value
// RF_FN[11:0]—Fractional Numerator for RF PLL

// --------------- R1 -----------------

// RF_PD—RF Power-Down Control Bit
// RF_P—RF Prescaler Bit
// RF_R [5:0]—RF R Divider Value
// RF_FD[11:0]—RF PLL Fractional Denominator

// --------------- R2 -----------------

// IF_PD—IF Power Down Bit
// IF_N[18:0]—IF N Divider Value

// --------------- R3 -----------------

// ACCESS—Register Access Word
// RF_CPG—RF PLL Charge Pump Gain
// IF_R[11:0]—IF R Divider Value

// --------------- R4 -----------------

// ATPU—PLL Automatic Power Up
// DITH[1:0]—Dithering Control
// FM[1:0]—Fractional Mode
// OSC2X—Oscillator Doubler Enable
// OSC_OUT Oscillator Output Buffer Enable
// IF_CPP—IF PLL Charge Pump Polarity
// RF_CPP—RF PLL Charge Pump Polarity
// IF_P—IF Prescaler
// MUX[3:0] Frequency Out and Lock Detect MUX


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
    // leave CS_PLL
  }
}
