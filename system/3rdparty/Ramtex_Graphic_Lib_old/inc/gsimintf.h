/********************************** gsimintf.h ***********************

   Interface functions for socket connections to the general LCD
   WIN32 simulator program.

   These generic LCD simulator functions are usually only activated
   via the simulator C module which emulates the functionality of a
   specific LCD controller chip.

   Revision date:     190407
   Revision Purpose:  Touch screen simulation interface added.

   Version 2
   Copyright RAMTEX International ApS 2001-2007

*********************************************************************/
#ifndef GSIMINTF_H
#define GSIMINTF_H

#ifdef __cplusplus
extern "C" {
#endif

typedef enum   /* Define interpretation mode for graphic buffer data */
   {
   PALETTE_NONE,      /* Accept B/W commands */
   PALETTE_1 = 1,     /* 1 data bit pr color pixel (2 palette colors) */
   PALETTE_2 = 2,     /* 2 data bit pr color pixel (4 palette colors) */
   PALETTE_4 = 4,     /* 4 data bit pr color pixel (16 palette colors) */
   PALETTE_8 = 8,     /* 8 data bit pr color pixel (256 palette colors) */
   PALETTE_RGB = 24   /* 24 data bytes Use RGB bytes (16M colors) */
   } GSIM_PALETTE_MODE;

typedef struct
   {
   unsigned char r;
   unsigned char g;
   unsigned char b;
   } GSIM_PALETTE_RGB;

typedef union
   {
   GSIM_PALETTE_RGB rgb;
   unsigned long par;
   } GSIM_RGB_PARAM;

unsigned char GSimInitC(unsigned short w, unsigned short h, GSIM_PALETTE_MODE pallette);
#define GSimInit(w, h) GSimInitC((w), (h), PALETTE_NONE)
void GSimClose(void);
unsigned char GSimGClr(void);
unsigned char GSimError(void);
unsigned char GSimPutsClr(void);
unsigned char GSimPuts(const char *str);
unsigned char GSimWrBit(unsigned short x, unsigned short y, unsigned char bitval);
unsigned char GSimWrRGBBit(unsigned short x, unsigned short y, GSIM_RGB_PARAM rgbbit);
unsigned char GSimWrPalette(const GSIM_PALETTE_RGB *palette, unsigned short no_elements);
unsigned char GSimFlush(void);
unsigned short GSimKbHit(void);
unsigned short GSimKbGet(void);
unsigned long GSimTimestamp(void);
unsigned char GSimTouchGet(unsigned char *eventp, unsigned char *levelp, unsigned short *xp, unsigned short *yp);

#ifdef __cplusplus
}
#endif

#endif
