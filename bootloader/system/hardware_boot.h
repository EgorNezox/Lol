/**
  ******************************************************************************
  * @file    hardware_boot.h
  * @author  Artem Pisarenko, PMR dept. software team, ONIIP, PJSC
  * @date    19.05.2016
  *
  ******************************************************************************
 */

#ifndef HARDWARE_BOOT_H_
#define HARDWARE_BOOT_H_

#define NEW_BOOTLOADER 1

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
	hwboottestOk,
	hwboottestErrorExtSram,
	hwboottestErrorHseClock
} hwboot_test_result_t;

hwboot_test_result_t hwboot_test_board(void);
bool hwboot_check_firmware(void);

#if NEW_BOOTLOADER
	bool hwboot_check_usbcdc(void);
#else
	void hwboot_jump_usbflasher(void);
#endif
void hwboot_jump_cdc(void);
void hwboot_jump_firmware(void);
void hwboot_jump_system_bootloader(void);

#ifdef __cplusplus
}
#endif

#endif /* HARDWARE_BOOT_H_ */
