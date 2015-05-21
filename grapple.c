/**************************************************************************
 *  File: grapple.c                                    Part of LuminariMUD *
 *  Usage: grapple combat maneuver mechanics and related functions         *
 *  Author: Zusuk                                                          *
 **************************************************************************/

#include <zconf.h>

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "interpreter.h"
#include "handler.h"
#include "db.h"
#include "spells.h"
#include "act.h"
#include "fight.h"
#include "mud_event.h"
#include "constants.h"
#include "spec_procs.h"
#include "class.h"
#include "mudlim.h"
#include "actions.h"
#include "actionqueues.h"
#include "assign_wpn_armor.h"
#include "feats.h"

/*****/

/* called by pulse_luminari to make sure we don't have extraneous funky
   grappling situations */
void grapple_cleanup(struct char_data *ch) {

  /* same room? */

  /* position check */

  /* has appropriate variables/flags for grappling scenario */
}

/* primary grapple and reversal entry point */
ACMD(do_grapple) {
  send_to_char(ch, "Under construction.\r\n");
  return;
}

/* as a free action, attempt to free oneself from grapple */
ACMD(do_struggle) {
  send_to_char(ch, "Under construction.\r\n");
  return;
}

/* as a free action, release your grapple victim */
ACMD(do_free_grapple) {
  send_to_char(ch, "Under construction.\r\n");
  return;
}


/*EOF*/
