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

typedef struct {
	uint8_t hours;
	uint8_t minutes;
	uint8_t seconds;
} hal_rtc_time_t;

typedef struct {
	uint8_t weekday;
	uint8_t day;
	uint8_t month;
	uint8_t year;
} hal_rtc_date_t;

void hal_rtc_init(void);
void hal_rtc_deinit(void);

void hal_rtc_clear_wakeup_it_pending_bit(void);

void hal_rtc_set_time(hal_rtc_time_t time);
hal_rtc_time_t hal_rtc_get_time();
void hal_rtc_set_date(hal_rtc_date_t date);
hal_rtc_date_t hal_rtc_get_date(void);

#ifdef __cplusplus
}
#endif

#endif /* HAL_RTC_H_ */
