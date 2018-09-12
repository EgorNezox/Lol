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
    SGUCHAR b[12*24];
} GCODE headphones_analog_gray[1] =
{
	#include "headphones_analog_gray.sym"
};

PGSYMBOL sym_headphones_analog_gray = (PGSYMBOL)headphones_analog_gray;

//-----------------------------


