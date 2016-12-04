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
static bool hal_rtc_enter_init_mode(void);
static bool hal_rtcex_deactivate_wakeup_timer(void);
static bool hal_rtcex_set_wakeup_timer_it(uint32_t WakeUpCounter, uint32_t WakeUpClock);

static bool hal_rtc_wait_for_synchro(void);

static uint8_t hal_rtc_byte_to_bcd2(uint8_t Value);
static uint8_t hal_rtc_bcd2_to_byte(uint8_t Value);


void halinternal_rtc_init(void) {
	hal_timer_params_t timer_params;
	timer_params.userid = (void *)RTC;
	timer_params.callbackTimeout = NULL;
	rtc_timer = hal_timer_create(&timer_params);
}

void hal_rtc_init(void) {
	if(!hal_rcc_osc_config())
		return;
	if(!hal_rccex_periph_clk_config())
		return;

	/* Enable RTC Clock */
	__HAL_RCC_RTC_ENABLE();

//	HAL_NVIC_SetPriority(RTC_WKUP_IRQn, 0x0F, 0);
//	HAL_NVIC_EnableIRQ(RTC_WKUP_IRQn);

	/* Disable the write protection for RTC registers */
	__HAL_RTC_WRITEPROTECTION_DISABLE();

	/* Set Initialization mode */
	if (!hal_rtc_enter_init_mode()) {
		/* Enable the write protection for RTC registers */
		__HAL_RTC_WRITEPROTECTION_ENABLE();
		return;
	}

	/* Clear RTC_CR FMT, OSEL and POL Bits */
	RTC->CR &= ((uint32_t)~(RTC_CR_FMT | RTC_CR_OSEL | RTC_CR_POL));
	/* Set RTC_CR register */
	RTC->CR |= (uint32_t)(RTC_HOURFORMAT_24 | RTC_OUTPUT_DISABLE | RTC_OUTPUT_POLARITY_HIGH);

	/* Configure the RTC PRER */
	RTC->PRER = (uint32_t)(RTC_SYNCH_PREDIV);
	RTC->PRER |= (uint32_t)(RTC_ASYNCH_PREDIV << 16);

	/* Exit Initialization mode */
	RTC->ISR &= (uint32_t)~RTC_ISR_INIT;

	RTC->TAFCR &= (uint32_t)~RTC_TAFCR_ALARMOUTTYPE;
	RTC->TAFCR |= (uint32_t)(RTC_OUTPUT_TYPE_OPENDRAIN);

	/* Enable the write protection for RTC registers */
	__HAL_RTC_WRITEPROTECTION_ENABLE();

	if(!hal_rtcex_deactivate_wakeup_timer())
		return;
	if(!hal_rtcex_set_wakeup_timer_it(32768/16, RTC_WAKEUPCLOCK_RTCCLK_DIV16))
		return;
}

void hal_rtc_deinit(void) {
	//TODO
}

void hal_rtc_clear_wakeup_it_pending_bit(void) {
	/* Clear the WAKEUPTIMER interrupt pending bit */
	__HAL_RTC_WAKEUPTIMER_CLEAR_FLAG(RTC_FLAG_WUTF);
}

void hal_rtc_set_time(hal_rtc_time_t time) {
	uint32_t tmpreg = 0;

    tmpreg = (uint32_t)(((uint32_t)hal_rtc_byte_to_bcd2(time.hours) << 16) | \
                        ((uint32_t)hal_rtc_byte_to_bcd2(time.minutes) << 8) | \
                        ((uint32_t)hal_rtc_byte_to_bcd2(time.seconds)) | \
                        ((/*(uint32_t)time.TimeFormat*/0) << 16));

    /* Disable the write protection for RTC registers */
    __HAL_RTC_WRITEPROTECTION_DISABLE();

    /* Set Initialization mode */
    if (!hal_rtc_enter_init_mode()) {
    	/* Enable the write protection for RTC registers */
    	__HAL_RTC_WRITEPROTECTION_ENABLE();
    	return;
    }

    /* Set the RTC_TR register */
    RTC->TR = (uint32_t)(tmpreg & RTC_TR_RESERVED_MASK);

    /* Clear the bits to be configured */
    RTC->CR &= (uint32_t)~RTC_CR_BCK;

    /* Configure the RTC_CR register */
//    RTC->CR |= (uint32_t)(sTime->DayLightSaving | sTime->StoreOperation);

    /* Exit Initialization mode */
    RTC->ISR &= (uint32_t)~RTC_ISR_INIT;

    if (!hal_rtc_wait_for_synchro()) {
    	/* Enable the write protection for RTC registers */
    	__HAL_RTC_WRITEPROTECTION_ENABLE();
    	return;
    }

    /* Enable the write protection for RTC registers */
    __HAL_RTC_WRITEPROTECTION_ENABLE();
}

hal_rtc_time_t hal_rtc_get_time() {
	/* Get the TR register */
	uint32_t tmpreg = (uint32_t)(RTC->TR & RTC_TR_RESERVED_MASK);

	hal_rtc_time_t time;
	/* Fill the structure fields with the read parameters */
	time.hours = (uint8_t)((tmpreg & (RTC_TR_HT | RTC_TR_HU)) >> 16);
	time.minutes = (uint8_t)((tmpreg & (RTC_TR_MNT | RTC_TR_MNU)) >>8);
	time.seconds = (uint8_t)(tmpreg & (RTC_TR_ST | RTC_TR_SU));
//	time->TimeFormat = (uint8_t)((tmpreg & (RTC_TR_PM)) >> 16);

	/* Convert the time structure parameters to Binary format */
	time.hours = hal_rtc_bcd2_to_byte(time.hours);
	time.minutes = hal_rtc_bcd2_to_byte(time.minutes);
	time.seconds = hal_rtc_bcd2_to_byte(time.seconds);

	return time;
}

void hal_rtc_set_date(hal_rtc_date_t date) {
	uint32_t datetmpreg = 0;

	if(/*(Format == RTC_FORMAT_BIN) &&*/ ((date.month & 0x10) == 0x10))
	{
		date.month = (uint8_t)((date.month & (uint8_t)~(0x10)) + (uint8_t)0x0A);
	}

	datetmpreg = (((uint32_t)hal_rtc_byte_to_bcd2(date.year) << 16) | \
			((uint32_t)hal_rtc_byte_to_bcd2(date.month) << 8) | \
			((uint32_t)hal_rtc_byte_to_bcd2(date.day)) | \
			((uint32_t)date.weekday << 13));

    /* Disable the write protection for RTC registers */
    __HAL_RTC_WRITEPROTECTION_DISABLE();

    /* Set Initialization mode */
    if (!hal_rtc_enter_init_mode()) {
    	/* Enable the write protection for RTC registers */
    	__HAL_RTC_WRITEPROTECTION_ENABLE();
    	return;
    }

    /* Set the RTC_DR register */
    RTC->DR = (uint32_t)(datetmpreg & RTC_DR_RESERVED_MASK);

    /* Exit Initialization mode */
    RTC->ISR &= (uint32_t)~RTC_ISR_INIT;

    if (!hal_rtc_wait_for_synchro()) {
    	/* Enable the write protection for RTC registers */
    	__HAL_RTC_WRITEPROTECTION_ENABLE();
    	return;
    }

    /* Enable the write protection for RTC registers */
    __HAL_RTC_WRITEPROTECTION_ENABLE();
}

hal_rtc_date_t hal_rtc_get_date(void) {
	/* Get the DR register */
	uint32_t datetmpreg = (uint32_t)(RTC->DR & RTC_DR_RESERVED_MASK);

	hal_rtc_date_t date;
	/* Fill the structure fields with the read parameters */
	date.year = (uint8_t)((datetmpreg & (RTC_DR_YT | RTC_DR_YU)) >> 16);
	date.month = (uint8_t)((datetmpreg & (RTC_DR_MT | RTC_DR_MU)) >> 8);
	date.day = (uint8_t)(datetmpreg & (RTC_DR_DT | RTC_DR_DU));
	date.weekday = (uint8_t)((datetmpreg & (RTC_DR_WDU)) >> 13);

	/* Convert the date structure parameters to Binary format */
	date.year = hal_rtc_bcd2_to_byte(date.year);
	date.month = hal_rtc_bcd2_to_byte(date.month);
	date.day = hal_rtc_bcd2_to_byte(date.day);

	return date;
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

static bool hal_rtc_enter_init_mode(void) {
	/* Check if the Initialization mode is set */
	if((RTC->ISR & RTC_ISR_INITF) == (uint32_t)RESET)
	{
		/* Set the Initialization mode */
		RTC->ISR = (uint32_t)RTC_INIT_MASK;

		/* Wait till RTC is in INIT state and if Time out is reached exit */
		hal_timer_start(rtc_timer, RTC_TIMEOUT_VALUE, 0);
		while((RTC->ISR & RTC_ISR_INITF) == (uint32_t)RESET) {
			if (hal_timer_check_timeout(rtc_timer)) {
				return false;
			}
		}
		hal_timer_stop(rtc_timer);
	}
	return true;
}

static bool hal_rtcex_deactivate_wakeup_timer(void) {
	/* Disable the write protection for RTC registers */
	__HAL_RTC_WRITEPROTECTION_DISABLE();

	/* Disable the Wake-up Timer */
	__HAL_RTC_WAKEUPTIMER_DISABLE();

	/* In case of interrupt mode is used, the interrupt source must disabled */
	__HAL_RTC_WAKEUPTIMER_DISABLE_IT(RTC_IT_WUT);

	/* Wait till RTC WUTWF flag is set and if Time out is reached exit */
	hal_timer_start(rtc_timer, RTC_TIMEOUT_VALUE, 0);
	while(__HAL_RTC_WAKEUPTIMER_GET_FLAG(RTC_FLAG_WUTWF) == RESET) {
		if (hal_timer_check_timeout(rtc_timer)) {
			/* Enable the write protection for RTC registers */
			__HAL_RTC_WRITEPROTECTION_ENABLE();
			return false;
		}
	}
	hal_timer_stop(rtc_timer);

	/* Enable the write protection for RTC registers */
	__HAL_RTC_WRITEPROTECTION_ENABLE();

	return true;
}

static bool hal_rtcex_set_wakeup_timer_it(uint32_t WakeUpCounter, uint32_t WakeUpClock) {
	/* Disable the write protection for RTC registers */
	__HAL_RTC_WRITEPROTECTION_DISABLE();

	/*Check RTC WUTWF flag is reset only when wake up timer enabled*/
	if((RTC->CR & RTC_CR_WUTE) != RESET)
	{
		/* Wait till RTC WUTWF flag is reset and if Time out is reached exit */
		hal_timer_start(rtc_timer, RTC_TIMEOUT_VALUE, 0);
		while(__HAL_RTC_WAKEUPTIMER_GET_FLAG(RTC_FLAG_WUTWF) == SET) {
			if (hal_timer_check_timeout(rtc_timer)) {
				/* Enable the write protection for RTC registers */
				__HAL_RTC_WRITEPROTECTION_ENABLE();
				return false;
			}
		}
		hal_timer_stop(rtc_timer);
	}

	/* Disable the Wake-up Timer */
	__HAL_RTC_WAKEUPTIMER_DISABLE();

	/* Wait till RTC WUTWF flag is set and if Time out is reached exit */
	hal_timer_start(rtc_timer, RTC_TIMEOUT_VALUE, 0);
	while(__HAL_RTC_WAKEUPTIMER_GET_FLAG(RTC_FLAG_WUTWF) == RESET) {
		if (hal_timer_check_timeout(rtc_timer)) {
			/* Enable the write protection for RTC registers */
			__HAL_RTC_WRITEPROTECTION_ENABLE();
			return false;
		}
	}
	hal_timer_stop(rtc_timer);

	/* Configure the Wake-up Timer counter */
	RTC->WUTR = (uint32_t)WakeUpCounter;

	/* Clear the Wake-up Timer clock source bits in CR register */
	RTC->CR &= (uint32_t)~RTC_CR_WUCKSEL;

	/* Configure the clock source */
	RTC->CR |= (uint32_t)WakeUpClock;

	/* RTC WakeUpTimer Interrupt Configuration: EXTI configuration */
//	__HAL_RTC_WAKEUPTIMER_EXTI_ENABLE_IT();

//	EXTI->RTSR |= RTC_EXTI_LINE_WAKEUPTIMER_EVENT;

	/* Configure the Interrupt in the RTC_CR register */
	__HAL_RTC_WAKEUPTIMER_ENABLE_IT(RTC_IT_WUT);

	/* Enable the Wake-up Timer */
	__HAL_RTC_WAKEUPTIMER_ENABLE();

	/* Enable the write protection for RTC registers */
	__HAL_RTC_WRITEPROTECTION_ENABLE();

	return true;
}

static bool hal_rtc_wait_for_synchro(void) {
	/* Clear RSF flag */
	RTC->ISR &= (uint32_t)RTC_RSF_MASK;

	/* Wait the registers to be synchronised */
	hal_timer_start(rtc_timer, RTC_TIMEOUT_VALUE, 0);
	while((RTC->ISR & RTC_ISR_RSF) == (uint32_t)RESET) {
		if (hal_timer_check_timeout(rtc_timer)) {
			return false;
		}
	}
	hal_timer_stop(rtc_timer);

	return true;
}

static uint8_t hal_rtc_byte_to_bcd2(uint8_t Value) {
	uint32_t bcdhigh = 0;

	while(Value >= 10)
	{
		bcdhigh++;
		Value -= 10;
	}

	return  ((uint8_t)(bcdhigh << 4) | Value);
}

static uint8_t hal_rtc_bcd2_to_byte(uint8_t Value) {
	uint32_t tmp = 0;
	tmp = ((uint8_t)(Value & (uint8_t)0xF0) >> (uint8_t)0x4) * 10;
	return (tmp + (Value & (uint8_t)0x0F));
}
