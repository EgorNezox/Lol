/*
 * headphones_none.c
 *
 *  Created on: 18 дек. 2015 г.
 *      Author: user
 */


//-----------------------------

#include <gdisphw.h>

static struct
{
	GCSYMHEAD sh;
    SGUCHAR b[3*24*24];
} GCODE headphones_none[1] =
{
	#include "headphones_none.sym"
};

PGSYMBOL sym_headphones_none = (PGSYMBOL)headphones_none;

//-----------------------------
