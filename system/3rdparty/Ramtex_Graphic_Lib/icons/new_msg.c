/**
  ******************************************************************************
  * @file     new_msg.c
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
    SGUCHAR b[3*24*24];
} GCODE new_msg[1] =
{
	#include "new_msg.sym"
};

PGSYMBOL sym_new_msg = (PGSYMBOL)new_msg;

//-----------------------------

