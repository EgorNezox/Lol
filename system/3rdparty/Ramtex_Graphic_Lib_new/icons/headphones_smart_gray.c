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
    SGUCHAR b[12*24];
} GCODE headphones_smart_gray [1] =
{
	#include "headphones_smart_gray.sym"
};

PGSYMBOL sym_headphones_smart_gray  = (PGSYMBOL)headphones_smart_gray ;

//-----------------------------


