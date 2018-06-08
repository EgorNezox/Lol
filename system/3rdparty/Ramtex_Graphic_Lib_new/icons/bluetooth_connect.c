/**
  ******************************************************************************
  * @file     bluetooth_connect.c
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
} GCODE bluetooth_connect[1] =
{
	#include "bluetooth_connect.sym"
};

PGSYMBOL sym_bltooth_cnct = (PGSYMBOL)bluetooth_connect;

//-----------------------------


