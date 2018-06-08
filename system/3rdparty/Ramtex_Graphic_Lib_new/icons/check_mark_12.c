/**
  ******************************************************************************
  * @file     check_mark_12.c
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
	SGUCHAR b[24];
} GCODE check_mark_12[1] =
{
	#include "check_mark_12.sym"
};

PGSYMBOL sym_check_mark_12 = (PGSYMBOL)check_mark_12;

//-----------------------------
