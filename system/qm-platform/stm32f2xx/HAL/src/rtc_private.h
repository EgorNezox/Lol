/**
  ******************************************************************************
  * @file    rtc_private.h
  * @author  Petr Dmitriev
  * @date    25.11.2016
  *
  ******************************************************************************
 */

#ifndef HAL_SRC_RTC_PRIVATE_H_
#define HAL_SRC_RTC_PRIVATE_H_

#include "stm32f2xx.h"

#define UNUSED(x) ((void)(x))

#define HAL_IS_BIT_SET(REG, BIT)         (((REG) & (BIT)) != RESET)
#define HAL_IS_BIT_CLR(REG, BIT)         (((REG) & (BIT)) == RESET)

/* LSI */
#define LSI_TIMEOUT_VALUE          ((uint32_t)100)  /* 100 ms */

#define RCC_FLAG_LSIRDY                  ((uint8_t)0x61)

#define RCC_OFFSET                 (RCC_BASE - PERIPH_BASE)

#define RCC_CSR_OFFSET             (RCC_OFFSET + 0x74)
#define RCC_LSION_BIT_NUMBER        0x00
#define RCC_CSR_LSION_BB           (PERIPH_BB_BASE + (RCC_CSR_OFFSET * 32) + (RCC_LSION_BIT_NUMBER * 4))

#define __HAL_RCC_LSI_ENABLE() (*(__IO uint32_t *) RCC_CSR_LSION_BB = ENABLE)
#define __HAL_RCC_LSI_DISABLE() (*(__IO uint32_t *) RCC_CSR_LSION_BB = DISABLE)

/* LSE */
#define RCC_DBP_TIMEOUT_VALUE      ((uint32_t)100)
#define RCC_LSE_TIMEOUT_VALUE      ((uint32_t)5000)

#define RCC_FLAG_LSERDY                  ((uint8_t)0x41)

#define RCC_LSE_OFF                      ((uint8_t)0x00)
#define RCC_LSE_ON                       ((uint8_t)0x01)

#define RCC_BDCR_OFFSET            (RCC_OFFSET + 0x70)
#define RCC_BDCR_BYTE0_ADDRESS     (PERIPH_BASE + RCC_BDCR_OFFSET)

#define __HAL_RCC_LSE_CONFIG(__STATE__)  (*(__IO uint8_t *) RCC_BDCR_BYTE0_ADDRESS = (__STATE__))

/* RCC */
#define RCC_RTCCLKSOURCE_LSE             ((uint32_t)0x00000100)

#define RCC_BDRST_BIT_NUMBER       0x10
#define RCC_BDCR_BDRST_BB          (PERIPH_BB_BASE + (RCC_BDCR_OFFSET * 32) + (RCC_BDRST_BIT_NUMBER * 4))

#define __HAL_RCC_BACKUPRESET_FORCE() (*(__IO uint32_t *) RCC_BDCR_BDRST_BB = ENABLE)
#define __HAL_RCC_BACKUPRESET_RELEASE() (*(__IO uint32_t *) RCC_BDCR_BDRST_BB = DISABLE)

#define __HAL_RCC_PWR_CLK_ENABLE()     do { \
                                        __IO uint32_t tmpreg; \
                                        SET_BIT(RCC->APB1ENR, RCC_APB1ENR_PWREN);\
                                        /* Delay after an RCC peripheral clock enabling */ \
                                        tmpreg = READ_BIT(RCC->APB1ENR, RCC_APB1ENR_PWREN);\
                                        UNUSED(tmpreg); \
                                        } while(0)

#define RCC_FLAG_MASK  ((uint8_t)0x1F)
#define __HAL_RCC_GET_FLAG(__FLAG__) (((((((__FLAG__) >> 5) == 1)? RCC->CR :((((__FLAG__) >> 5) == 2) ? RCC->BDCR :((((__FLAG__) >> 5) == 3)? RCC->CSR :RCC->CIR))) & ((uint32_t)1 << ((__FLAG__) & RCC_FLAG_MASK)))!= 0)? 1 : 0)

#define __HAL_RCC_RTC_CLKPRESCALER(__RTCCLKSource__) (((__RTCCLKSource__) & RCC_BDCR_RTCSEL) == RCC_BDCR_RTCSEL) ?    \
                                                 MODIFY_REG(RCC->CFGR, RCC_CFGR_RTCPRE, ((__RTCCLKSource__) & 0xFFFFCFF)) : CLEAR_BIT(RCC->CFGR, RCC_CFGR_RTCPRE)

#define __HAL_RCC_RTC_CONFIG(__RTCCLKSource__) do { __HAL_RCC_RTC_CLKPRESCALER(__RTCCLKSource__);    \
                                                    RCC->BDCR |= ((__RTCCLKSource__) & 0x00000FFF);  \
                                                   } while (0)

#define RCC_BDCR_OFFSET            (RCC_OFFSET + 0x70)
#define RCC_RTCEN_BIT_NUMBER       0x0F
#define RCC_BDCR_RTCEN_BB          (PERIPH_BB_BASE + (RCC_BDCR_OFFSET * 32) + (RCC_RTCEN_BIT_NUMBER * 4))

#define __HAL_RCC_RTC_ENABLE() (*(__IO uint32_t *) RCC_BDCR_RTCEN_BB = ENABLE)
#define __HAL_RCC_RTC_DISABLE() (*(__IO uint32_t *) RCC_BDCR_RTCEN_BB = DISABLE)

/* RTC */
#define RTC_INIT_MASK           ((uint32_t)0xFFFFFFFF)

#define RTC_TIMEOUT_VALUE       1000

#define RTC_HOURFORMAT_24              ((uint32_t)0x00000000)
#define RTC_OUTPUT_DISABLE             ((uint32_t)0x00000000)
#define RTC_OUTPUT_POLARITY_HIGH       ((uint32_t)0x00000000)
#define RTC_OUTPUT_TYPE_OPENDRAIN      ((uint32_t)0x00000000)

#define RTC_IT_WUT                        ((uint32_t)0x00004000)
#define RTC_FLAG_WUTWF                    ((uint32_t)0x00000004)
#define RTC_FLAG_WUTF                     ((uint32_t)0x00000400)

#define RTC_WAKEUPCLOCK_RTCCLK_DIV16        ((uint32_t)0x00000000)
#define RTC_WAKEUPCLOCK_RTCCLK_DIV8         ((uint32_t)0x00000001)
#define RTC_WAKEUPCLOCK_RTCCLK_DIV4         ((uint32_t)0x00000002)
#define RTC_WAKEUPCLOCK_RTCCLK_DIV2         ((uint32_t)0x00000003)

#define RTC_EXTI_LINE_WAKEUPTIMER_EVENT       ((uint32_t)EXTI_IMR_MR22)  /*!< External interrupt line 22 Connected to the RTC Wake-up event */

#define __HAL_RTC_WRITEPROTECTION_DISABLE()             \
                        do{                                       \
                            RTC->WPR = 0xCA;   \
                            RTC->WPR = 0x53;   \
                          } while(0)

#define __HAL_RTC_WRITEPROTECTION_ENABLE()              \
                        do{                                       \
                            RTC->WPR = 0xFF;   \
                          } while(0)

#define __HAL_RTC_WAKEUPTIMER_ENABLE()                      (RTC->CR |= (RTC_CR_WUTE))
#define __HAL_RTC_WAKEUPTIMER_DISABLE()                     (RTC->CR &= ~(RTC_CR_WUTE))

#define __HAL_RTC_WAKEUPTIMER_ENABLE_IT(__INTERRUPT__)    (RTC->CR |= (__INTERRUPT__))
#define __HAL_RTC_WAKEUPTIMER_DISABLE_IT(__INTERRUPT__)   (RTC->CR &= ~(__INTERRUPT__))

#define __HAL_RTC_WAKEUPTIMER_GET_FLAG(__FLAG__)          ((((RTC->ISR) & (__FLAG__)) != RESET)? SET : RESET)
#define __HAL_RTC_WAKEUPTIMER_CLEAR_FLAG(__FLAG__)            (RTC->ISR) = (~((__FLAG__) | RTC_ISR_INIT)|(RTC->ISR & RTC_ISR_INIT))

//#define __HAL_RTC_WAKEUPTIMER_EXTI_ENABLE_IT()       (EXTI->IMR |= RTC_EXTI_LINE_WAKEUPTIMER_EVENT)
//#define __HAL_RTC_WAKEUPTIMER_EXTI_DISABLE_IT()      (EXTI->IMR &= ~(RTC_EXTI_LINE_WAKEUPTIMER_EVENT))


/* Manual config */
#define RTC_ASYNCH_PREDIV  0x7F   /* LSE as RTC clock */
#define RTC_SYNCH_PREDIV   0x00FF /* LSE as RTC clock */

#endif /* HAL_SRC_RTC_PRIVATE_H_ */
