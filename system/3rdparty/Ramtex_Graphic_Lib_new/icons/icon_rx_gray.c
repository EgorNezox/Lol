/**
  ******************************************************************************
  * @file     icon_rx.c
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
	SGUCHAR b[12*24];
} GCODE icon_rx_gray[1] =
{
        #include "icon_rx_gray.sym"
};

PGSYMBOL sym_rx_gray = (PGSYMBOL)icon_rx_gray;

//-----------------------------


