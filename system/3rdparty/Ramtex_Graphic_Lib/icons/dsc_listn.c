/**
  ******************************************************************************
  * @file     dsc_listn.c
  * @author  user, PMR dept. software team, ONIIP, PJSC
  * @date    28 ���. 2015 �.
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
} GCODE dsc_listn[1] =
{
	#include "dsc_listn.sym"
};

PGSYMBOL sym_dsc_listn = (PGSYMBOL)dsc_listn;

//-----------------------------
