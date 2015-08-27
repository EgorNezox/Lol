/**
  ******************************************************************************
  * @file    qmdebug.h
  * @author  Artem Pisarenko, PMR dept. software team, ONIIP, PJSC
  * @date    11.09.2015
  *
  * This header provides following debug features:
  * - software breakpoints, macro QM_DEBUG_BREAK
  * - expression assertions (disabled in release builds), macro QM_ASSERT(<expression>)
  * - domain-separated and level-controlled printf-like message output (requires user to define domains),
  *   macros qmDebugMessage(type, format, ...) and qmDebugIsVerbose()
  *
  * Debug domains definitions are placed in special blocks located in source files.
  * Block wrapped with special includes and contains newline separated macros with arguments,
  * consisting of domain name and its default level.
  * Example:
  * #include "qmdebug_domains_start.h"
  * QMDEBUG_DEFINE_DOMAIN(MyDomain1, LevelDefault) // defines domain with name "MyDomain1" and default level
  * QMDEBUG_DEFINE_DOMAIN(MyDomain2, LevelVerbose) // defines domain with name "MyDomain2" and default level set to verbose
  * #include "qmdebug_domains_end.h"
  *
  * Domain assignment is done by defining QMDEBUGDOMAIN macro with one of defined domain name value
  * just before including this header in source file.
  * After such include following macros are accessible:
  * - qmDebugIsVerbose(): returns true if current level of domain is set to LevelVerbose
  * - qmDebugMessage(type, format, ...): prints new-line ended message of type (msg_type_t) with <format> string
  *   and optional variable arguments according to standard printf() function specification.
  *
  * Domain levels may be reconfigured at runtime by changing values of global variables qmdebug_domain_<name>.
  *
  ******************************************************************************
  */

#ifndef QMDEBUG_H_
#define QMDEBUG_H_

#if defined(__i386__) || defined(__x86_64__)
#define QM_DEBUG_BREAK __asm volatile("int $0x03")
#elif defined(__arm__)
#define QM_DEBUG_BREAK __asm volatile("bkpt")
#else
#error "Unsupported processor architecture"
#endif

#ifndef NDEBUG
#define QM_ASSERT(e)	do { if (!(e)) QM_DEBUG_BREAK; } while (0)
#else /* NDEBUG */
#define QM_ASSERT(e)	((void)(e))
#endif /* NDEBUG */


#define QMDEBUG_XTOKENPASTE(x, y) QMDEBUG_TOKENPASTE(x, y)
#define QMDEBUG_TOKENPASTE(x, y) x ## y
#define QMDEBUG_XSTRINGIFY(s) QMDEBUG_STRINGIFY(s)
#define QMDEBUG_STRINGIFY(s) #s

namespace QmDebug {

/*! Definition of output levels for each debug domain */
typedef enum {
	LevelOff = 0,		/*!< disables all output */
	LevelDefault = 10,	/*!< only error and warning messages output enabled */
	LevelInfo = 11,		/*!< default + info messages output enabled */
	LevelVerbose = 12	/*!< all messages output enabled */
} level_t;

/*! Definitions of message types */
typedef enum {
	Warning = 1,
	Error = 2,
	Info = 11,
	Dump = 12
} msg_type_t;

/* --- use macro qmDebugMessage() instead --- */
void message(const char * domain_name, msg_type_t type, const char * format, ...) __attribute__ ((format (printf, 3, 4)));

}

#endif /* QMDEBUG_H_ */

#ifdef QMDEBUGDOMAIN
#define QMDEBUG_DOMAIN_LEVEL_VAR QMDEBUG_XTOKENPASTE(qmdebug_domain_, QMDEBUGDOMAIN)
extern QmDebug::level_t QMDEBUG_DOMAIN_LEVEL_VAR;
#define qmDebugIsVerbose()	(QMDEBUG_DOMAIN_LEVEL_VAR >= QmDebug::LevelVerbose)
#define qmDebugMessage(type, format, ...) \
	do { \
		if ((QmDebug::level_t)(type) <= QMDEBUG_DOMAIN_LEVEL_VAR) \
			QmDebug::message(QMDEBUG_XSTRINGIFY(QMDEBUGDOMAIN), type, format, ##__VA_ARGS__); \
	} while (0)
#endif /* QMDEBUGDOMAIN */
