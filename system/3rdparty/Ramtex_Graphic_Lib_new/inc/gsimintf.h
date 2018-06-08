/******************************* gsimintf.h ***********************

   Interface functions for socket connections to the general LCD
   WIN32 simulator program.

   These generic LCD simulator functions are usually only activated
   via the simulator C module which emulates the functionality of a
   specific LCD controller chip.

   Revision date:     190407
   Revision Purpose:  Touch screen simulation interface added.
   Revision date:     220115
   Revision Purpose:  Extented touch infomation added (with drag speed support)
                      Multi-Touch screen simulation interface added.
                      New status and version commands added.
   Revision date:     22-01-15
   Revision Purpose:  General commands update for LCDserv version 3 and later
                      Multi-Touch screen simulation interface added.
                      New commands:
                         GSimStatus()        Report keyboard and touch states in one operation. No poll wait.
                         GSimVersion()       Report version for LCDSERV.EXE
                         GSimTouchInfo(..)   Fetch extended touch info. Single touch or multi-touch data.
                         GSimTouchSetup(..)  Modify touch screen behaviour and multi-touch emulation keys.
                      New switch GTOUCH_MAX_PRESSED (can be globally set) defines max number of emulated touch
                      positions.

   Revision date:     10-02-16
   Revision Purpose:  Facilitate that PC mode debug info messages are left in the
                      source modules as part of source documentation.
                      When GHW_PCSIM is undefined GSimxxx function calls generate
                      no machine code or maps to neutral constants.
                      Remove / minimize the need for wrapping GSimxxx(..) calls with
                      #ifdef GHW_PCSIM .. #endif in application source modules.

   Version 3.1
   Copyright RAMTEX International ApS 2001-2016
   Web site, support and upgrade: www.ramtex.dk


*******************************************************************/
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

/* GSimStatus() status bits */
#define GSIM_NO_CHANGE     0x0000
#define GSIM_KEY_PRESS     0x0001  /* Bit=1 if PC key board information is pending */
#define GSIM_TOUCH_PRESS   0x0002  /* Bit=1 if simulated touch information is pending */
#define GSIM_NO_CONNECT    0x8000  /* Bit=1 if communication is lost or have not been initiated */

/* Define maximum number of multi-touch position to emulate */
/* (or clean up any illegal GTOUCH_MAX_PRESSED definition (ex globally or in gdispcfg.h) ) */
#ifndef  GTOUCH_MAX_PRESSED
 /* Set default */
 #define GTOUCH_MAX_PRESSED 5  /* Max touch points supported by simulated touch controller (possible values 1-5) */
#else
  /* clean up if illegal definition */
  #if (GTOUCH_MAX_PRESSED < 1)
   #undef  GTOUCH_MAX_PRESSED
   #define GTOUCH_MAX_PRESSED 1
  #elif (GTOUCH_MAX_PRESSED > 5)
   #undef  GTOUCH_MAX_PRESSED
   #define GTOUCH_MAX_PRESSED 5
  #endif
#endif

/* Extended touch key state information structure */
typedef struct
   {
   unsigned char  change; /* True if edge or position change detected */
   unsigned char  id;     /* Touch identifier (identify/track individual touch events when multi touch is used) */
   unsigned char  edge;   /* True if touch down or touch up edge detected */
   unsigned char  press;  /* True if touch is active (pressed down) */
   unsigned short x;      /* Touch x value. Range {0, GDISPW-1} */
   unsigned short y;      /* Touch y value. Range {0, GDISPH-1} */
   unsigned long  t;      /* Time stamp for current touch point change (10 ms resolution). */
   unsigned short x1;     /* X position at time of t1. when t>t1 x drag "speed" is (x-x1)/(t-t1) */
   unsigned short y1;     /* Y position at time of t1. when t>t1 y drag "speed" is (y-y1)/(t-t1) */
   unsigned long  t1;     /* Time stamp for touch down start. Touch event duration time = (t-t1) */
   } GSIM_TOUCH_INFO;


#ifdef GHW_PCSIM

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
unsigned char GSimTouchGet(unsigned char *eventp, unsigned char *pressp, unsigned short *xp, unsigned short *yp);

/*** The following features require use of LCDserv.exe version 3 or later ***/
unsigned char GSimSync(void);
unsigned short GSimVersion(void);
unsigned short GSimStatus(void);

unsigned char GSimTouchInfo(GSIM_TOUCH_INFO *touch_info, unsigned char num_multi_touch_keys, unsigned char *num_touch );
void GSimTouchSetup(unsigned short touch_mode, unsigned char num_hold_keys, unsigned short *holdkey_scan_codes );

#else  /* GHW_PCSIM */

#define  GSimInitC(w, h, pallette) { /* Nothing */ }
#define  GSimInit(w, h) { /* Nothing */ }
#define  GSimClose()    { /* Nothing */ }
#define  GSimGClr()     { /* Nothing */ }
#define  GSimError() 0
#define  GSimPutsClr()  { /* Nothing */ }
#define  GSimPuts(str)  { /* Nothing */ }
#define  GSimWrBit(x, y, bitval) { /* Nothing */ }
#define  GSimWrRGBBit(x, y, rgbbit) { /* Nothing */ }
#define  GSimWrPalette(palette, no_elements) { /* Nothing */ }
#define  GSimFlush() { /* Nothing */ }
#define  GSimKbHit() 0
#define  GSimKbGet() 0
#define  GSimTimestamp() 0
#define  GSimTouchGet(eventp, pressp, xp, yp) 0
#define  GSimSync() 0
#define  GSimVersion() 0
#define  GSimStatus() 0
#define  GSimTouchInfo(touch_info, num_multi_touch_keys, num_touch ) 0
#define  GSimTouchSetup(touch_mode, num_hold_keys, holdkey_scan_codes ) { /* Nothing */ }

#endif  /* GHW_PCSIM */

#ifdef __cplusplus
}
#endif

#endif
