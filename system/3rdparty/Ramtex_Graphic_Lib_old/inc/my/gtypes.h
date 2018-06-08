#ifndef GTYPES_H
#define GTYPES_H

#include "stdint.h"

#define GNORMAL        0x00 /* Normal mode (not inversed not aligned no partial showing) */
#define GINVERSE       0x01 /* Inverse (typical white on black) */
#define GNOSCROLL      0x02 /* Scroll at viewport end is suppressed */
#define GALIGN_LEFT    0x04 /* String is aligned to the left viewport edge */
#define GALIGN_RIGHT   0x08 /* String is aligned to the right viewport edge */
#define GALIGN_HCENTER 0x0c /* String is centered horizontally */
#define GALIGN_TOP     0x10 /* String is aligned to the top */
#define GALIGN_BOTTOM  0x20 /* String is aligned to the bottom */
#define GALIGN_VCENTER 0x30 /* String is centered vertically */

/* Version >= 4 types */
#define GWORD_WRAP     0x40 /* Use word wrap (default is char wrap) */
#define GNO_WRAP       0x80 /* Use no character or word wrapping (only \n processing) */
#define GPARTIAL_LINE 0x100 /* Show lines where symbol height is partly visible  */
#define GPARTIAL_CHAR 0x200 /* Show characters where symbol width can only be partly visible */
#define GVPCLR_UP    0x0400 /* Clear vp area above text line(s) */
#define GVPCLR_DOWN  0x0800 /* Clear vp area below text line(s) */
#define GVPCLR_LEFT  0x1000 /* Clear vp area to the left of text line(s) */ /* default console mode */
#define GVPCLR_RIGHT 0x2000 /* Clear vp area to the right of text line(s) */
#define GTRANSPARENT 0x8000 /* Symbol background color is transperant color */
#define GMODELAST    0xffff /* Used by internal mode limit checks */

/* Compatibility with old source codes */
#define  GLINECUT ( GNO_WRAP | GPARTIAL_CHAR )
#define  GVPCLR   (GVPCLR_UP | GVPCLR_DOWN | GVPCLR_LEFT | GVPCLR_RIGHT)

/* Mode flags for symbol and text rotation modes (overlaids normal mode flags) */
#define GSYMCLR        GVPCLR_UP
#define GNOCOLORBLEND  GVPCLR_DOWN

#define  GLINE     0x00
#define  GFILL     0x03
#define  GFRAME    0x02
#define  GCARC_LT  0x10
#define  GCARC_LB  0x20
#define  GCARC_RT  0x40
#define  GCARC_RB  0x80
#define  GCARC_ALL  (GCARC_LT|GCARC_LB|GCARC_RT|GCARC_RB)

#define GETCHAR(cp) ((uint8_t) (* (cp)))
#define GINCPTR(cp) cp++
#define GETCTRLCHAR(cp) ((uint8_t) (*(cp)))

#define GLIMITU(x,u) \
   { \
   if ((x) > (u)) (x)=(u);\
   }

#define GLIMITD(x,d) \
   { \
   if ((x) < (d)) (x)=(d);\
   }

#define  GPIXEL(x)   ((x) % 8) /* b&w pixel */

#define giscolor(psymbol) ((psymbol)->csh.colorId == 0)

/* Insert default hw character width and height if not allready defined in gdispcfg.h */
#define GDISPCW 8
#define GDISPCH 8

enum CoordMode
{
	CM_PIXEL = 0,
	CM_PERCENT = 1
};

struct GSymHead
{
    uint8_t w;
    uint8_t h;
};

struct GSSymbol
{
   GSymHead sh;  		/* Symbol header */
   uint8_t  b[8];       /* Symbol data, fixed size = 8 bytes */
};

struct GColorSymHead
{
    uint8_t colorId; 	/* Color header id (= B&W_x = 0) */
    uint8_t bitPerPix; 	/* Number of bits pr pixel (= color mode) */
    uint8_t w; 			/* Symbol size in num X pixels */
    uint8_t h;    		/* Symbol size in num Y pixels */
};

union GSymbol /* One table entry */
{
	GSymHead sh;         /* B&W symbol header */
	GColorSymHead csh;   /* Color symbol header */
};

struct GBWSymbol /* One table entry */
{
	GSymHead head;    	 /* Symbol header */
    uint8_t b[3];   	 /* Symbol data, variable length = (cxpix/8+1)*cypix */
};

struct GCSymbol /* One table entry */
{
	GSymHead head;    	 /* Symbol header */
    uint8_t b[3];   	 /* Symbol data, variable length = (cxpix/8+1)*cypix */
};

struct GCodePageRange
{
    uint16_t min;  /* Minimum value included in range */
    uint16_t max;  /* Maximum value included in range */
    uint16_t index;  /* Index in symbol table for the first value */
};

struct GCodePageHeader /* codepage header */
{
    uint16_t codePageRangeNumber;    /* Number of GCP_RANGE elements ( >=1) */
    uint16_t defaultChar;   /* Default character used when not found in codepage */
};

struct GCodePage
{
	GCodePageHeader header;
	GCodePageRange range[1]; /* Dynamic length. Must contain cprnum elements. Minimum is 1 element */
};

struct GFont
{
    uint16_t   symWidth;   /* default width  (= 0 signals extended font structure) */
    uint16_t   symHeight;  /* default height */
    uint16_t   symSize;    /* number of bytes in a symbol */
    GSymbol*   symbols;   /* pointer to array of GSYMBOL's (may be NULL) */
    uint16_t   symCount;
    GCodePage* codePage;  /* pointer to default codepage for font (may be NULL) */
};

static const GSSymbol sysfontsym[128] =
{
    /* The default font MUST be a monospaced black & white (two-color) font */
    #include "../fonts/sfs0129.sym"
};

const GFont SYSFONT = /* Default system font */
{
    6,      /* width */
    8,      /* height */
    sizeof(sysfontsym[0]) - sizeof(GSymHead), /* number of data bytes in a symbol (including any alignment padding)*/
    (GSymbol*) sysfontsym,  /* pointer to array of SYMBOLS */
    0x80,   /* num symbols in sysfontsym[] */
    nullptr    /* pointer to code page */ /* NULL means code page is not used */
};

typedef uint16_t GColor;

struct GPoint
{
    uint16_t x;
    uint16_t y;
};

struct GPointF
{
    float x;
    float y;
};

struct GArea
{
	GPoint b;
	GPoint e;
};

struct GSize
{
    uint16_t w;
    uint16_t h;
};

struct GRgbPalette
{
    uint16_t r;
    uint16_t g;
    uint16_t b;
};

struct GColorSymbol /* One table entry */
{
	GColorSymHead head;     /* Symbol header */
    uint8_t b[3];   /* Symbol data, variable length = (cxpix/8+1)*cypix */
};

typedef uint16_t GMode;

struct GViewPort
{
	GPoint     begin; // leftTop
	GPoint     end;   // rightBottom
	GPoint     symbolPos;    /* Current char pos. in viewport, in pixels, absolute coordinate */
	GPoint     pixelPos;           /* Current pixel pos. in viewport, absolute coordinate */
	GCodePage* codePage;/* pointer to current codepage */
	GFont*     font;        /* pointer to current font */
    GMode      mode;          /* Current graphics mode */
    GColor     frColor;
    GColor     bgColor;
    uint8_t    check;    /* internal var, for GVP struct check */
};

struct StringMarks
{
    const char* begin;
    const char* end;
    const char* next;
    uint16_t    len;
};

#endif // GTYPES_H
