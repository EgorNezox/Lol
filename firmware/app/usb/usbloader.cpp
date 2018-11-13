/**
 ******************************************************************************
 * @file    usbloader.cpp
 * @author  Pankov
 * @date    01.08.2018
 *
 ******************************************************************************
 */

#include "usbloader.h"
#include "../../../system/usb_cdc.h"

#include "qm.h"
#define QMDEBUGDOMAIN	usbloader
#include "qmdebug.h"
#include <string.h>


namespace Multiradio {

#define hw_usb						2

usb_loader::usb_loader()
{
#ifndef PORT__PCSIMULATOR
	 usb = new QmUsb(hw_usb);
	 usb->usbwakeup.connect(sigc::mem_fun(this, &usb_loader::recievedData));
#endif
}

void usb_loader::startUsb()
{
#ifndef PORT__PCSIMULATOR
	 usb_start();
#endif
}

void usb_loader::transmit(uint8_t * sym, int len)
{
#ifndef PORT__PCSIMULATOR
    usb_tx(sym,len);
#endif
}

usb_loader::~usb_loader()
{
	//delete usb_loader;
}

uint16_t usb_loader::searchCadr(uint8_t *cadr, uint16_t len )
{

     for(int i = 0; i < len; i++)
     {
    	 if (counter == 0)
    	 {
    		 if (cadr[i] == 0x10) counter = 1;
    		 start = i;
    	 }
    	 else
    	 {
    		 if (counter == 1)
    		 {
    			 size  = cadr[i];
    		 }

    		 if (counter == 2)
    		 {
    			 size += cadr[i] << 8;
    		 }

    		 if (counter == size + 3)
    		 {
    			 isCadr  = false;
    			 isOk    = false;
    			 counter = 0;
    			 size    = 0;
    			 start   = 0;

    			 if (cadr[i] == 0x11)
    			 {
    				 manageCadr(out_buf,size);
    				 //return size;

    				 qmDebugMessage(QmDebug::Info, "BLA BLA: %i ", i);
    				 continue;
    			 }

    		 }

    		 if (counter - 1 >= len) qmDebugMessage(QmDebug::Info, "ERROR: CNT %i LEN %i", counter, len);
    		 out_buf[counter - 1] = cadr[i];
    		 ++counter;

    	 }

	 }

     return 0;
}

// 0x10 size1 size2 id pos name[32] DATA 0x11

void usb_loader::manageCadr(uint8_t *cadr, uint16_t len)
{
	uint8_t userid = cadr[2];

	uint16_t size = 0;

	size = cadr[0] + (cadr[1] << 8);

	int pos = 0;

	if (userid == 0x8)
	{
		//keyEmulate(cadr[3]);
	}

}

bool usb_loader::getUsbStatus()
{
 #ifndef PORT__PCSIMULATOR
	return usb->getStatus();
#else
    return false;
#endif

}

void usb_loader::setfs(DataStorage::FS *fs)
{
	this->fs = fs;
}

void usb_loader::recievedData()
{
     #ifndef PORT__PCSIMULATOR
	 uint8_t * buff = usb->getbuffer();

	 /* получаем текущую длинну сообщения */
	 int len =  usb->getLen();

	 if (len > 0)
	 searchCadr(buff,usb->getLen());

	 usb->resetLen();
     #endif
}

} /* namespace Multiradio */


#include "qmdebug_domains_start.h"
QMDEBUG_DEFINE_DOMAIN(usbloader, LevelVerbose)
#include "qmdebug_domains_end.h"