/************************* gftabs.c ********************************

   Creation date: 980226

   Revision date:     03-05-06
   Revision Purpose:  GTABS array replaced with gdata.tab array

   Revision date:     13-08-04
   Revision Purpose:  Named viewport function _vp added

   Version number: 2.1
   Copyright (c) RAMTEX Engineering Aps 1998-2004

*********************************************************************/
#include <gi_disp.h>

#ifndef GCONSTTAB

/********************************************************************
   Segment: Tabs
   Level: Fonts
   Sets gdata.tabs[] with tab-positions, spaced s.
   GTABS must be define by user
*/
void gsettabs( GXT s )
   {
   SGUCHAR n,i;
   n = sizeof(gdata.tabs)/sizeof(GXT); /* tab array size */
   for( i=0; i<n; i++ )
      gdata.tabs[i] = (GXT) s*(i+1);
   }

/********************************************************************
   Segment: Tabs
   Level: Fonts
   Sets gdata.tabs[] with a tab at s, moving other tabs.
   GTABS must be define by user
*/
void gsettab( GXT s )
   {
   SGUCHAR n,i,j;
   n = sizeof(gdata.tabs)/sizeof(GXT); /* tab array size */

   /* find insert pos */
   for( i=0; i<n; i++ )
      {
      if( gdata.tabs[i] >= s )
         break;
      if (gdata.tabs[i] == 0)
         break;
      }

   if( i >= n )
      return; /* Not room for a new tab */

   if( gdata.tabs[i] == s ) /* don't set two equals */
      return;

   /* move tabs */
   for( j=n-1; j>i; j-- )
      gdata.tabs[j] = gdata.tabs[j-1];

   gdata.tabs[i] = s;
   }

/********************************************************************
   Segment: Tabs
   Level: Fonts
   Clears gdata.tabs[] to zeros.
   GTABS must be define by user
*/
void gclrtabs(void)
   {
   SGUCHAR n;
   n = sizeof(gdata.tabs)/sizeof(GXT);
   while (n > 0)
      {
      gdata.tabs[--n] = 0;
      }
   }

#endif /* GCONSTTAB */


