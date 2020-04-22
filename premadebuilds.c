/*
 * Premade build code for LuminariMUD by Gicker aka Steve Squires
 */

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "db.h"
#include "comm.h"
#include "modify.h"
#include "screen.h"
#include "spells.h"
#include "handler.h"
#include "interpreter.h"
#include "class.h"
#include "race.h"
#include "spec_procs.h"  // for compute_ability
#include "mud_event.h"  // for purgemob event
#include "feats.h"
#include "spec_abilities.h"
#include "assign_wpn_armor.h"
#include "wilderness.h"
#include "domains_schools.h"
#include "constants.h"
#include "dg_scripts.h"
#include "templates.h"
#include "oasis.h"
