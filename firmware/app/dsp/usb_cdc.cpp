/**
  ******************************************************************************
  * @file    usb_cdc.cpp
  * @author  Pankov Denis
  * @date    23.03.2018
  *
  ******************************************************************************
 */

#include "dspcontroller.h"
#include "qmcrc.h"

#define NSPROTO_MSG_PREPARE_FLASH			5
#define NSPROTO_MSG_GENERIC_RESP_OK			1
#define NSPROTO_MSG_GENERIC_RESP_ERROR		2
#define NSPROTO_MSG_REQ_ERASE_SECTOR       10
#define NSPROTO_MSG_PROGRAM_DATA		   11
#define NSPROTO_MSG_REQ_CRC				   12
#define NSPROTO_MSG_RESP_CRC			   13

#define NSPROTO_MSG_PROGRAM_OK				3 // data отсутствует
#define NSPROTO_MSG_PROGRAM_ERROR			4 // data отсутствует

#define USB_RX_BUF_SIZE                  2048

#define USBFLASHER_START_DELIMITER	     0x10
#define USBFLASHER_END_DELIMITER	     0x11

#define M25P16_SECTOR_SIZE					64*1024

uint8_t usb_tx_buffer[USB_RX_BUF_SIZE];
uint32_t usb_tx_buffer_size = 0;


namespace Multiradio
{

/* создать crc класс для сверки */
	typedef QmCrc<uint32_t,32,0x04c11db7,0xffffffff,true,0xffffffff> crcUsb;

/* прием данных с пк */
void DspController::parsing_cadr_form_pc(uint8_t* buffer)
{
	uint16_t size = 0;

	/* user id */
	uint8_t userid = buffer[3];

	/*получаем данные о размере кадра */
	size = buffer[1] + (buffer[2] << 8);


	/* проверяем userid кадра */
	if (userid == NSPROTO_MSG_PREPARE_FLASH)
	{
		recSym = 0;
		max_erase_sector = 0;

		/* провеяем flash id, т.е. какую память прошиваем */
		if (buffer[4] == 3)
		{
			/* прошиваем флэш-память для данных HOST */
			transmit_answer_to_pc(NSPROTO_MSG_GENERIC_RESP_OK,NULL,0);
		}
		if (buffer[4] > 3 && buffer[4] < 1)
		{
			/*отправить ответ о ошибке */
			transmit_answer_to_pc(NSPROTO_MSG_GENERIC_RESP_ERROR,NULL,0);

		}
	}

    /*если пришла команда стирания секторов*/
	if (userid == NSPROTO_MSG_REQ_ERASE_SECTOR)
	{

		/* если сектор удалось стереть возвращем ок */
		if (mydevice->erase(max_erase_sector,M25P16_SECTOR_SIZE))
		{
			transmit_answer_to_pc(NSPROTO_MSG_GENERIC_RESP_OK,NULL,0);
			++max_erase_sector;
			newPacketUsb = true;
		}
		/* если сектор не удалось стереть, ошибка */
		else
		{
			transmit_answer_to_pc(NSPROTO_MSG_GENERIC_RESP_OK,NULL,0);
		}
	}

	if (userid == NSPROTO_MSG_PROGRAM_DATA)
	{

		if (mydevice->write(recSym,&buffer[4],size-1))
		{
			transmit_answer_to_pc(NSPROTO_MSG_PROGRAM_OK,NULL,0);
			recSym += size-1;
		}
		else
		{
			transmit_answer_to_pc(NSPROTO_MSG_PROGRAM_ERROR,NULL,0);
		}
	}

	if (userid == NSPROTO_MSG_REQ_CRC)
	{
		uint8_t value = 0;
//
		crcUsb crc32;
		// отправить нашу crc сумму на проверку
		for(int i = 0; i < recSym; i++)
		{
			mydevice->read(i, &value,1);
			crc32.update(&value,1);
		}

//		crcUsb crc32;
//
//		uint8_t* data = new uint8_t[recSym];
//		mydevice->read(0, data, recSym);
//		crc32.update(data, recSym);
//
//		delete [] data;

		uint32_t crc = crc32.result();
		transmit_answer_to_pc(NSPROTO_MSG_RESP_CRC,(uint8_t *)&crc, sizeof(crc));
	}

}

/* отправка ответов на пк */
void DspController::transmit_answer_to_pc(uint8_t id, uint8_t* data, uint16_t size)
{
	usb_tx_buffer_size = 0;
	usb_tx_buffer[usb_tx_buffer_size++] = USBFLASHER_START_DELIMITER;

	uint16_t frame_size = size + 1;
	usb_tx_buffer[usb_tx_buffer_size++] = frame_size        & 0xFF;
	usb_tx_buffer[usb_tx_buffer_size++] = (frame_size >> 8) & 0xFF;

	usb_tx_buffer[usb_tx_buffer_size++] = id;

	for (int i = 0; i < size; ++i)
	{
		usb_tx_buffer[usb_tx_buffer_size++] = data[i];
	}

	usb_tx_buffer[usb_tx_buffer_size++] = USBFLASHER_END_DELIMITER;

	usb_tx(usb_tx_buffer,usb_tx_buffer_size);
}

}



