/**
  ******************************************************************************
  * @file    usb_cdc.cpp
  * @author  Pankov Denis
  * @date    23.03.2018
  *
  ******************************************************************************
 */

#include "dspcontroller.h"


#define NSPROTO_MSG_PREPARE_FLASH			5
#define NSPROTO_MSG_GENERIC_RESP_OK			1
#define NSPROTO_MSG_GENERIC_RESP_ERROR		2

#define USB_RX_BUF_SIZE                  2048

#define USBFLASHER_START_DELIMITER	     0x10
#define USBFLASHER_END_DELIMITER	     0x11

uint8_t usb_tx_buffer[USB_RX_BUF_SIZE];
uint32_t usb_tx_buffer_size = 0;


namespace Multiradio
{

/* прием данных с пк */
void DspController::parsing_cadr_form_pc(uint8_t* buffer)
{
	uint16_t size = 0;

	/*получаем данные о размере кадра */
	size = buffer[2] + (buffer[1] << 8);

	/* проверяем userid кадра */
	if (buffer[3] == NSPROTO_MSG_PREPARE_FLASH)
	{
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


