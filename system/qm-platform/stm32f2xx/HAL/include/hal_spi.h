/**
  ******************************************************************************
  * @file    hal_spi.h
  * @author  Artem Pisarenko, PMR dept. software team, ONIIP, PJSC
  * @date    10.02.2016
  * @brief   Интерфейс аппаратной абстракции доступа к SPI
  *
  ******************************************************************************
  */

#ifndef HAL_SPI_H_
#define HAL_SPI_H_

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/*! Тип фазы тактового сигнала (CPHA) шины SPI */
typedef enum {
	hspiCPHA0 = 0,
	hspiCPHA1 = 1
} hal_spi_cpha_t;

/*! Тип полярности тактового сигнала (CPOL) шины SPI */
typedef enum {
	hspiCPOL0 = 0,
	hspiCPOL1 = 1
} hal_spi_cpol_t;

/*! Тип размерности данных шины SPI */
typedef enum {
	hspiDataSize8bit = 0,
	hspiDataSize16bit = 1
} hal_spi_data_size_t;

/*! Тип порядка бит шины SPI */
typedef enum {
	hspiFirstMSB = 0,
	hspiFirstLSB = 1
} hal_spi_first_bit_t;

/*! Описание master-передачи SPI */
struct hal_spi_master_transfer_t {
	uint32_t max_baud_rate;			/*!< конфигурация: макс. битовая скорость (0 - минимально возможная) */
	hal_spi_cpha_t cpha; 			/*!< конфигурация: CPHA */
	hal_spi_cpol_t cpol; 			/*!< конфигурация: CPOL */
	hal_spi_data_size_t data_size;	/*!< конфигурация: размер данных */
	hal_spi_first_bit_t first_bit;	/*!< конфигурация: порядок бит в данных */
	uint8_t *rx_buffer;				/*!< (опциональный) указатель на буфер данных для приема */
	uint8_t *tx_buffer;				/*!< (опциональный) указатель на буфер данных для передачи */
	int buffer_size;				/*!< размер буфера данных в байтах */
};

/*! Инициализирует описание master-передачи SPI значениями по умолчанию */
void hal_spi_init_master_transfer_struct(struct hal_spi_master_transfer_t *t);

/*! Выполняет full-duplex master-передачу данных на шине SPI в блокирующем режиме
 *
 * Буфер данных интерпретируется как массив 8-битных или 16-битных пакетов в соответствии
 * со значением t->data_size.
 * Приемный и передающий буферы могут совпадать или даже вообще отсутствовать.
 * Размер буфера должен быть кратен размеру данных.
 * Если результат передачи успешный, то:
 * - на всех позициях приемного буфера данных t->rx_buffer содержатся валидные данные;
 * - данные с всех позиций передающего буфера данных t->tx_buffer успешно переданы.
 * \param[in]		bus_instance	номер экземпляра аппаратного SPI в диапазоне [1..3]
 * \param[in,out]	t				указатель на описание передачи
 * \return true - передача успешно выполнена, false - передача не выполнена
 */
bool hal_spi_master_fd_transfer(int bus_instance, struct hal_spi_master_transfer_t *t);

#ifdef __cplusplus
}
#endif

#endif /* HAL_SPI_H_ */
