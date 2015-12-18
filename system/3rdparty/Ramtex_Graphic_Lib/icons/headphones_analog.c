/**
  ******************************************************************************
  * @file     headphones.c
  * @author  Egor, PMR dept. software team, ONIIP, PJSC
  * @date    10 ���. 2013 �.
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
} GCODE headphones_analog[1] =
{
	#include "headphones_analog.sym"
};

PGSYMBOL sym_headphones_analog = (PGSYMBOL)headphones_analog;

//-----------------------------


