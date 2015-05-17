/* ***********************************************************************
 *    File:   domain_powers.c                        Part of LuminariMUD  *
 * Purpose:   The code for handling all the individual domain powers      *
 *  Author:   Zusuk                                                       *
 *  Header:   domains_schools.h                                           *
 ************************************************************************ */

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "interpreter.h"
#include "db.h"
#include "spells.h"
#include "domains_schools.h"
#include "assign_wpn_armor.h"
#include "screen.h"


ACMD(do_lightningarc) {
  if (!has_domain_power(ch, DOMAIN_POWER_LIGHTNING_ARC)) {
    send_to_char(ch, "You do not have that domain power!\r\n");
    return;
  }

}

ACMD(do_aciddart) {
  if (!has_domain_power(ch, DOMAIN_POWER_ACID_DART)) {
    send_to_char(ch, "You do not have that domain power!\r\n");
    return;
  }

}

ACMD(do_firebolt) {
  if (!has_domain_power(ch, DOMAIN_POWER_FIRE_BOLT)) {
    send_to_char(ch, "You do not have that domain power!\r\n");
    return;
  }

}

ACMD(do_icicle) {
  if (!has_domain_power(ch, DOMAIN_POWER_ICICLE)) {
    send_to_char(ch, "You do not have that domain power!\r\n");
    return;
  }

}
