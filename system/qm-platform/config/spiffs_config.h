/**
  ******************************************************************************
  * @file    spiffs_config.h
  * @author  Artem Pisarenko, PMR dept. software team, ONIIP, PJSC
  * @date    16.02.2016
  *
  ******************************************************************************
 */

#ifndef SPIFFS_CONFIG_H_
#define SPIFFS_CONFIG_H_

#include "spiffs_config_platform.h"

// Multiple instances support
#define SPIFFS_SINGLETON				0
// Read/write file systems
#define SPIFFS_READ_ONLY				0
// HAL API required for linking spiffs instances and callbacks contexts
#define SPIFFS_HAL_CALLBACK_EXTRA		1
// Required for getting sizes for internal buffers allocation
#define SPIFFS_BUFFER_HELP				1
// No linking between file handles and file system instances required
#define SPIFFS_FILEHDL_OFFSET			0

/* Make file systems identifiable and verifiable */
#define SPIFFS_USE_MAGIC				1
#define SPIFFS_USE_MAGIC_LENGTH			1

/* File systems "fixed" configuration */
// "Block index type."
typedef u16_t spiffs_block_ix;
// "Page index type."
typedef u16_t spiffs_page_ix;
// "Object id type - most significant bit is reserved for index flag."
typedef u16_t spiffs_obj_id;
// "Object span index type."
typedef u16_t spiffs_span_ix;
// alignment
#define SPIFFS_ALIGNED_OBJECT_INDEX_TABLES	1

/* File systems cache disabled */
#define SPIFFS_CACHE					0

/* Garbage collection */
// "Define maximum number of gc runs to perform to reach desired free pages."
#define SPIFFS_GC_MAX_RUNS				5 // default
// "Garbage collecting heuristics - weight used for deleted pages."
#define SPIFFS_GC_HEUR_W_DELET			(5) // default
// "Garbage collecting heuristics - weight used for used pages."
#define SPIFFS_GC_HEUR_W_USED			(-1) // default
// "Garbage collecting heuristics - weight used for time between
// last erased and erase of this block."
#define SPIFFS_GC_HEUR_W_ERASE_AGE		(50) // default
// Disable statistics collection
#define SPIFFS_GC_STATS					0

/* Disable debugging and testing */
#define SPIFFS_DBG(...)
#define SPIFFS_GC_DBG(...)
#define SPIFFS_CACHE_DBG(...)
#define SPIFFS_CHECK_DBG(...)
#define SPIFFS_TEST_VISUALISATION		0

/* Miscellaneous */
// "Object name maximum length."
#define SPIFFS_OBJ_NAME_LEN				(32) // default
// "Size of buffer allocated on stack used when copying data.
// Lower value generates more read/writes. No meaning having it bigger
// than logical page size."
#define SPIFFS_COPY_BUFFER_STACK		(64) // default
// "Always check header of each accessed page to ensure consistent state."
#define SPIFFS_PAGE_CHECK				1

/* Integration with application system and QmStorage module */
#define SPIFFS_LOCK(fs)		qmstorageSpiffsLock(fs)
#define SPIFFS_UNLOCK(fs)	qmstorageSpiffsUnlock(fs)
struct spiffs_t;
void qmstorageSpiffsLock(struct spiffs_t *fs);
void qmstorageSpiffsUnlock(struct spiffs_t *fs);

#endif /* SPIFFS_CONFIG_H_ */
