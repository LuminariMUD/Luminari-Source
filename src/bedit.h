/* ************************************************************************
*   File: bedit.h                                Part of LuminariMUD    *
*  Usage: Board editor header                                           *
*                                                                        *
*  All rights reserved.  See license.doc for complete information.      *
************************************************************************ */

#ifndef __BEDIT_H__
#define __BEDIT_H__

/* Function prototypes */
void bedit_parse(struct descriptor_data *d, char *arg);
ACMD_DECL(do_bedit);
ACMD_DECL(do_blist);

#endif /* __BEDIT_H__ */
