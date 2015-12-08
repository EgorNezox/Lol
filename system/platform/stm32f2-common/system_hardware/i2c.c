/**
  ******************************************************************************
  * @file    i2c.c
  * @author  Artem Pisarenko, PMR dept. software team, ONIIP, PJSC
  * @date    24.11.2015
  * @brief   Реализация аппаратной абстракции доступа к I2C на STM32F2xx
  *
  * Notes:
  * - Don't set NOSTRETCH bit in slave transmit mode (workaround p2.3.4 of Errata sheet Rev 5).
  * - I2C bus must be enabled on persistent basis (rather than for master transfer duration only),
  *   because there are no way to get "STOP finished"/"bus not busy" event required to disable bus properly.
  * - After stopping master transfer, a dummy START-STOP transaction executed to workaround
  *   "shitty" STM32F2 I2C peripheral limitation (there are no "STOP finished"/"bus not busy" event)
  * - Enabling/disabling smbus host may execute dummy START-STOP transaction, because it's the only way
  *   to set/clear ACK bit while peripheral enabled avoiding racing with possible addressing from
  *   other master on the bus (when ADDR is set but we don't know whether peripheral acknowledged it).
  *
  ******************************************************************************
  */

#include "stm32f2xx.h"
#include "FreeRTOS.h"

#include "sys_internal.h"
#include "hal_i2c.h"

#define I2C_MASTER_SPEED_CLOCK	88000 // using 88kHz clock to workaround p2.3.3 of Errata sheet Rev 5
#define I2C_INSTANCES_COUNT 3
#define I2C_SR1_BUS_ERROR_FLAGS	(I2C_SR1_BERR | I2C_SR1_ARLO | I2C_SR1_TIMEOUT)
#define I2C_CR1_SMBHOST_BITS	(I2C_CR1_ACK | I2C_CR1_SMBTYPE | I2C_CR1_ENARP)

#define DEFINE_PCB_FROM_HANDLE(var_pcb, handle) \
	struct s_i2cbus_pcb *var_pcb = (struct s_i2cbus_pcb *)handle; \
	SYS_ASSERT(var_pcb != 0);

typedef enum {
	stateIdle,
	stateStartingMaster,
	stateMasterAddressing,
	stateMasterRxSingleByte,
	stateMasterRxFirstN2Bytes,
	stateMasterRxLast2Bytes,
	stateMasterTxData,
	stateMasterTxPEC,
	stateMasterTxStopping,
	stateMasterFinishing,
	stateSlaveTx,
	stateSlaveRxSMBusHostAddress,
	stateSlaveRxSMBusHostDataLow,
	stateSlaveRxSMBusHostDataHigh,
	stateSlaveStopping
} i2c_state_t;

static struct s_i2cbus_pcb {
	I2C_TypeDef* i2c;
	IRQn_Type ev_irq_n;
	IRQn_Type er_irq_n;
	uint32_t rcc_apb_mask;
	hal_i2c_mode_t mode;
	i2c_state_t state;
	bool smbus_host_enabled;
	hal_i2c_smbus_host_params_t smbus_host_params;
	struct hal_i2c_master_transfer_t *master_transfer_queue_top;
	struct hal_i2c_master_transfer_t *master_transfer_queue_bottom;
	struct hal_i2c_master_transfer_t *active_mt;
	hal_i2c_transfer_result_t active_mt_result;
	int active_mt_pos;
	uint8_t active_mt_pec;
	struct hal_i2c_master_transfer_t *aborted_mt;
	i2c_state_t last_state; // used in slave mode communications
	bool smbushost_transfer_ready;
	uint8_t smbushost_address;
	uint16_t smbushost_status;
} i2cbus_pcbs[I2C_INSTANCES_COUNT] = {
	{I2C1, I2C1_EV_IRQn, I2C1_ER_IRQn, RCC_APB1ENR_I2C1EN, hi2cModeOff, stateIdle, false, {0, 0}, 0, 0, 0, hi2cSuccess, 0, 0, 0, stateIdle, false, 0, 0},
	{I2C2, I2C2_EV_IRQn, I2C2_ER_IRQn, RCC_APB1ENR_I2C2EN, hi2cModeOff, stateIdle, false, {0, 0}, 0, 0, 0, hi2cSuccess, 0, 0, 0, stateIdle, false, 0, 0},
	{I2C3, I2C3_EV_IRQn, I2C3_ER_IRQn, RCC_APB1ENR_I2C3EN, hi2cModeOff, stateIdle, false, {0, 0}, 0, 0, 0, hi2cSuccess, 0, 0, 0, stateIdle, false, 0, 0}
};

static uint8_t i2c_pec_precalc_table[256];

static void i2c_push_to_master_transfer_queue(struct s_i2cbus_pcb *i2cbus, struct hal_i2c_master_transfer_t *t);
static struct hal_i2c_master_transfer_t* i2c_pop_from_master_transfer_queue(struct s_i2cbus_pcb *i2cbus);
static bool i2c_master_transfer_queue_is_empty(struct s_i2cbus_pcb *i2cbus);
static void i2c_remove_from_master_transfer_queue(struct s_i2cbus_pcb *i2cbus, struct hal_i2c_master_transfer_t *t);
static inline void i2c_set_irq_pending(struct s_i2cbus_pcb *i2cbus);
static uint32_t i2c_get_smbhost_bits(struct s_i2cbus_pcb *i2cbus);
static inline void i2c_request_start_condition(struct s_i2cbus_pcb *i2cbus);
static inline void i2c_request_stop_condition(struct s_i2cbus_pcb *i2cbus);
static void i2c_reset_master_mode(struct s_i2cbus_pcb *i2cbus, uint16_t i2c_SR2);
static void i2c_stop_master_transfer(struct s_i2cbus_pcb *i2cbus);
static void i2c_abort_master_transfer(struct s_i2cbus_pcb *i2cbus);
static void i2c_close_active_master_transfer(struct s_i2cbus_pcb *i2cbus, signed portBASE_TYPE *pxHigherPriorityTaskWoken);
static bool i2c_process_master_rx_error_condition(struct s_i2cbus_pcb *i2cbus, uint16_t i2c_SR1, signed portBASE_TYPE *pxHigherPriorityTaskWoken);
static bool i2c_process_master_tx_error_condition(struct s_i2cbus_pcb *i2cbus, uint16_t i2c_SR1, signed portBASE_TYPE *pxHigherPriorityTaskWoken);
static void i2c_receive_master_data_byte(struct s_i2cbus_pcb *i2cbus);
static void i2c_transmit_master_data_byte(struct s_i2cbus_pcb *i2cbus);
static void i2c_process_slave_mode_change(struct s_i2cbus_pcb *i2cbus);
static int i2c_get_master_max_bytes_to_receive(struct s_i2cbus_pcb *i2cbus, uint16_t i2c_SR2);
static inline uint8_t i2c_calc_pec(uint8_t pec, uint8_t data);
static void i2c_irq_handler(struct s_i2cbus_pcb *i2cbus);
static void i2c_irq_handler_master(struct s_i2cbus_pcb *i2cbus, uint16_t i2c_SR1, uint16_t i2c_SR2, signed portBASE_TYPE *pxHigherPriorityTaskWoken);
static void i2c_irq_handler_smbus_host(struct s_i2cbus_pcb *i2cbus, uint16_t i2c_SR1, uint16_t i2c_SR2, signed portBASE_TYPE *pxHigherPriorityTaskWoken);

void halinternal_i2c_init(void) {
	for (int i = 0; i < sizeof(i2cbus_pcbs)/sizeof(i2cbus_pcbs[0]); i++) {
		halinternal_set_nvic_priority(i2cbus_pcbs[i].ev_irq_n);
		NVIC_EnableIRQ(i2cbus_pcbs[i].ev_irq_n);
		halinternal_set_nvic_priority(i2cbus_pcbs[i].er_irq_n);
		NVIC_EnableIRQ(i2cbus_pcbs[i].er_irq_n);
	}
	for (uint32_t i = 0; i < sizeof(i2c_pec_precalc_table)/sizeof(i2c_pec_precalc_table[0]); i++) {
		uint32_t value = i;
		for (int j = 0; j < 8; j++)
			if (value & 0x80)
				value = (value << 1) ^ 0x07;
			else
				value <<= 1;
		i2c_pec_precalc_table[i] = value & 0xFF;
	}
}

void hal_i2c_set_bus_mode(int instance, hal_i2c_mode_t mode) {
	SYS_ASSERT((1 <= instance) && (instance <= I2C_INSTANCES_COUNT));
	struct s_i2cbus_pcb *i2cbus = &(i2cbus_pcbs[instance-1]);
	switch (mode) {
	case hi2cModeStandard:
	case hi2cModeSMBus: {
		uint32_t freqrange;
		halinternal_rcc_clocks_t rcc_clocks;
		uint32_t ccr_value;
		portDISABLE_INTERRUPTS();
		SYS_ASSERT(i2cbus->mode == hi2cModeOff);
		i2cbus->mode = mode;
		RCC->APB1ENR |= (i2cbus->rcc_apb_mask);
		__DSB();
		i2cbus->i2c->CR1 |= I2C_CR1_SWRST;
		i2cbus->i2c->CR1 &= ~I2C_CR1_SWRST;
		halinternal_get_rcc_clocks(&rcc_clocks);
		freqrange = rcc_clocks.pclk1_frequency/1000000;
		i2cbus->i2c->CR2 |= freqrange & I2C_CR2_FREQ;
		i2cbus->i2c->CR2 |= (I2C_CR2_ITEVTEN | I2C_CR2_ITERREN);
		i2cbus->i2c->TRISE = (freqrange + 1) & I2C_TRISE_TRISE; // using standard speed mode
		ccr_value = rcc_clocks.pclk1_frequency/(I2C_MASTER_SPEED_CLOCK << 1);
		i2cbus->i2c->CCR = (((ccr_value & I2C_CCR_CCR) < 4)? 4 : (ccr_value)) & I2C_CCR_CCR; // using standard speed mode
		if (mode == hi2cModeSMBus)
			i2cbus->i2c->CR1 |= I2C_CR1_SMBUS;
		i2cbus->i2c->CR1 |= I2C_CR1_PE;
		portENABLE_INTERRUPTS();
		break;
	}
	case hi2cModeOff: {
		bool iterface_is_busy;
		portDISABLE_INTERRUPTS();
		SYS_ASSERT(i2cbus->mode != hi2cModeOff);
		i2cbus->mode = mode;
		if (i2cbus->active_mt == 0) { // no master transfer in progress ?
			i2cbus->state = stateIdle;
		} else {
			i2c_abort_master_transfer(i2cbus);
		}
		i2cbus->smbus_host_enabled = false;
		portENABLE_INTERRUPTS();
		do {
			portDISABLE_INTERRUPTS();
			iterface_is_busy = !((i2cbus->state == stateIdle) && ((i2cbus->i2c->SR2 & I2C_SR2_MSL) == 0));
			if (!iterface_is_busy) {
				i2cbus->i2c->CR1 = 0;
				__DSB();
				RCC->APB1ENR &= ~(i2cbus->rcc_apb_mask);
			}
			portENABLE_INTERRUPTS();
		} while (iterface_is_busy);
		break;
	}
	default: SYS_ASSERT(0);
	}
}

bool hal_i2c_start_master_transfer(struct hal_i2c_master_transfer_t *t) {
	SYS_ASSERT(t);
	SYS_ASSERT((1 <= t->device.bus_instance) && (t->device.bus_instance <= I2C_INSTANCES_COUNT));
	SYS_ASSERT((t->device.address & 0x7F) == 0);
	SYS_ASSERT(t->size >= 0);
	if (t->size > 0) {
		SYS_ASSERT(t->dirs);
		SYS_ASSERT(t->data);
	} else {
		SYS_ASSERT(t->use_pec == false);
	}
	SYS_ASSERT(t->isrcallbackTransferCompleted);
	struct s_i2cbus_pcb *i2cbus = &(i2cbus_pcbs[t->device.bus_instance-1]);
	bool success = false;
	portDISABLE_INTERRUPTS();
	if (i2cbus->mode != hi2cModeOff) {
		i2c_push_to_master_transfer_queue(i2cbus, t);
		if (i2cbus->state == stateIdle)
			i2c_set_irq_pending(i2cbus);
		success = true;
	}
	portENABLE_INTERRUPTS();
	return success;
}

bool hal_i2c_abort_master_transfer(struct hal_i2c_master_transfer_t *t) {
	SYS_ASSERT(t);
	SYS_ASSERT((1 <= t->device.bus_instance) && (t->device.bus_instance <= I2C_INSTANCES_COUNT));
	struct s_i2cbus_pcb *i2cbus = &(i2cbus_pcbs[t->device.bus_instance-1]);
	bool success = false;
	portDISABLE_INTERRUPTS();
	if (i2cbus->mode != hi2cModeOff) {
		if (t == i2cbus->active_mt) { // is it in progress ?
			i2c_abort_master_transfer(i2cbus);
			success = true;
		} else { // it's expected to be still queued
			if (!i2c_master_transfer_queue_is_empty(i2cbus)) { // checks case when it removed from top of queue just few moments ago
				i2c_remove_from_master_transfer_queue(i2cbus, t);
				SYS_ASSERT(i2cbus->aborted_mt == 0);
				i2cbus->aborted_mt = t;
				i2c_set_irq_pending(i2cbus);
				success = true;
			}
		}
	}
	portENABLE_INTERRUPTS();
	return success;
}

hal_i2c_smbus_handle_t hal_i2c_open_smbus_host(int bus_instance, hal_i2c_smbus_host_params_t *params) {
	SYS_ASSERT(params);
	SYS_ASSERT((1 <= bus_instance) && (bus_instance <= I2C_INSTANCES_COUNT));
	SYS_ASSERT(params->isrcallbackMessageReceived);
	struct s_i2cbus_pcb *i2cbus = &(i2cbus_pcbs[bus_instance-1]);
	bool success = false;
	portDISABLE_INTERRUPTS();
	if ((i2cbus->mode == hi2cModeSMBus) && (i2cbus->smbus_host_enabled == false)) {
		i2cbus->smbus_host_enabled = true;
		i2cbus->smbus_host_params = *params;
		i2c_process_slave_mode_change(i2cbus);
		success = true;
	}
	portENABLE_INTERRUPTS();
	if (!success)
		i2cbus = 0;
	return (hal_i2c_smbus_handle_t)i2cbus;
}

void hal_i2c_close_smbus_host(hal_i2c_smbus_handle_t handle) {
	DEFINE_PCB_FROM_HANDLE(i2cbus, handle)
	portDISABLE_INTERRUPTS();
	if (i2cbus->smbus_host_enabled == true) {
		i2cbus->smbus_host_enabled = false;
		i2cbus->smbus_host_params.userid = 0;
		i2cbus->smbus_host_params.isrcallbackMessageReceived = 0;
		i2c_process_slave_mode_change(i2cbus);
	}
	portENABLE_INTERRUPTS();
}

void* hal_i2c_get_smbus_userid(hal_i2c_smbus_handle_t handle) {
	DEFINE_PCB_FROM_HANDLE(i2cbus, handle)
	return i2cbus->smbus_host_params.userid;
}

static void i2c_push_to_master_transfer_queue(struct s_i2cbus_pcb *i2cbus, struct hal_i2c_master_transfer_t *t) {
	t->previous = 0;
	t->next = 0;
	if (i2cbus->master_transfer_queue_bottom) {
		SYS_ASSERT(i2cbus->master_transfer_queue_top != 0);
		t->previous = i2cbus->master_transfer_queue_bottom;
		i2cbus->master_transfer_queue_bottom = t;
	} else {
		SYS_ASSERT(i2cbus->master_transfer_queue_top == 0);
		i2cbus->master_transfer_queue_bottom = t;
		i2cbus->master_transfer_queue_top = i2cbus->master_transfer_queue_bottom;
	}
}

static struct hal_i2c_master_transfer_t* i2c_pop_from_master_transfer_queue(struct s_i2cbus_pcb *i2cbus) {
	struct hal_i2c_master_transfer_t *next = i2cbus->master_transfer_queue_top;
	if (next)
		i2c_remove_from_master_transfer_queue(i2cbus, next);
	return next;
}

static bool i2c_master_transfer_queue_is_empty(struct s_i2cbus_pcb *i2cbus) {
	return (i2cbus->master_transfer_queue_top == 0);
}

static void i2c_remove_from_master_transfer_queue(struct s_i2cbus_pcb *i2cbus, struct hal_i2c_master_transfer_t *t) {
	if (t->previous) {
		SYS_ASSERT(i2cbus->master_transfer_queue_top != 0);
		t->previous->next = t->next;
		t->previous = 0;
	} else {
		SYS_ASSERT(i2cbus->master_transfer_queue_top == t);
		i2cbus->master_transfer_queue_top = t->next;
	}
	if (t->next) {
		SYS_ASSERT(i2cbus->master_transfer_queue_bottom != 0);
		t->next->previous = t->previous;
		t->next = 0;
	} else {
		SYS_ASSERT(i2cbus->master_transfer_queue_bottom == t);
		i2cbus->master_transfer_queue_bottom = t->previous;
	}
}

static inline void i2c_set_irq_pending(struct s_i2cbus_pcb *i2cbus) {
	NVIC_SetPendingIRQ(i2cbus->ev_irq_n);
}

static uint32_t i2c_get_smbhost_bits(struct s_i2cbus_pcb *i2cbus) {
	if (((i2cbus->mode != hi2cModeOff)) && i2cbus->smbus_host_enabled)
		return I2C_CR1_SMBHOST_BITS;
	return 0;
}

static inline void i2c_request_start_condition(struct s_i2cbus_pcb *i2cbus) {
	i2cbus->i2c->CR2 &= ~I2C_CR2_ITBUFEN; // otherwise RXNE,TXE flags (if set) will hold interrupt pending
	// set START + prevent setting STOP and/or PEC requests
	i2cbus->i2c->CR1 = (i2cbus->i2c->CR1 & ~(I2C_CR1_STOP | I2C_CR1_PEC)) | I2C_CR1_START;
}

static inline void i2c_request_stop_condition(struct s_i2cbus_pcb *i2cbus) {
	/* It's a safe place to change ACK bit for slave mode (see design note in source header) */
	// set STOP + prevent setting START,PEC requests + clear ACK,POS,ENPEC bits + change smbhost bits
	i2cbus->i2c->CR1 = \
			(i2cbus->i2c->CR1 & ~(I2C_CR1_START | I2C_CR1_PEC | I2C_CR1_ACK | I2C_CR1_POS | I2C_CR1_ENPEC | I2C_CR1_SMBHOST_BITS))
			| I2C_CR1_STOP
			| i2c_get_smbhost_bits(i2cbus);
}

static void i2c_reset_master_mode(struct s_i2cbus_pcb *i2cbus, uint16_t i2c_SR2) {
	if ((i2c_SR2 & I2C_SR2_MSL) == 0)
		return;
	i2cbus->i2c->CR2 &= ~I2C_CR2_ITBUFEN;
	/* Following operations must be executed exactly in specified sequence in order to
	 * avoid resetting/breaking/missing slave communication which may happen after STOP in multi-master bus.
	 */
	(void)i2cbus->i2c->SR1; // dummy read to reset SB(1st)
	i2c_request_stop_condition(i2cbus);
	i2cbus->i2c->DR = 0; // dummy write to reset SB(2nd) and BTF, it's safe to do after entering slave mode, because DR access ignored until ADDR cleared
}

static void i2c_stop_master_transfer(struct s_i2cbus_pcb *i2cbus) {
	i2cbus->i2c->CR2 &= ~I2C_CR2_ITBUFEN; // otherwise RXNE,TXE flags (if set) will hold interrupt pending
	/* It's a safe place to change ACK bit for slave mode (see design note in source header) */
	// set STOP,START + prevent setting PEC request + clear ACK,POS,ENPEC bits + change smbhost bits
	i2cbus->i2c->CR1 = \
			(i2cbus->i2c->CR1 & ~(I2C_CR1_PEC | I2C_CR1_ACK | I2C_CR1_POS | I2C_CR1_ENPEC | I2C_CR1_SMBHOST_BITS))
			| (I2C_CR1_STOP | I2C_CR1_START)
			| i2c_get_smbhost_bits(i2cbus);
	i2cbus->state = stateMasterFinishing;
}

static void i2c_abort_master_transfer(struct s_i2cbus_pcb *i2cbus) {
	SYS_ASSERT((i2cbus->active_mt != 0) && (i2cbus->aborted_mt == 0));
	i2cbus->aborted_mt = i2cbus->active_mt;
	if ((i2cbus->state == stateMasterFinishing)
			|| ((i2cbus->state == stateMasterRxSingleByte) && ((i2cbus->active_mt_pos + 1) == i2cbus->active_mt->size))) { // STOP requested ?
		i2cbus->i2c->CR2 &= ~I2C_CR2_ITBUFEN;
		/* For stateMasterFinishing case:
		 * START requested and interface will enter master mode. No worry - it will be handled in idle state.
		 */
		i2cbus->state = stateIdle;
	} else if (i2cbus->state != stateMasterAddressing) {
		/* All other states except addressing.
		 * In addressing state we are in race condition, which may cause ADDR flag left pending.
		 * This case has proper handling in corresponding state.
		 */
		i2c_reset_master_mode(i2cbus, i2cbus->i2c->SR2);
		i2cbus->state = stateIdle;
	}
	i2cbus->active_mt = 0;
	i2c_set_irq_pending(i2cbus);
}

static void i2c_close_active_master_transfer(struct s_i2cbus_pcb *i2cbus, signed portBASE_TYPE *pxHigherPriorityTaskWoken) {
	struct hal_i2c_master_transfer_t *t = i2cbus->active_mt;
	i2cbus->active_mt = 0;
	t->isrcallbackTransferCompleted(t, i2cbus->active_mt_result, pxHigherPriorityTaskWoken);
}

static bool i2c_process_master_rx_error_condition(struct s_i2cbus_pcb *i2cbus, uint16_t i2c_SR1, signed portBASE_TYPE *pxHigherPriorityTaskWoken) {
	if ((i2c_SR1 & I2C_SR1_BUS_ERROR_FLAGS) == 0) // don't handle AF (it cannot occur in master rx states)
		return false;
	i2cbus->i2c->SR1 = ~I2C_SR1_BUS_ERROR_FLAGS;
	i2cbus->i2c->CR2 &= ~I2C_CR2_ITBUFEN;
	(void)i2cbus->i2c->DR; // dummy read to clear RxNE flag
	i2cbus->active_mt_result = hi2cErrorBus;
	switch (i2cbus->state) {
	case stateMasterRxSingleByte:
		/* START/STOP already generated in addressing state or by peripheral.
		 * If addressing state generated (Repeated) START request, it's not clear whether peripheral cancels it or not.
		 * It's safer to close transfer here and fall-back to idle state.
		 * If peripheral didn't canceled START and will enter in master mode, it will handled in idle state (just in case).
		 */
		i2cbus->state = stateIdle;
		i2c_close_active_master_transfer(i2cbus, pxHigherPriorityTaskWoken);
		break;
	case stateMasterRxFirstN2Bytes:
	case stateMasterRxLast2Bytes:
		if ((i2c_SR1 & (I2C_SR1_BERR)) != 0) { // we should stop transfer ourselves ?..
			i2c_stop_master_transfer(i2cbus);
		} else { // ... or peripheral already did it ?
			i2c_request_start_condition(i2cbus); // just to properly set smbhost bits on stopping (see design notes in source header)
			i2cbus->state = stateMasterFinishing;
		}
		break;
	default: SYS_ASSERT(0);
	}
	return true;
}

static bool i2c_process_master_tx_error_condition(struct s_i2cbus_pcb *i2cbus, uint16_t i2c_SR1, signed portBASE_TYPE *pxHigherPriorityTaskWoken) {
	if ((i2c_SR1 & (I2C_SR1_AF | I2C_SR1_BUS_ERROR_FLAGS)) == 0)
		return false;
	i2cbus->i2c->SR1 = ~(I2C_SR1_AF | I2C_SR1_BUS_ERROR_FLAGS);
	i2cbus->i2c->CR2 &= ~I2C_CR2_ITBUFEN;
	if ((i2c_SR1 & I2C_SR1_BUS_ERROR_FLAGS) != 0)
		i2cbus->active_mt_result = hi2cErrorBus;
	else
		i2cbus->active_mt_result = hi2cErrorDataNACK;
	switch (i2cbus->state) {
	case stateMasterTxData:
	case stateMasterTxPEC:
	case stateMasterTxStopping:
		if ((i2c_SR1 & (I2C_SR1_AF | I2C_SR1_BERR)) != 0) { // we should stop transfer ourselves ?..
			i2c_stop_master_transfer(i2cbus);
		} else { // ... or peripheral already did it ?
			i2c_request_start_condition(i2cbus); // just to properly set smbhost bits on stopping (see design notes in source header)
			i2cbus->state = stateMasterFinishing;
		}
		break;
	default: SYS_ASSERT(0);
	}
	return true;
}

static void i2c_receive_master_data_byte(struct s_i2cbus_pcb *i2cbus) {
	uint8_t byte = i2cbus->i2c->DR;
	i2cbus->active_mt_pec = i2c_calc_pec(i2cbus->active_mt_pec, byte);
	i2cbus->active_mt->data[i2cbus->active_mt_pos++] = byte;
}

static void i2c_transmit_master_data_byte(struct s_i2cbus_pcb *i2cbus) {
	uint8_t byte = i2cbus->active_mt->data[i2cbus->active_mt_pos++];
	i2cbus->active_mt_pec = i2c_calc_pec(i2cbus->active_mt_pec, byte);
	i2cbus->i2c->DR = byte;
}

static void i2c_process_slave_mode_change(struct s_i2cbus_pcb *i2cbus) {
	if ((i2cbus->active_mt == 0) && (i2cbus->state != stateStartingMaster)) { // no master transaction in progress ?
		/* Request master transaction (empty) */
		i2c_request_start_condition(i2cbus);
	}
	/* After requested (or current) master transaction finished, change will take effect. */
}

static bool i2c_process_slave_rx_conditions(struct s_i2cbus_pcb *i2cbus, uint16_t i2c_SR1) {
	if ((i2c_SR1 & I2C_SR1_RXNE) == 0) {
		if ((i2c_SR1 & (I2C_SR1_STOPF | I2C_SR1_BUS_ERROR_FLAGS)) != 0)
			i2cbus->state = stateSlaveStopping;
		return false;
	}
	return true;
}

/* Function returns MAXIMUM raw bytes count scheduled for reception from current buffer position, but not actual count.
 * It looks for closest direction change within (pos, pos+2] buffer range (assuming buffer[pos] at rx direction).
 * Function used in master transfer sequences for N>=2 reception (see STM32F2 reference manual).
 */
static int i2c_get_master_max_bytes_to_receive(struct s_i2cbus_pcb *i2cbus, uint16_t i2c_SR2) {
	int bytes = i2cbus->active_mt->size - i2cbus->active_mt_pos + i2cbus->active_mt->use_pec;
	int offset = min(2, bytes - 1 - i2cbus->active_mt->use_pec);
	while (offset > 0) {
		if (i2cbus->active_mt->dirs[i2cbus->active_mt_pos + offset] != hi2cDirectionRx)
			bytes = offset;
		offset--;
	}
	return bytes;
}

static inline uint8_t i2c_calc_pec(uint8_t pec, uint8_t data) {
	return i2c_pec_precalc_table[(pec ^ data) & 0xFF];
}

static void i2c_irq_handler(struct s_i2cbus_pcb *i2cbus) {
	portBASE_TYPE xHigherPriorityTaskWoken = pdFALSE;
	if (i2cbus->aborted_mt != 0) {
		i2cbus->active_mt_result = hi2cErrorAborted;
		i2c_close_active_master_transfer(i2cbus, &xHigherPriorityTaskWoken);
	}
	if ((i2cbus->i2c->CR1 & I2C_CR1_PE) != 0) {
		/* I2C status registers must be read once and exactly in specified sequence in order to:
		 * - prevent loss of pending flags;
		 * - prevent loss of events occurred while irq handler is active.
		 */
		uint16_t i2c_SR2 = i2cbus->i2c->SR2;
		uint16_t i2c_SR1 = i2cbus->i2c->SR1;
		/* Process unsupported/unexpected interrupt events */
		if ((i2c_SR1 & (I2C_SR1_ADD10 | I2C_SR1_SMBALERT | I2C_SR1_PECERR | I2C_SR1_OVR)) != 0) {
			/* Something wrong, because:
			 * - 10-bit addressing not used (not supported)
			 * - SMBus alert feature isn't enabled;
			 * - PEC isn't used (PECEN=0) in any transfer;
			 * - clock stretching isn't disabled (NOSTRETCH=0).
			 */
			SYS_ASSERT(0);
			i2cbus->i2c->SR1 = ~(I2C_SR1_ADD10 | I2C_SR1_SMBALERT | I2C_SR1_PECERR | I2C_SR1_OVR);
		}
		/* Slave and master handlers must be run exactly in specified sequence in order to
		 * prevent stuck with non-handled pending flags kept from some communication scenarios.
		 * Example:
		 * - application requested master transfer (requested START, entered stateStartingMaster);
		 * - some master on bus executed empty "address-only" communication, which caused ADDR and STOPF flags being set;
		 * - requested START completed;
		 * - if master handler will detect SB first, it proceeds and change state to "master-only" state,
		 *   after that, slave handler will not clear STOPF.
		 * In other words, we should consider slave events as more prioritized over master events
		 * due to uncontrolled transitions in slave mode communication.
		 */
		i2c_irq_handler_smbus_host(i2cbus, i2c_SR1, i2c_SR2, &xHigherPriorityTaskWoken);
		i2c_irq_handler_master(i2cbus, i2c_SR1, i2c_SR2, &xHigherPriorityTaskWoken);
	}
	portEND_SWITCHING_ISR(xHigherPriorityTaskWoken);
}

static void i2c_irq_handler_master(struct s_i2cbus_pcb *i2cbus, uint16_t i2c_SR1, uint16_t i2c_SR2, signed portBASE_TYPE *pxHigherPriorityTaskWoken) {
	switch (i2cbus->state) {
	case stateMasterFinishing: { // must be placed before stateIdle and fall-through to it in order to request STOP (if required)
		if ((i2c_SR1 & I2C_SR1_TIMEOUT) == 0) { // don't handle BERR, AF and ARLO (they cannot occur in this state)
			if ((i2c_SR1 & I2C_SR1_SB) == 0)
				break;
		} else {
			i2cbus->i2c->SR1 = ~I2C_SR1_TIMEOUT;
			if (i2cbus->active_mt != 0)
				i2cbus->active_mt_result = hi2cErrorBus;
		}
		i2cbus->state = stateIdle;
		if (i2cbus->active_mt != 0)
			i2c_close_active_master_transfer(i2cbus, pxHigherPriorityTaskWoken);
		i2c_SR1 = i2cbus->i2c->SR1; // update for correct fall-through (synchronized with read sequence in i2c_irq_handler())
	}
	/* no break */
	case stateIdle: { // this state is universal entry for processing pending (queued) transfers: either start next one or drop all of them
		if ((i2c_SR2 & I2C_SR2_MSL) != 0) // required after stateMasterFinishing and some special cases when START request leaved pending (such as aborted transfers)
			i2c_reset_master_mode(i2cbus, i2c_SR2);
		if ((i2c_SR1 & (I2C_SR1_AF | I2C_SR1_BUS_ERROR_FLAGS)) != 0) {
			/* These flags might be left from previous aborted transfer. Just reset and forget them. */
			i2cbus->i2c->SR1 = ~(I2C_SR1_AF | I2C_SR1_BUS_ERROR_FLAGS);
		}
		if (i2cbus->mode == hi2cModeOff) {
			/* Flush all pending master transfers as aborted and quit */
			struct hal_i2c_master_transfer_t *t;
			while ((bool)(t = i2c_pop_from_master_transfer_queue(i2cbus)))
				t->isrcallbackTransferCompleted(t, hi2cErrorAborted, pxHigherPriorityTaskWoken);
			break;
		}
		if (i2c_master_transfer_queue_is_empty(i2cbus))
			break;
		// we cannot fetch transfer item before we entered master mode because of possible multi-master races, which could activate slave mode communication
		i2c_request_start_condition(i2cbus);
		i2cbus->state = stateStartingMaster;
		break;
	}
	case stateStartingMaster: { // this state also used for Repeated START and enabling/disabling smbus host
		// don't handle BERR, AF, ARLO, TIMEOUT because they cannot occur in this state
		if ((i2c_SR1 & I2C_SR1_SB) == 0)
			break;
		if (i2cbus->active_mt == 0) { // no master transfer in progress ?
			i2cbus->active_mt = i2c_pop_from_master_transfer_queue(i2cbus); // try(!) to fetch next master transfer
			if (i2cbus->active_mt == 0) {
				i2c_request_stop_condition(i2cbus);
				i2cbus->state = stateIdle;
				break;
			}
			i2cbus->active_mt_result = hi2cSuccess; // optimistic initialization (any error will overwrite it)
			i2cbus->active_mt_pos = 0;
			i2cbus->active_mt_pec = 0;
		}
		uint8_t address = (i2cbus->active_mt->device.address << 1); // transmitter mode selected
		// for zero-size transfer it's important to set transmitter mode
		if ((i2cbus->active_mt->size > 0)
				&& (i2cbus->active_mt_pos < i2cbus->active_mt->size)
				&& (i2cbus->active_mt->dirs[i2cbus->active_mt_pos] == hi2cDirectionRx))
			address |= 0x01; // selects receiver mode
		i2cbus->i2c->CR1 &= ~(I2C_CR1_ACK | I2C_CR1_POS | I2C_CR1_ENPEC);
		i2cbus->i2c->DR = address; // also clears RxNE flag from previous transfer
		i2cbus->state = stateMasterAddressing;
		i2cbus->active_mt_pec = i2c_calc_pec(i2cbus->active_mt_pec, address);
		break;
	}
	case stateMasterAddressing: {
		if ((i2c_SR1 & (I2C_SR1_AF | I2C_SR1_BUS_ERROR_FLAGS)) != 0) {
			(void)i2cbus->i2c->SR2; // dummy read to clear ADDR (properly synchronized with read sequence in i2c_irq_handler())
			i2cbus->i2c->SR1 = ~(I2C_SR1_AF | I2C_SR1_BUS_ERROR_FLAGS);
			if ((i2c_SR1 & I2C_SR1_BUS_ERROR_FLAGS) != 0)
				i2cbus->active_mt_result = hi2cErrorBus;
			else
				i2cbus->active_mt_result = hi2cErrorAddressNACK;
			if ((i2c_SR1 & (I2C_SR1_AF | I2C_SR1_BERR)) != 0) { // we should stop transfer ourselves ?..
				i2c_stop_master_transfer(i2cbus);
			} else { // ... or peripheral already did it ?
				i2c_request_start_condition(i2cbus); // just to properly set smbhost bits on stopping (see design notes in source header)
				i2cbus->state = stateMasterFinishing;
			}
			break;
		}
		if ((i2c_SR1 & I2C_SR1_ADDR) == 0)
			break;
		if ((i2cbus->active_mt != 0) && (i2cbus->active_mt->size > 0)) {
			int max_bytes_to_receive = 0;
			if ((i2c_SR2 & I2C_SR2_TRA) == 0) { // receiver mode ?
				max_bytes_to_receive = i2c_get_master_max_bytes_to_receive(i2cbus, i2c_SR2);
				if (max_bytes_to_receive > 1)
					i2cbus->i2c->CR1 |= I2C_CR1_ACK;
				if (max_bytes_to_receive == 2)
					i2cbus->i2c->CR1 |= I2C_CR1_POS;
			}
			i2cbus->i2c->CR2 |= I2C_CR2_ITBUFEN;
			(void)i2cbus->i2c->SR2; // dummy read to clear ADDR (properly synchronized with read sequence in i2c_irq_handler())
			if ((i2c_SR2 & I2C_SR2_TRA) == 0) { // receiver mode ?
				if (max_bytes_to_receive == 1) {
					i2cbus->state = stateMasterRxSingleByte;
					if ((i2cbus->active_mt_pos + 1) < i2cbus->active_mt->size) { // not last byte in transfer ?
						i2cbus->i2c->CR1 |= I2C_CR1_START; // Repeated START (we cannot use i2c_request_start_condition() because it disables ITBUFEN)
					} else { // last byte in transfer ?
						i2c_request_stop_condition(i2cbus); // STOP
					}
				} else {
					i2cbus->state = stateMasterRxFirstN2Bytes;
				}
			} else { // transmitter mode ?
				i2cbus->state = stateMasterTxData;
			}
		} else {
			// dummy read to clear ADDR (properly synchronized with read sequence in i2c_irq_handler()), transmitter mode ensures no transfer will be started
			(void)i2cbus->i2c->SR2;
			i2c_stop_master_transfer(i2cbus);
		}
		break;
	}
	case stateMasterRxSingleByte: {
		if (i2c_process_master_rx_error_condition(i2cbus, i2c_SR1, pxHigherPriorityTaskWoken))
			break;
		if ((i2c_SR1 & I2C_SR1_RXNE) == 0)
			break;
		if ((i2cbus->active_mt_pos + 1) < i2cbus->active_mt->size) { // not last byte in transfer ?
			// already requested START in addressing state
			i2cbus->i2c->CR2 &= ~I2C_CR2_ITBUFEN;
			i2cbus->state = stateStartingMaster;
		} else { // last byte in transfer ?
			// already requested STOP in addressing state
			i2c_request_start_condition(i2cbus);
			i2cbus->state = stateMasterFinishing;
		}
		/* received byte is data byte (single byte transfers cannot contain PEC) */
		i2c_receive_master_data_byte(i2cbus);
		break;
	}
	case stateMasterRxFirstN2Bytes: {
		if (i2c_process_master_rx_error_condition(i2cbus, i2c_SR1, pxHigherPriorityTaskWoken))
			break;
		if ((i2c_SR1 & I2C_SR1_RXNE) == 0)
			break;
		int max_bytes_to_receive = i2c_get_master_max_bytes_to_receive(i2cbus, i2c_SR2);
		if ((max_bytes_to_receive == 2) || (max_bytes_to_receive == 3)) {
			i2cbus->i2c->CR1 &= ~I2C_CR1_ACK; // must be disabled before N-2 byte read
			i2cbus->i2c->CR2 &= ~I2C_CR2_ITBUFEN; // otherwise RxNE flag will hold interrupt pending
		}
		if (max_bytes_to_receive > 2) {
			/* received byte is data byte (first N-2 bytes cannot contain PEC) */
			i2c_receive_master_data_byte(i2cbus);
			break;
		}
		i2cbus->state = stateMasterRxLast2Bytes;
		i2c_SR1 = i2cbus->i2c->SR1; // update for correct fall-through (synchronized with read sequence in i2c_irq_handler())
	}
	/* no break */
	case stateMasterRxLast2Bytes: {
		if (i2c_process_master_rx_error_condition(i2cbus, i2c_SR1, pxHigherPriorityTaskWoken))
			break;
		if ((i2c_SR1 & I2C_SR1_BTF) == 0)
			break;
		if ((i2cbus->active_mt_pos + 2) < i2cbus->active_mt->size) { // not last bytes in total transfer ?
			i2c_request_start_condition(i2cbus); // Repeated START
			i2cbus->state = stateStartingMaster;
		} else { // last bytes in total transfer ?
			i2c_stop_master_transfer(i2cbus); // STOP
		}
		for (int i = 0; i < 2; i++) {
			if (!((i == 1) && i2cbus->active_mt->use_pec)) {
				i2c_receive_master_data_byte(i2cbus);
			} else {
				uint8_t pec = i2cbus->i2c->DR;
				if (pec != i2cbus->active_mt_pec)
					i2cbus->active_mt_result = hi2cErrorPEC;
			}
		}
		break;
	}
	case stateMasterTxData: {
		if (i2c_process_master_tx_error_condition(i2cbus, i2c_SR1, pxHigherPriorityTaskWoken))
			break;
		if ((i2c_SR1 & I2C_SR1_TXE) == 0)
			break;
		i2c_transmit_master_data_byte(i2cbus);
		if (i2cbus->active_mt_pos < i2cbus->active_mt->size) {
			// transfer direction changed ?
			if (((i2c_SR2 & I2C_SR2_TRA) >> POSITION_VAL(I2C_SR2_TRA)) != i2cbus->active_mt->dirs[i2cbus->active_mt_pos]) {
				i2c_request_start_condition(i2cbus); // Repeated START
				i2cbus->state = stateStartingMaster;
			}
		} else {
			if (!i2cbus->active_mt->use_pec) {
				i2cbus->i2c->CR2 &= ~I2C_CR2_ITBUFEN; // otherwise TXE flag will hold interrupt pending
				i2cbus->state = stateMasterTxStopping;
			} else {
				i2cbus->state = stateMasterTxPEC;
			}
		}
		break;
	}
	case stateMasterTxPEC: {
		if (i2c_process_master_tx_error_condition(i2cbus, i2c_SR1, pxHigherPriorityTaskWoken))
			break;
		if ((i2c_SR1 & I2C_SR1_TXE) == 0)
			break;
		i2cbus->i2c->DR = i2cbus->active_mt_pec;
		i2cbus->i2c->CR2 &= ~I2C_CR2_ITBUFEN; // otherwise TXE flag will hold interrupt pending
		i2cbus->state = stateMasterTxStopping;
		i2c_SR1 = i2cbus->i2c->SR1; // update for correct fall-through (synchronized with read sequence in i2c_irq_handler())
	}
	/* no break */
	case stateMasterTxStopping: {
		if (i2c_process_master_tx_error_condition(i2cbus, i2c_SR1, pxHigherPriorityTaskWoken))
			break;
		if ((i2c_SR1 & I2C_SR1_BTF) == 0)
			break;
		i2c_stop_master_transfer(i2cbus);
		break;
	}
	default:
		break;
	}
}

static void i2c_irq_handler_smbus_host(struct s_i2cbus_pcb *i2cbus, uint16_t i2c_SR1, uint16_t i2c_SR2, signed portBASE_TYPE *pxHigherPriorityTaskWoken) {
	switch (i2cbus->state) {
	case stateIdle:
	case stateStartingMaster: { // we handling master state here because of possible multi-master races
		// possible I2C_SR1_BUS_ERROR_FLAGS are handled in master handler
		if ((i2c_SR1 & I2C_SR1_ADDR) == 0)
			break;
		(void)i2cbus->i2c->SR2; // dummy read to clear ADDR (properly synchronized with read sequence in i2c_irq_handler())
		if ((i2cbus->i2c->SR1 & I2C_SR1_STOPF) != 0) { // important check for STOPF must be done right here (also reading SR1 starts clearing flag)
			// dummy write to complete clearing STOPF (synchronized with read sequence in i2c_irq_handler()), also prevents setting START, STOP, PEC requests
			i2cbus->i2c->CR1 = i2cbus->i2c->CR1 & ~(I2C_CR1_START | I2C_CR1_STOP | I2C_CR1_PEC);
			break; // just stay at same state (it was empty "address-only" communication)
		}
		if ((i2cbus->i2c->CR1 & I2C_CR1_ACK) == 0) { // proper way to determine peripheral state (don't use smbus_host_enabled value)
			/* We are still getting ADDR requests from other masters on bus, because peripheral is always enabled (even with ACK cleared). */
			break;
		}
		i2cbus->last_state = i2cbus->state; // after transfer completion we should return to either stateIdle or stateStartingMaster
		i2cbus->smbushost_transfer_ready = false;
		i2cbus->i2c->CR2 |= I2C_CR2_ITBUFEN;
		if ((i2c_SR2 & I2C_SR2_TRA) == 0) {
			SYS_ASSERT((i2c_SR2 & I2C_SR2_SMBHOST) != 0);
			i2cbus->state = stateSlaveRxSMBusHostAddress;
		} else {
			/* We don't support slave transmitter mode, but there are no way to abort it. */
			i2cbus->state = stateSlaveTx;
		}
		break;
	}
	case stateSlaveTx: {
		/* We don't support slave transmitter mode, so we just feeding master until it stop transfer (or error occur) */
		if ((i2c_SR1 & (I2C_SR1_AF | I2C_SR1_BUS_ERROR_FLAGS | I2C_SR1_STOPF)) != 0) {
			i2cbus->i2c->SR1 = ~(I2C_SR1_AF | I2C_SR1_BUS_ERROR_FLAGS);
			i2cbus->i2c->CR2 &= ~I2C_CR2_ITBUFEN; // otherwise TXE flag will hold interrupt pending
			i2cbus->state = stateSlaveStopping;
		} else if ((i2c_SR1 & I2C_SR1_TXE) != 0) {
			i2cbus->i2c->DR = 0xFF; // also clears TXE, BTF flags
		}
		break;
	}
	case stateSlaveRxSMBusHostAddress: {
		if (!i2c_process_slave_rx_conditions(i2cbus, i2c_SR1))
			break;
		i2cbus->smbushost_address = i2cbus->i2c->DR;
		i2cbus->state = stateSlaveRxSMBusHostDataLow;
		i2c_SR1 = i2cbus->i2c->SR1; // update for correct RXNE/STOPF fall-through (synchronized with read sequence in i2c_irq_handler())
	}
	/* no break */
	case stateSlaveRxSMBusHostDataLow: {
		if (!i2c_process_slave_rx_conditions(i2cbus, i2c_SR1))
			break;
		i2cbus->smbushost_status = i2cbus->i2c->DR & 0xFF;
		i2cbus->state = stateSlaveRxSMBusHostDataHigh;
		i2c_SR1 = i2cbus->i2c->SR1; // update for correct RXNE/STOPF fall-through (synchronized with read sequence in i2c_irq_handler())
	}
	/* no break */
	case stateSlaveRxSMBusHostDataHigh: {
		if (!i2c_process_slave_rx_conditions(i2cbus, i2c_SR1))
			break;
		i2cbus->smbushost_status |= (i2cbus->i2c->DR << 8) & 0xFF00;
		i2cbus->smbushost_transfer_ready = true;
		i2cbus->state = stateSlaveStopping;
		i2c_SR1 = i2cbus->i2c->SR1; // update for correct RXNE/STOPF fall-through (synchronized with read sequence in i2c_irq_handler())
	}
	/* no break */
	case stateSlaveStopping: {
		if ((i2c_SR1 & I2C_SR1_RXNE) != 0) {
			i2cbus->smbushost_transfer_ready = false; // invalidate transfer if any extra bytes was received
			(void)i2cbus->i2c->DR; // dummy read to reset RxNE (byte in data register)
			(void)i2cbus->i2c->DR; // dummy read to reset RxNE (in case when second byte waited in shift register)
		}
		if ((i2c_SR1 & (I2C_SR1_STOPF | I2C_SR1_BUS_ERROR_FLAGS)) == 0)
			break;
		// dummy write to complete clearing STOPF (synchronized with read sequence in i2c_irq_handler()), also prevents setting START, STOP, PEC requests
		i2cbus->i2c->CR1 = i2cbus->i2c->CR1 & ~(I2C_CR1_START | I2C_CR1_STOP | I2C_CR1_PEC);
		if ((i2c_SR1 & I2C_SR1_BUS_ERROR_FLAGS) != 0) {
			i2cbus->smbushost_transfer_ready = false; // invalidate transfer on any error
			i2cbus->i2c->SR1 = ~I2C_SR1_BUS_ERROR_FLAGS;
		}
		i2cbus->i2c->CR2 &= ~I2C_CR2_ITBUFEN;
		i2cbus->state = i2cbus->last_state;
		if (i2cbus->smbus_host_enabled && i2cbus->smbushost_transfer_ready)
			i2cbus->smbus_host_params.isrcallbackMessageReceived((hal_i2c_smbus_handle_t)i2cbus, i2cbus->smbushost_address, i2cbus->smbushost_status, pxHigherPriorityTaskWoken);
		break;
	}
	default:
		break;
	}
}

void I2C1_EV_IRQHandler(void) {
	i2c_irq_handler(&i2cbus_pcbs[0]);
}
void I2C2_EV_IRQHandler(void) {
	i2c_irq_handler(&i2cbus_pcbs[1]);
}
void I2C3_EV_IRQHandler(void) {
	i2c_irq_handler(&i2cbus_pcbs[3]);
}
void I2C1_ER_IRQHandler(void) {
	i2c_irq_handler(&i2cbus_pcbs[0]);
}
void I2C2_ER_IRQHandler(void) {
	i2c_irq_handler(&i2cbus_pcbs[1]);
}
void I2C3_ER_IRQHandler(void) {
	i2c_irq_handler(&i2cbus_pcbs[2]);
}
