/**
  ******************************************************************************
  * @file     icon_ptt_warning.c
  * @author  Egor, PMR dept. software team, ONIIP, PJSC
  * @date    21 мая 2014 г.
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
} GCODE icon_ptt_warning[1] =
{
	#include "icon_ptt_warning.sym"
};

PGSYMBOL sym_icon_ptt_warning = (PGSYMBOL)icon_ptt_warning;

//-----------------------------

