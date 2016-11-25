/**
  ******************************************************************************
  * @file    hal_rtc.h
  * @author  Petr Dmitriev
  * @date    24.11.2016
  *
  ******************************************************************************
 */

#ifndef HAL_RTC_H_
#define HAL_RTC_H_

#ifdef __cplusplus
extern "C" {
#endif

void hal_rtc_init(void);
void hal_rtc_deinit(void);

void hal_rtc_set_time(void);
void hal_rtc_get_time(void);
void hal_rtc_set_date(void);
void hal_rtc_get_date(void);

#ifdef __cplusplus
}
#endif

#endif /* HAL_RTC_H_ */
