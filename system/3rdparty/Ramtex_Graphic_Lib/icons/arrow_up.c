/**
  ******************************************************************************
  * @file     arrow_up.c
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
} GCODE arrow_up[1] =
{
	#include "up_arrow.sym"
};

PGSYMBOL sym_arrow_up = (PGSYMBOL)arrow_up;

//-----------------------------

