/*
 * headphones_broken.c
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
} GCODE headphones_broken[1] =
{
	#include "headphones_broken.sym"
};

PGSYMBOL sym_headphones_broken = (PGSYMBOL)headphones_broken;

//-----------------------------
