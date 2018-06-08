/**
  ******************************************************************************
  * @file     dsc_in.c
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
} GCODE dsc_in[1] =
{
	#include "dsc_in.sym"
};

PGSYMBOL sym_dsc_in = (PGSYMBOL)dsc_in;

//-----------------------------
