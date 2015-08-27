/**
  ******************************************************************************
  * @file    syscalls.c
  * @author  Artem Pisarenko, PMR dept. software team, ONIIP, PJSC
  * @date    11.06.2013
  * @brief   newlib syscalls stubs (ARM CM3, CMSIS, FreeRTOS, GCC ARM Embedded toolchain)
  *
  * Заглушки системных вызовов обеспечивают интеграцию стандартной библиотеки C newlib и FreeRTOS.
  * Версия FreeRTOS должна быть не ниже V7.5.0 !
  * Реализация поддержки stdio:
  *   Стандартный вывод (stdout) направляется в отладочный вывод SWO на ITM-канал 0.
  *   Каждая задача должна самостоятельно включать для себя небуферизованный вывод
  *   путем вызова setvbuf(stdout, NULL, _IONBF, 0).
  * Реализация динамической памяти (malloc/free):
  *   Вместо стандартного memory allocator используется динамическая память FreeRTOS, т.е. обертка.
  *   Размер кучи задается в конфигурации FreeRTOS параметром configTOTAL_HEAP_SIZE (в байтах).
  *   Поддержка динамической памяти введена для внутренних нужд newlib и поддержки runtime-инициализации данных приложения.
  *   (Не рекомендуется к динамическому использованию приложением.)
  *
  ******************************************************************************
 */

#include "FreeRTOS.h"
#include "system_stm32f2xx.h"
#include "cm3_itm_debug.h"

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>

#if ( configUSE_NEWLIB_REENTRANT != 1 )
#error "FreeRTOSConfig.h must define configUSE_NEWLIB_REENTRANT enabled, check version also"
#endif

/* FreeRTOS heap_4.c */
void *_malloc_r(struct _reent *r, size_t size) {
	(void)r;
	return pvPortMalloc(size);
}
void *_calloc_r(struct _reent *r, size_t num, size_t size) {
	(void)r;
	void *ptr = pvPortMalloc(num*size);
	if (ptr) memset(ptr, 0, num*size);
	return ptr;
}
void *_realloc_r(struct _reent *r, void* ptr, size_t size) {
	// не поддерживается
	(void)r;
	(void)ptr;
	(void)size;
	__asm volatile("bkpt");
	errno = ENOMEM;
	return NULL;
}
void _free_r(struct _reent *r, void *ptr) {
	(void)r;
	vPortFree(ptr);
}

int _write_r (struct _reent *r, int file, char * ptr, int len)
{
	(void)r;
	(void)file;
	for (int i = 0; i < len; i++) CM3_ITM_SendByte(0, ptr[i]);
	return len;
}


/***************************************************************************/

int _read_r (struct _reent *r, int file, char * ptr, int len)
{
	(void)r;
	(void)file;
	(void)ptr;
	(void)len;
	errno = EINVAL;
	return -1;
}

int _lseek_r (struct _reent *r, int file, int ptr, int dir)
{
	(void)r;
	(void)file;
	(void)ptr;
	(void)dir;
	return 0;
}

int _close_r (struct _reent *r, int file)
{
	(void)r;
	(void)file;
	return 0;
}

caddr_t _sbrk_r (struct _reent *r, int incr)
{
	(void)r;
	(void)incr;
	__asm volatile("bkpt");
	errno = ENOMEM;
	return (caddr_t) -1;
}

int _fstat_r (struct _reent *r, int file, struct stat * st)
{
	(void)r;
	(void)file;
	(void)st;
	errno = EACCES;
	return -1;
}

int _isatty_r(struct _reent *r, int file)
{
	(void)r;
	(void)file;
	return 1;
}

int _getpid_r (struct _reent *r)
{
	(void)r;
	return 1;
}

int _kill_r (struct _reent *r, int pid, int sig)
{
	(void)r;
	(void)pid;
	(void)sig;
	errno = EINVAL;
	return -1;
}
