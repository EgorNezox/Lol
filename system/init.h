/**
  ******************************************************************************
  * @file    init.h
  * @author  Artem Pisarenko, PMR dept. software team, ONIIP, PJSC
  * @date    11.07.2016
  *
  ******************************************************************************
 */

#ifndef SYSTEM_INIT_H_
#define SYSTEM_INIT_H_

#ifdef __cplusplus
extern "C" {
#endif

void target_device_multiradio_init(int freq);
void timer2_init();
int get_tim1value();
int tune_frequency_generator(int freq ,bool isWrite);

#ifdef __cplusplus
}
#endif

#endif /* SYSTEM_INIT_H_ */
