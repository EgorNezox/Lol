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
    			 start   = 0;

    			 if (cadr[i] == 0x11)
    			 {
    				 manageCadr(out_buf,size);
    				 //return size;

    				 qmDebugMessage(QmDebug::Info, "BLA BLA: %i ", i);
    				 continue;
    			 }

    			 size = 0;
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

	if ((userid  > 0 && userid < 6) && size < 1024)
	{
		// read file from device
		if (size == 1)
		{
			// get size from file
			uint32_t fileSize = fs->getSizeDataFileFromId(userid);
			if (fileSize > 0)
			{
				fileSize  += sizeof(userid);
				uint8_t file_data[fileSize + 5] = {0};
				bool res = fs->getFastFileFromId(userid, fileSize, &file_data[4]);

				if (res)
				{
					file_data[0]        = 0x10;
					file_data[1]        = fileSize % 256;
					file_data[2]        = fileSize / 256;
					file_data[3] 		= userid;
					file_data[fileSize + 4] = 0x11;

				}
				transmit(file_data, fileSize+5);
			}
			else
			{
				/* send list of names */
				if (userid == 6)
				{
					/* get list names */
					std::vector<std::string> files;
					fs->findFilesToFiletree(files);

					uint16_t cnt_pack  =  0;
					uint16_t len       =  0;

					/* get names size */
					for(uint32_t i = 0; i < files.size(); i++)
						cnt_pack += files.at(i).size() + 1;


					while (cnt_pack > 0)
					{

						/* pack one or more packet */
						len  = (cnt_pack > 1024 - 5) ? 1024 - 5: cnt_pack;
						uint8_t file_data[len + 5];

						/* EMTRY PACKET */
						file_data[0]        = 0x10;
						file_data[1]        = len % 256;
						file_data[2]        = len / 256;
						file_data[3] 		= userid;
						file_data[len + 4] = 0x11;

						uint16_t index = 4;
						char *data;

						for(uint32_t i = 0; i < files.size(); i++)
						{

							data = (char*)files.at(i).c_str();
							uint16_t size = files.at(i).size();

							memcpy(&file_data[index], data, size);
							index += size;
						}

						transmit(file_data, len + 5);
						cnt_pack -= len;
					}


				}
			}
		}
		// load file to device
		else
		{
			fs->writeFileFromId(userid,&cadr[3],size - 1);
		}
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
