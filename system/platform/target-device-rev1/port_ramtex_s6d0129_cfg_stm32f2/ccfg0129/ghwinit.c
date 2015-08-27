/************************** ghwinit.c *****************************

   Low-level driver functions for the SEPS525 LCD display controller
   initialization and error handling.

   The SEPS525 controller is assumed to be used with a LCD module.

   The following LCD module characteristics MUST be correctly
   defined in GDISPCFG.H:

      GDISPW  Display width in pixels
      GDISPH  Display height in pixels
      GBUFFER If defined most of the functions operates on
              a memory buffer instead of the LCD hardware.
              The memory buffer content is copied to the LCD
              display with ghw_updatehw().
              (Equal to an implementation of delayed write)


   Revision date:    07-05-09
   Revision Purpose: The symbol software palette (data and functions) can
                     be optimized away if not used by defining
                     GHW_PALETTE_SIZE as 0 in gdispcfg.h
   Revision date:    11-11-10
   Revision Purpose: ghw_rgb_to_color(..) updated to use G_RGB_TO_COLOR(r,g,b) macro.
   Revision date:
   Revision Purpose:

   Version number: 1.4
   Copyright (c) RAMTEX Engineering Aps 2007-2010

*********************************************************************/

#include <gdisphw.h>  /* HW driver prototypes and types */
#include <s6d0129.h>  /* Controller specific definements */

/* Pixel width, height of internal video memory */
#if defined( GHW_SEPS525 )
  #define  GCTRLW 160
  #define  GCTRLH 128

  #define GCTRL_STATUS                   0x01
  #define GCTRL_OSC_CTL                  0x02
  #define GCTRL_CLOCK_DIV                0x03
  #define GCTRL_REDUCE_CURRENT           0x04
  #define GCTRL_SOFT_RST                 0x05
  #define GCTRL_DISP_ON_OFF              0x06
  #define GCTRL_PRECHARGE_TIME_R         0x08
  #define GCTRL_PRECHARGE_TIME_G         0x09
  #define GCTRL_PRECHARGE_TIME_B         0x0a
  #define GCTRL_PRECHARGE_CURRENT_R      0x0b
  #define GCTRL_PRECHARGE_CURRENT_G      0x0c
  #define GCTRL_PRECHARGE_CURRENT_B      0x0d
  #define GCTRL_DRIVING_CURRENT_R        0x10
  #define GCTRL_DRIVING_CURRENT_G        0x11
  #define GCTRL_DRIVING_CURRENT_B        0x12
  #define GCTRL_DISPLAY_MODE_SET         0x13
  #define GCTRL_RGB_IF                   0x14
  #define GCTRL_RGB_POL                  0x15
  #define GCTRL_MEMORY_WRITE_MODE        0x16
  #define GCTRL_MX1_ADDR                 0x17
  #define GCTRL_MX2_ADDR                 0x18
  #define GCTRL_MY1_ADDR                 0x19
  #define GCTRL_MY2_ADDR                 0x1a
  #define GCTRL_MEMORY_ACCESS_POINTER_X  0x20
  #define GCTRL_MEMORY_ACCESS_POINTER_Y  0x21
  #define GCTRL_RAMWR                    0x22
  #define GCTRL_RAMRD                    0x22
  /*#define GCTRL_DDRAM_DATA_ACCESS_PORT   0x22 */
  #define GCTRL_GRAY_SCALE_TABLE_INDEX   0x50
  #define GCTRL_GRAY_SCALE_TABLE_DATA    0x51
  #define GCTRL_DUTY                     0x28
  #define GCTRL_DSL                      0x29
  #define GCTRL_D1_DDRAM_FAC             0x2e
  #define GCTRL_D1_DDRAM_SAC             0x2f
  #define GCTRL_D2_DDRAM_FAC             0x31
  #define GCTRL_D2_DDRAM_SAC             0x32
  #define GCTRL_SCR1_FX1                 0x33
  #define GCTRL_SCR1_FX2                 0x34
  #define GCTRL_SCR1_FY1                 0x35
  #define GCTRL_SCR1_FY2                 0x36
  #define GCTRL_SCR2_FX1                 0x37
  #define GCTRL_SCR2_FX2                 0x38
  #define GCTRL_SCR2_FY1                 0x39
  #define GCTRL_SCR2_FY2                 0x3a
  #define GCTRL_SCREEN_SAVER_CONTROL     0x3b
  #define GCTRL_SS_SLEEP_TIMER           0x3c
  #define GCTRL_SCREEN_SAVER_MODE        0x3d
  #define GCTRL_SS_SCR1_FU               0x3e
  #define GCTRL_SS_SCR1_MXY              0x3f
  #define GCTRL_SS_SCR2_FU               0x40
  #define GCTRL_SS_SCR2_MXY              0x41
  #define GCTRL_MOVING_DIRECTION         0x42
  #define GCTRL_SS_SCR2_SX1              0x47
  #define GCTRL_SS_SCR2_SX2              0x48
  #define GCTRL_SS_SCR2_SY1              0x49
  #define GCTRL_SS_SCR2_SY2              0x4a

#else
  #error (Missing or illegal controller type definition in gdispcfg.h)
#endif

#if ((GDISPPIXW != 8) && (GDISPPIXW != 16) && (GDISPPIXW != 24))
  #error Wrong GDISPPIXW configuration. The pixel bit width is not supported.
#endif

#ifdef GHW_BUS16
/* Define amount of shifting needed to align cmd to hdw bus */
#define CMDSHIFT  8
#endif

/* Check display size settings */
#ifdef GHW_ROTATED
  #if ((GDISPH > GCTRLW) || (GDISPW > GCTRLH))
    #error (GDISPW, GDISPH, GHW_ROTATED configuration exceeds controller limits)
  #endif
#else
  #if ((GDISPW > GCTRLW) || (GDISPH > GCTRLH))
    #error (GDISPW, GDISPH, GHW_ROTATED configuration exceeds controller limits)
  #endif
#endif


/********************* Chip access definitions *********************/

#ifndef GHW_NOHDW
   #if defined( GHW_SINGLE_CHIP)
      /* User defined access types and simulated register address def */
      #include <bussim.h>
      #ifdef GHW_BUS8
        #define  sgwrby(a,d) simwrby((a),(d))
        #define  sgrdby(a)   simrdby((a))
      #else
        #define  sgwrwo(a,d) simwrwo((a),(d))
        #define  sgrdwo(a)   simrdwo((a))
      #endif
   #else
      /* Portable I/O functions + hardware port def */
      #include <sgio.h>
   #endif
#else
   #undef GHW_SINGLE_CHIP /* Ignore single chip mode */
#endif

/* Fix missing definitions in gdispcfg.h */
#ifndef GHW_XOFFSET
   #define GHW_XOFFSET 0
#endif
#ifndef GHW_YOFFSET
   #define GHW_YOFFSET 0
#endif

/***********************************************************************/
/** All static LCD driver data is located here in this ghwinit module **/
/***********************************************************************/

#ifdef GBASIC_INIT_ERR

/* Active foreground and background color */
GCOLOR ghw_def_foreground;
GCOLOR ghw_def_background;

#if (GHW_PALETTE_SIZE > 0)
/* Default grey scale palette
   The palette file can be edited directly with the ColorIconEdit program
*/
static GCODE GPALETTE_RGB FCODE ghw_palette[16] =
     #include <gcolor_4.pal>
     ;

/* Operative palette (current palette used for color lookup) */
GCOLOR ghw_palette_opr[16];
#endif

/* Use software font */
static struct
   {
   GSYMHEAD sh;        /* Symbol header */
   SGUCHAR  b[8];           /* Symbol data, fixed size = 8 bytes */
   }
GCODE FCODE sysfontsym[0x80] =
   {
   /* The default font MUST be a monospaced black & white (two-color) font */
   #include <sfs0129.sym> /* System font symbol table */
   };

/* Default system font */
GCODE GFONT FCODE SYSFONT =
   {
   6,      /* width */
   8,      /* height */
   sizeof(sysfontsym[0])-sizeof(GSYMHEAD), /* number of data bytes in a symbol (including any alignment padding)*/
   (PGSYMBOL) sysfontsym,  /* pointer to array of SYMBOLS */
   0x80,   /* num symbols in sysfontsym[] */
   NULL    /* pointer to code page */ /* NULL means code page is not used */
   };

#ifdef GBUFFER
   #ifdef GHW_ALLOCATE_BUF
      /* <stdlib.h> is included via gdisphw.h */
      GCOLOR *gbuf = NULL;           /* Graphic buffer pointer */
      static SGBOOL gbuf_owner = 0;   /* Identify pointer ownership */
   #else
      GCOLOR gbuf[GBUFSIZE];         /* Graphic buffer */
   #endif
   GXT GFAST iltx,irbx;     /* "Dirty area" speed optimizers in buffered mode */
   GYT GFAST ilty,irby;
   SGBOOL  ghw_upddelay;    /* Flag for delayed update */
#else
   GCOLOR ghw_tmpbuf[GDISPW]; /* Row line buffer (for block read-modify-write) */
#endif /* GBUFFER */


#ifdef GHW_INTERNAL_CONTRAST
static SGUCHAR ghw_contrast;/* Current contrast value */
#endif

SGBOOL glcd_err;            /* Internal error */
#ifndef GNOCURSOR
GCURSOR ghw_cursor;         /* Current cursor state */
#endif

#ifdef GHW_PCSIM
/* PC simulator declaration */
void ghw_init_sim( SGUINT dispw, SGUINT disph );
void ghw_exit_sim(void);
void ghw_set_xyrange_sim(GXT xb, GYT yb, GXT xe, GYT ye);
void ghw_set_xy_sim(GXT xb, GYT yb);
void ghw_autowr_sim( GCOLOR cval );
GCOLOR ghw_autord_sim( void );
void ghw_dispon_sim( void );
void ghw_dispoff_sim( void );
#endif
/**********************************************************************/
/** Low level SEPS525 interface functions used only by ghw_xxx modules **/
/**********************************************************************/

/* Bit mask values */
GCODE SGUCHAR FCODE sympixmsk[8] = {0x80,0x40,0x20,0x10,0x08,0x04,0x02,0x01};

/*
   Set the y range.
   The row position is set to y.
   After last write on row y2 the write position is reset to y
   Internal ghw function
*/
void ghw_set_xyrange(GXT xb, GYT yb, GXT xe, GYT ye)
   {
   #ifdef GHW_PCSIM
   ghw_set_xyrange_sim( xb, yb, xe, ye);
   #endif

   #ifndef GHW_NOHDW

   #ifdef GHW_ROTATED
    /* Rotated mode (swap x,y) */
    #if defined(GHW_BUS8)
    /* 8 bit data bus */
    sgwrby(GHWCMD,GCTRL_MY1_ADDR);
    sgwrby(GHWWR,(xb+GHW_XOFFSET));
    sgwrby(GHWCMD,GCTRL_MY2_ADDR);
    sgwrby(GHWWR,(xe+GHW_XOFFSET));
    sgwrby(GHWCMD,GCTRL_MX1_ADDR);
    sgwrby(GHWWR,(yb+GHW_YOFFSET));
    sgwrby(GHWCMD,GCTRL_MX2_ADDR);
    sgwrby(GHWWR,(ye+GHW_YOFFSET));

    sgwrby(GHWCMD,GCTRL_MEMORY_ACCESS_POINTER_X);
    sgwrby(GHWWR,(yb+GHW_YOFFSET));
    sgwrby(GHWCMD,GCTRL_MEMORY_ACCESS_POINTER_Y);
    sgwrby(GHWWR,(xb+GHW_XOFFSET));
    /* Prepare for auto write */
    sgwrby(GHWCMD,GCTRL_RAMWR);

    #else
    /* 16 bit data bus (commands are on MSB) */
    sgwrwo(GHWCMDW,((SGUINT)(GCTRL_MY1_ADDR))<<CMDSHIFT);
    sgwrwo(GHWWRW,((SGUINT)((xb+GHW_XOFFSET)))<<CMDSHIFT);
    sgwrwo(GHWCMDW,((SGUINT)(GCTRL_MY2_ADDR))<<CMDSHIFT);
    sgwrwo(GHWWRW,((SGUINT)((xe+GHW_XOFFSET)))<<CMDSHIFT);
    sgwrwo(GHWCMDW,((SGUINT)(GCTRL_MX1_ADDR))<<CMDSHIFT);
    sgwrwo(GHWWRW,((SGUINT)((yb+GHW_YOFFSET)))<<CMDSHIFT);
    sgwrwo(GHWCMDW,((SGUINT)(GCTRL_MX2_ADDR))<<CMDSHIFT);
    sgwrwo(GHWWRW,((SGUINT)((ye+GHW_YOFFSET)))<<CMDSHIFT);

    sgwrwo(GHWCMDW,((SGUINT)(GCTRL_MEMORY_ACCESS_POINTER_X))<<CMDSHIFT);
    sgwrwo(GHWWRW,((SGUINT)((yb+GHW_YOFFSET)))<<CMDSHIFT);
    sgwrwo(GHWCMDW,((SGUINT)(GCTRL_MEMORY_ACCESS_POINTER_Y))<<CMDSHIFT);
    sgwrwo(GHWWRW,((SGUINT)((xb+GHW_XOFFSET)))<<CMDSHIFT);
    /* Prepare for auto write */
    sgwrby(GHWCMDW,GCTRL_RAMWR);

    #endif /* GHW_BUS8 */

   #else /* GHW_ROTATED */

    /* Set range (rotated display) */

    #if defined(GHW_BUS8)
    /* 8 bit data bus */
    sgwrby(GHWCMD,GCTRL_MY1_ADDR);
    sgwrby(GHWWR,(yb+GHW_YOFFSET));
    sgwrby(GHWCMD,GCTRL_MY2_ADDR);
    sgwrby(GHWWR,(ye+GHW_YOFFSET));
    sgwrby(GHWCMD,GCTRL_MX1_ADDR);
    sgwrby(GHWWR,(xb+GHW_XOFFSET));
    sgwrby(GHWCMD,GCTRL_MX2_ADDR);
    sgwrby(GHWWR,(xe+GHW_XOFFSET));

    sgwrby(GHWCMD,GCTRL_MEMORY_ACCESS_POINTER_X);
    sgwrby(GHWWR,(xb+GHW_XOFFSET));
    sgwrby(GHWCMD,GCTRL_MEMORY_ACCESS_POINTER_Y);
    sgwrby(GHWWR,(yb+GHW_XOFFSET));
    /* Prepare for auto write */
    sgwrby(GHWCMD,GCTRL_RAMWR);

    #else
    /* 16 bit data bus (commands are on MSB) */
    sgwrwo(GHWCMDW,((SGUINT)(GCTRL_MY1_ADDR))<<CMDSHIFT);
    sgwrwo(GHWWRW,((SGUINT)((yb+GHW_YOFFSET)))<<CMDSHIFT);
    sgwrwo(GHWCMDW,((SGUINT)(GCTRL_MY2_ADDR))<<CMDSHIFT);
    sgwrwo(GHWWRW,((SGUINT)((ye+GHW_YOFFSET)))<<CMDSHIFT);
    sgwrwo(GHWCMDW,((SGUINT)(GCTRL_MX1_ADDR))<<CMDSHIFT);
    sgwrwo(GHWWRW,((SGUINT)((xb+GHW_XOFFSET)))<<CMDSHIFT);
    sgwrwo(GHWCMDW,((SGUINT)(GCTRL_MX2_ADDR))<<CMDSHIFT);
    sgwrwo(GHWWRW,((SGUINT)((xe+GHW_XOFFSET)))<<CMDSHIFT);

    sgwrwo(GHWCMDW,((SGUINT)(GCTRL_MEMORY_ACCESS_POINTER_X))<<CMDSHIFT);
    sgwrwo(GHWWRW,((SGUINT)((xb+GHW_XOFFSET)))<<CMDSHIFT);
    sgwrwo(GHWCMDW,((SGUINT)(GCTRL_MEMORY_ACCESS_POINTER_Y))<<CMDSHIFT);
    sgwrwo(GHWWRW,((SGUINT)((yb+GHW_XOFFSET)))<<CMDSHIFT);
    /* Prepare for auto write */
    sgwrwo(GHWCMDW,((SGUINT)(GCTRL_RAMWR))<<CMDSHIFT);
    #endif /* GHW_BUS8 */

   #endif /* GHW_ROTATED */
   #endif /* GHW_NOHDW */
   }

/*
   Write databyte to controller (at current position) and increment
   internal xadr.

   Internal ghw function
*/
void ghw_auto_wr(GCOLOR dat)
   {
   #ifdef GHW_PCSIM
   ghw_autowr_sim( dat );
   #endif

   #ifndef GHW_NOHDW
   #if (GDISPPIXW <= 16)
     /* 16 bit color mode */
     #ifdef GHW_BUS8
      /* 8 bit bus mode (64K color) */
      sgwrby(GHWWR, (SGUCHAR)(dat>>8));     /* MSB */
      sgwrby(GHWWR, (SGUCHAR)(dat & 0xff)); /* LSB */
     #else
      /* 16 bit bus mode (64K color) */
      sgwrwo(GHWWRW, dat);           /* 16 bit color */
     #endif
   #else
      /* 24 bit color mode (of which 18 bit is used by controller) */
      #ifdef GHW_BUS8
      /* 8 bit bus mode uses standard RGB value (i.e. one byte pr color) */
      sgwrby(GHWWR, (SGUCHAR)((dat>>16) & 0xff));  /* R */
      sgwrby(GHWWR, (SGUCHAR)((dat>>8) & 0xff));   /* G */
      sgwrby(GHWWR, (SGUCHAR)(dat & 0xff));        /* B */
      #else
      /* 16 bit bus mode */
      /* It is assumed that display DB17(msb) is connected to bus DB15 (msb) */
      /* (Convert from same 24 bit RGB mode as used by 8 bit bus mode) */
      SGUINT w;
      w = (SGUINT)(((dat >> 16) & 0xfc00) | ((dat >> 6) & 0x0380));
      sgwrwo(GHWWRW, w);
      w = (SGUINT)(((dat << 2) & 0xfc00) | ((dat << 5) & 0x1f80));
      sgwrwo(GHWWRW, w);
      #endif
   #endif /* GDISPPIXW */
   #endif /* GHW_NOHDW */
   }


/*
   Read databyte from controller at current pointer and
   increment pointer
   Wait for controller + data ready

   Internal ghw function
*/
GCOLOR ghw_rd(GXT xb, GYT yb)
   {
   /* Set position */
   #ifndef GHW_NOHDW
   GCOLOR ret;
   #ifdef GHW_ROTATED
    /* Rotated mode */
    #ifdef GHW_BUS8
    sgwrby(GHWCMD,GCTRL_MEMORY_ACCESS_POINTER_X);
    sgwrby(GHWWR,(yb+GHW_YOFFSET));
    sgwrby(GHWCMD,GCTRL_MEMORY_ACCESS_POINTER_Y);
    sgwrby(GHWWR,(xb+GHW_XOFFSET));
    sgwrby(GHWCMD,GCTRL_RAMRD);
    #else
    sgwrwo(GHWCMDW,((SGUINT)(GCTRL_MEMORY_ACCESS_POINTER_X))<<CMDSHIFT);
    sgwrwo(GHWWRW,((SGUINT)((yb+GHW_YOFFSET)))<<CMDSHIFT);
    sgwrwo(GHWCMDW,((SGUINT)(GCTRL_MEMORY_ACCESS_POINTER_Y))<<CMDSHIFT);
    sgwrwo(GHWWRW,((SGUINT)((xb+GHW_XOFFSET)))<<CMDSHIFT);
    sgwrwo(GHWCMDW,((SGUINT)(GCTRL_RAMRD))<<CMDSHIFT);
    #endif
   #else
    /* Normal mode */
    #ifdef GHW_BUS8
    sgwrby(GHWCMD,GCTRL_MEMORY_ACCESS_POINTER_X);
    sgwrby(GHWWR,(xb+GHW_XOFFSET));
    sgwrby(GHWCMD,GCTRL_MEMORY_ACCESS_POINTER_Y);
    sgwrby(GHWWR,(yb+GHW_YOFFSET));
    sgwrby(GHWCMD,GCTRL_RAMRD);
    #else
    sgwrwo(GHWCMDW,((SGUINT)(GCTRL_MEMORY_ACCESS_POINTER_X))<<CMDSHIFT);
    sgwrwo(GHWWRW,((SGUINT)((xb+GHW_XOFFSET)))<<CMDSHIFT);
    sgwrwo(GHWCMDW,((SGUINT)(GCTRL_MEMORY_ACCESS_POINTER_Y))<<CMDSHIFT);
    sgwrwo(GHWWRW,((SGUINT)((yb+GHW_YOFFSET)))<<CMDSHIFT);
    sgwrwo(GHWCMDW,((SGUINT)(GCTRL_RAMRD))<<CMDSHIFT);
    #endif
   #endif

   /* Read data according to pixel width mode */
   #if (GDISPPIXW <= 16)
      /* 16 bit data */
      #ifdef GHW_BUS8
      ret = (GCOLOR) sgrdby(GHWRD); /* dummy */
      ret = (GCOLOR) sgrdby(GHWRD); /* MSB*/
      ret <<= 8;
      ret |=  (GCOLOR) sgrdby(GHWRD); /* LSB*/
      #else
      ret = sgrdwo(GHWRDW); /* Dummy */
      ret = sgrdwo(GHWRDW); /* Data */
      return ret;
      #endif
   #else
      /* 24 (18) bit data */
      #ifdef GHW_BUS8
      /* 8 bit bus mode */
      ret = sgrdby(GHWRD); /* dummy */
      ret =  (((GCOLOR) (sgrdby(GHWRD) & 0xfc)) << 16); /* MSB (RRRRRR**) */
      ret |= (((GCOLOR) (sgrdby(GHWRD) & 0xfc)) << 8);  /*     (GGGGGG**) */
      ret |=   (GCOLOR) (sgrdby(GHWRD) & 0xfc);         /* LSB (BBBBBB**) */
      #else
      {
      SGUINT w1,w0;
      /* 16 bit bus mode (convert to same 24 bit RGB data used with 8 bit bus) */
      /* It is assumed that display DB17(msb) is connected to bus DB15 (msb) */
      ret = sgrdwo(GHWRDW); /* Dummy */
      w1 =  (SGUINT) sgrdwo(GHWRDW);       /* MSB (RRRRRRGGG*****) */
      w0 =  (SGUINT) sgrdwo(GHWRDW);       /* LSB (GGGBBBBBB*****) */
      ret =  ((GCOLOR)(w1 & 0xfc00)) << 8;  /* R */
      ret |= ((GCOLOR)(w1 & 0x0380)) << 6;  /* G1 */
      ret |= ((GCOLOR)(w0 & 0xe000)) >> 3;  /* G2 */
      ret |= ((GCOLOR)(w0 & 0x1f80)) >> 5;  /* B */
      }
      #endif
    #endif /* GDISPPIXW */
    return ret;
   #else
     #ifdef GHW_PCSIM
      return ghw_autord_sim();
     #else
      return 0;
     #endif
   #endif /* GHW_NOHDW */
   }

/*********************** local configuration functions *******************/

/*
   Send a command + command byte to SEPS525

   Internal ghw function
*/
static void ghw_cmd_dat_wr(SGUCHAR cmd, SGUCHAR dat)
   {
   #ifndef GHW_NOHDW
   #ifdef GHW_BUS8
   sgwrby(GHWCMD, cmd);
   sgwrby(GHWWR, dat);
   #else
   sgwrwo(GHWCMDW, ((SGUINT) cmd) << CMDSHIFT);
   sgwrwo(GHWWRW,  ((SGUINT) dat) << CMDSHIFT);
   #endif
   #endif
   }

/*
   Read status
   Not used by library, Can be used for initial test.
*/
/*
SGUCHAR ghw_sta(void)
   {
   #ifdef GHW_NOHDW
   return 0;
   #else

   #ifdef GHW_BUS8
      sgwrby(GHWCMD, GCTRL_STATUS);
      return sgrdby(GHWSTA);
   #else
      sgwrwo(GHWCMDW, (GCTRL_STATUS << CMDSHIFT));
      return (SGUCHAR) (sgrdwo(GHWSTAW) >> CMDSHIFT);
   #endif

   #endif
   }
*/

/***********************************************************************/
/**        SEPS525 Initialization and error handling functions       **/
/***********************************************************************/

/*
   Change default (palette) colors
*/
void ghw_setcolor(GCOLOR fore, GCOLOR back)
   {
   /* Update active colors */
   ghw_def_foreground = fore;
   ghw_def_background = back;
   }

/*
   Convert an RGB structure to a color value using the current color mode
*/
GCOLOR ghw_rgb_to_color( GCONSTP GPALETTE_RGB *palette )
   {
   if (palette == NULL)
      return 0;

   return G_RGB_TO_COLOR(palette->r,palette->g,palette->b);
   }


#if (GHW_PALETTE_SIZE > 0)
/*
   Load a new palette or update the existing palette
   (Palette is only used with symbols using 2 or 4 bits pr pixel)
*/
SGBOOL ghw_palette_wr(SGUINT start_index, SGUINT num_elements, GCONSTP GPALETTE_RGB PFCODE *palette)
   {
   if ((num_elements == 0) ||
       ((start_index + num_elements) > 16) ||
       (palette == NULL))
      {
      glcd_err = 1;
      return 1;
      }
   glcd_err = 0;

   /* (Partial) update of operative palette values */
   while(num_elements-- > 0)
      {
      /* Make local palette copy here to be compatible with compilers
         having a non-standard conforming handling of pointer
         (i.e when PFCODE memory qualifer is used) */
      GPALETTE_RGB pal;
      pal.r = palette->r;
      pal.g = palette->g;
      pal.b = palette->b;
      ghw_palette_opr[start_index++] = ghw_rgb_to_color(&pal);
      palette++;

      /* ghw_palette_opr[start_index++] = ghw_rgb_to_color(&palette++); */
      }

   return glcd_err;
   }
#endif

/*
   Fast set or clear of LCD module RAM buffer
   Internal ghw function
*/
static void ghw_bufset(GCOLOR color)
   {
   GBUFINT cnt;
   cnt = 0;
   ghw_set_xyrange(0,0,GDISPW-1,GDISPH-1);
   do
      {
      /* Clear using X,Y autoincrement */
      ghw_auto_wr(color);  /* Set LCD buffer */
      #ifdef GBUFFER
      gbuf[cnt] = color; /* Set ram buffer as well */
      #endif
      }
   while (++cnt < ((GBUFINT) GDISPW) * ((GBUFINT) GDISPH)); /* Loop until x+y wrap */
   }


/*
   Make write-readback test on controller memory.

   This test will fail if some databus and control signals is not connected correctly.

   This test will fail if 16/8 bit bus mode selection in the configuration settings
   does not match the actual bus configuration for the hardware (display and processor
   16/8 bit bus width, 8080/6800 bus type settings, word / byte address offsets, etc).

   This test may fail if illegal GCTRLW, GCTRLH, GHW_XOFFSET, GHW_YOFFSET
   configuration settings cause overrun of the on-chip video RAM.

   This test can be exectuted correctly with only logic power on the display module.
   No high-level voltages are nessesary for the test to run (although nothing then can
   be shown on the display)

   Return 0 if no error,
   Return != 0 if some readback error is detected (the bit pattern may give information
   about connector pins in error)

   NOTE:
   This function should be commented out in serial mode. In serial mode
   SEPS525 does not provide read-back facility and this test will always
   fail.
*/

static GCOLOR ghw_wr_rd_test(void)
   {
   #ifndef GHW_NOHDW
   int i,j;
   GCOLOR msk,result;
   ghw_set_xyrange(0,0,GDISPPIXW*2,GDISPPIXW*2);
   for (i = 0, msk = 1; i < GDISPPIXW; i++)
      {
      ghw_auto_wr(msk);
      /* printf("0x%04x ", (unsigned int) msk); */
      ghw_auto_wr(~msk);
      /* printf("0x%04x ", (unsigned int) (~msk & 0xffff)); */
      msk <<= 1;
      }

   /* printf("\n"); */

   for (i=0,j=0,msk=1,result=0; i < GDISPPIXW;i++)
      {
      GCOLOR val;
      val = ghw_rd(j++,0);
      /* printf("0x%04x ", (unsigned short) (val & 0xffff)); */
      result |= (val ^ msk);
      val = ghw_rd(j++,0);
      /* printf("0x%04x ", (unsigned short) (val & 0xffff)); */
      result |= (val ^ (~msk));
      msk <<= 1;
      }
   #if (GDISPPIXW == 24)
     result &= 0x00fcfcfc; /* Mask bits not used by controller (left aligned color) */
   #endif
   return result;  /* 0 = Nul errors */
   #else
   return 0; /* 0 = Nul errors */
   #endif
   }

/*
   Initialize display, clear ram  (low-level)
   Clears glcd_err status before init

   Return 0 if no error,
   Return != 0 if some error
*/
SGBOOL ghw_init(void)
   {
   SGUCHAR dat;

   #ifdef GBUFFER
   iltx = 1;
   ilty = 1;
   irbx = 0;
   irby = 0;
   ghw_upddelay = 0;
   #endif

   glcd_err = 0;
   ghw_io_init(); /* Set any hardware interface lines, controller hardware reset */

   #if (defined( GHW_ALLOCATE_BUF) && defined( GBUFFER ))
   if (gbuf == NULL)
      {
      /* Allocate graphic ram buffer */
      if ((gbuf = (GCOLOR *)calloc(ghw_gbufsize(),1)) == NULL)
         glcd_err = 1;
      else
         gbuf_owner = 1;
      }
   #endif

   if (glcd_err)
      return 1;

   #ifdef GHW_PCSIM
   /* Tell simulator about the visual LCD screen organization */
   ghw_init_sim( GDISPW, GDISPH );
   #endif
   /* Set default colors */
   ghw_setcolor( GHW_PALETTE_FOREGROUND, GHW_PALETTE_BACKGROUND );

   #if (GHW_PALETTE_SIZE > 0)
   /* Load palette */
   ghw_palette_wr(0, sizeof(ghw_palette)/sizeof(GPALETTE_RGB), (GCONSTP GPALETTE_RGB PFCODE *)&ghw_palette[0]);
   #endif

   /* Initialize controller according to gdispcfg.h configuration */
   ghw_cmd_dat_wr(GCTRL_OSC_CTL,             0x01 ); /* External resistor */
   ghw_cmd_dat_wr(GCTRL_CLOCK_DIV,           0x30 ); /* 90 Hz */
   ghw_cmd_dat_wr(GCTRL_PRECHARGE_TIME_R   , 0x01 );
   ghw_cmd_dat_wr(GCTRL_PRECHARGE_TIME_G   , 0x01 );
   ghw_cmd_dat_wr(GCTRL_PRECHARGE_TIME_B   , 0x02 );
   ghw_cmd_dat_wr(GCTRL_PRECHARGE_CURRENT_R, 0x0c );
   ghw_cmd_dat_wr(GCTRL_PRECHARGE_CURRENT_G, 0x19 );
   ghw_cmd_dat_wr(GCTRL_PRECHARGE_CURRENT_B, 0x15 );

   ghw_cont_set( 20 );       /* 50% contrast = reset default */
   #ifdef GHW_ROTATED
     #ifdef GHW_MIRROR_VER
     dat = 0x00;
     #else
     dat = 0x10;
     #endif
     #ifdef GHW_MIRROR_HOR
     dat |= 0x20;
     #endif
   #else
     #ifdef GHW_MIRROR_HOR
     dat = 0x10;
     #else
     dat = 0x00;
     #endif
     #ifdef GHW_MIRROR_VER
     dat |= 0x20;
     #endif
   #endif
   /* dat |= 0x02; */  /* All pixels on (for initial test) */
   ghw_cmd_dat_wr(GCTRL_DISPLAY_MODE_SET, dat );

   ghw_cmd_dat_wr(GCTRL_RGB_IF,       0x11 );
   ghw_cmd_dat_wr(GCTRL_DSL,          0x00 );
   ghw_cmd_dat_wr(GCTRL_D1_DDRAM_FAC, 0x00 );
   ghw_cmd_dat_wr(GCTRL_D1_DDRAM_SAC, 0x00 );

   #ifdef GHW_ROTATED
   ghw_cmd_dat_wr(GCTRL_DUTY,     GDISPW-1 );
   ghw_cmd_dat_wr(GCTRL_SCR1_FX1, 0x0 );
   ghw_cmd_dat_wr(GCTRL_SCR1_FX2, GDISPH-1 );
   ghw_cmd_dat_wr(GCTRL_SCR1_FY1, 0x00 );
   ghw_cmd_dat_wr(GCTRL_SCR1_FY2, GDISPW-1 );
   dat = 7;  /* Auto address increment, vertical */
   #else
   ghw_cmd_dat_wr(GCTRL_DUTY,     GDISPH-1 );
   ghw_cmd_dat_wr(GCTRL_SCR1_FX1, 0x0 );
   ghw_cmd_dat_wr(GCTRL_SCR1_FX2, GDISPW-1 );
   ghw_cmd_dat_wr(GCTRL_SCR1_FY1, 0x00 );
   ghw_cmd_dat_wr(GCTRL_SCR1_FY2, GDISPH-1 );
   dat = 6;  /* Auto address increment, hortizontal */
   #endif

   #ifdef GHW_BUS8
      #if (GDISPPIXW <= 16)
      dat |= 0x60;   /* 8 bit bus, Dual transfer, 65k support */
      #else
      dat |= 0x70;   /* 8 bit bus, Triple transfer, 262k support */
      #endif
   #else
      #if (GDISPPIXW <= 16)
      dat |= 0x20;   /* 16 bit bus, Single transfer, 65k support */
      #else
      dat |= 0x40;   /* 16 bit bus, dual 9bit transfer, 262k support */
      #endif
   #endif
   ghw_cmd_dat_wr(0x80, 0);
   ghw_cmd_dat_wr(GCTRL_MEMORY_WRITE_MODE, dat );
   /*  ghw_cmd_dat_wr(GCTRL_DISP_ON_OFF,  0x81 );*/ /* Display on */

   /*
      NOTE:
      The call of ghw_wr_rd_test() should be commented out in serial mode.
      In serial mode the display controller does not provide read-back facility
      and this test will therefore always fail.
   */
   if (ghw_wr_rd_test() != ((GCOLOR) 0))
      {
      /* Controller memory write-readback error detected
      (Check the cable or power connections to the display) */
      G_WARNING("Hardware interface error\nCheck display connections\n");  /* Test Warning message output */
      glcd_err = 1;
      return 1;
      }

   ghw_bufset( ghw_def_background );


   /* Set display on  */
   ghw_dispon();

   #ifndef GNOCURSOR
   ghw_cursor = GCURSIZE1;    /* Cursor is off initially */
   /* ghw_cursor = GCURSIZE1 | GCURON; */ /* Uncomment to set cursor on initially */
   #endif

   ghw_updatehw();  /* Flush to display hdw or simulator */

   return (glcd_err != 0) ? 1 : 0;
   }

/*
   Return last error state. Called from applications to
   check for LCD HW or internal errors.
   The error state is reset by ghw_init and all high_level
   LCD functions.

   Return == 0 : No errors
   Return != 0 : Some errors
*/
SGUCHAR ghw_err(void)
   {
   #if (defined(_WIN32) && defined( GHW_PCSIM))
   if (GSimError())
      return 1;
   #endif
   return (glcd_err == 0) ? 0 : 1;
   }


/*
   Display a (fatal) error message.
   The LCD display module is always cleared and initialized to
   the system font in advance.
   The error message is automatically centered on the screen
   and any \n characters in the string is processed.

   str = ASCII string to write at display center
*/
void ghw_puterr( PGCSTR str )
   {
   PGCSTR idx;
   SGUINT xcnt;
   GXT xp;
   GYT yp,h,y, sidx;
   PGSYMBYTE psym;
   GCOLOR pval;
   SGUCHAR val;
   #ifdef GBUFFER
   GBUFINT gbufidx;
   #endif

   if (ghw_init() != 0)  /* (Re-) initialize display */
      return;            /* Some initialization error */

   /* Count number of lines in string */
   idx=str;
   if (idx == NULL)
      return;
   xcnt = 1;
   while(*idx)
      {
      if (*(idx++) == '\n')
         xcnt++;
      }

   /* Set start character line */
   h = SYSFONT.symheight;
   yp = (xcnt*h > GDISPH) ? 0 : ((GDISPH-1)-xcnt*h)/2;
   /* Set character height in pixel lines */

   idx=str;
   do
      {
      xcnt=0;  /* Set start x position so line is centered */
      while ((idx[xcnt]!=0) && (idx[xcnt]!='\n') && (xcnt < GDISPBW))
         {
         xcnt++;
         }

      /* Calculate start position for centered line */
      xp = (GDISPW-xcnt*SYSFONT.symwidth)/2;

      /* Display text line */
      while (xcnt-- > 0)
         {
         /* Point to graphic content for character symbol */
         psym = &(sysfontsym[(*idx) & 0x7f].b[0]);
         ghw_set_xyrange(xp,yp,xp+SYSFONT.symwidth-1,yp+(h-1));

         /* Display rows in symbol */
         for (y = 0; y < h; y++)
            {
            /* Get symbol row value */
            val = *psym++;
            /* Initiate LCD controller address pointer */
            #ifdef GBUFFER
            gbufidx = GINDEX(xp, (GBUFINT)yp+y );
            #endif

            /* Display colums in symbol row */
            for (sidx = 0; sidx < SYSFONT.symwidth; sidx++)
               {
               if ((val & sympixmsk[sidx]) != 0)
                  pval = ghw_def_foreground;
               else
                  pval = ghw_def_background;

               /* End of symbol or end of byte reached */
               #ifdef GBUFFER
               gbuf[gbufidx++] = pval;
               #endif
               ghw_auto_wr(pval);
               }
            }

         idx++;
         xp += SYSFONT.symwidth; /* Move to next symbol in line */
         }

      /* Next text line */
      yp += h;
      if (*idx == '\n')
         idx++;
      }
   while ((*idx != 0) && (yp < GDISPH));

   ghw_updatehw();  /* Flush to display hdw or simulator */
   }

void ghw_exit(void)
   {
   #if defined( GHW_ALLOCATE_BUF)
   if (gbuf != NULL)
      {
      if (gbuf_owner != 0)
         {
         /* Buffer is allocated by ginit, so release graphic buffer here */
         free(gbuf);
         gbuf_owner = 0;
         }
      gbuf = NULL;
      }
   #endif
   ghw_io_exit();         /* Release any LCD hardware resources, if required */
   #ifdef GHW_PCSIM
   ghw_exit_sim(); /* Release simulator resources */
   #endif
   }

#ifndef GNOCURSOR
/*
   Replace cursor type data (there is no HW cursor support in SEPS525)
*/
void ghw_setcursor( GCURSOR type)
   {
   ghw_cursor = type;
   #ifdef GHW_ALLOCATE_BUF
   if (gbuf == NULL)
      glcd_err = 1;
   #endif
   }
#endif


/*
   Turn display off
   (Minimize power consumption)
*/
void ghw_dispoff(void)
   {
   #ifdef GHW_PCSIM
   ghw_dispoff_sim();
   #endif
    /* Blank display */
   ghw_cmd_dat_wr(GCTRL_DISP_ON_OFF, 0x80 );
   }

/*
   Turn display on
*/
void ghw_dispon(void)
   {
   #ifdef GHW_PCSIM
   ghw_dispon_sim();
   #endif
   /* Enable display */
   ghw_cmd_dat_wr(GCTRL_DISP_ON_OFF, 0x81 );
   }

#ifdef GHW_INTERNAL_CONTRAST
/*
   Set contrast (Normalized value range [0 : 99] )
   Returns the old value.
*/
SGUCHAR ghw_cont_set(SGUCHAR contrast)
   {
   SGUCHAR tmp;
   GLIMITU(contrast,99);
   tmp = ghw_contrast;
   ghw_contrast = contrast;

   #if (defined( GHW_ALLOCATE_BUF) && defined( GBUFFER ))
   if (gbuf == NULL) {glcd_err = 1; return contrast;}
   #endif

   contrast = (SGUCHAR)((((SGUINT) contrast) * 128) / 99); /* Set contrast level (0-255) */
   ghw_cmd_dat_wr(GCTRL_DRIVING_CURRENT_R, 0x32);//contrast );
   ghw_cmd_dat_wr(GCTRL_DRIVING_CURRENT_G, 0x27);//contrast );
   ghw_cmd_dat_wr(GCTRL_DRIVING_CURRENT_B, 0x2b);//contrast );

   return tmp;
   }

/*
   Change contrast (Normalized value range [-99 : +99] )
   Returns the old value.
*/
SGUCHAR ghw_cont_change(SGCHAR contrast_diff)
   {
   SGINT tmp = (SGINT) ghw_contrast;
   tmp += (SGINT) contrast_diff;
   GLIMITU(tmp,99);
   GLIMITD(tmp,0);
   return ghw_cont_set((SGUCHAR)tmp);
   }
#endif /* GHW_INTERNAL_CONTRAST */

#if defined( GHW_ALLOCATE_BUF)
/*
   Size of buffer requied to save the whole screen state
*/
GBUFINT ghw_gbufsize( void )
   {
   return (GBUFINT) GBUFSIZE * sizeof(GCOLOR) + (GBUFINT) sizeof(GHW_STATE);
   }

#ifdef GSCREENS
/*
   Check if screen buf owns the screen ressources.
*/
SGUCHAR ghw_is_owner( SGUCHAR *buf )
   {
   return (((GCOLOR *)buf == gbuf) && (gbuf != NULL)) ? 1 : 0;
   }

/*
   Save the current state to the screen buffer
*/
SGUCHAR *ghw_save_state( SGUCHAR *buf )
   {
   GHW_STATE *ps;
   if (!ghw_is_owner(buf))
      return NULL;

   ps = (GHW_STATE *)(&gbuf[GBUFSIZE]);
   ps->upddelay = (ghw_upddelay != 0);
   #ifndef GNOCURSOR
   ps->cursor = ghw_cursor;
   #endif
   ps->foreground = ghw_def_foreground; /* Palette may vary, save it */
   ps->background = ghw_def_background;
   return (SGUCHAR *) gbuf;
   }

/*
   Set state to buf.
   If buffer has not been initiated by to a screen before, only
   the pointer is updated. Otherwise the the buffer
*/
void ghw_set_state(SGUCHAR *buf, SGUCHAR doinit)
   {
   if (gbuf != NULL)
      {
      /* The LCD controller has been initiated before */
      if (gbuf_owner != 0)
         {
         /* Buffer was allocated by ginit, free it so screen can be used instead*/
         free(gbuf);
         gbuf_owner = 0;
         gbuf = NULL;
         }
      }

   if (doinit != 0)
      {
      /* First screen initialization, just set buffer pointer and
         leave rest of initialization to a later call of ghw_init() */
      gbuf = (GCOLOR *) buf;
      gbuf_owner = 0;
      }
   else
      {
      if ((gbuf = (GCOLOR *) buf) != NULL)
         {
         GHW_STATE *ps;
         ps = (GHW_STATE *)(&gbuf[GBUFSIZE]);

         #ifndef GNOCURSOR
         ghw_cursor = ps->cursor;
         #endif
         ghw_upddelay = 0;        /* Force update of whole screen */
         iltx = 0;
         ilty = 0;
         irbx = GDISPW-1;
         irby = GDISPH-1;
         ghw_updatehw();
         ghw_upddelay = (ps->upddelay != 0) ? 1 : 0;
         /* Restore drawing color */
         ghw_setcolor(ps->foreground, ps->background);
         }
      }
   }
#endif  /* GSCREENS */
#endif  /* GHW_ALLOCATE_BUF */

#endif /* GBASIC_INIT_ERR */

