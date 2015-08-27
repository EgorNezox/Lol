/**
  ******************************************************************************
  * @file    qmdebug_domains_start.h
  * @author  Artem Pisarenko, PMR dept. software team, ONIIP, PJSC
  * @date    11.09.2015
  *
  * Include this header before domains definitions block.
  * (See how to define domain definitions in qmdebug.h description.)
  *
  ******************************************************************************
  */

#include "qmdebug.h"
#define QMDEBUG_DEFINE_DOMAIN(name, level)	QmDebug::level_t qmdebug_domain_ ## name = QmDebug::level;
