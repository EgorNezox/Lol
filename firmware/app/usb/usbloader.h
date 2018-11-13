/**
 ******************************************************************************
 * @file    usbloader.h
 * @author  Pankov
 * @date    01.08.2018
 *
 ******************************************************************************
 */

#ifndef FIRMWARE_APP_USB_USBLOADER_H_
#define FIRMWARE_APP_USB_USBLOADER_H_

#include "qmobject.h"
#include "qmusb.h"
#include "../datastorage/fs.h"

namespace Multiradio {

class usb_loader : public QmObject
{
public:
	 usb_loader();
	~usb_loader();
	void setfs(DataStorage::FS *fs);

	void startUsb       ();
	void recievedData   ();
	bool getUsbStatus   ();

	uint16_t searchCadr (uint8_t *cadr, uint16_t len );
	void     manageCadr (uint8_t *cadr, uint16_t len );

	void transmit(uint8_t * sym, int len);

	sigc::signal<void> test;
	bool debugMode = false;

private:
	QmUsb *usb;
	DataStorage::FS *fs;
	uint8_t out_buf[2048];

	volatile int isCadr   = false;
	volatile int isOk     = 0;
	volatile int counter  = 0;
	volatile int size     = 0;
	volatile int start    = 0;
};

} /* namespace Multiradio */

#endif /* FIRMWARE_APP_USB_USBLOADER_H_ */
