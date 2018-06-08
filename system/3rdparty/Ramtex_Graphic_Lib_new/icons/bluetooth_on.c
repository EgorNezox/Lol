/**
  ******************************************************************************
  * @file     bluetooth_on.c
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
} GCODE bluetooth_on[1] =
{
	#include "bluetooth_on.sym"
};

PGSYMBOL sym_bltooth_on = (PGSYMBOL)bluetooth_on;


//-----------------------------

