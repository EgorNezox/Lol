/**
  ******************************************************************************
  * @file     ar_up_12.c
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
} GCODE ar_up_12[1] =
{
	#include "ar_up_12.sym"
};

PGSYMBOL sym_ar_up_12 = (PGSYMBOL)ar_up_12;

//-----------------------------

