/**
* @file hedit.h                               LuminariMUD
* Oasis OLC Help Editor.
* 
* Part of the core tbaMUD source code distribution, which is a derivative
* of, and continuation of, CircleMUD.
*                                                                        
* Author: Steve Wolfe, Scott Meisenholder, Rhade
* All rights reserved.  See license.doc for complete information.
* CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.                             
*
*/
#ifndef _HEDIT_H_
#define _HEDIT_H_

#include "utils.h" /* for ACMD definition */

/* Functions made externally available */
/* Utility functions */
/*
 * All the following functions are declared in oasis.h
  void hedit_parse(struct descriptor_data *, char *);
  void hedit_string_cleanup(struct descriptor_data *, int);
  ACMD_DECL(do_oasis_hedit);
*/
/* Action fuctions */
ACMD_DECL(do_helpcheck);
ACMD_DECL(do_hindex);

#endif /* _HEDIT_H_*/
