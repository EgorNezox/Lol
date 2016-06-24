//-----------------------------

#include <gdisphw.h>

static struct
{
	GCSYMHEAD sh;
        SGUCHAR b[3*24*24];
} GCODE GPS_lock[1] =
{
        #include "gps_unlock.sym"
};

PGSYMBOL sym_gps_unlock = (PGSYMBOL)GPS_lock;

//-----------------------------

