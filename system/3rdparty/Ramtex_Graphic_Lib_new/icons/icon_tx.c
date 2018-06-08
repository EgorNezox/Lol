/**
  ******************************************************************************
  * @file     icon_tx.c
  * @author  Egor, PMR dept. software team, ONIIP, PJSC
  * @date    10 окт. 2013 г.
  * @brief   
  *
  ******************************************************************************
  */


//-----------------------------

#include <gdisphw.h>

static struct
{
	GCSYMHEAD sh;
	SGUCHAR b[2*24*24];
} GCODE icon_tx[1] =
{
	#include "icon_tx.sym"
};

PGSYMBOL sym_tx = (PGSYMBOL)icon_tx;

//-----------------------------

