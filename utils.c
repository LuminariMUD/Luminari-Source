/**
 * @file utils.c                LuminariMUD
 * Various utility functions used within the core mud code.
 *
 * Part of the core tbaMUD source code distribution, which is a derivative
 * of, and continuation of, CircleMUD.
 *
 * All rights reserved.  See license for complete information.
 * Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University
 * CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.
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
#include "act.h"
#include "spec_procs.h" // for compute_ability
#include "mud_event.h"  // for purgemob event
#include "feats.h"
#include "spec_abilities.h"
#include "assign_wpn_armor.h"
#include "wilderness.h"
#include "domains_schools.h"
#include "constants.h"
#include "dg_scripts.h"
#include "alchemy.h"
#include "premadebuilds.h"
#include "craft.h"
#include "fight.h"
#include "missions.h"
#include "psionics.h"
#include "evolutions.h"
#include "backgrounds.h"
#include "char_descs.h"
#include "treasure.h"

/* kavir's protocol (isspace_ignoretabes() was moved to utils.h */

/* Functions of a general utility nature
   Functions directly related to utils.h needs
 */

/* MSDP GUI Wrappers */
void gui_combat_wrap_open(struct char_data *ch)
{
  if (!IS_NPC(ch) && PRF_FLAGGED(ch, PRF_GUI_MODE))
  { /* GUI Mode wrap open: combat */
    // send_to_char(ch, "<combat_message>\r\n");
  }
}

void gui_combat_wrap_notvict_open(struct char_data *ch, struct char_data *vict_obj)
{
  if (!ch)
    return;
  if (IN_ROOM(ch) == NOWHERE)
    return;
  if (IS_NPC(ch))
    return;

  /* we accept NULL victim */

  int to_sleeping = 0;
  struct char_data *to = world[IN_ROOM(ch)].people;

  for (; to; to = to->next_in_room)
  {
    if (!SENDOK(to) || (to == ch))
      continue;
    if (to == vict_obj) /* ch == victim? */
      continue;
    if (!PRF_FLAGGED(to, PRF_GUI_MODE))
      continue;
    // perform_act("<combat_message>\r\n", ch, NULL, vict_obj, to, FALSE);
  }
}

void gui_combat_wrap_close(struct char_data *ch)
{
  if (!IS_NPC(ch) && PRF_FLAGGED(ch, PRF_GUI_MODE))
  { /* GUI Mode wrap close: combat */
    // send_to_char(ch, "</combat_message>\r\n");
  }
}

void gui_combat_wrap_notvict_close(struct char_data *ch, struct char_data *vict_obj)
{
  if (!ch)
    return;
  if (IN_ROOM(ch) == NOWHERE)
    return;
  if (IS_NPC(ch))
    return;

  /* we accept NULL victim */

  int to_sleeping = 0;
  struct char_data *to = world[IN_ROOM(ch)].people;

  for (; to; to = to->next_in_room)
  {
    if (to == ch)
      continue;
    if (!SENDOK(to))
      continue;
    if (to == vict_obj) /* ch == victim? */
      continue;
    if (!PRF_FLAGGED(to, PRF_GUI_MODE))
      continue;
    // perform_act("</combat_message>\r\n", ch, NULL, vict_obj, to, FALSE);
  }
}

void gui_room_desc_wrap_open(struct char_data *ch)
{
  if (!IS_NPC(ch) && PRF_FLAGGED(ch, PRF_GUI_MODE))
  { /* GUI Mode wrap open: room description */
    send_to_char(ch, "<room_desc>");
  }
}

void gui_room_desc_wrap_close(struct char_data *ch)
{
  if (!IS_NPC(ch) && PRF_FLAGGED(ch, PRF_GUI_MODE))
  { /* GUI Mode wrap close: room description */
    send_to_char(ch, "</room_desc>");
  }
}

/* can this CH select the option to change their 'known' spells
 in the study system? */
bool can_study_known_spells(struct char_data *ch)
{

  /* sorcerer*/
  if (LEVELUP(ch)->class == CLASS_SORCERER ||
      ((LEVELUP(ch)->class == CLASS_ARCANE_ARCHER || LEVELUP(ch)->class == CLASS_MYSTIC_THEURGE ||
        LEVELUP(ch)->class == CLASS_ARCANE_SHADOW || LEVELUP(ch)->class == CLASS_SPELLSWORD ||
        LEVELUP(ch)->class == CLASS_KNIGHT_OF_THE_THORN ||
        LEVELUP(ch)->class == CLASS_ELDRITCH_KNIGHT || (LEVELUP(ch)->class == CLASS_NECROMANCER && NECROMANCER_CAST_TYPE(ch) == 1)) &&
       GET_PREFERRED_ARCANE(ch) == CLASS_SORCERER))
    return TRUE;

  /* bard */
  if (LEVELUP(ch)->class == CLASS_BARD ||
      ((LEVELUP(ch)->class == CLASS_ARCANE_ARCHER || LEVELUP(ch)->class == CLASS_MYSTIC_THEURGE ||
        LEVELUP(ch)->class == CLASS_ARCANE_SHADOW || LEVELUP(ch)->class == CLASS_SPELLSWORD ||
        LEVELUP(ch)->class == CLASS_KNIGHT_OF_THE_THORN ||
        LEVELUP(ch)->class == CLASS_ELDRITCH_KNIGHT || (LEVELUP(ch)->class == CLASS_NECROMANCER && NECROMANCER_CAST_TYPE(ch) == 1)) &&
       GET_PREFERRED_ARCANE(ch) == CLASS_BARD))
    return TRUE;

  /* summoner */
  if (LEVELUP(ch)->class == CLASS_SUMMONER ||
      ((LEVELUP(ch)->class == CLASS_ARCANE_ARCHER || LEVELUP(ch)->class == CLASS_MYSTIC_THEURGE ||
        LEVELUP(ch)->class == CLASS_ARCANE_SHADOW || LEVELUP(ch)->class == CLASS_SPELLSWORD ||
        LEVELUP(ch)->class == CLASS_KNIGHT_OF_THE_THORN ||
        LEVELUP(ch)->class == CLASS_ELDRITCH_KNIGHT || (LEVELUP(ch)->class == CLASS_NECROMANCER && NECROMANCER_CAST_TYPE(ch) == 1)) &&
       GET_PREFERRED_ARCANE(ch) == CLASS_SUMMONER))
    return TRUE;

  /* inquisitor */
  if (LEVELUP(ch)->class == CLASS_INQUISITOR ||
      ((LEVELUP(ch)->class == CLASS_MYSTIC_THEURGE || LEVELUP(ch)->class == CLASS_KNIGHT_OF_THE_SWORD || LEVELUP(ch)->class == CLASS_KNIGHT_OF_THE_ROSE || 
      LEVELUP(ch)->class == CLASS_KNIGHT_OF_THE_SKULL || 
      (LEVELUP(ch)->class == CLASS_NECROMANCER && NECROMANCER_CAST_TYPE(ch) == 2)) && 
      GET_PREFERRED_DIVINE(ch) == CLASS_INQUISITOR))
    return TRUE;

  /* warlock */
  if (LEVELUP(ch)->class == CLASS_WARLOCK)
    return TRUE;

  /* nope! */
  return FALSE;
}

/* can this CH select the option to change their 'known' psionic powers in the study system? */
bool can_study_known_psionics(struct char_data *ch)
{

  if (LEVELUP(ch)->class == CLASS_PSIONICIST)
    return TRUE;

  /* nope! */
  return FALSE;
}

/* ch, given class we're computing bonus spells for, figure out
 if one of our other classes (probably just prestige, example is
 arcane archer) is adding bonus caster levels */
int compute_bonus_caster_level(struct char_data *ch, int class)
{
  int bonus_levels = 0;

  switch (class)
  {
  case CLASS_WIZARD:
  case CLASS_SORCERER:
  case CLASS_BARD:
  case CLASS_SUMMONER:
    bonus_levels += CLASS_LEVEL(ch, CLASS_ARCANE_ARCHER) * 3 / 4 + CLASS_LEVEL(ch, CLASS_ARCANE_SHADOW) + CLASS_LEVEL(ch, CLASS_ELDRITCH_KNIGHT) + 
                    ((1 + CLASS_LEVEL(ch, CLASS_SPELLSWORD)) / 2) + CLASS_LEVEL(ch, CLASS_MYSTIC_THEURGE) + CLASS_LEVEL(ch, CLASS_KNIGHT_OF_THE_THORN) +
                    CLASS_LEVEL(ch, CLASS_NECROMANCER);
    break;
  case CLASS_CLERIC:
  case CLASS_DRUID:
  case CLASS_RANGER:
  case CLASS_PALADIN:
  case CLASS_INQUISITOR:
    bonus_levels += CLASS_LEVEL(ch, CLASS_NECROMANCER);
    bonus_levels += CLASS_LEVEL(ch, CLASS_MYSTIC_THEURGE);
    bonus_levels += CLASS_LEVEL(ch, CLASS_SACRED_FIST);
    bonus_levels += CLASS_LEVEL(ch, CLASS_KNIGHT_OF_THE_SWORD);
    bonus_levels += CLASS_LEVEL(ch, CLASS_KNIGHT_OF_THE_ROSE);
    bonus_levels += CLASS_LEVEL(ch, CLASS_KNIGHT_OF_THE_SKULL);
    break;
  default:
    break;
  }

  return bonus_levels;
}

int compute_arcane_level(struct char_data *ch)
{
  int arcane_level = 0;

  if (IS_NPC(ch)) /* npc is simple for now */
    return (GET_LEVEL(ch));

  arcane_level += CLASS_LEVEL(ch, CLASS_WIZARD);
  arcane_level += CLASS_LEVEL(ch, CLASS_SORCERER);
  arcane_level += CLASS_LEVEL(ch, CLASS_BARD);
  arcane_level += CLASS_LEVEL(ch, CLASS_SUMMONER);
  arcane_level += CLASS_LEVEL(ch, CLASS_ARCANE_SHADOW);
  arcane_level += CLASS_LEVEL(ch, CLASS_KNIGHT_OF_THE_THORN);
  if (NECROMANCER_CAST_TYPE(ch) == 1)
    arcane_level += CLASS_LEVEL(ch, CLASS_NECROMANCER);
  arcane_level += CLASS_LEVEL(ch, CLASS_ELDRITCH_KNIGHT);
  arcane_level += CLASS_LEVEL(ch, CLASS_ARCANE_ARCHER) * 3 / 4;
  arcane_level += CLASS_LEVEL(ch, CLASS_MYSTIC_THEURGE) / 2;
  arcane_level += compute_arcana_golem_level(ch) - (SPELLBATTLE(ch) / 2);
  arcane_level += (CLASS_LEVEL(ch, CLASS_SPELLSWORD) + 1) / 2;

  return arcane_level;
}

int compute_divine_level(struct char_data *ch)
{
  int divine_level = 0;

  if (IS_NPC(ch)) /* npc is simple for now */
    return (GET_LEVEL(ch));

  divine_level += CLASS_LEVEL(ch, CLASS_CLERIC);
  divine_level += CLASS_LEVEL(ch, CLASS_DRUID);
  divine_level += CLASS_LEVEL(ch, CLASS_INQUISITOR);
  divine_level += CLASS_LEVEL(ch, CLASS_SACRED_FIST);
  divine_level += CLASS_LEVEL(ch, CLASS_KNIGHT_OF_THE_SWORD);
  divine_level += CLASS_LEVEL(ch, CLASS_KNIGHT_OF_THE_ROSE);
  if (NECROMANCER_CAST_TYPE(ch) == 2)
    divine_level += CLASS_LEVEL(ch, CLASS_NECROMANCER);
  divine_level += MAX(0, CLASS_LEVEL(ch, CLASS_PALADIN) - 3);
  divine_level += MAX(0, CLASS_LEVEL(ch, CLASS_BLACKGUARD) - 3);
  divine_level += MAX(0, CLASS_LEVEL(ch, CLASS_RANGER) - 3);
  divine_level += CLASS_LEVEL(ch, CLASS_MYSTIC_THEURGE) / 2;
  divine_level += CLASS_LEVEL(ch, CLASS_KNIGHT_OF_THE_SKULL);
  divine_level += compute_arcana_golem_level(ch) - (SPELLBATTLE(ch) / 2);

  return divine_level;
}

int compute_channel_energy_level(struct char_data *ch)
{
  if (!ch)
    return 0;

  int level = 0;

  level += CLASS_LEVEL(ch, CLASS_CLERIC);
  level += CLASS_LEVEL(ch, CLASS_INQUISITOR);
  level += CLASS_LEVEL(ch, CLASS_SACRED_FIST);
  level += CLASS_LEVEL(ch, CLASS_KNIGHT_OF_THE_SWORD);
  level += CLASS_LEVEL(ch, CLASS_KNIGHT_OF_THE_ROSE);
  level += MAX(0, CLASS_LEVEL(ch, CLASS_PALADIN) - 4);
  level += MAX(0, CLASS_LEVEL(ch, CLASS_BLACKGUARD) - 4);
  level += CLASS_LEVEL(ch, CLASS_MYSTIC_THEURGE) / 2;

  return level;
}

/* check to see if CH has a weapon attached to a combat feat
this use to be a nice(?) compact macro, but circumstances forced expansion */
bool compute_has_combat_feat(struct char_data *ch, int cfeat, int weapon)
{
  bool using_comp = FALSE;
  bool has_comp_feat = FALSE;

  if (cfeat == -1)
    return false;

  /* had to add special test to weapon combat feats because of the way
     I set up composite bows with various strength modifiers -zusuk */
  switch (weapon)
  {
  case WEAPON_TYPE_COMPOSITE_LONGBOW:
  case WEAPON_TYPE_COMPOSITE_LONGBOW_2:
  case WEAPON_TYPE_COMPOSITE_LONGBOW_3:
  case WEAPON_TYPE_COMPOSITE_LONGBOW_4:
  case WEAPON_TYPE_COMPOSITE_LONGBOW_5:
    using_comp = TRUE;
    break;
  default:
    break; /* most cases */
  }
  if (IS_SET_AR((ch)->char_specials.saved.combat_feats[cfeat],
                WEAPON_TYPE_COMPOSITE_LONGBOW) ||
      IS_SET_AR((ch)->char_specials.saved.combat_feats[cfeat],
                WEAPON_TYPE_COMPOSITE_LONGBOW_2) ||
      IS_SET_AR((ch)->char_specials.saved.combat_feats[cfeat],
                WEAPON_TYPE_COMPOSITE_LONGBOW_3) ||
      IS_SET_AR((ch)->char_specials.saved.combat_feats[cfeat],
                WEAPON_TYPE_COMPOSITE_LONGBOW_4) ||
      IS_SET_AR((ch)->char_specials.saved.combat_feats[cfeat],
                WEAPON_TYPE_COMPOSITE_LONGBOW_5))
  {
    has_comp_feat = TRUE;
  }

  if (using_comp && has_comp_feat)
    return TRUE; /* any comp longbow feat, any comp longbow used */

  /* reset variables */
  has_comp_feat = FALSE;
  using_comp = FALSE;

  switch (weapon)
  {
  case WEAPON_TYPE_COMPOSITE_SHORTBOW:
  case WEAPON_TYPE_COMPOSITE_SHORTBOW_2:
  case WEAPON_TYPE_COMPOSITE_SHORTBOW_3:
  case WEAPON_TYPE_COMPOSITE_SHORTBOW_4:
  case WEAPON_TYPE_COMPOSITE_SHORTBOW_5:
    using_comp = TRUE;
    break;
  default:
    break; /* most cases */
  }
  if (IS_SET_AR((ch)->char_specials.saved.combat_feats[cfeat],
                WEAPON_TYPE_COMPOSITE_SHORTBOW) ||
      IS_SET_AR((ch)->char_specials.saved.combat_feats[cfeat],
                WEAPON_TYPE_COMPOSITE_SHORTBOW_2) ||
      IS_SET_AR((ch)->char_specials.saved.combat_feats[cfeat],
                WEAPON_TYPE_COMPOSITE_SHORTBOW_3) ||
      IS_SET_AR((ch)->char_specials.saved.combat_feats[cfeat],
                WEAPON_TYPE_COMPOSITE_SHORTBOW_4) ||
      IS_SET_AR((ch)->char_specials.saved.combat_feats[cfeat],
                WEAPON_TYPE_COMPOSITE_SHORTBOW_5))
  {
    has_comp_feat = TRUE;
  }

  if (using_comp && has_comp_feat)
    return TRUE; /* any comp bow feat, any comp bow used */

  /*debug*/
  // send_to_char(ch, "feat: %d, weapon: %d\r\n", cfeat, weapon);

  /* normal test now */
  if ((IS_SET_AR((ch)->char_specials.saved.combat_feats[(cfeat)], (weapon))))
    return TRUE;

  /* nope, nothing! */
  return FALSE;
}

int compute_dexterity_bonus(struct char_data *ch)
{
  if (!ch)
    return 0;
  int dexterity_bonus = (GET_DEX(ch) - 10) / 2;
  if (AFF_FLAGGED(ch, AFF_GRAPPLED) || AFF_FLAGGED(ch, AFF_ENTANGLED))
    dexterity_bonus -= 2;
  if (AFF_FLAGGED(ch, AFF_PINNED))
    dexterity_bonus = -5;
  dexterity_bonus -= get_char_affect_modifier(ch, AFFECT_LEVEL_DRAIN, APPLY_SPECIAL);

  return (MIN(compute_gear_max_dex(ch), dexterity_bonus));
}

int compute_strength_bonus(struct char_data *ch)
{
  if (!ch)
    return 0;
  int bonus = (GET_STR(ch) - 10) / 2;
  bonus -= get_char_affect_modifier(ch, AFFECT_LEVEL_DRAIN, APPLY_SPECIAL);

  return bonus;
}

int compute_constitution_bonus(struct char_data *ch)
{
  if (!ch)
    return 0;
  int bonus = (GET_CON(ch) - 10) / 2;
  bonus -= get_char_affect_modifier(ch, AFFECT_LEVEL_DRAIN, APPLY_SPECIAL);

  return bonus;
}

int compute_intelligence_bonus(struct char_data *ch)
{
  if (!ch)
    return 0;
  int bonus = (GET_INT(ch) - 10) / 2;
  bonus -= get_char_affect_modifier(ch, AFFECT_LEVEL_DRAIN, APPLY_SPECIAL);

  return bonus;
}

int compute_wisdom_bonus(struct char_data *ch)
{
  if (!ch)
    return 0;
  int bonus = (GET_WIS(ch) - 10) / 2;
  bonus -= get_char_affect_modifier(ch, AFFECT_LEVEL_DRAIN, APPLY_SPECIAL);

  return bonus;
}

int compute_charisma_bonus(struct char_data *ch)
{
  if (!ch)
    return 0;
  int bonus = (GET_CHA(ch) - 10) / 2;
  bonus -= get_char_affect_modifier(ch, AFFECT_LEVEL_DRAIN, APPLY_SPECIAL);

  return bonus;
}

int stats_cost_chart[11] = {/* cost for total points */
                            /*0  1  2  3  4  5  6  7  8   9   10 */
                            0, 1, 2, 3, 4, 5, 6, 8, 10, 13, 16};

int comp_base_dex(struct char_data *ch)
{
  int base_dex = BASE_STAT;
  /*
  switch (GET_RACE(ch)) {
    case RACE_ELF: base_dex += 2;
      break;
    case RACE_DROW: base_dex += 2;
      break;
    case RACE_HALFLING: base_dex += 2;
      break;
    case RACE_HALF_TROLL: base_dex += 2;
      break;
    case RACE_TRELUX: base_dex += 8;
      break;
  }
  */
  return base_dex;
}

int comp_dex_cost(struct char_data *ch, int number)
{
  int base_dex = comp_base_dex(ch), current_dex = GET_REAL_DEX(ch) + number;
  return stats_cost_chart[current_dex - base_dex];
}

int comp_base_str(struct char_data *ch)
{
  int base_str = BASE_STAT;
  /*
  switch (GET_RACE(ch)) {
    case RACE_HALFLING: base_str -= 2;
      break;
    case RACE_GNOME: base_str -= 2;
      break;
    case RACE_HALF_ORC:
    case RACE_HALF_TROLL: base_str += 2;
      break;
    case RACE_CRYSTAL_DWARF: base_str += 2;
      break;
    case RACE_TRELUX: base_str += 2;
      break;
    case RACE_ARCANA_GOLEM: base_str -= 2;
      break;
  }
  */
  return base_str;
}

int comp_str_cost(struct char_data *ch, int number)
{
  int base_str = comp_base_str(ch),
      current_str = GET_REAL_STR(ch) + number;
  return stats_cost_chart[current_str - base_str];
}

int comp_base_con(struct char_data *ch)
{
  int base_con = BASE_STAT;
  /*
  switch (GET_RACE(ch)) {
    case RACE_DROW: base_con -= 2;
      break;
    case RACE_ELF: base_con -= 2;
      break;
    case RACE_DWARF: base_con += 2;
      break;
    case RACE_GNOME: base_con += 2;
      break;
    case RACE_HALF_TROLL: base_con += 2;
      break;
    case RACE_CRYSTAL_DWARF: base_con += 8;
      break;
    case RACE_TRELUX: base_con += 4;
      break;
    case RACE_ARCANA_GOLEM: base_con -= 2;
      break;
  }
  */
  return base_con;
}

int comp_con_cost(struct char_data *ch, int number)
{
  int base_con = comp_base_con(ch), current_con = GET_REAL_CON(ch) + number;
  return stats_cost_chart[current_con - base_con];
}

int comp_base_inte(struct char_data *ch)
{
  int base_inte = BASE_STAT;
  /*
  switch (GET_RACE(ch)) {
    case RACE_HALF_ORC: base_inte -= 2;
      break;
    case RACE_HALF_TROLL: base_inte -= 4;
      break;
    case RACE_ARCANA_GOLEM: base_inte += 2;
      break;
    case RACE_DROW: base_inte += 2;
      break;
  }
  */
  return base_inte;
}

int comp_inte_cost(struct char_data *ch, int number)
{
  int base_inte = comp_base_inte(ch), current_inte = GET_REAL_INT(ch) + number;
  return stats_cost_chart[current_inte - base_inte];
}

int comp_base_wis(struct char_data *ch)
{
  int base_wis = BASE_STAT;
  /*
  switch (GET_RACE(ch)) {
    case RACE_HALF_TROLL: base_wis -= 4;
      break;
    case RACE_CRYSTAL_DWARF: base_wis += 2;
      break;
    case RACE_ARCANA_GOLEM: base_wis += 2;
      break;
    case RACE_DROW: base_wis += 2;
      break;
  }
  */
  return base_wis;
}

int comp_wis_cost(struct char_data *ch, int number)
{
  int base_wis = comp_base_wis(ch), current_wis = GET_REAL_WIS(ch) + number;
  return stats_cost_chart[current_wis - base_wis];
}

int comp_base_cha(struct char_data *ch)
{
  int base_cha = BASE_STAT;
  /*
  switch (GET_RACE(ch)) {
    case RACE_DWARF:
    case RACE_HALF_ORC: base_cha -= 2;
      break;
    case RACE_DROW: base_cha += 2;
      break;
    case RACE_HALF_TROLL: base_cha -= 4;
      break;
    case RACE_CRYSTAL_DWARF: base_cha += 2;
      break;
    case RACE_ARCANA_GOLEM: base_cha += 2;
      break;
  }
  */
  return base_cha;
}

int comp_cha_cost(struct char_data *ch, int number)
{
  int base_cha = comp_base_cha(ch), current_cha = GET_REAL_CHA(ch) + number;
  return stats_cost_chart[current_cha - base_cha];
}

int comp_total_stat_points(struct char_data *ch)
{
  return (comp_cha_cost(ch, 0) + comp_wis_cost(ch, 0) + comp_inte_cost(ch, 0) +
          comp_str_cost(ch, 0) + comp_dex_cost(ch, 0) + comp_con_cost(ch, 0));
}

int stats_point_left(struct char_data *ch)
{
  if (GET_PREMADE_BUILD_CLASS(ch) != CLASS_UNDEFINED)
    return 0;
  return (TOTAL_STAT_POINTS(ch) - comp_total_stat_points(ch));
}

/* unused */
int spell_level_ch(struct char_data *ch, int spell)
{
  int domain = 0, domain_spell = 0, this_spell = -1, i = 0;

  if (CLASS_LEVEL(ch, CLASS_CLERIC) || CLASS_LEVEL(ch, CLASS_INQUISITOR))
  {
    for (domain = 0; domain < NUM_DOMAINS; domain++)
    {

      if (GET_1ST_DOMAIN(ch) == domain || GET_2ND_DOMAIN(ch) == domain)
      {
        /* we have this domain, check if spell is granted */

        for (domain_spell = 0; domain_spell < MAX_DOMAIN_SPELLS; domain_spell++)
        {
          this_spell = domain_list[domain].domain_spells[domain_spell];
          if (this_spell == spell)
          {
            return ((domain_spell + 1) * 2 - 1); /* returning level that corresponds to circle (i) */
          }
        }
      }
    }
  }
  else
  { /* not cleric */
  }

  for (i = 0; i < NUM_CLASSES; i++)
  {
    if (spell_info[spell].min_level[i] < LVL_IMMORT)
    {
      return (spell_info[spell].min_level[i]);
    }
  }

  return (LVL_IMPL + 1);
}

int compute_level_domain_spell_is_granted(int domain, int spell)
{
  int i = 0, this_spell = -1;

  for (i = 0; i < MAX_DOMAIN_SPELLS; i++)
  {
    this_spell = domain_list[domain].domain_spells[i];
    if (this_spell == spell)
    {
      return ((i + 1) * 2 - 1); /* returning level that corresponds to circle (i) */
    }
  }

  /* failed */
  return (LVL_IMPL + 1);
}

/* check if the player's size should be different than that stored in-file */
int compute_current_size(struct char_data *ch)
{
  int size = SIZE_UNDEFINED;
  if (!ch)
    return size;
  int racenum = GET_DISGUISE_RACE(ch);

  if (AFF_FLAGGED(ch, AFF_WILD_SHAPE) && racenum)
  { // wildshaped
    size = race_list[racenum].size;
    if (affected_by_spell(ch, SPELL_ENLARGE_PERSON))
      size++;
    if (affected_by_spell(ch, SPELL_SHRINK_PERSON))
      size--;
  }
  else
  {
    size = (ch)->points.size;
  }

  if (size < SIZE_FINE)
    size = SIZE_FINE;
  if (size > SIZE_COLOSSAL)
    size = SIZE_COLOSSAL;

  return size;
}

/* Take a room and direction and give the resulting room vnum, made by zusuk
 * for ornir's wilderness, returns "NOWHERE" on failure */
room_vnum get_direction_vnum(room_rnum room_origin, int direction)
{
  room_rnum exit_rnum = NOWHERE;
  room_vnum exit_vnum = NOWHERE;
  int x_coordinate = -1, y_coordinate = -1;

  /* exit values */
  if (!VALID_ROOM_RNUM(room_origin))
    return NOWHERE;
  if (direction >= NUM_OF_INGAME_DIRS || direction < 0)
    return NOWHERE;

  /* is there even an exit this way? */
  if (W_EXIT(room_origin, direction))
  {
    exit_rnum = W_EXIT(room_origin, direction)->to_room;
  }
  else
  {
    return NOWHERE;
  }
  exit_vnum = GET_ROOM_VNUM(exit_rnum);

  /* handle wilderness, if the room exists we have to fix the vnum */
  if (exit_vnum == 1000000)
  {
    x_coordinate = world[room_origin].coords[0];
    y_coordinate = world[room_origin].coords[1];
    switch (direction)
    {
    case NORTH:
      y_coordinate++;
      break;
    case SOUTH:
      y_coordinate--;
      break;
    case EAST:
      x_coordinate++;
      break;
    case WEST:
      x_coordinate--;
      break;
    default:
      log("SYSERR: Wilderness utility failure.");
      // Bad direction for wilderness travel (up/down)
      return NOWHERE;
    }
    exit_rnum = find_room_by_coordinates(x_coordinate, y_coordinate);
    if (exit_rnum == NOWHERE)
    {
      exit_rnum = find_available_wilderness_room();
      if (exit_rnum == NOWHERE)
      {
        log("SYSERR: Wilderness utility failure.");
        return NOWHERE;
      }
      /* Must set the coords, etc in the exit room. */
      assign_wilderness_room(exit_rnum, x_coordinate, y_coordinate);
    }
    exit_vnum = GET_ROOM_VNUM(exit_rnum);
    /* what should we do if it's a wilderness room? */
  }
  else
  { /* should be a normal room */
    /* exit_vnum already has the correct value */
  }

  return exit_vnum;
}

/* this function in conjuction with the AFF_GROUP flag will cause mobs who
   load in the same room to group up */
void set_mob_grouping(struct char_data *ch)
{
  struct char_data *tch = NULL, *tch_next = NULL;

  for (tch = world[ch->in_room].people; tch; tch = tch_next)
  {
    tch_next = tch->next_in_room;
    if (IS_NPC(tch) && IS_NPC(ch) && AFF_FLAGGED(ch, AFF_GROUP) && tch != ch &&
        AFF_FLAGGED(tch, AFF_GROUP) && !tch->master)
    {
      if (!GROUP(tch))
        create_group(tch);
      add_follower(ch, tch);
      if (!GROUP(ch))
        join_group(ch, GROUP(tch));
      return;
    }
  }
}

/* identifies if room is outdoors or not (as opposed to room num)
 used by:  ROOM_OUTSIDE() macro */
bool is_room_outdoors(room_rnum room_number)
{
  if (room_number == NOWHERE)
  {
    log("is_room_outdoors() found NOWHERE");
    return FALSE;
  }

  if (ROOM_FLAGGED(room_number, ROOM_INDOORS))
    return FALSE;

  switch (SECT(room_number))
  {
  case SECT_INSIDE:
  case SECT_INSIDE_ROOM:
  case SECT_OCEAN:
  case SECT_UD_WILD:
  case SECT_UD_CITY:
  case SECT_UD_INSIDE:
  case SECT_UD_WATER:
  case SECT_UD_NOSWIM:
  case SECT_UD_NOGROUND:
  case SECT_LAVA:
  case SECT_UNDERWATER:
    return FALSE;
  }

  return TRUE;
}

bool is_in_water(struct char_data *ch)
{
  if (IN_ROOM(ch) == NOWHERE)
    return false;

  switch (world[IN_ROOM(ch)].sector_type)
  {
    case SECT_BEACH:
    case SECT_OCEAN:
    case SECT_RIVER:
    case SECT_SEAPORT:
    case SECT_UD_NOSWIM:
    case SECT_UD_WATER:
    case SECT_WATER_NOSWIM:
    case SECT_WATER_SWIM:
    case SECT_UNDERWATER:
      return true;
  }
  return false;
}

bool is_in_wilderness(struct char_data *ch)
{
  if (!OUTDOORS(ch))
    return false;

  switch (world[IN_ROOM(ch)].sector_type)
  {
    case SECT_BEACH:
    case SECT_DESERT:
    case SECT_FIELD:
    case SECT_FOREST:
    case SECT_HIGH_MOUNTAIN:
    case SECT_HILLS:
    case SECT_JUNGLE:
    case SECT_MARSHLAND:
    case SECT_MOUNTAIN:
    case SECT_TAIGA:
    case SECT_TUNDRA:
      return true;
  }
  return false;
}

/* identifies if CH is outdoors or not (as opposed to room num)
 used by:  OUTSIDE() macro */
bool is_outdoors(struct char_data *ch)
{
  if (!ch)
  {
    log("is_outdoors() receive NULL ch");
    return FALSE;
  }

  return (is_room_outdoors(IN_ROOM(ch)));
}

/* will check if ch should suffer from ultra-blindness */
bool ultra_blind(struct char_data *ch, room_rnum room_number)
{
  if (!AFF_FLAGGED(ch, AFF_ULTRAVISION))
    return FALSE;

  if (world[room_number].globe)
    return FALSE;

  if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_INDOORS))
    return FALSE;

  if (AFF_FLAGGED(ch, AFF_DARKVISION))
    return FALSE;

  if (IS_SET_AR(ROOM_FLAGS(room_number), ROOM_FOG) && GET_LEVEL(ch) < LVL_IMMORT)
    return FALSE;

  switch (SECT(room_number))
  {
  case SECT_INSIDE:
  case SECT_INSIDE_ROOM:
  case SECT_FOREST:
  case SECT_MARSHLAND:
  case SECT_UNDERWATER:
  case SECT_UD_WILD:
  case SECT_UD_CITY:
  case SECT_UD_INSIDE:
  case SECT_UD_WATER:
  case SECT_UD_NOSWIM:
  case SECT_UD_NOGROUND:
  case SECT_LAVA:
    return FALSE;
  default:
    break;
  }

  if (weather_info.sunlight == SUN_LIGHT)
    return TRUE;

  return FALSE;
}

/** Calculate the number of objects in the given obj.
 * @param obj - The object to check for contents. */
int num_obj_in_obj(struct obj_data *obj)
{
  int i = 0;

  for (; obj; obj = obj->next_content)
    i++;

  return i;
}

/* Ils: color code counter */

/* homeland-port - don't think it even works in our codebase */
int color_count(char *bufptr)
{
  int count = 0;
  char *temp = bufptr;

  while ((temp = strchr(temp, '@')) != NULL)
  {
    if (*(temp + 1) == '@')
    {
      count++; /* adjust count 1 char for every && */
      temp++;  /* point to char after 2nd && */
    }
    else
    {
      count += 2; /* adjust count by 3 for every color change */
      temp++;     /* point to char after color codes */
    }
    if (*temp == '\0')
      break;
  }

  return count;
}

/* determines if you can crit this target based on attacker
     added attacker so we can account for powerful beings that can critical hit through anything potentially -zusuk */
int is_immune_to_crits(struct char_data *attacker, struct char_data *target)
{
  int powerful_being = 0;

  /* new code to help really powerful beings overcome checks here */
  if (IS_POWERFUL_BEING(attacker))
  {
    /* base 20% chance of overcoming defense */
    powerful_being = 20;

    /* every level above 30 gives another 10% */
    powerful_being += (GET_LEVEL(attacker) - (LVL_IMMORT - 1)) * 10;

    if (rand_number(1, 100) < powerful_being)
      return FALSE; /* immune to this crit this pass! */
  }

  /* now to normal conditions for critical immunity */

  /* undead immune to crits, not vital organs */
  if (IS_UNDEAD(target))
    return TRUE;

  if (affected_by_spell(target, PSIONIC_SHADOW_BODY))
    return TRUE;
  if (affected_by_spell(target, PSIONIC_BODY_OF_IRON))
    return TRUE;
  
  if (HAS_FEAT(target, FEAT_ESSENCE_OF_UNDEATH))
    return true;

  /* preserve organs as 25% of stopping crits */
  if (!IS_NPC(target) && (KNOWS_DISCOVERY(target, ALC_DISC_PRESERVE_ORGANS) && dice(1, 4) == 1))
    return TRUE; /* avoided this crit! */

  if (affected_by_spell(target, SPELL_SHIELD_OF_FORTIFICATION) && dice(1, 4) == 1)
    return TRUE; /* avoided this crit! */

  if (affected_by_spell(target, PSIONIC_BODY_OF_IRON))
    return TRUE; /* avoided this crit! */
  if (affected_by_spell(target, PSIONIC_SHADOW_BODY))
    return TRUE; /* avoided this crit! */

  /* not immune to crits! */
  return FALSE;
}

/* support function for check_npc_followers(), checks to see if this mobile should be counted
   towards your npc pet/charmee limit */
bool not_npc_limit(struct char_data *pet)
{
  bool counts = FALSE;

  /* we have a list of flags to reference, then specific VNUMS to check */

  /* flags */
  if (MOB_FLAGGED(pet, MOB_C_O_T_N))
    counts = TRUE;
  if (MOB_FLAGGED(pet, MOB_VAMP_SPWN))
    counts = TRUE;
  if (MOB_FLAGGED(pet, MOB_DRAGON_KNIGHT))
    counts = TRUE;
  if (MOB_FLAGGED(pet, MOB_MUMMY_DUST))
    counts = TRUE;
  if (MOB_FLAGGED(pet, MOB_SHADOW))
    counts = TRUE;
  /*
  if (MOB_FLAGGED(pet, MOB_MERCENARY))
    counts = TRUE;
  */
  if (MOB_FLAGGED(pet, MOB_PLANAR_ALLY))
    counts = TRUE;
  if (MOB_FLAGGED(pet, MOB_ANIMATED_DEAD))
    counts = TRUE;
  if (MOB_FLAGGED(pet, MOB_ELEMENTAL))
    counts = TRUE;
  if (MOB_FLAGGED(pet, MOB_C_ANIMAL))
    counts = TRUE;
  if (MOB_FLAGGED(pet, MOB_C_FAMILIAR))
    counts = TRUE;
  if (MOB_FLAGGED(pet, MOB_C_MOUNT))
    counts = TRUE;
  if (MOB_FLAGGED(pet, MOB_EIDOLON))
    counts = TRUE;

  /* vnums */
  switch (GET_MOB_VNUM(pet))
  {

  /* spirit eagle */
  case 101225:
    counts = TRUE;
    break;

  /* large spirit eagle */
  case 132131:
    counts = TRUE;
    break;

  /* Fullstaff's horn */
  case 11389:
    counts = TRUE;
    break;

  /* small figurine carved in black stone */
  case 132199:
    counts = TRUE;
    break;

  default:
    break;
  }

  return counts;
}

bool isGenieKind(int vnum)
{
  switch (vnum)
  {
    case MOB_DJINNI_KIND:
    case MOB_EFREETI_KIND:
    case MOB_MARID_KIND:
    case MOB_SHAITAN_KIND:
      return true;
  }
  return false;
}

bool can_add_follower_by_flag(struct char_data *ch, int flag)
{
  struct char_data *pet;
  struct follow_type *k, *next;
  int undead = 0;
  int undead_allowed = CLASS_LEVEL(ch, CLASS_NECROMANCER) ? 2 : 1;


  /* loop through followers */
  for (k = ch->followers; k; k = next)
  {
    next = k->next;

    pet = k->follower;
    if (IS_PET(pet))
    {
      if (MOB_FLAGGED(pet, flag))
      {
        undead++;
        if (undead <= undead_allowed)
          return false;
      }
    }
  }
  return true;
}

bool can_add_follower(struct char_data *ch, int mob_vnum)
{
  struct char_data *pet;
  struct follow_type *k, *next;
  
  int summons_allowed = 1,
      pets_allowed = 1,
      mercs_allowed = 1,
      genie_allowed = 1;

  if (IS_SUMMONER(ch))
    summons_allowed++;

  /* loop through followers */
  for (k = ch->followers; k; k = next)
  {
    next = k->next;

    pet = k->follower;
    if (IS_PET(pet))
    {
      if (isGenieKind(mob_vnum))
      {
        genie_allowed--;
      }
      else if (isSummonMob(mob_vnum))
      {
        summons_allowed--;
      }
      else if (MOB_FLAGGED(pet, MOB_MERCENARY))
      {
        mercs_allowed--;
      }
      else
      {
        pets_allowed--;
      }
    }
  }

  struct char_data *mob = read_mobile(mob_vnum, VIRTUAL);

  if (!mob)
  {
    send_to_char(ch, "Mob vnum %d not found.\r\n", mob_vnum);
    return false;
  }

  char_to_room(mob, 0);

  // there's probably a better way of doing this, but this will ensure they can only have 1 of each
  // except in special circumstances. Eg. summoner can have 2 summons instead of 1.
  if (isGenieKind(mob_vnum))
  {
    extract_char(mob);
    if (genie_allowed > 0)
      return true;
    return false;
  }
  else if (isSummonMob(mob_vnum))
  {
    extract_char(mob);
    if (summons_allowed > 0)
      return true;
    return false;
  }
  else if (MOB_FLAGGED(mob, MOB_MERCENARY))
  {
    extract_char(mob);
    if (mercs_allowed > 0)
      return true;
    return false;
  }
  else
  {
    extract_char(mob);
    if (pets_allowed > 0)
      return true;
    return false;
  }
  extract_char(mob);
  return false;
}

/*
this function is to deal with our follower army! -zusuk
   in - ch: pc we're dealing with
   in - mode: what mode are we using, we have display, flag based, total count, specific vnum
   in - variable: for flag mode, mob_flag...  specific mode, mob_vnum
   out - count of followers
   */
/* reference
  #define NPC_MODE_DISPLAY 0
  #define NPC_MODE_FLAG 1
  #define NPC_MODE_SPECIFIC 2
  #define NPC_MODE_COUNT 3
  #define NPC_MODE_SPARE 4
*/
int check_npc_followers(struct char_data *ch, int mode, int variable)
{
  struct follow_type *k = NULL, *next = NULL;
  struct char_data *pet = NULL;
  int total_count = 0, flag_count = 0, vnum_count = 0, merc_slot = 0,
      paid_slot = 0, free_slot = 0, summon_slot = 0, spare = 0;

  if (mode == NPC_MODE_DISPLAY)
  {
    text_line(ch, "\tYPets Charmees NPC Followers\tn", 80, '-', '-');
  }

  /* loop through followers */
  for (k = ch->followers; k; k = next)
  {
    next = k->next;

    pet = k->follower;

    /* is this a charmee? */
    if (IS_PET(pet))
    {

      /* found a pet!  this is our total # of followers*/
      total_count++;

      /* we differentiate between npc's that don't take up slots vs every other form of charmee here */
      if (not_npc_limit(pet))
        free_slot++;
      else if (MOB_FLAGGED(pet, MOB_MERCENARY))
        merc_slot++;
      else if (isSummonMob(GET_MOB_VNUM(pet)))
        summon_slot++;
      else
        paid_slot++;

      switch (mode)
      {

      case NPC_MODE_FLAG:
        if (MOB_FLAGGED(pet, variable))
        {
          flag_count++;
        }
        break;

      case NPC_MODE_SPECIFIC:
        if (GET_MOB_VNUM(pet) == variable)
        {
          vnum_count++;
        }
        break;

      case NPC_MODE_DISPLAY:
        send_to_char(ch, "\tC%-2d\tw)\tC %-8s \tw-\tC %s \tw-\tC Slot?: %s\r\n",
                     total_count,
                     GET_NAME(pet),
                     world[IN_ROOM(pet)].name,
                     not_npc_limit(pet) ? "\tWNo\tn" : "\tRYes\tn");
        break;

      } /* end switch */
    }   /* end charmee check */
  }     /* end for */

#if !defined(CAMPAIGN_FR) && !defined(CAMPAIGN_DL)
  /* charisma bonus, spare represents our extra slots */
  if (GET_CHA_BONUS(ch) <= 0)
    spare = 0;
  else
    spare = GET_CHA_BONUS(ch);
#endif

  spare++; /* base 1 */

  spare = spare - paid_slot - (MAX(0, summon_slot - 1));

  /* out we go! */
  switch (mode)
  {

  case NPC_MODE_FLAG:
    return flag_count;

  case NPC_MODE_SPECIFIC:
    return vnum_count;

  case NPC_MODE_DISPLAY:
    draw_line(ch, 80, '-', '-');

    if (mode == NPC_MODE_DISPLAY)
    {
      send_to_char(ch, "\tCYou have %d pets, %d of them don't take slots, %d do...  your Charisma allows for %d more.  (minimum 1 extra)\tn\r\n",
                   total_count, free_slot, paid_slot, spare);
    }

    break;

  case NPC_MODE_SPARE:

    return (spare);

  } /* end switch */

  return total_count;
}

bool char_pets_to_char_loc(struct char_data *ch)
{

  bool found = false;

  struct char_data *tch = NULL;

  for (tch = character_list; tch; tch = tch->next)
  {
    if (tch == ch)
      continue;
    if (!IS_NPC(tch))
      continue;
    if (!AFF_FLAGGED(tch, AFF_CHARM))
      continue;
    if (tch->master != ch)
      continue;
    if (IN_ROOM(tch) == NOWHERE)
      continue;
    if (IN_ROOM(tch) == IN_ROOM(ch))
      continue;

    /* leave the current room into the ether */
    act("$n disappears in a flash of light.", FALSE, tch, 0, 0, TO_ROOM);
    char_from_room(tch);

    /* set coords if necessary */
    if (ZONE_FLAGGED(GET_ROOM_ZONE(IN_ROOM(ch)), ZONE_WILDERNESS))
    {
      X_LOC(tch) = world[IN_ROOM(ch)].coords[0];
      Y_LOC(tch) = world[IN_ROOM(ch)].coords[1];
    }

    /* move into the new location! */
    char_to_room(tch, IN_ROOM(ch));
    act("$n appears in a flash of light.", FALSE, tch, 0, 0, TO_ROOM);

    found = true;
  }
  return found;
}

/* this will calculate the arcana-golem race bonus caster level */
int compute_arcana_golem_level(struct char_data *ch)
{
  if (!ch)
    return 0;

  if (IS_NPC(ch))
    return 0;

  if (GET_RACE(ch) != RACE_ARCANA_GOLEM)
    return 0;

  return ((int)(GET_LEVEL(ch) / 6));
}

/* function to check if a char (ranger) has given favored enemy (race) */
bool is_fav_enemy_of(struct char_data *ch, int race)
{
  int i = 0;

  for (i = 0; i < MAX_ENEMIES; i++)
  {
    if (GET_FAVORED_ENEMY(ch, i) == race)
      return TRUE;
  }
  return FALSE;
}

/* returns a or an based on first character of next word */
const char *a_or_an(const char *string)
{
  switch (tolower(*string))
  {
  case 'a':
  case 'e':
  case 'i':
  case 'u':
  case 'o':
    return "an";
  }

  return "a";
}

/* function for difficulty of passing a sneak-check
 * ch = listener (challenge), sneaker = sneaker (DC)
 */
bool can_hear_sneaking(struct char_data *ch, struct char_data *sneaker)
{
  /* free passes */
  if (!AFF_FLAGGED(sneaker, AFF_SNEAK))
    return TRUE;

  /* do listen check here */
  bool can_hear = FALSE;
  int challenge = d20(ch), dc = (d20(sneaker) + 10);

  // challenger bonuses/penalty (ch)
  if (!IS_NPC(ch))
  {
    challenge += compute_ability(ch, ABILITY_PERCEPTION);
  }
  else /* NPC */
    challenge += GET_LEVEL(ch);

  if (AFF_FLAGGED(ch, AFF_LISTEN))
    challenge += 10;

  /* sneak bonus/penalties (sneaker) */
  if (!IS_NPC(sneaker))
  {
    dc += compute_ability((struct char_data *)sneaker, ABILITY_STEALTH);

    if (IN_NATURE(sneaker) && HAS_FEAT(sneaker, FEAT_TRACKLESS_STEP))
    {
      dc += 4;
    }
    if (IN_NATURE(sneaker) && HAS_FEAT(sneaker, FEAT_CAMOUFLAGE))
    {
      dc += 6;
    }
  }
  else /* NPC */
  {
    dc += GET_LEVEL(sneaker);
  }

  /* size bonus */
  dc += (GET_SIZE(ch) - GET_SIZE(sneaker)) * 2;

  if (challenge > dc)
    can_hear = TRUE;

  return (can_hear);
}

/* function for hide-check
 * ch = spotter (challenge), vict = hider (DC)
 */
bool can_see_hidden(struct char_data *ch, struct char_data *hider)
{
  /* free passes */
  if (!AFF_FLAGGED(hider, AFF_HIDE) || AFF_FLAGGED(ch, AFF_TRUE_SIGHT) || HAS_FEAT(ch, FEAT_TRUE_SIGHT))
    return TRUE;

  /* do spot check here */
  bool can_see = FALSE, challenge = d20(ch), dc = (d20(hider) + 10);

  // challenger bonuses/penalty (ch)
  if (!IS_NPC(ch))
    challenge += compute_ability(ch, ABILITY_PERCEPTION);
  else /* NPC */
    challenge += GET_LEVEL(ch);
  if (AFF_FLAGGED(ch, AFF_SPOT))
    challenge += 10;

  // hider bonus/penalties (vict)
  if (!IS_NPC(hider))
  {
    dc += compute_ability((struct char_data *)hider, ABILITY_STEALTH);

    if (IN_NATURE(hider) && HAS_FEAT(hider, FEAT_TRACKLESS_STEP))
    {
      dc += 4;
    }
    if (IN_NATURE(hider) && HAS_FEAT(hider, FEAT_CAMOUFLAGE))
    {
      dc += 6;
    }

    /* NPC */
  }
  else
  {
    dc += GET_LEVEL(hider);
  }

  dc += (GET_SIZE(ch) - GET_SIZE(hider)) * 2; // size bonus

  if (challenge > dc)
    can_see = TRUE;

  return (can_see);
}

/* function to calculate a skill roll for given ch */
int skill_roll(struct char_data *ch, int skillnum)
{
  int roll = d20(ch);

  if (skillnum == ABILITY_DIPLOMACY && affected_by_spell(ch, SPELL_HONEYED_TONGUE))
  {
    roll = MAX(roll, d20(ch));
  }

  if (HAS_FEAT(ch, FEAT_ADAPTABILITY) && dice(1, 5) == 1)
    roll += 3;

  /*if (PRF_FLAGGED(ch, PRF_TAKE_TEN))
    roll = 10;*/

  roll += compute_ability(ch, skillnum);

  return roll;
}

/* function to perform a skill check */
int skill_check(struct char_data *ch, int skill, int dc)
{
  int result = skill_roll(ch, skill);

  if (result == dc) /* woo barely passed! */
    return 1;
  else if (result < dc) /*failed*/
    return 0;
  else
    return (result - dc);
}

/* deprecated code (skill notching) */

/* simple function to increase skills
   takes character structure and skillnum, based on chance, can
   increase skill permanently up to cap */
/* some easy configure values, this is percent chance of a used
   skill, such as bash increasing per use:  !rand_number(0, this)
 suggested:  75
 */
#define USE 75
/* some easy configure values, this is percent chance of a skill
   that is passive, such as dodge increasing per use:
   !rand_number(0, this)
   suggested:  (500) */
#define PASS 500
/* this define is for crafting skills, they increase much easier
 suggested:  (20) */
#define C_SKILL 20
/* for stricter crafting skill notching (fast crafting) */
#define C_SKILL_SLOW 100

void increase_skill(struct char_data *ch, int skillnum)
{
  int notched = FALSE;

  // if the skill isn't learned or is mastered, don't adjust
  if (GET_SKILL(ch, skillnum) < 1 || GET_SKILL(ch, skillnum) == 99)
    return;
  if (GET_SKILL(ch, skillnum) > 99)
  {
    GET_SKILL(ch, skillnum) = 99;
    return;
  }

  // We don't want to allow crafting skill improvements if they are
  // resizing to their racial size, since such resizing has no
  // gold cost.
  if (GET_CRAFTING_TYPE(ch) == SCMD_RESIZE)
  {
    if (GET_CRAFTING_OBJ(ch) && GET_OBJ_SIZE(GET_CRAFTING_OBJ(ch)) == GET_SIZE(ch))
      return;
  }

  int use = rand_number(0, USE);
  int pass = rand_number(0, PASS);
  int craft = rand_number(0, C_SKILL);
  int slow_craft = rand_number(0, C_SKILL_SLOW);

  switch (skillnum)
  {
  case SKILL_BACKSTAB:
    if (!use)
    {
      notched = TRUE;
      GET_SKILL(ch, skillnum)
      ++;
    }
    break;
  case SKILL_BASH:
    if (!use)
    {
      notched = TRUE;
      GET_SKILL(ch, skillnum)
      ++;
    }
    break;
  case SKILL_MUMMY_DUST:
    if (!use)
    {
      notched = TRUE;
      GET_SKILL(ch, skillnum)
      ++;
    }
    break;
  case SKILL_KICK:
    if (!use)
    {
      notched = TRUE;
      GET_SKILL(ch, skillnum)
      ++;
    }
    break;
  case SKILL_WEAPON_SPECIALIST:
    if (!pass)
    {
      notched = TRUE;
      GET_SKILL(ch, skillnum)
      ++;
    }
    break;
  case SKILL_WHIRLWIND:
    if (!use)
    {
      notched = TRUE;
      GET_SKILL(ch, skillnum)
      ++;
    }
    break;
  case SKILL_RESCUE:
    if (!use)
    {
      notched = TRUE;
      GET_SKILL(ch, skillnum)
      ++;
    }
    break;
  case SKILL_DRAGON_KNIGHT:
    if (!use)
    {
      notched = TRUE;
      GET_SKILL(ch, skillnum)
      ++;
    }
    break;
  case SKILL_LUCK_OF_HEROES:
    if (!pass)
    {
      notched = TRUE;
      GET_SKILL(ch, skillnum)
      ++;
    }
    break;
  case SKILL_TRACK:
    if (!use)
    {
      notched = TRUE;
      GET_SKILL(ch, skillnum)
      ++;
    }
    break;
  case SKILL_QUICK_CHANT:
    if (!use)
    {
      notched = TRUE;
      GET_SKILL(ch, skillnum)
      ++;
    }
    break;
  case SKILL_AMBIDEXTERITY:
    if (!pass)
    {
      notched = TRUE;
      GET_SKILL(ch, skillnum)
      ++;
    }
    break;
  case SKILL_DIRTY_FIGHTING:
    if (!pass)
    {
      notched = TRUE;
      GET_SKILL(ch, skillnum)
      ++;
    }
    break;
  case SKILL_DODGE:
    if (!pass)
    {
      notched = TRUE;
      GET_SKILL(ch, skillnum)
      ++;
    }
    break;
  case SKILL_IMPROVED_CRITICAL:
    if (!pass)
    {
      notched = TRUE;
      GET_SKILL(ch, skillnum)
      ++;
    }
    break;
  case SKILL_MOBILITY:
    if (!pass)
    {
      notched = TRUE;
      GET_SKILL(ch, skillnum)
      ++;
    }
    break;
  case SKILL_SPRING_ATTACK:
    if (!pass)
    {
      notched = TRUE;
      GET_SKILL(ch, skillnum)
      ++;
    }
    break;
  case SKILL_TOUGHNESS:
    if (!pass)
    {
      notched = TRUE;
      GET_SKILL(ch, skillnum)
      ++;
    }
    break;
  case SKILL_TWO_WEAPON_FIGHT:
    if (!pass)
    {
      notched = TRUE;
      GET_SKILL(ch, skillnum)
      ++;
    }
    break;
  case SKILL_FINESSE:
    if (!pass)
    {
      notched = TRUE;
      GET_SKILL(ch, skillnum)
      ++;
    }
    break;
  case SKILL_ARMOR_SKIN:
    if (!pass)
    {
      notched = TRUE;
      GET_SKILL(ch, skillnum)
      ++;
    }
    break;
  case SKILL_BLINDING_SPEED:
    if (!pass)
    {
      notched = TRUE;
      GET_SKILL(ch, skillnum)
      ++;
    }
    break;
  case SKILL_DAMAGE_REDUC_1:
    if (!pass)
    {
      notched = TRUE;
      GET_SKILL(ch, skillnum)
      ++;
    }
    break;
  case SKILL_DAMAGE_REDUC_2:
    if (!pass)
    {
      notched = TRUE;
      GET_SKILL(ch, skillnum)
      ++;
    }
    break;
  case SKILL_DAMAGE_REDUC_3:
    if (!pass)
    {
      notched = TRUE;
      GET_SKILL(ch, skillnum)
      ++;
    }
    break;
  case SKILL_EPIC_TOUGHNESS:
    if (!pass)
    {
      notched = TRUE;
      GET_SKILL(ch, skillnum)
      ++;
    }
    break;
  case SKILL_OVERWHELMING_CRIT:
    if (!pass)
    {
      notched = TRUE;
      GET_SKILL(ch, skillnum)
      ++;
    }
    break;
  case SKILL_SELF_CONCEAL_1:
    if (!pass)
    {
      notched = TRUE;
      GET_SKILL(ch, skillnum)
      ++;
    }
    break;
  case SKILL_SELF_CONCEAL_2:
    if (!pass)
    {
      notched = TRUE;
      GET_SKILL(ch, skillnum)
      ++;
    }
    break;
  case SKILL_SELF_CONCEAL_3:
    if (!pass)
    {
      notched = TRUE;
      GET_SKILL(ch, skillnum)
      ++;
    }
    break;
  case SKILL_TRIP:
    if (!use)
    {
      notched = TRUE;
      GET_SKILL(ch, skillnum)
      ++;
    }
    break;
  case SKILL_IMPROVED_WHIRL:
    if (!use)
    {
      notched = TRUE;
      GET_SKILL(ch, skillnum)
      ++;
    }
    break;
  case SKILL_CLEAVE:
    if (!pass)
    {
      notched = TRUE;
      GET_SKILL(ch, skillnum)
      ++;
    }
    break;
  case SKILL_GREAT_CLEAVE:
    if (!pass)
    {
      notched = TRUE;
      GET_SKILL(ch, skillnum)
      ++;
    }
    break;
  case SKILL_SPELLPENETRATE:
    if (!pass)
    {
      notched = TRUE;
      GET_SKILL(ch, skillnum)
      ++;
    }
    break;
  case SKILL_SPELLPENETRATE_2:
    if (!pass)
    {
      notched = TRUE;
      GET_SKILL(ch, skillnum)
      ++;
    }
    break;
  case SKILL_PROWESS:
    if (!pass)
    {
      notched = TRUE;
      GET_SKILL(ch, skillnum)
      ++;
    }
    break;
  case SKILL_EPIC_PROWESS:
    if (!pass)
    {
      notched = TRUE;
      GET_SKILL(ch, skillnum)
      ++;
    }
    break;
  case SKILL_EPIC_WILDSHAPE:
    if (!pass)
    {
      notched = TRUE;
      GET_SKILL(ch, skillnum)
      ++;
    }
    break;
  case SKILL_EPIC_2_WEAPON:
    if (!pass)
    {
      notched = TRUE;
      GET_SKILL(ch, skillnum)
      ++;
    }
    break;
  case SKILL_SPELLPENETRATE_3:
    if (!pass)
    {
      notched = TRUE;
      GET_SKILL(ch, skillnum)
      ++;
    }
    break;
  case SKILL_SPELL_RESIST_1:
    if (!pass)
    {
      notched = TRUE;
      GET_SKILL(ch, skillnum)
      ++;
    }
    break;
  case SKILL_SPELL_RESIST_2:
    if (!pass)
    {
      notched = TRUE;
      GET_SKILL(ch, skillnum)
      ++;
    }
    break;
  case SKILL_SPELL_RESIST_3:
    if (!pass)
    {
      notched = TRUE;
      GET_SKILL(ch, skillnum)
      ++;
    }
    break;
  case SKILL_SPELL_RESIST_4:
    if (!pass)
    {
      notched = TRUE;
      GET_SKILL(ch, skillnum)
      ++;
    }
    break;
  case SKILL_SPELL_RESIST_5:
    if (!pass)
    {
      notched = TRUE;
      GET_SKILL(ch, skillnum)
      ++;
    }
    break;
  case SKILL_INITIATIVE:
    if (!pass)
    {
      notched = TRUE;
      GET_SKILL(ch, skillnum)
      ++;
    }
    break;
  case SKILL_EPIC_CRIT:
    if (!pass)
    {
      notched = TRUE;
      GET_SKILL(ch, skillnum)
      ++;
    }
    break;
  case SKILL_IMPROVED_BASH:
    if (!use)
    {
      notched = TRUE;
      GET_SKILL(ch, skillnum)
      ++;
    }
    break;
  case SKILL_IMPROVED_TRIP:
    if (!use)
    {
      notched = TRUE;
      GET_SKILL(ch, skillnum)
      ++;
    }
    break;
  case SKILL_POWER_ATTACK:
    if (!pass)
    {
      notched = TRUE;
      GET_SKILL(ch, skillnum)
      ++;
    }
    break;
  case SKILL_EXPERTISE:
    if (!pass)
    {
      notched = TRUE;
      GET_SKILL(ch, skillnum)
      ++;
    }
    break;
  case SKILL_GREATER_RUIN:
    if (!use)
    {
      notched = TRUE;
      GET_SKILL(ch, skillnum)
      ++;
    }
    break;
  case SKILL_HELLBALL:
    if (!use)
    {
      notched = TRUE;
      GET_SKILL(ch, skillnum)
      ++;
    }
    break;
  case SKILL_EPIC_MAGE_ARMOR:
    if (!use)
    {
      notched = TRUE;
      GET_SKILL(ch, skillnum)
      ++;
    }
    break;
  case SKILL_EPIC_WARDING:
    if (!use)
    {
      notched = TRUE;
      GET_SKILL(ch, skillnum)
      ++;
    }
    break;
  case SKILL_CRIPPLING_CRITICAL:
    if (!use)
    {
      notched = TRUE;
      GET_SKILL(ch, skillnum)
      ++;
    }
    break;
  case SKILL_RAGE:
    if (!use)
    {
      notched = TRUE;
      GET_SKILL(ch, skillnum)
      ++;
    }
    break;
  case SKILL_DEFENSIVE_STANCE:
    if (!use)
    {
      notched = TRUE;
      GET_SKILL(ch, skillnum)
      ++;
    }
    break;
  case SKILL_PROF_MINIMAL:
    if (!pass)
    {
      notched = TRUE;
      GET_SKILL(ch, skillnum)
      ++;
    }
    break;
  case SKILL_PROF_BASIC:
    if (!pass)
    {
      notched = TRUE;
      GET_SKILL(ch, skillnum)
      ++;
    }
    break;
  case SKILL_PROF_ADVANCED:
    if (!pass)
    {
      notched = TRUE;
      GET_SKILL(ch, skillnum)
      ++;
    }
    break;
  case SKILL_PROF_MASTER:
    if (!pass)
    {
      notched = TRUE;
      GET_SKILL(ch, skillnum)
      ++;
    }
    break;
  case SKILL_PROF_EXOTIC:
    if (!pass)
    {
      notched = TRUE;
      GET_SKILL(ch, skillnum)
      ++;
    }
    break;
  case SKILL_PROF_LIGHT_A:
    if (!pass)
    {
      notched = TRUE;
      GET_SKILL(ch, skillnum)
      ++;
    }
    break;
  case SKILL_PROF_MEDIUM_A:
    if (!pass)
    {
      notched = TRUE;
      GET_SKILL(ch, skillnum)
      ++;
    }
    break;
  case SKILL_PROF_HEAVY_A:
    if (!pass)
    {
      notched = TRUE;
      GET_SKILL(ch, skillnum)
      ++;
    }
    break;
  case SKILL_PROF_SHIELDS:
    if (!pass)
    {
      notched = TRUE;
      GET_SKILL(ch, skillnum)
      ++;
    }
    break;
  case SKILL_PROF_T_SHIELDS:
    if (!pass)
    {
      notched = TRUE;
      GET_SKILL(ch, skillnum)
      ++;
    }
    break;
  case SKILL_MURMUR:
    if (!use)
    {
      notched = TRUE;
      GET_SKILL(ch, skillnum)
      ++;
    }
    break;
  case SKILL_PROPAGANDA:
    if (!use)
    {
      notched = TRUE;
      GET_SKILL(ch, skillnum)
      ++;
    }
    break;
  case SKILL_LOBBY:
    if (!use)
    {
      notched = TRUE;
      GET_SKILL(ch, skillnum)
      ++;
    }
    break;
  case SKILL_STUNNING_FIST:
    if (!use)
    {
      notched = TRUE;
      GET_SKILL(ch, skillnum)
      ++;
    }
    break;
  case SKILL_POWERFUL_BLOW:
    if (!use)
    {
      notched = TRUE;
      GET_SKILL(ch, skillnum)
      ++;
    }
    break;
  case SKILL_SURPRISE_ACCURACY:
    if (!use)
    {
      notched = TRUE;
      GET_SKILL(ch, skillnum)
      ++;
    }
    break;
  case SKILL_COME_AND_GET_ME:
    if (!use)
    {
      notched = TRUE;
      GET_SKILL(ch, skillnum)
      ++;
    }
    break;
  case SKILL_FEINT:
    if (!use)
    {
      notched = TRUE;
      GET_SKILL(ch, skillnum)
      ++;
    }
    break;

    /* crafting skills */
  case SKILL_MINING:
    if (!craft)
    {
      notched = TRUE;
      GET_SKILL(ch, skillnum)
      ++;
    }
    break;
  case SKILL_HUNTING:
    if (!craft)
    {
      notched = TRUE;
      GET_SKILL(ch, skillnum)
      ++;
    }
    break;
  case SKILL_FORESTING:
    if (!craft)
    {
      notched = TRUE;
      GET_SKILL(ch, skillnum)
      ++;
    }
    break;
  case SKILL_KNITTING:
    if (!craft)
    {
      notched = TRUE;
      GET_SKILL(ch, skillnum)
      ++;
    }
    break;
  case SKILL_CHEMISTRY:
    if (!craft)
    {
      notched = TRUE;
      GET_SKILL(ch, skillnum)
      ++;
    }
    break;
  case SKILL_ARMOR_SMITHING:
    if (!craft)
    {
      notched = TRUE;
      GET_SKILL(ch, skillnum)
      ++;
    }
    break;
  case SKILL_WEAPON_SMITHING:
    if (!craft)
    {
      notched = TRUE;
      GET_SKILL(ch, skillnum)
      ++;
    }
    break;
  case SKILL_JEWELRY_MAKING:
    if (!craft)
    {
      notched = TRUE;
      GET_SKILL(ch, skillnum)
      ++;
    }
    break;
  case SKILL_LEATHER_WORKING:
    if (!craft)
    {
      notched = TRUE;
      GET_SKILL(ch, skillnum)
      ++;
    }
    break;
  case SKILL_FAST_CRAFTER:
    if (!slow_craft)
    {
      notched = TRUE;
      GET_SKILL(ch, skillnum)
      ++;
    }
    break;
  case SKILL_BONE_ARMOR:
    if (!craft)
    {
      notched = TRUE;
      GET_SKILL(ch, skillnum)
      ++;
    }
    break;
  case SKILL_ELVEN_CRAFTING:
    if (!craft)
    {
      notched = TRUE;
      GET_SKILL(ch, skillnum)
      ++;
    }
    break;
  case SKILL_MASTERWORK_CRAFTING:
    if (!craft)
    {
      notched = TRUE;
      GET_SKILL(ch, skillnum)
      ++;
    }
    break;
  case SKILL_DRACONIC_CRAFTING:
    if (!craft)
    {
      notched = TRUE;
      GET_SKILL(ch, skillnum)
      ++;
    }
    break;
  case SKILL_DWARVEN_CRAFTING:
    if (!craft)
    {
      notched = TRUE;
      GET_SKILL(ch, skillnum)
      ++;
    }
    break;
    /* end crafting */

  case SKILL_LIGHTNING_REFLEXES:
    if (!pass)
    {
      notched = TRUE;
      GET_SKILL(ch, skillnum)
      ++;
    }
    break;
  case SKILL_GREAT_FORTITUDE:
    if (!pass)
    {
      notched = TRUE;
      GET_SKILL(ch, skillnum)
      ++;
    }
    break;
  case SKILL_IRON_WILL:
    if (!pass)
    {
      notched = TRUE;
      GET_SKILL(ch, skillnum)
      ++;
    }
    break;
  case SKILL_EPIC_REFLEXES:
    if (!pass)
    {
      notched = TRUE;
      GET_SKILL(ch, skillnum)
      ++;
    }
    break;
  case SKILL_EPIC_FORTITUDE:
    if (!pass)
    {
      notched = TRUE;
      GET_SKILL(ch, skillnum)
      ++;
    }
    break;
  case SKILL_EPIC_WILL:
    if (!pass)
    {
      notched = TRUE;
      GET_SKILL(ch, skillnum)
      ++;
    }
    break;
  case SKILL_SHIELD_SPECIALIST:
    if (!pass)
    {
      notched = TRUE;
      GET_SKILL(ch, skillnum)
      ++;
    }
    break;
  default:
    if (!pass)
    {
      notched = TRUE;
      GET_SKILL(ch, skillnum)
      ++;
    }
    return;
    ;
  }

  if (notched)
    send_to_char(ch, "\tMYou feel your skill in \tC%s\tM improve! Your skill at "
                     "\tC%s\tM is now %d!\tn",
                 spell_info[skillnum].name, spell_info[skillnum].name,
                 GET_SKILL(ch, skillnum));
  return;
}
#undef USE
#undef PASS
#undef C_SKILL

/** A portable random number function.
 * @param from The lower bounds of the random number.
 * @param to The upper bounds of the random number.
 * @retval int The resulting randomly generated number. */
int rand_number(int from, int to)
{
  /* error checking in case people call this incorrectly */
  if (from > to)
  {
    int tmp = from;
    from = to;
    to = tmp;
    log("SYSERR: rand_number() should be called with lowest, then highest. (%d, %d), not (%d, %d).", from, to, to, from);
  }

  /* This should always be of the form: ((float)(to - from + 1) * rand() /
   * (float)(RAND_MAX + from) + from); If you are using rand() due to historical
   * non-randomness of the lower bits in older implementations.  We always use
   * circle_random() though, which shouldn't have that problem. Mean and
   * standard deviation of both are identical (within the realm of statistical
   * identity) if the rand() implementation is non-broken. */
  return ((circle_random() % (to - from + 1)) + from);
}

/** floating-point version of the random number function above.
 * @param from The lower bounds of the random number.
 * @param to The upper bounds of the random number.
 * @retval int The resulting randomly generated number. */
float rand_float(float from, float to)
{
  float ret;
  /* error checking in case people call this incorrectly */
  if (from > to)
  {
    float tmp = from;
    from = to;
    to = tmp;
    log("SYSERR: rand_float() should be called with lowest, then highest. (%f, %f), not (%f, %f).", from, to, to, from);
  }

  /* This should always be of the form: ((float)(to - from + 1) * rand() /
   * (float)(RAND_MAX + from) + from); If you are using rand() due to historical
   * non-randomness of the lower bits in older implementations.  We always use
   * circle_random() though, which shouldn't have that problem. Mean and
   * standard deviation of both are identical (within the realm of statistical
   * identity) if the rand() implementation is non-broken. */
  // need to fix this line below , prolly the math.h portion
  // ret = fmod( ((float)(circle_random())), (to - from)) ;
  ret = 1.0;
  return (ret + from);
}

/** Simulates a single dice roll from one to many of a certain sized die.
 * @param num The number of dice to roll.
 * @param size The number of sides each die has, and hence the number range
 * of the die.
 * @retval int The sum of all the dice rolled. A random number. */
int dice(int num, int size)
{
  int sum = 0;

  if (size <= 0 || num <= 0)
    return (0);

  while (num-- > 0)
    sum += rand_number(1, size);

  return (sum);
}

/** Return the smaller number. Original note: Be wary of sign issues with this.
 * @param a The first number.
 * @param b The second number.
 * @retval int The smaller of the two, a or b. */
int MIN(int a, int b)
{
  return (a < b ? a : b);
}

float FLOATMIN(float a, float b)
{
  return (a < b ? a : b);
}

/** Return the larger number. Original note: Be wary of sign issues with this.
 * @param a The first number.
 * @param b The second number.
 * @retval int The larger of the two, a or b. */
int MAX(int a, int b)
{
  return (a > b ? a : b);
}

float FLOATMAX(float a, float b)
{
  return (a > b ? a : b);
}

/** Used to capitalize a string. Will not change any mud specific color codes.
 * @param txt The string to capitalize.
 * @retval char* Pointer to the capitalized string. */
char *CAP(char *txt)
{
  char *p = txt;

  /* Skip all preceeding color codes and ANSI codes */
  while ((*p == '\t' && *(p + 1)) || (*p == '\x1B' && *(p + 1) == '['))
  {
    if (*p == '\t')
      p += 2; /* Skip \t sign and color letter/number */
    else
    {
      p += 2; /* Skip the CSI section of the ANSI code */
      while (*p && !isalpha(*p))
        p++; /* Skip until a 'letter' is found */
      if (*p)
        p++; /* Skip the letter */
    }
  }

  if (*p)
    *p = UPPER(*p);
  return (txt);
}

/** Used to uncapitalize a string. Will not change any mud specific color codes.
 * @parUam txt The string to uncapitalize.
 * @retUval char* Pointer to the uncapitalized string. */
char *UNCAP(char *txt)
{
  char *p = txt;

  /* Skip all preceeding color codes and ANSI codes */
  while ((*p == '\t' && *(p + 1)) || (*p == '\x1B' && *(p + 1) == '['))
  {
    if (*p == '\t')
      p += 2; /* Skip \t sign and color letter/number */
    else
    {
      p += 2; /* Skip the CSI section of the ANSI code */
      while (*p && !isalpha(*p))
        p++; /* Skip until a 'letter' is found */
      if (*p)
        p++; /* Skip the letter */
    }
  }

  if (*p)
    *p = LOWER(*p);
  return (txt);
}

/*
Returns total length of the string that would have been created.
*/
size_t strlcat(char *buf, const char *src, size_t bufsz)
{
  size_t buf_len = strlen(buf);
  size_t src_len = strlen(src);
  size_t rtn = buf_len + src_len;

  if (buf_len < (bufsz - 1))
  {
    if (src_len >= (bufsz - buf_len))
    {
      src_len = bufsz - buf_len - 1;
    }
    memcpy(buf + buf_len, src, src_len);
    buf[buf_len + src_len] = '\0';
  }

  return rtn;
}

#if !defined(HAVE_STRLCPY)

/** A 'strlcpy' function in the same fashion as 'strdup' below. This copies up
 * to totalsize - 1 bytes from the source string, placing them and a trailing
 * NUL into the destination string. Returns the total length of the string it
 * tried to copy, not including the trailing NUL.  So a '>= totalsize' test
 * says it was truncated. (Note that you may have _expected_ truncation
 * because you only wanted a few characters from the source string.) Portable
 * function, in case your system does not have strlcpy. */
size_t strlcpy(char *dest, const char *source, size_t totalsize)
{
  strncpy(dest, source, totalsize - 1); /* strncpy: OK (we must assume 'totalsize' is correct) */
  dest[totalsize - 1] = '\0';
  return strlen(source);
}
#endif

#if !defined(HAVE_STRDUP)

/** Create a duplicate of a string function. Portable. */
char *strdup(const char *source)
{
  char *new_z;

  CREATE(new_z, char, strlen(source) + 1);
  return (strcpy(new_z, source)); /* strcpy: OK */
}
#endif

/** Strips "\r\n" from just the end of a string. Will not remove internal
 * "\r\n" values to the string.
 * @post Replaces any "\r\n" values at the end of the string with null.
 * @param txt The writable string to prune. */
void prune_crlf(char *txt)
{
  int i = strlen(txt) - 1;

  while (txt[i] == '\n' || txt[i] == '\r')
    txt[i--] = '\0';
}

#ifndef str_cmp

/** a portable, case-insensitive version of strcmp(). Returns: 0 if equal, > 0
 * if arg1 > arg2, or < 0 if arg1 < arg2. Scan until strings are found
 * different or we reach the end of both. */
int str_cmp(const char *arg1, const char *arg2)
{
  int chk, i;

  if (arg1 == NULL || arg2 == NULL)
  {
    log("SYSERR: str_cmp() passed a NULL pointer, %p or %p.", arg1, arg2);
    return (0);
  }

  for (i = 0; arg1[i] || arg2[i]; i++)
    if ((chk = LOWER(arg1[i]) - LOWER(arg2[i])) != 0)
      return (chk); /* not equal */

  return (0);
}
#endif

#ifndef strn_cmp

/** a portable, case-insensitive version of strncmp(). Returns: 0 if equal, > 0
 * if arg1 > arg2, or < 0 if arg1 < arg2. Scan until strings are found
 * different, the end of both, or n is reached. */
int strn_cmp(const char *arg1, const char *arg2, int n)
{
  int chk, i;

  if (arg1 == NULL || arg2 == NULL)
  {
    log("SYSERR: strn_cmp() passed a NULL pointer, %p or %p.", arg1, arg2);
    return (0);
  }

  for (i = 0; (arg1[i] || arg2[i]) && (n > 0); i++, n--)
    if ((chk = LOWER(arg1[i]) - LOWER(arg2[i])) != 0)
      return (chk); /* not equal */

  return (0);
}
#endif

/** New variable argument log() function; logs messages to disk.
 * Works the same as the old for previously written code but is very nice
 * if new code wishes to implment printf style log messages without the need
 * to make prior sprintf calls.
 * @param format The message to log. Standard printf formatting and variable
 * arguments are allowed.
 * @param args The comma delimited, variable substitutions to make in str. */
void basic_mud_vlog(const char *format, va_list args)
{
  time_t ct = time(0);
  char *time_s = asctime(localtime(&ct));

  if (logfile == NULL)
  {
    puts("SYSERR: Using log() before stream was initialized!");
    return;
  }

  if (format == NULL)
    format = "SYSERR: log() received a NULL format.";

  time_s[strlen(time_s) - 1] = '\0';

  fprintf(logfile, "%-15.15s :: ", time_s + 4);
  vfprintf(logfile, format, args);
  fputc('\n', logfile);
  fflush(logfile);
}

/** Log messages directly to syslog on disk, no display to in game immortals.
 * Supports variable string modification arguments, a la printf. Most likely
 * any calls to plain old log() have been redirected, via macro, to this
 * function.
 * @param format The message to log. Standard printf formatting and variable
 * arguments are allowed.
 * @param ... The comma delimited, variable substitutions to make in str. */
void basic_mud_log(const char *format, ...)
{
  va_list args;

  va_start(args, format);
  basic_mud_vlog(format, args);
  va_end(args);
}

/** Essentially the touch command. Create an empty file or update the modified
 * time of a file.
 * @param path The filepath to "touch." This filepath is relative to the /lib
 * directory relative to the root of the mud distribution.
 * @retval int 0 on a success, -1 on a failure; standard system call exit
 * values. */
int touch(const char *path)
{
  FILE *fl;

  if (!(fl = fopen(path, "a")))
  {
    log("SYSERR: %s: %s", path, strerror(errno));
    return (-1);
  }
  else
  {
    fclose(fl);
    return (0);
  }
}

/** Log mud messages to a file & to online imm's syslogs.
 * @param type The minimum syslog level that needs be set to see this message.
 * OFF, BRF, NRM and CMP are the values from lowest to highest. Using mudlog
 * with type = OFF should be avoided as every imm will see the message even
 * if they have syslog turned off.
 * @param level Minimum character level needed to see this message.
 * @param file TRUE log this to the syslog file, FALSE do not log this to disk.
 * @param str The message to log. Standard printf formatting and variable
 * arguments are allowed.
 * @param ... The comma delimited, variable substitutions to make in str. */
void mudlog(int type, int level, int file, const char *str, ...)
{
  char buf[MAX_STRING_LENGTH] = {'\0'};
  struct descriptor_data *i;
  va_list args;

  if (str == NULL)
    return; /* eh, oh well. */

  if (file)
  {
    va_start(args, str);
    basic_mud_vlog(str, args);
    va_end(args);
  }

  if (level < 0)
    return;

  strcpy(buf, "[ "); /* strcpy: OK */
  va_start(args, str);
  vsnprintf(buf + 2, sizeof(buf) - 6, str, args);
  va_end(args);
  strlcat(buf, " ]\tn\r\n", sizeof(buf)); /* strcat: OK */

  for (i = descriptor_list; i; i = i->next)
  {
    if (STATE(i) != CON_PLAYING || IS_NPC(i->character)) /* switch */
      continue;
    if (GET_LEVEL(i->character) < level)
      continue;
    if (PLR_FLAGGED(i->character, PLR_WRITING))
      continue;
    if (type > (PRF_FLAGGED(i->character, PRF_LOG1) ? 1 : 0) + (PRF_FLAGGED(i->character, PRF_LOG2) ? 2 : 0))
      continue;

    send_to_char(i->character, "%s%s%s", CCNRM(i->character, C_NRM), buf, CCNRM(i->character, C_NRM));
  }
}

/** Take a bitvector and return a human readable
 * description of which bits are set in it.
 * @pre The final element in the names array must contain a one character
 * string consisting of a single newline character "\n". Caller of function is
 * responsible for creating the memory buffer for the result string.
 * @param[in] bitvector The bitvector to test for set bits.
 * @param[in] names An array of human readable strings describing each possible
 * bit. The final element in this array must be a string made of a single
 * newline character (eg "\n").
 * If you don't have a 'const' array for the names param, cast it as such.
 * @param[out] result Holds the names of the set bits in bitvector. The bit
 * names will be delimited by a single space.
 * Caller of sprintbit is responsible for creating the buffer for result.
 * Will be set to "NOBITS" if no bits are set in bitvector (ie bitvector = 0).
 * @param[in] reslen The length of the available memory in the result buffer.
 * Ideally, results will be large enough to hold the description of every bit
 * that could possibly be set in bitvector.
 * @retval size_t The length of the string copied into result. */
size_t sprintbit(bitvector_t bitvector, const char *names[], char *result, size_t reslen)
{
  size_t len = 0;
  int nlen;
  long nr;

  *result = '\0';

  for (nr = 0; bitvector && len < reslen; bitvector >>= 1)
  {
    if (IS_SET(bitvector, 1))
    {
      nlen = snprintf(result + len, reslen - len, "%s ", *names[nr] != '\n' ? names[nr] : "UNDEFINED");
      if (len + nlen >= reslen || nlen < 0)
        break;
      len += nlen;
    }

    if (*names[nr] != '\n')
      nr++;
  }

  if (!*result)
    len = strlcpy(result, "None ", reslen);

  return (len);
}

/** Return the human readable name of a defined type.
 * @pre The final element in the names array must contain a one character
 * string consisting of a single newline character "\n". Caller of function is
 * responsible for creating the memory buffer for the result string.
 * @param[in] type The type number to be translated.
 * @param[in] names An array of human readable strings describing each possible
 * bit. The final element in this array must be a string made of a single
 * newline character (eg "\n").
 * @param[out] result Holds the translated name of the type.
 * Caller of sprintbit is responsible for creating the buffer for result.
 * Will be set to "UNDEFINED" if the type is greater than the number of names
 * available.
 * @param[in] reslen The length of the available memory in the result buffer.
 * @retval size_t The length of the string copied into result. */
size_t sprinttype(int type, const char *names[], char *result, size_t reslen)
{
  int nr = 0;

  while (type && *names[nr] != '\n')
  {
    type--;
    nr++;
  }

  return strlcpy(result, *names[nr] != '\n' ? names[nr] : "UNDEFINED", reslen);
}

/** Take a bitarray and return a human readable description of which bits are
 * set in it.
 * @pre The final element in the names array must contain a one character
 * string consisting of a single newline character "\n". Caller of function is
 * responsible for creating the memory buffer for the result string large enough
 * to hold all possible bit translations. There is no error checking for
 * possible array overflow for result.
 * @param[in] bitvector The bitarray in which to test for set bits.
 * @param[in] names An array of human readable strings describing each possible
 * bit. The final element in this array must be a string made of a single
 * newline character (eg "\n").
 * If you don't have a 'const' array for the names param, cast it as such.
 * @param[in] maxar The number of 'bytes' in the bitarray. This number will
 * usually be pre-defined for the particular bitarray you are using.
 * @param[out] result Holds the names of the set bits in bitarray. The bit
 * names are delimited by a single space. Ideally, results will be large enough
 * to hold the description of every bit that could possibly be set in bitvector.
 * Will be set to "NOBITS" if no bits are set in bitarray (ie all bits in the
 * bitarray are equal to 0).
 */
void sprintbitarray(int bitvector[], const char *names[], int maxar, char *result)
{
  int nr, teller, found = FALSE, count = 0;

  *result = '\0';

  for (teller = 0; teller < maxar && !found; teller++)
  {
    for (nr = 0; nr < 32 && !found; nr++)
    {
      if (IS_SET_AR(bitvector, (teller * 32) + nr))
      {
        if (*names[(teller * 32) + nr] != '\n')
        {
          if (*names[(teller * 32) + nr] != '\0')
          {
            strcat(result, names[(teller * 32) + nr]);
            strcat(result, " ");
            count++;
            if (count >= 8)
            {
              strcat(result, "\r\n");
              count = 0;
            }
          }
        }
        else
        {
          strcat(result, "UNDEFINED ");
        }
      }
      if (*names[(teller * 32) + nr] == '\n')
        found = TRUE;
    }
  }

  if (!*result)
    strcpy(result, "None ");
}

/** Calculate the REAL time passed between two time invervals.
 * @param t2 The later time.
 * @param t1 The earlier time.
 * @retval time_info_data The real hours, days, months and years passed between t2 and t1. */
struct time_info_data *real_time_passed(time_t t2, time_t t1)
{
  long secs;
  static struct time_info_data now;

  secs = t2 - t1;

  now.hours = (secs / SECS_PER_REAL_HOUR) % 24; /* 0..23 hours */
  secs -= SECS_PER_REAL_HOUR * now.hours;

  now.day = (secs / SECS_PER_REAL_DAY) % 35; /* 0..34 days  */
  secs -= SECS_PER_REAL_DAY * now.day;

  now.month = (secs / (SECS_PER_REAL_YEAR / 12)) % 12; /* 0..11 months */
  secs -= (SECS_PER_REAL_YEAR / 12) * now.month;

  now.year = (secs / SECS_PER_REAL_YEAR);
  secs -= SECS_PER_REAL_YEAR * now.year;

  return (&now);
}

/** Calculate the MUD time passed between two time invervals.
 * @param t2 The later time.
 * @param t1 The earlier time.
 * @retval time_info_data A pointer to the mud hours, days, months and years
 * that have passed between the two time intervals. DO NOT FREE the structure
 * pointed to by the return value. */
struct time_info_data *mud_time_passed(time_t t2, time_t t1)
{
  long secs;
  static struct time_info_data now;

  secs = t2 - t1;

  now.hours = (secs / SECS_PER_MUD_HOUR) % 24; /* 0..23 hours */
  secs -= SECS_PER_MUD_HOUR * now.hours;

  now.day = (secs / SECS_PER_MUD_DAY) % 35; /* 0..34 days  */
  secs -= SECS_PER_MUD_DAY * now.day;

  now.month = (secs / SECS_PER_MUD_MONTH) % 17; /* 0..16 months */
  secs -= SECS_PER_MUD_MONTH * now.month;

  now.year = (secs / SECS_PER_MUD_YEAR); /* 0..XX? years */

  return (&now);
}

/** Translate the current mud time to real seconds (in type time_t).
 * @param now The current mud time to translate into a real time unit.
 * @retval time_t The real time that would have had to have passed
 * to represent the mud time represented by the now parameter. */
time_t mud_time_to_secs(struct time_info_data *now)
{
  time_t when = 0;

  when += now->year * SECS_PER_MUD_YEAR;
  when += now->month * SECS_PER_MUD_MONTH;
  when += now->day * SECS_PER_MUD_DAY;
  when += now->hours * SECS_PER_MUD_HOUR;
  return (time(NULL) - when);
}

/** Calculate a player's MUD age.
 * @todo The minimum starting age of 17 is hardcoded in this function. Recommend
 * changing the minimum age to a property (variable) external to this function.
 * @param ch A valid player character.
 * @retval time_info_data A pointer to the mud age in years of the player
 * character. DO NOT FREE the structure pointed to by the return value. */
struct time_info_data *age(struct char_data *ch)
{
  static struct time_info_data player_age;

  player_age = *mud_time_passed(time(0), ch->player.time.birth);

  player_age.year += 17; /* All players start at 17 */

  return (&player_age);
}

/** Check if making ch follow victim will create an illegal follow loop. In
 * essence, this prevents someone from following a character in a group that
 * is already being lead by the character.
 * @param ch The character trying to follow.
 * @param victim The character being followed.
 * @retval bool TRUE if ch is already leading someone in victims group, FALSE
 * if it is okay for ch to follow victim. */
bool circle_follow(struct char_data *ch, struct char_data *victim)
{
  struct char_data *k;

  for (k = victim; k; k = k->master)
  {
    if (k == ch)
      return (TRUE);
  }

  return (FALSE);
}

/* had to create this to get the "meat" out of stop_follower() for application
   in other scenarios */
void stop_follower_engine(struct char_data *ch)
{
  struct follow_type *k = NULL;
  struct follow_type *j = NULL;

  if (ch->master->followers->follower == ch)
  { /* Head of follower-list? */
    k = ch->master->followers;
    ch->master->followers = k->next;
    free(k);
  }
  else
  { /* locate follower who is not head of list */
    for (k = ch->master->followers; k->next->follower != ch; k = k->next)
      ;

    j = k->next;
    k->next = j->next;
    free(j);
  }
}

/** Call on a character (NPC or PC) to stop them from following someone and
 * to break any charm affect.
 * @todo Make the messages returned from the broken charm affect more
 * understandable.
 * @pre ch MUST be following someone, else core dump.
 * @post The charm affect (AFF_CHARM) will be removed from the character and
 * the character will stop following the "master" they were following.
 * @param ch The character (NPC or PC) to stop from following.
 * */
void stop_follower(struct char_data *ch)
{

  /* Makes sure this function is not called when it shouldn't be called. */
  if (ch->master == NULL)
  {
    core_dump();
    return;
  }

  if (AFF_FLAGGED(ch, AFF_CHARM))
  {
    act("You realize that you do not need to serve $N anymore!",
        FALSE, ch, 0, ch->master, TO_CHAR);
    act("$n is no longer serving you!", FALSE, ch, 0, ch->master, TO_VICT);

    /* hope i got everything - zusuk */
    if (affected_by_spell(ch, SPELL_CHARM))
      affect_from_char(ch, SPELL_CHARM);
    if (affected_by_spell(ch, SPELL_CHARM_ANIMAL))
      affect_from_char(ch, SPELL_CHARM_ANIMAL);
    if (affected_by_spell(ch, SPELL_CHARM_MONSTER))
      affect_from_char(ch, SPELL_CHARM_MONSTER);
    if (affected_by_spell(ch, SPELL_DOMINATE_PERSON))
      affect_from_char(ch, SPELL_DOMINATE_PERSON);
    if (affected_by_spell(ch, SPELL_MASS_DOMINATION))
      affect_from_char(ch, SPELL_MASS_DOMINATION);
    if (affected_by_spell(ch, SPELL_CONTROL_PLANTS))
      affect_from_char(ch, SPELL_CONTROL_PLANTS);

    if (GROUP(ch))
    {
      leave_group(ch);
    }
  }
  else
  {
    if (ch->master)
    {
      act("You stop following $N.", FALSE, ch, 0, ch->master, TO_CHAR);
      act("$n stops following $N.", TRUE, ch, 0, ch->master, TO_NOTVICT);
      act("$n stops following you.", TRUE, ch, 0, ch->master, TO_VICT);
    }
  }

  stop_follower_engine(ch); /* moved this out to function above -zusuk */

  ch->master = NULL;
  REMOVE_BIT_AR(AFF_FLAGS(ch), AFF_CHARM);

  /* for cleanup -zusuk */
  if (IS_NPC(ch) && (MOB_FLAGGED(ch, MOB_C_ANIMAL) || MOB_FLAGGED(ch, MOB_C_FAMILIAR) ||
                     MOB_FLAGGED(ch, MOB_C_MOUNT) || MOB_FLAGGED(ch, MOB_ELEMENTAL) ||
                     MOB_FLAGGED(ch, MOB_ANIMATED_DEAD)))
    attach_mud_event(new_mud_event(ePURGEMOB, ch, NULL), (12 * PASSES_PER_SEC));
}

/** Finds the number of follows that are following, and charmed by, the
 * character (PC or NPC).
 * @param ch The character to check for charmed followers.
 * @retval int The number of followers that are also charmed by the character.
 */
int num_followers_charmed(struct char_data *ch)
{
  struct follow_type *lackey;
  int total = 0;

  for (lackey = ch->followers; lackey; lackey = lackey->next)
    if (AFF_FLAGGED(lackey->follower, AFF_CHARM) && lackey->follower->master == ch)
      total++;

  return (total);
}

/** Called when a character that follows/is followed dies. If the character
 * is the leader of a group, it stops everyone in the group from following
 * them. Despite the title, this function does not actually perform the kill on
 * the character passed in as the argument.
 * @param ch The character (NPC or PC) to stop from following.
 * */
void die_follower(struct char_data *ch)
{
  struct follow_type *j, *k;

  if (ch->master)
    stop_follower(ch);

  for (k = ch->followers; k; k = j)
  {
    j = k->next;
    stop_follower(k->follower);
  }
}

/** Adds a new follower to a leader (following).
 * @todo Maybe make circle_follow an inherent part of this function?
 * @pre Make sure to call circle_follow first. ch may also not already
 * be following anyone, otherwise core dump.
 * @param ch The character to follow.
 * @param leader The character to be followed. */
void add_follower(struct char_data *ch, struct char_data *leader)
{
  struct follow_type *k;

  if (ch->master)
  {
    core_dump();
    return;
  }

  ch->master = leader;

  CREATE(k, struct follow_type, 1);

  k->follower = ch;
  k->next = leader->followers;
  leader->followers = k;

  act("You now follow $N.", FALSE, ch, 0, leader, TO_CHAR);
  if (CAN_SEE(leader, ch))
    act("$n starts following you.", TRUE, ch, 0, leader, TO_VICT);
  act("$n starts to follow $N.", TRUE, ch, 0, leader, TO_NOTVICT);

  if (!IS_NPC(leader) && PRF_FLAGGED(leader, PRF_AUTO_GROUP) && GROUP(leader) && !GROUP(ch))
    join_group(ch, GROUP(leader));
}

/** Reads the next non-blank line off of the input stream. Empty lines are
 * skipped. Lines which begin with '*' are considered to be comments and are
 * skipped.
 * @pre Caller must allocate memory for buf.
 * @post If a there is a line to be read, the newline character is removed from
 * the file line ending and the string is returned. Else a null string is
 * returned in buf.
 * @param[in] fl The file to be read from.
 * @param[out] buf The next non-blank line read from the file. Buffer given must
 * be at least READ_SIZE (512) characters large.
 * @retval int The number of lines advanced in the file. */
int get_line(FILE *fl, char *buf)
{
  char temp[READ_SIZE];
  int lines = 0;
  int sl;

  do
  {
    if (!fgets(temp, READ_SIZE, fl))
      return (0);
    lines++;
  } while (*temp == '*' || *temp == '\n' || *temp == '\r');

  /* Last line of file doesn't always have a \n, but it should. */
  sl = strlen(temp);
  while (sl > 0 && (temp[sl - 1] == '\n' || temp[sl - 1] == '\r'))
    temp[--sl] = '\0';

  strcpy(buf, temp); /* strcpy: OK, if buf >= READ_SIZE (512) */
  return (lines);
}

/** Create the full path, relative to the library path, of the player type
 * file to open.
 * @todo Make the return type bool.
 * @pre Caller is responsible for allocating memory buffer for the created
 * file name.
 * @post The potential file path to open is created. This function does not
 * actually open any file descriptors.
 * @param[out] filename Buffer to store the full path of the file to open.
 * @param[in] fbufsize The maximum size of filename, and the maximum size
 * of the path that can be written to it.
 * @param[in] mode What type of files can be created. Currently, recognized
 * modes are CRASH_FILE, ETEXT_FILE, SCRIPT_VARS_FILE and PLR_FILE.
 * @param[in] orig_name The player name to create the filepath (of type mode)
 * for.
 * @retval int 0 if filename cannot be created, 1 if it can. */
int get_filename(char *filename, size_t fbufsize, int mode, const char *orig_name)
{
  const char *prefix, *middle, *suffix;
  char name[MAX_PATH], *ptr;

  if (orig_name == NULL || *orig_name == '\0' || filename == NULL)
  {
    log("SYSERR: NULL pointer or empty string passed to get_filename(), %p or %p.",
        orig_name, filename);
    return (0);
  }

  switch (mode)
  {
  case CRASH_FILE:
    prefix = LIB_PLROBJS;
    suffix = SUF_OBJS;
    break;
  case ETEXT_FILE:
    prefix = LIB_PLRTEXT;
    suffix = SUF_TEXT;
    break;
  case SCRIPT_VARS_FILE:
    prefix = LIB_PLRVARS;
    suffix = SUF_MEM;
    break;
  case PLR_FILE:
    prefix = LIB_PLRFILES;
    suffix = SUF_PLR;
    break;
  default:
    return (0);
  }

  strlcpy(name, orig_name, sizeof(name));
  for (ptr = name; *ptr; ptr++)
    *ptr = LOWER(*ptr);

  switch (LOWER(*name))
  {
  case 'a':
  case 'b':
  case 'c':
  case 'd':
  case 'e':
    middle = "A-E";
    break;
  case 'f':
  case 'g':
  case 'h':
  case 'i':
  case 'j':
    middle = "F-J";
    break;
  case 'k':
  case 'l':
  case 'm':
  case 'n':
  case 'o':
    middle = "K-O";
    break;
  case 'p':
  case 'q':
  case 'r':
  case 's':
  case 't':
    middle = "P-T";
    break;
  case 'u':
  case 'v':
  case 'w':
  case 'x':
  case 'y':
  case 'z':
    middle = "U-Z";
    break;
  default:
    middle = "ZZZ";
    break;
  }

  snprintf(filename, fbufsize, "%s%s" SLASH "%s.%s", prefix, middle, name, suffix);
  return (1);
}

/** Calculate the number of player characters (PCs) in the room. Any NPC (mob)
 * is not returned in the count.
 * @param room The room to check for PCs. */
int num_pc_in_room(struct room_data *room)
{
  int i = 0;
  struct char_data *ch;

  for (ch = room->people; ch != NULL; ch = ch->next_in_room)
    if (!IS_NPC(ch))
      i++;

  return (i);
}

/** This function (derived from basic fork() abort() idea by Erwin S Andreasen)
 * causes your MUD to dump core (assuming you can) but continue running. The
 * core dump will allow post-mortem debugging that is less severe than assert();
 * Don't call this directly as core_dump_unix() but as simply 'core_dump()' so
 * that it will be excluded from systems not supporting them. You still want to
 * call abort() or exit(1) for non-recoverable errors, of course. Wonder if
 * flushing streams includes sockets?
 * @param who The file in which this call was made.
 * @param line The line at which this call was made. */
void core_dump_real(const char *who, int line)
{
  log("SYSERR: Assertion failed at %s:%d!", who, line);

#if 1 /* By default, let's not litter. */
#if defined(CIRCLE_UNIX)
  /* These would be duplicated otherwise...make very sure. */
  fflush(stdout);
  fflush(stderr);
  fflush(logfile);
  /* Everything, just in case, for the systems that support it. */
  fflush(NULL);

  /* Kill the child so the debugger or script doesn't think the MUD crashed.
   * The 'autorun' script would otherwise run it again. */
  if (fork() == 0)
    abort();
#endif
#endif
}

/** Count the number bytes taken up by color codes in a string that will be
 * empty space once the color codes are converted and made non-printable.
 * @param string The string in which to check for color codes.
 * @retval int the number of color codes found. */
int count_color_chars(char *string)
{
  int i, len;
  int num = 0;

  if (!string || !*string)
    return 0;

  len = strlen(string);
  for (i = 0; i < len; i++)
  {
    while (string[i] == '\t')
    {
      if (string[i + 1] == '\t')
        num++;
      else
        num += 2;
      i += 2;
    }
  }
  return num;
}

/* Not the prettiest thing I've ever written but it does the task which
 * is counting all characters in a string which are not part of the
 * protocol system. This is with the exception of detailed MXP codes. */
int count_non_protocol_chars(const char *str)
{
  int count = 0;
  const char *string = str;

  while (*string)
  {
    if (*string == '\r' || *string == '\n')
    {
      string++;
      continue;
    }
    if (*string == '@' || *string == '\t')
    {
      string++;
      if (*string != '[' && *string != '<' && *string != '>' && *string != '(' && *string != ')')
        string++;
      else if (*string == '[')
      {
        while (*string && *string != ']')
          string++;
        string++;
      }
      else
        string++;
      continue;
    }
    count++;
    string++;
  }

  return count;
}

/* simple test to check if the given ch has infravision */
bool char_has_infra(struct char_data *ch)
{
  if (AFF_FLAGGED(ch, AFF_INFRAVISION))
    return TRUE;

  if (HAS_FEAT(ch, FEAT_INFRAVISION))
    return TRUE;

  if (GET_RACE(ch) == RACE_ELF)
    return TRUE;
  if (GET_RACE(ch) == RACE_DWARF)
    return TRUE;
  if (GET_RACE(ch) == RACE_CRYSTAL_DWARF)
    return TRUE;
  if (GET_RACE(ch) == RACE_H_ELF)
    return TRUE;
  if (GET_RACE(ch) == RACE_HALFLING)
    return TRUE;
  if (GET_RACE(ch) == RACE_GNOME)
    return TRUE;

  return FALSE;
}

/* simple test to check if the given ch has ultra (perfect dark vision) */
bool char_has_ultra(struct char_data *ch)
{
  if (AFF_FLAGGED(ch, AFF_ULTRAVISION))
    return TRUE;

  if (AFF_FLAGGED(ch, AFF_DARKVISION))
    return TRUE;

  if (HAS_FEAT(ch, FEAT_ULTRAVISION))
    return TRUE;

  if (char_has_infra(ch)) return TRUE;

  if (GET_RACE(ch) == RACE_HALF_TROLL)
    return TRUE;
  if (GET_RACE(ch) == RACE_H_ORC)
    return TRUE;
  if (GET_RACE(ch) == RACE_DROW)
    return TRUE;
  if (GET_RACE(ch) == RACE_HALF_DROW)
    return TRUE;
  if (GET_RACE(ch) == RACE_DUERGAR)
    return TRUE;
  if (GET_RACE(ch) == RACE_TRELUX)
    return TRUE;
  if (GET_RACE(ch) == RACE_LICH)
    return TRUE;
  if (GET_RACE(ch) == RACE_VAMPIRE)
    return TRUE;

  return FALSE;
}

/* test to see if a given room is "daylit", useful for dayblind races, etc
 rules detailed in code comments -zusuk */
bool room_is_daylit(room_rnum room)
{
  if (!VALID_ROOM_RNUM(room))
  {
    log("room_is_daylit: Invalid room rnum %d. (0-%d)", room, top_of_world);
    return (FALSE);
  }

  if (ROOM_AFFECTED(room, RAFF_LIGHT))
    return (TRUE);

  /* disqualifiers */
  /* sectors */
  if (SECT(room) == SECT_INSIDE)
    return (FALSE);
  if (SECT(room) == SECT_INSIDE_ROOM)
    return (FALSE);
  if (SECT(room) == SECT_UNDERWATER)
    return (FALSE);
  if (SECT(room) == SECT_PLANES)
    return (FALSE);
  if (SECT(room) == SECT_UD_WILD)
    return (FALSE);
  if (SECT(room) == SECT_UD_CITY)
    return (FALSE);
  if (SECT(room) == SECT_UD_INSIDE)
    return (FALSE);
  if (SECT(room) == SECT_UD_WATER)
    return (FALSE);
  if (SECT(room) == SECT_UD_NOSWIM)
    return (FALSE);
  if (SECT(room) == SECT_UD_NOGROUND)
    return (FALSE);
  if (SECT(room) == SECT_CAVE)
    return (FALSE);
  /* room flags */
  if (ROOM_FLAGGED(room, ROOM_DARK))
    return (FALSE);
  if (ROOM_FLAGGED(room, ROOM_MAGICDARK))
    return (FALSE);
  /* room affections */
  if (ROOM_AFFECTED(room, RAFF_DARKNESS))
    return (FALSE);
  /* time/weather system */
  if (weather_info.sunlight == SUN_SET || weather_info.sunlight == SUN_DARK)
    return (FALSE);

  /* room is not indoors, room is not 'dark', room is not affected by darkness
     it is NOT sun-set and NOT sun-dark */
  return (TRUE);
}

/** Tests to see if a room is dark. Rules (unless overridden by ROOM_DARK):
 * Inside and City rooms are always lit. Outside rooms are dark at sunset and
 * night.
 * @param room The real room to test for.
 * @retval int FALSE if the room is lit, TRUE if the room is dark. */
bool room_is_dark(room_rnum room)
{
  if (!VALID_ROOM_RNUM(room))
  {
    log("room_is_dark: Invalid room rnum %d. (0-%d)", room, top_of_world);
    return (FALSE);
  }

  if (SECT(room) == SECT_INSIDE || SECT(room) == SECT_INSIDE_ROOM || SECT(room) == SECT_CITY)
    return (FALSE);

  if (ROOM_FLAGGED(room, ROOM_MAGICLIGHT))
    return (FALSE);

  if (ROOM_AFFECTED(room, RAFF_LIGHT))
    return (FALSE);

  struct char_data *tch = NULL;
  for (tch = world[room].people; tch; tch = tch->next_in_room)
  {
    // persons blinded by blinding ray emit light like a sunrod
    if (affected_by_spell(tch, SPELL_BLINDING_RAY))
      return (FALSE);
    if (HAS_FEAT(tch, FEAT_AURA_OF_LIGHT))
      return FALSE;
  }

  /* magic-dark flag will over-ride lights */
  if (ROOM_FLAGGED(room, ROOM_MAGICDARK))
    return (TRUE);

  for (tch = world[room].people; tch; tch = tch->next_in_room)
  {
    // persons impaled by a holy javelin emit light like a torch, which will not penetrate magical darkness
    if (char_has_mud_event(tch, eHOLYJAVELIN))
      return (FALSE);
  }

  if (world[room].light)
    return (FALSE);

  if (ROOM_FLAGGED(room, ROOM_DARK))
    return (TRUE);

  if (weather_info.sunlight == SUN_SET || weather_info.sunlight == SUN_DARK)
    return (TRUE);

  if (ROOM_AFFECTED(room, RAFF_DARKNESS))
    return (TRUE);

  /* sectors dark by nature */
  if (SECT(room) == SECT_UNDERWATER)
    return (TRUE);
  if (SECT(room) == SECT_UD_WILD)
    return (TRUE);
  if (SECT(room) == SECT_UD_CITY)
    return (TRUE);
  if (SECT(room) == SECT_UD_INSIDE)
    return (TRUE);
  if (SECT(room) == SECT_UD_WATER)
    return (TRUE);
  if (SECT(room) == SECT_UD_NOSWIM)
    return (TRUE);
  if (SECT(room) == SECT_UD_NOGROUND)
    return (TRUE);
  if (SECT(room) == SECT_CAVE)
    return (TRUE);
  /* end sectors */

  return (FALSE);
}

// returns true if there are sunlight conditions in specified room,
// mainly used to check for vampire abilities and weaknesses
bool is_room_in_sunlight(room_rnum room)
{
  if (room == NOWHERE)
    return false;
  if (!ROOM_OUTDOORS(room))
    return false;
  if (weather_info.sunlight != SUN_LIGHT || weather_info.sky != SKY_CLOUDLESS)
    return false;
  if (ROOM_FLAGGED(room, ROOM_DARK) && ROOM_FLAGGED(room, ROOM_MAGICDARK))
    return false;
  if (ROOM_FLAGGED(room, ROOM_FOG))
    return false;
  if (ROOM_AFFECTED(room, RAFF_DARKNESS) ||
      ROOM_AFFECTED(room, RAFF_ACID_FOG) ||
      ROOM_AFFECTED(room, RAFF_BILLOWING) ||
      ROOM_AFFECTED(room, RAFF_OBSCURING_MIST) ||
      ROOM_AFFECTED(room, RAFF_FOG))
    return false;
  /* sectors dark by nature */
  if (SECT(room) == SECT_UNDERWATER)
    return false;
  if (SECT(room) == SECT_UD_WILD)
    return false;
  if (SECT(room) == SECT_UD_CITY)
    return false;
  if (SECT(room) == SECT_UD_INSIDE)
    return false;
  if (SECT(room) == SECT_UD_WATER)
    return false;
  if (SECT(room) == SECT_UD_NOSWIM)
    return false;
  if (SECT(room) == SECT_UD_NOGROUND)
    return false;
  if (SECT(room) == SECT_CAVE)
    return false;

  return true;
}

/** Calculates the Levenshtein distance between two strings. Currently used
 * by the mud to make suggestions to the player when commands are mistyped.
 * This function is most useful when an index of possible choices are available
 * and the results of this function are constrained and used to help narrow
 * down the possible choices. For more information about Levenshtein distance,
 * recommend doing an internet or wikipedia search.
 * @param s1 The input string.
 * @param s2 The string to be compared to.
 * @retval int The Levenshtein distance between s1 and s2. */
int levenshtein_distance(const char *s1, const char *s2)
{
  int **d, i, j;
  int s1_len = strlen(s1), s2_len = strlen(s2);

  CREATE(d, int *, s1_len + 1);

  for (i = 0; i <= s1_len; i++)
  {
    CREATE(d[i], int, s2_len + 1);
    d[i][0] = i;
  }

  for (j = 0; j <= s2_len; j++)
    d[0][j] = j;
  for (i = 1; i <= s1_len; i++)
    for (j = 1; j <= s2_len; j++)
      d[i][j] = MIN(d[i - 1][j] + 1, MIN(d[i][j - 1] + 1,
                                         d[i - 1][j - 1] + ((s1[i - 1] == s2[j - 1]) ? 0 : 1)));

  i = d[s1_len][s2_len];

  for (j = 0; j <= s1_len; j++)
    free(d[j]);
  free(d);

  return i;
}

/** Removes a character from a piece of furniture. Unlike some of the other
 * _from_ functions, this does not place the character into NOWHERE.
 * @post ch is unattached from the furniture object.
 * @param ch The character to remove from the furniture object.
 */
void char_from_furniture(struct char_data *ch)
{
  struct obj_data *furniture;
  struct char_data *tempch;

  if (!SITTING(ch))
    return;

  if (!(furniture = SITTING(ch)))
  {
    log("SYSERR: No furniture for char in char_from_furniture.");
    SITTING(ch) = NULL;
    NEXT_SITTING(ch) = NULL;
    return;
  }

  if (!(tempch = OBJ_SAT_IN_BY(furniture)))
  {
    log("SYSERR: Char from furniture, but no furniture!");
    SITTING(ch) = NULL;
    NEXT_SITTING(ch) = NULL;
    GET_OBJ_VAL(furniture, 1) = 0;
    return;
  }

  if (tempch == ch)
  {
    if (!NEXT_SITTING(ch))
    {
      OBJ_SAT_IN_BY(furniture) = NULL;
    }
    else
    {
      OBJ_SAT_IN_BY(furniture) = NEXT_SITTING(ch);
    }
  }
  else
  {
    for (tempch = OBJ_SAT_IN_BY(furniture); tempch; tempch = NEXT_SITTING(tempch))
    {
      if (NEXT_SITTING(tempch) == ch)
      {
        NEXT_SITTING(tempch) = NEXT_SITTING(ch);
      }
    }
  }
  GET_OBJ_VAL(furniture, 1) -= 1;
  SITTING(ch) = NULL;
  NEXT_SITTING(ch) = NULL;

  if (GET_OBJ_VAL(furniture, 1) < 1)
  {
    OBJ_SAT_IN_BY(furniture) = NULL;
    GET_OBJ_VAL(furniture, 1) = 0;
  }

  return;
}

void char_from_buff_targets(struct char_data *ch)
{
  struct char_data *tch;

  for (tch = character_list; tch; tch = tch->next)
  {
    if (!IS_NPC(tch) && GET_BUFF_TARGET(tch) == ch)
      GET_BUFF_TARGET(tch) = NULL;
  }
}

/* column_list
   The list is output in a fixed format, and only the number of columns can be adjusted
   This function will output the list to the player
   Vars:
     ch          - the player
     num_cols    - the desired number of columns
     list        - a pointer to a list of strings
     list_length - So we can work with lists that don't end with /n
     show_nums   - when set to TRUE, it will show a number before the list entry.
 */
void column_list(struct char_data *ch, int num_cols, const char **list, int list_length, bool show_nums)
{
  int num_per_col, col_width, r, c, i, offset = 0, len = 0, temp_len, max_len = 0;
  char buf[MAX_STRING_LENGTH] = {'\0'};

  /* Work out the longest list item */
  for (i = 0; i < list_length; i++)
    if (max_len < strlen(list[i]))
      max_len = strlen(list[i]);

  /* auto columns case */
  if (num_cols == 0)
  {
    num_cols = (IS_NPC(ch) ? 80 : GET_SCREEN_WIDTH(ch)) / (max_len + (show_nums ? 5 : 1));
  }

  /* Ensure that the number of columns is in the range 1-10 */
  num_cols = MIN(MAX(num_cols, 1), 10);

  /* Work out the longest list item */
  for (i = 0; i < list_length; i++)
    if (max_len < strlen(list[i]))
      max_len = strlen(list[i]);

  /* Calculate the width of each column */
  if (IS_NPC(ch))
    col_width = 80 / num_cols;
  else
    col_width = (GET_SCREEN_WIDTH(ch)) / num_cols;

  if (show_nums)
    col_width -= 4;

  if (col_width < max_len)
    log("Warning: columns too narrow for correct output to %s in simple_column_list (utils.c)", GET_NAME(ch));

  /* Calculate how many list items there should be per column */
  num_per_col = (list_length / num_cols) + ((list_length % num_cols) ? 1 : 0);

  /* Fill 'buf' with the columnised list */
  for (r = 0; r < num_per_col; r++)
  {
    for (c = 0; c < num_cols; c++)
    {
      offset = (c * num_per_col) + r;
      if (offset < list_length)
      {
        if (show_nums)
          temp_len = snprintf(buf + len, sizeof(buf) - len, "%2d) %-*s", offset + 1, col_width, list[(offset)]);
        else
          temp_len = snprintf(buf + len, sizeof(buf) - len, "%-*s", col_width, list[(offset)]);
        len += temp_len;
      }
    }
    temp_len = snprintf(buf + len, sizeof(buf) - len, "\r\n");
    len += temp_len;
  }

  if (len >= sizeof(buf))
    snprintf((buf + MAX_STRING_LENGTH) - 22, 22, "\r\n*** OVERFLOW ***\r\n");

  /* Send the list to the player */
  page_string(ch->desc, buf, TRUE);
}

/* column_list
   The list is output in a fixed format, and only the number of columns can be adjusted
   This function will output the list to the player
   Vars:
     ch          - the player
     num_cols    - the desired number of columns
     list        - a pointer to a list of strings
     list_length - So we can work with lists that don't end with /n
     show_nums   - when set to TRUE, it will show a number before the list entry.
 */
void column_list_applies(struct char_data *ch, struct obj_data *obj, int num_cols, const char **list, int list_length, bool show_nums)
{

  if (!ch || !obj)
    return;

  int num_per_col, col_width, r, c, i, offset = 0, len = 0, temp_len, max_len = 0;
  char buf[MAX_STRING_LENGTH] = {'\0'};
  bool highlight = false;

  /* Work out the longest list item */
  for (i = 0; i < list_length; i++)
    if (max_len < strlen(list[i]))
      max_len = strlen(list[i]);

  /* auto columns case */
  if (num_cols == 0)
  {
    num_cols = (IS_NPC(ch) ? 80 : GET_SCREEN_WIDTH(ch)) / (max_len + (show_nums ? 5 : 1));
  }

  /* Ensure that the number of columns is in the range 1-10 */
  num_cols = MIN(MAX(num_cols, 1), 10);

  /* Work out the longest list item */
  for (i = 0; i < list_length; i++)
    if (max_len < strlen(list[i]))
      max_len = strlen(list[i]);

  /* Calculate the width of each column */
  if (IS_NPC(ch))
    col_width = 80 / num_cols;
  else
    col_width = (GET_SCREEN_WIDTH(ch)) / num_cols;

  if (show_nums)
    col_width -= 4;

  if (col_width < max_len)
    log("Warning: columns too narrow for correct output to %s in simple_column_list (utils.c)", GET_NAME(ch));

  /* Calculate how many list items there should be per column */
  num_per_col = (list_length / num_cols) + ((list_length % num_cols) ? 1 : 0);

  /* Fill 'buf' with the columnised list */
  for (r = 0; r < num_per_col; r++)
  {
    for (c = 0; c < num_cols; c++)
    {
      offset = (c * num_per_col) + r;
      if (offset < list_length)
      {
        highlight = highlight_apply_by_obj(obj, offset);         
        
        if (show_nums)
          temp_len = snprintf(buf + len, sizeof(buf) - len, "%s%2d) %-*s\tn", highlight ? "\tC" : "", offset + 1, col_width, list[(offset)]);
        else
          temp_len = snprintf(buf + len, sizeof(buf) - len, "%s%-*s\tn", highlight ? "\tC" : "", col_width, list[(offset)]);
        len += temp_len;
      }
    }
    temp_len = snprintf(buf + len, sizeof(buf) - len, "\r\n");
    len += temp_len;
  }

  if (len >= sizeof(buf))
    snprintf((buf + MAX_STRING_LENGTH) - 22, 22, "\r\n*** OVERFLOW ***\r\n");

  /* Send the list to the player */
  page_string(ch->desc, buf, TRUE);
}

/**
 * Search through a string array of flags for a particular flag.
 * @param flag_list An array of flag name strings. The final element must
 * be a string made up of a single newline.
 * @param flag_name The name to search in flag_list.
 * @retval int Returns the element number in flag_list of flag_name or
 * NOFLAG (-1) if no match.
 */
int get_flag_by_name(const char *flag_list[], char *flag_name)
{
  int i = 0;
  for (; flag_list[i] && *flag_list[i] && strcmp(flag_list[i], "\n") != 0; i++)
    if (!strcmp(flag_list[i], flag_name))
      return (i);
  return (NOFLAG);
}

/**
 * Reads a certain number of lines from the begining of a file, like performing
 * a 'head'.
 * @pre Expects an already open file and the user to supply enough memory
 * in the output buffer to hold the lines read from the file. Assumes the
 * file is a text file. Expects buf to be nulled out if the entire buf is
 * to be used, otherwise, appends file information beyond the first null
 * character. lines_to_read is assumed to be a positive number.
 * @post Rewinds the file pointer to the beginning of the file. If buf is
 * too small to handle the requested output, **OVERFLOW** is appended to the
 * buffer.
 * @param[in] file A pointer to an already successfully opened file.
 * @param[out] buf Buffer to hold the data read from the file. Will not
 * overwrite preexisting information in a non-null string.
 * @param[in] bufsize The total size of the buffer.
 * @param[in] lines_to_read The number of lines to be read from the front of
 * the file.
 * @retval int The number of lines actually read from the file. Can be used
 * the compare with the number of lines requested to be read to determine if the
 * entire file was read. If lines_to_read is <= 0, no processing occurs
 * and lines_to_read is returned.
 */
int file_head(FILE *file, char *buf, size_t bufsize, int lines_to_read)
{
  /* Local variables */
  int lines_read = 0;                            /* The number of lines read so far. */
  char line[READ_SIZE];                          /* Retrieval buffer for file. */
  size_t buflen;                                 /* Amount of previous existing data in buffer. */
  int readstatus = 1;                            /* Are we at the end of the file? */
  int n = 0;                                     /* Return value from snprintf. */
  const char *overflow = "\r\n**OVERFLOW**\r\n"; /* Appended if overflow. */

  /* Quick check for bad arguments. */
  if (lines_to_read <= 0)
  {
    return lines_to_read;
  }

  /* Initialize local variables not already initialized. */
  buflen = strlen(buf);

  /* Read from the front of the file. */
  rewind(file);

  while ((lines_read < lines_to_read) &&
         (readstatus > 0) && (buflen < bufsize))
  {
    /* Don't use get_line to set lines_read because get_line will return
     * the number of comments skipped during reading. */
    readstatus = get_line(file, line);

    if (readstatus > 0)
    {
      n = snprintf(buf + buflen, bufsize - buflen, "%s\r\n", line);
      buflen += n;
      lines_read++;
    }
  }

  /* Check to see if we had a potential buffer overflow. */
  if (buflen >= bufsize)
  {
    /* We should never see this case, but... */
    if ((strlen(overflow) + 1) >= bufsize)
    {
      core_dump();
      snprintf(buf, bufsize, "%s", overflow);
    }
    else
    {
      /* Append the overflow statement to the buffer. */
      snprintf(buf + buflen - strlen(overflow) - 1, strlen(overflow) + 1, "%s", overflow);
    }
  }

  rewind(file);

  /* Return the number of lines. */
  return lines_read;
}

/**
 * Reads a certain number of lines from the end of the file, like performing
 * a 'tail'.
 * @pre Expects an already open file and the user to supply enough memory
 * in the output buffer to hold the lines read from the file. Assumes the
 * file is a text file. Expects buf to be nulled out if the entire buf is
 * to be used, otherwise, appends file information beyond the first null
 * character in buf. lines_to_read is assumed to be a positive number.
 * @post Rewinds the file pointer to the beginning of the file. If buf is
 * too small to handle the requested output, **OVERFLOW** is appended to the
 * buffer.
 * @param[in] file A pointer to an already successfully opened file.
 * @param[out] buf Buffer to hold the data read from the file. Will not
 * overwrite preexisting information in a non-null string.
 * @param[in] bufsize The total size of the buffer.
 * @param[in] lines_to_read The number of lines to be read from the back of
 * the file.
 * @retval int The number of lines actually read from the file. Can be used
 * the compare with the number of lines requested to be read to determine if the
 * entire file was read. If lines_to_read is <= 0, no processing occurs
 * and lines_to_read is returned.
 */
int file_tail(FILE *file, char *buf, size_t bufsize, int lines_to_read)
{
  /* Local variables */
  int lines_read = 0;                            /* The number of lines read so far. */
  int total_lines = 0;                           /* The total number of lines in the file. */
  char c;                                        /* Used to fast forward the file. */
  char line[READ_SIZE];                          /* Retrieval buffer for file. */
  size_t buflen;                                 /* Amount of previous existing data in buffer. */
  int readstatus = 1;                            /* Are we at the end of the file? */
  int n = 0;                                     /* Return value from snprintf. */
  const char *overflow = "\r\n**OVERFLOW**\r\n"; /* Appended if overflow. */

  /* Quick check for bad arguments. */
  if (lines_to_read <= 0)
  {
    return lines_to_read;
  }

  /* Initialize local variables not already initialized. */
  buflen = strlen(buf);
  total_lines = file_numlines(file); /* Side effect: file is rewound. */

  /* Fast forward to the location we should start reading from */
  while (((lines_to_read + lines_read) < total_lines))
  {
    do
    {
      c = fgetc(file);
    } while (c != '\n');

    lines_read++;
  }

  /* We reuse the lines_read counter. */
  lines_read = 0;

  /** From here on, we perform just like file_head */
  while ((lines_read < lines_to_read) &&
         (readstatus > 0) && (buflen < bufsize))
  {
    /* Don't use get_line to set lines_read because get_line will return
     * the number of comments skipped during reading. */
    readstatus = get_line(file, line);

    if (readstatus > 0)
    {
      n = snprintf(buf + buflen, bufsize - buflen, "%s\r\n", line);
      buflen += n;
      lines_read++;
    }
  }

  /* Check to see if we had a potential buffer overflow. */
  if (buflen >= bufsize)
  {
    /* We should never see this case, but... */
    if ((strlen(overflow) + 1) >= bufsize)
    {
      core_dump();
      snprintf(buf, bufsize, "%s", overflow);
    }
    else
    {
      /* Append the overflow statement to the buffer. */
      snprintf(buf + buflen - strlen(overflow) - 1, strlen(overflow) + 1, "%s", overflow);
    }
  }

  rewind(file);

  /* Return the number of lines read. */
  return lines_read;
}

/** Returns the byte size of a file. We assume size_t to be a large enough type
 * to handle all of the file sizes in the mud, and so do not make SIZE_MAX
 * checks.
 * @pre file parameter must already be opened.
 * @post file will be rewound.
 * @param file The file to determine the size of.
 * @retval size_t The byte size of the file (we assume no errors will be
 * encountered in this function).
 */
size_t file_sizeof(FILE *file)
{
  size_t numbytes = 0;

  rewind(file);

  /* It would be so much easier to do a byte count if an fseek SEEK_END and
   * ftell pair of calls was portable for text files, but all information
   * I've found says that getting a file size from ftell for text files is
   * not portable. Oh well, this method should be extremely fast for the
   * relatively small filesizes in the mud, and portable, too. */
  while (!feof(file))
  {
    fgetc(file);
    numbytes++;
  }

  rewind(file);

  return numbytes;
}

/** Returns the number of newlines '\n' in a file, which we equate to number of
 * lines. We assume the int type more than adequate to count the number of lines
 * and do not make checks for overrunning INT_MAX.
 * @pre file parameter must already be opened.
 * @post file will be rewound.
 * @param file The file to determine the size of.
 * @retval size_t The byte size of the file (we assume no errors will be
 * encountered in this function).
 */
int file_numlines(FILE *file)
{
  int numlines = 0;
  char c;

  rewind(file);

  while (!feof(file))
  {
    c = fgetc(file);
    if (c == '\n')
    {
      numlines++;
    }
  }

  rewind(file);

  return numlines;
}

/** A string converter designed to deal with the compile sensitive IDXTYPE.
 * Relies on the friendlier strtol function.
 * @pre Assumes that NOWHERE, NOTHING, NOBODY, NOFLAG, etc are all equal.
 * @param str_to_conv A string of characters to attempt to convert to an
 * IDXTYPE number.
 * @retval IDXTYPE A valid index number, or NOWHERE if not valid.
 */
IDXTYPE atoidx(const char *str_to_conv)
{
  long int result;

  /* Check for errors */
  errno = 0;

  result = strtol(str_to_conv, NULL, 10);

  if (errno || (result > IDXTYPE_MAX) || (result < 0))
    return NOWHERE; /* All of the NO* settings should be the same */
  else
    return (IDXTYPE)result;
}

/*
   strfrmt (String Format) function
   Used by automap/map system
   Re-formats a string to fit within a particular size box.
   Recognises @ color codes, and if a line ends in one color, the
   next line will start with the same color.
   Ends every line with \tn to prevent color bleeds.
 */
char *strfrmt(char *str, int w, int h, int justify, int hpad, int vpad)
{
  static char ret[MAX_STRING_LENGTH] = {'\0'};
  char line[MAX_INPUT_LENGTH] = {'\0'};
  char *sp = str;
  char *lp = line;
  char *rp = ret;
  char *wp = NULL;
  int wlen = 0, llen = 0, lcount = 0;
  char last_color = 'n';
  bool new_line_started = FALSE;

  memset(line, '\0', MAX_INPUT_LENGTH);
  /* Nomalize spaces and newlines */
  /* Split into lines, including convert \\ into \r\n */
  while (*sp)
  {
    /* eat leading space */
    while (*sp && isspace_ignoretabs(*sp))
      sp++;
    /* word begins */
    wp = sp;
    wlen = 0;
    while (*sp)
    { /* Find the end of the word */
      if (isspace_ignoretabs(*sp))
        break;
      if (*sp == '\\' && sp[1] && sp[1] == '\\')
      {
        if (sp != wp)
          break; /* Finish dealing with the current word */
        sp += 2; /* Eat the marker and any trailing space */
        while (*sp && isspace_ignoretabs(*sp))
          sp++;
        wp = sp;
        /* Start a new line */
        if (hpad)
          for (; llen < w; llen++)
            *lp++ = ' ';
        *lp++ = '\r';
        *lp++ = '\n';
        *lp++ = '\0';
        rp += sprintf(rp, "%s", line);
        llen = 0;
        lcount++;
        lp = line;
      }
      else if (*sp == '`' || *sp == '$' || *sp == '#')
      {
        if (sp[1] && (sp[1] == *sp))
          wlen++; /* One printable char here */
        sp += 2;  /* Eat the whole code regardless */
      }
      else if (*sp == '\t' && sp[1])
      {
        char MXPcode = (sp[1] == '[' ? ']' : sp[1] == '<' ? '>'
                                                          : '\0');

        if (!MXPcode)
          last_color = sp[1];

        sp += 2; /* Eat the code */
        if (MXPcode)
        {
          while (*sp != '\0' && *sp != MXPcode)
            ++sp; /* Eat the rest of the code */
        }
      }
      else
      {
        wlen++;
        sp++;
      }
    }
    if (llen + wlen + (lp == line ? 0 : 1) > w)
    {
      /* Start a new line */
      if (hpad)
        for (; llen < w; llen++)
          *lp++ = ' ';
      *lp++ = '\t'; /* 'normal' color */
      *lp++ = 'n';
      *lp++ = '\r'; /* New line */
      *lp++ = '\n';
      *lp++ = '\0';
      sprintf(rp, "%s", line);
      rp += strlen(line);
      llen = 0;
      lcount++;
      lp = line;
      if (last_color != 'n')
      {
        *lp++ = '\t'; /* restore previous color */
        *lp++ = last_color;
        new_line_started = TRUE;
      }
    }
    /* add word to line */
    if (lp != line && new_line_started != TRUE)
    {
      *lp++ = ' ';
      llen++;
    }
    new_line_started = FALSE;
    llen += wlen;
    for (; wp != sp; *lp++ = *wp++)
      ;
  }
  /* Copy over the last line */
  if (lp != line)
  {
    if (hpad)
      for (; llen < w; llen++)
        *lp++ = ' ';
    *lp++ = '\r';
    *lp++ = '\n';
    *lp++ = '\0';
    sprintf(rp, "%s", line);
    rp += strlen(line);
    lcount++;
  }
  if (vpad)
  {
    while (lcount < h)
    {
      if (hpad)
      {
        memset(rp, ' ', w);
        rp += w;
      }
      *rp++ = '\r';
      *rp++ = '\n';
      lcount++;
    }
    *rp = '\0';
  }
  return ret;
}

/**
   Takes two long strings (multiple lines) and joins them side-by-side.
   Used by the automap/map system
   @param str1 The string to be displayed on the left.
   @param str2 The string to be displayed on the right.
   @param joiner ???.
   @retval char * Pointer to the output to be displayed?
 */
const char *strpaste(const char *str1, const char *str2, const char *joiner)
{
  static char ret[MAX_STRING_LENGTH + 1];
  const char *sp1 = str1;
  const char *sp2 = str2;
  char *rp = ret;
  int jlen = strlen(joiner);

  while ((rp - ret) < MAX_STRING_LENGTH && (*sp1 || *sp2))
  {
    /* Copy line from str1 */
    while ((rp - ret) < MAX_STRING_LENGTH && *sp1 && !ISNEWL(*sp1))
      *rp++ = *sp1++;
    /* Eat the newline */
    if (*sp1)
    {
      if (sp1[1] && sp1[1] != sp1[0] && ISNEWL(sp1[1]))
        sp1++;
      sp1++;
    }

    /* Add the joiner */
    if ((rp - ret) + jlen >= MAX_STRING_LENGTH)
      break;
    strcpy(rp, joiner);
    rp += jlen;

    /* Copy line from str2 */
    while ((rp - ret) < MAX_STRING_LENGTH && *sp2 && !ISNEWL(*sp2))
      *rp++ = *sp2++;
    /* Eat the newline */
    if (*sp2)
    {
      if (sp2[1] && sp2[1] != sp2[0] && ISNEWL(sp2[1]))
        sp2++;
      sp2++;
    }

    /* Add the newline */
    if ((rp - ret) + 2 >= MAX_STRING_LENGTH)
      break;
    *rp++ = '\r';
    *rp++ = '\n';
  }
  /* Close off the string */
  *rp = '\0';
  return ret;
}

/* with given name, returns character structure if found */
struct char_data *is_playing(char *vict_name)
{
  extern struct descriptor_data *descriptor_list;
  struct descriptor_data *i, *next_i;
  char name_copy[MAX_NAME_LENGTH + 1];

  /* We need to CAP the name, so make a copy otherwise original is damaged */
  snprintf(name_copy, MAX_NAME_LENGTH + 1, "%s", vict_name);

  for (i = descriptor_list; i; i = next_i)
  {
    next_i = i->next;
    if (i->connected == CON_PLAYING && !strcmp(i->character->player.name, CAP(name_copy)))
      return i->character;
  }
  return NULL;
}

/* for displaying large numbers with commas */
char *add_commas(long num)
{
  int i, j = 0, len;
  int negative = (num < 0);
  char num_string[MAX_INPUT_LENGTH] = {'\0'};
  static char commastring[MAX_INPUT_LENGTH] = {'\0'};

  snprintf(num_string, sizeof(num_string), "%ld", num);
  len = strlen(num_string);

  for (i = 0; num_string[i]; i++)
  {
    if ((len - i) % 3 == 0 && i && i - negative)
      commastring[j++] = ',';
    commastring[j++] = num_string[i];
  }
  commastring[j] = '\0';

  return commastring;
}

/* Create a blank affect struct */
void new_affect(struct affected_type *af)
{
  int i;
  af->spell = 0;
  af->duration = 0;
  af->modifier = 0;
  af->location = APPLY_NONE;
  af->bonus_type = BONUS_TYPE_ENHANCEMENT;
  af->specific = 0;

  for (i = 0; i < AF_ARRAY_MAX; i++)
    af->bitvector[i] = 0;
}

/* Free an affect struct */
void free_affect(struct affected_type *af)
{
  // struct damage_reduction_type *dr;
  if (af == NULL)
    return;
  free(af);
}

/* Handy function to get class ID number by name (abbreviations allowed) */
int get_class_by_name(char *classname)
{
  int i;

  for (i = 0; i < NUM_CLASSES; i++)
    if (is_abbrev(classname, CLSLIST_NAME(i)))
      return (i);

  return (-1);
}

/* Handy function to get race ID number by name (abbreviations allowed) */
int get_race_by_name(char *racename)
{
  int i;

  for (i = 0; i < NUM_RACES; i++)
    if (is_abbrev(racename, race_list[i].type))
      return (i);

  return (-1);
}

/* Handy function to get subrace ID number by name (abbreviations allowed) */
int get_subrace_by_name(char *racename)
{
  int i;

  for (i = 0; i < NUM_SUB_RACES; i++)
    if (is_abbrev(racename, npc_subrace_types[i]))
      return (i);

  return (-1);
}

/* parse tabs function */
char *convert_from_tabs(char *string)
{
  static char buf[MAX_STRING_LENGTH * 8];

  strcpy(buf, string);
  parse_tab(buf);
  return (buf);
}

/* this function takes old-system alignment and converts it to new */
int convert_alignment(int align)
{
  if (align >= 800)
    return LAWFUL_GOOD;
  if (align >= 575 && align < 800)
    return NEUTRAL_GOOD;
  if (align >= 350 && align < 575)
    return CHAOTIC_GOOD;
  if (align >= 125 && align < 350)
    return LAWFUL_NEUTRAL;
  if (align < 125 && align > -125)
    return TRUE_NEUTRAL;
  if (align <= -125 && align > -350)
    return CHAOTIC_NEUTRAL;
  if (align <= -350 && align > -575)
    return LAWFUL_EVIL;
  if (align <= -575 && align > -800)
    return NEUTRAL_EVIL;
  if (align <= -800)
    return CHAOTIC_EVIL;

  /* shouldn't get here */
  return TRUE_NEUTRAL;
}

void set_alignment(struct char_data *ch, int alignment)
{
  switch (alignment)
  {
  case LAWFUL_GOOD:
    GET_ALIGNMENT(ch) = 900;
    break;
  case NEUTRAL_GOOD:
    GET_ALIGNMENT(ch) = 690;
    break;
  case CHAOTIC_GOOD:
    GET_ALIGNMENT(ch) = 460;
    break;
  case LAWFUL_NEUTRAL:
    GET_ALIGNMENT(ch) = 235;
    break;
  case TRUE_NEUTRAL:
    GET_ALIGNMENT(ch) = 0;
    break;
  case CHAOTIC_NEUTRAL:
    GET_ALIGNMENT(ch) = -235;
    break;
  case LAWFUL_EVIL:
    GET_ALIGNMENT(ch) = -460;
    break;
  case NEUTRAL_EVIL:
    GET_ALIGNMENT(ch) = -690;
    break;
  case CHAOTIC_EVIL:
    GET_ALIGNMENT(ch) = -900;
    break;
  default:
    break;
  }
}
/* return colored two-letter string referring to alignment */

/* also have in constants.c
 * const char *alignment_names[] = {
 */
const char *get_align_by_num_cnd(int align)
{
  if (align >= 800)
    return "\tWLG\tn";
  if (align >= 575 && align < 800)
    return "\tWNG\tn";
  if (align >= 350 && align < 575)
    return "\tWCG\tn";
  if (align >= 125 && align < 350)
    return "\tcLN\tn";
  if (align < 125 && align > -125)
    return "\tcTN\tn";
  if (align <= -125 && align > -350)
    return "\tcCN\tn";
  if (align <= -350 && align > -575)
    return "\tDLE\tn";
  if (align <= -575 && align > -800)
    return "\tDNE\tn";
  if (align <= -800)
    return "\tDCE\tn";

  return "??";
}

/* return colored full string referring to alignment */

/* also have in constants.c
 * const char *alignment_names[] = {
 */
const char *get_align_by_num(int align)
{
  if (align >= 800)
    return "\tYLawful \tWGood\tn";
  if (align >= 575 && align < 800)
    return "\tcNeutral \tWGood\tn";
  if (align >= 350 && align < 575)
    return "\tRChaotic \tWGood\tn";
  if (align >= 125 && align < 350)
    return "\tYLawful \tcNeutral\tn";
  if (align < 125 && align > -125)
    return "\tcTrue Neutral\tn";
  if (align <= -125 && align > -350)
    return "\tRChaotic \tcNeutral\tn";
  if (align <= -350 && align > -575)
    return "\tYLawful \tDEvil\tn";
  if (align <= -575 && align > -800)
    return "\tcNeutral \tDEvil\tn";
  if (align <= -800)
    return "\tRChaotic \tDEvil\tn";

  return "Unknown";
}
/* Feats */
int get_feat_value(struct char_data *ch, int featnum)
{
  struct obj_data *obj;
  struct char_data *mob = NULL;
  int i = 0, j = 0;
  int featval = 0;

  if ((featnum <= FEAT_UNDEFINED) || (featnum >= FEAT_LAST_FEAT))
  {
    log("SYSERR: %s called get_feat_value with invalid featnum: %d", GET_NAME(ch), featnum);
    return 0;
  }

  /* Check for the feat. */
  if (IS_NPC(ch))
    featval = MOB_HAS_FEAT(ch, featnum);
  else if (AFF_FLAGGED(ch, AFF_WILD_SHAPE) && GET_DISGUISE_RACE(ch))
    featval = MOB_HAS_FEAT(ch, featnum);
  else
  {
    /* check if we got this feat equipped */
    for (j = 0; j < NUM_WEARS; j++)
    {
      if ((obj = GET_EQ(ch, j)) == NULL)
        continue;
      for (i = 0; i < MAX_OBJ_AFFECT; i++)
      {
        if (obj->affected[i].location == APPLY_FEAT && obj->affected[i].modifier == featnum)
        {
          featval++;
          break; /* capped at +1, sorry folks */
        }
      }
    }
    featval += HAS_REAL_FEAT(ch, featnum);
    if ((mob = get_mob_follower(ch, MOB_EIDOLON)))
    {
      if (HAS_EVOLUTION(mob, EVOLUTION_RIDER_BOND) && featnum == FEAT_MOUNTED_COMBAT)
        featval++;
    }
  }

  return featval;
}

int find_armor_type(int specType)
{

  switch (specType)
  {

  case SPEC_ARMOR_TYPE_PADDED:
  case SPEC_ARMOR_TYPE_LEATHER:
  case SPEC_ARMOR_TYPE_STUDDED_LEATHER:
  case SPEC_ARMOR_TYPE_LIGHT_CHAIN:
    return ARMOR_TYPE_LIGHT;

  case SPEC_ARMOR_TYPE_HIDE:
  case SPEC_ARMOR_TYPE_SCALE:
  case SPEC_ARMOR_TYPE_CHAINMAIL:
  case SPEC_ARMOR_TYPE_PIECEMEAL:
    return ARMOR_TYPE_MEDIUM;

  case SPEC_ARMOR_TYPE_SPLINT:
  case SPEC_ARMOR_TYPE_BANDED:
  case SPEC_ARMOR_TYPE_HALF_PLATE:
  case SPEC_ARMOR_TYPE_FULL_PLATE:
    return ARMOR_TYPE_HEAVY;

  case SPEC_ARMOR_TYPE_BUCKLER:
  case SPEC_ARMOR_TYPE_SMALL_SHIELD:
  case SPEC_ARMOR_TYPE_LARGE_SHIELD:
  case SPEC_ARMOR_TYPE_TOWER_SHIELD:
    return ARMOR_TYPE_SHIELD;
  }
  return ARMOR_TYPE_LIGHT;
}

/* did CH successfully making his saving-throw? */
int savingthrow(struct char_data *ch, int save, int modifier, int dc)
{
  int roll = d20(ch);

  /* 1 is an automatic failure. */
  if (roll == 1)
    return FALSE;

  /* 20 is an automatic success. */
  if (roll >= 20)
    return TRUE;

  roll += compute_mag_saves(ch, save, modifier);

  if (roll >= dc)
    return TRUE;
  else
    return FALSE;
}

/* Utilities for managing daily use abilities for players. */

int get_daily_uses(struct char_data *ch, int featnum)
{
  int daily_uses = 0;

  switch (featnum)
  {
    case FEAT_QUICK_CHANT:
    case FEAT_QUICK_MIND:
      daily_uses = 2;
      break;
    case FEAT_TOUCH_OF_UNDEATH:
      if (CLASS_LEVEL(ch, CLASS_NECROMANCER) >= 10)
        daily_uses = 3;
      else if (CLASS_LEVEL(ch, CLASS_NECROMANCER) >= 8)
        daily_uses = 2;
      else
        daily_uses = 1;
      break;
    case FEAT_STRENGTH_OF_HONOR:
      daily_uses = CLASS_LEVEL(ch, CLASS_KNIGHT_OF_THE_CROWN);
      break;
    case FEAT_COSMIC_UNDERSTANDING:
      daily_uses = 3;
      break;
    case FEAT_DRAGOON_POINTS:
      daily_uses = CLASS_LEVEL(ch, CLASS_DRAGONRIDER) * ((GET_DRAGON_BOND_TYPE(ch) == DRAGON_BOND_MAGE) + 1);
      break;
    case FEAT_CROWN_OF_KNIGHTHOOD:
      daily_uses = 1;
      break;
    case FEAT_SOUL_OF_KNIGHTHOOD:
      daily_uses = 1;
      break;
    case FEAT_RALLYING_CRY:
      daily_uses = 3;
      break;
    case FEAT_INSPIRE_COURAGE:
      daily_uses = 1 + HAS_FEAT(ch, FEAT_INSPIRE_COURAGE);
      break;
    case FEAT_WISDOM_OF_THE_MEASURE:
      daily_uses = 2;
      break;
    case FEAT_FINAL_STAND:
      daily_uses = 1;
      break;
    case FEAT_KNIGHTHOODS_FLOWER:
      daily_uses = 1;
      break;
    case FEAT_VAMPIRE_CHILDREN_OF_THE_NIGHT:
      daily_uses = 1;
      break;
    case FEAT_VAMPIRE_BLOOD_DRAIN:
      daily_uses = 2 + (GET_LEVEL(ch) / 3);
      break;
    case FEAT_STONES_ENDURANCE:
      daily_uses += 2 + GET_LEVEL(ch) / 6;
      break;
    case FEAT_VAMPIRE_ENERGY_DRAIN:
      daily_uses = 1 + (GET_LEVEL(ch) / 3);
      break;
    case FEAT_STUNNING_FIST:
      daily_uses += CLASS_LEVEL(ch, CLASS_MONK) + (GET_LEVEL(ch) - CLASS_LEVEL(ch, CLASS_MONK)) / 4;
      break;
    case FEAT_LAYHANDS:
      daily_uses += CLASS_LEVEL(ch, CLASS_PALADIN) / 2 + GET_CHA_BONUS(ch);
      break;
    case FEAT_JUDGEMENT:
      daily_uses += ((CLASS_LEVEL(ch, CLASS_INQUISITOR) - 1) / 3) + 1;
      break;
    case FEAT_TRUE_JUDGEMENT:
      daily_uses += CLASS_LEVEL(ch, CLASS_INQUISITOR) / 5;
      break;
    case FEAT_BANE:
      daily_uses += 2 + CLASS_LEVEL(ch, CLASS_INQUISITOR) / 5;
      break;
    case FEAT_TOUCH_OF_CORRUPTION:
      daily_uses += CLASS_LEVEL(ch, CLASS_BLACKGUARD) / 2 + GET_CHA_BONUS(ch);
      break;
    case FEAT_TURN_UNDEAD:
    case FEAT_CHANNEL_ENERGY:
      daily_uses += 3 + GET_CHA_BONUS(ch) + HAS_FEAT(ch, FEAT_EXTRA_TURNING) * 2;
      break;
    case FEAT_BARDIC_MUSIC:
      daily_uses += CLASS_LEVEL(ch, CLASS_BARD);
      break;
    case FEAT_REMOVE_DISEASE:
      daily_uses += HAS_FEAT(ch, FEAT_REMOVE_DISEASE);
      break;
    case FEAT_CHANNEL_SPELL:
      daily_uses += 2 + HAS_FEAT(ch, FEAT_CHANNEL_SPELL);
      break;
    case FEAT_PSIONIC_FOCUS:
      daily_uses += (CLASS_LEVEL(ch, CLASS_PSIONICIST) >= 1) ? 1 + (CLASS_LEVEL(ch, CLASS_PSIONICIST) / 10) : 0;
      break;
    case FEAT_DOUBLE_MANIFEST:
      daily_uses = 1 + MAX(0, (CLASS_LEVEL(ch, CLASS_PSIONICIST) - 14) / 5);
      break;
    case FEAT_LICH_TOUCH:
      daily_uses = 3 + GET_INT_BONUS(ch);
      break;
    case FEAT_LICH_FEAR:
      daily_uses = 3;
      break;
    case FEAT_SLA_INVIS:
    case FEAT_SLA_STRENGTH:
    case FEAT_SLA_ENLARGE:
    case FEAT_CRYSTAL_FIST:
    case FEAT_INSECTBEING:
    case FEAT_CRYSTAL_BODY:
    case FEAT_SLA_FAERIE_FIRE:
    case FEAT_SLA_LEVITATE:
    case FEAT_SLA_DARKNESS:
    case FEAT_MASTER_OF_THE_MIND:
      daily_uses = 3;
      break;
    case FEAT_SHADOW_ILLUSION:
      daily_uses += CLASS_LEVEL(ch, CLASS_SHADOWDANCER) / 2;
      break;
    case FEAT_SHADOW_CALL:
    case FEAT_SHADOW_JUMP:
      daily_uses += MAX(0, (CLASS_LEVEL(ch, CLASS_SHADOWDANCER) - 2) / 2);
      break;
    case FEAT_SHADOW_POWER:
      daily_uses += MAX(0, (CLASS_LEVEL(ch, CLASS_SHADOWDANCER) - 6) / 2);
      break;
    case FEAT_BATTLE_RAGE:/*fallthrough*/
    case FEAT_MASS_INVIS:/*fallthrough*/
    case FEAT_COPYCAT:/*fallthrough*/
    case FEAT_DESTRUCTIVE_AURA:
      daily_uses = GET_WIS_BONUS(ch);
      break;
    case FEAT_AURA_OF_PROTECTION:/*fallthrough*/
    case FEAT_BLESSED_TOUCH:/*fallthrough*/
    case FEAT_EYE_OF_KNOWLEDGE:/*fallthrough*/
    case FEAT_HEALING_TOUCH:/*fallthrough*/
    case FEAT_GOOD_TOUCH: /*fallthrough*/
    case FEAT_FIRE_BOLT: /* fallthrough */
    case FEAT_ICICLE: /* fallthrough */
    case FEAT_ACID_DART: /* fallthrough */
    case FEAT_CURSE_TOUCH: /* fallthrough */
    case FEAT_DESTRUCTIVE_SMITE: /* fallthrough */
    case FEAT_EVIL_TOUCH:/*fallthrough*/
    case FEAT_LIGHTNING_ARC:
      daily_uses = 3 + GET_WIS_BONUS(ch);
      break;
    case FEAT_SEEKER_ARROW:
    case FEAT_IMBUE_ARROW:
      daily_uses += HAS_FEAT(ch, featnum) * 2;
      break;
    case FEAT_INVISIBLE_ROGUE:
      daily_uses += 1 + HAS_FEAT(ch, featnum) + GET_INT_BONUS(ch); 
      break;
    case FEAT_IMPROMPTU_SNEAK_ATTACK:
      daily_uses += 1 + HAS_FEAT(ch, featnum); 
    break;
    case FEAT_SMITE_EVIL:/*fallthrough*/
    case FEAT_SMITE_GOOD:/*fallthrough*/
    case FEAT_RAGE:/*fallthrough*/
    case FEAT_SACRED_FLAMES:/*fallthrough*/
    case FEAT_INNER_FIRE:/*fallthrough*/
    case FEAT_DEFENSIVE_STANCE:/*fallthrough*/
    case FEAT_QUIVERING_PALM:/*fallthrough*/
    case FEAT_ARROW_OF_DEATH:/*fallthrough*/
    case FEAT_SWARM_OF_ARROWS:/*fallthrough*/
    case FEAT_WILD_SHAPE:/*fallthrough*/
    case FEAT_ANIMATE_DEAD:/*fallthrough*/
    case FEAT_VANISH:/*fallthrough*/
      daily_uses += HAS_FEAT(ch, featnum);
      break;
    case FEAT_DRACONIC_HERITAGE_BREATHWEAPON:
      if (CLASS_LEVEL(ch, CLASS_SORCERER) >= 9) daily_uses++;
      if (CLASS_LEVEL(ch, CLASS_SORCERER) >= 17) daily_uses++;
      if (CLASS_LEVEL(ch, CLASS_SORCERER) >= 20) daily_uses++;
      break;
    case FEAT_PIXIE_DUST:
      daily_uses = GET_REAL_CHA(ch) + 4;
      break;
    case FEAT_EFREETI_MAGIC:
      daily_uses = 10;
      break;
    case FEAT_DRAGON_MAGIC:
      daily_uses = 10;
      break;
    case FEAT_TABAXI_CATS_CLAWS:
      daily_uses += MAX(1, GET_LEVEL(ch) / 3);
      break;
    case FEAT_DRACONIC_HERITAGE_CLAWS:
      daily_uses += 3 + GET_CHA_BONUS(ch);
      break;
    case FEAT_DRAGONBORN_BREATH:
      daily_uses = 3;
      break;
    case FEAT_MUTAGEN:
      daily_uses = 1;
      break;
    case FEAT_TINKER:
      daily_uses = 1;
      break;
    case FEAT_PSYCHOKINETIC:
      daily_uses = 1;
      break;
    case FEAT_METAMAGIC_ADEPT:
      daily_uses = 1;
      break;
    case FEAT_CURING_TOUCH:
      if (KNOWS_DISCOVERY(ch, ALC_DISC_HEALING_TOUCH))
        daily_uses += CLASS_LEVEL(ch, CLASS_ALCHEMIST);
      else if (KNOWS_DISCOVERY(ch, ALC_DISC_SPONTANEOUS_HEALING))
        daily_uses += CLASS_LEVEL(ch, CLASS_ALCHEMIST) / 2;
      else
        daily_uses = -1;
      break;
  }

  return daily_uses;
}

/* Function to create an event, based on the mud_event passed in, that will either:
 * 1.) Create a new event with the proper sVariables
 * 2.) Update an existing event with a new sVariable value
 *
 * Returns the current number of uses on cooldown. */
int start_daily_use_cooldown(struct char_data *ch, int featnum)
{
  struct mud_event_data *pMudEvent = NULL;
  int uses = 0, daily_uses = 0;
  char buf[MAX_STRING_LENGTH];
  event_id iId = 0;

  /* Transform the feat number to the event id for that ability. */
  if ((iId = feat_list[featnum].event) == eNULL)
  {
    log("SYSERR: in start_daily_use_cooldown, no cooldown event defined for %s!", feat_list[featnum].name);
    return (0);
  }

  if ((daily_uses = get_daily_uses(ch, featnum)) < 1)
  {
    /* ch has no uses of this ability at all!  Error! */
    log("SYSERR: in start_daily_use_cooldown, cooldown initiated for invalid ability (featnum: %d)!", featnum);
    return (0);
  }

  if ((pMudEvent = char_has_mud_event(ch, iId)))
  {
    /* Player is on cooldown for this ability - just update the event. */
    /* The number of uses is stored in the pMudEvent->sVariables field. */
    if (pMudEvent->sVariables == NULL)
    {
      /* This is odd - This field should always be populated for daily-use abilities,
       * maybe some legacy code or bad id. */
      log("SYSERR: 2 sVariables field is NULL for daily-use-cooldown-event: %d", iId);
    }
    else
    {

      if (sscanf(pMudEvent->sVariables, "uses:%d", &uses) != 1)
      {
        log("SYSERR: In start_daily_use_cooldown, bad sVariables for daily-use-cooldown-event: %d", iId);
        uses = 0;
      }
      free(pMudEvent->sVariables);
    }
    uses++;

    if (uses > daily_uses)
      log("SYSERR: Daily uses exceeed maximum for %s, feat %s", GET_NAME(ch), feat_list[featnum].name);

    snprintf(buf, sizeof(buf), "uses:%d", uses);
    pMudEvent->sVariables = strdup(buf);
  }
  else
  {
    /* No event - so attach one. */
    uses = 1;
    attach_mud_event(
      new_mud_event(iId, ch, "uses:1"), 
      (SECS_PER_MUD_DAY / daily_uses) RL_SEC);
  }

  return uses;
}

/* Function to return the number of daily uses remaining for a particular ability.
 * Returns the number of daily uses available or -1 if the feat is not a daily-use feat. */
int daily_uses_remaining(struct char_data *ch, int featnum)
{
  struct mud_event_data *pMudEvent = NULL;
  int uses = 0;
  int uses_per_day = 0;
  event_id iId = 0;

  if ((iId = feat_list[featnum].event) == eNULL)
    return -1;

  if ((pMudEvent = char_has_mud_event(ch, iId)))
  {
    if (pMudEvent->sVariables == NULL)
    {
      /* This is odd - This field should always be populated for daily-use abilities,
       * maybe some legacy code or bad id. */
      log("SYSERR: 3 sVariables field is NULL for daily-use-cooldown-event: %d", iId);
    }
    else
    {
      if (sscanf(pMudEvent->sVariables, "uses:%d", &uses) != 1)
      {
        log("SYSERR: In daily_uses_remaining, bad sVariables for daily-use-cooldown-event: %d", iId);
        uses = 0;
      }
    }
  }

  uses_per_day = get_daily_uses(ch, featnum);

  return uses_per_day - uses;
}

/* Function to create an event, based on the mud_event passed in, that will either:
 * 1.) Create a new event with the proper sVariables
 * 2.) Update an existing event with a new sVariable value
 *
 * Returns the current number of uses on cooldown. */
int start_item_specab_daily_use_cooldown(struct obj_data *obj, int specab)
{
  struct mud_event_data *pMudEvent = NULL;
  int uses = 0, daily_uses = 0;
  char buf[128];
  event_id iId = 0;

  /* Transform the feat number to the event id for that ability. */
  if ((iId = special_ability_info[specab].event) == eNULL)
  {
    log("SYSERR: in start_daily_use_cooldown, no cooldown event defined for %s!", special_ability_info[specab].name);
    return (0);
  }

  if ((daily_uses = special_ability_info[specab].daily_uses) < 1)
  {
    /* ch has no uses of this ability at all!  Error! */
    log("SYSERR: in start_daily_use_cooldown, cooldown initiated for invalid ability (specab: %d)!", specab);
    return (0);
  }

  if ((pMudEvent = obj_has_mud_event(obj, iId)))
  {
    /* Player is on cooldown for this ability - just update the event. */
    /* The number of uses is stored in the pMudEvent->sVariables field. */
    if (pMudEvent->sVariables == NULL)
    {
      /* This is odd - This field should always be populated for daily-use abilities,
       * maybe some legacy code or bad id. */
      log("SYSERR: 4 sVariables field is NULL for daily-use-cooldown-event: %d", iId);
    }
    else
    {

      if (sscanf(pMudEvent->sVariables, "uses:%d", &uses) != 1)
      {
        log("SYSERR: In start_daily_use_cooldown, bad sVariables for daily-use-cooldown-event: %d", iId);
        uses = 0;
      }
      free(pMudEvent->sVariables);
    }
    uses++;

    if (uses > daily_uses)
      log("SYSERR: Daily uses exceeed maximum for %s, specab %s", obj->name, special_ability_info[specab].name);

    snprintf(buf, sizeof(buf), "uses:%d", uses);
    pMudEvent->sVariables = strdup(buf);
  }
  else
  {
    /* No event - so attach one. */
    uses = 1;
    attach_mud_event(new_mud_event(iId, obj, "uses:1"), (SECS_PER_MUD_DAY / daily_uses) RL_SEC);
  }

  return uses;
}

/* Function to return the number of daily uses remaining for a particular armor special ability.
 * Returns the number of daily uses available or -1 if the object does not have that special ability. */
int daily_item_specab_uses_remaining(struct obj_data *obj, int specab)
{
  struct mud_event_data *pMudEvent = NULL;
  int uses = 0;
  int uses_per_day = 0;
  event_id iId = 0;

  if ((iId = special_ability_info[specab].event) == eNULL)
    return -1;

  if ((pMudEvent = obj_has_mud_event(obj, iId)))
  {
    if (pMudEvent->sVariables == NULL)
    {
      /* This is odd - This field should always be populated for daily-use abilities,
       * maybe some legacy code or bad id. */
      log("SYSERR: 5 sVariables field is NULL for daily-use-cooldown-event: %d", iId);
    }
    else
    {
      if (sscanf(pMudEvent->sVariables, "uses:%d", &uses) != 1)
      {
        log("SYSERR: In daily_uses_remaining, bad sVariables for daily-use-cooldown-event: %d", iId);
        uses = 0;
      }
    }
  }

  uses_per_day = special_ability_info[specab].daily_uses;

  return uses_per_day - uses;
}

/* line_string()
 * Generate and return a string, alternating between first and second for length characters.
 */
char *line_string(int length, char first, char second)
{
  static char buf[MAX_STRING_LENGTH] = {'\0'}; /* Note - static! */
  int i = 0;
  while (i < length)
    if ((i % 2) == 0)
      buf[i++] = first;
    else
      buf[i++] = second;

  buf[i++] = '\r';
  buf[i++] = '\n';
  buf[i] = '\0'; /* String terminator */

  return buf;
}

void draw_line(struct char_data *ch, int length, char first, char second)
{
  send_to_char(ch, "%s", line_string(length, first, second));
}

/*  text_line_string()
 *  Generate and return a string, as above, with text centered.
 */
const char *text_line_string(const char *text, int length, char first, char second)
{
  int text_length, text_print_length, pre_length;
  int i = 0, j = 0;
  static char buf[MAX_STRING_LENGTH] = {'\0'}; /* Note - static! */

  text_length = strlen(text);
  text_print_length = count_non_protocol_chars(text);

  pre_length = (length - (text_print_length)) / 2; /* (length - (text length + '[  ]'))/2 */
  //  pre_length = (length - (text_length + 4))/2; /* (length - (text length + '[  ]'))/2 */
  //  pre_length = 2;

  while (i < pre_length)
  {
    if ((i % 2) == 0)
      buf[i] = first;
    else
      buf[i] = second;
    i++;
  }
  //  buf[i++] = '[';
  //  buf[i++] = ' ';

  while (j < text_length)
    buf[i++] = text[j++];

  //  buf[i++] = ' ';
  //  buf[i++] = ']';

  while (i < length + (text_length - text_print_length)) /* Have to include the non printables */
    if ((i % 2) == 0)
      buf[i++] = first;
    else
      buf[i++] = second;
  buf[i++] = '\r';
  buf[i++] = '\n';
  buf[i] = '\0'; /* Terminate the string. */

  return buf;
}

void text_line(struct char_data *ch, const char *text, int length, char first, char second)
{
  send_to_char(ch, "%s", text_line_string(text, length, first, second));
}

/* Name:   calculate_cp
 * Author: Ornir
 * Param:  Object to calculate cp for
 * Return: cp value for the given object. */
int calculate_cp(struct obj_data *obj)
{
  int current_cp = 0;
  struct obj_special_ability *specab;

  if (!obj)
    return 0;

  switch (GET_OBJ_TYPE(obj))
  {
  case ITEM_WEAPON: /* obj is a weapon, use the cp calc for weapons. */
    current_cp += GET_ENHANCEMENT_BONUS(obj) * GET_ENHANCEMENT_BONUS(obj) * 2000;

    for (specab = obj->special_abilities; specab != NULL; specab = specab->next)
    {
      current_cp += (special_ability_info[specab->ability].cost * special_ability_info[specab->ability].cost * 2000);
    }

    break;
  default: /* No idea what this is. */
    break;
  }

  return 0;
}

bool is_incorporeal(struct char_data *ch)
{

  if (AFF_FLAGGED(ch, AFF_IMMATERIAL) || HAS_SUBRACE(ch, SUBRACE_INCORPOREAL))
    return true;

  // not the most elegant... but we can fix later if we want.
  return is_immaterial(ch);
}

bool paralysis_immunity(struct char_data *ch)
{
  if (!ch)
    return FALSE;
  if (IS_NPC(ch) && MOB_FLAGGED(ch, MOB_NOPARALYZE))
    return TRUE;
  if (HAS_FEAT(ch, FEAT_DRACONIC_HERITAGE_POWER_OF_WYRMS))
    return TRUE;
  if (HAS_FEAT(ch, FEAT_PARALYSIS_IMMUNITY))
    return TRUE;
  if (HAS_FEAT(ch, FEAT_ONE_OF_US))
    return TRUE;
  if (HAS_FEAT(ch, FEAT_ESSENCE_OF_UNDEATH))
    return TRUE;
  if (AFF_FLAGGED(ch, AFF_FREE_MOVEMENT))
    return TRUE;
  if (HAS_EVOLUTION(ch, EVOLUTION_UNDEAD_APPEARANCE) && get_evolution_appearance_save_bonus(ch) == 100)
    return TRUE;
  

  return FALSE;
}

bool sleep_immunity(struct char_data *ch)
{
  if (!ch)
    return FALSE;
  if (HAS_FEAT(ch, FEAT_DRACONIC_HERITAGE_POWER_OF_WYRMS))
    return TRUE;
  if (HAS_FEAT(ch, FEAT_SLEEP_ENCHANTMENT_IMMUNITY))
    return TRUE;
  if (HAS_FEAT(ch, FEAT_ONE_OF_US))
    return TRUE;
  if (HAS_EVOLUTION(ch, EVOLUTION_UNDEAD_APPEARANCE) && get_evolution_appearance_save_bonus(ch) == 100)
    return TRUE;
  
  return FALSE;
}

sbyte is_immune_death_magic(struct char_data *ch, struct char_data *victim, sbyte display)
{
  if (AFF_FLAGGED(victim, AFF_DEATH_WARD))
  {
    if (display)
    {
      send_to_char(ch, "%s appears to be immune to death magic!\r\n", GET_NAME(victim));
      send_to_char(victim, "You are protected from death magic by your death ward!\r\n");
    }
    return TRUE;
  }
  if (HAS_REAL_FEAT(victim, FEAT_ESSENCE_OF_UNDEATH))
  {
    if (display)
    {
      send_to_char(ch, "%s appears to be immune to death magic!\r\n", GET_NAME(victim));
      send_to_char(victim, "You are protected from death magic by your undead essence!\r\n");
    }
    return TRUE;
  }

  return FALSE;
}

sbyte is_immune_fear(struct char_data *ch, struct char_data *victim, sbyte display)
{

  if (HAS_FEAT(victim, FEAT_KENDER_FEARLESSNESS))
    return true;

  if (HAS_FEAT(victim, FEAT_UNBREAKABLE_WILL))
    return true;
  
  if (HAS_FEAT(victim, FEAT_KNIGHTLY_COURAGE))
    return true;

  if (affected_by_aura_of_cowardice(victim))
  {
    return FALSE;
  }

  if (affected_by_spell(victim, SPELL_LITANY_OF_DEFENSE))
  {
    if (display)
    {
      send_to_char(ch, "%s appears to be fearless!\r\n", GET_NAME(victim));
      send_to_char(victim, "Your litany of defense protects you!\r\n");
    }
    return TRUE;
  }

  if (!IS_NPC(victim) && has_aura_of_courage(victim))
  {
    if (display)
    {
      send_to_char(ch, "%s appears to be fearless!\r\n", GET_NAME(victim));
      send_to_char(victim, "Your divine courage protects you!\r\n");
    }
    return TRUE;
  }
  if (!IS_NPC(victim) && HAS_FEAT(victim, FEAT_RP_FEARLESS_RAGE) &&
      affected_by_spell(victim, SKILL_RAGE))
  {
    if (display)
    {
      send_to_char(ch, "%s appears to be fearless!\r\n", GET_NAME(victim));
      send_to_char(victim, "Your fearless rage protects you!\r\n");
    }
    return TRUE;
  }
  if (!IS_NPC(victim) && HAS_FEAT(victim, FEAT_FEARLESS_DEFENSE) &&
      affected_by_spell(victim, SKILL_DEFENSIVE_STANCE))
  {
    if (display)
    {
      send_to_char(ch, "%s appears to be fearless!\r\n", GET_NAME(victim));
      send_to_char(victim, "Your fearless defense protects you!\r\n");
    }
    return TRUE;
  }

  if (AFF_FLAGGED(victim, AFF_BRAVERY))
  {
    if (display)
    {
      send_to_char(victim, "Your bravery overcomes the fear!\r\n");
      act("$n \tWovercomes the \tDfear\tW with bravery!\tn\tn", TRUE, ch, 0, 0, TO_CHAR);
    }
    return TRUE;
  }

  if (affected_by_spell(victim, SPELL_GREATER_HEROISM))
  {
    if (display)
    {
      send_to_char(victim, "Your heroism overcomes the fear!\r\n");
      act("$n \tWovercomes the \tDfear\tW with heroism!\tn\tn", TRUE, ch, 0, 0, TO_CHAR);
    }
    return TRUE;
  }

  return FALSE;
}

sbyte is_immune_mind_affecting(struct char_data *ch, struct char_data *victim, sbyte display)
{

  if ((IS_UNDEAD(victim) || HAS_FEAT(victim, FEAT_ONE_OF_US)) && !HAS_FEAT(ch, FEAT_UNDEAD_BLOODLINE_ARCANA))
  {
    if (display)
    {
      send_to_char(ch, "%s is undead and thus immune to mind affecting spells and abilities!\r\n", GET_NAME(victim));
      send_to_char(victim, "You are undead and thus are immune to %s's mind-affecting spells and abilities!\r\n", GET_NAME(ch));
    }
    return TRUE;
  }

  if (IS_CONSTRUCT(victim))
  {
    if (display)
    {
      send_to_char(ch, "%s is a construct and thus immune to mind affecting spells and abilities!\r\n", GET_NAME(victim));
      send_to_char(victim, "You are a construct and thus are immune to %s's mind-affecting spells and abilities!\r\n", GET_NAME(ch));
    }
    return TRUE;
  }

  if (IS_OOZE(victim))
  {
    if (display)
    {
      send_to_char(ch, "%s is of racial type 'ooze' and thus immune to mind affecting spells and abilities!\r\n", GET_NAME(victim));
      send_to_char(victim, "You are of racial type 'ooze' and thus are immune to %s's mind-affecting spells and abilities!\r\n", GET_NAME(ch));
    }
    return TRUE;
  }

  if (AFF_FLAGGED(victim, AFF_MIND_BLANK))
  {
    if (display)
    {
      send_to_char(ch, "Mind blank protects %s!\r\n", GET_NAME(victim));
      send_to_char(victim, "Mind blank protects you from %s!\r\n", GET_NAME(ch));
    }
    return TRUE;
  }

  if (affected_by_spell(victim, PSIONIC_THOUGHT_SHIELD))
  {
    int resist = get_char_affect_modifier(victim, PSIONIC_THOUGHT_SHIELD, APPLY_SPECIAL) + get_power_resist_mod(victim);
    int penetrate = d20(ch) + GET_PSIONIC_LEVEL(ch) + get_power_penetrate_mod(ch);
    if (resist > penetrate)
    {
      if (display)
      {
        send_to_char(ch, "Thought shield protects %s!\r\n", GET_NAME(victim));
        send_to_char(victim, "Thought shield protects you from %s!\r\n", GET_NAME(ch));
      }
      return TRUE;
    }
  }

  return FALSE;
}

sbyte is_immune_charm(struct char_data *ch, struct char_data *victim, sbyte display)
{
  if (AFF_FLAGGED(victim, AFF_MIND_BLANK))
  {
    if (display)
    {
      send_to_char(ch, "Mind blank protects %s!\r\n", GET_NAME(victim));
      send_to_char(victim, "Mind blank protects you from %s!\r\n", GET_NAME(ch));
    }
    return TRUE;
  }

  if (affected_by_spell(victim, SPELL_HOLY_AURA))
  {
    if (display)
    {
      send_to_char(ch, "Holy Aura protects %s!\r\n", GET_NAME(victim));
      send_to_char(victim, "Holy Aura protects you from %s!\r\n", GET_NAME(ch));
    }
    return TRUE;
  }

  if (affected_by_spell(victim, AFFECT_KNIGHTHOODS_FLOWER))
  {
    if (display)
    {
      send_to_char(ch, "Knighthood's Flower protects %s!\r\n", GET_NAME(victim));
      send_to_char(victim, "Knighthood's Flower protects you from %s!\r\n", GET_NAME(ch));
    }
    return TRUE;
  }

  if (HAS_FEAT(victim, FEAT_SLEEP_ENCHANTMENT_IMMUNITY))
    return TRUE;
  return FALSE;
}

bool has_aura_of_courage(struct char_data *ch)
{
  if (!ch)
    return false;

  if (HAS_FEAT(ch, FEAT_AURA_OF_COURAGE))
    return true;

  struct char_data *tch = NULL;

  bool has_aura = FALSE;

  if (GROUP(ch) && GROUP(ch)->members && GROUP(ch)->members->iSize)
  {
    struct iterator_data Iterator;

    tch = (struct char_data *)merge_iterator(&Iterator, GROUP(ch)->members);
    for (; tch; tch = next_in_list(&Iterator))
    {
      if (IN_ROOM(tch) != IN_ROOM(ch))
        continue;
      if (HAS_FEAT(tch, FEAT_AURA_OF_COURAGE))
      {
        has_aura = TRUE;
        break;
      }
    }
    remove_iterator(&Iterator);
  }

  return has_aura;
}

bool has_aura_of_terror(struct char_data *ch)
{
  if (!ch)
    return false;

  if (HAS_FEAT(ch, FEAT_AURA_OF_TERROR))
    return true;

  struct char_data *tch = NULL;

  bool has_aura = FALSE;

  if (GROUP(ch) && GROUP(ch)->members && GROUP(ch)->members->iSize)
  {
    struct iterator_data Iterator;

    tch = (struct char_data *)merge_iterator(&Iterator, GROUP(ch)->members);
    for (; tch; tch = next_in_list(&Iterator))
    {
      if (IN_ROOM(tch) != IN_ROOM(ch))
        continue;
      if (HAS_FEAT(tch, FEAT_AURA_OF_TERROR))
      {
        has_aura = TRUE;
        break;
      }
    }
    remove_iterator(&Iterator);
  }

  return has_aura;
}

int leadership_exp_multiplier(struct char_data *ch)
{
  int exp_mult = 100;
  if (!ch)
    return exp_mult;

  if (IN_ROOM(ch) == NOWHERE)
    return exp_mult;

  if (!GROUP(ch) && HAS_FEAT(ch, FEAT_LEADERSHIP))
  {
    exp_mult = 100 + ((HAS_FEAT(ch, FEAT_LEADERSHIP) + 1) * 5);
    return exp_mult;
  }


  struct char_data *tch = NULL;

  for (tch = world[IN_ROOM(ch)].people; tch; tch = tch->next_in_room)
  {
    if (GROUP(ch) != GROUP(tch)) continue;
    if (HAS_FEAT(tch, FEAT_LEADERSHIP))
      {
        exp_mult = MAX(exp_mult, 100 + ((HAS_FEAT(tch, FEAT_LEADERSHIP) + 1) * 5));
      }
  }

  return exp_mult;
}

bool has_fortune_of_many_bonus(struct char_data *ch)
{
  if (!ch)
    return false;

  struct char_data *tch = NULL;
  int num_members = 0;
  bool has_fortune = FALSE;

  if (GROUP(ch) && GROUP(ch)->members && GROUP(ch)->members->iSize)
  {
    struct iterator_data Iterator;

    tch = (struct char_data *)merge_iterator(&Iterator, GROUP(ch)->members);
    for (; tch; tch = next_in_list(&Iterator))
    {
      if (IN_ROOM(tch) != IN_ROOM(ch))
        continue;
      num_members++;
      if (HAS_FEAT(tch, FEAT_FORTUNE_OF_THE_MANY))
      {
        has_fortune = TRUE;
        break;
      }
    }
    remove_iterator(&Iterator);
  }

  return (has_fortune && (num_members > 1));
}

bool has_authoritative_bonus(struct char_data *ch)
{
  if (!ch)
    return false;

  struct char_data *tch = NULL;
  int num_members = 0;
  bool has_authority = FALSE;

  if (GROUP(ch) && GROUP(ch)->members && GROUP(ch)->members->iSize)
  {
    struct iterator_data Iterator;

    tch = (struct char_data *)merge_iterator(&Iterator, GROUP(ch)->members);
    for (; tch; tch = next_in_list(&Iterator))
    {
      if (IN_ROOM(tch) != IN_ROOM(ch))
        continue;
      num_members++;
      if (HAS_FEAT(tch, FEAT_AUTHORITATIVE))
      {
        has_authority = TRUE;
        break;
      }
    }
    remove_iterator(&Iterator);
  }

  return (has_authority && (num_members > 1));
}

bool has_bite_attack(struct char_data *ch)
{
  
  if (HAS_FEAT(ch, FEAT_DRACONIAN_BITE))
    return true;

  return false;
}

void perform_draconian_death_throes(struct char_data *ch)
{
  if (!IS_DRACONIAN(ch))
    return;

  switch (GET_RACE(ch))
  {
    case DL_RACE_BAAZ_DRACONIAN:
      call_magic(ch, 0, 0, ABILITY_BAAZ_DRACONIAN_DEATH_THROES, 0, GET_LEVEL(ch), CAST_INNATE);
      return;
    case DL_RACE_KAPAK_DRACONIAN:
      call_magic(ch, 0, 0, ABILITY_KAPAK_DRACONIAN_DEATH_THROES, 0, GET_LEVEL(ch), CAST_INNATE);
      return;
    case DL_RACE_BOZAK_DRACONIAN:
      call_magic(ch, 0, 0, ABILITY_BOZAK_DRACONIAN_DEATH_THROES, 0, GET_LEVEL(ch), CAST_INNATE);
      return;
  }
}

bool is_grouped_with_dragon(struct char_data *ch)
{
  if (!ch)
    return false;

  struct char_data *tch = NULL;

  bool is_dragon = false;

  if (GROUP(ch) && GROUP(ch)->members && GROUP(ch)->members->iSize)
  {
    struct iterator_data Iterator;

    tch = (struct char_data *)merge_iterator(&Iterator, GROUP(ch)->members);
    for (; tch; tch = next_in_list(&Iterator))
    {
      if (IN_ROOM(tch) != IN_ROOM(ch))
        continue;
      if (IS_DRAGON(tch) || IS_DRACONIAN(tch))
      {
        is_dragon = TRUE;
        break;
      }
    }
    remove_iterator(&Iterator);
  }

  return is_dragon;
}

bool has_aura_of_good(struct char_data *ch)
{
  if (!ch)
    return false;

  if (IS_NPC(ch) && (GET_SUBRACE(ch, 0) == SUBRACE_GOOD || GET_SUBRACE(ch, 1) == SUBRACE_GOOD || GET_SUBRACE(ch, 2) == SUBRACE_GOOD))
    return true;

  if (HAS_FEAT(ch, FEAT_AURA_OF_GOOD))
    return true;

  struct char_data *tch = NULL;

  bool has_aura = FALSE;

  if (GROUP(ch) && GROUP(ch)->members && GROUP(ch)->members->iSize)
  {
    struct iterator_data Iterator;

    tch = (struct char_data *)merge_iterator(&Iterator, GROUP(ch)->members);
    for (; tch; tch = next_in_list(&Iterator))
    {
      if (IN_ROOM(tch) != IN_ROOM(ch))
        continue;
      if (HAS_FEAT(tch, FEAT_AURA_OF_GOOD))
      {
        has_aura = TRUE;
        break;
      }
    }
    remove_iterator(&Iterator);
  }

  return has_aura;
}

bool has_one_thought(struct char_data *ch)
{
  if (!ch)
    return false;

  if (IS_NPC(ch))
    return false;

  if (HAS_FEAT(ch, FEAT_ONE_THOUGHT))
    return true;

  struct char_data *tch = NULL;

  bool has_one_thought = FALSE;

  if (GROUP(ch) && GROUP(ch)->members && GROUP(ch)->members->iSize)
  {
    struct iterator_data Iterator;

    tch = (struct char_data *)merge_iterator(&Iterator, GROUP(ch)->members);
    for (; tch; tch = next_in_list(&Iterator))
    {
      if (IN_ROOM(tch) != IN_ROOM(ch))
        continue;
      if (HAS_FEAT(tch, FEAT_ONE_THOUGHT))
      {
        has_one_thought = TRUE;
        break;
      }
    }
    remove_iterator(&Iterator);
  }

  return has_one_thought;
}

bool is_grouped_in_room(struct char_data *ch)
{
  if (!ch)
    return false;

  struct char_data *tch = NULL;
  bool grouped = false;
  bool num = 0;

  if (GROUP(ch) && GROUP(ch)->members && GROUP(ch)->members->iSize)
  {
    struct iterator_data Iterator;

    tch = (struct char_data *)merge_iterator(&Iterator, GROUP(ch)->members);
    for (; tch; tch = next_in_list(&Iterator))
    {
      if (IN_ROOM(tch) != IN_ROOM(ch))
        continue;
      
      num++;
      grouped = true;
    }
    remove_iterator(&Iterator);
  }

  return (num >= 2 && grouped);
}

bool has_aura_of_evil(struct char_data *ch)
{
  if (!ch)
    return false;

  if (IS_NPC(ch) && (GET_SUBRACE(ch, 0) == SUBRACE_EVIL || GET_SUBRACE(ch, 1) == SUBRACE_EVIL || GET_SUBRACE(ch, 2) == SUBRACE_EVIL))
    return true;

  if (HAS_FEAT(ch, FEAT_AURA_OF_EVIL))
    return true;

  struct char_data *tch = NULL;

  bool has_aura = FALSE;

  if (GROUP(ch) && GROUP(ch)->members && GROUP(ch)->members->iSize)
  {
    struct iterator_data Iterator;

    tch = (struct char_data *)merge_iterator(&Iterator, GROUP(ch)->members);
    for (; tch; tch = next_in_list(&Iterator))
    {
      if (IN_ROOM(tch) != IN_ROOM(ch))
        continue;
      if (HAS_FEAT(tch, FEAT_AURA_OF_EVIL))
      {
        has_aura = TRUE;
        break;
      }
    }
    remove_iterator(&Iterator);
  }

  return has_aura;
}

bool can_speak_language(struct char_data *ch, int language)
{
  if (!ch) return false;
  if (language < LANG_COMMON || language >= NUM_LANGUAGES) return false;

  if (GET_LEVEL(ch) >= LVL_IMMORT) return true;
  if (language == LANG_COMMON) return true;
  if (language == (race_list[GET_REAL_RACE(ch)].racial_language - SKILL_LANG_LOW)) return true;
  if (ch->player_specials->saved.languages_known[language]) return true;
  if (language == LANG_DRUIDIC && CLASS_LEVEL(ch, CLASS_DRUID)) return true;
  if (language == LANG_THIEVES_CANT && (CLASS_LEVEL(ch, CLASS_ROGUE) || HAS_FEAT(ch, FEAT_BG_CRIMINAL))) return true;
  if (language == get_region_language(GET_REGION(ch))) return true;

  return false;
}

bool group_member_affected_by_spell(struct char_data *ch, int spellnum)
{
  if (!ch)
    return false;

  if (affected_by_spell(ch, spellnum))
    return true;

  struct char_data *tch = NULL;

  bool has_aura = FALSE;

  if (GROUP(ch) && GROUP(ch)->members && GROUP(ch)->members->iSize)
  {
    struct iterator_data Iterator;

    tch = (struct char_data *)merge_iterator(&Iterator, GROUP(ch)->members);
    for (; tch; tch = next_in_list(&Iterator))
    {
      if (IN_ROOM(tch) != IN_ROOM(ch))
        continue;
      if (affected_by_spell(tch, spellnum))
      {
        has_aura = TRUE;
        break;
      }
    }
    remove_iterator(&Iterator);
  }

  return has_aura;
}

bool affected_by_aura_of_sin(struct char_data *ch)
{
  if (!ch)
    return false;
  if (IN_ROOM(ch) == NOWHERE)
    return false;
  if (!IS_EVIL(ch))
    return false;

  struct char_data *tch = NULL;

  for (tch = world[IN_ROOM(ch)].people; tch; tch = tch->next_in_room)
  {
    if (ch == tch)
      return false;
    if (GROUP(ch) != GROUP(tch))
      return false;
    if (HAS_FEAT(tch, FEAT_AURA_OF_SIN))
      return true;
  }

  return true;
}

bool affected_by_aura_of_faith(struct char_data *ch)
{
  if (!ch)
    return false;
  if (IN_ROOM(ch) == NOWHERE)
    return false;
  if (!IS_GOOD(ch))
    return false;

  struct char_data *tch = NULL;

  for (tch = world[IN_ROOM(ch)].people; tch; tch = tch->next_in_room)
  {
    if (ch == tch)
      return false;
    if (GROUP(ch) != GROUP(tch))
      return false;
    if (HAS_FEAT(tch, FEAT_AURA_OF_FAITH))
      return true;
  }

  return true;
}

bool affected_by_aura_of_righteousness(struct char_data *ch)
{
  if (!ch)
    return false;
  if (IN_ROOM(ch) == NOWHERE)
    return false;

  struct char_data *tch = NULL;

  for (tch = world[IN_ROOM(ch)].people; tch; tch = tch->next_in_room)
  {
    if (ch == tch)
      return false;
    if (GROUP(ch) != GROUP(tch))
      return false;
    if (HAS_FEAT(tch, FEAT_AURA_OF_RIGHTEOUSNESS))
      return true;
  }

  return true;
}

bool affected_by_aura_of_cowardice(struct char_data *ch)
{
  if (!ch)
    return false;
  if (IN_ROOM(ch) == NOWHERE)
    return false;

  struct char_data *tch = NULL;

  for (tch = world[IN_ROOM(ch)].people; tch; tch = tch->next_in_room)
  {
    if (ch == tch)
      return false;
    if (GROUP(ch) == GROUP(tch))
      return false;
    if (!pvp_ok_single(ch, false))
      return false;
    if (HAS_FEAT(tch, FEAT_AURA_OF_COWARDICE))
      return true;
  }

  return true;
}

bool affected_by_aura_of_depravity(struct char_data *ch)
{
  if (!ch)
    return false;
  if (IN_ROOM(ch) == NOWHERE)
    return false;

  struct char_data *tch = NULL;

  for (tch = world[IN_ROOM(ch)].people; tch; tch = tch->next_in_room)
  {
    if (ch == tch)
      return false;
    if (GROUP(ch) == GROUP(tch))
      return false;
    if (!pvp_ok_single(ch, false))
      return false;
    if (HAS_FEAT(tch, FEAT_AURA_OF_DEPRAVITY))
      return true;
  }

  return true;
}

bool affected_by_aura_of_despair(struct char_data *ch)
{
  if (!ch)
    return false;
  if (IN_ROOM(ch) == NOWHERE)
    return false;

  struct char_data *tch = NULL;

  for (tch = world[IN_ROOM(ch)].people; tch; tch = tch->next_in_room)
  {
    if (ch == tch)
      return false;
    if (GROUP(ch) == GROUP(tch))
      return false;
    if (!pvp_ok_single(ch, false))
      return false;
    if (HAS_FEAT(tch, FEAT_AURA_OF_DESPAIR))
      return true;
  }

  return true;
}

bool is_fear_spell(int spellnum)
{
  switch (spellnum)
  {
  case SPELL_DOOM:
    return true;
  }
  return false;
}

void remove_fear_affects(struct char_data *ch, sbyte display)
{
  // aura of cowardice nullifies any fear immunity
  if (affected_by_aura_of_cowardice(ch))
    return;

  /* fear -> skill-courage*/
  if ((AFF_FLAGGED(ch, AFF_FEAR) || AFF_FLAGGED(ch, AFF_SHAKEN)) && (!IS_NPC(ch) &&
                                                                     has_aura_of_courage(ch)))
  {
    REMOVE_BIT_AR(AFF_FLAGS(ch), AFF_FEAR);
    REMOVE_BIT_AR(AFF_FLAGS(ch), AFF_SHAKEN);
    send_to_char(ch, "Your divine courage overcomes the fear!\r\n");
    act("$n \tWovercomes the \tDfear\tW with courage!\tn\tn",
        TRUE, ch, 0, 0, TO_ROOM);
    return;
  }

  /* fearless rage */
  if ((AFF_FLAGGED(ch, AFF_FEAR) || AFF_FLAGGED(ch, AFF_SHAKEN)) && !IS_NPC(ch) &&
      HAS_FEAT(ch, FEAT_RP_FEARLESS_RAGE) &&
      affected_by_spell(ch, SKILL_RAGE))
  {
    REMOVE_BIT_AR(AFF_FLAGS(ch), AFF_FEAR);
    REMOVE_BIT_AR(AFF_FLAGS(ch), AFF_SHAKEN);
    send_to_char(ch, "Your fearless rage overcomes the fear!\r\n");
    act("$n \tWis bolstered by $s fearless rage and overcomes $s \tDfear!\tn\tn",
        TRUE, ch, 0, 0, TO_ROOM);
    return;
  }

  /* fearless defense */
  if ((AFF_FLAGGED(ch, AFF_FEAR) || AFF_FLAGGED(ch, AFF_SHAKEN)) && !IS_NPC(ch) &&
      HAS_FEAT(ch, FEAT_FEARLESS_DEFENSE) &&
      affected_by_spell(ch, SKILL_DEFENSIVE_STANCE))
  {
    REMOVE_BIT_AR(AFF_FLAGS(ch), AFF_FEAR);
    REMOVE_BIT_AR(AFF_FLAGS(ch), AFF_SHAKEN);
    send_to_char(ch, "Your fearless defense overcomes the fear!\r\n");
    act("$n \tWis bolstered by $s fearless defense and overcomes $s \tDfear!\tn\tn",
        TRUE, ch, 0, 0, TO_ROOM);
    return;
  }

  /* fear -> spell-bravery */
  if ((AFF_FLAGGED(ch, AFF_FEAR) || AFF_FLAGGED(ch, AFF_SHAKEN)) && AFF_FLAGGED(ch, AFF_BRAVERY))
  {
    REMOVE_BIT_AR(AFF_FLAGS(ch), AFF_FEAR);
    REMOVE_BIT_AR(AFF_FLAGS(ch), AFF_SHAKEN);
    send_to_char(ch, "Your bravery overcomes the fear!\r\n");
    act("$n \tWovercomes the \tDfear\tW with bravery!\tn\tn",
        TRUE, ch, 0, 0, TO_ROOM);
    return;
  }
}

int get_poison_save_mod(struct char_data *ch, struct char_data *victim)
{
  int bonus = 0;

  if (HAS_FEAT(victim, FEAT_POISON_RESIST)) // poison resist feat
    bonus += 4;
  if (GET_RACE(victim) == RACE_DWARF || // dwarf dwarven poison resist
      GET_RACE(victim) == RACE_DUERGAR ||
      GET_RACE(victim) == RACE_CRYSTAL_DWARF)
    bonus += 2;
  if (KNOWS_DISCOVERY(ch, ALC_DISC_MALIGNANT_POISON))
    bonus -= 4;
  if (HAS_EVOLUTION(ch, EVOLUTION_UNDEAD_APPEARANCE))
    bonus += get_evolution_appearance_save_bonus(ch);
  else if (HAS_EVOLUTION(ch, EVOLUTION_CELESTIAL_APPEARANCE))
    bonus += get_evolution_appearance_save_bonus(ch);

  bonus += HAS_FEAT(ch, FEAT_POISON_SAVE_BONUS);

  return bonus;
}

// will return TRUE if the poison is resisted, otherwise will return false
// includes the saving throw against poison.
bool check_poison_resist(struct char_data *ch, struct char_data *victim, int casttype, int level)
{

  if (!can_poison(victim))
    return true;

  if (HAS_FEAT(victim, FEAT_POISON_IMMUNITY) || HAS_FEAT(victim, FEAT_SOUL_OF_THE_FEY))
    return TRUE;

  int bonus = 0;

  if (casttype != CAST_INNATE && mag_resistance(ch, victim, 0))
    return TRUE;

  bonus += get_poison_save_mod(ch, victim);

  if (mag_savingthrow(ch, victim, SAVING_FORT, bonus, casttype, level, ENCHANTMENT))
  {
    return TRUE;
  }
  return FALSE;
}

int is_player_grouped(struct char_data *target, struct char_data *group)
{

  struct char_data *k = NULL;
  struct follow_type *f = NULL;

  if (!target || !group)
    return FALSE;

  if (group == target)
    return TRUE;

  if (!(target->group && group->group && target->group == group->group))
    return FALSE;

  if (group == target->master || target == group->master)
    return TRUE;

  if (group->master)
    k = group->master;
  else
    k = group;

  for (f = k->followers; f; f = f->next)
  {
    if (f->follower == target)
      return TRUE;
  }

  return FALSE;
}

bool can_fly(struct char_data *ch)
{

  if (!ch)
    return FALSE;

  if (HAS_FEAT(ch, FEAT_WINGS))
    return TRUE;

  if (HAS_FEAT(ch, FEAT_FAE_FLIGHT))
    return TRUE;

  if (KNOWS_DISCOVERY(ch, ALC_DISC_WINGS))
    return TRUE;

  if (affected_by_spell(ch, SKILL_DRHRT_WINGS))
    return TRUE;

  if (affected_by_spell(ch, SPELL_FLY))
    return TRUE;

  struct obj_data *obj = NULL;
  int i = 0;

  /* Non-wearable flying items in inventory will do it. */
  for (obj = ch->carrying; obj; obj = obj->next_content)
    if (OBJAFF_FLAGGED(obj, AFF_FLYING) && (find_eq_pos(ch, obj, NULL) < 0))
      return (1);

  /* Any equipped objects with AFF_FLYING will do it too. */
  for (i = 0; i < NUM_WEARS; i++)
    if (GET_EQ(ch, i) && OBJAFF_FLAGGED(GET_EQ(ch, i), AFF_FLYING))
      return (1);

  return FALSE;
}

bool is_flying(struct char_data *ch)
{

  if (!ch)
    return FALSE;

  if (AFF_FLAGGED(ch, AFF_FLYING))
    return TRUE;

  if (RIDING(ch))
  {
    if (AFF_FLAGGED(RIDING(ch), AFF_FLYING))
      return TRUE;
  }

  return FALSE;
}

void do_study_spell_help(struct char_data *ch, int spellnum)
{

  char buf[MEDIUM_STRING] = {'\0'};

  switch (spellnum)
  {
  case SPELL_STRENGTH:
  case SPELL_SLOW:
  case SPELL_WISDOM:
  case SPELL_CHARISMA:
  case SPELL_TELEPORT:
    snprintf(buf, sizeof(buf), "spell %s", spell_info[spellnum].name);
    do_help(ch, buf, 0, 0);
    break;

  case SPELL_NON_DETECTION:
    snprintf(buf, sizeof(buf), "non detection");
    do_help(ch, buf, 0, 0);
    break;

  case SPELL_IRRESISTIBLE_DANCE:
    snprintf(buf, sizeof(buf), "irrisistable dance");
    do_help(ch, buf, 0, 0);
    break;

  case SPELL_IRONSKIN:
    snprintf(buf, sizeof(buf), "ironskin");
    do_help(ch, buf, 0, 0);
    break;

  default:
    do_help(ch, spell_info[spellnum].name, 0, 0);
    break;
  }
}

bool pvp_ok(struct char_data *ch, struct char_data *target, bool display)
{

  if (!ch || !target)
    return false;

  bool pvp_ok = true;

  // right now, there's no opt-in pvp, so we'll return true until we add that in
  return pvp_ok;

  if (PRF_FLAGGED(ch, PRF_PVP) && PRF_FLAGGED(target, PRF_PVP))
    return true;

  // are they in the arena?
  if (world[IN_ROOM(ch)].number >= ARENA_START && world[IN_ROOM(ch)].number <= ARENA_END)
    if (world[IN_ROOM(target)].number >= ARENA_START && world[IN_ROOM(target)].number <= ARENA_END)
      return true;

  return false;
}

bool pvp_ok_single(struct char_data *ch, bool display)
{

  bool pvp_ok = true;

  if (!ch)
  {
    pvp_ok = false;
  }

  if (IS_NPC(ch) && ch->master && !IS_NPC(ch->master) && !PRF_FLAGGED(ch->master, PRF_PVP))
    pvp_ok = false;

  if (!IS_NPC(ch) && !PRF_FLAGGED(ch, PRF_PVP))
    pvp_ok = false;

  // are they in the arena?
  if (world[IN_ROOM(ch)].number >= ARENA_START && world[IN_ROOM(ch)].number <= ARENA_END)
    pvp_ok = true;

  if (!pvp_ok && display)
    send_to_char(ch, "That would be a PvP action, and is not eligible because you do not have pvp enabled.\r\n");

  return pvp_ok;
}

bool is_pc_idnum_in_room(struct char_data *ch, long int idnum)
{
  if (!ch)
    return false;

  struct char_data *tch = NULL;

  for (tch = world[IN_ROOM(ch)].people; tch; tch = tch->next_in_room)
  {
    if (tch == ch)
      continue;
    if (IS_NPC(tch))
      continue;
    if (GET_IDNUM(tch) == idnum)
      return true;
  }
  return false;
}

int find_ability_num_by_name(char *name)
{
  char skOne[MEDIUM_STRING] = {'\0'};
  int i = 0, j = 0;

  for (i = 0; i < strlen(name); i++)
    name[i] = tolower(name[i]);

  for (j = START_GENERAL_ABILITIES; j < NUM_ABILITIES; j++)
  {
    snprintf(skOne, sizeof(skOne), "%s", ability_names[j]);
    for (i = 0; i < strlen(skOne); i++)
      skOne[i] = tolower(skOne[i]);
    if (!strcmp(name, skOne))
      return j;
  }
  return 0;
}

bool using_monk_gloves(struct char_data *ch)
{
  if (!ch)
    return false;

  if (!GET_EQ(ch, WEAR_HANDS))
    return false;

  if (!GET_OBJ_VAL(GET_EQ(ch, WEAR_HANDS), 0))
    return false;

  return true;
}

bool is_monk_weapon(struct obj_data *obj)
{
  if (!obj) return false;

  switch (GET_OBJ_VAL(obj, 0))
  {
    case WEAPON_TYPE_UNARMED:
    case WEAPON_TYPE_QUARTERSTAFF:
    case WEAPON_TYPE_KAMA:
    case WEAPON_TYPE_NUNCHAKU:
    case WEAPON_TYPE_SAI:
    case WEAPON_TYPE_SIANGHAM:
    case WEAPON_TYPE_SHURIKEN:
    case WEAPON_TYPE_DAGGER:
    case WEAPON_TYPE_CLUB:
    case WEAPON_TYPE_HAND_AXE:
    case WEAPON_TYPE_SHORTSPEAR:
    case WEAPON_TYPE_SPEAR:
    case WEAPON_TYPE_SHORT_SWORD:
      return true;
  }
  return false;
}

int get_max_party_level_same_room(struct char_data *ch)
{
  if (!ch)
    return 0;

  struct char_data *master = NULL, *tch = NULL;
  struct follow_type *f = NULL;
  int level = 0;

  if (ch->master)
    master = ch->master;
  else
    master = ch;

  level = GET_LEVEL(master);

  for (f = master->followers; f; f = f->next)
  {
    tch = f->follower;
    if (!tch)
      continue;
    if (IS_NPC(tch)) continue;
    if (IN_ROOM(tch) != IN_ROOM(master))
      continue;
    if (GET_LEVEL(tch) > level)
      level = GET_LEVEL(tch);
  }

  return level;
}

int get_avg_party_level_same_room(struct char_data *ch)
{
  if (!ch)
    return 0;

  struct char_data *master = NULL, *tch = NULL;
  struct follow_type *f = NULL;
  int level = 0, size = 1;

  if (ch->master)
    master = ch->master;
  else
    master = ch;

  level = GET_LEVEL(master);

  for (f = master->followers; f; f = f->next)
  {
    tch = f->follower;
    if (!tch)
      continue;
    if (IN_ROOM(tch) != IN_ROOM(master))
      continue;
    level += GET_LEVEL(tch);
    size++;
  }

  return (level / size);
}

int get_party_size_same_room(struct char_data *ch)
{
  if (!ch)
    return 0;

  if (IN_ROOM(ch) == NOWHERE)
    return 0;

  struct char_data *master = NULL, *tch = NULL;
  struct follow_type *f = NULL;
  int size = 1;

  if (ch->master)
    master = ch->master;
  else
    master = ch;

  for (f = master->followers; f; f = f->next)
  {
    tch = f->follower;
    if (!tch)
      continue;
    if (IN_ROOM(tch) != IN_ROOM(master))
      continue;
    size++;
  }

  return size;
}

int combat_skill_roll(struct char_data *ch, int skillnum)
{
  int roll = skill_roll(ch, skillnum);

  if (roll >= 75)
  {
    return 8;
  }
  else if (roll >= 65)
  {
    return 7;
  }
  else if (roll >= 55)
  {
    return 6;
  }
  else if (roll >= 45)
  {
    return 5;
  }
  else if (roll >= 35)
  {
    return 4;
  }
  else if (roll >= 25)
  {
    return 3;
  }
  else if (roll >= 20)
  {
    return 2;
  }
  else if (roll >= 15)
  {
    return 1;
  }
  else
  {
    return 0;
  }
}

bool can_flee_speed(struct char_data *ch)
{
  if (!ch || IN_ROOM(ch) == NOWHERE)
    return false;

  int ch_speed = get_speed(ch, false);
  int mob_speed = 0;

  if (HAS_REAL_FEAT(ch, FEAT_NIMBLE_ESCAPE))
    return true;
  
  if (HAS_FEAT(ch, FEAT_COWARDLY))
    return true;

  struct char_data *tch = NULL;

  for (tch = world[IN_ROOM(ch)].people; tch; tch = tch->next_in_room)
  {
    if (MOB_FLAGGED(tch, MOB_ENCOUNTER))
    {
      if (CAN_SEE(tch, ch))
        mob_speed = MAX(mob_speed, get_speed(tch, false));
    }
  }

  if (ch_speed <= mob_speed)
    return false;
  return true;
}

int d20(struct char_data *ch)
{
  if (!ch)
    return dice(1, 20);

  int roll = dice(1, 20);
  int roll2 = 0;
  int low = 0, high = 0;

  // critical failure always returns a roll of one
  if (roll == 1) return 1;

  // critical success will always rerturn 20
  if (roll == 20) return 20;

  if (roll <= 5 && HAS_REAL_FEAT(ch, FEAT_FORTUNE_OF_THE_MANY))
  {
    if (dice(1, 10) == 1)
    {
      roll2 = dice(1, 20);
      if (roll != roll2)
      {
        low = MIN(roll, roll2);
        roll = high = MAX(roll, roll2);
        // send_to_char(ch, "\tY[Fortune of the Many Reroll! %d to %d]\tn\r\n", low, high);
      }
    }
  }

  if (dice(1, 100) <= 10 && get_lucky_weapon_bonus(ch))
  {
    roll += MAX(1, get_lucky_weapon_bonus(ch) / 2);
    send_combat_roll_info(ch, "\tY[Lucky Weapon Bonus! +%d]\tn\r\n", MAX(1, get_lucky_weapon_bonus(ch) / 2));
  }

  if (ch && !IS_NPC(ch) && ch->player_specials && ch->player_specials->cosmic_awareness)
  {
    roll += GET_PSIONIC_LEVEL(ch);
    ch->player_specials->cosmic_awareness = false;
    send_to_char(ch, "\tY[Cosmic Awareness Bonus! +%d]\tn\r\n", GET_PSIONIC_LEVEL(ch));
  }

  if (ch && affected_by_spell(ch, PSIONIC_ABILITY_PSIONIC_FOCUS) && HAS_FEAT(ch, FEAT_PERPETUAL_FORESIGHT))
  if (dice(1, 10) == 1)
  {
    roll += GET_INT_BONUS(ch);
    send_to_char(ch, "\tY[Perpetual Foresight Bonus! +%d]\tn\r\n", GET_INT_BONUS(ch));
  }

  if (affected_by_spell(ch, AFFECT_FORETELL))
  {
    if (GET_FORETELL_USES(ch) > 0)
    {
      GET_FORETELL_USES(ch)--;
      roll += 5;
      if (GET_FORETELL_USES(ch) == 0)
      {
        affect_from_char(ch, AFFECT_FORETELL);
      }
    }
  }

  return roll;
}

const char *get_wearoff(int abilnum)
{
  if (spell_info[abilnum].schoolOfMagic != NOSCHOOL)
    return (const char *)spell_info[abilnum].wear_off_msg;

  if (spell_info[abilnum].schoolOfMagic == ACTIVE_SKILL)
    return (const char *)spell_info[abilnum].wear_off_msg;

  return (const char *)spell_info[abilnum].wear_off_msg;
}

int damage_type_to_resistance_type(int type)
{
  switch (type)
  {
  case DAM_FIRE:
    return APPLY_RES_FIRE;
  case DAM_COLD:
    return APPLY_RES_COLD;
  case DAM_AIR:
    return APPLY_RES_AIR;
  case DAM_EARTH:
    return APPLY_RES_EARTH;
  case DAM_ACID:
    return APPLY_RES_ACID;
  case DAM_HOLY:
    return APPLY_RES_HOLY;
  case DAM_ELECTRIC:
    return APPLY_RES_ELECTRIC;
  case DAM_UNHOLY:
    return APPLY_RES_UNHOLY;
  case DAM_SLICE:
    return APPLY_RES_SLICE;
  case DAM_PUNCTURE:
    return APPLY_RES_PUNCTURE;
  case DAM_FORCE:
    return APPLY_RES_FORCE;
  case DAM_SOUND:
    return APPLY_RES_SOUND;
  case DAM_POISON:
    return APPLY_RES_POISON;
  case DAM_DISEASE:
    return APPLY_RES_DISEASE;
  case DAM_NEGATIVE:
    return APPLY_RES_NEGATIVE;
  case DAM_ILLUSION:
    return APPLY_RES_ILLUSION;
  case DAM_MENTAL:
    return APPLY_RES_MENTAL;
  case DAM_LIGHT:
    return APPLY_RES_LIGHT;
  case DAM_ENERGY:
    return APPLY_RES_ENERGY;
  case DAM_WATER:
    return APPLY_RES_WATER;
  }
  return APPLY_NONE;
}

int get_power_penetrate_mod(struct char_data *ch)
{
  int bonus = 0;

  // insert challenge bonuses here (ch)
  if (!IS_NPC(ch) && HAS_FEAT(ch, FEAT_POWER_PENETRATION))
    bonus += 2;
  if (!IS_NPC(ch) && HAS_FEAT(ch, FEAT_GREATER_POWER_PENETRATION))
    bonus += 3;
  if (!IS_NPC(ch) && HAS_FEAT(ch, FEAT_MIGHTY_POWER_PENETRATION))
    bonus += 3;
  if (!IS_NPC(ch) && HAS_FEAT(ch, FEAT_EPIC_POWER_PENETRATION))
    bonus += 6;

  if (affected_by_spell(ch, PSIONIC_ABILITY_PSIONIC_FOCUS) && HAS_FEAT(ch, FEAT_BREACH_POWER_RESISTANCE))
    bonus += MAX(0, GET_INT_BONUS(ch));

  return bonus;
}

int get_power_resist_mod(struct char_data *ch)
{
  int bonus = 0;

  if (affected_by_spell(ch, PSIONIC_POWER_RESISTANCE))
    bonus += get_char_affect_modifier(ch, PSIONIC_POWER_RESISTANCE, APPLY_POWER_RES);
  if (affected_by_spell(ch, PSIONIC_TOWER_OF_IRON_WILL))
    bonus = MAX(bonus, get_char_affect_modifier(ch, PSIONIC_TOWER_OF_IRON_WILL, APPLY_POWER_RES));

  return bonus;
}

// TRUE = resisted
// FALSE = Failed to resist
// modifier applies to victim, higher the better (for the victim)
bool power_resistance(struct char_data *ch, struct char_data *victim, int modifier)
{
  int resist = get_power_resist_mod(victim);
  int penetrate = d20(ch) + GET_PSIONIC_LEVEL(ch) + get_power_penetrate_mod(ch) + modifier;

  return (penetrate < resist);
}

/** returns true if the spell is a psionic power, or false if
 *  it is a spell or spell-like affect */
bool is_spellnum_psionic(int spellnum)
{
  if (spellnum >= PSIONIC_POWER_START && spellnum <= PSIONIC_POWER_END)
    return true;

  return false;
}

void absorb_energy_conversion(struct char_data *ch, int dam_type, int dam)
{
  switch (dam_type)
  {
  case DAM_FIRE:
  case DAM_COLD:
  case DAM_ACID:
  case DAM_SOUND:
  case DAM_ELECTRIC:
    if (!IS_NPC(ch) && affected_by_spell(ch, PSIONIC_ENERGY_CONVERSION) && dam >= 2 && ch->player_specials->energy_conversion[dam_type] < GET_PSIONIC_LEVEL(ch))
    {
      dam /= 2;
      if ((dam + ch->player_specials->energy_conversion[dam_type]) > GET_PSIONIC_LEVEL(ch))
        dam = GET_PSIONIC_LEVEL(ch) - ch->player_specials->energy_conversion[dam_type];
      ch->player_specials->energy_conversion[dam_type] += dam;
      send_to_char(ch, "\tY[Energy Conversion Absorb %s! +%d]\tn\r\n", damtypes[dam_type], dam / 2);
    }
    break;
  }
}

// returns true if the target doesn't have immunity to blindness
bool can_blind(struct char_data *ch)
{
  if (IS_NPC(ch) && MOB_FLAGGED(ch, MOB_NOBLIND))
    return false;
  if (affected_by_spell(ch, PSIONIC_OAK_BODY))
    return false;
  if (affected_by_spell(ch, PSIONIC_BODY_OF_IRON))
    return false;

  return true;
}

// returns true if the target doesn't have immunity to deafness
bool can_deafen(struct char_data *ch)
{
  if (IS_NPC(ch) && MOB_FLAGGED(ch, MOB_NODEAF))
    return false;
  if (affected_by_spell(ch, PSIONIC_OAK_BODY))
    return false;
  if (affected_by_spell(ch, PSIONIC_BODY_OF_IRON))
    return false;

  return true;
}

// returns true if the target doesn't have immunity to disease
bool can_disease(struct char_data *ch)
{
  if (affected_by_spell(ch, PSIONIC_OAK_BODY))
    return false;
  if (affected_by_spell(ch, PSIONIC_BODY_OF_IRON))
    return false;
  if (affected_by_spell(ch, PSIONIC_SHADOW_BODY))
    return false;
  if (HAS_REAL_FEAT(ch, FEAT_TOUGH_AS_BONE))
      return false;
  if (HAS_FEAT(ch, FEAT_DIVINE_HEALTH))
    return false;
  if (HAS_FEAT(ch, FEAT_DIAMOND_BODY))
    return false;
  if (HAS_FEAT(ch, FEAT_PLAGUE_BRINGER))
    return false;
  if (HAS_FEAT(ch, FEAT_DRACONIAN_DISEASE_IMMUNITY))
    return false;
  if (IS_CONSTRUCT(ch))
    return false;
  if (IS_UNDEAD(ch))
    return false;
  if (HAS_EVOLUTION(ch, EVOLUTION_CELESTIAL_APPEARANCE) && get_evolution_appearance_save_bonus(ch) == 100)
    return false;

  return true;
}

// returns true if the vict can be pushed or false if they are somehow immune
// also attempts a combat maneuver check at the end
bool push_attempt(struct char_data *ch, struct char_data *vict, bool display)
{
  if (!vict) return false;

  if (!IS_NPC(vict))
  {
    if (!PRF_FLAGGED(vict, PRF_PVP))
    {
      if (display)
        send_to_char(ch, "Your subject has their pvp flag turned off.\r\n");
      return false;
    }
    if (!PRF_FLAGGED(ch, PRF_PVP))
    {
      if (display)
        send_to_char(ch, "You have your pvp flag turned off.\r\n");
      return false;
    }
  }  

  if (IS_NPC(vict))
  {
    // we want nobash and nokill mobs at the top, to refuse
    // being pushed because we don't want people to charm
    // powerful mobs and push them out of the room to get
    // around having to kill them.  This attempts to
    // preserve builder intentions for zones they've created.
    if (MOB_FLAGGED(vict, MOB_NOBASH))
    {
      if (display)
        send_to_char(ch, "That mob can't be pushed.\r\n");
      return false;
    }
    if (MOB_FLAGGED(vict, MOB_NOKILL))
    {
      if (display)
        send_to_char(ch, "That mob can't be pushed.\r\n");
      return false;
    }
    if (vict->master && AFF_FLAGGED(vict, AFF_CHARM))
    {
      if (vict->master == ch)
        return true;
      else
      {
        if (display)
          send_to_char(ch, "That mob can't be pushed.\r\n");
        return false;
      }
    }
    if (MOB_FLAGGED(vict, MOB_SENTINEL))
    {
      if (display)
        send_to_char(ch, "That mob can't be pushed.\r\n");
      return false;
    }
    if (MOB_FLAGGED(vict, MOB_NOGRAPPLE))
    {
      if (display)
        send_to_char(ch, "That mob can't be pushed.\r\n");
      return false;
    }
  }

  if (AFF_FLAGGED(vict, AFF_FREE_MOVEMENT) || GET_LEVEL(vict) >= LVL_IMMORT)
  {
    if (display)
      send_to_char(ch, "They cannot be pushed.\r\n");
    return false;
  }

  if (AFF_FLAGGED(vict, AFF_PROTECT_EVIL) && IS_EVIL(ch) && mag_savingthrow(ch, vict, SAVING_WILL, 0, CAST_INNATE, GET_LEVEL(ch), NOSCHOOL))
  {
    if (display)
      send_to_char(ch, "A protective field repels you.\r\n");
    return false;
  }
    
  if (AFF_FLAGGED(vict, AFF_PROTECT_GOOD) && IS_GOOD(ch) && mag_savingthrow(ch, vict, SAVING_WILL, 0, CAST_INNATE, GET_LEVEL(ch), NOSCHOOL))
  {
    if (display)
      send_to_char(ch, "A protective field repels you.\r\n");
    return false;
  }

  if (AFF_FLAGGED(vict, AFF_STUN))
    return true;
  if (AFF_FLAGGED(vict, AFF_DAZED))
    return true;
  if (AFF_FLAGGED(vict, AFF_PARALYZED))
    return true;

  int cmb_bonus = 0;

  if (AFF_FLAGGED(vict, AFF_SLOW))
    cmb_bonus += 4;
  if (AFF_FLAGGED(ch, AFF_SLOW))
    cmb_bonus -= 4;
  if (AFF_FLAGGED(vict, AFF_HASTE))
    cmb_bonus += 2;
  if (AFF_FLAGGED(ch, AFF_HASTE))
    cmb_bonus += 2;

  if (!combat_maneuver_check(ch, vict, 0, cmb_bonus))
    return false;

  return true;
}

// returns true if they have blindsense, which allows
// seeing in the dark and if blinded.
bool has_blindsense(struct char_data *ch)
{
  if (HAS_FEAT(ch, FEAT_BLINDSENSE))
    return true;
  if (HAS_EVOLUTION(ch, EVOLUTION_BLINDSIGHT))
    return true;
  
  return false;
}

// returns true if the target doesn't have immunity to poison
bool can_poison(struct char_data *ch)
{
  if (affected_by_spell(ch, PSIONIC_OAK_BODY))
    return false;
  if (affected_by_spell(ch, PSIONIC_BODY_OF_IRON))
    return false;
  if (affected_by_spell(ch, PSIONIC_SHADOW_BODY))
    return false;
  if (HAS_FEAT(ch, FEAT_DIVINE_HEALTH))
    return false;
  if (HAS_REAL_FEAT(ch, FEAT_ESSENCE_OF_UNDEATH))
    return false;
  if (HAS_FEAT(ch, FEAT_DIAMOND_BODY))
    return false;
  if (IS_CONSTRUCT(ch))
    return false;
  if (IS_UNDEAD(ch))
    return false;
  if (HAS_EVOLUTION(ch, EVOLUTION_CELESTIAL_APPEARANCE) && get_evolution_appearance_save_bonus(ch) == 100)
    return false;

  return true;
}

// returns true if the target doesn't have immunity to stun
bool can_stun(struct char_data *ch)
{
  if (affected_by_spell(ch, PSIONIC_OAK_BODY))
    return false;
  if (affected_by_spell(ch, PSIONIC_BODY_OF_IRON))
    return false;
  if (IS_NPC(ch) && MOB_FLAGGED(ch, MOB_NOPARALYZE))
    return false;
  if (HAS_REAL_FEAT(ch, FEAT_TOUGH_AS_BONE))
    return false;
  if (HAS_EVOLUTION(ch, EVOLUTION_UNDEAD_APPEARANCE) && get_evolution_appearance_save_bonus(ch) == 100)
    return false;

  return true;
}

// returns true if the target doesn't have immunity to paralysis
bool can_paralyze(struct char_data *ch)
{
  return !paralysis_immunity(ch);
}

// returns true if the target doesn't have immunity to confusion
bool can_confuse(struct char_data *ch)
{
  if (IS_NPC(ch) && MOB_FLAGGED(ch, MOB_NOCONFUSE))
    return false;

  return true;
}

// returns true if under the effect of psionic body forms such as oak body, shadow body, body of iron
bool has_psionic_body_form_active(struct char_data *ch)
{
  if (affected_by_spell(ch, PSIONIC_OAK_BODY))
    return true;
  if (affected_by_spell(ch, PSIONIC_BODY_OF_IRON))
    return true;
  if (affected_by_spell(ch, PSIONIC_SHADOW_BODY))
    return true;

  return false;
}

bool can_spell_be_revoked(int spellnum)
{
  switch (spellnum)
  {
  // spells
  case SPELL_DJINNI_KIND:
  case SPELL_EFREETI_KIND:
  case SPELL_MARID_KIND:
  case SPELL_SHAITAN_KIND:
  case SPELL_WIND_WALL:
  case SPELL_GASEOUS_FORM:
  case SPELL_AIR_WALK:
  case SPELL_SHIELD_OF_FAITH:
  case SPELL_BLESS:
  case SPELL_DETECT_ALIGN:
  case SPELL_DETECT_INVIS:
  case SPELL_DETECT_MAGIC:
  case SPELL_DETECT_POISON:
  case SPELL_INVISIBLE:
  case SPELL_PROT_FROM_EVIL:
  case SPELL_SANCTUARY:
  case SPELL_STRENGTH:
  case SPELL_SENSE_LIFE:
  case SPELL_GROUP_SHIELD_OF_FAITH:
  case SPELL_INFRAVISION:
  case SPELL_WATERWALK:
  case SPELL_FLY:
  case SPELL_BLUR:
  case SPELL_MIRROR_IMAGE:
  case SPELL_STONESKIN:
  case SPELL_ENDURANCE:
  case SPELL_EPIC_MAGE_ARMOR:
  case SPELL_EPIC_WARDING:
  case SPELL_PROT_FROM_GOOD:
  case SPELL_POLYMORPH:
  case SPELL_ENDURE_ELEMENTS:
  case SPELL_EXPEDITIOUS_RETREAT:
  case SPELL_IRON_GUTS:
  case SPELL_MAGE_ARMOR:
  case SPELL_SHIELD:
  case SPELL_TRUE_STRIKE:
  case SPELL_FALSE_LIFE:
  case SPELL_GRACE:
  case SPELL_RESIST_ENERGY:
  case SPELL_WATER_BREATHE:
  case SPELL_HEROISM:
  case SPELL_NON_DETECTION:
  case SPELL_HASTE:
  case SPELL_CIRCLE_A_EVIL:
  case SPELL_CIRCLE_A_GOOD:
  case SPELL_CUNNING:
  case SPELL_WISDOM:
  case SPELL_CHARISMA:
  case SPELL_FIRE_SHIELD:
  case SPELL_COLD_SHIELD:
  case SPELL_GREATER_INVIS:
  case SPELL_MINOR_GLOBE:
  case SPELL_ENLARGE_PERSON:
  case SPELL_SHRINK_PERSON:
  case SPELL_FSHIELD_DAM:
  case SPELL_CSHIELD_DAM:
  case SPELL_ASHIELD_DAM:
  case SPELL_ACID_SHEATH:
  case SPELL_INTERPOSING_HAND:
  case SPELL_TRANSFORMATION:
  case SPELL_MASS_HASTE:
  case SPELL_GREATER_HEROISM:
  case SPELL_ANTI_MAGIC_FIELD:
  case SPELL_GREATER_MIRROR_IMAGE:
  case SPELL_TRUE_SEEING:
  case SPELL_GLOBE_OF_INVULN:
  case SPELL_MASS_FLY:
  case SPELL_MASS_INVISIBILITY:
  case SPELL_REPULSION:
  case SPELL_MASS_CHARM_MONSTER:
  case SPELL_DISPLACEMENT:
  case SPELL_PROTECT_FROM_SPELLS:
  case SPELL_SPELL_MANTLE:
  case SPELL_MASS_WISDOM:
  case SPELL_MASS_CHARISMA:
  case SPELL_REFUGE:
  case SPELL_SPELL_TURNING:
  case SPELL_MIND_BLANK:
  case SPELL_IRONSKIN:
  case SPELL_MASS_CUNNING:
  case SPELL_SHADOW_SHIELD:
  case SPELL_GREATER_SPELL_MANTLE:
  case SPELL_MASS_ENHANCE:
  case SPELL_HOLY_SWORD:
  case SPELL_AID:
  case SPELL_BRAVERY:
  case SPELL_REGENERATION:
  case SPELL_FREE_MOVEMENT:
  case SPELL_STRENGTHEN_BONE:
  case SPELL_PRAYER:
  case SPELL_WORD_OF_FAITH:
  case SPELL_SALVATION:
  case SPELL_SPRING_OF_LIFE:
  case SPELL_DEATH_SHIELD:
  case SPELL_AIR_WALKER:
  case SPELL_JUMP:
  case SPELL_MAGIC_FANG:
  case SPELL_BARKSKIN:
  case SPELL_FLAME_BLADE:
  case SPELL_GREATER_MAGIC_FANG:
  case SPELL_DEATH_WARD:
  case SPELL_MASS_ENDURANCE:
  case SPELL_MASS_STRENGTH:
  case SPELL_MASS_GRACE:
  case SPELL_SPELL_RESISTANCE:
  case SPELL_SPELLSTAFF:
  case SPELL_SHAPECHANGE:
  case SPELL_BLADE_BARRIER:
  case SPELL_BATTLETIDE:
  case SPELL_LEVITATE:
  case SPELL_CONTINUAL_LIGHT:
  case SPELL_ELEMENTAL_RESISTANCE:
  case SPELL_PRESERVE:
  case SPELL_ESHIELD_DAM:
  case SPELL_MASS_FALSE_LIFE:
  case SPELL_SHADOW_WALK:
  case SPELL_INCORPOREAL_FORM:
  case SPELL_HEAL_MOUNT:
  case SPELL_RESISTANCE:
  case SPELL_LESSER_RESTORATION:
  case SPELL_HEDGING_WEAPONS:
  case SPELL_HONEYED_TONGUE:
  case SPELL_SHIELD_OF_FORTIFICATION:
  case SPELL_STUNNING_BARRIER:
  case SPELL_SUN_METAL:
  case SPELL_TACTICAL_ACUMEN:
  case SPELL_VEIL_OF_POSITIVE_ENERGY:
  case SPELL_BESTOW_WEAPON_PROFICIENCY:
  case SPELL_SPIRITUAL_WEAPON:
  case SPELL_DANCING_WEAPON:
  case SPELL_WEAPON_OF_AWE:
  case SPELL_MAGIC_VESTMENT:
  case SPELL_GREATER_MAGIC_WEAPON:
  case SPELL_WEAPON_OF_IMPACT:
  case SPELL_KEEN_EDGE:
  case SPELL_PROTECTION_FROM_ENERGY:
  case SPELL_DIVINE_POWER:
  case SPELL_MINOR_ILLUSION:
  case SPELL_WARDING_WEAPON:
  case SPELL_GIRD_ALLIES:
  case SPELL_PROTECTION_FROM_ARROWS:
  case SPELL_HUMAN_POTENTIAL:
  case SPELL_SPIDER_CLIMB:
  case SPELL_RAGE:
  case SPELL_CAUSTIC_BLOOD:
  case SPELL_HOLY_AURA:

  // psionic powers
  case PSIONIC_BROKER:
  case PSIONIC_CALL_TO_MIND:
  case PSIONIC_CATFALL:
  case PSIONIC_FORCE_SCREEN:
  case PSIONIC_FORTIFY:
  case PSIONIC_INERTIAL_ARMOR:
  case PSIONIC_INEVITABLE_STRIKE:
  case PSIONIC_DEFENSIVE_PRECOGNITION:
  case PSIONIC_OFFENSIVE_PRECOGNITION:
  case PSIONIC_OFFENSIVE_PRESCIENCE:
  case PSIONIC_VIGOR:
  case PSIONIC_BIOFEEDBACK:
  case PSIONIC_BODY_EQUILIBRIUM:
  case PSIONIC_CONCEALING_AMORPHA:
  case PSIONIC_DETECT_HOSTILE_INTENT:
  case PSIONIC_ELFSIGHT:
  case PSIONIC_ENERGY_ADAPTATION_SPECIFIED:
  case PSIONIC_PSYCHIC_BODYGUARD:
  case PSIONIC_SHARE_PAIN:
  case PSIONIC_THOUGHT_SHIELD:
  case PSIONIC_BODY_ADJUSTMENT:
  case PSIONIC_ENDORPHIN_SURGE:
  case PSIONIC_ENERGY_RETORT:
  case PSIONIC_HEIGHTENED_VISION:
  case PSIONIC_MENTAL_BARRIER:
  case PSIONIC_MIND_TRAP:
  case PSIONIC_SHARPENED_EDGE:
  case PSIONIC_UBIQUITUS_VISION:
  case PSIONIC_EMPATHIC_FEEDBACK:
  case PSIONIC_ENERGY_ADAPTATION:
  case PSIONIC_INTELLECT_FORTRESS:
  case PSIONIC_SLIP_THE_BONDS:
  case PSIONIC_ADAPT_BODY:
  case PSIONIC_PIERCE_VEIL:
  case PSIONIC_POWER_RESISTANCE:
  case PSIONIC_TOWER_OF_IRON_WILL:
  case PSIONIC_REMOTE_VIEW_TRAP:
  case PSIONIC_SUSTAINED_FLIGHT:
  case PSIONIC_BARRED_MIND:
  case PSIONIC_COSMIC_AWARENESS:
  case PSIONIC_ENERGY_CONVERSION:
  case PSIONIC_EVADE_BURST:
  case PSIONIC_OAK_BODY:
  case PSIONIC_BODY_OF_IRON:
  case PSIONIC_SHADOW_BODY:
  case PSIONIC_TIMELESS_BODY:
  case PSIONIC_BARRED_MIND_PERSONAL:
  case PSIONIC_TRUE_METABOLISM:
  case PSIONIC_ASSIMILATE:
  case PSIONIC_CONCUSSIVE_ONSLAUGHT:

  // Other
  case RACIAL_ABILITY_CRYSTAL_BODY:
  case RACIAL_ABILITY_CRYSTAL_FIST:
  case RACIAL_ABILITY_INSECTBEING:
  case AFFECT_FOOD:
  case AFFECT_DRINK:
  case ABILITY_STRENGTH_OF_HONOR:
  case ABILITY_CROWN_OF_KNIGHTHOOD:
  case AFFECT_HOLY_AURA_RETRIBUTION:
  case AFFECT_RALLYING_CRY:
  case AFFECT_INSPIRE_COURAGE:
  case AFFECT_FINAL_STAND:
  case AFFECT_KNIGHTHOODS_FLOWER:
  case AFFECT_INSPIRE_GREATNESS:

    return true;
  }
  return false;
}

// returns true if no damage to be applied
// returns false if damage should proceed normally
bool process_iron_golem_immunity(struct char_data *ch, struct char_data *victim, int element, int dam)
{

  if (HAS_FEAT(victim, FEAT_IRON_GOLEM_IMMUNITY) && element == DAM_FIRE)
  {
    GET_HIT(victim) += dam;
    if (GET_HIT(victim) > GET_MAX_HIT(victim))
      GET_HIT(victim) = GET_MAX_HIT(victim);
    act("The spell heals you instead!", TRUE, ch, 0, victim, TO_VICT);
    act("The spell heals $N instead!", TRUE, ch, 0, victim, TO_ROOM);
    return true;
  }
  if (HAS_FEAT(victim, FEAT_IRON_GOLEM_IMMUNITY) && element == DAM_ELECTRIC)
  {
    struct affected_type af;
    new_affect(&af);
    af.spell = SPELL_SLOW;
    af.duration = 3;
    SET_BIT_AR(af.bitvector, AFF_SLOW);
    affect_join(victim, &af, FALSE, FALSE, FALSE, FALSE);
    act("The spell deals no damage, but slows you instead!", TRUE, ch, 0, victim, TO_CHAR);
    act("The spell deals no damage, but slows $N instead!", TRUE, ch, 0, victim, TO_ROOM);
    return true;
  }
  return false;
}

int smite_evil_target_type(struct char_data *ch)
{
  if (!ch)
    return 0;

  if (!IS_EVIL(ch))
    return 1;

  if (IS_DRAGON(ch) || IS_OUTSIDER(ch) || IS_UNDEAD(ch))
    return 3;

  return 2;
}

int smite_good_target_type(struct char_data *ch)
{
  if (!ch)
    return 0;

  if (!IS_GOOD(ch))
    return 1;

  if (IS_DRAGON(ch) || IS_OUTSIDER(ch) ||
      CLASS_LEVEL(ch, CLASS_CLERIC) > 0 ||
      CLASS_LEVEL(ch, CLASS_PALADIN) > 0)
    return 3;

  return 2;
}

/** This will run every 6 seconds as well as whenever a new affect is added or removed
 *  from a character, such as receiving or losing a spell affect, wearing or removing
 *  a piece of gear, etc. This should only ever be used upon players, not NPCs.
 */
void calculate_max_hp(struct char_data *ch, bool display)
{
  if (!ch || IS_NPC(ch) || !ch->desc)
    return;

  int i = 0, j = 0;
  int max_hp = 20; // start point
  int max_value[NUM_BONUS_TYPES];
  int max_val_spell[NUM_BONUS_TYPES];
  int max_val_worn_slot[NUM_BONUS_TYPES];
  struct obj_data *obj = NULL;
  struct affected_type *aff = NULL;
  char affect_buf[2400], gear_buf[2400], temp_buf[200];

  for (i = 0; i < NUM_BONUS_TYPES; i++)
  {
    max_value[i] = 0;
    max_val_spell[i] = -1;
    max_val_worn_slot[i] = -1;
  }
  snprintf(affect_buf, sizeof(affect_buf), "\tn");
  snprintf(gear_buf, sizeof(gear_buf), "\tn");

  // base amount
  if (display)
    send_to_char(ch, "%-40s = +20\r\n", "Base Amount");

  // classes
  for (i = 0; i < NUM_CLASSES; i++)
  {
    max_hp += CLASS_LEVEL(ch, i) * class_list[i].hit_dice;
    if (display && CLASS_LEVEL(ch, i) > 0)
    {
      snprintf(temp_buf, sizeof(temp_buf), "%d levels in %s", CLASS_LEVEL(ch, i), class_list[i].name);
      send_to_char(ch, "%-40s = +%d\r\n", temp_buf, CLASS_LEVEL(ch, i) * class_list[i].hit_dice);
    }
  }

  // con
  max_hp += GET_LEVEL(ch) * GET_CON_BONUS(ch);
  if (display && GET_CON_BONUS(ch) != 0)
  {
    send_to_char(ch, "%-40s = %s%d\r\n", "Constitution bonus", GET_CON_BONUS(ch) > 0 ? "+" : "", GET_CON_BONUS(ch) * GET_LEVEL(ch));
  }

  // We'll do spells then gear.  We want to make sure that bonus
  // types don't stack

  // spells
  for (aff = ch->affected; aff; aff = aff->next)
  {
    if (aff->location == APPLY_HIT)
    {
      // some bonus types always stack, so we'll just add this right on now
      if (BONUS_TYPE_STACKS(aff->bonus_type))
      {
        max_hp += aff->modifier;
        if (display)
        {
          snprintf(temp_buf, sizeof(temp_buf), "Affect '%s'", spell_info[aff->spell].name);
          snprintf(affect_buf, sizeof(affect_buf), "%s%-40s = %s%d\r\n", affect_buf, temp_buf, aff->modifier > 0 ? "+" : "", aff->modifier);
        }
      }
      // penalties and debuffs are always applied
      else if (aff->modifier < 0)
      {
        max_hp -= aff->modifier;
        if (display)
        {
          snprintf(temp_buf, sizeof(temp_buf), "Affect '%s'", spell_info[aff->spell].name);
          snprintf(affect_buf, sizeof(affect_buf), "%s%-40s = %d\r\n", affect_buf, temp_buf, aff->modifier);
        }
      }
      // we only want the maximum per bonus type
      else if (aff->modifier > max_value[aff->bonus_type])
      {
        max_value[aff->bonus_type] = aff->modifier;
        max_val_spell[aff->bonus_type] = aff->spell;
        max_val_worn_slot[aff->bonus_type] = -1;
      }
    }
  }

  // gear
  for (i = 0; i < NUM_WEARS; i++)
  {
    if (!(obj = GET_EQ(ch, i)))
      continue;
    for (j = 0; j < MAX_OBJ_AFFECT; j++)
    {
      if (obj->affected[j].location == APPLY_HIT)
      {
        // some bonus types always stack, so we'll just add this right on now
        if (BONUS_TYPE_STACKS(obj->affected[j].bonus_type))
        {
          max_hp += obj->affected[j].modifier;
          if (display)
          {
            snprintf(temp_buf, sizeof(temp_buf), "Worn Item '%s'", obj->short_description);
            snprintf(gear_buf, sizeof(gear_buf), "%s%-40s = %s%d\r\n", gear_buf, temp_buf, obj->affected[j].modifier > 0 ? "+" : "", obj->affected[j].modifier);
          }
        }
        // penalties and debuffs are always applied
        else if (obj->affected[j].modifier < 0)
        {
          max_hp -= obj->affected[j].modifier;
          if (display)
          {
            snprintf(temp_buf, sizeof(temp_buf), "Worn Item '%s'", obj->short_description);
            snprintf(gear_buf, sizeof(gear_buf), "%s%-40s = %s%d\r\n", gear_buf, temp_buf, obj->affected[j].modifier > 0 ? "+" : "", obj->affected[j].modifier);
          }
        }
        // we only want the maximum per bonus type
        else if (obj->affected->modifier > max_value[obj->affected[j].bonus_type])
        {
          max_value[obj->affected[j].bonus_type] = obj->affected[j].modifier;
          max_val_spell[obj->affected[j].bonus_type] = -1;
          max_val_worn_slot[obj->affected[j].bonus_type] = i;
        }
      }
    }
  }

  // now let's add up all of the highest per bonus type from spells and gear
  for (i = 0; i < NUM_BONUS_TYPES; i++)
  {
    if (BONUS_TYPE_STACKS(i))
      continue;
    max_hp += max_value[i];
    if (display)
    {
      if (max_val_spell[i] != -1)
      {
        snprintf(temp_buf, sizeof(temp_buf), "Affect '%s'", spell_info[max_val_spell[i]].name);
        snprintf(affect_buf, sizeof(affect_buf), "%s%-40s = %s%d\r\n", affect_buf, temp_buf, max_value[i] > 0 ? "+" : "", max_value[i]);
      }
      else if (max_val_worn_slot[i] != -1)
      {
        for (j = 0; j < MAX_OBJ_AFFECT; j++)
        {
          if (GET_EQ(ch, max_val_worn_slot[i])->affected[j].location == APPLY_HIT)
          {
            snprintf(temp_buf, sizeof(temp_buf), "Worn Item '%s'", GET_EQ(ch, max_val_worn_slot[i])->short_description);
            snprintf(gear_buf, sizeof(gear_buf), "%s%-40s = %s%d\r\n", gear_buf, temp_buf,
                     GET_EQ(ch, max_val_worn_slot[i])->affected[j].modifier > 0 ? "+" : "",
                     GET_EQ(ch, max_val_worn_slot[i])->affected[j].modifier);
          }
        }
      }
    }
  }
  if (display)
  {
    send_to_char(ch, "%s", affect_buf);
    send_to_char(ch, "%s", gear_buf);
  }

  // feats
  if (HAS_FEAT(ch, FEAT_TOUGHNESS))
  {
    max_hp += GET_LEVEL(ch) * HAS_FEAT(ch, FEAT_TOUGHNESS);
    if (display)
      send_to_char(ch, "%-40s = +%d\r\n", "Feat 'Toughness'", GET_LEVEL(ch) * HAS_FEAT(ch, FEAT_TOUGHNESS));
  }
  if (HAS_FEAT(ch, FEAT_EPIC_TOUGHNESS))
  {
    max_hp += 30 * HAS_FEAT(ch, FEAT_EPIC_TOUGHNESS);
    if (display)
      send_to_char(ch, "%-40s = +%d\r\n", "Feat 'Epic Toughness'", 30 * HAS_FEAT(ch, FEAT_EPIC_TOUGHNESS));
  }

  // race
  if (GET_REAL_RACE(ch) == RACE_CRYSTAL_DWARF)
  {
    max_hp += GET_LEVEL(ch) * 4;
    if (display)
      send_to_char(ch, "%-40s = +%d\r\n", "Crystal Dwarf Racial Hit Point Bonus", GET_LEVEL(ch) * 4);
  }
  if (GET_REAL_RACE(ch) == RACE_TRELUX)
  {
    max_hp += GET_LEVEL(ch) * 4;
    if (display)
      send_to_char(ch, "%-40s = +%d\r\n", "Trelux Racial Hit Point Bonus", GET_LEVEL(ch) * 4);
  }
  if (GET_REAL_RACE(ch) == RACE_LICH)
  {
    max_hp += GET_LEVEL(ch) * 4;
    if (display)
      send_to_char(ch, "%-40s = +%d\r\n", "Lich Racial Hit Point Bonus", GET_LEVEL(ch) * 4);
  }
  if (GET_REAL_RACE(ch) == RACE_VAMPIRE)
  {
    max_hp += GET_LEVEL(ch) * 4;
    if (display)
      send_to_char(ch, "%-40s = +%d\r\n", "Vampire Racial Hit Point Bonus", GET_LEVEL(ch) * 4);
  }

  // misc
  max_hp += CONFIG_EXTRA_PLAYER_HP_PER_LEVEL * GET_LEVEL(ch);
  if (display && CONFIG_EXTRA_PLAYER_HP_PER_LEVEL > 0)
    send_to_char(ch, "%-40s = +%d\r\n", "Game Setting 'Extra HP Gains'", GET_LEVEL(ch) * CONFIG_EXTRA_PLAYER_HP_PER_LEVEL);

  if (display)
  {
    for (i = 0; i < 80; i++)
      send_to_char(ch, "-");

    send_to_char(ch, "\r\n\tY%-40s = %d\tn\r\n", "Final Maximum Hit Points", max_hp);
  }

  GET_REAL_MAX_HIT(ch) = GET_MAX_HIT(ch) = max_hp;
}

void dismiss_all_followers(struct char_data *ch)
{
  if (!ch || ch->master || IS_NPC(ch))
    return;

  struct follow_type *k = NULL;

  for (k = ch->followers; k; k = k->next)
    if (IS_NPC(k->follower) && AFF_FLAGGED(k->follower, AFF_CHARM))
    {
      k->follower->char_specials.is_charmie = true;
      extract_char(k->follower);
    }
}

void remove_any_spell_with_aff_flag(struct char_data *ch, struct char_data *vict, int aff_flag, bool display)
{
  if (!ch || !vict)
    return;

  if (aff_flag == AFF_DONTUSE)
    return;

  struct affected_type *af = NULL, *next_af = NULL;

  int spell = 0;

  for (af = vict->affected; af; af = next_af)
  {
    next_af = af->next;
    
    if (IS_SET_AR(af->bitvector, aff_flag))
    {
      spell = af->spell;
      if (spell < 1 || spell >= TOP_SPELLS_POWERS_SKILLS_BOMBS)
        continue;
      if (display)
      {
        send_to_char(vict, "Affect '%s' has been healed!\r\n", spell_info[spell].name);
        send_to_char(ch, "%s's Affect '%s' has been healed!\r\n", GET_NAME(vict), spell_info[spell].name);
      }
      affect_from_char(vict, af->spell);
    }
  }
}

char *randstring(int length)
{
  if (length < 1 || length > 255)
    return NULL;

  char buf[length + 1];
  char char_list[64];
  int i = 0;

  snprintf(char_list, sizeof(char_list), "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ01234567890");

  for (i = 0; i < length; i++)
    buf[i] = char_list[dice(1, 63) - 1];

  buf[length + 1] = '\0';
  return strdup(buf);
}

bool is_paladin_mount(struct char_data *ch, struct char_data *victim)
{
  if (!victim)
    return false;
  if (!IS_NPC(victim))
    return false;
  if (!AFF_FLAGGED(victim, AFF_CHARM))
    return false;
  if (victim->master != ch)
    return false;

  switch (GET_MOB_VNUM(victim))
  {
  case MOB_PALADIN_MOUNT:
  case MOB_PALADIN_MOUNT_SMALL:
  case MOB_EPIC_PALADIN_MOUNT:
  case MOB_EPIC_PALADIN_MOUNT_SMALL:
    return true;
  }
  return false;
}

// determines if vict is a valid mark target for various
// assassin feat functionality
bool is_marked_target(struct char_data *ch, struct char_data *vict)
{
  if (!ch || !vict)
    return false;

  if (HAS_FEAT(ch, FEAT_ANGEL_OF_DEATH))
    return true;

  if (!HAS_FEAT(ch, FEAT_DEATH_ATTACK))
    return false;

  if (GET_MARK(ch) == vict && GET_MARK_ROUNDS(ch) >= 3)
    return true;

  return false;
}

// applies any special affects an assassin has when performing a backstab on
// a marked target
void apply_assassin_backstab_bonuses(struct char_data *ch, struct char_data *vict)
{

  if (is_marked_target(ch, vict))
  {
    if (HAS_FEAT(ch, FEAT_TRUE_DEATH))
    {
      GET_MARK_HIT_BONUS(ch) += 2;
      GET_MARK_DAM_BONUS(ch) += 10;
    }
    if (HAS_FEAT(ch, FEAT_ANGEL_OF_DEATH))
    {
      GET_MARK_HIT_BONUS(ch) += 3;
      GET_MARK_DAM_BONUS(ch) += 10;
    }
    if (HAS_FEAT(ch, FEAT_QUIET_DEATH) && !affected_by_spell(ch, SPELL_GREATER_INVIS) && !affected_by_spell(ch, SPELL_INVISIBLE))
    {
      act("You fade from view as you perform your death attack.", FALSE, ch, 0, 0, TO_CHAR);
      act("$n fades into invisibility as $e performs a death attack.", TRUE, ch, 0, 0, TO_ROOM);

      struct affected_type invis_effect;

      new_affect(&invis_effect);
      invis_effect.spell = SPELL_GREATER_INVIS;
      invis_effect.duration = 3;
      SET_BIT_AR(invis_effect.bitvector, AFF_INVISIBLE);
      affect_join(ch, &invis_effect, TRUE, FALSE, FALSE, FALSE);
    }
  }
}

// This will return the saving throw bonus for evolutions like
// undead appearance, celestial appearance and fiendish appearance.
int get_evolution_appearance_save_bonus(struct char_data *ch)
{
  if (!ch)
    return 0;
  
  // at level 12, they're immune
  if (GET_CALL_EIDOLON_LEVEL(ch) >= 12)
    return 100;
  if (GET_CALL_EIDOLON_LEVEL(ch) >= 7)
    return +4;

  return +2;
}

void remove_locked_door_flags(room_rnum room, int door)
{
  REMOVE_BIT(EXITN(room, door)->exit_info, EX_LOCKED);
  REMOVE_BIT(EXITN(room, door)->exit_info, EX_LOCKED_EASY);
  REMOVE_BIT(EXITN(room, door)->exit_info, EX_LOCKED_MEDIUM);
  REMOVE_BIT(EXITN(room, door)->exit_info, EX_LOCKED_HARD);
}

bool is_door_locked(room_rnum room, int door)
{

  if (room == NOWHERE)
    return false;

  if (door < NORTH || door > NUM_OF_DIRS)
    return false;

  if (IS_SET(EXITN(room, door)->exit_info, EX_LOCKED) ||
      IS_SET(EXITN(room, door)->exit_info, EX_LOCKED_EASY) ||
      IS_SET(EXITN(room, door)->exit_info, EX_LOCKED_MEDIUM) ||
      IS_SET(EXITN(room, door)->exit_info, EX_LOCKED_HARD))
    return true;

  return false;
}

int get_judgement_bonus(struct char_data *ch, int type)
{

  if (!HAS_REAL_FEAT(ch, FEAT_JUDGEMENT))
    return 0;

  int bonus = 1;
  int level = CLASS_LEVEL(ch, CLASS_INQUISITOR);
  if (HAS_REAL_FEAT(ch, FEAT_SLAYER) && type == GET_SLAYER_JUDGEMENT(ch))
    level += 5;

  switch (type)
  {
  case INQ_JUDGEMENT_DESTRUCTION:
  case INQ_JUDGEMENT_HEALING:
  case INQ_JUDGEMENT_PIERCING:
  case INQ_JUDGEMENT_RESISTANCE:
    bonus += level / 3;
    break;

  case INQ_JUDGEMENT_JUSTICE:
  case INQ_JUDGEMENT_PROTECTION:
  case INQ_JUDGEMENT_PURITY:
  case INQ_JUDGEMENT_RESILIENCY:
    bonus += level / 5;
    break;
  }

  bonus += HAS_REAL_FEAT(ch, FEAT_PERFECT_JUDGEMENT);

  if (type == INQ_JUDGEMENT_RESISTANCE)
    bonus *= 4;

  return bonus;
}

int judgement_bonus_type(struct char_data *ch)
{
  if (IS_EVIL(ch))
    return BONUS_TYPE_PROFANE;
  else
    return BONUS_TYPE_SACRED;
}

bool is_judgement_possible(struct char_data *ch, struct char_data *t, int type)
{
  if (!ch || !t)
    return false;

  if (!HAS_REAL_FEAT(ch, FEAT_JUDGEMENT))
    return false;

  if (GET_JUDGEMENT_TARGET(ch) != t)
    return false;

  if (!IS_JUDGEMENT_ACTIVE(ch, type))
    return false;

  return true;
}

// Returns 0 if no other group members have the teamwork feat
// or if the character doesn't either.
// Otherwise returns the number of people in the group who have the feat.
int has_teamwork_feat(struct char_data *ch, int featnum)
{

  if (!ch)
    return 0;
  if (!GROUP(ch))
    return 0;

  if (!HAS_REAL_FEAT(ch, featnum))
    return false;

  struct char_data *k = NULL;
  int num_with_feat = 0;

  while ((k = (struct char_data *)simple_list(ch->group->members)) != NULL)
  {
    if (IN_ROOM(k) != IN_ROOM(ch))
      continue;
    if (HAS_REAL_FEAT(k, featnum) || HAS_REAL_FEAT(ch, FEAT_SOLO_TACTICS))
    {
      num_with_feat++;
    }
  }

  if (num_with_feat > 0)
    return (num_with_feat + 1); // we include ch here as well

  return 0;
}

// This function will return the total bonus number, which works either as
// a return of TRUE or for the bonus number for feats like shield wall.
// If no party members who have the featnum have a shield, it returns FALSE.
int teamwork_using_shield(struct char_data *ch, int featnum)
{
  struct char_data *k = NULL;
  struct obj_data *shield = NULL;
  int bonus = 0;

  if (!ch)
    return 0;
  if (!GROUP(ch))
    return 0;

  while ((k = (struct char_data *)simple_list(ch->group->members)) != NULL)
  {
    if (IN_ROOM(k) != IN_ROOM(ch))
      continue;
    if (HAS_REAL_FEAT(k, featnum) || HAS_REAL_FEAT(ch, FEAT_SOLO_TACTICS))
    {
      if ((shield = GET_EQ(k, WEAR_SHIELD)) && IS_SHIELD(GET_OBJ_VAL(shield, 1)))
      {
        if (GET_OBJ_VAL(shield, 1) == SPEC_ARMOR_TYPE_SMALL_SHIELD || GET_OBJ_VAL(shield, 1) == SPEC_ARMOR_TYPE_BUCKLER)
          bonus++;
        else
          bonus += 2;
      }
    }
  }

  return bonus;
}

int num_fighting(struct char_data *ch)
{
  struct char_data *k = NULL, *tch = NULL;
  int count = 0;

  if (!ch)
    return 0;
  if (!GROUP(ch))
    return 0;

  for (tch = world[IN_ROOM(ch)].people; tch; tch = tch->next_in_room)
  {
    if (tch == FIGHTING(ch) || ch == FIGHTING(tch))
    {
      count++;
    }
    else
    {
      while ((k = (struct char_data *)simple_list(ch->group->members)) != NULL)
        if (tch == FIGHTING(k) || k == FIGHTING(tch))
          count++;
    }
  }
  return count;
}

int teamwork_best_stealth(struct char_data *ch, int featnum)
{
  if (!HAS_REAL_FEAT(ch, featnum))
    return false;

  if (!ch)
    return 0;
  if (!GROUP(ch))
    return 0;

  struct char_data *k = NULL;
  int stealth = compute_ability_full(ch, ABILITY_STEALTH, FALSE);

  while ((k = (struct char_data *)simple_list(ch->group->members)) != NULL)
  {
    if (IN_ROOM(k) != IN_ROOM(ch))
      continue;
    if (HAS_REAL_FEAT(k, featnum) || HAS_REAL_FEAT(ch, FEAT_SOLO_TACTICS))
    {
      stealth = MAX(stealth, compute_ability_full(k, ABILITY_STEALTH, TRUE));
    }
  }

  return stealth;
}

int teamwork_best_d20(struct char_data *ch, int featnum)
{
  if (!HAS_REAL_FEAT(ch, featnum))
    return false;

  if (!ch)
    return 0;
  if (!GROUP(ch))
    return 0;

  struct char_data *k = NULL;
  int d20roll = d20(ch);

  while ((k = (struct char_data *)simple_list(ch->group->members)) != NULL)
  {
    if (IN_ROOM(k) != IN_ROOM(ch))
      continue;
    if (HAS_REAL_FEAT(k, featnum) || HAS_REAL_FEAT(ch, FEAT_SOLO_TACTICS))
    {
      d20roll = MAX(d20roll, d20(ch));
    }
  }

  return d20roll;
}

int count_teamwork_feats_available(struct char_data *ch)
{

  int i = 0;
  int known = 0;
  int available = HAS_REAL_FEAT(ch, FEAT_TEAMWORK);

  if (!ch)
    return 0;

  for (i = 0; i < NUM_FEATS; i++)
  {
    if (feat_list[i].feat_type == FEAT_TYPE_TEAMWORK && HAS_REAL_FEAT(ch, i))
      known++;
  }

  return (available - known);
}

bool can_silence(struct char_data *ch)
{
  return true;
}

int get_default_spell_weapon(struct char_data *ch)
{
  if (!ch)
    return WEAPON_TYPE_UNARMED;

  if (IS_EVIL(ch))
    return WEAPON_TYPE_FLAIL;
  else if (IS_GOOD(ch))
    return WEAPON_TYPE_WARHAMMER;
  else if (IS_LAWFUL(ch))
    return WEAPON_TYPE_LONG_SWORD;
  else if (IS_CHAOTIC(ch))
    return WEAPON_TYPE_BATTLE_AXE;

  return WEAPON_TYPE_HEAVY_MACE;
}

bool is_caster_class(int class)
{
  switch (class)
  {
  case CLASS_WIZARD:
  case CLASS_CLERIC:
  case CLASS_PALADIN:
  case CLASS_RANGER:
  case CLASS_BARD:
  case CLASS_DRUID:
  case CLASS_ALCHEMIST:
  case CLASS_INQUISITOR:
  case CLASS_BLACKGUARD:
  case CLASS_SORCERER:
  case CLASS_SUMMONER:
    return true;
  }
  return false;
}

int count_spellcasting_classes(struct char_data *ch)
{
  if (!ch)
    return 0;

  int num = 0, i = 0;

  for (i = 0; i < NUM_CLASSES; i++)
    if (is_caster_class(i) && CLASS_LEVEL(ch, i) > 0)
      num++;

  return num;
}

int get_spellcasting_class(struct char_data *ch)
{
  if (!ch)
    return 0;

  int i = 0;

  for (i = 0; i < NUM_CLASSES; i++)
    if (is_caster_class(i) && CLASS_LEVEL(ch, i) > 0)
      return i;

  return -1;
}

// will return true if the sector type offers opportunity for cover
bool can_room_sector_give_cover(int type)
{
  switch (type)
  {
  case SECT_CITY:          // people, crates, etc.
  case SECT_FOREST:        // trees and shrubs
  case SECT_HIGH_MOUNTAIN: // boulders, snow drifts
  case SECT_JUNGLE:        // trees, brush
  case SECT_MARSHLAND:     // trees
  case SECT_MOUNTAIN:      // boulders
    return true;
  }
  return false;
}

// will return true if the character has cover
bool has_cover(struct char_data *ch, struct char_data *t)
{

  if (!ch || !t)
    return false;

  if (FIGHTING(t) == ch)
    return false;

  room_rnum room = IN_ROOM(ch);
  if (room != NOWHERE && can_room_sector_give_cover(world[room].sector_type))
    return true;

  return false;
}

// will return true if the conditions allow for the use of the
// one with shadow feat.
bool can_one_with_shadows(struct char_data *ch)
{

  if (!ch)
    return false;

  if (!HAS_REAL_FEAT(ch, FEAT_ONE_WITH_SHADOW)) return false;

  if (compute_concealment(ch, NULL) > 0)
    return true;

  if (has_cover(ch, FIGHTING(ch)))
    return true;

  if (!ch->group)
    return false;

  struct char_data *k = NULL;
  int num = 0;
  while ((k = (struct char_data *)simple_list(ch->group->members)) != NULL)
  {
    if (IN_ROOM(k) != IN_ROOM(ch))
      continue;
    if (k == ch)
      continue;
    num++;
  }
  if (num > 0)
    return true;

  return false;
}

// This will check if there is a creature in the room, aside from the 'ch'
// which is both larger than ch and not targetting ch in battle.
bool can_naturally_stealthy(struct char_data *ch)
{

  if (!ch)
    return false;
  if (IN_ROOM(ch) == NOWHERE)
    return false;
  if (!HAS_REAL_FEAT(ch, FEAT_NATURALLY_STEALTHY))
    return false;

  struct char_data *t = NULL;
  int num = 0;

  for (t = world[IN_ROOM(ch)].people; t; t = t->next_in_room)
  {
    if (t == ch)
      continue;
    if (FIGHTING(t) == ch)
      continue;
    if (GET_SIZE(t) <= GET_SIZE(ch))
      continue;
    num++;
  }

  if (num > 0)
    return true;

  return false;
}

// returns true if the damage type is to be displayed on certain
// lists, like the resistancesd command.
bool display_dam_type(int dam_type)
{
  switch (dam_type)
  {
  case DAM_CHAOS:
  /* case DAM_SUNLIGHT:
  case DAM_MOVING_WATER: */
  case DAM_CELESTIAL_POISON:
  case DAM_BLEEDING:
  case DAM_TEMPORAL:
    return false;
  }
  return true;
}

bool is_swimming(struct char_data *ch)
{
  if (!ch)
    return false;
  if (IN_ROOM(ch) == NOWHERE)
    return false;

  switch (world[IN_ROOM(ch)].sector_type)
  {
  case SECT_OCEAN:
  case SECT_UD_WATER:
  case SECT_UNDERWATER:
  case SECT_WATER_SWIM:
    /* case SECT_RIVER: */
    return true;
  }
  return false;
}

bool is_ghost(struct char_data *ch)
{
  if (!ch)
    return false;

  /* if (HAS_TEMPLATE(ch, TEMPLATE_GHOST))
    return true; */

  return false;
}

void clear_group_marks(struct char_data *ch, struct char_data *victim)
{
  if (!ch || !victim || IN_ROOM(ch) == NOWHERE || IN_ROOM(victim) == NOWHERE)
    return;

  struct char_data *tch = NULL;

  for (tch = world[IN_ROOM(ch)].people; tch; tch = tch->next_in_room)
  {
    if (!is_player_grouped(tch, ch))
      continue;
    if (GET_MARK(tch) == victim)
    {
      GET_MARK(tch) = NULL;
      GET_MARK_ROUNDS(tch) = 0;
    }
  }
}

bool is_poison_spell(int spell)
{
  switch (spell)
  {
    case POISON_TYPE_SCORPION_WEAK:
    case POISON_TYPE_SCORPION_NORMAL:
    case POISON_TYPE_SCORPION_STRONG:
    case POISON_TYPE_SNAKE_WEAK:
    case POISON_TYPE_SNAKE_NORMAL:
    case POISON_TYPE_SNAKE_STRONG:
    case POISON_TYPE_SPIDER_WEAK:
    case POISON_TYPE_SPIDER_NORMAL:
    case POISON_TYPE_SPIDER_STRONG:
    case POISON_TYPE_CENTIPEDE_WEAK:
    case POISON_TYPE_CENTIPEDE_NORMAL:
    case POISON_TYPE_CENTIPEDE_STRONG:
    case POISON_TYPE_WASP_WEAK:
    case POISON_TYPE_WASP_NORMAL:
    case POISON_TYPE_WASP_STRONG:
    case POISON_TYPE_FUNGAL_WEAK:
    case POISON_TYPE_FUNGAL_NORMAL:
    case POISON_TYPE_FUNGAL_STRONG:
    case POISON_TYPE_DROW_WEAK:
    case POISON_TYPE_DROW_NORMAL:
    case POISON_TYPE_DROW_STRONG:
    case POISON_TYPE_WYVERN:
    case POISON_TYPE_PURPLE_WORM:
    case POISON_TYPE_COCKATRICE:
    case POISON_TYPE_KAPAK:
    case SPELL_POISON:
    case WEAPON_POISON_BLACK_ADDER_VENOM:
    case SPELL_POISON_BREATHE:
      return true;
  }
  return false;
}

bool is_spell_restoreable(int spell)
{
  switch (spell)
  {
  case SPELL_BLINDNESS:
  case SPELL_POISON:
  case SPELL_DEAFNESS:
  case SPELL_SLOW:
  case SPELL_ENLARGE_PERSON:
  case SPELL_SHRINK_PERSON:
  case SPELL_POWER_WORD_BLIND:
  case SPELL_POWER_WORD_STUN:
  case SPELL_POWER_WORD_SILENCE:
  case SPELL_CONTAGION:
  case SPELL_BALEFUL_POLYMORPH:
  case SPELL_CONFUSION:
  case SPELL_SILENCE:
  case SPELL_POISON_BREATHE:
  case SPELL_FEAR:
  case AFFECT_LEVEL_DRAIN:
    /* case SKILL_AFFECT_WILDSHAPE:
    case SKILL_AFFECT_RAGE:
    case SPELL_AFFECT_PRAYER_DEBUFF: */
    return true;
  }
  if (is_poison_spell(spell))
    return true;
  return false;
}

bool is_spell_or_spell_like(int type)
{
  if (type > SPELL_RESERVED_DBC && type < NUM_SPELLS)
    return true;
  if (type > PSIONIC_POWER_START && type < PSIONIC_POWER_END)
    return true;

  switch (type)
  {
  case SPELL_AFFECT_MIND_TRAP_NAUSEA:
  case PSIONIC_ABILITY_PSIONIC_FOCUS:
  case PSIONIC_ABILITY_DOUBLE_MANIFESTATION:
  case SPELL_DRAGONBORN_ANCESTRY_BREATH:
  case PALADIN_MERCY_INJURED_FAST_HEALING:
  case BLACKGUARD_TOUCH_OF_CORRUPTION:
  case BLACKGUARD_CRUELTY_AFFECTS:
  case ABILITY_CHANNEL_POSITIVE_ENERGY:
  case ABILITY_CHANNEL_NEGATIVE_ENERGY:
  case SPELL_AFFECT_STUNNING_BARRIER:
  case AFFECT_ENTANGLING_FLAMES:
  case SPELL_EFFECT_DAZZLED:
  case SPELL_AFFECT_DEATH_ATTACK:
  case RACIAL_LICH_TOUCH:
  case RACIAL_LICH_FEAR:
  case RACIAL_LICH_REJUV:
  case ABILITY_AFFECT_TRUE_JUDGEMENT:
  case SPELL_AFFECT_WEAPON_OF_AWE:
  case RACIAL_ABILITY_SKELETON_DR:
  case RACIAL_ABILITY_ZOMBIE_DR:
  case RACIAL_ABILITY_PRIMORDIAL_DR:
  case ABILITY_SCORE_DAMAGE:
  case VAMPIRE_ABILITY_CHILDREN_OF_THE_NIGHT:
  case AFFECT_LEVEL_DRAIN:
  case ABILITY_CREATE_VAMPIRE_SPAWN:
  case ABILITY_BLOOD_DRAIN:
    return true;
  }

  return false;
}

bool can_dam_be_resisted(int type)
{
  switch (type)
  {
  case DAM_SUNLIGHT:
  case DAM_MOVING_WATER:
  case DAM_BLOOD_DRAIN:
  case DAM_BLEEDING:
    return false;
  }

  return true;
}

void set_vampire_spawn_feats(struct char_data *mob)
{
  if (!mob || !IS_NPC(mob))
    return;

  SET_FEAT(mob, FEAT_ALERTNESS, 1);
  SET_FEAT(mob, FEAT_COMBAT_REFLEXES, 1);
  SET_FEAT(mob, FEAT_DODGE, 1);
  SET_FEAT(mob, FEAT_IMPROVED_INITIATIVE, 1);
  SET_FEAT(mob, FEAT_LIGHTNING_REFLEXES, 1);
  SET_FEAT(mob, FEAT_TOUGHNESS, 1);
  SET_FEAT(mob, FEAT_VAMPIRE_NATURAL_ARMOR, 1);
  SET_FEAT(mob, FEAT_VAMPIRE_DAMAGE_REDUCTION, 1);
  SET_FEAT(mob, FEAT_VAMPIRE_ENERGY_RESISTANCE, 1);
  SET_FEAT(mob, FEAT_VAMPIRE_FAST_HEALING, 1);
  SET_FEAT(mob, FEAT_VAMPIRE_WEAKNESSES, 1);
  SET_FEAT(mob, FEAT_VAMPIRE_BLOOD_DRAIN, 1);
  SET_FEAT(mob, FEAT_VAMPIRE_ENERGY_DRAIN, 1);
  SET_FEAT(mob, FEAT_VAMPIRE_CHANGE_SHAPE, 1);
  SET_FEAT(mob, FEAT_VAMPIRE_GASEOUS_FORM, 1);
  SET_FEAT(mob, FEAT_VAMPIRE_SPIDER_CLIMB, 1);
}

// returns true if the character is wearing a non-container
// object on his about worn slot.  Also, if the person is
// grappling or affected by a wind_wall affect, it will
// return false.
bool is_covered(struct char_data *ch)
{
  if (!ch)
    return false;

  // wind will blow off any covering they have.
  if (AFF_FLAGGED(ch, AFF_WIND_WALL))
    return false;

  // if grappling, they cannot keep their cloak covering them
  if (AFF_FLAGGED(ch, AFF_GRAPPLED))
    return false;

  // They must be wearing a vampire cloak on the about location
  // to resist sunlight affects
  struct obj_data *cloak = GET_EQ(ch, WEAR_ABOUT);

  if (!cloak || GET_OBJ_VNUM(cloak) != VAMPIRE_CLOAK_OBJ_VNUM)
    return false;

  return true;
}

char *apply_types_lowercase(int apply_type)
{
  char apply_text[100];
  int i = 0;

  if (apply_type >= NUM_APPLIES || apply_type < 0)
    return NULL;

  snprintf(apply_text, sizeof(apply_text), "%s", apply_types[apply_type]);

  for (i = 0; i < strlen(apply_text); i++)
  {
    apply_text[i] = tolower(apply_text[i]);
    if (apply_text[i] == '-')
      apply_text[i] = ' ';
  }

  return strdup(apply_text);
}

bool valid_vampire_cloak_apply(int type)
{
  switch (type)
  {
  case APPLY_STR:
  case APPLY_DEX:
  case APPLY_CON:
  case APPLY_INT:
  case APPLY_WIS:
  case APPLY_CHA:
  case APPLY_SAVING_REFL:
  case APPLY_SAVING_FORT:
  case APPLY_SAVING_WILL:
  case APPLY_AC_NEW:
  case APPLY_DAMROLL:
  case APPLY_DR:
  case APPLY_HIT:
  case APPLY_HITROLL:
  case APPLY_HP_REGEN:
  case APPLY_MOVE:
  case APPLY_MV_REGEN:
  case APPLY_PSP:
  case APPLY_PSP_REGEN:
  case APPLY_RES_FIRE:
  case APPLY_RES_COLD:
  case APPLY_RES_AIR:
  case APPLY_RES_EARTH:
  case APPLY_RES_ACID:
  case APPLY_RES_HOLY:
  case APPLY_RES_ELECTRIC:
  case APPLY_RES_UNHOLY:
  case APPLY_RES_SLICE:
  case APPLY_RES_PUNCTURE:
  case APPLY_RES_FORCE:
  case APPLY_RES_SOUND:
  case APPLY_RES_POISON:
  case APPLY_RES_DISEASE:
  case APPLY_RES_NEGATIVE:
  case APPLY_RES_ILLUSION:
  case APPLY_RES_MENTAL:
  case APPLY_RES_LIGHT:
  case APPLY_RES_ENERGY:
  case APPLY_RES_WATER:
    return true;
  }
  return false;
}

int get_vampire_cloak_bonus(int level, int type)
{

  int amount = (level / 15) + 1;

  switch (type)
  {
  case APPLY_STR:
  case APPLY_DEX:
  case APPLY_CON:
  case APPLY_INT:
  case APPLY_WIS:
  case APPLY_CHA:
  case APPLY_SAVING_REFL:
  case APPLY_SAVING_FORT:
  case APPLY_SAVING_WILL:
    return amount * 2;

  case APPLY_AC_NEW:
  case APPLY_DR:
  case APPLY_HP_REGEN:
  case APPLY_MV_REGEN:
  case APPLY_PSP_REGEN:
    return amount;

  case APPLY_DAMROLL:
  case APPLY_HITROLL:
    return amount + 1;

  case APPLY_HIT:
  case APPLY_MOVE:
  case APPLY_PSP:
    return amount * 20;

  case APPLY_RES_FIRE:
  case APPLY_RES_COLD:
  case APPLY_RES_AIR:
  case APPLY_RES_EARTH:
  case APPLY_RES_ACID:
  case APPLY_RES_HOLY:
  case APPLY_RES_ELECTRIC:
  case APPLY_RES_UNHOLY:
  case APPLY_RES_SLICE:
  case APPLY_RES_PUNCTURE:
  case APPLY_RES_FORCE:
  case APPLY_RES_SOUND:
  case APPLY_RES_POISON:
  case APPLY_RES_DISEASE:
  case APPLY_RES_NEGATIVE:
  case APPLY_RES_ILLUSION:
  case APPLY_RES_MENTAL:
  case APPLY_RES_LIGHT:
  case APPLY_RES_ENERGY:
  case APPLY_RES_WATER:
    return amount * 10;
  }
  return 0;
}

void clear_misc_cooldowns(struct char_data *ch)
{
  if (!ch || IS_NPC(ch))
    return;

  GET_SETCLOAK_TIMER(ch) = 0;
  PIXIE_DUST_TIMER(ch) = 0;
  EFREETI_MAGIC_TIMER(ch) = 0;
  DRAGON_MAGIC_TIMER(ch) = 0;
  LAUGHING_TOUCH_TIMER(ch) = 0;
  FLEETING_GLANCE_TIMER(ch) = 0;
  FEY_SHADOW_WALK_TIMER(ch) = 0;
  GRAVE_TOUCH_TIMER(ch) = 0;
  GRASP_OF_THE_DEAD_TIMER(ch) = 0;
  INCORPOREAL_FORM_TIMER(ch) = 0;
  GET_MISSION_COOLDOWN(ch) = 0;
  GET_FORAGE_COOLDOWN(ch) = 0;
  GET_SCROUNGE_COOLDOWN(ch) = 0;
}

bool can_mastermind_power(struct char_data *ch, int spellnum)
{
  if (!ch)
    return false;

  if (spellnum < PSIONIC_POWER_START || spellnum > PSIONIC_POWER_END)
    return false;

  // We want to exclude some spell groups.
  if ((IS_SET(spell_info[spellnum].routines, MAG_ALTER_OBJS)) ||
      (IS_SET(spell_info[spellnum].routines, MAG_GROUPS)) ||
      (IS_SET(spell_info[spellnum].routines, MAG_MASSES)) ||
      (IS_SET(spell_info[spellnum].routines, MAG_AREAS)) ||
      (IS_SET(spell_info[spellnum].routines, MAG_SUMMONS)) ||
      (IS_SET(spell_info[spellnum].routines, MAG_CREATIONS)) ||
      (IS_SET(spell_info[spellnum].routines, MAG_ROOM)))
    return false;

  // // Self only spells we will also skip
  // if (IS_SET(spell_info[spellnum].targets, TAR_SELF_ONLY))
  //   return false;

  // There are some powers we may want to exclude
  // empty right now -- remove this comment if we add any --
  switch (spellnum)
  {
  default:
    break;
  }

  if (!affected_by_spell(ch, PSIONIC_ABILITY_MASTERMIND))
    return false;

  return true;
}

bool has_epic_power(struct char_data *ch, int powernum)
{
  if (powernum < PSIONIC_POWER_START || powernum > PSIONIC_POWER_END)
    return false;

  if (!psionic_powers[powernum].is_epic)
    return false;

  switch (powernum)
  {
  case PSIONIC_IMPALE_MIND:
    if (HAS_FEAT(ch, FEAT_PSI_POWER_IMPALE_MIND))
      return true;
    else
      return false;
  case PSIONIC_RAZOR_STORM:
    if (HAS_FEAT(ch, FEAT_PSI_POWER_RAZOR_STORM))
      return true;
    else
      return false;
  case PSIONIC_PSYCHOKINETIC_THRASHING:
    if (HAS_FEAT(ch, FEAT_PSI_POWER_PSYCHOKINETIC_THRASHING))
      return true;
    else
      return false;
  case PSIONIC_EPIC_PSIONIC_WARD:
    if (HAS_FEAT(ch, FEAT_PSI_POWER_EPIC_PSIONIC_WARD))
      return true;
    else
      return false;
  }

  return false;
}

void manifest_mastermind_power(struct char_data *ch)
{
  struct char_data *tch = NULL;
  char buf[200];

  if (IN_ROOM(ch) == NOWHERE)
    return;

  if (can_mastermind_power(ch, CASTING_SPELLNUM(ch)))
  {
    for (tch = world[IN_ROOM(ch)].people; tch; tch = tch->next_in_room)
    {
      if (spell_info[CASTING_SPELLNUM(ch)].violent)
      {
        if (!aoeOK(ch, tch, CASTING_SPELLNUM(ch)))
        {
          continue;
        }
        if (ch == tch)
        {
          snprintf(buf, sizeof(buf), "Your mastermind ability manifests '%s' on You.", spell_info[CASTING_SPELLNUM(ch)].name);
          act(buf, FALSE, ch, 0, tch, TO_CHAR);
          snprintf(buf, sizeof(buf), "$n's mastermind ability manifests '%s' on $N.", spell_info[CASTING_SPELLNUM(ch)].name);
          act(buf, FALSE, ch, 0, tch, TO_ROOM);
        }
        else
        {
          snprintf(buf, sizeof(buf), "Your mastermind ability manifests '%s' on $N.", spell_info[CASTING_SPELLNUM(ch)].name);
          act(buf, FALSE, ch, 0, tch, TO_CHAR);
          snprintf(buf, sizeof(buf), "$n's mastermind ability manifests '%s' on You.", spell_info[CASTING_SPELLNUM(ch)].name);
          act(buf, FALSE, ch, 0, tch, TO_VICT);
          snprintf(buf, sizeof(buf), "$n's mastermind ability manifests '%s' on $N.", spell_info[CASTING_SPELLNUM(ch)].name);
          act(buf, FALSE, ch, 0, tch, TO_NOTVICT);
        }
        call_magic(ch, tch, 0, CASTING_SPELLNUM(ch), 0, GET_PSIONIC_LEVEL(ch), CAST_SPELL);
      }
      else
      {
        if (aoeOK(ch, tch, CASTING_SPELLNUM(ch)))
          continue;

        call_magic(ch, tch, 0, CASTING_SPELLNUM(ch), 0, GET_PSIONIC_LEVEL(ch), CAST_SPELL);
      }
    }
  }
}

bool can_blood_drain_target(struct char_data *ch, struct char_data *vict)
{
  if (!IS_LIVING(vict))
  {
    send_to_char(ch, "This can only be used on the living.\r\n");
    return false;
  }

  if (IS_OOZE(vict))
  {
    send_to_char(ch, "This cannot be used on oozes.\r\n");
    return false;
  }

  if (IS_ELEMENTAL(vict))
  {
    send_to_char(ch, "This cannot be used on oozes.\r\n");
    return false;
  }

  if (IS_GOOD(ch))
  {
    if (!IS_EVIL(vict) && IS_SENTIENT(vict))
    {
      send_to_char(ch, "Good aligned vampires can only feed on evil creatures or non-sentient creatures.\r\n");
      return false;
    }
  }
  
  return true;
}

int vampire_last_feeding_adjustment(struct char_data *ch)
{
  if (IS_VAMPIRE(ch))
  {
    if (TIME_SINCE_LAST_FEEDING(ch) <= 50)
    {
      return 2;
    }
    else if (TIME_SINCE_LAST_FEEDING(ch) >= 150)
    {
      return -2;
    }
  }
  return 0;
}

bool is_immaterial(struct char_data *ch)
{
  if (!ch)
    return false;

  if (affected_by_spell(ch, SPELL_GASEOUS_FORM))
    return true;

  if (AFF_FLAGGED(ch, AFF_IMMATERIAL))
    return true;

  return false;
}

// returns true if the spell should NOT be listed in spell lists
bool do_not_list_spell(int spellnum)
{
  switch (spellnum)
  {
  case SPELL_LUSKAN_RECALL:
  case SPELL_TRIBOAR_RECALL:
  case SPELL_MIRABAR_RECALL:
  case SPELL_SILVERYMOON_RECALL:
  case SPELL_PALANTHAS_RECALL:
  case SPELL_SOLACE_RECALL:
  case SPELL_SANCTION_RECALL:
    return true;
  }
  return false;
}

// returns 0 if neither spell or psionic power
// returns 1 if psionic power
// returns 2 if spell
// returns 3 if warlock power
int is_spell_or_power(int spellnum)
{
  if (spellnum <= 0 || spellnum >= NUM_SPELLS)
  {
    if (spellnum >= PSIONIC_POWER_START && spellnum <= PSIONIC_POWER_END)
    {
      return 1;
    }
    else if (spellnum >= WARLOCK_POWER_START && spellnum <= WARLOCK_POWER_END)
    {
      return 3;
    }
    else
    {
      return 0;
    }
  }
  return 2;
}

int get_number_of_spellcasting_classes(struct char_data *ch)
{
  int i = 0;
  int num_classes = 0;

  for (i = 0; i < NUM_CLASSES; i++)
    if (CLASS_LEVEL(ch, i) > 0)
      if (IS_SPELLCASTER_CLASS(i))
        num_classes++;

  return num_classes;
}

int get_first_spellcasting_classes(struct char_data *ch)
{
  int i = 0;

  for (i = 0; i < NUM_CLASSES; i++)
    if (CLASS_LEVEL(ch, i) > 0)
      if (IS_SPELLCASTER_CLASS(i))
        return i;

  return i;
}

int warlock_spell_type(int spellnum)
{

  switch (spellnum)
  {
    case WARLOCK_ELDRITCH_SPEAR:
    case WARLOCK_HIDEOUS_BLOW:
    case WARLOCK_ELDRITCH_CHAIN:
    case WARLOCK_ELDRITCH_CONE:
    case WARLOCK_ELDRITCH_DOOM:
      return WARLOCK_POWER_SHAPE;
    
    case WARLOCK_DRAINING_BLAST:
    case WARLOCK_FRIGHTFUL_BLAST:
    case WARLOCK_BESHADOWED_BLAST:
    case WARLOCK_BRIMSTONE_BLAST:
    case WARLOCK_HELLRIME_BLAST:
    case WARLOCK_BEWITCHING_BLAST:
    case WARLOCK_NOXIOUS_BLAST:
    case WARLOCK_VITRIOLIC_BLAST:
    case WARLOCK_BINDING_BLAST:
    case WARLOCK_UTTERDARK_BLAST:
      return WARLOCK_POWER_ESSENCE;
    
    case WARLOCK_BEGUILING_INFLUENCE:
    case WARLOCK_DARK_ONES_OWN_LUCK:
    case WARLOCK_DARKNESS:
    case WARLOCK_DEVILS_SIGHT:
    case WARLOCK_ENTROPIC_WARDING:
    case WARLOCK_LEAPS_AND_BOUNDS:
    case WARLOCK_OTHERWORLDLY_WHISPERS:
    case WARLOCK_SEE_THE_UNSEEN:
    case WARLOCK_CHARM:
    case WARLOCK_CURSE_OF_DESPAIR:
    case WARLOCK_DREAD_SEIZURE:
    case WARLOCK_FLEE_THE_SCENE:
    case WARLOCK_THE_DEAD_WALK:
    case WARLOCK_VORACIOUS_DISPELLING:
    case WARLOCK_WALK_UNSEEN:
    case WARLOCK_CHILLING_TENTACLES:
    case WARLOCK_DEVOUR_MAGIC:
    case WARLOCK_TENACIOUS_PLAGUE:
    case WARLOCK_WALL_OF_PERILOUS_FLAME:
    case WARLOCK_DARK_FORESIGHT:
    case WARLOCK_RETRIBUTIVE_INVISIBILITY:
      return WARLOCK_POWER_SPELL;    
    
    // not implemented yet
    case WARLOCK_WORD_OF_CHANGING:
    default:
      return WARLOCK_POWER_NONE;
  }
  return WARLOCK_POWER_NONE;
}

void set_x_y_coords(int start, int *x, int *y, int *room)
{
  *x = MIN(159, MAX(0, ((start - 600161) % 160)));
  *y = MIN(159, MAX(0, ((start - 600161) / 160)));

  *room = 600161 + (160 * *y) + *x;
}

// This function will return true if the character can be affected by the
// daze affect, or false if it cannot.
bool can_daze(struct char_data *ch)
{
  if (!ch)
    return false;

  if (GET_NODAZE_COOLDOWN(ch) > 0)
    return false;

  if (IS_NPC(ch) && MOB_FLAGGED(ch, MOB_NOPARALYZE))
    return false;

  if (AFF_FLAGGED(ch, AFF_FREE_MOVEMENT))
    return false;

  return true;
}


// This function simply checks to see if there's a random treasure chest in the room or not.
bool is_random_chest_in_room(room_rnum rrnum)
{
  if (rrnum == NOWHERE)
    return false;

  struct obj_data *obj = NULL;
  bool found = false;

  for (obj = world[rrnum].contents; obj; obj = obj->next_content)
  {
    if (GET_OBJ_TYPE(obj) == ITEM_TREASURE_CHEST && GET_OBJ_VAL(obj, 2) == 1)
    {
      found = true;
      break;
    }
  }

  return found; 
}

int number_of_chests_per_zone(int num_zone_rooms)
{
  return (num_zone_rooms / NUM_OF_ZONE_ROOMS_PER_RANDOM_CHEST);
}


// This function checks to see if a random treasure chest can be placed in the room.
// It is based on different factors, such as whether the room or zone is flagged to
// allow random chests, how many chests are already in the zone, and so forth.
bool can_place_random_chest_in_room(room_rnum rrnum, int num_zone_rooms, int num_chests)
{

  if (rrnum == NOWHERE)
    return false;

  zone_rnum zone = GET_ROOM_ZONE(rrnum);

  if (num_zone_rooms <= 0)
    return false;

  // rooms flagged can only load a chest if there isn't any there already
  if (ROOM_FLAGGED(rrnum, ROOM_RANDOM_CHEST))
  {
    if (is_random_chest_in_room(rrnum))
      return false;
    return true;
  }

  if (ZONE_FLAGGED(zone, ZONE_RANDOM_CHESTS))
  {
    if (number_of_chests_per_zone(num_zone_rooms) < num_chests)
      return false;
    return true;
  }

  //mudlog(NRM, 34, FALSE, "ZReset B: %d %s %d %s", zone_table[zone].number, zone_table[zone].name, world[rrnum].number, world[rrnum].name);

  return false;
}

// This function will return a 'grade' of gear based on the level entered.
// For example, a return value of 1 is a mundane item, which is masterwork
// or normal.  A 2 is a minor magic item, generally +1. A 6 is a superior
// item generally level 26 or higher. These grades are mainly used for treasure
// chest objects.
int get_random_chest_item_level(int level)
{
  if (level <= 5)
  {
    switch (dice(1, 5))
    {
      case 1: return 1;
      case 5: return 3;
      default: return 2;
    }
  }
  else if (level <= 10)
  {
    switch (dice(1, 10))
    {
      case 1: return 1;
      case 7: case 8: case 9: case 10: return 3;
      default: return 2;
    }
  }
  else if (level <= 15)
  {
    switch (dice(1, 10))
    {
      case 1: return 2;
      case 7: case 8: case 9: case 10: return 4;
      default: return 3;
    }
  }
  else if (level <= 20)
  {
    switch (dice(1, 10))
    {
      case 1: return 3;
      case 7: case 8: case 9: case 10: return 5;
      default: return 4;
    }
  }
  else if (level <= 25)
  {
    switch (dice(1, 10))
    {
      case 1: return 4;
      case 7: case 8: case 9: case 10: return 6;
      default: return 5;
    }
  }
  else
  {
    switch (dice(1, 10))
    {
      case 7: case 8: case 9: case 10: return 5;
      default: return 6;
    }
  }

  return 2;
}

// This will return a random kind of chest type. Types 2 through 7 are weapons, armor, consumables, trinkets, gold and crystals.
// anything else is 1, which is generic and has a chance to drop anything based on the normal coded treasure tables.
int get_chest_contents_type(void)
{
  int type = dice(1, 15);

  if (type > 1 && type <= 7)
    return type;

  return 1;
}

// This function will return a chest dc based on the level provided
// There is a 75% chance to return a 0, meaning the queried affect
// is inactive on this chest.  Example: Used to get a pick lock dc
// If the chest level is 10, there's a 75% chance to return 0 which
// means the chest is not locked, otherwise it will return a dc of 15.
int get_random_chest_dc(int level)
{

  if (level <= 0)
    return 0;

  // 75 % chance it won't be hidden at all
  if (dice(1, 4) > 1)
    return 0;

  if (level <= 5)
    return 10;
  else if (level <= 10)
    return 15;
  else if (level <= 15)
    return 20;
  else if (level <= 20)
    return 25;
  else if (level <= 25)
    return 30;
  else if (level <= 30)
    return 35;

  return 40;
}

// This function will place a random treasure chest in the room
// level will determine level of items dropped
// search dc will be the desired search dc, 0 if you don't want it hidden or -1 if you want a random chance for a search dc based on chest level
// pick dc will be the desired pick lock dc, 0 if you don't want it locked or -1 if you want a random chance for a pick dc based on chest level
// trap chance is the percentage chance that the chest will be trapped. If it is the trap type will always be triggered by open container with a trap type based on chest level
void place_random_chest(room_rnum rrnum, int level, int search_dc, int pick_dc, int trap_chance)
{
  if (rrnum == NOWHERE)
    return;

  struct obj_data *obj = read_object(RANDOM_TREASURE_CHEST_VNUM, VIRTUAL);

  if (obj == NULL)
    return;

  // determines the level of items dropped
  GET_OBJ_VAL(obj, 0) = get_random_chest_item_level(level);
  // determines the type of object dropped
  GET_OBJ_VAL(obj, 1) = get_chest_contents_type();
  // we set this to 1 to tag it as a random treasure chest, meaning it can only be looted once
  // with no cooldown and is extracted immediately after being looted.
  GET_OBJ_VAL(obj, 2) = 1;
  // This is the search dc.  If above zero it will cannot be interacted with unless located with search command
  GET_OBJ_VAL(obj, 3) = (search_dc == -1) ? get_random_chest_dc(level) : search_dc;
  // This is the pick lock dc.  If above zero it will cannot be looted unless unlocked with the picklock command.
  GET_OBJ_VAL(obj, 4) = (pick_dc == -1) ? get_random_chest_dc(level) : pick_dc;
  // We're going to see if there should be an associated trap
  // This is currently not implemented
  if (dice(1, 100) <= trap_chance)
    GET_OBJ_VAL(obj, 5) = 0;

  obj_to_room(obj, rrnum);
}

bool has_reach(struct char_data *ch)
{
  if (!ch) return false;

  struct obj_data *wielded = GET_EQ(ch, WEAR_WIELD_1);
  if (!wielded)
    wielded = GET_EQ(ch, WEAR_WIELD_2H);

  if (wielded)
    if (IS_SET(weapon_list[GET_OBJ_VAL(wielded, 0)].weaponFlags, WEAPON_FLAG_REACH))
      return true;

  if (HAS_EVOLUTION(ch, EVOLUTION_REACH))
    return true;

  return false;
}

// This will return the desired mob follower or NULL if not found.
// mob_type refers to the mob flag normally associated with the call
// command.
struct char_data * get_mob_follower(struct char_data *ch, int mob_type)
{

  struct follow_type *k = NULL, *next = NULL;

  for (k = ch->followers; k; k = next)
  {
    next = k->next;
    if (IS_NPC(k->follower) && AFF_FLAGGED(k->follower, AFF_CHARM) && MOB_FLAGGED(k->follower, mob_type))
    {
      return k->follower;
    }
  }

  return NULL;
}

void AoEDamageRoom(struct char_data *ch, int dam, int spellnum, int dam_type)
{
  if (!ch) return;

  struct char_data *victim = NULL;

  for (victim = world[IN_ROOM(ch)].people; victim; victim = victim->next_in_room)
  {
    if (aoeOK(ch, victim, spellnum))
    {
      damage(ch, victim, dam, spellnum, dam_type, FALSE);
    }
  }
}

bool can_act(struct char_data *ch)
{
  if (!ch)
    return false;

  if (AFF_FLAGGED(ch, AFF_PINNED))
    return false;

  if (AFF_FLAGGED(ch, AFF_STUN))
    return false;
  
  if (AFF_FLAGGED(ch, AFF_PARALYZED))
    return false;

  if (AFF_FLAGGED(ch, AFF_DAZED))
    return false;

  if (!AWAKE(ch))
    return false;

  return true;
}

bool show_combat_roll(struct char_data *ch)
{
  if (!IS_NPC(ch) && PRF_FLAGGED(ch, PRF_COMBATROLL))
    return true;
  if (IS_NPC(ch) && AFF_FLAGGED(ch, AFF_CHARM) && ch->master && !IS_NPC(ch->master) && PRF_FLAGGED(ch->master, PRF_CHARMIE_COMBATROLL))
    return true;
  return false;
}

void send_combat_roll_info(struct char_data *ch, const char *messg, ...)
{
  if (!ch) return;

  if (!IS_NPC(ch) && PRF_FLAGGED(ch, PRF_COMBATROLL))
  {
    if (ch->desc && messg && *messg)
    {
      size_t left;
      va_list args;

      va_start(args, messg);
      left = vwrite_to_output(ch->desc, messg, args);
      va_end(args);
    }
  }
  if (IS_NPC(ch) && AFF_FLAGGED(ch, AFF_CHARM) && ch->master && !IS_NPC(ch->master) && PRF_FLAGGED(ch->master, PRF_CHARMIE_COMBATROLL))
  {
    if (ch->master->desc && messg && *messg)
    {
      size_t left;
      va_list args;

      va_start(args, messg);
      left = vwrite_to_output(ch->master->desc, messg, args);
      va_end(args);
    }
  } 

}

int can_carry_weight_limit(struct char_data *ch)
{
  if (!ch) return 0;

  int strength = GET_CARRY_STRENGTH(ch);
  int limit = 0;

  if (strength <= 1)
    limit = 5;
  else
  {
    switch (strength)
    {
      case 2: limit = 13; break;
      case 3: limit = 20; break;
      case 4: limit = 26; break;
      case 5: limit = 33; break;
      case 6: limit = 40; break;
      case 7: limit = 46; break;
      case 8: limit = 53; break;
      case 9: limit = 60; break;
      case 10: limit = 66; break;
      case 11: limit = 76; break;
      case 12: limit = 86; break;
      case 13: limit = 100; break;
      case 14: limit = 116; break;
      case 15: limit = 133; break;
      case 16: limit = 153; break;
      case 17: limit = 173; break;
      case 18: limit = 200; break;
      case 19: limit = 233; break;
      case 20: limit = 266; break;
      case 21: limit = 306; break;
      case 22: limit = 346; break;
      case 23: limit = 400; break;
      case 24: limit = 466; break;
      case 25: limit = 533; break;
      case 26: limit = 613; break;
      case 27: limit = 693; break;
      case 28: limit = 800; break;
      case 29: limit = 933; break;
      // 30+
      default:
        limit = 1000 + MAX(0, (strength - 30)) * 200;
        break;
    }
  }

  // the above tables use the pathfinder value for a medium load.
  // level 30+ we've customized so things don't get too insane.
  // here we'll multiply by 1.5 to get their max load.
  // if at some point in the future we want to fully implement
  // the encumbrance system with light/medium/heavy loads we can
  // add a mode variable to the function and multiply the above values
  // accordingly. x0.5 for light x1.5 for heavy. For now all limits
  // are heavy, so x1.5
  limit = (int) (limit * 1.5);

  switch (GET_SIZE(ch))
  {
    case SIZE_FINE: limit = (int) (limit * 0.125); break;
    case SIZE_DIMINUTIVE: limit = (int) (limit * 0.25); break;
    case SIZE_TINY: limit = (int) (limit * 0.5); break;
    case SIZE_SMALL: limit = (int) (limit * 0.75); break;
    case SIZE_LARGE: limit = (int) (limit * 2); break;
    case SIZE_HUGE: limit = (int) (limit * 4); break;
    case SIZE_GARGANTUAN: limit = (int) (limit * 8); break;
    case SIZE_COLOSSAL: limit = (int) (limit * 16); break;
  }

  limit = MIN(25000, limit);

  return MAX(1, limit);

}

bool is_valid_ability_number(int num)
{
  switch (num)
  {
    case ABILITY_ACROBATICS:
    case ABILITY_STEALTH:
    case ABILITY_PERCEPTION:
    case ABILITY_MEDICINE:
    case ABILITY_INTIMIDATE:
    case ABILITY_CONCENTRATION:
    case ABILITY_SPELLCRAFT:
    case ABILITY_APPRAISE:
    case ABILITY_DISCIPLINE:
    case ABILITY_TOTAL_DEFENSE:
    case ABILITY_ARCANA:
    case ABILITY_RIDE:
    case ABILITY_SLEIGHT_OF_HAND:
    case ABILITY_DECEPTION:
    case ABILITY_PERSUASION:
    case ABILITY_DISABLE_DEVICE:
    case ABILITY_DISGUISE:
    case ABILITY_HANDLE_ANIMAL:
    case ABILITY_INSIGHT:
    case ABILITY_SURVIVAL:
    case ABILITY_USE_MAGIC_DEVICE:
    case ABILITY_LINGUISTICS:
    case ABILITY_PERFORM:
    case ABILITY_HISTORY:
    case ABILITY_RELIGION:
      return true;
  }
  return false;
}

struct obj_data *get_char_bag(struct char_data *ch, int bagnum)
{
  if (bagnum < 1 || bagnum > MAX_BAGS)
    return NULL;

  switch (bagnum)
  {
  case 1:
    return ch->bags->bag1;
  case 2:
    return ch->bags->bag2;
  case 3:
    return ch->bags->bag3;
  case 4:
    return ch->bags->bag4;
  case 5:
    return ch->bags->bag5;
  case 6:
    return ch->bags->bag6;
  case 7:
    return ch->bags->bag7;
  case 8:
    return ch->bags->bag8;
  case 9:
    return ch->bags->bag9;
  case 10:
    return ch->bags->bag10;
  }

  log("Error in get_char_bag returning NULL");

  return NULL;
}

bool is_spellcasting_class(int class_name)
{
  switch (class_name)
  {
    case CLASS_WIZARD:
    case CLASS_CLERIC:
    case CLASS_DRUID:
    case CLASS_SORCERER:
    case CLASS_PALADIN:
    case CLASS_RANGER:
    case CLASS_BARD:
    case CLASS_ALCHEMIST:
    case CLASS_BLACKGUARD:
    case CLASS_INQUISITOR:
    case CLASS_SUMMONER:
      return true;
  }
  return false;
}

int countlines(char *filename)
{
  // count the number of lines in the file called filename
  FILE *fp = fopen(filename, "r");
  int ch = 0;
  int lines = 0;

  if (fp == NULL)
      ;
  return 0;

  lines++;
  while ((ch = fgetc(fp)) != EOF)
  {
      if (ch == '\n')
        lines++;
  }
  fclose(fp);
  return lines;
}

bool is_crafting_kit(struct obj_data *kit)
{
  if (!kit) return false;
  if (GET_OBJ_TYPE(kit) != ITEM_CONTAINER) return false;
  if (!strncmp(kit->name, "corpse ", 7)) return false;
  obj_rnum rnum = GET_OBJ_RNUM(kit);
  if (rnum != NOTHING && obj_index[rnum].func != crafting_kit) return false;
  return true;
}

int get_apply_type_gear_mod(struct char_data *ch, int apply)
{
  int i = 0, j = 0, full_bonus = 0;
  int bonuses[NUM_BONUS_TYPES];
  struct obj_data *obj;

  for (i = 0; i < NUM_BONUS_TYPES; i++)
    bonuses[i] = 0;

  for (i = 0; i < NUM_WEARS; i++)
  {
    if ((obj = GET_EQ(ch, i)))
    {
      for (j = 0; j < 6; j++)
      {
        if (obj->affected[j].location == apply)
        {
          if (BONUS_TYPE_STACKS(obj->affected[j].bonus_type))
          {
            bonuses[obj->affected[j].bonus_type] += obj->affected[j].modifier;
          }
          else if (obj->affected[j].modifier > bonuses[obj->affected[j].bonus_type])
          {
            bonuses[obj->affected[j].bonus_type] = obj->affected[j].modifier;
          }
        }
      }
    }
  }

  for (i = 0; i < NUM_BONUS_TYPES; i++)
    full_bonus += bonuses[i];

  return full_bonus;
}

int get_fast_healing_amount(struct char_data *ch)
{
  int hp = 0;

  if (affected_by_spell(ch, SPELL_GREATER_PLANAR_HEALING))
    hp += 4;
  else if (affected_by_spell(ch, SPELL_PLANAR_HEALING))
    hp += 1;

  if (affected_by_spell(ch, AFFECT_PLANAR_SOUL_SURGE))
    hp += 2;

  if (affected_by_spell(ch, EIDOLON_MERGE_FORMS_EFFECT))
    hp += get_char_affect_modifier(ch, EIDOLON_MERGE_FORMS_EFFECT, APPLY_FAST_HEALING);

  hp += get_char_affect_modifier(ch, AFFECT_FOOD, APPLY_FAST_HEALING);
  hp += get_char_affect_modifier(ch, AFFECT_DRINK, APPLY_FAST_HEALING);

  hp += HAS_EVOLUTION(ch, EVOLUTION_FAST_HEALING) * 2;

  hp += get_apply_type_gear_mod(ch, APPLY_FAST_HEALING);

  return hp;
}

int get_hp_regen_amount(struct char_data *ch)
{
  int hp = 0;

  hp += get_char_affect_modifier(ch, AFFECT_FOOD, APPLY_HP_REGEN);
  hp += get_char_affect_modifier(ch, AFFECT_DRINK, APPLY_HP_REGEN);
  hp += get_apply_type_gear_mod(ch, APPLY_HP_REGEN);

  return hp;
}

int get_psp_regen_amount(struct char_data *ch)
{
  int psp = 0;

  psp += get_char_affect_modifier(ch, AFFECT_FOOD, APPLY_PSP_REGEN);
  psp += get_char_affect_modifier(ch, AFFECT_DRINK, APPLY_PSP_REGEN);
  psp += get_apply_type_gear_mod(ch, APPLY_PSP_REGEN);
  psp += MAX(0, GET_INT_BONUS(ch));

  return psp;
}

int get_mv_regen_amount(struct char_data *ch)
{
  int mv = 0;

  mv += get_char_affect_modifier(ch, AFFECT_FOOD, APPLY_MV_REGEN);
  mv += get_char_affect_modifier(ch, AFFECT_DRINK, APPLY_MV_REGEN);
  mv += get_apply_type_gear_mod(ch, APPLY_MV_REGEN);

  return mv;
}

int  get_bonus_from_liquid_type(int liquid)
{
  switch (liquid)
  {
    case LIQ_BEER: return APPLY_DEX;
    case LIQ_WINE: return APPLY_INT;
    case LIQ_ALE: return APPLY_STR;
    case LIQ_DARKALE: return APPLY_CON;
    case LIQ_WHISKY: return APPLY_HITROLL;
    case LIQ_LEMONADE: return APPLY_MV_REGEN;
    case LIQ_FIREBRT: return APPLY_DAMROLL;
    case LIQ_LOCALSPC: return APPLY_PSP_REGEN;
    case LIQ_SLIME: return APPLY_NONE;
    case LIQ_MILK: return APPLY_CHA;
    case LIQ_TEA: return APPLY_WIS;
    case LIQ_COFFE: return APPLY_INT;
    case LIQ_BLOOD: return APPLY_NONE;
    case LIQ_SALTWATER: return APPLY_NONE;
    case LIQ_CLEARWATER: return APPLY_HIT;
    default: return APPLY_HP_REGEN;
  }
  return APPLY_HP_REGEN;
}

bool is_road_room(room_rnum room, int type)
{
  if (room == NOWHERE)
    return false;
    
  if (ZONE_FLAGGED(GET_ROOM_ZONE(room), ZONE_MISSIONS) && type == 1)
    return true;
  else if (ZONE_FLAGGED(GET_ROOM_ZONE(room), ZONE_HUNTS) && type == 2)
    return true;
  else if (ZONE_FLAGGED(GET_ROOM_ZONE(room), ZONE_RANDOM_ENCOUNTERS) && type == 3)
    return true;
  else if (world[room].sector_type == SECT_ROAD_EW)
    return true;
  else if (world[room].sector_type == SECT_ROAD_INT)
    return true;
  else if (world[room].sector_type == SECT_ROAD_NS)
    return true;
  return false;
}

// This function is the same as the dice() function except any rolls below min will be rerolled
int min_dice(int num, int size, int min)
{
  if (num <= 0 || size <= 0)
   return 0;
  
  if (min <= 0)
    return dice(num, size);

  int i = 0;
  int amount = 0;
  int temp = 0;

  for (i = 0; i < num; i++)
  {
    temp = dice(1, size);
    while (temp < min)
      temp = dice(1, size);
    amount += temp;
  }
  
  return amount;
}

bool is_wearing_metal(struct char_data *ch)
{

  struct obj_data *obj;
  int i;

  if (!ch) return false;

  for (i = 0; i < NUM_WEARS; i++)
  {
    obj = GET_EQ(ch, i);
    if (!obj) continue;
    if (IS_CONDUCTIVE_METAL(GET_OBJ_MATERIAL(obj)))
      return true;
  }
  return false;
}

bool can_npc_command(struct char_data *ch)
{

#if !defined(CAMPAIGN_DL)
  if (IS_NPC(ch))
  {
    send_to_char(ch, "You have no idea how.\r\n");
    return FALSE;
  }
#endif

  return TRUE;
}

bool is_riding_dragon_mount(struct char_data *ch)
{
  if (!ch) return false;

  if (IS_NPC(ch)) return false;

  if (!RIDING(ch)) return false;

  if (!is_dragon_rider_mount(RIDING(ch))) return false;

  if (IN_ROOM(ch) != IN_ROOM(RIDING(ch))) return false;

  return true;

}

bool is_dragon_rider_mount(struct char_data *ch)
{
  if (!ch) return false;

  if (!IS_NPC(ch)) return false;

  if (GET_MOB_VNUM(ch) >= 40401 && GET_MOB_VNUM(ch) <= 40410)
    return true;

  return false;

}

int get_encumbrance_mod(struct char_data *ch)
{
  int mod[NUM_BONUS_TYPES];
  int penalty[NUM_BONUS_TYPES];
  int i = 0, j = 0, final = 0;
  struct affected_type *aff = NULL;
  struct obj_data *obj = NULL;

  // initialize array
  for (i = 0; i < NUM_BONUS_TYPES; i++)
  {
    mod[i] = 0;
    penalty[i] = 0;
  }

  // get from affects
  for (aff = ch->affected; aff; aff = aff->next)
  {
    if (aff->location == APPLY_ENCUMBRANCE)
    {
      if (aff->modifier > 0)
        mod[aff->bonus_type] = MAX(mod[aff->bonus_type], aff->modifier);
      else
        penalty[aff->bonus_type] = MIN(penalty[aff->bonus_type], aff->modifier);
    }
  }

  // get from gear
  for (i = 0; i < NUM_WEARS; i++)
  {
    if ((obj = GET_EQ(ch, i)) != NULL)
    {
      for (j = 0; j < 6; j++)
      {
        if (obj->affected[j].location == APPLY_ENCUMBRANCE)
        {
          if (obj->affected[j].modifier > 0)
            mod[obj->affected[j].bonus_type] = MAX(mod[obj->affected[j].bonus_type], obj->affected[j].modifier);
          else
            penalty[obj->affected[j].bonus_type] = MIN(penalty[obj->affected[j].bonus_type], obj->affected[j].modifier);
        }
      }
    }
  }

  for (i = 0; i < NUM_BONUS_TYPES; i++)
  {
    final += mod[i] + penalty[i];
  }

  return final;

}

bool ok_call_mob_vnum(int mob_num)
{

  if (mob_num >= 1 && mob_num <= 99) return true;
  
  if (mob_num == 60289) return true;

  if (mob_num == MOB_NUM_EIDOLON) return true;

  if (mob_num >= 40400 && mob_num <= 40410) return true;

  if (mob_num >= 20803 && mob_num <= 20805) return true;

  return false;

}

bool is_selectable_region(int region)
{
  switch (region)
  {
    case REGION_OUTER_PLANES:
      return false;
  }
  return true;
}

bool is_in_hometown(struct char_data *ch)
{
  if (!ch || IN_ROOM(ch) == NOWHERE) return false;

  if (zone_table[world[IN_ROOM(ch)].zone].city == CITY_NONE) return false;

  if (GET_HOMETOWN(ch) == CITY_NONE) return false;

  if (zone_table[world[IN_ROOM(ch)].zone].city == GET_HOMETOWN(ch))
    return true;

  return false;
}

bool is_crafting_skill(int skillnum)
{
  return (spell_info[skillnum].schoolOfMagic == CRAFTING_SKILL);
}

int get_knowledge_skill_from_creature_type(int race_type)
{
  switch (race_type)
  {
    case RACE_TYPE_HUMANOID:
    case RACE_TYPE_ANIMAL:
    case RACE_TYPE_GIANT:
    case RACE_TYPE_FEY:
    case RACE_TYPE_MONSTROUS_HUMANOID:
    case RACE_TYPE_OOZE:
    case RACE_TYPE_PLANT:
    case RACE_TYPE_VERMIN:
      return ABILITY_NATURE;

    case RACE_TYPE_UNDEAD:
    case RACE_TYPE_OUTSIDER:
    case RACE_TYPE_LYCANTHROPE:
      return ABILITY_RELIGION;
    
    case RACE_TYPE_DRAGON:
    case RACE_TYPE_ABERRATION:
    case RACE_TYPE_CONSTRUCT:
    case RACE_TYPE_ELEMENTAL:
    case RACE_TYPE_MAGICAL_BEAST:
      return ABILITY_ARCANA;    
  }
  return ABILITY_NATURE;
}

bool has_sage_mob_bonus(struct char_data *ch)
{
  struct char_data *tch, *mob;
  bool result = false;

  if (IN_ROOM(ch) == NOWHERE)
    return false;

  for (tch = world[IN_ROOM(ch)].people; tch; tch = tch->next_in_room)
  {
    if (!IS_NPC(tch) && HAS_FEAT(tch, FEAT_BG_SAGE) && GET_SAGE_MOB_VNUM(tch) > 0)
    {
      if (GROUP(tch) == GROUP(ch) || tch == ch)
      {
        for (mob = world[IN_ROOM(ch)].people; mob; mob = mob->next_in_room)
        {
          if (!IS_NPC(mob)) continue;
          if (GET_MOB_VNUM(mob) == GET_SAGE_MOB_VNUM(tch))
          {
            result = true;
            break;
          }
        }
      }
    }
  }
  return result;
}

bool is_grouped_with_soldier(struct char_data *ch)
{

  struct char_data *tch;

  if (!ch)
    return false;

  if (IN_ROOM(ch) == NOWHERE)
    return false;

  if (IS_NPC(ch))
    return false;

  if (!HAS_FEAT(ch, FEAT_BG_SOLDIER))
    return false;

  for (tch = world[IN_ROOM(ch)].people; tch; tch = tch->next_in_room)
  {
    if (ch == tch) continue;
    if (GROUP(tch) != GROUP(ch)) continue;
    return true;
  }
  return false;
}

bool is_retainer_in_room(struct char_data *ch)
{
  struct char_data *tch;

  if (!ch)
    return false;

  if (IN_ROOM(ch) == NOWHERE)
    return false;

  if (IS_NPC(ch))
    return false;

  if (!HAS_FEAT(ch, FEAT_BG_SQUIRE))
    return false;

  for (tch = world[IN_ROOM(ch)].people; tch; tch = tch->next_in_room)
  {
    if (!IS_NPC(tch)) continue;
    if (tch->master != ch) continue;
    if (!MOB_FLAGGED(tch, MOB_RETAINER)) continue;
    return true;
  }
  return false;
}

struct char_data *get_retainer_from_room(struct char_data *ch)
{
  struct char_data *tch;

  if (!ch)
    return NULL;

  if (IN_ROOM(ch) == NOWHERE)
    return NULL;

  if (IS_NPC(ch))
    return NULL;

  if (!HAS_FEAT(ch, FEAT_BG_SQUIRE))
    return NULL;

  for (tch = world[IN_ROOM(ch)].people; tch; tch = tch->next_in_room)
  {
    if (!IS_NPC(tch)) continue;
    if (tch->master != ch) continue;
    if (!MOB_FLAGGED(tch, MOB_RETAINER)) continue;
    return tch;
  }
  return NULL;
}

int get_smite_evil_level(struct char_data *ch)
{
 int smite_level = 0;

 smite_level += CLASS_LEVEL(ch, CLASS_PALADIN);
 smite_level += CLASS_LEVEL(ch, CLASS_KNIGHT_OF_THE_SWORD);

 return smite_level;
}

int get_smite_good_level(struct char_data *ch)
{
 int smite_level = 0;

 smite_level += CLASS_LEVEL(ch, CLASS_BLACKGUARD);
 smite_level += CLASS_LEVEL(ch, CLASS_KNIGHT_OF_THE_SKULL);

 return smite_level;
}

bool has_dr_affect(struct char_data *ch, int spell)
{
  if (!ch) return false;

  struct damage_reduction_type *dr;
  for (dr = GET_DR(ch); dr != NULL; dr = dr->next)
  {
    if (dr->spell == spell)
    {
      return true;
    }
  }
  return false;
}

bool is_high_hp_mob(struct char_data *mob)
{

  if (!mob) return false;
  if (!IS_NPC(mob)) return false;

  int hp = GET_MAX_HIT(mob);
  int level = GET_LEVEL(mob);
  int high_hp = (level * level) + (level * 10) * 2;

  if (hp >= high_hp)
    return true;

  return false;

}

bool has_overwhelming_critical_prereqs(struct char_data *ch, struct obj_data *wielded)
{
  if (!ch) return false;

  // barehanded
  if (wielded == NULL)
  {
    if (!HAS_COMBAT_FEAT(ch, feat_to_cfeat(FEAT_IMPROVED_CRITICAL), WEAPON_FAMILY_MONK))
      return false;
    if (!HAS_COMBAT_FEAT(ch, feat_to_cfeat(FEAT_WEAPON_FOCUS), WEAPON_FAMILY_MONK))
      return false;
    if (!HAS_COMBAT_FEAT(ch, feat_to_cfeat(FEAT_OVERWHELMING_CRITICAL), WEAPON_FAMILY_MONK))
      return false;
  }
  else
  {
    if (!HAS_COMBAT_FEAT(ch, feat_to_cfeat(FEAT_IMPROVED_CRITICAL), weapon_list[GET_WEAPON_TYPE(wielded)].weaponFamily))
      return false;
    if (!HAS_COMBAT_FEAT(ch, feat_to_cfeat(FEAT_WEAPON_FOCUS), weapon_list[GET_WEAPON_TYPE(wielded)].weaponFamily))
      return false;
    if (!HAS_COMBAT_FEAT(ch, feat_to_cfeat(FEAT_OVERWHELMING_CRITICAL), weapon_list[GET_WEAPON_TYPE(wielded)].weaponFamily))
      return false;
  }
  return true;
}

bool has_devastating_critical_prereqs(struct char_data *ch, struct obj_data *wielded)
{

  if (!has_overwhelming_critical_prereqs(ch, wielded))
    return false;

  if (wielded == NULL)
  {
    if (!HAS_COMBAT_FEAT(ch, feat_to_cfeat(FEAT_DEVASTATING_CRITICAL), WEAPON_FAMILY_MONK))
      return false;
  }
  else
  {
    if (!HAS_COMBAT_FEAT(ch, feat_to_cfeat(FEAT_DEVASTATING_CRITICAL), weapon_list[GET_WEAPON_TYPE(wielded)].weaponFamily))
      return false;
  }

  return true;
}

bool is_valid_apply_location_and_circle(int apply, int circle)
{
  switch (apply)
  {
    case APPLY_SPELL_CIRCLE_1: if (circle == 1) return true; else return false;
    case APPLY_SPELL_CIRCLE_2: if (circle == 2) return true; else return false;
    case APPLY_SPELL_CIRCLE_3: if (circle == 3) return true; else return false;
    case APPLY_SPELL_CIRCLE_4: if (circle == 4) return true; else return false;
    case APPLY_SPELL_CIRCLE_5: if (circle == 5) return true; else return false;
    case APPLY_SPELL_CIRCLE_6: if (circle == 6) return true; else return false;
    case APPLY_SPELL_CIRCLE_7: if (circle == 7) return true; else return false;
    case APPLY_SPELL_CIRCLE_8: if (circle == 8) return true; else return false;
    case APPLY_SPELL_CIRCLE_9: if (circle == 9) return true; else return false;
  }
  return false;
}

int get_bonus_spells_by_circle_and_class(struct char_data *ch, int ch_class, int circle)
{
  if (!ch || IS_NPC(ch) || !ch->desc)
    return 0;

  int i = 0, j = 0;
  int bonus_circles = 0;
  int max_value[NUM_BONUS_TYPES];
  int max_val_spell[NUM_BONUS_TYPES];
  int max_val_worn_slot[NUM_BONUS_TYPES];
  struct obj_data *obj = NULL;
  struct affected_type *aff = NULL;

  for (i = 0; i < NUM_BONUS_TYPES; i++)
  {
    max_value[i] = 0;
    max_val_spell[i] = -1;
    max_val_worn_slot[i] = -1;
  }

  // We'll do spells then gear.  We want to make sure that bonus
  // types don't stack

  // spells
  for (aff = ch->affected; aff; aff = aff->next)
  {
    if (is_valid_apply_location_and_circle(aff->location, circle) && aff->specific == ch_class)
    {
      // some bonus types always stack, so we'll just add this right on now
      if (BONUS_TYPE_STACKS(aff->bonus_type))
      {
        bonus_circles += aff->modifier;
      }
      // penalties and debuffs are always applied
      else if (aff->modifier < 0)
      {
        bonus_circles -= aff->modifier;
      }
      // we only want the maximum per bonus type
      else if (aff->modifier > max_value[aff->bonus_type])
      {
        max_value[aff->bonus_type] = aff->modifier;
        max_val_spell[aff->bonus_type] = aff->spell;
        max_val_worn_slot[aff->bonus_type] = -1;
      }
    }
  }

  // gear
  for (i = 0; i < NUM_WEARS; i++)
  {
    if (!(obj = GET_EQ(ch, i)))
      continue;
    for (j = 0; j < MAX_OBJ_AFFECT; j++)
    {
      if (is_valid_apply_location_and_circle(obj->affected[j].location, circle) && obj->affected[j].specific == ch_class)
      {
        // some bonus types always stack, so we'll just add this right on now
        if (BONUS_TYPE_STACKS(obj->affected[j].bonus_type))
        {
          bonus_circles += obj->affected[j].modifier;
        }
        // penalties and debuffs are always applied
        else if (obj->affected[j].modifier < 0)
        {
          bonus_circles -= obj->affected[j].modifier;
        }
        // we only want the maximum per bonus type
        else if (obj->affected->modifier > max_value[obj->affected[j].bonus_type])
        {
          max_value[obj->affected[j].bonus_type] = obj->affected[j].modifier;
          max_val_spell[obj->affected[j].bonus_type] = -1;
          max_val_worn_slot[obj->affected[j].bonus_type] = i;
        }
      }
    }
  }

  // now let's add up all of the highest per bonus type from spells and gear
  for (i = 0; i < NUM_BONUS_TYPES; i++)
  {
    if (BONUS_TYPE_STACKS(i))
      continue;
    bonus_circles += max_value[i];
  }

  return bonus_circles;
}

int get_spell_potency_bonus(struct char_data *ch)
{
  if (!ch || IS_NPC(ch) || !ch->desc)
    return 100;

  int i = 0, j = 0;
  int potency_bonus = 100;
  int max_value[NUM_BONUS_TYPES];
  int max_val_spell[NUM_BONUS_TYPES];
  int max_val_worn_slot[NUM_BONUS_TYPES];
  struct obj_data *obj = NULL;
  struct affected_type *aff = NULL;

  for (i = 0; i < NUM_BONUS_TYPES; i++)
  {
    max_value[i] = 0;
    max_val_spell[i] = -1;
    max_val_worn_slot[i] = -1;
  }

  // We'll do spells then gear.  We want to make sure that bonus
  // types don't stack

  // spells
  for (aff = ch->affected; aff; aff = aff->next)
  {
    if (aff->location == APPLY_SPELL_POTENCY)
    {
      // some bonus types always stack, so we'll just add this right on now
      if (BONUS_TYPE_STACKS(aff->bonus_type))
      {
        potency_bonus += aff->modifier;
      }
      // penalties and debuffs are always applied
      else if (aff->modifier < 0)
      {
        potency_bonus -= aff->modifier;
      }
      // we only want the maximum per bonus type
      else if (aff->modifier > max_value[aff->bonus_type])
      {
        max_value[aff->bonus_type] = aff->modifier;
        max_val_spell[aff->bonus_type] = aff->spell;
        max_val_worn_slot[aff->bonus_type] = -1;
      }
    }
  }

  // gear
  for (i = 0; i < NUM_WEARS; i++)
  {
    if (!(obj = GET_EQ(ch, i)))
      continue;
    for (j = 0; j < MAX_OBJ_AFFECT; j++)
    {
      if (obj->affected[j].location == APPLY_SPELL_POTENCY)
      {
        // some bonus types always stack, so we'll just add this right on now
        if (BONUS_TYPE_STACKS(obj->affected[j].bonus_type))
        {
          potency_bonus += obj->affected[j].modifier;
        }
        // penalties and debuffs are always applied
        else if (obj->affected[j].modifier < 0)
        {
          potency_bonus -= obj->affected[j].modifier;
        }
        // we only want the maximum per bonus type
        else if (obj->affected->modifier > max_value[obj->affected[j].bonus_type])
        {
          max_value[obj->affected[j].bonus_type] = obj->affected[j].modifier;
          max_val_spell[obj->affected[j].bonus_type] = -1;
          max_val_worn_slot[obj->affected[j].bonus_type] = i;
        }
      }
    }
  }

  // now let's add up all of the highest per bonus type from spells and gear
  for (i = 0; i < NUM_BONUS_TYPES; i++)
  {
    if (BONUS_TYPE_STACKS(i))
      continue;
    potency_bonus += max_value[i];
  }

  return potency_bonus;
}

int get_spell_dc_bonus(struct char_data *ch)
{
  if (!ch || IS_NPC(ch) || !ch->desc)
    return 0;

  int i = 0, j = 0;
  int dc_bonus = 0;
  int max_value[NUM_BONUS_TYPES];
  int max_val_spell[NUM_BONUS_TYPES];
  int max_val_worn_slot[NUM_BONUS_TYPES];
  struct obj_data *obj = NULL;
  struct affected_type *aff = NULL;

  for (i = 0; i < NUM_BONUS_TYPES; i++)
  {
    max_value[i] = 0;
    max_val_spell[i] = -1;
    max_val_worn_slot[i] = -1;
  }

  // We'll do spells then gear.  We want to make sure that bonus
  // types don't stack

  // spells
  for (aff = ch->affected; aff; aff = aff->next)
  {
    if (aff->location == APPLY_SPELL_DC)
    {
      // some bonus types always stack, so we'll just add this right on now
      if (BONUS_TYPE_STACKS(aff->bonus_type))
      {
        dc_bonus += aff->modifier;
      }
      // penalties and debuffs are always applied
      else if (aff->modifier < 0)
      {
        dc_bonus -= aff->modifier;
      }
      // we only want the maximum per bonus type
      else if (aff->modifier > max_value[aff->bonus_type])
      {
        max_value[aff->bonus_type] = aff->modifier;
        max_val_spell[aff->bonus_type] = aff->spell;
        max_val_worn_slot[aff->bonus_type] = -1;
      }
    }
  }

  // gear
  for (i = 0; i < NUM_WEARS; i++)
  {
    if (!(obj = GET_EQ(ch, i)))
      continue;
    for (j = 0; j < MAX_OBJ_AFFECT; j++)
    {
      if (obj->affected[j].location == APPLY_SPELL_DC)
      {
        // some bonus types always stack, so we'll just add this right on now
        if (BONUS_TYPE_STACKS(obj->affected[j].bonus_type))
        {
          dc_bonus += obj->affected[j].modifier;
        }
        // penalties and debuffs are always applied
        else if (obj->affected[j].modifier < 0)
        {
          dc_bonus -= obj->affected[j].modifier;
        }
        // we only want the maximum per bonus type
        else if (obj->affected->modifier > max_value[obj->affected[j].bonus_type])
        {
          max_value[obj->affected[j].bonus_type] = obj->affected[j].modifier;
          max_val_spell[obj->affected[j].bonus_type] = -1;
          max_val_worn_slot[obj->affected[j].bonus_type] = i;
        }
      }
    }
  }

  // now let's add up all of the highest per bonus type from spells and gear
  for (i = 0; i < NUM_BONUS_TYPES; i++)
  {
    if (BONUS_TYPE_STACKS(i))
      continue;
    dc_bonus += max_value[i];
  }

  return dc_bonus;
}

int get_spell_penetration_bonus(struct char_data *ch)
{
  if (!ch || IS_NPC(ch) || !ch->desc)
    return 0;

  int i = 0, j = 0;
  int penetration_bonus = 0;
  int max_value[NUM_BONUS_TYPES];
  int max_val_spell[NUM_BONUS_TYPES];
  int max_val_worn_slot[NUM_BONUS_TYPES];
  struct obj_data *obj = NULL;
  struct affected_type *aff = NULL;

  for (i = 0; i < NUM_BONUS_TYPES; i++)
  {
    max_value[i] = 0;
    max_val_spell[i] = -1;
    max_val_worn_slot[i] = -1;
  }

  // We'll do spells then gear.  We want to make sure that bonus
  // types don't stack

  // spells
  for (aff = ch->affected; aff; aff = aff->next)
  {
    if (aff->location == APPLY_SPELL_PENETRATION)
    {
      // some bonus types always stack, so we'll just add this right on now
      if (BONUS_TYPE_STACKS(aff->bonus_type))
      {
        penetration_bonus += aff->modifier;
      }
      // penalties and debuffs are always applied
      else if (aff->modifier < 0)
      {
        penetration_bonus -= aff->modifier;
      }
      // we only want the maximum per bonus type
      else if (aff->modifier > max_value[aff->bonus_type])
      {
        max_value[aff->bonus_type] = aff->modifier;
        max_val_spell[aff->bonus_type] = aff->spell;
        max_val_worn_slot[aff->bonus_type] = -1;
      }
    }
  }

  // gear
  for (i = 0; i < NUM_WEARS; i++)
  {
    if (!(obj = GET_EQ(ch, i)))
      continue;
    for (j = 0; j < MAX_OBJ_AFFECT; j++)
    {
      if (obj->affected[j].location == APPLY_SPELL_PENETRATION)
      {
        // some bonus types always stack, so we'll just add this right on now
        if (BONUS_TYPE_STACKS(obj->affected[j].bonus_type))
        {
          penetration_bonus += obj->affected[j].modifier;
        }
        // penalties and debuffs are always applied
        else if (obj->affected[j].modifier < 0)
        {
          penetration_bonus -= obj->affected[j].modifier;
        }
        // we only want the maximum per bonus type
        else if (obj->affected->modifier > max_value[obj->affected[j].bonus_type])
        {
          max_value[obj->affected[j].bonus_type] = obj->affected[j].modifier;
          max_val_spell[obj->affected[j].bonus_type] = -1;
          max_val_worn_slot[obj->affected[j].bonus_type] = i;
        }
      }
    }
  }

  // now let's add up all of the highest per bonus type from spells and gear
  for (i = 0; i < NUM_BONUS_TYPES; i++)
  {
    if (BONUS_TYPE_STACKS(i))
      continue;
    penetration_bonus += max_value[i];
  }

  return penetration_bonus;
}

int get_spell_duration_bonus(struct char_data *ch)
{
  if (!ch || IS_NPC(ch) || !ch->desc)
    return 100;

  int i = 0, j = 0;
  int duration_bonus = 100;
  int max_value[NUM_BONUS_TYPES];
  int max_val_spell[NUM_BONUS_TYPES];
  int max_val_worn_slot[NUM_BONUS_TYPES];
  struct obj_data *obj = NULL;
  struct affected_type *aff = NULL;

  for (i = 0; i < NUM_BONUS_TYPES; i++)
  {
    max_value[i] = 0;
    max_val_spell[i] = -1;
    max_val_worn_slot[i] = -1;
  }

  // We'll do spells then gear.  We want to make sure that bonus
  // types don't stack

  // spells
  for (aff = ch->affected; aff; aff = aff->next)
  {
    if (aff->location == APPLY_SPELL_DURATION)
    {
      // some bonus types always stack, so we'll just add this right on now
      if (BONUS_TYPE_STACKS(aff->bonus_type))
      {
        duration_bonus += aff->modifier;
      }
      // penalties and debuffs are always applied
      else if (aff->modifier < 0)
      {
        duration_bonus -= aff->modifier;
      }
      // we only want the maximum per bonus type
      else if (aff->modifier > max_value[aff->bonus_type])
      {
        max_value[aff->bonus_type] = aff->modifier;
        max_val_spell[aff->bonus_type] = aff->spell;
        max_val_worn_slot[aff->bonus_type] = -1;
      }
    }
  }

  // gear
  for (i = 0; i < NUM_WEARS; i++)
  {
    if (!(obj = GET_EQ(ch, i)))
      continue;
    for (j = 0; j < MAX_OBJ_AFFECT; j++)
    {
      if (obj->affected[j].location == APPLY_SPELL_DURATION)
      {
        // some bonus types always stack, so we'll just add this right on now
        if (BONUS_TYPE_STACKS(obj->affected[j].bonus_type))
        {
          duration_bonus += obj->affected[j].modifier;
        }
        // penalties and debuffs are always applied
        else if (obj->affected[j].modifier < 0)
        {
          duration_bonus -= obj->affected[j].modifier;
        }
        // we only want the maximum per bonus type
        else if (obj->affected->modifier > max_value[obj->affected[j].bonus_type])
        {
          max_value[obj->affected[j].bonus_type] = obj->affected[j].modifier;
          max_val_spell[obj->affected[j].bonus_type] = -1;
          max_val_worn_slot[obj->affected[j].bonus_type] = i;
        }
      }
    }
  }

  // now let's add up all of the highest per bonus type from spells and gear
  for (i = 0; i < NUM_BONUS_TYPES; i++)
  {
    if (BONUS_TYPE_STACKS(i))
      continue;
    duration_bonus += max_value[i];
  }

  return MAX(100, duration_bonus);
}

int get_random_spellcaster_class(void)
{
  int chclass = -1;
  do
  {
    chclass = rand_number(0, NUM_CLASSES);
  }
  while (!IS_SPELLCASTER_CLASS(chclass));

  return chclass;
}

int determine_random_spell_circle_bonus(void)
{
  int which = dice(1, 100);

  if (which <= 20)
    return APPLY_SPELL_CIRCLE_1;
  else if (which <= 35)
    return APPLY_SPELL_CIRCLE_2;
  else if (which <= 50)
    return APPLY_SPELL_CIRCLE_3;
  else if (which <= 65)
    return APPLY_SPELL_CIRCLE_4;
  else if (which <= 75)
    return APPLY_SPELL_CIRCLE_5;
  else if (which <= 85)
    return APPLY_SPELL_CIRCLE_6;
  else if (which <= 90)
    return APPLY_SPELL_CIRCLE_7;
  else if (which <= 95)
    return APPLY_SPELL_CIRCLE_8;
  else
    return APPLY_SPELL_CIRCLE_9;

  return APPLY_SPELL_CIRCLE_1;
}

// need to add crafting skills once new crafting system is in -- gicker
int get_random_skill(void)
{
  return dice(1, END_GENERAL_ABILITIES);
}

// checks to see if target is in the list of ch's known people
bool in_intro_list(struct char_data *ch, struct char_data *target)
{
  int i;

  for (i = 0; i < MAX_INTROS; i++)
  {
    if (GET_INTRO(ch, i) == GET_IDNUM(target))
      return true;
    else
      continue;
  }
  return false;
}

// checks to see if ch knows target's real name
bool has_intro(struct char_data *ch, struct char_data *target)
{

  if (!ch || !target)
    return false;

  if (ch == target)
    return true;

  if (GET_LEVEL(ch) >= LVL_IMMORT || GET_LEVEL(target) >= LVL_IMMORT)
    return true;

  if (!IS_NPC(target) && PRF_FLAGGED(target, PRF_NON_ROLEPLAYER))
    return true;

  if (in_intro_list(ch, target))
      true;

  return false;
}

char * which_desc(struct char_data *ch, struct char_data *target)
{

  if (!target)
    return strdup("error");

  if (IS_NPC(target))
    return GET_SHORT(target);

  if (IS_MORPHED(target))
    return current_morphed_desc(target);

  if (IS_WILDSHAPED(target))
    return current_wildshape_desc(target);
    
  if (GET_DISGUISE_RACE(target))
    return current_disguise_desc(target);
  
  return current_short_desc(target);
}

// checks the spell or skill num to see if it should hide damage
// messages from persons other than the recipient of the spell/skill's affect
bool hide_damage_message(int snum)
{
  switch (snum)
  {
    case SPELL_POISON:
      return true;
  }
  return false;
}

bool is_valid_apply(int apply)
{
  switch (apply)
  {
    case APPLY_NONE:
    case APPLY_CLASS:
    case APPLY_LEVEL:
    case APPLY_AGE:
    case APPLY_CHAR_WEIGHT:
    case APPLY_CHAR_HEIGHT:
    case APPLY_GOLD:
    case APPLY_EXP:
    case APPLY_AC:
    case APPLY_SAVING_POISON:
    case APPLY_SAVING_DEATH:
    case APPLY_SIZE:
    case APPLY_DR:
    case APPLY_SPECIAL:
    case APPLY_ELDRITCH_SHAPE:
    case APPLY_ELDRITCH_ESSENCE:
      return false;
  }

  return true;
}

int max_bonus_modifier(int location, int bonus_type)
{

  int max_modifier = 0;

  switch (location)
  {
    case APPLY_STR:
    case APPLY_DEX:
    case APPLY_INT:
    case APPLY_WIS:
    case APPLY_CON:
    case APPLY_CHA:
      max_modifier = 9; break;

    case APPLY_PSP:
      max_modifier = 100; break;
    
    case APPLY_HIT:
      max_modifier = 100; break;
    
    case APPLY_MOVE:
      max_modifier = 1000; break;
    
    case APPLY_HITROLL:
    case APPLY_DAMROLL:
      max_modifier = 6; break;
    
    case APPLY_SAVING_FORT:
    case APPLY_SAVING_REFL:
    case APPLY_SAVING_WILL:
      max_modifier = 6; break;
    
    case APPLY_AC_NEW:
      max_modifier = 9; break;
    
    case APPLY_RES_FIRE:
    case APPLY_RES_COLD:
    case APPLY_RES_AIR:
    case APPLY_RES_EARTH:
    case APPLY_RES_ACID:
    case APPLY_RES_HOLY:
    case APPLY_RES_ELECTRIC:
    case APPLY_RES_UNHOLY:
    case APPLY_RES_SLICE:
    case APPLY_RES_PUNCTURE:
    case APPLY_RES_FORCE:
    case APPLY_RES_SOUND:
    case APPLY_RES_POISON:
    case APPLY_RES_DISEASE:
    case APPLY_RES_NEGATIVE:
    case APPLY_RES_ILLUSION:
    case APPLY_RES_MENTAL:
    case APPLY_RES_LIGHT:
    case APPLY_RES_ENERGY:
    case APPLY_RES_WATER:
      max_modifier = 30; break;

    case APPLY_FEAT:
      max_modifier = 1; break;
    
    case APPLY_SKILL:
      max_modifier = 6; break;

    case APPLY_POWER_RES:
    case APPLY_SPELL_RES:
      max_modifier = 20; break;

    case APPLY_HP_REGEN:
    case APPLY_PSP_REGEN:
      max_modifier = 6; break;
    
    case APPLY_MV_REGEN:
      max_modifier = 60; break;
   
    case APPLY_ENCUMBRANCE:
      max_modifier = 6; break;
    
    case APPLY_FAST_HEALING:
      max_modifier = 3; break;
    
    case APPLY_INITIATIVE:
      max_modifier = 6; break;
    
    case APPLY_SPELL_CIRCLE_1:
    case APPLY_SPELL_CIRCLE_2:
    case APPLY_SPELL_CIRCLE_3:    
    case APPLY_SPELL_CIRCLE_4:
    case APPLY_SPELL_CIRCLE_5:
    case APPLY_SPELL_CIRCLE_6:
    case APPLY_SPELL_CIRCLE_7:
    case APPLY_SPELL_CIRCLE_8:
    case APPLY_SPELL_CIRCLE_9:
      max_modifier = 3; break;
    
    case APPLY_SPELL_POTENCY:
    case APPLY_SPELL_DURATION:
      max_modifier = 30; break;

    case APPLY_SPELL_DC:
      max_modifier = 3; break;
    
    case APPLY_SPELL_PENETRATION:
      max_modifier = 3; break;
  }

  if (bonus_type == BONUS_TYPE_UNIVERSAL)
    max_modifier /= 3;

  return MAX(1, max_modifier);

}

bool is_exit_hidden(struct char_data *ch, int dir)
{

  if (dir < 0 || dir >= NUM_OF_DIRS)
    return false;
  
  if (GET_LEVEL(ch) >= LVL_IMMORT)
    return false;

   if (EXIT_FLAGGED(EXIT(ch, dir), EX_HIDDEN))
    return true;
   else if (EXIT_FLAGGED(EXIT(ch, dir), EX_HIDDEN_EASY))
    return true;
  else if (EXIT_FLAGGED(EXIT(ch, dir), EX_HIDDEN_MEDIUM))
    return true;
  else if (EXIT_FLAGGED(EXIT(ch, dir), EX_HIDDEN_HARD))
    return true;

  return false;
}

bool is_exit_locked(struct char_data *ch, int dir)
{
  if (dir < 0 || dir >= NUM_OF_DIRS)
    return false;
  
  if (EXIT_FLAGGED(EXIT(ch, dir), EX_LOCKED))
    return true;
  else if (EXIT_FLAGGED(EXIT(ch, dir), EX_LOCKED_EASY))
    return true;
  else if (EXIT_FLAGGED(EXIT(ch, dir), EX_LOCKED_MEDIUM))
    return true;
  else if (EXIT_FLAGGED(EXIT(ch, dir), EX_LOCKED_HARD))
    return true;

  return false;
}

bool is_weapon_wielded_two_handed(struct obj_data *obj, struct char_data *ch)
{

  int wsize, csize;

  if (!obj || !ch)
    return false;

  if (GET_OBJ_TYPE(obj) != ITEM_WEAPON)
    return false;

  wsize = GET_OBJ_SIZE(obj);
  csize = GET_SIZE(ch);

  if (wsize >= csize)
  {
    if (GET_EQ(ch, WEAR_HOLD_1))
      return false;
    if (GET_EQ(ch, WEAR_HOLD_2))
      return false;
    if (GET_EQ(ch, WEAR_WIELD_OFFHAND))
      return false;
    if (GET_EQ(ch, WEAR_SHIELD))
      return false;
  }
  else
    return false;

  return true;  
}

bool is_valid_skill(int snum)
{
  switch (snum)
  {
    case ABILITY_ACROBATICS:
    case ABILITY_APPRAISE:
    case ABILITY_ARCANA:
    case ABILITY_ATHLETICS:
    case ABILITY_CONCENTRATION:
    case ABILITY_DECEPTION:
    case ABILITY_DISABLE_DEVICE:
    case ABILITY_DISCIPLINE:
    case ABILITY_DISGUISE:
    case ABILITY_HANDLE_ANIMAL:
    case ABILITY_HISTORY:
    case ABILITY_INSIGHT:
    case ABILITY_INTIMIDATE:
    case ABILITY_LINGUISTICS:
    case ABILITY_MEDICINE:
    case ABILITY_NATURE:
    case ABILITY_PERCEPTION:
    case ABILITY_PERFORM:
    case ABILITY_PERSUASION:
    case ABILITY_RELIGION:
    case ABILITY_RIDE:
    case ABILITY_SLEIGHT_OF_HAND:
    case ABILITY_SPELLCRAFT:
    case ABILITY_STEALTH:
    case ABILITY_TOTAL_DEFENSE:
    case ABILITY_USE_MAGIC_DEVICE:
      return true;
  }
  return false;
}

/* EoF */

