/**************************************************************************
 *  File: mob_act.h                                   Part of LuminariMUD *
 *  Usage: Main header for mobile activity coordination                   *
 *                                                                         *
 *  All rights reserved.  See license for complete information.           *
 *                                                                         *
 *  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
 *  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.              *
 **************************************************************************/

#ifndef _MOB_ACT_H_
#define _MOB_ACT_H_

/* Include all mob module headers */
#include "mob_memory.h"
#include "mob_utils.h"
#include "mob_race.h"
#include "mob_psionic.h"
#include "mob_class.h"
#include "mob_spells.h"

/* Main mobile activity function */
void mobile_activity(void);

#endif /* _MOB_ACT_H_ */