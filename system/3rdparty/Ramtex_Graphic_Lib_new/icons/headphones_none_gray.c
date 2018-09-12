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
    SGUCHAR b[12*24];
} GCODE headphones_none_gray[1] =
{
	#include "headphones_none_gray.sym"
};

PGSYMBOL sym_headphones_none_gray  = (PGSYMBOL)headphones_none_gray ;

//-----------------------------
