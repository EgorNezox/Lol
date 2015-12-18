/**
  ******************************************************************************
  * @file     gps_lock.c
  * @author  Egor, PMR dept. software team, ONIIP, PJSC
  * @date    10 окт. 2013 г.
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
} GCODE GPS_lock[1] =
{
	#include "gps_lock.sym"
};

PGSYMBOL sym_gps = (PGSYMBOL)GPS_lock;

//-----------------------------

