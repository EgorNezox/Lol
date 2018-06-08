/**
  ******************************************************************************
  * @file     dsc_out.c
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
} GCODE dsc_out[1] =
{
	#include "dsc_out.sym"
};

PGSYMBOL sym_dsc_out = (PGSYMBOL)dsc_out;

//-----------------------------

