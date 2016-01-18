/*
 * icon_tx_tune.c
 *
 *  Created on: 18 янв. 2016 г.
 *      Author: Egor Dudyak
 */


//-----------------------------

#include <gdisphw.h>

static struct
{
	GCSYMHEAD sh;
	SGUCHAR b[2*24*24];
} GCODE icon_tx_tune[1] =
{
	#include "icon_tx_tune.sym"
};

PGSYMBOL sym_tx_tune = (PGSYMBOL)icon_tx_tune;

//-----------------------------
