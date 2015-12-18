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
} GCODE headphones[1] =
{
	#include "headphones.sym"
};

PGSYMBOL sym_headphones = (PGSYMBOL)headphones;

//-----------------------------


