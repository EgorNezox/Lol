/**
  ******************************************************************************
  * @file    system.h
  * @author  Artem Pisarenko, PMR dept. software team, ONIIP, PJSC
  * @date    23.03.2016
  *
  ******************************************************************************
 */

#ifndef SYSTEM_H_
#define SYSTEM_H_

#ifdef __cplusplus
extern "C" {
#endif

void stm32f2_enable_interrupts(void);
void stm32f2_disable_interrupts(void);

void stm32f2_enter_bootloader(void);

#ifdef __cplusplus
}
#endif

#endif /* SYSTEM_H_ */
