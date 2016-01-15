/**
  ******************************************************************************
  * @file    i2c.c
  * @author  Artem Pisarenko, PMR dept. software team, ONIIP, PJSC
  * @date    24.11.2015
  * @brief   Реализация аппаратной абстракции доступа к I2C на STM32F2xx
  *
  * Notes:
  * - Don't set NOSTRETCH bit in slave transmit mode (workaround p2.3.4 of Errata sheet Rev 5).
  * - Don't set NOSTRETCH bit because otherwise we cannot handle slave mode transfer properly
  *   (not only because of data can't be ready in time, but also because of avoiding of possible races
  *   between writing CR1 register and losing STOPF flag).
  * - I2C bus must be enabled on persistent basis (rather than for master transfer duration only),
  *   because there are no way to get "STOP finished"/"bus not busy" event required to disable bus properly.
  * - After stopping master transfer, a dummy START-STOP transaction executed.
  *   It's a workaround for "shitty" STM32F2 I2C peripheral limitation (there are no "STOP finished"/"bus not busy" event).
  * - Enabling/disabling smbus host may execute dummy START-STOP transaction, because it's the only way
  *   to set/clear ACK bit while peripheral enabled avoiding racing with possible addressing in multimaster bus
  *   (when ADDR is set but we don't know whether peripheral acknowledged it).
  *   You might suppose that we would temporarily disable bus, but we cannot check or wait a state,
  *   when interface will be slave mode, because it requires SR2 polling (unacceptable),
  *   and also reading SR2 may cause side effects we cannot handle.
  * - SMBus mode violates specification by not supporting timeout mechanism.
  *   It's because we cannot use controller in SMBUS=1 hardware mode due to defective TIMEOUT error implementation.
  *   TIMEOUT error in master transfer may cause exit master mode in uncontrolled and "software-asynchronous" way.
  *   Firstly, it doesn't allow to safely set/clear ACK bit due to racing condition (see previous notes).
  *   Secondly, if ACK bit left set while smbus host disabled, controller will acknowledge any arbitrary address (from OAR1)
  *   issued by other master on bus between returning to slave mode and restoring ACK=0 by software.
  *   As a consequence, SMBus host notify feature uses OAR1 with SMBus Host address rather than using ENARP=1 and SMBTYPE=1.
  *   (But even if we would use SMBUS=1, we still must use this method, because we want to acknowledge only this one address,
  *    but we cannot disable acknowledging of address contained in OAR1.)
  * - In SMBus mode host may miss host notification if it happens at the same time (or shortly after)
  *   it accesses bus to perform master transaction and loses arbitration.
  *   It's a hardware limitations we are not able to workaround.
  * - During testing a new hardware bug has been discovered (not present in errata sheet and isn't confirmed by ST team at this time).
  *   Conditions: controller configured in slave mode (Standard Mode I2C, NOSTRETCH=0), and two consecutive transfers (START-address-data-...-STOP)
  *   being executed: slave transmitter mode, immediately(!) followed by slave receiver mode without enough clock stretching.
  *   After that, controller cannot generate START condition until software issues SWRST.
  *   Workaround is to insert 1ms delay in slave transfer between getting ADDR=1 and clearing ADDR, forcing controller to stretch SCL low.
  * Features, limitations and strange behavior in this implementation are result of many compromises and hard work.
  * (Say thanks to STMicroelectronics for their wonderful controller!)
  *
  ******************************************************************************
  */

#include "stm32f2xx.h"
#include "FreeRTOS.h"
#include "timers.h"

#include "sys_internal.h"
#include "hal_i2c.h"

#define I2C_MASTER_SPEED_CLOCK	88000 // using 88kHz clock to workaround p2.3.3 of Errata sheet Rev 5
#define I2C_INSTANCES_COUNT 3
#define I2C_SMBUS_HOST_ADDRESS	0x08
#define I2C_SR1_BUS_ERROR_FLAGS	(I2C_SR1_BERR | I2C_SR1_ARLO)

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
	stateMasterTxStarting,
	stateMasterTxData,
	stateMasterTxPEC,
	stateMasterTxStopping,
	stateMasterFinishing,
	stateSlaveAddressed,
	stateSlaveReadyToTransfer,
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
	TimerHandle_t slave_addr_delay_timer;
	hal_i2c_mode_t mode;
	i2c_state_t state;
	bool smbus_host_enabled;
	bool smbus_host_change_request;
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
	{I2C1, I2C1_EV_IRQn, I2C1_ER_IRQn, RCC_APB1ENR_I2C1EN, 0, hi2cModeOff, stateIdle, false, false, {0, 0}, 0, 0, 0, hi2cSuccess, 0, 0, 0, stateIdle, false, 0, 0},
	{I2C2, I2C2_EV_IRQn, I2C2_ER_IRQn, RCC_APB1ENR_I2C2EN, 0, hi2cModeOff, stateIdle, false, false, {0, 0}, 0, 0, 0, hi2cSuccess, 0, 0, 0, stateIdle, false, 0, 0},
	{I2C3, I2C3_EV_IRQn, I2C3_ER_IRQn, RCC_APB1ENR_I2C3EN, 0, hi2cModeOff, stateIdle, false, false, {0, 0}, 0, 0, 0, hi2cSuccess, 0, 0, 0, stateIdle, false, 0, 0}
};

static uint8_t i2c_pec_precalc_table[256];

static void i2c_push_to_master_transfer_queue(struct s_i2cbus_pcb *i2cbus, struct hal_i2c_master_transfer_t *t);
static struct hal_i2c_master_transfer_t* i2c_pop_from_master_transfer_queue(struct s_i2cbus_pcb *i2cbus);
static bool i2c_master_transfer_queue_is_empty(struct s_i2cbus_pcb *i2cbus);
static void i2c_remove_from_master_transfer_queue(struct s_i2cbus_pcb *i2cbus, struct hal_i2c_master_transfer_t *t);
static inline void i2c_set_irq_pending(struct s_i2cbus_pcb *i2cbus);
static uint32_t i2c_get_slave_mode_ack(struct s_i2cbus_pcb *i2cbus);
static inline void i2c_request_start_condition(struct s_i2cbus_pcb *i2cbus);
static inline void i2c_request_stop_condition(struct s_i2cbus_pcb *i2cbus);
static void i2c_stop_master_transfer(struct s_i2cbus_pcb *i2cbus);
static void i2c_reset_master_mode(struct s_i2cbus_pcb *i2cbus);
static void i2c_reset_master_mode_to_idle(struct s_i2cbus_pcb *i2cbus);
static void i2c_restore_bus_after_error(struct s_i2cbus_pcb *i2cbus, uint16_t i2c_SR1, uint32_t i2c_SR1_stop_flags);
static void i2c_abort_master_transfer(struct s_i2cbus_pcb *i2cbus);
static void i2c_close_active_master_transfer(struct s_i2cbus_pcb *i2cbus, signed portBASE_TYPE *pxHigherPriorityTaskWoken);
static bool i2c_process_master_rx_error_condition(struct s_i2cbus_pcb *i2cbus, uint16_t i2c_SR1, signed portBASE_TYPE *pxHigherPriorityTaskWoken);
static bool i2c_process_master_tx_error_condition(struct s_i2cbus_pcb *i2cbus, uint16_t i2c_SR1, signed portBASE_TYPE *pxHigherPriorityTaskWoken);
static void i2c_receive_master_data_byte(struct s_i2cbus_pcb *i2cbus);
static void i2c_transmit_master_data_byte(struct s_i2cbus_pcb *i2cbus);
static void i2c_request_change_slave_mode(struct s_i2cbus_pcb *i2cbus);
static bool i2c_process_slave_rx_conditions(struct s_i2cbus_pcb *i2cbus, uint16_t i2c_SR1);
static int i2c_get_master_max_bytes_to_receive(struct s_i2cbus_pcb *i2cbus);
static inline uint8_t i2c_calc_pec(uint8_t pec, uint8_t data);
static void i2c_timer_slave_addr_delay_callback(TimerHandle_t xTimer);
static void i2c_irq_handler(struct s_i2cbus_pcb *i2cbus);
static void i2c_irq_handler_smbus_host(struct s_i2cbus_pcb *i2cbus, uint16_t i2c_SR1, signed portBASE_TYPE *pxHigherPriorityTaskWoken);
static void i2c_irq_handler_master(struct s_i2cbus_pcb *i2cbus, uint16_t i2c_SR1, signed portBASE_TYPE *pxHigherPriorityTaskWoken);

void halinternal_i2c_init(void) {
	for (int i = 0; i < sizeof(i2cbus_pcbs)/sizeof(i2cbus_pcbs[0]); i++) {
		struct s_i2cbus_pcb *i2cbus = &(i2cbus_pcbs[i]);
		i2cbus->slave_addr_delay_timer = xTimerCreate("HAL_I2C_slave_addr_delay_X", 1/portTICK_PERIOD_MS, pdFALSE, (void *)i2cbus, i2c_timer_slave_addr_delay_callback);
		halinternal_freertos_timer_queue_length += 1; // must match usage scenario (xTimerXXX calls, calls contexts, worst case of queuing)
		halinternal_set_nvic_priority(i2cbus->ev_irq_n);
		NVIC_EnableIRQ(i2cbus->ev_irq_n);
		halinternal_set_nvic_priority(i2cbus->er_irq_n);
		NVIC_EnableIRQ(i2cbus->er_irq_n);
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
		i2cbus->i2c->CCR = (((ccr_value & I2C_CCR_CCR) < 4)? 4 : (ccr_value)) & I2C_CCR_CCR; // using standard speed mode (also for SMBus mode)
		if (mode == hi2cModeSMBus) {
			/* don't use SMBUS=1 mode (see design notes in source header) */
			i2cbus->i2c->OAR1 = (I2C_SMBUS_HOST_ADDRESS << POSITION_VAL(I2C_OAR1_ADD1_7)); // slave mode: 7-bit addressing, SMBus Host address
		}
		i2cbus->i2c->CR1 |= I2C_CR1_PE;
		i2cbus->smbus_host_change_request = false;
		portENABLE_INTERRUPTS();
		break;
	}
	case hi2cModeOff: {
		bool interface_is_busy;
		portDISABLE_INTERRUPTS();
		SYS_ASSERT(i2cbus->mode != hi2cModeOff);
		i2cbus->mode = mode;
		i2cbus->smbus_host_enabled = false;
		if (i2cbus->active_mt == 0) { // no master transfer in progress ?
			if (i2cbus->state != stateStartingMaster) { // special case (no worries, it has proper handling for updated mode)
				/* Slave transfer may be in progress and will be dropped. */
				i2cbus->state = stateIdle;
				i2c_set_irq_pending(i2cbus);
			}
		} else {
			i2c_abort_master_transfer(i2cbus); // it will return to idle state
		}
		/* Since now we are in idle state (or on way to it). Idle state has appropriate cleanup for switching off. */
		portENABLE_INTERRUPTS();
		do {
			portDISABLE_INTERRUPTS();
			// short-circuit evaluation is important here: don't read SR2 until we reach idle state (to prevent loss of flags)
			interface_is_busy = !((i2cbus->state == stateIdle) && ((i2cbus->i2c->SR2 & I2C_SR2_MSL) == 0));
			if (!interface_is_busy) {
				i2cbus->i2c->CR1 = 0;
				__DSB();
				RCC->APB1ENR &= ~(i2cbus->rcc_apb_mask);
			}
			portENABLE_INTERRUPTS();
		} while (interface_is_busy);
		break;
	}
	default: SYS_ASSERT(0);
	}
}

bool hal_i2c_start_master_transfer(struct hal_i2c_master_transfer_t *t) {
	SYS_ASSERT(t);
	SYS_ASSERT((1 <= t->device.bus_instance) && (t->device.bus_instance <= I2C_INSTANCES_COUNT));
	SYS_ASSERT((t->device.address & ~0x7F) == 0);
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
		i2c_request_change_slave_mode(i2cbus);
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
		i2c_request_change_slave_mode(i2cbus);
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
		i2cbus->master_transfer_queue_bottom->next = t;
		i2cbus->master_transfer_queue_bottom = t;
	} else {
		SYS_ASSERT(i2cbus->master_transfer_queue_top == 0);
		i2cbus->master_transfer_queue_bottom = t;
		i2cbus->master_transfer_queue_top = t;
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
	} else {
		SYS_ASSERT(i2cbus->master_transfer_queue_top == t);
		i2cbus->master_transfer_queue_top = t->next;
	}
	if (t->next) {
		SYS_ASSERT(i2cbus->master_transfer_queue_bottom != 0);
		t->next->previous = t->previous;
	} else {
		SYS_ASSERT(i2cbus->master_transfer_queue_bottom == t);
		i2cbus->master_transfer_queue_bottom = t->previous;
	}
	t->previous = 0;
	t->next = 0;
}

static inline void i2c_set_irq_pending(struct s_i2cbus_pcb *i2cbus) {
	NVIC_SetPendingIRQ(i2cbus->ev_irq_n);
}

static uint32_t i2c_get_slave_mode_ack(struct s_i2cbus_pcb *i2cbus) {
	uint32_t ack_bit = 0;
	if ((i2cbus->mode != hi2cModeOff) && i2cbus->smbus_host_enabled)
		ack_bit = I2C_CR1_ACK;
	return ack_bit;
}

static inline void i2c_request_start_condition(struct s_i2cbus_pcb *i2cbus) {
	i2cbus->i2c->CR2 &= ~I2C_CR2_ITBUFEN; // otherwise RXNE,TXE flags (if set) will hold interrupt pending
	// set START + prevent setting STOP, PEC requests
	i2cbus->i2c->CR1 = (i2cbus->i2c->CR1 & ~(I2C_CR1_STOP | I2C_CR1_PEC)) | I2C_CR1_START;
	i2cbus->state = stateStartingMaster;
}

static inline void i2c_request_stop_condition(struct s_i2cbus_pcb *i2cbus) {
	/* Set STOP + prevent setting START,PEC requests + clear POS,ENPEC bits + change slave mode ack.
	 * It's a safe place to change ACK bit (also used to service "change slave mode" request, see design note in source header).
	 */
	i2cbus->i2c->CR1 = \
			(i2cbus->i2c->CR1 & ~(I2C_CR1_START | I2C_CR1_PEC | I2C_CR1_ACK | I2C_CR1_POS | I2C_CR1_ENPEC))
			| I2C_CR1_STOP
			| i2c_get_slave_mode_ack(i2cbus);
	i2cbus->smbus_host_change_request = false;
}

static void i2c_stop_master_transfer(struct s_i2cbus_pcb *i2cbus) {
	i2cbus->i2c->CR2 &= ~I2C_CR2_ITBUFEN; // otherwise RXNE,TXE flags (if set) will hold interrupt pending
	/* Set STOP,START + prevent setting PEC request + clear POS,ENPEC bits + change slave mode ack.
	 * It's a safe place to change ACK bit (also used to service "change slave mode" request, see design note in source header).
	 * Here software relies on not clearly documented behavior, but tested on hardware (STM32F207IGH6).
	 * In master mode it generates STOP and then START.
	 */
	i2cbus->i2c->CR1 = \
			(i2cbus->i2c->CR1 & ~(I2C_CR1_PEC | I2C_CR1_ACK | I2C_CR1_POS | I2C_CR1_ENPEC))
			| (I2C_CR1_STOP | I2C_CR1_START)
			| i2c_get_slave_mode_ack(i2cbus);
	i2cbus->smbus_host_change_request = false;
	i2cbus->state = stateMasterFinishing;
}

static void i2c_reset_master_mode(struct s_i2cbus_pcb *i2cbus) {
	i2cbus->i2c->CR2 &= ~I2C_CR2_ITBUFEN;
	/* Following operations must be executed exactly in specified sequence in order to
	 * avoid resetting/breaking/missing slave communication which may happen after STOP in multi-master bus.
	 */
	(void)i2cbus->i2c->SR1; // dummy read to reset SB(1st)
	i2c_request_stop_condition(i2cbus);
	i2cbus->i2c->DR = 0; // dummy write to reset SB(2nd) and BTF, it's safe to do after entering slave mode, because DR access ignored until ADDR cleared
}

static void i2c_reset_master_mode_to_idle(struct s_i2cbus_pcb *i2cbus) {
	i2c_reset_master_mode(i2cbus);
	i2cbus->state = stateIdle;
	i2c_set_irq_pending(i2cbus);
}

static void i2c_restore_bus_after_error(struct s_i2cbus_pcb *i2cbus, uint16_t i2c_SR1, uint32_t i2c_SR1_stop_flags) {
	if ((i2c_SR1 & i2c_SR1_stop_flags) != 0) // we should stop transfer ourselves ?..
		i2c_request_stop_condition(i2cbus);
	else  // ... or peripheral already did it ?
		i2cbus->smbus_host_change_request = true; // just to restore slave mode ack (see design notes in source header)
	i2cbus->state = stateIdle;
	i2c_set_irq_pending(i2cbus);
}

static void i2c_abort_master_transfer(struct s_i2cbus_pcb *i2cbus) {
	SYS_ASSERT((i2cbus->active_mt != 0) && (i2cbus->aborted_mt == 0));
	i2cbus->aborted_mt = i2cbus->active_mt;
	/* All states (except starting tx) will handle abort itself in order to prevent racing with possible bus errors. */
	switch (i2cbus->state) {
	case stateMasterTxStarting: {
		/* Already in sync with data boundary (it's safe to stop here). */
		i2c_reset_master_mode_to_idle(i2cbus);
		break;
	}
	case stateMasterRxFirstN2Bytes:
	case stateMasterTxData:
	case stateMasterTxPEC: {
		/* Required to sync BTF. */
		i2cbus->i2c->CR2 &= ~I2C_CR2_ITBUFEN; // otherwise RXNE,TXE flags will hold interrupt pending
		break;
	}
	case stateMasterFinishing: {
		i2cbus->state = stateStartingMaster;
		break;
	}
	default: break;
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
	i2cbus->i2c->CR2 &= ~I2C_CR2_ITBUFEN;
	switch (i2cbus->state) {
	case stateMasterRxSingleByte:
	case stateMasterRxFirstN2Bytes:
	case stateMasterRxLast2Bytes:
		i2c_restore_bus_after_error(i2cbus, i2c_SR1, (I2C_SR1_BERR));
		// dummy read to clear BTF flag and "push" stopping if required (it's safe to do after entering slave mode, because DR access ignored until ADDR cleared)
		(void)i2cbus->i2c->DR;
		break;
	default: SYS_ASSERT(0);
	}
	i2cbus->i2c->SR1 = ~I2C_SR1_BUS_ERROR_FLAGS;
	if (i2cbus->active_mt) {
		i2cbus->active_mt_result = hi2cErrorBus;
		i2c_close_active_master_transfer(i2cbus, pxHigherPriorityTaskWoken);
	}
	return true;
}

static bool i2c_process_master_tx_error_condition(struct s_i2cbus_pcb *i2cbus, uint16_t i2c_SR1, signed portBASE_TYPE *pxHigherPriorityTaskWoken) {
	if ((i2c_SR1 & (I2C_SR1_AF | I2C_SR1_BUS_ERROR_FLAGS)) == 0)
		return false;
	i2cbus->i2c->CR2 &= ~I2C_CR2_ITBUFEN;
	switch (i2cbus->state) {
	case stateMasterTxData:
	case stateMasterTxPEC:
	case stateMasterTxStopping:
		i2c_restore_bus_after_error(i2cbus, i2c_SR1, (I2C_SR1_AF | I2C_SR1_BERR));
		// dummy read to clear BTF flag, it's safe to do after entering slave mode, because DR access ignored until ADDR cleared
		(void)i2cbus->i2c->DR;
		break;
	default: SYS_ASSERT(0);
	}
	i2cbus->i2c->SR1 = ~(I2C_SR1_AF | I2C_SR1_BUS_ERROR_FLAGS);
	if (i2cbus->active_mt) {
		if ((i2c_SR1 & I2C_SR1_BUS_ERROR_FLAGS) != 0)
			i2cbus->active_mt_result = hi2cErrorBus;
		else
			i2cbus->active_mt_result = hi2cErrorDataNACK;
		i2c_close_active_master_transfer(i2cbus, pxHigherPriorityTaskWoken);
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

static void i2c_request_change_slave_mode(struct s_i2cbus_pcb *i2cbus) {
	i2cbus->smbus_host_change_request = true;
	i2c_set_irq_pending(i2cbus);
}

static bool i2c_process_slave_rx_conditions(struct s_i2cbus_pcb *i2cbus, uint16_t i2c_SR1) {
	if ((i2c_SR1 & I2C_SR1_RXNE) == 0) {
		if ((i2c_SR1 & (I2C_SR1_STOPF | I2C_SR1_BUS_ERROR_FLAGS)) != 0) // BERR flag may also mean Repeated START but we don't support it anyway
			i2cbus->state = stateSlaveStopping;
		return false;
	}
	return true;
}

/* Function returns MAXIMUM raw bytes count scheduled for reception from current buffer position, but not actual count.
 * It looks for closest direction change within (pos, pos+2] buffer range (assuming buffer[pos] at rx direction).
 * Function used in master transfer sequences for N>=2 reception (see STM32F2 reference manual).
 */
static int i2c_get_master_max_bytes_to_receive(struct s_i2cbus_pcb *i2cbus) {
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

static void i2c_timer_slave_addr_delay_callback(TimerHandle_t xTimer) {
	DEFINE_PCB_FROM_HANDLE(i2cbus, pvTimerGetTimerID(xTimer))
	/* It's a part of workaround for hardware bug (see design notes in source header). */
	portDISABLE_INTERRUPTS();
	if ((i2cbus->mode != hi2cModeOff) && (i2cbus->state == stateSlaveAddressed)) {
		i2cbus->state = stateSlaveReadyToTransfer;
		/* Enable back previously disabled interrupt.
		 * Since ADDR flag is set, it should activate interrupt (and further processing) immediately after this section.
		 */
		i2cbus->i2c->CR2 |= I2C_CR2_ITEVTEN;
	}
	portENABLE_INTERRUPTS();
}

static void i2c_irq_handler(struct s_i2cbus_pcb *i2cbus) {
	portBASE_TYPE xHigherPriorityTaskWoken = pdFALSE;
	if (i2cbus->aborted_mt != 0) {
		struct hal_i2c_master_transfer_t *t = i2cbus->aborted_mt;
		i2cbus->aborted_mt = 0;
		t->isrcallbackTransferCompleted(t, hi2cErrorAborted, &xHigherPriorityTaskWoken);
	}
	if ((i2cbus->i2c->CR1 & I2C_CR1_PE) != 0) {
		/* I2C status registers must be read very carefully in order to:
		 * - prevent loss of pending flags;
		 * - prevent loss of events occurred while irq handler is active.
		 */
		uint16_t i2c_SR1 = i2cbus->i2c->SR1; // such caching allows independent post-processing of already reset flags.
		/* Process unsupported/unexpected interrupt events. */
		if ((i2c_SR1 & (I2C_SR1_ADD10 | I2C_SR1_OVR | I2C_SR1_PECERR | I2C_SR1_TIMEOUT | I2C_SR1_SMBALERT)) != 0) {
			/* Something wrong, because:
			 * - 10-bit addressing not used (not supported);
			 * - clock stretching isn't disabled (NOSTRETCH=0);
			 * - PEC isn't used (PECEN=0) in any transfer;
			 * - timeout error cannot occur in I2C mode (SMBUS=0);
			 * - SMBus alert feature isn't enabled.
			 */
			SYS_ASSERT(0);
			i2cbus->i2c->SR1 = ~(I2C_SR1_ADD10 | I2C_SR1_OVR | I2C_SR1_PECERR | I2C_SR1_TIMEOUT | I2C_SR1_SMBALERT);
		}
		/* Process very specific error conditions before mode handlers. */
		if (((i2cbus->state == stateIdle) || (i2cbus->state == stateStartingMaster) || (i2cbus->state == stateMasterFinishing))
				&& ((i2c_SR1 & (I2C_SR1_AF | I2C_SR1_BUS_ERROR_FLAGS)) != 0)) { // errors left from previous aborted transfer and/or finishing procedure ?
			/* Just ignore and reset them before we may enter master/slave transaction (current state either hasn't error handling, or state will be changed). */
			i2cbus->i2c->SR1 = ~(I2C_SR1_AF | I2C_SR1_BUS_ERROR_FLAGS);
		}
		/* Slave and master handlers must be run exactly in specified sequence in order to
		 * prevent stuck with not handled pending flags kept from some communication scenarios.
		 * In other words, we should consider slave events as more prioritized over master events
		 * due to uncontrolled transitions in slave mode communication.
		 */
		i2c_irq_handler_smbus_host(i2cbus, i2c_SR1, &xHigherPriorityTaskWoken);
		i2c_irq_handler_master(i2cbus, i2c_SR1, &xHigherPriorityTaskWoken);
	}
	portEND_SWITCHING_ISR(xHigherPriorityTaskWoken);
}

static void i2c_irq_handler_smbus_host(struct s_i2cbus_pcb *i2cbus, uint16_t i2c_SR1, signed portBASE_TYPE *pxHigherPriorityTaskWoken) {
	switch (i2cbus->state) {
	case stateIdle:
	case stateStartingMaster:
	case stateMasterFinishing: { // we handle master states here because of possible multi-master races
		if ((i2c_SR1 & I2C_SR1_ADDR) == 0)
			break;
		/* don't check STOPF flag, because we cannot get it here when clock stretching enabled. */
		/* It's not clearly documented, whether we get ADDR requests when ACK is cleared, or don't.
		 * The most unclear thing is about how hardware behaves in possible racing between changing ACK bit and getting address match on bus.
		 * Just in case...
		 */
		if (((i2cbus->i2c->CR1 & I2C_CR1_ACK) == 0) // proper way to determine peripheral state (don't use smbus_host_enabled value)
				|| (i2cbus->mode == hi2cModeOff)) {
			(void)i2cbus->i2c->SR2; // dummy read to clear ADDR flag (properly synchronized with reading SR1 in i2c_irq_handler())
			break;
		}
		/* Since here controller is in transfer state (either supported one - receiving SMBus Host notification, or any transmission) */
		if (i2cbus->state == stateMasterFinishing) {
			/* Looks like bus master was fast enough to inject its transaction within bus free time between a STOP and START (Tbuf).
			 * (It should not happen with correct controllers, but handle it just in case.)
			 * Various checks within other places assume that no master transfer could wait finishing while slave communication in progress,
			 * so we must close it here.
			 */
			SYS_ASSERT(i2cbus->active_mt); // aborted transfers cannot reach this state
			if ((i2c_SR1 & I2C_SR1_BUS_ERROR_FLAGS) != 0) {
				i2cbus->active_mt_result = hi2cErrorBus;
				/* No need to reset error flags, it's already done in i2c_irq_handler(). */
			}
			i2cbus->state = stateStartingMaster; // because START request should still be pending
			i2c_close_active_master_transfer(i2cbus, pxHigherPriorityTaskWoken);
		}
		i2cbus->last_state = i2cbus->state; // after transfer completion we should return exactly(!) to current state
		(void)i2cbus->i2c->DR; // dummy read to clear RxNE flag, it's safe to do here because DR access ignored until ADDR cleared
		/* Instead of immediately starting transfer (by clearing ADDR) we have to implement workaround for hardware bug (see design notes in source header) */
		i2cbus->state = stateSlaveAddressed;
		xTimerStartFromISR(i2cbus->slave_addr_delay_timer, pxHigherPriorityTaskWoken);
		i2cbus->i2c->CR2 &= ~I2C_CR2_ITEVTEN; // otherwise ADDR flag will hold interrupt pending
		break;
	}
	case stateSlaveAddressed:
		/* Nothing to do here. Waiting when slave_addr_delay_timer expires to proceed with following state.
		 * It's a part of workaround for hardware bug (see design notes in source header).
		 * Also don't handle BERR, ARLO, AF errors since they cannot occur in this state.
		 */
		break;
	case stateSlaveReadyToTransfer: {
		/* don't handle BERR, ARLO, AF errors since they cannot occur in this state */
		uint32_t i2c_SR2 = i2cbus->i2c->SR2; // also clears ADDR flag (properly synchronized with reading SR1 in i2c_irq_handler())
		i2cbus->smbushost_transfer_ready = false;
		if ((i2c_SR2 & I2C_SR2_TRA) == 0) {
			SYS_ASSERT((((i2cbus->i2c->OAR1 & I2C_OAR1_ADD1_7) >> POSITION_VAL(I2C_OAR1_ADD1_7)) == I2C_SMBUS_HOST_ADDRESS)
					&& ((i2cbus->i2c->OAR1 & I2C_OAR1_ADDMODE) == 0)
					&& ((i2c_SR2 & (I2C_SR2_DUALF | I2C_SR2_SMBHOST | I2C_SR2_SMBDEFAULT | I2C_SR2_GENCALL)) == 0));
			i2cbus->state = stateSlaveRxSMBusHostAddress;
		} else {
			/* We don't support slave transmitter mode, but it's not possible to abort it. */
			i2cbus->state = stateSlaveTx;
		}
		i2cbus->i2c->CR2 |= I2C_CR2_ITBUFEN;
		break;
	}
	case stateSlaveTx: {
		/* We don't support slave transmitter mode, so we just feeding master until it stops transfer (or error occurs) */
		if ((i2c_SR1 & (I2C_SR1_STOPF | I2C_SR1_AF | I2C_SR1_BUS_ERROR_FLAGS)) != 0) { // BERR flag may also mean Repeated START, but we don't support it anyway
			/* don't clear flags, leave them for proper processing in stopping state */
			i2cbus->state = stateSlaveStopping;
		} else if ((i2c_SR1 & I2C_SR1_TXE) != 0) {
			i2cbus->i2c->DR = 0xFF; // also clears TXE, BTF flags
		}
		break;
	}
	case stateSlaveRxSMBusHostAddress: {
		if (!i2c_process_slave_rx_conditions(i2cbus, i2c_SR1))
			break;
		i2cbus->smbushost_address = i2cbus->i2c->DR & 0xFF;
		i2cbus->state = stateSlaveRxSMBusHostDataLow;
		i2c_SR1 = i2cbus->i2c->SR1; // update for correct RXNE/STOPF fall-through
	}
	/* no break */
	case stateSlaveRxSMBusHostDataLow: {
		if (!i2c_process_slave_rx_conditions(i2cbus, i2c_SR1))
			break;
		i2cbus->smbushost_status = i2cbus->i2c->DR & 0xFF;
		i2cbus->state = stateSlaveRxSMBusHostDataHigh;
		i2c_SR1 = i2cbus->i2c->SR1; // update for correct RXNE/STOPF fall-through
	}
	/* no break */
	case stateSlaveRxSMBusHostDataHigh: {
		if (!i2c_process_slave_rx_conditions(i2cbus, i2c_SR1))
			break;
		i2cbus->smbushost_status |= (i2cbus->i2c->DR << 8) & 0xFF00;
		i2cbus->smbushost_transfer_ready = true;
		i2cbus->state = stateSlaveStopping;
		i2c_SR1 = i2cbus->i2c->SR1; // update for correct RXNE/STOPF fall-through
	}
	/* no break */
	case stateSlaveStopping: {
		if ((i2c_SR1 & I2C_SR1_RXNE) != 0) { // extra(unexpected) bytes received ?
			i2cbus->smbushost_transfer_ready = false; // invalidate transfer (if any)
			(void)i2cbus->i2c->DR; // dummy read to continue transfer and reset BTF
		}
		if ((i2c_SR1 & (I2C_SR1_STOPF | I2C_SR1_AF | I2C_SR1_BUS_ERROR_FLAGS)) == 0)
			break;
		/* Dummy write to complete clearing STOPF (synchronized with reading SR1 in i2c_irq_handler() and previous state).
		 * It's safe to write to CR1 in this state:
		 * - if START bit still holds pending request, then it means, that new slave transfer pending and setting it again should not repeat it;
		 * - STOP bit cannot be set in this state (neither by us, neither by hardware, and at least we already in sync with error conditions);
		 * - PEC isn't used nowhere anyway.
		 */
		if ((i2c_SR1 & I2C_SR1_STOPF) != 0)
			i2cbus->i2c->CR1 = (volatile uint32_t)i2cbus->i2c->CR1;
		/* It's ok to check and clear errors even if we are in master mode now (it may happen if START request was pending)
		 * Errors are guaranteed to relate to finished slave transfer, because they cannot occur after START generated only.
		 */
		if ((i2c_SR1 & I2C_SR1_BUS_ERROR_FLAGS) != 0) {
			i2cbus->smbushost_transfer_ready = false; // invalidate transfer on any error
			i2cbus->i2c->SR1 = ~I2C_SR1_BUS_ERROR_FLAGS;
		}
		if ((i2c_SR1 & I2C_SR1_AF) != 0) // it's not error actually (flag is being left after transmission finished)
			i2cbus->i2c->SR1 = ~I2C_SR1_AF;
		i2cbus->i2c->CR2 &= ~I2C_CR2_ITBUFEN;
		i2cbus->state = i2cbus->last_state; // restore source state
		if (i2cbus->smbus_host_enabled && i2cbus->smbushost_transfer_ready)
			i2cbus->smbus_host_params.isrcallbackMessageReceived((hal_i2c_smbus_handle_t)i2cbus, i2cbus->smbushost_address, i2cbus->smbushost_status, pxHigherPriorityTaskWoken);
		break;
	}
	default:
		break;
	}
}

static void i2c_irq_handler_master(struct s_i2cbus_pcb *i2cbus, uint16_t i2c_SR1, signed portBASE_TYPE *pxHigherPriorityTaskWoken) {
	switch (i2cbus->state) {
	case stateMasterFinishing: { // must be placed before stateIdle and fall-through to it for post-processing
		SYS_ASSERT(i2cbus->active_mt); // aborted transfers cannot reach this state
		if ((i2c_SR1 & I2C_SR1_BUS_ERROR_FLAGS) == 0) {
			if ((i2c_SR1 & I2C_SR1_SB) == 0)
				break; // everything ok, just wait SB flag...
			i2c_reset_master_mode(i2cbus);
		} else {
			i2cbus->active_mt_result = hi2cErrorBus;
			if ((i2c_SR1 & I2C_SR1_SB) != 0)
				i2c_reset_master_mode(i2cbus);
			/* No need to reset error flags, it's already done in i2c_irq_handler(). */
		}
		i2cbus->state = stateIdle;
		i2c_close_active_master_transfer(i2cbus, pxHigherPriorityTaskWoken);
		i2c_SR1 = i2cbus->i2c->SR1; // update for correct fall-through
	}
	/* no break */
	case stateIdle: { // this state is universal entry for processing pending (queued) master transfers (either start next one or drop all of them) and other requests
		if (i2cbus->mode == hi2cModeOff) {
			/* Disable interrupts, flush all pending master transfers as aborted (one per interrupt invocation) and quit. */
			i2cbus->i2c->CR2 &= ~(I2C_CR2_ITEVTEN | I2C_CR2_ITERREN); // otherwise possible requests may hold interrupt pending
			struct hal_i2c_master_transfer_t *next_mt = i2c_pop_from_master_transfer_queue(i2cbus);
			if (next_mt) {
				next_mt->isrcallbackTransferCompleted(next_mt, hi2cErrorAborted, pxHigherPriorityTaskWoken);
				i2c_set_irq_pending(i2cbus); // schedule removing next transfer
				break;
			}
			break;
		}
		/* Since here we start servicing pending master-mode requests */
		if (!(!i2c_master_transfer_queue_is_empty(i2cbus) || i2cbus->smbus_host_change_request))
			break;
		i2c_request_start_condition(i2cbus);
		break;
	}
	case stateStartingMaster: { // this state also used for Repeated START and enabling/disabling smbus host
		/* don't handle BERR, AF, ARLO because they either cannot occur in this state, or already handled in i2c_irq_handler() earlier */
		if ((i2c_SR1 & I2C_SR1_SB) == 0)
			break;
		if (i2cbus->mode == hi2cModeOff) { // it's possible when pending transfer requested master mode and user immediately switched off bus
			i2c_reset_master_mode_to_idle(i2cbus);
			break;
		}
		if (i2cbus->active_mt == 0) { // no master transfer in progress ?
			i2cbus->active_mt = i2c_pop_from_master_transfer_queue(i2cbus); // try(!) to fetch next master transfer
			if (i2cbus->active_mt == 0) {
				i2c_reset_master_mode_to_idle(i2cbus);
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
		/* ACK,POS bits must be cleared before addressing starts:
		 * - it's default value used in following master transfer states;
		 * - slave mode addressing must be disabled in case of uncontrolled switching to slave mode (due to bus errors).
		 */
		i2cbus->i2c->CR1 &= ~(I2C_CR1_ACK | I2C_CR1_POS | I2C_CR1_ENPEC);
		/* Starting addressing */
		i2cbus->i2c->DR = address; // also clears RxNE flag from previous transfer
		i2cbus->state = stateMasterAddressing;
		i2cbus->active_mt_pec = i2c_calc_pec(i2cbus->active_mt_pec, address);
		break;
	}
	case stateMasterAddressing: {
		if ((i2c_SR1 & (I2C_SR1_AF | I2C_SR1_BUS_ERROR_FLAGS)) != 0) {
			// interface may be in slave mode here, but disabled slave mode addressing allows us to miss possible ADDR request (and no STOPF possible)
			(void)i2cbus->i2c->SR2; // dummy read to clear ADDR flag (properly synchronized with reading SR1 in i2c_irq_handler())
			i2c_restore_bus_after_error(i2cbus, i2c_SR1, (I2C_SR1_AF | I2C_SR1_BERR));
			i2cbus->i2c->SR1 = ~(I2C_SR1_AF | I2C_SR1_BUS_ERROR_FLAGS);
			if (i2cbus->active_mt != 0) {
				if ((i2c_SR1 & I2C_SR1_BUS_ERROR_FLAGS) != 0)
					i2cbus->active_mt_result = hi2cErrorBus;
				else
					i2cbus->active_mt_result = hi2cErrorAddressNACK;
				i2c_close_active_master_transfer(i2cbus, pxHigherPriorityTaskWoken);
			}
			break;
		}
		if ((i2c_SR1 & I2C_SR1_ADDR) == 0)
			break;
		if (!((i2cbus->active_mt != 0) && (i2cbus->active_mt->size > 0))) { // zero-sized or aborted transfer ?
			/* Dummy read to clear ADDR flag (properly synchronized with reading SR1 in i2c_irq_handler()).
			 * For zero-size transfer case: transmitter mode ensures no transfer will be started.
			 * Interface may be in slave mode here, but disabled slave mode addressing allows us to miss possible ADDR request (and no STOPF possible).
			 */
			(void)i2cbus->i2c->SR2;
			if (i2cbus->active_mt != 0) {
				i2c_stop_master_transfer(i2cbus);
			} else {
				i2c_reset_master_mode_to_idle(i2cbus);
			}
			break;
		}
		/* Since here we start transfer of data bytes */
		int max_bytes_to_receive = 0; // not used in transmitter mode
		/* Receiver mode: ACK,POS bits must be set (as required) before ADDR cleared */
		if (i2cbus->active_mt->dirs[i2cbus->active_mt_pos] == hi2cDirectionRx) {
			max_bytes_to_receive = i2c_get_master_max_bytes_to_receive(i2cbus);
			if (max_bytes_to_receive > 2)
				i2cbus->i2c->CR1 |= I2C_CR1_ACK;
			if (max_bytes_to_receive == 2)
				i2cbus->i2c->CR1 |= I2C_CR1_POS;
		}
		// dummy read to clear ADDR flag (properly synchronized with reading SR1 in i2c_irq_handler())
		(void)i2cbus->i2c->SR2; // also it starts transfer in receiver mode !
		if (i2cbus->active_mt->dirs[i2cbus->active_mt_pos] == hi2cDirectionRx) {
			/* Receiver mode */
			switch (max_bytes_to_receive) {
			case 1:
				i2cbus->state = stateMasterRxSingleByte;
				i2cbus->i2c->CR2 |= I2C_CR2_ITBUFEN;
				/* don't follow manufacturer recommended sequence to request START/STOP here because it's not safe due to possible ARLO racing */
				break;
			case 2:
				i2cbus->state = stateMasterRxLast2Bytes;
				break;
			case 3:
				i2cbus->state = stateMasterRxFirstN2Bytes;
				break;
			default:
				i2cbus->state = stateMasterRxFirstN2Bytes;
				i2cbus->i2c->CR2 |= I2C_CR2_ITBUFEN;
				break;
			}
		} else {
			/* Transmitter mode */
			i2cbus->state = stateMasterTxStarting;
			i2cbus->i2c->CR2 |= I2C_CR2_ITBUFEN;
		}
		break;
	}
	case stateMasterRxSingleByte: {
		if (i2c_process_master_rx_error_condition(i2cbus, i2c_SR1, pxHigherPriorityTaskWoken))
			break;
		if ((i2c_SR1 & I2C_SR1_RXNE) == 0)
			break;
		if (i2cbus->active_mt == 0) { // aborted transfer ?
			i2c_reset_master_mode_to_idle(i2cbus);
			break;
		}
		if ((i2cbus->active_mt_pos + 1) < i2cbus->active_mt->size) { // not last byte in total transfer ?
			i2c_request_start_condition(i2cbus); // Repeated START
		} else { // last byte in transfer ?
			i2c_stop_master_transfer(i2cbus); // STOP
		}
		/* received byte is data byte (single byte transfers cannot contain PEC) */
		i2c_receive_master_data_byte(i2cbus);
		break;
	}
	case stateMasterRxFirstN2Bytes: {
		if (i2c_process_master_rx_error_condition(i2cbus, i2c_SR1, pxHigherPriorityTaskWoken))
			break;
		if (i2cbus->active_mt == 0) { // aborted transfer ?
			if ((i2c_SR1 & I2C_SR1_BTF) == 0)
				break;
			i2cbus->i2c->CR1 &= ~I2C_CR1_ACK; // sync with BTF (see above) guarantees that we may safely change ACK bit (holding master mode)
			(void)i2cbus->i2c->DR; // dummy read to start receiving next byte with NACK
			i2c_reset_master_mode_to_idle(i2cbus);
			break;
		}
		int max_bytes_to_receive = i2c_get_master_max_bytes_to_receive(i2cbus);
		if (max_bytes_to_receive > 3) {
			if ((i2c_SR1 & I2C_SR1_RXNE) == 0)
				break;
			if (max_bytes_to_receive == 4) // next byte, we will wait for, is N-2 ?
				i2cbus->i2c->CR2 &= ~I2C_CR2_ITBUFEN; // otherwise RxNE flag will hold interrupt pending
		} else {
			if ((i2c_SR1 & I2C_SR1_BTF) == 0)
				break;
			i2cbus->i2c->CR1 &= ~I2C_CR1_ACK; // must be disabled before N-2 byte read
		}
		/* received byte is data byte (first N-2 bytes cannot contain PEC) */
		i2c_receive_master_data_byte(i2cbus);
		if (max_bytes_to_receive > 3) // actually means ">2" if we account just received byte
			break;
		i2cbus->state = stateMasterRxLast2Bytes;
		i2c_SR1 = i2cbus->i2c->SR1; // update for correct fall-through
	}
	/* no break */
	case stateMasterRxLast2Bytes: {
		if (i2c_process_master_rx_error_condition(i2cbus, i2c_SR1, pxHigherPriorityTaskWoken))
			break;
		if ((i2c_SR1 & I2C_SR1_BTF) == 0)
			break;
		if (i2cbus->active_mt == 0) { // aborted transfer ?
			i2c_reset_master_mode_to_idle(i2cbus); // also should clear BTF flag
			break;
		}
		if ((i2cbus->active_mt_pos + 2) < i2cbus->active_mt->size) { // not last bytes in total transfer ?
			i2c_request_start_condition(i2cbus); // Repeated START
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
	case stateMasterTxStarting: { // this extra-state required for special case of aborting
		i2cbus->state = stateMasterTxData;
	}
	/* no break */
	case stateMasterTxData: {
		if (i2c_process_master_tx_error_condition(i2cbus, i2c_SR1, pxHigherPriorityTaskWoken))
			break;
		if (i2cbus->active_mt == 0) { // aborted transfer ?
			if ((i2c_SR1 & I2C_SR1_BTF) == 0)
				break;
			i2c_reset_master_mode_to_idle(i2cbus);
			break;
		}
		if ((i2c_SR1 & I2C_SR1_TXE) == 0)
			break;
		i2c_transmit_master_data_byte(i2cbus);
		if (i2cbus->active_mt_pos < i2cbus->active_mt->size) {
			// transfer direction changed ?
			if (i2cbus->active_mt->dirs[i2cbus->active_mt_pos] != hi2cDirectionTx)
				i2c_request_start_condition(i2cbus); // Repeated START
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
		if (i2cbus->active_mt == 0) { // aborted transfer ?
			if ((i2c_SR1 & I2C_SR1_BTF) == 0)
				break;
			i2c_reset_master_mode_to_idle(i2cbus);
			break;
		}
		if ((i2c_SR1 & I2C_SR1_TXE) == 0)
			break;
		i2cbus->i2c->DR = i2cbus->active_mt_pec;
		i2cbus->i2c->CR2 &= ~I2C_CR2_ITBUFEN; // otherwise TXE flag will hold interrupt pending
		i2cbus->state = stateMasterTxStopping;
		i2c_SR1 = i2cbus->i2c->SR1; // update for correct fall-through
	}
	/* no break */
	case stateMasterTxStopping: {
		if (i2c_process_master_tx_error_condition(i2cbus, i2c_SR1, pxHigherPriorityTaskWoken))
			break;
		if ((i2c_SR1 & I2C_SR1_BTF) == 0)
			break;
		if (i2cbus->active_mt == 0) { // aborted transfer ?
			i2c_reset_master_mode_to_idle(i2cbus);
			break;
		}
		i2c_stop_master_transfer(i2cbus);
		/* Dummy read to clear BTF flag (it's safe to do after entering slave mode, because DR access ignored until ADDR cleared).
		 * We do it explicitly because we don't want to leave interrupt pending until STOP condition finally generates.
		 */
		(void)i2cbus->i2c->DR;
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
	i2c_irq_handler(&i2cbus_pcbs[2]);
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
