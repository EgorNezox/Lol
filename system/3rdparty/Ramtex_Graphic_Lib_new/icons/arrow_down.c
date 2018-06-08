/**
  ******************************************************************************
  * @file     arrow_down.c
  * @author  Egor, PMR dept. software team, ONIIP, PJSC
  * @date    14 но€б. 2013 г.
  * @brief   
  *
  ******************************************************************************
  */


//-----------------------------

#include <gdisphw.h>

static struct
{
	GSYMHEAD sh;
	SGUCHAR b[8];
} GCODE arrow_down[1] =
{
	#include "down_arrow.sym"
};

PGSYMBOL sym_arrow_down = (PGSYMBOL)arrow_down;

//-----------------------------

