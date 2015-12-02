/**
  ******************************************************************************
  * @file    hal_i2c.h
  * @author  Artem Pisarenko, PMR dept. software team, ONIIP, PJSC
  * @date    24.11.2015
  * @brief   Интерфейс аппаратной абстракции доступа к I2C
  *
  ******************************************************************************
  */

#ifndef HAL_I2C_H_
#define HAL_I2C_H_

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef void* hal_i2c_smbus_handle_t;

/*! Тип возвращаемого результата завершения передачи */
typedef enum {
	hi2cSuccess = 0,		/*!< успешная передача */
	hi2cErrorAborted,		/*!< отменено/прервано (hal_i2c_abort_master_transfer(...) или hal_i2c_set_bus_mode(hi2cModeOff)) */
	hi2cErrorPEC,			/*!< несовпадение контрольной суммы PEC при приеме */
	hi2cErrorBus,			/*!< ошибка во время коммуникации на шине (misplaced START/STOP, arbitration loss, SMBus timeout) */
	hi2cErrorAddressNACK,	/*!< устройство не ответило на адресацию */
	hi2cErrorDataNACK		/*!< устройство не подтвердило байт данных (при передаче) */
} hal_i2c_transfer_result_t;

/*! Тип режима работы шины I2C */
typedef enum {
	hi2cModeOff,		/*!< выключена */
	hi2cModeStandard,	/*!< оригинальный I2C */
	hi2cModeSMBus		/*!< SMBus 2.0 */
} hal_i2c_mode_t;

/*! Тип описания устройства I2C */
typedef struct {
	int bus_instance;	/*!< номер экземпляра аппаратного I2C в диапазоне [1..3] */
	uint8_t address;	/*!< 7-битный адрес устройства на шине */
} hal_i2c_device_t;

/*! Тип направления передачи байта */
typedef enum {
	hi2cDirectionRx = 0,	/*!< прием (байт записывается в буфер) */
	hi2cDirectionTx = 1		/*!< передача (байт считывается из буфера) */
} hal_i2c_direction_t;

/*! Описание master-передачи I2C */
struct hal_i2c_master_transfer_t {
	hal_i2c_device_t device;	/*!< описание slave-устройства */
	bool use_pec;				/*!< true - использовать PEC */
	hal_i2c_direction_t *dirs;	/*!< указатель на направления передачи для соответствующих байт data */
	uint8_t *data;				/*!< указатель на буфер данных для чтения/записи */
	int size;					/*!< размер данных */
	void *userid;				/*!< (опциональный) пользовательский идентификатор */
	/*! callback, вызываемый из ISR, индицирующий завершение передачи, начатой вызовом hal_i2c_start_master_transfer() */
	void (*isrcallbackTransferCompleted)(struct hal_i2c_master_transfer_t *t, hal_i2c_transfer_result_t result, signed portBASE_TYPE *pxHigherPriorityTaskWoken);
	struct hal_i2c_master_transfer_t *previous, *next; /*!< для внутреннего использования */
};

/*! Описание параметров режима SMBus host */
typedef struct {
	void *userid;		/*!< (опциональный) пользовательский идентификатор для callback'ов */
	/*! callback, вызываемый из ISR, индицирующий получение сообщения host notify */
	void (*isrcallbackMessageReceived)(hal_i2c_smbus_handle_t handle, uint8_t address, uint16_t status, signed portBASE_TYPE *pxHigherPriorityTaskWoken);
} hal_i2c_smbus_host_params_t;

/*! Устанавливает режим работы шины I2C
 *
 * Внимание! При установке hi2cModeOff:
 * - если на шине активна передача, то она прерывается и вызов блокируется до завершения операции (STOP condition);
 * - ожидающие в очереди передачи отменяются.
 * \param[in] instance	номер экземпляра аппаратного I2C в диапазоне [1..3]
 * \param[in] mode		режим работы
 */
void hal_i2c_set_bus_mode(int instance, hal_i2c_mode_t mode);

/*! Запускает master-передачу данных из/в slave-устройство I2C
 *
 * Вызов запускает передачу t и возвращает результат запуска.
 * После успешного запуска структура *t и все данные, описываемые ей,
 * должны оставаться валидными до окончания передачи или ее отмены.
 * Запущенная передача становится в очередь обработки.
 * При завершении передачи выполняется isrcallbackTransferCompleted с результатом.
 * Если результат передачи успешный, то:
 * - на всех позициях hi2cDirectionRx буфера данных t->data содержатся валидные данные;
 * - данные с всех позиций hi2cDirectionTx буфера данных t->data успешно переданы.
 * \param[in,out] t		указатель на описание передачи
 * \return true - передача успешно запущена, false - передача не запущена
 */
bool hal_i2c_start_master_transfer(struct hal_i2c_master_transfer_t *t);

/*! Отменяет master-передачу I2C
 *
 * Если передача t была запущена ранее вызовом hal_i2c_start_master_transfer() и в данный момент активна, то она прерывается.
 * При завершении отмены выполняется isrcallbackTransferCompleted с результатом hi2cErrorAborted.
 * \param[in,out] t		указатель на описание передачи
 * \return true - передача успешно отменена, false - передача не отменена
 */
bool hal_i2c_abort_master_transfer(struct hal_i2c_master_transfer_t *t);

/*! Открывает режим SMBus host на I2C-шине
 *
 * I2C должен быть установлен в режим hi2cModeSMBus.
 * \param[in] bus_instance	номер экземпляра аппаратного I2C в диапазоне [1..3]
 * \param[in] params		указатель на параметры SMBus host
 * \retval handle			хэндл для использования в последующих вызовах (NULL в случае ошибки)
 */
hal_i2c_smbus_handle_t hal_i2c_open_smbus_host(int bus_instance, hal_i2c_smbus_host_params_t *params);

/*! Закрывает режим SMBus host на I2C-шине
 *
 * Если идет прием сообщения, то он не прерывается, а просто отбрасывается по завершении.
 */
void hal_i2c_close_smbus_host(hal_i2c_smbus_handle_t handle);

/*! Возвращает значение пользовательского идентификатора SMBus host
 *
 * \return значение поля userid параметров открытия
 */
void* hal_i2c_get_smbus_userid(hal_i2c_smbus_handle_t handle);

#ifdef __cplusplus
}
#endif

#endif /* HAL_I2C_H_ */
