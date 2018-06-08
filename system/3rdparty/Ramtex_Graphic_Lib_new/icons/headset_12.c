/**
  ******************************************************************************
  * @file     headset_12.c
  * @author  Egor, PMR dept. software team, ONIIP, PJSC
  * @date    20 мая 2014 г.
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
} GCODE headset_12[1] =
{
	#include "headset_12.sym"
};

PGSYMBOL sym_headset_12 = (PGSYMBOL)headset_12;

//-----------------------------
