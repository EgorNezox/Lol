/*
 * headphones_smart.c
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
} GCODE headphones_smart[1] =
{
	#include "headphones_smart.sym"
};

PGSYMBOL sym_headphones_smart = (PGSYMBOL)headphones_smart;

//-----------------------------


