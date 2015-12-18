/**
  ******************************************************************************
  * @file     signal_m.c
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
} GCODE signal_m[1] =
{
	#include "signal_m.sym"
};

PGSYMBOL sym_signal_m = (PGSYMBOL)signal_m;

//-----------------------------

