/**
  ******************************************************************************
  * @file     icon_rx.c
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
} GCODE icon_rx[1] =
{
	#include "icon_rx.sym"
};

PGSYMBOL sym_rx = (PGSYMBOL)icon_rx;

//-----------------------------


