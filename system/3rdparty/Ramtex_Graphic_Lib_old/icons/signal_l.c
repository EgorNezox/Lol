/**
  ******************************************************************************
  * @file     signal_l.c
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
} GCODE signal_l[1] =
{
	#include "signal_l.sym"
};

PGSYMBOL sym_signal_l = (PGSYMBOL)signal_l;

//-----------------------------

