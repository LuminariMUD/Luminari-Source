/**************************************************************************
 *  File: mob_class.c                                 Part of LuminariMUD *
 *  Usage: Mobile class-specific behavior functions                       *
 *                                                                         *
 *  All rights reserved.  See license for complete information.           *
 *                                                                         *
 *  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
 *  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.              *
 **************************************************************************/

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "db.h"
#include "handler.h"
#include "spells.h"
#include "constants.h"
#include "act.h"
#include "fight.h"
#include "evolutions.h"  /* for EVOLUTION_UNDEAD_APPEARANCE */
#include "mob_utils.h"
#include "mob_race.h"
#include "mob_psionic.h"
#include "mob_spells.h"
#include "mob_class.h"

void npc_ability_behave(struct char_data *ch)
{
  if (dice(1, 2) == 1)
    npc_racial_behave(ch);
  else
  {
    if (GET_CLASS(ch) == CLASS_PSIONICIST)
      npc_offensive_powers(ch);
    else
      npc_offensive_spells(ch);
  }
  return;

  // we need to code the abilities before we go ahead with this
  struct char_data *vict = NULL;
  int num_targets = 0;

  if (!can_continue(ch, TRUE))
    return;

  /* retrieve random valid target and number of targets */
  if (!(vict = npc_find_target(ch, &num_targets)))
    return;
}

// monk behaviour, behave based on level

void npc_monk_behave(struct char_data *ch, struct char_data *vict,
                     int engaged)
{

  /* list of skills to use:
   1) switch opponents
   2) springleap
   3) stunning fist
   4) quivering palm
   */

  if (!can_continue(ch, TRUE))
    return;

  /* switch opponents attempt */
  if (!rand_number(0, 2) && npc_switch_opponents(ch, vict))
    return;

  switch (rand_number(1, 6))
  {
  case 1:
    perform_stunningfist(ch);
    break;
  case 2:
  case 3:
  case 4:
  case 5:
  case 6:
    perform_springleap(ch, vict);
    break;
  case 7: /* hahah just kidding */
    perform_quiveringpalm(ch);
    break;
  default:
    break;
  }
}
// rogue behaviour, behave based on level

void npc_rogue_behave(struct char_data *ch, struct char_data *vict,
                      int engaged)
{

  /* almost finished victims, they will stop using these skills -zusuk */
  if (GET_HIT(vict) <= 5)
    return;

  /* list of skills to use:
   1) trip
   2) dirt kick
   3) sap  //todo
   4) backstab / circle
   */
  if (GET_LEVEL(ch) >= 2 && !HAS_FEAT(ch, FEAT_SNEAK_ATTACK))
  {
    MOB_SET_FEAT(ch, FEAT_SNEAK_ATTACK, (GET_LEVEL(ch)) / 2);
  }

  if (!can_continue(ch, TRUE))
    return;

  switch (rand_number(1, 2))
  {
  case 1:
    if (perform_knockdown(ch, vict, SKILL_TRIP, true, true))
      break;
    /* fallthrough */
  case 2:
    if (perform_dirtkick(ch, vict))
    {
      send_to_char(ch, "Succeeded dirtkick\r\n");
      break;
    }
    else
      send_to_char(ch, "Failed dirtkick\r\n");
    /* fallthrough */
  default:
    if (perform_backstab(ch, vict))
      break;
    break;
  }
}
// bard behaviour, behave based on level

void npc_bard_behave(struct char_data *ch, struct char_data *vict,
                     int engaged)
{

  /* list of skills to use:
   1) trip
   2) dirt kick
   3) perform
   4) kick
   */
  /* try to throw up song */
  perform_perform(ch);

  if (!can_continue(ch, TRUE))
    return;

  switch (rand_number(1, 3))
  {
  case 1:
    perform_knockdown(ch, vict, SKILL_TRIP, true, true);
    break;
  case 2:
    perform_dirtkick(ch, vict);
    break;
  case 3:
    perform_kick(ch, vict);
    break;
  default:
    break;
  }
}
// warrior behaviour, behave based on circle

void npc_warrior_behave(struct char_data *ch, struct char_data *vict,
                        int engaged)
{

  /* list of skills to use:
   1) rescue
   2) bash
   3) shieldpunch
   4) switch opponents
   */

  /* first rescue friends/master */
  if (npc_rescue(ch))
    return;

  if (!can_continue(ch, TRUE))
    return;

  /* switch opponents attempt */
  if (!rand_number(0, 2) && npc_switch_opponents(ch, vict))
    return;

  switch (rand_number(1, 2))
  {
  case 1:
    if (perform_knockdown(ch, vict, SKILL_BASH, true, true))
      break;
  case 2:
    if (perform_shieldpunch(ch, vict))
      break;
  default:
    break;
  }
}
// ranger behaviour, behave based on level

void npc_ranger_behave(struct char_data *ch, struct char_data *vict,
                       int engaged)
{

  /* list of skills to use:
   1) rescue
   2) switch opponents
   3) call companion
   4) kick
   */

  /* attempt to call companion if appropriate */
  if (npc_should_call_companion(ch, MOB_C_ANIMAL))
    perform_call(ch, MOB_C_ANIMAL, GET_LEVEL(ch));

  /* next rescue friends/master */
  if (npc_rescue(ch))
    return;

  if (!can_continue(ch, TRUE))
    return;

  /* switch opponents attempt */
  if (!rand_number(0, 2) && npc_switch_opponents(ch, vict))
    return;

  perform_kick(ch, vict);
}

// paladin behaviour, behave based on level

void npc_paladin_behave(struct char_data *ch, struct char_data *vict,
                        int engaged)
{
  float percent = ((float)GET_HIT(ch) / (float)GET_MAX_HIT(ch)) * 100.0;

  /* list of skills to use:
   1) call mount
   2) rescue
   3) lay on hands
   4) smite evil
   5) switch opponents
   6) turn undead
   */

  /* attempt to call mount if appropriate */
  if (npc_should_call_companion(ch, MOB_C_MOUNT))
    perform_call(ch, MOB_C_MOUNT, GET_LEVEL(ch));

  /* first rescue friends/master */
  if (npc_rescue(ch))
    return;

  if (!can_continue(ch, TRUE))
    return;

  /* switch opponents attempt */
  if (!rand_number(0, 2) && npc_switch_opponents(ch, vict))
    return;

  if (IS_EVIL(vict))
    perform_smite(ch, SMITE_TYPE_EVIL);

  if (IS_UNDEAD(vict) && GET_LEVEL(ch) > 2)
    perform_turnundead(ch, vict, (GET_LEVEL(ch) - 2));

  if (percent <= 25.0)
    perform_layonhands(ch, ch);
}

// berserk behaviour, behave based on level

void npc_berserker_behave(struct char_data *ch, struct char_data *vict,
                          int engaged)
{

  /* list of skills to use:
   1) rescue
   2) berserk
   3) headbutt
   4) switch opponents
   */

  /* first rescue friends/master */
  if (npc_rescue(ch))
    return;

  if (!can_continue(ch, TRUE))
    return;

  /* switch opponents attempt */
  if (!rand_number(0, 2) && npc_switch_opponents(ch, vict))
    return;

  perform_rage(ch);

  perform_headbutt(ch, vict);
}

/* this is our non-caster's entry point in combat AI
 all semi-casters such as ranger/paladin will go through here */
void npc_class_behave(struct char_data *ch)
{
  struct char_data *vict = NULL;
  int num_targets = 0;

  if (MOB_FLAGGED(ch, MOB_NOCLASS))
    return;

  /* retrieve random valid target and number of targets */
  if (!(vict = npc_find_target(ch, &num_targets)))
    return;

  switch (GET_CLASS(ch))
  {
  case CLASS_BARD:
    npc_bard_behave(ch, vict, num_targets);
    break;
  case CLASS_BERSERKER:
    npc_berserker_behave(ch, vict, num_targets);
    break;
  case CLASS_PALADIN:
  case CLASS_BLACKGUARD:
    npc_paladin_behave(ch, vict, num_targets);
    break;
  case CLASS_RANGER:
    npc_ranger_behave(ch, vict, num_targets);
    break;
  case CLASS_ROGUE:
    npc_rogue_behave(ch, vict, num_targets);
    break;
  case CLASS_MONK:
    npc_monk_behave(ch, vict, num_targets);
    break;
  case CLASS_WARRIOR:
  case CLASS_WEAPON_MASTER: /*todo!*/
  default:
    npc_warrior_behave(ch, vict, num_targets);
    break;
  }
}