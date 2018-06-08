/**
  ******************************************************************************
  * @file     arrow_12.c
  * @author  Egor, PMR dept. software team, ONIIP, PJSC
  * @date    19 но€б. 2013 г.
  * @brief   
  *
  ******************************************************************************
  */


//-----------------------------

#include <gdisphw.h>

static struct
{
	GSYMHEAD sh;
	SGUCHAR b[12];
} GCODE arrow_12[1] =
{
	#include "arrow_12.sym"
};

PGSYMBOL sym_arrow_12 = (PGSYMBOL)arrow_12;

//-----------------------------

