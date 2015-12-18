/**
  ******************************************************************************
  * @file     paired_mark_12.c
  * @author  Egor, PMR dept. software team, ONIIP, PJSC
  * @date    11 мая 2014 г.
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
} GCODE paired_mark_12[1] =
{
	#include "paired_mark_12.sym"
};

PGSYMBOL sym_paired_mark_12 = (PGSYMBOL)paired_mark_12;

//-----------------------------
