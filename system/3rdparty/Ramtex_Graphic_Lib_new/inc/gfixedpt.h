#ifndef G_FIXEDPT_H
#define G_FIXEDPT_H
/****************************** GFIXEDPT.H *****************************
   Macros for fixed point type creation and conversion

   The fixed point value type defined here enables floating point type
   operations to be handled via native integer type arithmetic instead.
   This give significant faster processing with small processors.

   Revision date: 28-12-2015
   Revision Purpose: SGFIXP changed from a typedef to a #define to enable
                     #ifdef .. tests and to match a similar definitions in gi_disp.h

   Version number: 1.01
   Copyright (c) RAMTEX International Aps 2012-2015
   Web site, support and upgrade: www.ramtex.dk


***********************************************************************/
#include <sgtypes.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
   Best compromise for display coordinate calculations is a fraction part
   equal to number of bits for type / 3, where the fraction part have >=
   the bits required to represent the full screen size.

   This enables that:
   - two fixed point values (ex screen coordinates) up to 1024 can be
     multiplied without risk of overflow
   - pixel position moving using offset addition can be done incrementally
     while still maintaining a full screen sum-error less than one pixel

  Layout: 22 bit signed integer part with a 10 bit fraction part
*/
#ifndef SGFIXP
  #define SGFIXP SGLONG
#endif
#define SGFIXP_BITS     10  /* size of fraction part */

/* conversion constants */
#define SGFIXP_SCALE    ((SGFIXP)(1<<SGFIXP_BITS))  /* Multiply / division factor */
#define SGFRACTION_MSK  ((1<<SGFIXP_BITS)-1)        /* Mask for fraction bits  */

/* Convert integer to fixed point  */
#define SGI_TO_FIXP(i)  (((SGFIXP)(i))*SGFIXP_SCALE)
/* Convert fixed point to integer  (preserve sign) */
#define SGFIXP_TO_I(fp) ((SGINT)(((SGLONG)(fp))/((SGLONG)SGFIXP_SCALE)))

/* Fast convert unsigned integer to fixed point  */
#define SGU_TO_FIXP(ui) ((SGFIXP)(((SGLONG)((SGULONG)(ui))) << SGFIXP_BITS))
/* Fast convert of fixed point to unsigned integer  (assumes fp >= 0) */
#define SGFIXP_TO_U(fp) ((SGUINT)(((SGULONG)(fp)) >> SGFIXP_BITS))

/* Convert float to fixed point  */
#define SGFLOAT_TO_FIXP(f) ((SGFIXP)((f)*(1.0*SGFIXP_SCALE)))
#define SGFIXP_TO_FLOAT(fp) (((float)(fp))/(1.0*SGFIXP_SCALE))

/* Extract (signed) fraction part of fixed point value (preserve sign) */
#define SGFIXP_FRACTION(fp) ((SGINT)((fp)%SGFIXP_SCALE))

/* Absolute value of fixed point */
#define SGFIXP_ABS(fp)  (((fp) >= 0) ? (fp) : ((fp)*((SGFIXP)(-1))))

/* Multiply two fixed point values (while keeping precision) */
#define SGFIXP_MUL(fp1,fp2) (((fp1)*(fp2))/SGFIXP_SCALE)

/* Divide two fixed point values (while keeping precision) */
#define SGFIXP_DIV(fp1,fp2) (((fp1)*SGFIXP_SCALE)/(fp2))

#ifdef __cplusplus
}
#endif

#endif /* G_FIXEDPT_H */

