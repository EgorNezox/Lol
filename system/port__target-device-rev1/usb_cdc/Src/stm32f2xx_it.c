#include "stm32f2xx_hal.h"
#include "stm32f2xx.h"
#include "../Inc/stm32f2xx_it.h"

extern PCD_HandleTypeDef hpcd_USB_OTG_FS;

void OTG_FS_IRQHandler(void)
{
  HAL_PCD_IRQHandler(&hpcd_USB_OTG_FS);
}
