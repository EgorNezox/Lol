/**
  ******************************************************************************
  * @file    rcc.c
  * @author  Artem Pisarenko, PMR dept. software team, ONIIP, PJSC
  * @date    19.11.2015
  *
  ******************************************************************************
  */

#include "stm32f2xx.h"

#include "sys_internal.h"

static __I uint8_t rcc_apb_ahb_presc_table[16] = {0, 0, 0, 0, 1, 2, 3, 4, 1, 2, 3, 4, 6, 7, 8, 9};

void halinternal_get_rcc_clocks(halinternal_rcc_clocks_t* clocks) {
	uint32_t tmp = 0, presc = 0, pllvco = 0, pllp = 2, pllsource = 0, pllm = 2;

	/* Get SYSCLK source -------------------------------------------------------*/
	tmp = RCC->CFGR & RCC_CFGR_SWS;

	switch (tmp) {
	case 0x00:  /* HSI used as system clock source */
		clocks->sysclk_frequency = HSI_VALUE;
		break;
	case 0x04:  /* HSE used as system clock  source */
		clocks->sysclk_frequency = HSE_VALUE;
		break;
	case 0x08:  /* PLL used as system clock  source */
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
	presc = rcc_apb_ahb_presc_table[tmp];
	/* HCLK clock frequency */
	clocks->hclk_frequency = clocks->sysclk_frequency >> presc;

	/* Get PCLK1 prescaler */
	tmp = (RCC->CFGR & RCC_CFGR_PPRE1) >> POSITION_VAL(RCC_CFGR_PPRE1);
	presc = rcc_apb_ahb_presc_table[tmp];
	/* PCLK1 clock frequency */
	clocks->pclk1_frequency = clocks->hclk_frequency >> presc;

	/* Get PCLK2 prescaler */
	tmp = (RCC->CFGR & RCC_CFGR_PPRE2) >> POSITION_VAL(RCC_CFGR_PPRE2);
	presc = rcc_apb_ahb_presc_table[tmp];
	/* PCLK2 clock frequency */
	clocks->pclk2_frequency = clocks->hclk_frequency >> presc;
}
