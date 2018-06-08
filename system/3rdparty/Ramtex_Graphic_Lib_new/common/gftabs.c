/************************* gftabs.c ********************************

   Creation date: 980226

   Revision date:     03-05-06
   Revision Purpose:  GTABS array replaced with gdata.tab array

   Revision date:     13-08-04
   Revision Purpose:  Named viewport function _vp added

   Revision date:     14-11-12
   Revision Purpose:  Generic GDATA macros used

   Version number: 2.1
   Copyright (c) RAMTEX International Aps 1998-2012
   Web site, support and upgrade: www.ramtex.dk

*********************************************************************/
#include <gi_disp.h>

#ifndef GCONSTTAB

/********************************************************************
   Segment: Tabs
   Level: Fonts
   Sets GDATA_TABS[] with tab-positions, spaced s.
   GTABS must be define by user
*/
void gsettabs( GXT s )
   {
   SGUCHAR i;
   for( i=0; i<GMAXTABS; i++ )
      GDATA_TABS[i] = (GXT) s*(i+1);
   }

/********************************************************************
   Segment: Tabs
   Level: Fonts
   Sets GDATA_TABS[] with a tab at s, moving other tabs.
   GTABS must be define by user
*/
void gsettab( GXT s )
   {
   SGUCHAR i,j;

   /* find insert pos */
   for( i=0; i<GMAXTABS; i++ )
      {
      if( GDATA_TABS[i] >= s ) break;
      if (GDATA_TABS[i] == 0)  break;
      }

   if( i >= GMAXTABS )
      return; /* Not room for a new tab */

   if( GDATA_TABS[i] == s ) /* don't set two equals */
      return;
   for( j=(GMAXTABS-1); j>i; j-- )  /* move tabs */
      GDATA_TABS[j] = GDATA_TABS[j-1];
   GDATA_TABS[i] = s;
   }

/********************************************************************
   Segment: Tabs
   Level: Fonts
   Clears GDATA_TABS[] to zeros.
   GTABS must be define by user
*/
void gclrtabs(void)
   {
   SGUCHAR n;
   n = GMAXTABS;
   while (n > 0)
      {
      GDATA_TABS[--n] = 0;
      }
   }

#endif /* GCONSTTAB */


