/**
  ******************************************************************************
  * @file     sand_clock.c
  * @author  user, PMR dept. software team, ONIIP, PJSC
  * @date    08 окт. 2014 г.
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
} GCODE sand_clock[1] =
{
	#include "sand_clock.sym"
};

PGSYMBOL sym_sand_clock = (PGSYMBOL)sand_clock;

//-----------------------------

