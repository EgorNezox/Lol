/**
  ******************************************************************************
  * @file     ar_down_12.c
  * @author  Egor, PMR dept. software team, ONIIP, PJSC
  * @date    22 мая 2014 г.
  * @brief   
  *
  ******************************************************************************
  */


//-----------------------------

#include <gdisphw.h>

static struct
{
	GSYMHEAD sh;
	SGUCHAR b[24];
} GCODE ar_down_12[1] =
{
	#include "ar_down_12.sym"
};

PGSYMBOL sym_ar_down_12 = (PGSYMBOL)ar_down_12;

//-----------------------------

