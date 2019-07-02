/**
 ******************************************************************************
 * @file    smarttransport.h
 * @author  Petr Dmitriev
 * @date    28.01.2016
 *
 ******************************************************************************
 */

#ifndef FIRMWARE_APP_HEADSET_SMARTTRANSPORT_H_
#define FIRMWARE_APP_HEADSET_SMARTTRANSPORT_H_

#include "qmobject.h"
#include "qmiopin.h"

class QmUart;

namespace Headset {

/*!
 * \brief Класс, реализующий транспортный уровень обмена данными между HOST и гарнитурой СКЗИ.
 *
 * Осуществляет прием/передачу пакетов данных по UART, проверку и подсчет контрольной суммы пакетов,
 * обработку специальных символов.
 */
class SmartTransport : public QmObject
{
public:

	QmIopin *pin_debug;
	/*!
	 * \param uart_resource идентификатор аппаратного ресурса UART
	 * \param max_tx_queue_size максимальный размер очереди пакетов на передачу
	 * \param parent родительский объект QmObject
	 */
	SmartTransport(int uart_resource, int max_tx_queue_size, QmObject *parent);
	~SmartTransport();
	/*!
	 * \brief Включает приемопередачу
	 */
	void enable();
	/*!
	 * \brief Выключает приемопередачу
	 */
	void disable();
	/*!
	 * \brief Передает пакет данных
	 * \param cmd код команды
	 * \param data данные
	 * \param data_len размер данных (не может превышать \a MAX_FRAME_DATA_SIZE)
	 */
	void transmitCmd(uint8_t cmd, uint8_t *data, int data_len);
	/*!
	 * \brief Осуществляет повторную передачу последнего переданного пакета данных
	 */
	void repeatLastCmd();
	/*!
	 * \brief Сигнал события приема пакета данных
	 * \param cmd код команды
	 * \param data данные
	 * \param data_len размер данных (не может превышать \a MAX_FRAME_DATA_SIZE)
	 */
	sigc::signal<void, uint8_t/*cmd*/, uint8_t*/*data*/, int/*data_len*/> receivedCmd;

	static const int MAX_FRAME_DATA_SIZE; /*!< Максимальный размер пакета данных */

private:
	/*!
	 * \brief Обработчик приема данных по UART
	 */
	void processUartReceivedData();
	/*!
	 * \brief Обработчик ошибок приема данных по UART
	 */
	void processUartReceivedErrors(bool data_errors, bool overflow);
	/*!
	 * \brief Сбрасывает состояние према
	 */
	void dropRxSync();
	/*!
	 * \brief Подсчитывает котрольную сумму CRC16 пакета данных
	 * \param cmd код команды
	 * \param data данные
	 * \param data_len размер данных (не может превышать \a MAX_FRAME_DATA_SIZE)
	 * \return CRC16 пакета дынных
	 */
	uint16_t calcFrameCRC(uint8_t cmd, uint8_t *data, int data_len);
	/*!
	 * \brief Извлекает котрольную сумму CRC16 пакета данных (чтение имеющейся)
	 * \param data данные
	 * \param data_len размер данных (не может превышать \a MAX_FRAME_DATA_SIZE)
	 * \return CRC16 пакета дынных
	 */
	uint16_t extractFrameCRC(uint8_t *data, int data_len);
	/*!
	 * \brief Кодирует данные
	 * \param input_data исходные данные
	 * \param output_data кодированные данные
	 * \param data_len размер исходных данных
	 * \return размер кодированных данных
	 */
	int encodeFrameData(uint8_t* input_data, uint8_t* output_data, int data_len);
	/*!
	 * \brief Декодирует данные
	 * \param input_data исходные данные
	 * \param output_data декодированные данные
	 * \param data_len размер исходных данных
	 * \return размер декодированных данных
	 */
	int decodeFrameData(uint8_t* input_data, uint8_t* output_data, int data_len);

	/*!< Состояния приема данных */
	enum {
		rxstateNone,
		rxstateFrame
	} rx_state;
	QmUart *uart; /*!< UART приемопередачи */
	uint8_t *rx_frame_buf; /*!< Приемный буфер */
	int rx_frame_size; /*!< Размер принятых данных */

	uint8_t last_cmd; /*!< Последняя переданная команда */
	uint8_t* last_cmd_data; /*!< Последние переданные данные */
	int last_cmd_data_size; /*!< Размер последних переданных данных */
};

} /* namespace Headset */

#endif /* FIRMWARE_APP_HEADSET_SMARTTRANSPORT_H_ */
