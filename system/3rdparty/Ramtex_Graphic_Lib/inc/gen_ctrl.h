#ifndef GEN_CTRL_H
#define GEN_CTRL_H
/*
   Define low-level driver functions specific for the generic RGB controller driver 
   library, in addition to the normal settings in s6d0129.h
*/

#include <s6d0129.h>   /* Controller specific definements */

#ifdef __cplusplus
extern "C" {
#endif

/********** extended library functions in ghwinitctrl.c ********/

void ghw_ctrl_init(void);
void ghw_ctrl_exit(void);
void ghw_ctrl_line_wr( GXT xb, GYT yb, GCOLOR *cp, GXT endidx );

#ifdef __cplusplus
}
#endif

#endif /* GEN_CTRL_H */

