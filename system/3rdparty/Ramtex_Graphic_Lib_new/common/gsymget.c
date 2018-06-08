/************************* gsymget.c *******************************

   Creation date: 980223

   Revision date:     03-26-01
   Revision Purpose:  Bit mask on GINVERSE mode
   Revision date:     13-08-04
   Revision Purpose:  Named viewport function _vp added
   Revision date:     7-01-05
   Revision Purpose:  Pointer changed from PGSYMBOL to a GSYMBOL* type
                      (PGSYMBOL is now equal to 'const GSYMBOL* ')
   Revision date:     05-04-05
   Revision Purpose:  B&W or 2 color symbol mode now respect GINVERSE settings
   Revision date:     270405
   Revision Purpose:  The bw parameter to ghw_rdrsym(..) changed to SGUINT
                      to accomondate all combinations of display size and
                      pixel resolutions.
   Revision date:     290707
   Revision Purpose:  bw calculation corrected for odd pixel widths > 8
   Revision date:     16-11-07
   Revision Purpose:  Vfont adaption
   Revision date:     17-03-09
   Revision Purpose:  Enabled when GHW_NO_LCD_READ_SUPPORT and buffered mode
   Revision date:     20-03-09
   Revision Purpose:  Symbol pixel size forced to 1,2,4,8,16,24 or 32 bits
                      dependent on GDISPPIXW. Grey-level flag added for
                      grey-level displays for compatibility.
   Revision date:     19-11-09
   Revision Purpose:  Header symbol width for 18 bit pr pixel symbol corrected.
                      (24 bit pr pixel storage size is used for 18 bit symbol)
   Revision date:     10-01-12
   Revision Purpose:  Special header symbol default width for 18 bit pr pixel
                      compilation mode. Optimized with compile time constants.
   Revision date:     15-05-12
   Revision Purpose:  Optimized to use the GSYM_SIZE(w,h) macro
                      The old gsymsize(..) gsymsize_vp(..) is converted to macros
                      Use of the old gsymsize_vp(..) is deprecated.
   Revision date:     15-10-12
   Revision Purpose:  Byte row width calculation fixed

   Version number: 2.13
   Copyright (c) RAMTEX International Aps 1998-2012
   Web site, support and upgrade: www.ramtex.dk

*********************************************************************/
#include <gi_disp.h> /* gLCD prototypes */

#ifdef GSOFT_SYMBOLS

#if (defined(GBUFFER) || !defined( GHW_NO_LCD_READ_SUPPORT ))

/*******************************************************************
   Segment: Software symbols
   Level: Graphics
   Get rectangular screen area (or screen buffer area) as a graphic symbol
   using the current pixel resolution
*/
void ggetsym(GXT xs, GYT ys, GXT xe, GYT ye, GSYMBOL *psym, GBUFINT size )
   {
   GYT h;
   GXT w;
   gi_datacheck(); /* check internal data for errors */
   /* normalize to view-port */
   xs += gcurvp->lt.x;
   ys += gcurvp->lt.y;
   xe += gcurvp->lt.x;
   ye += gcurvp->lt.y;

   /* limit values to view-port */
   LIMITTOVP( "ggetsym",xs,ys,xe,ye );
   if( psym == NULL )
      {
      G_WARNING( "ggetsym: parameter, psym == NULL" );
      return;
      }

   glcd_err = 0; /* Reset HW error flag */
   h = (ye-ys)+1;
   w = (xe-xs)+1;
   if( size < GSYM_SIZE(w,h) )
      {
      G_WARNING( "ggetsym: parameter, buffer too small" );
      return;
      }

   #if (GDISPPIXW == 1)

   /* b&w mode */
   psym->sh.cypix = h;
   psym->sh.cxpix = w;
   ghw_rdsym(xs, ys, xe, ye, (SGUCHAR*)&(((GBWSYMBOL *)psym)->b[0]));

   #else

   /* Color / greylevel mode mode */
   psym->csh.cypix = h;
   psym->csh.cxpix = w;
   psym->csh.colorid = 0;
   /* Matching / fastest bit-pr-pixel selected by low-level driver */
   psym->csh.pbits = (GYT) ghw_rdsym(xs, ys, xe, ye, (SGUCHAR*)&(((GCSYMBOL*)psym)->b[0]));
   #ifdef GHW_USING_GREY_LEVEL
   psym->csh.pbits |= 0x40; /* Just for symbol compatibility */
   #endif
   #endif
   }

#if defined( GFUNC_VP )
void ggetsym_vp( SGUCHAR vp, GXT xs, GYT ys, GXT xe, GYT ye, GSYMBOL * psym, GBUFINT size )
   {
   GSETFUNCVP(vp, ggetsym(xs,ys,xe,ye,psym,size) );
   }
#endif /* GFUNC_VP */

#endif /* GBUFFER || !GHW_NO_LCD_READ_SUPPORT */

#endif /* GSOFT_SYMBOLS */

