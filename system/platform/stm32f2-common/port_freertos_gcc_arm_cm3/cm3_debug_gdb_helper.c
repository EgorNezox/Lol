/*
 * cm3_debug_gdb_helper.c
 *
 *  Created on: 17.05.2013
 *      Author: Artem Pisarenko
 *
 * Helper file for ARM Cortex-M3 port of FreeRTOS.
 * Used in conjunction with .gdbinit-FreeRTOS-helpers script.
 *
 * Add following replacement macro in FreeRTOSConfig.h:
 * 	#define xPortPendSVHandler	xPortPendSVHandler_native // any valid c identifier name
 *
 * Replaced xPortPendSVHandler handles command invocations from gdb via dbgXXX variables
 * and performs context switch.
 */

#include "FreeRTOS.h"

#ifdef xPortPendSVHandler

#define DBG_STRINGIFY_THIS_(arg)	#arg
#define DBG_STRINGIFY_THIS(arg)		DBG_STRINGIFY_THIS_(arg)

/* Switch control variable:
 * 0 - no hook installed (normal execution),
 * 1 - hook installation performed,
 * 2 - following hooked switches
 */
static int volatile dbgPendSVHookState = 0;
/* Requested target task handle variable */
static void * volatile dbgPendingTaskHandle;

static const int volatile dbgFreeRTOSConfig_suspend_value = INCLUDE_vTaskSuspend;
static const int volatile dbgFreeRTOSConfig_delete_value = INCLUDE_vTaskDelete;

static void __attribute__ ((naked, used)) xPortPendSVHandler_jumper( void ) {
	__asm volatile("b " DBG_STRINGIFY_THIS(xPortPendSVHandler));
}
#undef xPortPendSVHandler
void __attribute__ ((naked)) xPortPendSVHandler( void ) {
	/* Double-right-shifted (tabbed) lines are those from native (port provided) xPortPendSVHandler */
	__asm volatile
	(
		"	ldr r1, _dbgPendSVHookState		\n" /* Check hook installed */
		"	ldr r0, [r1]						\n"
		"	cmp r0, #0							\n"
		"	beq xPortPendSVHandler_jumper		\n" /* if no hook instaled then jump to native handler, else proceed... */
		"	cmp r0, #1							\n" /* check whether hook triggered for the first time...  */
		"	bne dbg_switch_to_pending_task		\n" /* if not so, then jump to switching right now, otherwise current task context must be saved first...  */
		"	mov r0, #2							\n" /* mark hook after triggered for the first time */
		"	str r0, [r1]						\n"
				"	mrs r0, psp							\n"
				"	isb									\n"
				"										\n"
				"	ldr	r3, pxCurrentTCBConst			\n" /* Get the location of the current TCB. */
				"	ldr	r2, [r3]						\n"
				"										\n"
				"	stmdb r0!, {r4-r11}					\n" /* Save the remaining registers. */
				"	str r0, [r2]						\n" /* Save the new top of stack into the first member of the TCB. */
				"										\n"
#if 0
				"	stmdb sp!, {r3, r14}				\n"
				"	mov r0, %0							\n"
				"	msr basepri, r0						\n"
				"	bl vTaskSwitchContext				\n"
				"	mov r0, #0							\n"
				"	msr basepri, r0						\n"
				"	ldmia sp!, {r3, r14}				\n"
#else
		"dbg_switch_to_pending_task:		\n"
		"	ldr r3, _dbgPendingTaskHandle	\n" /* --> Load task handle going to switch to <-- */
#endif
				"										\n"	/* Restore the context. */
				"	ldr r1, [r3]						\n"
				"	ldr r0, [r1]						\n" /* The first item in the TCB is the task top of stack. */
				"	ldmia r0!, {r4-r11}					\n" /* Pop the registers. */
				"	msr psp, r0							\n"
				"	isb									\n"
#if 1
		"	bkpt								\n" /* <-- here debugger stops and steps out to target task context */
#endif
				"	bx r14								\n"
				"										\n"
				"	.align 2							\n"
				"pxCurrentTCBConst: .word pxCurrentTCB	\n"
		"_dbgPendSVHookState: .word dbgPendSVHookState		\n"
		"_dbgPendingTaskHandle: .word dbgPendingTaskHandle	\n"
		".word dbgFreeRTOSConfig_suspend_value						\n" /* force keep these symbols from cutting away by garbage collector */
		".word dbgFreeRTOSConfig_delete_value						\n"
				::"i"(configMAX_SYSCALL_INTERRUPT_PRIORITY)
	);
}

#endif /* xPortPendSVHandler */
