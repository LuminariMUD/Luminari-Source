/* *************************************************************************
 *   File: combat_modes.c                              Part of LuminariMUD *
 *  Usage: Source file for combat modes.                                   *
 * Author: Ornir                                                           *
 ***************************************************************************
 * In Luminari, certain feats and classes allow the use of 'combat modes', *
 * a mode where the char gains certain bonuses and have certain penalties. *
 * A good example of this is power attack - Gain a bonus in damage by      *
 * taking a penalty to you attacks.  These modes can be changed during     *
 * combat, increasing the versatility of the fighter classes and providing *
 * additional options for the fighting man.                                *
 *                                                                         *
 ***************************************************************************/
#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "feats.h"
#include "comm.h"
#include "interpreter.h"
#include "handler.h"
#include "db.h" /* For dummy_mob */
#include "spells.h"
#include "class.h" /* For BAB() */
#include "mud_event.h"
#include "combat_modes.h"

/* Modes that cannot be overlapped:
 *
 *           - Power attack
 *  Group 1 |  Combat expertise
 *           - Spellbattle
 *
 *           - Two-Weapon Fighting
 *  Group 2 |  Flurry of Blows
 *           - Rapidshot
 *
 *           - Counterspell
 *  Group 3 |  Defensive casting
 *           - Auto * spell (?)
 *
 *
 *  Modes in the same group can not overlap, modes from different groups
 *  CAN overlap.
 *
 *  Group 1 MODIFIES attacks
 *  Group 2 ADDS attacks
 *  Group 3 MODIFIES spellcasting
 *
 *  As a rule, changing modes does not take an action.  This does not mean that
 *  changing modes is instantaneous, and there may be changes to only allow
 *  changing modes in group 2 at the beginning of combat rounds.  This is not
 *  implemented yet.
 */

struct combat_mode_data combat_mode_info[] = {
    {"!UNDEFINED!", 0, 0, FALSE, MODE_GROUP_NONE},
    /* Group 1 */
    {"power attack", AFF_POWER_ATTACK, FEAT_POWER_ATTACK, TRUE, MODE_GROUP_1},
    {"combat expertise", AFF_EXPERTISE, FEAT_COMBAT_EXPERTISE, TRUE, MODE_GROUP_1},
    {"spellbattle", AFF_SPELLBATTLE, FEAT_SPELLBATTLE, TRUE, MODE_GROUP_1},
    {"total defense", AFF_TOTAL_DEFENSE, FEAT_UNDEFINED, FALSE, MODE_GROUP_1},
    /* Group 2 */
    {"dual wield", AFF_DUAL_WIELD, FEAT_UNDEFINED, FALSE, MODE_GROUP_2},
    {"flurry of blows", AFF_FLURRY_OF_BLOWS, FEAT_FLURRY_OF_BLOWS, FALSE, MODE_GROUP_2},
    {"rapid shot", AFF_RAPID_SHOT, FEAT_RAPID_SHOT, FALSE, MODE_GROUP_2},
    /* Group 3 */
    /* none currently */
    /* No Group */
    {"counterspell", AFF_COUNTERSPELL, FEAT_UNDEFINED, FALSE, MODE_GROUP_NONE},
    {"defensive casting", AFF_DEFENSIVE_CASTING, FEAT_UNDEFINED, FALSE, MODE_GROUP_NONE},
    {"whirlwind attack", AFF_WHIRLWIND_ATTACK, FEAT_WHIRLWIND_ATTACK, MODE_GROUP_NONE}};

/* Unified combat mode management */
bool is_mode_enabled(const struct char_data *ch, const int mode)
{
  if (AFF_FLAGGED(ch, combat_mode_info[mode].affect_flag))
  {
    return TRUE;
  }
  else
  {
    return FALSE;
  }
}

int is_mode_blocked(const struct char_data *ch, const int mode)
{
  int i = 0;

  if (mode < 0 || mode >= MAX_MODES)
  {
    log("SYSERR: Invalid mode '%d' passed to is_mode_blocked()", mode);
    return MODE_NONE;
  }

  /* Check the modes groups.  Modes set to MODE_GROUP_NONE do not ever block. */
  for (i = 1; i < MAX_MODES; i++)
  {
    if ((i != MODE_GROUP_NONE) &&
        (combat_mode_info[mode].group == combat_mode_info[i].group) &&
        is_mode_enabled(ch, i))
    {
      return i;
    }
  }
  return MODE_NONE;
}

bool can_enable_mode(struct char_data *ch, const int mode)
{
  if ((is_mode_blocked(ch, mode)) ||
      (!HAS_FEAT(ch, combat_mode_info[mode].required_feat)))
  {
    return FALSE;
  }
  else
  {
    return TRUE;
  }
}

void enable_combat_mode(struct char_data *ch, const int mode, const int value)
{
  //  if ( can_enable_mode(ch, mode) ) {
  SET_BIT_AR(AFF_FLAGS(ch), combat_mode_info[mode].affect_flag);
  if (combat_mode_info[mode].has_value)
  {
    COMBAT_MODE_VALUE(ch) = value;
  }
  //  }
}

void disable_combat_mode(struct char_data *ch, int mode)
{
  REMOVE_BIT_AR(AFF_FLAGS(ch), combat_mode_info[mode].affect_flag);
}

/* Generic mode manager */
ACMD(do_mode)
{

  char arg[MAX_INPUT_LENGTH] = {'\0'};
  int number = -1;
  int blocking_mode = 0;
  int mode = subcmd;

  if (argument)
  {
    one_argument(argument, arg);
  }
  if (is_mode_enabled(ch, mode) &&
      ((combat_mode_info[mode].has_value == FALSE) ||
       ((combat_mode_info[mode].has_value == TRUE) &&
        (!*arg))))
  {
    send_to_char(ch, "You leave %s mode.\r\n", combat_mode_info[mode].name);
    disable_combat_mode(ch, mode);
    if (combat_mode_info[mode].has_value)
    {
      COMBAT_MODE_VALUE(ch) = -1;
    }
    return;
  }
  if ((combat_mode_info[mode].required_feat != FEAT_UNDEFINED) &&
      (!HAS_FEAT(ch, combat_mode_info[mode].required_feat)))
  {
    send_to_char(ch, "You have no idea how to do that.\r\n");
    return;
  }
  if (((blocking_mode = is_mode_blocked(ch, mode)) != MODE_NONE) &&
      (blocking_mode != mode))
  {
    /* There is a blocking mode */
    send_to_char(ch, "You can't combine %s and %s modes!\r\n", combat_mode_info[mode].name, combat_mode_info[blocking_mode].name);
    return;
  }
  if (combat_mode_info[mode].has_value)
  {
    /* No argument */
    if (!*arg)
    {
      send_to_char(ch, "You must specify an argument when entering %s mode.\r\n", combat_mode_info[mode].name);
      return;
    }

    if (IS_NPC(ch))
    {
      number = 5;
    }
    else if (is_number(arg))
      number = atoi(arg);
    else
    {
      send_to_char(ch, "The argument must be a number!\r\n");
      return;
    }

    if (number == 1)
      ;
    else if (!IS_NPC(ch) && number > MODE_CAP)
    {
      send_to_char(ch, "The maximum value you can specify for %s is %d (mode cap).\r\n", combat_mode_info[mode].name, MODE_CAP);
      return;
    }
    else if (!IS_NPC(ch) && number > BAB(ch))
    {
      send_to_char(ch, "Mode %s is limited to %d - your base attack bonus (BAB).\r\n", combat_mode_info[mode].name, BAB(ch));
      return;
    }
    else if (number < 1)
    {
      send_to_char(ch, "The minimum value you can specify for %s is 1.\r\n", combat_mode_info[mode].name);
      return;
    }
  }
  if (!is_mode_enabled(ch, mode))
  {
    send_to_char(ch, "You enter %s mode.\r\n", combat_mode_info[mode].name);
  }
  else if (combat_mode_info[mode].has_value)
  {
    send_to_char(ch, "You change the value for %s mode to %d.\r\n", combat_mode_info[mode].name, number);
  }
  else
  {
    send_to_char(ch, "You are already in %s mode!\r\n", combat_mode_info[mode].name);
    return;
  }
  enable_combat_mode(ch, mode, number);
}

#define SPELLBATTLE_CAP 12
#define SPELLBATTLE_AFFECTS 4

ACMD(do_spellbattle)
{
  char arg[MAX_INPUT_LENGTH] = {'\0'};
  int number = -1, cap = SPELLBATTLE_CAP, duration = 1, i = 0;
  struct affected_type af[SPELLBATTLE_AFFECTS];

  if (IS_NPC(ch) || (!IS_NPC(ch) && GET_RACE(ch) != RACE_ARCANA_GOLEM))
  {
    send_to_char(ch, "You have no idea how to do that.\r\n");
    return;
  }

  if (AFF_FLAGGED(ch, AFF_EXPERTISE) ||
      AFF_FLAGGED(ch, AFF_TOTAL_DEFENSE) ||
      AFF_FLAGGED(ch, AFF_POWER_ATTACK))
  {
    send_to_char(ch, "You can't be in a combat-mode and enter "
                     "spell-battle.\r\n");
    return;
  }

  if (argument)
    one_argument(argument, arg);

  /* OK no argument, that means we're attempting to turn it off */
  if (!*arg)
  {
    if (AFF_FLAGGED(ch, AFF_SPELLBATTLE) &&
        !char_has_mud_event(ch, eSPELLBATTLE))
    {
      REMOVE_BIT_AR(AFF_FLAGS(ch), AFF_SPELLBATTLE);
      send_to_char(ch, "You leave 'spellbattle' mode.\r\n");
      SPELLBATTLE(ch) = 0;
      return;
    }
    if (char_has_mud_event(ch, eSPELLBATTLE))
    {
      send_to_char(ch, "You can't exit spellbattle until the cooldown wears"
                       " off.\r\n");
      return;
    }
    else
    {
      send_to_char(ch, "Spellbattle requires an argument to be activated!\r\n");
      return;
    }
  }

  if (char_has_mud_event(ch, eSPELLBATTLE) ||
      affected_by_spell(ch, SKILL_SPELLBATTLE))
  {
    send_to_char(ch, "You are already in spellbattle mode!\r\n");
    return;
  }

  /* ok we have an arg, lets make sure its valid */
  if (is_number(arg))
    number = atoi(arg);
  else
  {
    send_to_char(ch, "The argument needs to be a number!\r\n");
    return;
  }

  /* we have a cap of SPELLBATTLE_CAP or BAB or CASTER_LEVEL, whatever is
   lowest */
  if (cap > BAB(ch))
    cap = BAB(ch);
  if (cap > CASTER_LEVEL(ch))
    cap = CASTER_LEVEL(ch);

  if (number > cap)
  {
    send_to_char(ch, "Your maximum spellbattle level is %d!\r\n", cap);
    return;
  }

  /* passed all the tests, we should have a valid number for spellbattle */
  SPELLBATTLE(ch) = number;
  send_to_char(ch, "You are now in 'spellbattle' mode.\r\n");
  duration = (1 * SECS_PER_REAL_HOUR) / PULSE_VIOLENCE; // this should match our event duration

  /* init affect array */
  for (i = 0; i < SPELLBATTLE_AFFECTS; i++)
  {
    new_affect(&(af[i]));
    af[i].spell = SKILL_SPELLBATTLE;
    af[i].duration = duration;
  }

  af[0].location = APPLY_INT;
  af[0].modifier = -2;
  af[1].location = APPLY_WIS;
  af[1].modifier = -2;
  af[2].location = APPLY_CHA;
  af[2].modifier = -2;
  af[3].location = APPLY_HIT;
  af[3].modifier = SPELLBATTLE(ch) * 10;

  for (i = 0; i < SPELLBATTLE_AFFECTS; i++)
    affect_join(ch, af + i, FALSE, FALSE, FALSE, FALSE);

  SET_BIT_AR(AFF_FLAGS(ch), AFF_SPELLBATTLE);
  attach_mud_event(new_mud_event(eSPELLBATTLE, ch, NULL),
                   1 * SECS_PER_REAL_HOUR);
}

#undef SPELLBATTLE_CAP
#undef SPELLBATTLE_AFFECTS
#undef MODE_CAP
