/**
  ******************************************************************************
  * @file    cpp_exceptions.cpp
  * @author  Artem Pisarenko, PMR dept. software team, ONIIP, PJSC
  * @date    17.05.2016
  *
  ******************************************************************************
 */

#include <exception>
#include "sys_internal.h"

void __attribute__((constructor)) hal_system_set_cpp_exceptions_handler() {
	std::set_terminate(__gnu_cxx::__verbose_terminate_handler);
}

namespace __gnu_cxx {
	void __verbose_terminate_handler() {
		halinternal_system_fault_handler();
	}
}

extern "C" void __cxa_pure_virtual(void) {
	halinternal_system_fault_handler();
}
