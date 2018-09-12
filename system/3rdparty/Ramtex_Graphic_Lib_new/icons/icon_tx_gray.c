/**
  ******************************************************************************
  * @file     icon_rx.c
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
} GCODE icon_tx_gray[1] =
{
        #include "icon_tx_gray.sym"
};

PGSYMBOL sym_tx_gray = (PGSYMBOL)icon_tx_gray;

//-----------------------------


