/**
  ******************************************************************************
  * @file     sygnal_h.c
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
} GCODE signal_h[1] =
{
	#include "signal_h.sym"
};

PGSYMBOL sym_signal_h = (PGSYMBOL)signal_h;

//-----------------------------


