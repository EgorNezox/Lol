/**
  ******************************************************************************
  * @file    rtc.c
  * @author  Petr Dmitriev
  * @date    24.11.2016
  *
  ******************************************************************************
 */

#include <stdbool.h>
#include "stm32f2xx.h"
#include "FreeRTOS.h"

#include "sys_internal.h"
#include "hal_timer.h"
#include "hal_rtc.h"
#include "rtc_private.h"


static hal_timer_handle_t rtc_timer;


static bool hal_rcc_osc_config(void);
static bool hal_rccex_periph_clk_config(void);


void halinternal_rtc_init(void) {
	hal_timer_params_t timer_params;
	timer_params.userid = (void *)&rtc_timer;
	timer_params.callbackTimeout = NULL;
	rtc_timer = hal_timer_create(&timer_params);
}

void hal_rtc_init(void) {
	bool success = false;
	success = hal_rcc_osc_config();
	success = hal_rccex_periph_clk_config();

	/* Enable RTC Clock */
	__HAL_RCC_RTC_ENABLE();
}

void hal_rtc_deinit(void) {
	//TODO
}

static bool hal_rcc_osc_config(void) {
	/*------------------------------ LSI Configuration -------------------------*/
	/* Disable the Internal Low Speed oscillator (LSI). */
	__HAL_RCC_LSI_DISABLE();

	/* Wait till LSI is ready */
	hal_timer_start(rtc_timer, LSI_TIMEOUT_VALUE, 0);
	while(__HAL_RCC_GET_FLAG(RCC_FLAG_LSIRDY) != RESET)	{
		if (hal_timer_check_timeout(rtc_timer)) {
			return false;
		}
	}
	hal_timer_stop(rtc_timer);

	/*------------------------------ LSE Configuration -------------------------*/
	/* Enable Power Clock*/
	__HAL_RCC_PWR_CLK_ENABLE();

	/* Enable write access to Backup domain */
	PWR->CR |= PWR_CR_DBP;

	/* Wait for Backup domain Write protection disable */
	hal_timer_start(rtc_timer, RCC_DBP_TIMEOUT_VALUE, 0);
	while((PWR->CR & PWR_CR_DBP) == RESET) {
		if (hal_timer_check_timeout(rtc_timer)) {
			return false;
		}
	}
	hal_timer_stop(rtc_timer);

	/* Reset LSEON and LSEBYP bits before configuring the LSE ----------------*/
	__HAL_RCC_LSE_CONFIG(RCC_LSE_OFF);

	/* Wait till LSE is ready */
	hal_timer_start(rtc_timer, RCC_LSE_TIMEOUT_VALUE, 0);
	while(__HAL_RCC_GET_FLAG(RCC_FLAG_LSERDY) != RESET) {
		if (hal_timer_check_timeout(rtc_timer)) {
			return false;
		}
	}
	hal_timer_stop(rtc_timer);

	/* Set the new LSE configuration -----------------------------------------*/
	__HAL_RCC_LSE_CONFIG(RCC_LSE_ON);

	/* Wait till LSE is ready */
	hal_timer_start(rtc_timer, RCC_LSE_TIMEOUT_VALUE, 0);
	while(__HAL_RCC_GET_FLAG(RCC_FLAG_LSERDY) == RESET) {
		if (hal_timer_check_timeout(rtc_timer)) {
			return false;
		}
	}
	hal_timer_stop(rtc_timer);

	return true;
}

static bool hal_rccex_periph_clk_config(void) {
	/* Enable Power Clock*/
	__HAL_RCC_PWR_CLK_ENABLE();

	/* Enable write access to Backup domain */
	PWR->CR |= PWR_CR_DBP;

	/* Wait for Backup domain Write protection disable */
	hal_timer_start(rtc_timer, RCC_DBP_TIMEOUT_VALUE, 0);
	while((PWR->CR & PWR_CR_DBP) == RESET) {
		if (hal_timer_check_timeout(rtc_timer)) {
			return false;
		}
	}
	hal_timer_stop(rtc_timer);

	/* Reset the Backup domain only if the RTC Clock source selection is modified */
	if((RCC->BDCR & RCC_BDCR_RTCSEL) != (RCC_RTCCLKSOURCE_LSE & RCC_BDCR_RTCSEL))
	{
		/* Store the content of BDCR register before the reset of Backup Domain */
		uint32_t tmpreg1 = (RCC->BDCR & ~(RCC_BDCR_RTCSEL));
		/* RTC Clock selection can be changed only if the Backup Domain is reset */
		__HAL_RCC_BACKUPRESET_FORCE();
		__HAL_RCC_BACKUPRESET_RELEASE();
		/* Restore the Content of BDCR register */
		RCC->BDCR = tmpreg1;
		/* Wait for LSERDY if LSE was enabled */
		if(HAL_IS_BIT_SET(tmpreg1, RCC_BDCR_LSERDY))
		{
			/* Wait till LSE is ready */
			hal_timer_start(rtc_timer, RCC_LSE_TIMEOUT_VALUE, 0);
			while(__HAL_RCC_GET_FLAG(RCC_FLAG_LSERDY) == RESET) {
				if (hal_timer_check_timeout(rtc_timer)) {
					return false;
				}
			}
			hal_timer_stop(rtc_timer);
		}
		__HAL_RCC_RTC_CONFIG(RCC_RTCCLKSOURCE_LSE);
	}
	return true;
}
