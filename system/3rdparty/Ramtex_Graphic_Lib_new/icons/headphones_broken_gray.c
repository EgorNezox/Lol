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
    SGUCHAR b[12*24];
} GCODE headphones_broken_gray[1] =
{
	#include "headphones_broken_gray.sym"
};

PGSYMBOL sym_headphones_broken_gray  = (PGSYMBOL)headphones_broken_gray ;

//-----------------------------
