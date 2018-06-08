#include <gi_disp.h> /* gLCD prototypes */

#ifdef GMULTIBYTE
/*
   Fetch either a character or a multibyte character from string and
   increment string pointer to the following character (or the terminating 0)

   When GMULTIBYTE is defined then gi_mbtowc(..) is used internally by
   the LCD driver library to fetch characters from strings.

   This module support two different multi-byte implementations
   1) The "shift value" multibyte mode where a character is 1 or 2 bytes depending
      on the value of the (first) byte. With G_MULTIBYTE_LIMIT defined as
      0x80 then the range covered is {0-0x7f} and {0x8000-0xffff}
      "Shift value mode" is default ( GMULTIBYTE_UTF8 undefined)

   2) The "UTF-8 mode" where a character is 1,2,3,4,5,6 bytes depending
      on the value of the first bits in the first byte.
      If the MSB bit = 1 defines if it is a multibyte value and the
      following bits in the first byte defines the number of bytes to follow.
      The UTF-8 mode covers the character value range 0-0x7ffffff
      "UTF-8 mode" is selected by defining GMULTIBYTE_UTF8 in gdispcfg.h

   If the LCD driver library should support another multi-byte shift /
   fetch method then this module is the place to add the modifications.

   Revision date:     090207
   Revision Purpose:  gi_sizeofmb(..) defined to detect number of byte
                      occupied by (multibyte) character. Isolate multi-byte
                      implementation knowledge to this module.

   Revision date:     190407
   Revision Purpose:  GMULTIBYTE_UTF8 support added
   Revision date:     191207
   Revision Purpose:  Use of WCHAR_MAX introduced

   Version number: 2.3
   Copyright (c) RAMTEX Engineering Aps 2004-2007
*/

#ifndef GMULTIBYTE_UTF8

/*
   Shift value multibyte mode

   This implementation is for compilers which support
   2 byte multibyte characters, where the shift to the
   multibyte character mode takes place when the byte is
   above G_MULTIBYTE_LIMIT. The byte then become the
   MSB byte of a 2 byte character. G_MULTIBYTE_LIMIT
   is defined in gdisphw.h

*/

/* Return the number of bytes occupied by the (multi-byte) character pointed to */
SGUCHAR gi_sizeofmb(PGCSTR str)
   {
   if (((SGUCHAR)(*str)) >= ((SGUCHAR) G_MULTIBYTE_LIMIT))
      return 2;
   else
      return 1;
   }

/* Fetch multibyte, leave pointer on last char */
GWCHAR gi_mbtowc(PGCSTR *strp, SGUCHAR using_codepage)
   {
   GWCHAR wc;
   /* Fetch first byte */
   wc = (GWCHAR) *((PGCUCHAR)(*strp)); /* Get byte and convert it to wide char */
   if ((wc >= G_MULTIBYTE_LIMIT) &&   /* Is character the first byte of a 2 byte value ?*/
       (using_codepage != 0))         /* Multibyte characters is only allowed with gputs with codepages */
      {
      /* Fetch second byte */
      (*strp)++;
      wc = (GWCHAR) (wc * 256) + (GWCHAR) *((PGCUCHAR)(*strp));
      }
   return wc;
   }

#else  /* GMULTIBYTE_UTF8 */

/*
   UTF-8 multibyte mode

   This implementation is for UTF-8 coded strings.
   According to the UTF-8 standard a logical character value can occupy 1-6
   string bytes, covering a logical character value range from 0-0x7fffffff.

   This implementation only returns values in the range 0x0000-0xffff.
   Logical values >= 0x10000 are treated as a malformed multibyte and the
   MALFORMED_RETURNCHAR character value is returned instead.
   The multi-byte value is however always consumed normally from the string.

   Oversized multi-bytes (use of multibyte format larger than needed to hold
   the logical value) are just consumed normally for maximum decoder tolerance
   and not marked as mal-formed. The only exception is a converted logic value
   of 0 which will be returned as MALFORMED_RETURNCHAR in order not to confuse
   a program which use the return value to test for string \0 termination.

   If a malformed multibyte is detected all string characters up to the next
   valid character value are consumed and MALFORMED_RETURNCHAR (?) is returned
   instead, in order to ease debugging.
*/

/* Malformed multibyte.
   Skip until plain ascii or valid multibyte start or cnt overrun
*/
static SGUCHAR gi_skip_ill_mb(PGCSTR str)
   {
   SGUCHAR cnt;
   for(cnt = 0; cnt < 0xff; cnt++)
      {
      if ( (((SGUCHAR)(*str)) < ((SGUCHAR)0x80)) ||
          ((*str & 0xe0) == 0xc0)  ||
          ((*str & 0xf0) == 0xe0)  ||
          ((*str & 0xf8) == 0xf0)  ||
          ((*str & 0xfc) == 0xf8)  ||
          ((*str & 0xfe) == 0xfc))
         /* Valid ASCII or start of new multibyte detected */
         return cnt; /* Number of malformed multibyte values skipped */
      str++;
      }
   return cnt; /* Completely illegal serie of multibyte values */
   }

/*
   Return number of storage bytes occupied by (multi-byte) character
*/
SGUCHAR gi_sizeofmb(PGCSTR str)
   {
   if (((SGUCHAR)(*str)) < ((SGUCHAR)0x80))
      return 1;
   if ((((SGUCHAR)(*str)) & 0xe0) == 0xc0)
      return 2;
   if ((((SGUCHAR)(*str)) & 0xf0) == 0xe0)
      return 3;
   if ((((SGUCHAR)(*str)) & 0xf8) == 0xf0)
      return 4;
   if ((((SGUCHAR)(*str)) & 0xfc) == 0xf8)
      return 5;
   if ((((SGUCHAR)(*str)) & 0xfe) == 0xfc)
      return 6;
   else
      {
      /* Malformed multibyte detected.
         Skip until plain ascii or valid multibyte start or cnt overrun */
      return gi_skip_ill_mb(str);
      }
   }

/*
    Fetch next (multi-byte) character from string.
    Leave pointer on last char

    If an illegal multibyte encoding is used a replacement character is returned instead.
    If using codepage = 0, the return value should be below 255, otherwise the
    replacement character is returned.
*/
GWCHAR gi_mbtowc(PGCSTR *strp, SGUCHAR using_codepage)
   {
   SGUCHAR c,cnt;
   SGULONG lw;
   c = (SGUCHAR) *((PGCUCHAR)(*strp)); /* Get byte */
   if (c < ((SGUCHAR)0x80))
      return (GWCHAR) c;  /* Plain ASCII */

   /* Multibyte, Do UTF-8 decoding, first bits contains UTF-8 size information */
   if ((c & 0xe0) == 0xc0)
      {
      c &= 0x1f; /* two byte multibyte */
      cnt = 1;
      }
   else
   if ((c & 0xf0) == 0xe0)
      {
      c &= 0x0f; /* three byte multibyte */
      cnt = 2;
      }
   else
   if ((c & 0xf8) == 0xf0)
      {
      c &= 0x07; /* four byte multibyte */
      cnt = 3;
      }
   else
   if ((c & 0xfc) == 0xf8)
      {
      c &= 0x03; /* five byte multibyte */
      cnt = 4;
      }
   else
   if ((c & 0xfe) == 0xfc)
      {
      c &= 0x01; /* six byte multibyte */
      cnt = 5;
      }
   else
      {
      /* Malformed multibyte, skip it (them) and return replacement value instead */
      return (GWCHAR) G_MALFORMED_RETURNCHAR;
      }
   /* Fetch second and following bytes */
   lw = (SGULONG) c;
   do
      {
      /* Get next byte and check it */
      c = (SGUCHAR) ((PGCUCHAR)(*strp))[1];
      if ((c & 0xc0) != 0x80)
         {
         /* Malformed multibyte, skip it and return replacement value instead */
         return (GWCHAR) G_MALFORMED_RETURNCHAR;
         }
      /* Accepted, was a multibyte continuation char, add it to value */
      lw =  (lw << 6) | ((SGULONG)(c & 0x3f));
      (*strp)++;
      }
   while (--cnt != 0);
   /* Logical character value assembled */

   if (lw == 0) /* Do not allow oversized UTF-8 0 to be interpreted as string terminator */
      return (GWCHAR) G_MALFORMED_RETURNCHAR;

   /* Check for value oversize */
   if (using_codepage != 0)
      {
      /* For compilers using 32 bit wide char this check can be commented out */
      if (!(lw <= WCHAR_MAX))     /* > 0xffff for GWCHAR = 2 bytes */
         /* Library parameter overrun
            Multibyte value can not be handled by library GWCHAR configuration
            Return replacement value instead */
         return (GWCHAR) G_MALFORMED_RETURNCHAR;
      }
   else
      { /* Assume ISO 8859 fonts (256 symbols max) */
      if (lw >= 256)
         /* Library font overrun will occur.
            Multibyte value can not be handled by viewport font type
            Return replacement value instead */
         return (GWCHAR) G_MALFORMED_RETURNCHAR;
      }
   return (GWCHAR)(lw);
   }

#endif /* GMULTIBYTE_UTF8 */

#endif /* GMULTIBYTE */

/*
   Fetch (multibyte) character from C string and update
   string pointer to the following character
*/
GWCHAR ggetmbc(PGCSTR *strp)
   {
   GWCHAR wc = 0;
   if (strp != NULL)
      if (*strp != NULL)
         {
         #ifdef GMULTIBYTE
         if ((wc = gi_mbtowc(strp, 1 )) != 0)
         #else
         if ((wc = (GWCHAR) **strp) != 0)
         #endif
            (*strp)++;
         }
   return wc;
   }

