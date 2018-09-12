#ifndef SYSTEM_USB_CDC_H_
#define SYSTEM_USB_CDC_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>

void usb_start();
void usb_tx(uint8_t * sym, int len);


#ifdef __cplusplus
}
#endif

#endif
