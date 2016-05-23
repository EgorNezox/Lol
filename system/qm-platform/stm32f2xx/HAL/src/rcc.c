/**
  ******************************************************************************
  * @file    rcc.c
  * @author  Artem Pisarenko, PMR dept. software team, ONIIP, PJSC
  * @date    19.11.2015
  * @brief   Реализация доступа к внутренней периферии RCC на STM32F2xx
  *
  ******************************************************************************
  */

#include "stm32f2xx.h"

#include "sys_internal.h"
#include "hal_timer.h"
#include "hal_rcc.h"

static hal_timer_handle_t rcc_timer;
static __I uint8_t rcc_apb_ahb_presc_table[16] = {0, 0, 0, 0, 1, 2, 3, 4, 1, 2, 3, 4, 6, 7, 8, 9};

void halinternal_rcc_init(void) {
	rcc_timer = hal_timer_create(0);
}

void hal_rcc_get_clocks(hal_rcc_clocks_t* clocks) {
	uint32_t tmp = 0, pllvco = 0, pllp = 2, pllsource = 0, pllm = 2;

	/* Get SYSCLK source -------------------------------------------------------*/
	tmp = RCC->CFGR & RCC_CFGR_SWS;

	switch (RCC->CFGR & RCC_CFGR_SWS) {
	case RCC_CFGR_SWS_HSI:  /* HSI used as system clock source */
		clocks->sysclk_frequency = HSI_VALUE;
		break;
	case RCC_CFGR_SWS_HSE:  /* HSE used as system clock  source */
		clocks->sysclk_frequency = HSE_VALUE;
		break;
	case RCC_CFGR_SWS_PLL:  /* PLL used as system clock  source */
		/* PLL_VCO = (HSE_VALUE or HSI_VALUE / PLLM) * PLLN
         SYSCLK = PLL_VCO / PLLP
		 */
		pllsource = (RCC->PLLCFGR & RCC_PLLCFGR_PLLSRC) >> POSITION_VAL(RCC_PLLCFGR_PLLSRC);
		pllm = RCC->PLLCFGR & RCC_PLLCFGR_PLLM;

		if (pllsource != 0) {
			/* HSE used as PLL clock source */
			pllvco = (HSE_VALUE / pllm) * ((RCC->PLLCFGR & RCC_PLLCFGR_PLLN) >> POSITION_VAL(RCC_PLLCFGR_PLLN));
		} else {
			/* HSI used as PLL clock source */
			pllvco = (HSI_VALUE / pllm) * ((RCC->PLLCFGR & RCC_PLLCFGR_PLLN) >> POSITION_VAL(RCC_PLLCFGR_PLLN));
		}

		pllp = (((RCC->PLLCFGR & RCC_PLLCFGR_PLLP) >> POSITION_VAL(RCC_PLLCFGR_PLLP)) + 1 ) *2;
		clocks->sysclk_frequency = pllvco/pllp;
		break;
	default:
		clocks->sysclk_frequency = HSI_VALUE;
		break;
	}

	/* Compute HCLK, PCLK1 and PCLK2 clocks frequencies ------------------------*/

	/* Get HCLK prescaler */
	tmp = (RCC->CFGR & RCC_CFGR_HPRE) >> POSITION_VAL(RCC_CFGR_HPRE);
	/* HCLK clock frequency */
	clocks->hclk_frequency = clocks->sysclk_frequency >> rcc_apb_ahb_presc_table[tmp];

	/* Get PCLK1 prescaler */
	tmp = (RCC->CFGR & RCC_CFGR_PPRE1) >> POSITION_VAL(RCC_CFGR_PPRE1);
	/* PCLK1 clock frequency */
	clocks->pclk1_frequency = clocks->hclk_frequency >> rcc_apb_ahb_presc_table[tmp];

	/* Get PCLK2 prescaler */
	tmp = (RCC->CFGR & RCC_CFGR_PPRE2) >> POSITION_VAL(RCC_CFGR_PPRE2);
	/* PCLK2 clock frequency */
	clocks->pclk2_frequency = clocks->hclk_frequency >> rcc_apb_ahb_presc_table[tmp];
}

bool hal_rcc_enable_hse(bool bypass_oscillator, unsigned int startup_timeout_ms) {
	uint32_t status;
	if ((RCC->CR & RCC_CR_HSEON) != 0)
		hal_rcc_disable_hse();
	if (bypass_oscillator)
		RCC->CR |= RCC_CR_HSEBYP;
	RCC->CR |= RCC_CR_HSEON;
	hal_timer_start(rcc_timer, startup_timeout_ms, 0);
	do {
		status = RCC->CR & RCC_CR_HSERDY;
	} while((status == 0) && (!hal_timer_check_timeout(rcc_timer)));
	hal_timer_stop(rcc_timer);
	return (status != 0);
}

void hal_rcc_disable_hse() {
	RCC->CR &= ~(RCC_CR_HSEON | RCC_CR_HSEBYP);
	while ((RCC->CR & RCC_CR_HSERDY) != 0);
}
