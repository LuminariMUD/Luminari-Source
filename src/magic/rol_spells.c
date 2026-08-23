/**************************************************************************
 *  File: rol_spells.c                                 Part of LuminariMUD *
 *  Usage: Spells converted from Realms of Luminari.                       *
 *                                                                         *
 *  All rights reserved.  See license for complete information.            *
 **************************************************************************/

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "handler.h"
#include "db.h"
#include "interpreter.h"
#include "combat/fight.h"
#include "character/evolutions.h"
#include "domains_schools.h"
#include "spells.h"
#include "rol_spells.h"

#define ROL_NO_AFFECT_FLAG (-1)

static int rol_spell_duration(int level, int minimum, int divisor);
static void apply_rol_affect(struct char_data *victim, int spellnum, int duration, int location,
                             int modifier, int affect_flag, int affect2_flag);
static bool resist_rol_spell(struct char_data *ch, struct char_data *victim, int level,
                             int casttype, int save, int school);
static bool can_adjust_age(struct char_data *ch, struct char_data *victim);
static void adjust_age_years(struct char_data *victim, int years);
static void cast_farsee(int level, struct char_data *ch);
static void cast_rejuvenate_major(struct char_data *ch, struct char_data *victim);
static void cast_rejuvenate_minor(int level, struct char_data *ch, struct char_data *victim);
static void cast_age(struct char_data *ch, struct char_data *victim);
static void cast_command_undead(int level, struct char_data *ch, struct char_data *victim,
                                int casttype);
static void cast_command_horde(int level, struct char_data *ch, int casttype);
static void cast_slow_poison(int level, struct char_data *victim);
static void cast_comprehend_languages(int level, struct char_data *victim);
static void cast_fumble(int level, struct char_data *ch, struct char_data *victim, int casttype);
static void cast_stumble(int level, struct char_data *ch, struct char_data *victim, int casttype);
static void cast_enervate(int level, struct char_data *ch, struct char_data *victim, int casttype);
static void cast_protect_undead(int level, struct char_data *victim);
static void cast_protection_from_undead(int level, struct char_data *victim);
static void cast_ancestral_shield(int level, struct char_data *ch);
static void cast_protection_from_animals(int level, struct char_data *victim);
static void cast_pass_without_trace(int level, struct char_data *ch);
static void cast_greater_realm_of_protection(int level, struct char_data *victim);
static void stop_fights_with(struct char_data *victim);
static void cast_feign_death(int level, struct char_data *victim);
static void cast_tranquility(int level, struct char_data *ch);
static void cast_agility(int level, struct char_data *victim);
static void cast_natures_blessing(int level, struct char_data *ch);
static void cast_song_of_travel(int level, struct char_data *ch);

static int rol_spell_duration(int level, int minimum, int divisor)
{
  if (divisor < 1)
    divisor = 1;
  return MAX(minimum, level / divisor);
}

static void apply_rol_affect(struct char_data *victim, int spellnum, int duration, int location,
                             int modifier, int affect_flag, int affect2_flag)
{
  struct affected_type af;

  if (victim == NULL)
    return;

  new_affect(&af);
  af.spell = spellnum;
  af.duration = MAX(1, duration);
  af.location = location;
  af.modifier = modifier;
  if (affect_flag > AFF_DONTUSE)
    SET_BIT_AR(af.bitvector, affect_flag);
  if (affect2_flag > AFF2_DONTUSE)
    SET_BIT_AR(af.bitvector2, affect2_flag);
  affect_join(victim, &af, FALSE, FALSE, FALSE, FALSE);
}

static bool resist_rol_spell(struct char_data *ch, struct char_data *victim, int level,
                             int casttype, int save, int school)
{
  if (ch == NULL || victim == NULL)
    return TRUE;

  if (mag_resistance(ch, victim, 0) || savingthrow(ch, victim, save, 0, casttype, level, school))
  {
    act("$N resists your spell.", FALSE, ch, NULL, victim, TO_CHAR);
    act("You resist $n's spell.", FALSE, ch, NULL, victim, TO_VICT);
    return TRUE;
  }

  return FALSE;
}

static bool can_adjust_age(struct char_data *ch, struct char_data *victim)
{
  if (ch == NULL || victim == NULL || IS_NPC(victim))
  {
    if (ch != NULL)
      send_to_char(ch, "That spell can only alter a player character's age.\r\n");
    return FALSE;
  }

  if (ch != victim && !is_player_grouped(ch, victim))
  {
    send_to_char(ch, "%s must be grouped with you to accept that lasting change.\r\n",
                 GET_NAME(victim));
    return FALSE;
  }

  return TRUE;
}

static void adjust_age_years(struct char_data *victim, int years)
{
  time_t now;
  time_t delta;

  if (victim == NULL || IS_NPC(victim) || years == 0)
    return;

  now = time(NULL);
  delta = (time_t)years * SECS_PER_MUD_YEAR;
  if (years > 0)
    victim->player.time.birth -= delta;
  else
  {
    victim->player.time.birth -= delta;
    if (victim->player.time.birth > now)
      victim->player.time.birth = now;
  }
}

static void cast_farsee(int level, struct char_data *ch)
{
  if (ch == NULL)
    return;

  apply_rol_affect(ch, SPELL_FARSEE, MAX(4, level * 2), APPLY_NONE, 0, AFF_FARSEE,
                   ROL_NO_AFFECT_FLAG);
  send_to_char(ch, "Your vision sharpens and reaches far beyond the horizon.\r\n");
}

static void cast_rejuvenate_major(struct char_data *ch, struct char_data *victim)
{
  int years;

  if (!can_adjust_age(ch, victim))
    return;

  years = dice(1, 3);
  adjust_age_years(victim, -years);
  send_to_char(victim, "Warmth settles into your bones as %d year%s fall away.\r\n", years,
               years == 1 ? "" : "s");
  if (ch != victim)
    act("$N looks subtly younger.", FALSE, ch, NULL, victim, TO_CHAR);
}

static void cast_rejuvenate_minor(int level, struct char_data *ch, struct char_data *victim)
{
  int years;

  if (ch == NULL || victim == NULL)
    return;
  if (ch != victim && !is_player_grouped(ch, victim))
  {
    send_to_char(ch, "That spell can only be shared with a group member.\r\n");
    return;
  }

  years = MAX(1, dice(2, MAX(1, level)) / 2);
  apply_rol_affect(victim, SPELL_REJUVENATE_MINOR, MAX(4, level), APPLY_AGE, -years,
                   ROL_NO_AFFECT_FLAG, ROL_NO_AFFECT_FLAG);
  send_to_char(victim, "You feel younger, though the change is only temporary.\r\n");
}

static void cast_age(struct char_data *ch, struct char_data *victim)
{
  int years;

  if (!can_adjust_age(ch, victim))
    return;

  years = dice(2, 8);
  adjust_age_years(victim, years);
  send_to_char(victim, "A sudden weight settles on you as you age %d years.\r\n", years);
  if (ch != victim)
    act("$N looks noticeably older.", FALSE, ch, NULL, victim, TO_CHAR);
}

static void cast_command_undead(int level, struct char_data *ch, struct char_data *victim,
                                int casttype)
{
  if (ch == NULL || victim == NULL)
    return;
  if (!IS_NPC(victim) || !IS_UNDEAD(victim))
  {
    send_to_char(ch, "Only an undead creature can be commanded by this spell.\r\n");
    return;
  }
  if (GET_LEVEL(victim) > level)
  {
    act("$N is too powerful for you to command.", FALSE, ch, NULL, victim, TO_CHAR);
    return;
  }

  effect_charm(ch, victim, SPELL_COMMAND_UNDEAD, casttype, level);
}

static void cast_command_horde(int level, struct char_data *ch, int casttype)
{
  struct char_data *victim;
  struct char_data *next_victim;
  int commanded;

  if (ch == NULL || IN_ROOM(ch) == NOWHERE)
    return;

  commanded = 0;
  for (victim = world[IN_ROOM(ch)].people; victim; victim = next_victim)
  {
    next_victim = victim->next_in_room;
    if (victim == ch || !IS_NPC(victim) || !IS_UNDEAD(victim) || GET_LEVEL(victim) > level ||
        !aoeOK(ch, victim, SPELL_COMMAND_HORDE))
      continue;
    cast_command_undead(level, ch, victim, casttype);
    if (victim->master == ch)
      commanded++;
  }

  if (commanded == 0)
    send_to_char(ch, "No undead in the area submit to your command.\r\n");
}

static void cast_slow_poison(int level, struct char_data *victim)
{
  if (victim == NULL)
    return;

  apply_rol_affect(victim, SPELL_SLOW_POISON, rol_spell_duration(level, 4, 2), APPLY_NONE, 0,
                   ROL_NO_AFFECT_FLAG, AFF2_ROL_SLOW_POISON);
  send_to_char(victim, "Your pulse steadies as poisons begin moving more slowly.\r\n");
}

static void cast_comprehend_languages(int level, struct char_data *victim)
{
  if (victim == NULL)
    return;

  apply_rol_affect(victim, SPELL_COMPREHEND_LANGUAGES, MAX(4, level * 2), APPLY_NONE, 0,
                   ROL_NO_AFFECT_FLAG, ROL_NO_AFFECT_FLAG);
  send_to_char(victim, "Unfamiliar languages begin to make sense to you.\r\n");
}

static void cast_fumble(int level, struct char_data *ch, struct char_data *victim, int casttype)
{
  int penalty;

  if (victim == NULL || resist_rol_spell(ch, victim, level, casttype, SAVING_WILL, ENCHANTMENT))
    return;

  penalty = MIN(0, 1 - GET_REAL_DEX(victim));
  apply_rol_affect(victim, SPELL_FUMBLE, rol_spell_duration(level, 3, 4), APPLY_DEX, penalty,
                   ROL_NO_AFFECT_FLAG, ROL_NO_AFFECT_FLAG);
  act("Your hands become impossibly clumsy.", FALSE, ch, NULL, victim, TO_VICT);
  act("$N fumbles as precise movement deserts $M.", FALSE, ch, NULL, victim, TO_CHAR);
}

static void cast_stumble(int level, struct char_data *ch, struct char_data *victim, int casttype)
{
  int duration;

  if (victim == NULL || resist_rol_spell(ch, victim, level, casttype, SAVING_WILL, ENCHANTMENT))
    return;

  duration = rol_spell_duration(level, 3, 4);
  apply_rol_affect(victim, SPELL_STUMBLE, duration, APPLY_AC_NEW, -4, ROL_NO_AFFECT_FLAG,
                   ROL_NO_AFFECT_FLAG);
  apply_rol_affect(victim, SPELL_STUMBLE, duration, APPLY_SAVING_REFL, -4, ROL_NO_AFFECT_FLAG,
                   ROL_NO_AFFECT_FLAG);
  apply_rol_affect(victim, SPELL_STUMBLE, duration, APPLY_INITIATIVE, -4, AFF_STAGGERED,
                   ROL_NO_AFFECT_FLAG);
  act("Your balance fails and every step becomes uncertain.", FALSE, ch, NULL, victim, TO_VICT);
  act("$N staggers as $S balance deserts $M.", FALSE, ch, NULL, victim, TO_CHAR);
}

static void cast_enervate(int level, struct char_data *ch, struct char_data *victim, int casttype)
{
  int penalty;

  if (victim == NULL || resist_rol_spell(ch, victim, level, casttype, SAVING_FORT, NECROMANCY))
    return;

  penalty = MIN(0, 1 - GET_REAL_CON(victim));
  apply_rol_affect(victim, SPELL_ENERVATE, rol_spell_duration(level, 3, 4), APPLY_CON, penalty,
                   ROL_NO_AFFECT_FLAG, ROL_NO_AFFECT_FLAG);
  act("Your vitality drains away, leaving you frighteningly frail.", FALSE, ch, NULL, victim,
      TO_VICT);
  act("$N pales as $S vitality drains away.", FALSE, ch, NULL, victim, TO_CHAR);
}

static void cast_protect_undead(int level, struct char_data *victim)
{
  int duration;

  if (victim == NULL)
    return;
  if (!IS_UNDEAD(victim))
  {
    send_to_char(victim, "The ward finds no undead essence to protect.\r\n");
    return;
  }

  duration = rol_spell_duration(level, 5, 2);
  apply_rol_affect(victim, SPELL_PROT_UNDEAD, duration, APPLY_AC_NEW, 4, AFF_WARDED,
                   ROL_NO_AFFECT_FLAG);
  apply_rol_affect(victim, SPELL_PROT_UNDEAD, duration, APPLY_SAVING_WILL, 2, ROL_NO_AFFECT_FLAG,
                   ROL_NO_AFFECT_FLAG);
  send_to_char(victim, "A dark ward settles around your undead form.\r\n");
}

static void cast_protection_from_undead(int level, struct char_data *victim)
{
  int duration;

  if (victim == NULL)
    return;

  duration = rol_spell_duration(level, 5, 2);
  apply_rol_affect(victim, SPELL_PROT_FROM_UNDEAD, duration, APPLY_AC_NEW, 2, AFF_WARDED,
                   ROL_NO_AFFECT_FLAG);
  apply_rol_affect(victim, SPELL_PROT_FROM_UNDEAD, duration, APPLY_SAVING_WILL, 2,
                   ROL_NO_AFFECT_FLAG, ROL_NO_AFFECT_FLAG);
  send_to_char(victim, "A pale ward rises between you and the undead.\r\n");
}

static void cast_ancestral_shield(int level, struct char_data *ch)
{
  struct char_data *victim;

  if (ch == NULL || IN_ROOM(ch) == NOWHERE)
    return;

  for (victim = world[IN_ROOM(ch)].people; victim; victim = victim->next_in_room)
  {
    if (!is_player_grouped(ch, victim))
      continue;
    apply_rol_affect(victim, SPELL_ANCESTRAL_SHIELD, rol_spell_duration(level, 2, 10), APPLY_NONE,
                     0, ROL_NO_AFFECT_FLAG, ROL_NO_AFFECT_FLAG);
    send_to_char(victim, "Ancestral spirits gather into a shimmering shield around you.\r\n");
  }
}

static void cast_protection_from_animals(int level, struct char_data *victim)
{
  int duration;

  if (victim == NULL)
    return;

  duration = rol_spell_duration(level, 5, 2);
  apply_rol_affect(victim, SPELL_PROTECTION_FROM_ANIMALS, duration, APPLY_AC_NEW, 2, AFF_WARDED,
                   ROL_NO_AFFECT_FLAG);
  apply_rol_affect(victim, SPELL_PROTECTION_FROM_ANIMALS, duration, APPLY_SAVING_REFL, 2,
                   ROL_NO_AFFECT_FLAG, ROL_NO_AFFECT_FLAG);
  send_to_char(victim, "A primal ward rises between you and hostile beasts.\r\n");
}

static void cast_pass_without_trace(int level, struct char_data *ch)
{
  if (ch == NULL)
    return;

  apply_rol_affect(ch, SPELL_PASS_WITHOUT_TRACE, rol_spell_duration(level, 4, 2), APPLY_NONE, 0,
                   AFF_NOTRACK, ROL_NO_AFFECT_FLAG);
  send_to_char(ch, "Your passage ceases to leave any trail.\r\n");
}

static void cast_greater_realm_of_protection(int level, struct char_data *victim)
{
  static const int resistance_applies[] = {
      APPLY_RES_FIRE,  APPLY_RES_COLD, APPLY_RES_AIR,
      APPLY_RES_EARTH, APPLY_RES_ACID, APPLY_RES_ELECTRIC,
  };
  size_t index;
  int duration;
  int resistance;

  if (victim == NULL)
    return;

  duration = rol_spell_duration(level, 5, 2);
  resistance = 10 + MIN(20, level / 2);
  for (index = 0; index < sizeof(resistance_applies) / sizeof(resistance_applies[0]); index++)
    apply_rol_affect(victim, SPELL_GREATER_REALM_OF_PROTECTION, duration, resistance_applies[index],
                     resistance, ROL_NO_AFFECT_FLAG, ROL_NO_AFFECT_FLAG);
  send_to_char(victim, "Layered wards shield you from every elemental realm.\r\n");
}

static void stop_fights_with(struct char_data *victim)
{
  struct char_data *person;
  struct char_data *next_person;

  if (victim == NULL || IN_ROOM(victim) == NOWHERE)
    return;

  for (person = world[IN_ROOM(victim)].people; person; person = next_person)
  {
    next_person = person->next_in_room;
    if (FIGHTING(person) == victim)
      stop_fighting(person);
  }
  if (FIGHTING(victim) != NULL)
    stop_fighting(victim);
}

static void cast_feign_death(int level, struct char_data *victim)
{
  if (victim == NULL)
    return;

  stop_fights_with(victim);
  apply_rol_affect(victim, SPELL_FEIGN_DEATH, rol_spell_duration(level, 2, 5), APPLY_NONE, 0,
                   AFF_REFUGE, ROL_NO_AFFECT_FLAG);
  send_to_char(victim, "Your breath stills and you take on the semblance of death.\r\n");
  act("$n goes utterly still, showing no sign of life.", FALSE, victim, NULL, NULL, TO_ROOM);
}

static void cast_tranquility(int level, struct char_data *ch)
{
  struct char_data *victim;
  struct char_data *next_victim;
  int duration;

  if (ch == NULL || IN_ROOM(ch) == NOWHERE)
    return;

  duration = rol_spell_duration(level, 2, 10);
  for (victim = world[IN_ROOM(ch)].people; victim; victim = next_victim)
  {
    next_victim = victim->next_in_room;
    if (victim != ch && !is_player_grouped(ch, victim) && !aoeOK(ch, victim, SPELL_TRANQUILITY))
      continue;
    stop_fights_with(victim);
    apply_rol_affect(victim, SPELL_TRANQUILITY, duration, APPLY_NONE, 0, ROL_NO_AFFECT_FLAG,
                     AFF2_ROL_DOCILE);
  }
  send_to_room(IN_ROOM(ch), "A profound tranquility settles over the area.\r\n");
}

static void cast_agility(int level, struct char_data *victim)
{
  int duration;

  if (victim == NULL)
    return;

  duration = MAX(4, level);
  apply_rol_affect(victim, SPELL_AGILITY, duration, APPLY_AC_NEW, 4, ROL_NO_AFFECT_FLAG,
                   ROL_NO_AFFECT_FLAG);
  apply_rol_affect(victim, SPELL_AGILITY, duration, APPLY_SAVING_REFL, 4, ROL_NO_AFFECT_FLAG,
                   ROL_NO_AFFECT_FLAG);
  apply_rol_affect(victim, SPELL_AGILITY, duration, APPLY_INITIATIVE, 4, ROL_NO_AFFECT_FLAG,
                   ROL_NO_AFFECT_FLAG);
  send_to_char(victim, "Your balance and reactions become supernaturally agile.\r\n");
}

static void cast_natures_blessing(int level, struct char_data *ch)
{
  int duration;
  int hit_bonus;
  int save_bonus;

  if (ch == NULL)
    return;

  duration = MAX(5, level / 2);
  hit_bonus = level < 35 ? 2 : 3;
  save_bonus = level < 18 ? 3 : (level < 49 ? 4 : 5);
  apply_rol_affect(ch, SPELL_NATURES_BLESSING, duration, APPLY_HITROLL, hit_bonus,
                   ROL_NO_AFFECT_FLAG, ROL_NO_AFFECT_FLAG);
  apply_rol_affect(ch, SPELL_NATURES_BLESSING, duration, APPLY_SAVING_FORT, save_bonus,
                   ROL_NO_AFFECT_FLAG, ROL_NO_AFFECT_FLAG);
  apply_rol_affect(ch, SPELL_NATURES_BLESSING, duration, APPLY_SAVING_REFL, save_bonus,
                   ROL_NO_AFFECT_FLAG, ROL_NO_AFFECT_FLAG);
  apply_rol_affect(ch, SPELL_NATURES_BLESSING, duration, APPLY_SAVING_WILL, save_bonus,
                   ROL_NO_AFFECT_FLAG, ROL_NO_AFFECT_FLAG);
  send_to_char(ch, "Nature's blessing wraps around you like a warm mantle.\r\n");
}

static void cast_song_of_travel(int level, struct char_data *ch)
{
  struct char_data *victim;
  int duration;

  if (ch == NULL || IN_ROOM(ch) == NOWHERE)
    return;

  duration = rol_spell_duration(level, 4, 2);
  for (victim = world[IN_ROOM(ch)].people; victim; victim = victim->next_in_room)
  {
    if (!is_player_grouped(ch, victim))
      continue;
    GET_MOVE(victim) = MIN(GET_MAX_MOVE(victim), GET_MOVE(victim) + MAX(10, level * 2));
    apply_rol_affect(victim, SPELL_SONG_OF_TRAVEL, duration, APPLY_NONE, 0, AFF_FLYING,
                     ROL_NO_AFFECT_FLAG);
    send_to_char(victim,
                 "A traveling melody lightens your feet and lifts you from the ground.\r\n");
  }
}

int rol_spell_adjust_area_damage(struct char_data *victim, int damage)
{
  if (victim == NULL || damage <= 0)
    return MAX(0, damage);

  if (affected_by_spell(victim, SPELL_ANCESTRAL_SHIELD) ||
      affected_by_spell(victim, SPELL_NATURES_BLESSING))
    damage = damage * 3 / 4;

  return MAX(0, damage);
}

int rol_spell_adjust_incoming_damage(struct char_data *attacker, struct char_data *victim,
                                     int damage)
{
  if (attacker == NULL || victim == NULL || damage <= 0 || attacker == victim)
    return MAX(0, damage);

  if ((IS_UNDEAD(attacker) && affected_by_spell(victim, SPELL_PROT_FROM_UNDEAD)) ||
      (IS_ANIMAL(attacker) && affected_by_spell(victim, SPELL_PROTECTION_FROM_ANIMALS)))
    damage = damage * 3 / 4;

  return MAX(0, damage);
}

bool is_rol_gap_spell(int spellnum)
{
  switch (spellnum)
  {
  case SPELL_FARSEE:
  case SPELL_REJUVENATE_MAJOR:
  case SPELL_REJUVENATE_MINOR:
  case SPELL_AGE:
  case SPELL_COMMAND_UNDEAD:
  case SPELL_SLOW_POISON:
  case SPELL_COMPREHEND_LANGUAGES:
  case SPELL_FUMBLE:
  case SPELL_STUMBLE:
  case SPELL_ENERVATE:
  case SPELL_PROT_UNDEAD:
  case SPELL_PROT_FROM_UNDEAD:
  case SPELL_COMMAND_HORDE:
  case SPELL_ANCESTRAL_SHIELD:
  case SPELL_PROTECTION_FROM_ANIMALS:
  case SPELL_PASS_WITHOUT_TRACE:
  case SPELL_GREATER_REALM_OF_PROTECTION:
  case SPELL_FEIGN_DEATH:
  case SPELL_TRANQUILITY:
  case SPELL_AGILITY:
  case SPELL_NATURES_BLESSING:
  case SPELL_SONG_OF_TRAVEL:
    return TRUE;
  default:
    return FALSE;
  }
}

void cast_rol_gap_spell(int spellnum, int level, struct char_data *ch, struct char_data *victim,
                        struct obj_data *obj, int casttype)
{
  (void)obj;

  switch (spellnum)
  {
  case SPELL_FARSEE:
    cast_farsee(level, ch);
    break;
  case SPELL_REJUVENATE_MAJOR:
    cast_rejuvenate_major(ch, victim);
    break;
  case SPELL_REJUVENATE_MINOR:
    cast_rejuvenate_minor(level, ch, victim);
    break;
  case SPELL_AGE:
    cast_age(ch, victim);
    break;
  case SPELL_COMMAND_UNDEAD:
    cast_command_undead(level, ch, victim, casttype);
    break;
  case SPELL_COMMAND_HORDE:
    cast_command_horde(level, ch, casttype);
    break;
  case SPELL_SLOW_POISON:
    cast_slow_poison(level, victim);
    break;
  case SPELL_COMPREHEND_LANGUAGES:
    cast_comprehend_languages(level, victim);
    break;
  case SPELL_FUMBLE:
    cast_fumble(level, ch, victim, casttype);
    break;
  case SPELL_STUMBLE:
    cast_stumble(level, ch, victim, casttype);
    break;
  case SPELL_ENERVATE:
    cast_enervate(level, ch, victim, casttype);
    break;
  case SPELL_PROT_UNDEAD:
    cast_protect_undead(level, victim);
    break;
  case SPELL_PROT_FROM_UNDEAD:
    cast_protection_from_undead(level, victim);
    break;
  case SPELL_ANCESTRAL_SHIELD:
    cast_ancestral_shield(level, ch);
    break;
  case SPELL_PROTECTION_FROM_ANIMALS:
    cast_protection_from_animals(level, victim);
    break;
  case SPELL_PASS_WITHOUT_TRACE:
    cast_pass_without_trace(level, ch);
    break;
  case SPELL_GREATER_REALM_OF_PROTECTION:
    cast_greater_realm_of_protection(level, victim);
    break;
  case SPELL_FEIGN_DEATH:
    cast_feign_death(level, victim);
    break;
  case SPELL_TRANQUILITY:
    cast_tranquility(level, ch);
    break;
  case SPELL_AGILITY:
    cast_agility(level, victim);
    break;
  case SPELL_NATURES_BLESSING:
    cast_natures_blessing(level, ch);
    break;
  case SPELL_SONG_OF_TRAVEL:
    cast_song_of_travel(level, ch);
    break;
  default:
    log("SYSERR: cast_rol_gap_spell called for unsupported spell %d", spellnum);
    break;
  }
}

void assign_rol_gap_spells(void)
{
  spello(SPELL_FARSEE, "farsee", 0, 0, 0, POS_STANDING, TAR_IGNORE, FALSE, MAG_MANUAL,
         "Your far-reaching vision returns to normal.", 5, 5, DIVINATION, FALSE);
  spello(SPELL_REJUVENATE_MAJOR, "rejuvenate major", 0, 0, 0, POS_STANDING, TAR_CHAR_ROOM, FALSE,
         MAG_MANUAL, NULL, 6, 6, TRANSMUTATION, FALSE);
  spello(SPELL_REJUVENATE_MINOR, "rejuvenate minor", 0, 0, 0, POS_STANDING, TAR_CHAR_ROOM, FALSE,
         MAG_MANUAL, "Your apparent youth fades.", 4, 4, TRANSMUTATION, FALSE);
  spello(SPELL_AGE, "age", 0, 0, 0, POS_STANDING, TAR_CHAR_ROOM, FALSE, MAG_MANUAL, NULL, 4, 4,
         TRANSMUTATION, FALSE);
  spello(SPELL_COMMAND_UNDEAD, "command undead", 0, 0, 0, POS_FIGHTING,
         TAR_CHAR_ROOM | TAR_NOT_SELF, TRUE, MAG_MANUAL, "You regain your own will.", 4, 4,
         NECROMANCY, FALSE);
  spello(SPELL_SLOW_POISON, "slow poison", 0, 0, 0, POS_FIGHTING, TAR_CHAR_ROOM, FALSE, MAG_MANUAL,
         "Poisons resume their normal course through your body.", 2, 2, TRANSMUTATION, FALSE);
  spello(SPELL_COMPREHEND_LANGUAGES, "comprehend languages", 0, 0, 0, POS_FIGHTING, TAR_CHAR_ROOM,
         FALSE, MAG_MANUAL, "Unfamiliar languages lose their meaning again.", 4, 4, DIVINATION,
         FALSE);
  spello(SPELL_FUMBLE, "fumble", 0, 0, 0, POS_FIGHTING,
         TAR_CHAR_ROOM | TAR_FIGHT_VICT | TAR_NOT_SELF, TRUE, MAG_MANUAL,
         "Your manual dexterity returns.", 2, 2, ENCHANTMENT, FALSE);
  spello(SPELL_STUMBLE, "stumble", 0, 0, 0, POS_FIGHTING,
         TAR_CHAR_ROOM | TAR_FIGHT_VICT | TAR_NOT_SELF, TRUE, MAG_MANUAL, "Your balance returns.",
         2, 2, ENCHANTMENT, FALSE);
  spello(SPELL_ENERVATE, "enervate", 0, 0, 0, POS_FIGHTING,
         TAR_CHAR_ROOM | TAR_FIGHT_VICT | TAR_NOT_SELF, TRUE, MAG_MANUAL, "Your vitality returns.",
         2, 2, NECROMANCY, FALSE);
  spello(SPELL_PROT_UNDEAD, "protect undead", 0, 0, 0, POS_FIGHTING, TAR_CHAR_ROOM, FALSE,
         MAG_MANUAL, "The ward around your undead form fades.", 4, 4, ABJURATION, FALSE);
  spello(SPELL_PROT_FROM_UNDEAD, "protection from undead", 0, 0, 0, POS_FIGHTING, TAR_CHAR_ROOM,
         FALSE, MAG_MANUAL, "Your protection from undead fades.", 4, 4, ABJURATION, FALSE);
  spello(SPELL_COMMAND_HORDE, "command horde", 0, 0, 0, POS_FIGHTING, TAR_IGNORE, TRUE, MAG_MANUAL,
         NULL, 6, 6, NECROMANCY, FALSE);
  spello(SPELL_ANCESTRAL_SHIELD, "ancestral shield", 0, 0, 0, POS_FIGHTING, TAR_IGNORE, FALSE,
         MAG_MANUAL, "Your ancestral shield dissipates.", 3, 3, ABJURATION, FALSE);
  spello(SPELL_PROTECTION_FROM_ANIMALS, "protection from animals", 0, 0, 0, POS_FIGHTING,
         TAR_CHAR_ROOM, FALSE, MAG_MANUAL, "Your protection from animals fades.", 4, 4, ABJURATION,
         FALSE);
  spello(SPELL_PASS_WITHOUT_TRACE, "pass without trace", 0, 0, 0, POS_FIGHTING, TAR_IGNORE, FALSE,
         MAG_MANUAL, "You begin leaving tracks again.", 2, 2, TRANSMUTATION, FALSE);
  spello(SPELL_GREATER_REALM_OF_PROTECTION, "greater realm of protection", 0, 0, 0, POS_FIGHTING,
         TAR_CHAR_ROOM, FALSE, MAG_MANUAL, "Your layered elemental wards fade.", 4, 4, ABJURATION,
         FALSE);
  spello(SPELL_FEIGN_DEATH, "feign death", 0, 0, 0, POS_FIGHTING, TAR_CHAR_ROOM, FALSE, MAG_MANUAL,
         "Your deathlike stillness passes.", 3, 3, ILLUSION, FALSE);
  spello(SPELL_TRANQUILITY, "tranquility", 0, 0, 0, POS_FIGHTING, TAR_IGNORE, TRUE, MAG_MANUAL,
         "The imposed tranquility leaves you.", 2, 2, ENCHANTMENT, FALSE);
  spello(SPELL_AGILITY, "agility", 0, 0, 0, POS_FIGHTING, TAR_CHAR_ROOM, FALSE, MAG_MANUAL,
         "Your supernatural agility fades.", 4, 4, TRANSMUTATION, FALSE);
  spello(SPELL_NATURES_BLESSING, "natures blessing", 0, 0, 0, POS_FIGHTING,
         TAR_CHAR_ROOM | TAR_SELF_ONLY, FALSE, MAG_MANUAL, "Nature's blessing leaves you.", 3, 3,
         ABJURATION, FALSE);
  spello(SPELL_SONG_OF_TRAVEL, "song of travel", 0, 0, 0, POS_FIGHTING, TAR_IGNORE, FALSE,
         MAG_MANUAL, "The traveling melody fades from your thoughts.", 3, 3, TRANSMUTATION, FALSE);
}
