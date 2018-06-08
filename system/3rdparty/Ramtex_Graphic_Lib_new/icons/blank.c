/**
  ******************************************************************************
  * @file     blank.c
  * @author  Egor, PMR dept. software team, ONIIP, PJSC
  * @date    11 ���. 2013 �.
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
} GCODE blank[1] =
{
	#include "blank.sym"
};

PGSYMBOL sym_blank = (PGSYMBOL)blank;

//-----------------------------
