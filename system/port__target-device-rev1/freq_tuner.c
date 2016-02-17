/**
  ******************************************************************************
  * @file    freq_tuner.c
  * @author  Artem Pisarenko, PMR dept. software team, ONIIP, PJSC
  * @date    12.01.2016
  *
  ******************************************************************************
 */

#include "stm32f2xx_hal.h"

#define DACx                            DAC
#define DACx_CLK_ENABLE()               __HAL_RCC_DAC_CLK_ENABLE()
#define DACx_CHANNEL_GPIO_CLK_ENABLE()  __HAL_RCC_GPIOA_CLK_ENABLE()
#define DACx_FORCE_RESET()              __HAL_RCC_DAC_FORCE_RESET()
#define DACx_RELEASE_RESET()            __HAL_RCC_DAC_RELEASE_RESET()
#define DACx_CHANNEL_PIN                GPIO_PIN_4
#define DACx_CHANNEL_GPIO_PORT          GPIOA
#define DACx_CHANNEL                    DAC_CHANNEL_1

#define MIN_DAC_OUTPUT_VALUE	620		// 0,5В при Vref=3,3В
#define MAX_DAC_OUTPUT_VALUE	3100	// 2,5В при Vref=3,3В

static uint32_t load_dac_output_value(void);

static const int8_t const *freqtuneroffset_value_addr = (int8_t *)0x080E0004;

void tune_frequency_generator(void) {
	DAC_HandleTypeDef    DacHandle;
	DAC_ChannelConfTypeDef sConfig;
	DacHandle.Instance = DACx;
	HAL_DAC_Init(&DacHandle);
	sConfig.DAC_Trigger = DAC_TRIGGER_NONE;
	sConfig.DAC_OutputBuffer = DAC_OUTPUTBUFFER_ENABLE;
	HAL_DAC_ConfigChannel(&DacHandle, &sConfig, DACx_CHANNEL);
	HAL_DAC_SetValue(&DacHandle, DACx_CHANNEL, DAC_ALIGN_12B_R, load_dac_output_value());
	HAL_DAC_Start(&DacHandle, DACx_CHANNEL);
}

static uint32_t load_dac_output_value(void) {
	int8_t offset_value = 0;
	if ((uint8_t)(*freqtuneroffset_value_addr) != 0xFF)
		offset_value = *freqtuneroffset_value_addr;
	return (MIN_DAC_OUTPUT_VALUE + ((100.0 + offset_value)/200.0)*(MAX_DAC_OUTPUT_VALUE - MIN_DAC_OUTPUT_VALUE));
}

void HAL_DAC_MspInit(DAC_HandleTypeDef* hdac)
{
  GPIO_InitTypeDef          GPIO_InitStruct;

  /*##-1- Enable peripherals and GPIO Clocks #################################*/
  /* Enable GPIO clock ****************************************/
  DACx_CHANNEL_GPIO_CLK_ENABLE();
  /* DAC Periph clock enable */
  DACx_CLK_ENABLE();

  /*##-2- Configure peripheral GPIO ##########################################*/
  /* DAC Channel1 GPIO pin configuration */
  GPIO_InitStruct.Pin = DACx_CHANNEL_PIN;
  GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(DACx_CHANNEL_GPIO_PORT, &GPIO_InitStruct);
}
