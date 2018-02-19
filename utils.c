/**
 * @file utils.c
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
#include "spec_procs.h"  // for compute_ability
#include "mud_event.h"  // for purgemob event
#include "feats.h"
#include "spec_abilities.h"
#include "assign_wpn_armor.h"
#include "wilderness.h"
#include "domains_schools.h"
#include "constants.h"
#include "dg_scripts.h"

/* kavir's protocol (isspace_ignoretabes() was moved to utils.h */

/* Functions of a general utility nature
   Functions directly related to utils.h needs
 */

/* MSDP GUI Wrappers */
void gui_combat_wrap_open(struct char_data *ch) {
  if (!IS_NPC(ch) && PRF_FLAGGED(ch, PRF_GUI_MODE)) { /* GUI Mode wrap open: combat */
    //send_to_char(ch, "<combat_message>\r\n");
  }
}

void gui_combat_wrap_notvict_open(struct char_data *ch, struct char_data *vict_obj) {
  if (!ch)
    return;
  if (IN_ROOM(ch) == NOWHERE)
    return;
  if (IS_NPC(ch))
    return;

  /* we accept NULL victim */

  int to_sleeping = 0;
  struct char_data *to = world[IN_ROOM(ch)].people;

  for (; to; to = to->next_in_room) {
    if (!SENDOK(to) || (to == ch))
      continue;
    if (to == vict_obj) /* ch == victim? */
      continue;
    if (!PRF_FLAGGED(to, PRF_GUI_MODE))
      continue;
    //perform_act("<combat_message>\r\n", ch, NULL, vict_obj, to, FALSE);
  }
}

void gui_combat_wrap_close(struct char_data *ch) {
  if (!IS_NPC(ch) && PRF_FLAGGED(ch, PRF_GUI_MODE)) { /* GUI Mode wrap close: combat */
    //send_to_char(ch, "</combat_message>\r\n");
  }
}

void gui_combat_wrap_notvict_close(struct char_data *ch, struct char_data *vict_obj) {
  if (!ch)
    return;
  if (IN_ROOM(ch) == NOWHERE)
    return;
  if (IS_NPC(ch))
    return;

  /* we accept NULL victim */

  int to_sleeping = 0;
  struct char_data *to = world[IN_ROOM(ch)].people;

  for (; to; to = to->next_in_room) {
    if (to == ch)
      continue;
    if (!SENDOK(to))
      continue;
    if (to == vict_obj) /* ch == victim? */
      continue;
    if (!PRF_FLAGGED(to, PRF_GUI_MODE))
      continue;
    //perform_act("</combat_message>\r\n", ch, NULL, vict_obj, to, FALSE);
  }
}

void gui_room_desc_wrap_open(struct char_data *ch) {
  if (!IS_NPC(ch) && PRF_FLAGGED(ch, PRF_GUI_MODE)) { /* GUI Mode wrap open: room description */
    send_to_char(ch, "<room_desc>");
  }
}

void gui_room_desc_wrap_close(struct char_data *ch) {
  if (!IS_NPC(ch) && PRF_FLAGGED(ch, PRF_GUI_MODE)) { /* GUI Mode wrap close: room description */
    send_to_char(ch, "</room_desc>");
  }
}

/* can this CH select the option to change their 'known' spells
 in the study system? */
bool can_study_known_spells(struct char_data *ch) {

  /* sorcerer*/
  if (LEVELUP(ch)->class == CLASS_SORCERER ||
          (LEVELUP(ch)->class == CLASS_ARCANE_ARCHER &&
          GET_PREFERRED_ARCANE(ch) == CLASS_SORCERER))
    return TRUE;

  /* bard */
  if (LEVELUP(ch)->class == CLASS_BARD ||
          (LEVELUP(ch)->class == CLASS_ARCANE_ARCHER &&
          GET_PREFERRED_ARCANE(ch) == CLASS_BARD))
    return TRUE;

  /* nope! */
  return FALSE;
}

/* ch, given class we're computing bonus spells for, figure out
 if one of our other classes (probably just prestige, example is
 arcane archer) is adding bonus caster levels */
int compute_bonus_caster_level(struct char_data *ch, int class) {
  int bonus_levels = 0;

  switch (class) {
    case CLASS_WIZARD:
    case CLASS_SORCERER:
    case CLASS_BARD:
      if (class == GET_PREFERRED_ARCANE(ch))
        bonus_levels += CLASS_LEVEL(ch, CLASS_ARCANE_ARCHER) * 3 / 4;
      break;
    case CLASS_CLERIC:
    case CLASS_DRUID:
      break;
    default:break;
  }

  return bonus_levels;
}

int compute_arcane_level(struct char_data *ch) {
  int arcane_level = 0;

  if (IS_NPC(ch)) /* npc is simple for now */
    return (GET_LEVEL(ch));

  arcane_level += CLASS_LEVEL(ch, CLASS_WIZARD);
  arcane_level += CLASS_LEVEL(ch, CLASS_SORCERER);
  arcane_level += CLASS_LEVEL(ch, CLASS_BARD);
  arcane_level += CLASS_LEVEL(ch, CLASS_ARCANE_ARCHER) * 3 / 4;
  arcane_level += compute_arcana_golem_level(ch) - (SPELLBATTLE(ch) / 2);

  return arcane_level;
}

/* check to see if CH has a weapon attached to a combat feat
this use to be a nice(?) compact macro, but circumstances forced expansion */
bool compute_has_combat_feat(struct char_data *ch, int cfeat, int weapon) {
  bool using_comp = FALSE;
  bool has_comp_feat = FALSE;

  /* had to add special test to weapon combat feats because of the way
     I set up composite bows with various strength modifiers -zusuk */
  switch (weapon) {
    case WEAPON_TYPE_COMPOSITE_LONGBOW:
    case WEAPON_TYPE_COMPOSITE_LONGBOW_2:
    case WEAPON_TYPE_COMPOSITE_LONGBOW_3:
    case WEAPON_TYPE_COMPOSITE_LONGBOW_4:
    case WEAPON_TYPE_COMPOSITE_LONGBOW_5:
      using_comp = TRUE;
      break;
    default:break; /* most cases */
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
          WEAPON_TYPE_COMPOSITE_LONGBOW_5)
          ) {
    has_comp_feat = TRUE;
  }

  if (using_comp && has_comp_feat)
    return TRUE; /* any comp longbow feat, any comp longbow used */

  /* reset variables */
  has_comp_feat = FALSE;
  using_comp = FALSE;

  switch (weapon) {
    case WEAPON_TYPE_COMPOSITE_SHORTBOW:
    case WEAPON_TYPE_COMPOSITE_SHORTBOW_2:
    case WEAPON_TYPE_COMPOSITE_SHORTBOW_3:
    case WEAPON_TYPE_COMPOSITE_SHORTBOW_4:
    case WEAPON_TYPE_COMPOSITE_SHORTBOW_5:
      using_comp = TRUE;
      break;
    default:break; /* most cases */
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
          WEAPON_TYPE_COMPOSITE_SHORTBOW_5)
          ) {
    has_comp_feat = TRUE;
  }

  if (using_comp && has_comp_feat)
    return TRUE; /* any comp bow feat, any comp bow used */

  /*debug*/
  //send_to_char(ch, "feat: %d, weapon: %d\r\n", cfeat, weapon);

  /* normal test now */
  if ((IS_SET_AR((ch)->char_specials.saved.combat_feats[(cfeat)], (weapon))))
    return TRUE;

  /* nope, nothing! */
  return FALSE;
}

int compute_dexterity_bonus(struct char_data *ch) {
  if (!ch) return 0;
  int dexterity_bonus = (GET_DEX(ch) - 10) / 2;
  if (AFF_FLAGGED(ch, AFF_GRAPPLED) || AFF_FLAGGED(ch, AFF_ENTANGLED))
    dexterity_bonus -= 2;
  if (AFF_FLAGGED(ch, AFF_PINNED))
    dexterity_bonus = -5;

  return (MIN(compute_gear_max_dex(ch), dexterity_bonus));
}

#define TOTAL_STAT_POINTS 30
#define MAX_POINTS_IN_A_STAT 10
#define BASE_STAT 8
int stats_cost_chart[11] = {/* cost for total points */
  /*0  1  2  3  4  5  6  7  8   9   10 */
  0, 1, 2, 3, 4, 5, 6, 8, 10, 13, 16
};

int comp_base_dex(struct char_data *ch) {
  int base_dex = BASE_STAT;
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
  return base_dex;
}

int comp_dex_cost(struct char_data *ch, int number) {
  int base_dex = comp_base_dex(ch), current_dex = GET_REAL_DEX(ch) + number;
  return stats_cost_chart[current_dex - base_dex];
}

int comp_base_str(struct char_data *ch) {
  int base_str = BASE_STAT;
  switch (GET_RACE(ch)) {
    case RACE_HALFLING: base_str -= 2;
      break;
    case RACE_GNOME: base_str -= 2;
      break;
    case RACE_HALF_TROLL: base_str += 2;
      break;
    case RACE_CRYSTAL_DWARF: base_str += 2;
      break;
    case RACE_TRELUX: base_str += 2;
      break;
    case RACE_ARCANA_GOLEM: base_str -= 2;
      break;
  }
  return base_str;
}

int comp_str_cost(struct char_data *ch, int number) {
  int base_str = comp_base_str(ch),
          current_str = GET_REAL_STR(ch) + number;
  return stats_cost_chart[current_str - base_str];
}

int comp_base_con(struct char_data *ch) {
  int base_con = BASE_STAT;
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
  return base_con;
}

int comp_con_cost(struct char_data *ch, int number) {
  int base_con = comp_base_con(ch), current_con = GET_REAL_CON(ch) + number;
  return stats_cost_chart[current_con - base_con];
}

int comp_base_inte(struct char_data *ch) {
  int base_inte = BASE_STAT;
  switch (GET_RACE(ch)) {
    case RACE_HALF_TROLL: base_inte -= 4;
      break;
    case RACE_ARCANA_GOLEM: base_inte += 2;
      break;
    case RACE_DROW: base_inte += 2;
      break;
  }
  return base_inte;
}

int comp_inte_cost(struct char_data *ch, int number) {
  int base_inte = comp_base_inte(ch), current_inte = GET_REAL_INT(ch) + number;
  return stats_cost_chart[current_inte - base_inte];
}

int comp_base_wis(struct char_data *ch) {
  int base_wis = BASE_STAT;
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
  return base_wis;
}

int comp_wis_cost(struct char_data *ch, int number) {
  int base_wis = comp_base_wis(ch), current_wis = GET_REAL_WIS(ch) + number;
  return stats_cost_chart[current_wis - base_wis];
}

int comp_base_cha(struct char_data *ch) {
  int base_cha = BASE_STAT;
  switch (GET_RACE(ch)) {
    case RACE_DWARF: base_cha -= 2;
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
  return base_cha;
}

int comp_cha_cost(struct char_data *ch, int number) {
  int base_cha = comp_base_cha(ch), current_cha = GET_REAL_CHA(ch) + number;
  return stats_cost_chart[current_cha - base_cha];
}

int comp_total_stat_points(struct char_data *ch) {
  return (comp_cha_cost(ch, 0) + comp_wis_cost(ch, 0) + comp_inte_cost(ch, 0) +
          comp_str_cost(ch, 0) + comp_dex_cost(ch, 0) + comp_con_cost(ch, 0));
}

int stats_point_left(struct char_data *ch) {
  return (TOTAL_STAT_POINTS - comp_total_stat_points(ch));
}

/* unused */
int spell_level_ch(struct char_data *ch, int spell) {
  int domain = 0, domain_spell = 0, this_spell = -1, i = 0;

  if (CLASS_LEVEL(ch, CLASS_CLERIC)) {
    for (domain = 0; domain < NUM_DOMAINS; domain++) {

      if (GET_1ST_DOMAIN(ch) == domain || GET_2ND_DOMAIN(ch) == domain) {
        /* we have this domain, check if spell is granted */

        for (domain_spell = 0; domain_spell < MAX_DOMAIN_SPELLS; domain_spell++) {
          this_spell = domain_list[domain].domain_spells[domain_spell];
          if (this_spell == spell) {
            return ((domain_spell + 1) * 2 - 1); /* returning level that corresponds to circle (i) */
          }

        }

      }
    }
  } else { /* not cleric */
  }

  for (i = 0; i < NUM_CLASSES; i++) {
    if (spell_info[spell].min_level[i] < LVL_IMMORT) {
      return (spell_info[spell].min_level[i]);
    }
  }

  return (LVL_IMPL + 1);
}

int compute_level_domain_spell_is_granted(int domain, int spell) {
  int i = 0, this_spell = -1;

  for (i = 0; i < MAX_DOMAIN_SPELLS; i++) {
    this_spell = domain_list[domain].domain_spells[i];
    if (this_spell == spell) {
      return ((i + 1) * 2 - 1); /* returning level that corresponds to circle (i) */
    }
  }

  /* failed */
  return (LVL_IMPL + 1);
}

/* check if the player's size should be different than that stored in-file */
int compute_current_size(struct char_data *ch) {
  int size = SIZE_UNDEFINED;
  if (!ch) return size;
  int racenum = GET_DISGUISE_RACE(ch);

  if (AFF_FLAGGED(ch, AFF_WILD_SHAPE) && racenum) /* wildhsaped */
    size = race_list[racenum].size;
  else
    size = (ch)->points.size;

  if (size < SIZE_FINE)
    size = SIZE_FINE;
  if (size > SIZE_COLOSSAL)
    size = SIZE_COLOSSAL;

  return size;
}

/* Take a room and direction and give the resulting room vnum, made by zusuk
 * for ornir's wilderness, returns "NOWHERE" on failure */
room_vnum get_direction_vnum(room_rnum room_origin, int direction) {
  room_rnum exit_rnum = NOWHERE;
  room_vnum exit_vnum = NOWHERE;
  int x_coordinate = -1, y_coordinate = -1;

  /* exit values */
  if (!VALID_ROOM_RNUM(room_origin))
    return NOWHERE;
  if (direction >= NUM_OF_INGAME_DIRS || direction < 0)
    return NOWHERE;

  /* is there even an exit this way? */
  if (W_EXIT(room_origin, direction)) {
    exit_rnum = W_EXIT(room_origin, direction)->to_room;
  } else {
    return NOWHERE;
  }
  exit_vnum = GET_ROOM_VNUM(exit_rnum);

  /* handle wilderness, if the room exists we have to fix the vnum */
  if (exit_vnum == 1000000) {
    x_coordinate = world[room_origin].coords[0];
    y_coordinate = world[room_origin].coords[1];
    switch (direction) {
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
        //Bad direction for wilderness travel (up/down)
        return NOWHERE;
    }
    exit_rnum = find_room_by_coordinates(x_coordinate, y_coordinate);
    if (exit_rnum == NOWHERE) {
      exit_rnum = find_available_wilderness_room();
      if (exit_rnum == NOWHERE) {
        log("SYSERR: Wilderness utility failure.");
        return NOWHERE;
      }
      /* Must set the coords, etc in the exit room. */
      assign_wilderness_room(exit_rnum, x_coordinate, y_coordinate);
    }
    exit_vnum = GET_ROOM_VNUM(exit_rnum);
    /* what should we do if it's a wilderness room? */
  } else { /* should be a normal room */
    /* exit_vnum already has the correct value */
  }

  return exit_vnum;
}

/* this function in conjuction with the AFF_GROUP flag will cause mobs who
   load in the same room to group up */
void set_mob_grouping(struct char_data *ch) {
  struct char_data *tch = NULL, *tch_next = NULL;

  for (tch = world[ch->in_room].people; tch; tch = tch_next) {
    tch_next = tch->next_in_room;
    if (IS_NPC(tch) && IS_NPC(ch) && AFF_FLAGGED(ch, AFF_GROUP) && tch != ch &&
            AFF_FLAGGED(tch, AFF_GROUP) && !tch->master) {
      if (!GROUP(tch))
        create_group(tch);
      add_follower(ch, tch);
      join_group(ch, GROUP(tch));
      return;
    }
  }

}

/* identifies if room is outdoors or not (as opposed to room num)
 used by:  ROOM_OUTSIDE() macro */
bool is_room_outdoors(room_rnum room_number) {
  if (room_number == NOWHERE) {
    log("is_room_outdoors() found NOWHERE");
    return FALSE;
  }

  if (ROOM_FLAGGED(room_number, ROOM_INDOORS))
    return FALSE;

  switch (SECT(room_number)) {
    case SECT_INSIDE:
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

/* identifies if CH is outdoors or not (as opposed to room num)
 used by:  OUTSIDE() macro */
bool is_outdoors(struct char_data *ch) {
  if (!ch) {
    log("is_outdoors() receive NULL ch");
    return FALSE;
  }

  return (is_room_outdoors(IN_ROOM(ch)));
}

/* will check if ch should suffer from ultra-blindness */
bool ultra_blind(struct char_data *ch, room_rnum room_number) {
  if (!AFF_FLAGGED(ch, AFF_ULTRAVISION))
    return FALSE;

  if (world[room_number].globe)
    return FALSE;

  if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_INDOORS))
    return FALSE;

  if (AFF_FLAGGED(ch, AFF_DARKVISION))
    return FALSE;

  if (IS_SET_AR(ROOM_FLAGS(room_number), ROOM_FOG))
    return FALSE;

  switch (SECT(room_number)) {
    case SECT_INSIDE:
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
int num_obj_in_obj(struct obj_data *obj) {
  int i = 0;

  for (; obj; obj = obj->next_content)
    i++;

  return i;
}

/* Ils: color code counter */

/* homeland-port - don't think it even works in our codebase */
int color_count(char *bufptr) {
  int count = 0;
  char *temp = bufptr;

  while ((temp = strchr(temp, '@')) != NULL) {
    if (*(temp + 1) == '@') {
      count++; /* adjust count 1 char for every && */
      temp++; /* point to char after 2nd && */
    } else {
      count += 2; /* adjust count by 3 for every color change */
      temp++; /* point to char after color codes */
    }
    if (*temp == '\0')
      break;
  }

  return count;
}

/* check if ch has a misc follower or not */
bool has_pet_follower(struct char_data *ch) {
  struct follow_type *k = NULL, *next = NULL;

  for (k = ch->followers; k; k = next) {
    next = k->next;
    if (IS_NPC(k->follower) && AFF_FLAGGED(k->follower, AFF_CHARM) &&
            !MOB_FLAGGED(k->follower, MOB_ELEMENTAL) &&
            !MOB_FLAGGED(k->follower, MOB_ANIMATED_DEAD) &&
            !MOB_FLAGGED(k->follower, MOB_C_FAMILIAR) &&
            !MOB_FLAGGED(k->follower, MOB_C_ANIMAL) &&
            !MOB_FLAGGED(k->follower, MOB_C_MOUNT)
            ) {
      return TRUE;
    }
  }

  return FALSE;
}

/* check if ch has an elemental follower or not */
bool has_elemental_follower(struct char_data *ch) {
  struct follow_type *k = NULL, *next = NULL;

  for (k = ch->followers; k; k = next) {
    next = k->next;
    if (IS_NPC(k->follower) && AFF_FLAGGED(k->follower, AFF_CHARM) &&
            (MOB_FLAGGED(k->follower, MOB_ELEMENTAL))) {
      return TRUE;
    }
  }

  return FALSE;
}

/* check if ch has an undead follower or not */
bool has_undead_follower(struct char_data *ch) {
  struct follow_type *k = NULL, *next = NULL;

  for (k = ch->followers; k; k = next) {
    next = k->next;
    if (IS_NPC(k->follower) && AFF_FLAGGED(k->follower, AFF_CHARM) &&
            (MOB_FLAGGED(k->follower, MOB_ANIMATED_DEAD))) {
      return TRUE;
    }
  }

  return FALSE;
}

/* this will calculate the arcana-golem race bonus caster level */
int compute_arcana_golem_level(struct char_data *ch) {
  if (!ch)
    return 0;

  if (IS_NPC(ch))
    return 0;

  if (GET_RACE(ch) != RACE_ARCANA_GOLEM)
    return 0;

  return ((int) (GET_LEVEL(ch) / 6));
}

/* function to check if a char (ranger) has given favored enemy (race) */
bool is_fav_enemy_of(struct char_data *ch, int race) {
  int i = 0;

  for (i = 0; i < MAX_ENEMIES; i++) {
    if (GET_FAVORED_ENEMY(ch, i) == race)
      return TRUE;
  }
  return FALSE;
}

/* returns a or an based on first character of next word */
char * a_or_an(char *string) {
  switch (tolower(*string)) {
    case 'a':
    case 'e':
    case 'i':
    case 'u':
    case 'o':
      return "an";
  }

  return "a";
}

/* function for sneak-check
 * ch = listener (challenge), vict = sneaker (DC)
 */
bool can_hear_sneaking(struct char_data *ch, struct char_data *vict) {
  /* free passes */
  if (!AFF_FLAGGED(vict, AFF_SNEAK))
    return TRUE;

  /* do listen check here */
  bool can_hear = FALSE, challenge = dice(1, 20), dc = (dice(1, 20) + 10);

  //challenger bonuses/penalty (ch)
  if (!IS_NPC(ch)) {
    challenge += compute_ability(ch, ABILITY_STEALTH);
  } else
    challenge += GET_LEVEL(ch);
  if (AFF_FLAGGED(ch, AFF_LISTEN))
    challenge += 10;

  //hider bonus/penalties (vict)
  if (!IS_NPC(vict)) {
    dc += compute_ability((struct char_data *) vict, ABILITY_STEALTH);

    if (IN_NATURE(vict) && HAS_FEAT(vict, FEAT_TRACKLESS_STEP)) {
      dc += 4;
    }
    if (IN_NATURE(vict) && HAS_FEAT(vict, FEAT_CAMOUFLAGE)) {
      dc += 6;
    }
  } else
    dc += GET_LEVEL(vict);
  dc += (GET_SIZE(ch) - GET_SIZE(vict)) * 2; //size bonus

  if (challenge > dc)
    can_hear = TRUE;

  return (can_hear);
}

/* function for hide-check
 * ch = spotter (challenge), vict = hider (DC)
 */
bool can_see_hidden(struct char_data *ch, struct char_data *vict) {
  /* free passes */
  if (!AFF_FLAGGED(vict, AFF_HIDE) || AFF_FLAGGED(ch, AFF_TRUE_SIGHT))
    return TRUE;

  /* do spot check here */
  bool can_see = FALSE, challenge = dice(1, 20), dc = (dice(1, 20) + 10);

  //challenger bonuses/penalty (ch)
  if (!IS_NPC(ch))
    challenge += compute_ability(ch, ABILITY_PERCEPTION);
  else
    challenge += GET_LEVEL(ch);
  if (AFF_FLAGGED(ch, AFF_SPOT))
    challenge += 10;

  //hider bonus/penalties (vict)
  if (!IS_NPC(vict)) {
    dc += compute_ability((struct char_data *) vict, ABILITY_STEALTH);

    if (IN_NATURE(vict) && HAS_FEAT(vict, FEAT_TRACKLESS_STEP)) {
      dc += 4;
    }
    if (IN_NATURE(vict) && HAS_FEAT(vict, FEAT_CAMOUFLAGE)) {
      dc += 6;
    }
  } else
    dc += GET_LEVEL(vict);
  dc += (GET_SIZE(ch) - GET_SIZE(vict)) * 2; //size bonus

  if (challenge > dc)
    can_see = TRUE;

  return (can_see);
}

/* function to calculate a skill roll for given ch */
int skill_roll(struct char_data *ch, int skillnum) {
  int roll = dice(1, 20);

  /*if (PRF_FLAGGED(ch, PRF_TAKE_TEN))
    roll = 10;*/

  roll += compute_ability(ch, skillnum);

  return roll;
}

/* function to perform a skill check */
int skill_check(struct char_data *ch, int skill, int dc) {
  int result = skill_roll(ch, skill);

  if (result == dc) /* woo barely passed! */
    return 1;
  else if (result < dc) /*failed*/
    return 0;
  else
    return (result - dc);
}


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

void increase_skill(struct char_data *ch, int skillnum) {
  int notched = FALSE;

  //if the skill isn't learned or is mastered, don't adjust
  if (GET_SKILL(ch, skillnum) < 1 || GET_SKILL(ch, skillnum) == 99)
    return;
  if (GET_SKILL(ch, skillnum) > 99) {
    GET_SKILL(ch, skillnum) = 99;
    return;
  }

  int use = rand_number(0, USE);
  int pass = rand_number(0, PASS);
  int craft = rand_number(0, C_SKILL);
  int slow_craft = rand_number(0, C_SKILL_SLOW);

  switch (skillnum) {
    case SKILL_BACKSTAB:
      if (!use) {
        notched = TRUE;
        GET_SKILL(ch, skillnum)++;
      }
      break;
    case SKILL_BASH:
      if (!use) {
        notched = TRUE;
        GET_SKILL(ch, skillnum)++;
      }
      break;
    case SKILL_MUMMY_DUST:
      if (!use) {
        notched = TRUE;
        GET_SKILL(ch, skillnum)++;
      }
      break;
    case SKILL_KICK:
      if (!use) {
        notched = TRUE;
        GET_SKILL(ch, skillnum)++;
      }
      break;
    case SKILL_WEAPON_SPECIALIST:
      if (!pass) {
        notched = TRUE;
        GET_SKILL(ch, skillnum)++;
      }
      break;
    case SKILL_WHIRLWIND:
      if (!use) {
        notched = TRUE;
        GET_SKILL(ch, skillnum)++;
      }
      break;
    case SKILL_RESCUE:
      if (!use) {
        notched = TRUE;
        GET_SKILL(ch, skillnum)++;
      }
      break;
    case SKILL_DRAGON_KNIGHT:
      if (!use) {
        notched = TRUE;
        GET_SKILL(ch, skillnum)++;
      }
      break;
    case SKILL_LUCK_OF_HEROES:
      if (!pass) {
        notched = TRUE;
        GET_SKILL(ch, skillnum)++;
      }
      break;
    case SKILL_TRACK:
      if (!use) {
        notched = TRUE;
        GET_SKILL(ch, skillnum)++;
      }
      break;
    case SKILL_QUICK_CHANT:
      if (!use) {
        notched = TRUE;
        GET_SKILL(ch, skillnum)++;
      }
      break;
    case SKILL_AMBIDEXTERITY:
      if (!pass) {
        notched = TRUE;
        GET_SKILL(ch, skillnum)++;
      }
      break;
    case SKILL_DIRTY_FIGHTING:
      if (!pass) {
        notched = TRUE;
        GET_SKILL(ch, skillnum)++;
      }
      break;
    case SKILL_DODGE:
      if (!pass) {
        notched = TRUE;
        GET_SKILL(ch, skillnum)++;
      }
      break;
    case SKILL_IMPROVED_CRITICAL:
      if (!pass) {
        notched = TRUE;
        GET_SKILL(ch, skillnum)++;
      }
      break;
    case SKILL_MOBILITY:
      if (!pass) {
        notched = TRUE;
        GET_SKILL(ch, skillnum)++;
      }
      break;
    case SKILL_SPRING_ATTACK:
      if (!pass) {
        notched = TRUE;
        GET_SKILL(ch, skillnum)++;
      }
      break;
    case SKILL_TOUGHNESS:
      if (!pass) {
        notched = TRUE;
        GET_SKILL(ch, skillnum)++;
      }
      break;
    case SKILL_TWO_WEAPON_FIGHT:
      if (!pass) {
        notched = TRUE;
        GET_SKILL(ch, skillnum)++;
      }
      break;
    case SKILL_FINESSE:
      if (!pass) {
        notched = TRUE;
        GET_SKILL(ch, skillnum)++;
      }
      break;
    case SKILL_ARMOR_SKIN:
      if (!pass) {
        notched = TRUE;
        GET_SKILL(ch, skillnum)++;
      }
      break;
    case SKILL_BLINDING_SPEED:
      if (!pass) {
        notched = TRUE;
        GET_SKILL(ch, skillnum)++;
      }
      break;
    case SKILL_DAMAGE_REDUC_1:
      if (!pass) {
        notched = TRUE;
        GET_SKILL(ch, skillnum)++;
      }
      break;
    case SKILL_DAMAGE_REDUC_2:
      if (!pass) {
        notched = TRUE;
        GET_SKILL(ch, skillnum)++;
      }
      break;
    case SKILL_DAMAGE_REDUC_3:
      if (!pass) {
        notched = TRUE;
        GET_SKILL(ch, skillnum)++;
      }
      break;
    case SKILL_EPIC_TOUGHNESS:
      if (!pass) {
        notched = TRUE;
        GET_SKILL(ch, skillnum)++;
      }
      break;
    case SKILL_OVERWHELMING_CRIT:
      if (!pass) {
        notched = TRUE;
        GET_SKILL(ch, skillnum)++;
      }
      break;
    case SKILL_SELF_CONCEAL_1:
      if (!pass) {
        notched = TRUE;
        GET_SKILL(ch, skillnum)++;
      }
      break;
    case SKILL_SELF_CONCEAL_2:
      if (!pass) {
        notched = TRUE;
        GET_SKILL(ch, skillnum)++;
      }
      break;
    case SKILL_SELF_CONCEAL_3:
      if (!pass) {
        notched = TRUE;
        GET_SKILL(ch, skillnum)++;
      }
      break;
    case SKILL_TRIP:
      if (!use) {
        notched = TRUE;
        GET_SKILL(ch, skillnum)++;
      }
      break;
    case SKILL_IMPROVED_WHIRL:
      if (!use) {
        notched = TRUE;
        GET_SKILL(ch, skillnum)++;
      }
      break;
    case SKILL_CLEAVE:
      if (!pass) {
        notched = TRUE;
        GET_SKILL(ch, skillnum)++;
      }
      break;
    case SKILL_GREAT_CLEAVE:
      if (!pass) {
        notched = TRUE;
        GET_SKILL(ch, skillnum)++;
      }
      break;
    case SKILL_SPELLPENETRATE:
      if (!pass) {
        notched = TRUE;
        GET_SKILL(ch, skillnum)++;
      }
      break;
    case SKILL_SPELLPENETRATE_2:
      if (!pass) {
        notched = TRUE;
        GET_SKILL(ch, skillnum)++;
      }
      break;
    case SKILL_PROWESS:
      if (!pass) {
        notched = TRUE;
        GET_SKILL(ch, skillnum)++;
      }
      break;
    case SKILL_EPIC_PROWESS:
      if (!pass) {
        notched = TRUE;
        GET_SKILL(ch, skillnum)++;
      }
      break;
    case SKILL_EPIC_2_WEAPON:
      if (!pass) {
        notched = TRUE;
        GET_SKILL(ch, skillnum)++;
      }
      break;
    case SKILL_SPELLPENETRATE_3:
      if (!pass) {
        notched = TRUE;
        GET_SKILL(ch, skillnum)++;
      }
      break;
    case SKILL_SPELL_RESIST_1:
      if (!pass) {
        notched = TRUE;
        GET_SKILL(ch, skillnum)++;
      }
      break;
    case SKILL_SPELL_RESIST_2:
      if (!pass) {
        notched = TRUE;
        GET_SKILL(ch, skillnum)++;
      }
      break;
    case SKILL_SPELL_RESIST_3:
      if (!pass) {
        notched = TRUE;
        GET_SKILL(ch, skillnum)++;
      }
      break;
    case SKILL_SPELL_RESIST_4:
      if (!pass) {
        notched = TRUE;
        GET_SKILL(ch, skillnum)++;
      }
      break;
    case SKILL_SPELL_RESIST_5:
      if (!pass) {
        notched = TRUE;
        GET_SKILL(ch, skillnum)++;
      }
      break;
    case SKILL_INITIATIVE:
      if (!pass) {
        notched = TRUE;
        GET_SKILL(ch, skillnum)++;
      }
      break;
    case SKILL_EPIC_CRIT:
      if (!pass) {
        notched = TRUE;
        GET_SKILL(ch, skillnum)++;
      }
      break;
    case SKILL_IMPROVED_BASH:
      if (!use) {
        notched = TRUE;
        GET_SKILL(ch, skillnum)++;
      }
      break;
    case SKILL_IMPROVED_TRIP:
      if (!use) {
        notched = TRUE;
        GET_SKILL(ch, skillnum)++;
      }
      break;
    case SKILL_POWER_ATTACK:
      if (!pass) {
        notched = TRUE;
        GET_SKILL(ch, skillnum)++;
      }
      break;
    case SKILL_EXPERTISE:
      if (!pass) {
        notched = TRUE;
        GET_SKILL(ch, skillnum)++;
      }
      break;
    case SKILL_GREATER_RUIN:
      if (!use) {
        notched = TRUE;
        GET_SKILL(ch, skillnum)++;
      }
      break;
    case SKILL_HELLBALL:
      if (!use) {
        notched = TRUE;
        GET_SKILL(ch, skillnum)++;
      }
      break;
    case SKILL_EPIC_MAGE_ARMOR:
      if (!use) {
        notched = TRUE;
        GET_SKILL(ch, skillnum)++;
      }
      break;
    case SKILL_EPIC_WARDING:
      if (!use) {
        notched = TRUE;
        GET_SKILL(ch, skillnum)++;
      }
      break;
    case SKILL_CRIPPLING_CRITICAL:
      if (!use) {
        notched = TRUE;
        GET_SKILL(ch, skillnum)++;
      }
      break;
    case SKILL_RAGE:
      if (!use) {
        notched = TRUE;
        GET_SKILL(ch, skillnum)++;
      }
      break;
    case SKILL_DEFENSIVE_STANCE:
      if (!use) {
        notched = TRUE;
        GET_SKILL(ch, skillnum)++;
      }
      break;
    case SKILL_PROF_MINIMAL:
      if (!pass) {
        notched = TRUE;
        GET_SKILL(ch, skillnum)++;
      }
      break;
    case SKILL_PROF_BASIC:
      if (!pass) {
        notched = TRUE;
        GET_SKILL(ch, skillnum)++;
      }
      break;
    case SKILL_PROF_ADVANCED:
      if (!pass) {
        notched = TRUE;
        GET_SKILL(ch, skillnum)++;
      }
      break;
    case SKILL_PROF_MASTER:
      if (!pass) {
        notched = TRUE;
        GET_SKILL(ch, skillnum)++;
      }
      break;
    case SKILL_PROF_EXOTIC:
      if (!pass) {
        notched = TRUE;
        GET_SKILL(ch, skillnum)++;
      }
      break;
    case SKILL_PROF_LIGHT_A:
      if (!pass) {
        notched = TRUE;
        GET_SKILL(ch, skillnum)++;
      }
      break;
    case SKILL_PROF_MEDIUM_A:
      if (!pass) {
        notched = TRUE;
        GET_SKILL(ch, skillnum)++;
      }
      break;
    case SKILL_PROF_HEAVY_A:
      if (!pass) {
        notched = TRUE;
        GET_SKILL(ch, skillnum)++;
      }
      break;
    case SKILL_PROF_SHIELDS:
      if (!pass) {
        notched = TRUE;
        GET_SKILL(ch, skillnum)++;
      }
      break;
    case SKILL_PROF_T_SHIELDS:
      if (!pass) {
        notched = TRUE;
        GET_SKILL(ch, skillnum)++;
      }
      break;
    case SKILL_MURMUR:
      if (!use) {
        notched = TRUE;
        GET_SKILL(ch, skillnum)++;
      }
      break;
    case SKILL_PROPAGANDA:
      if (!use) {
        notched = TRUE;
        GET_SKILL(ch, skillnum)++;
      }
      break;
    case SKILL_LOBBY:
      if (!use) {
        notched = TRUE;
        GET_SKILL(ch, skillnum)++;
      }
      break;
    case SKILL_STUNNING_FIST:
      if (!use) {
        notched = TRUE;
        GET_SKILL(ch, skillnum)++;
      }
      break;
    case SKILL_POWERFUL_BLOW:
      if (!use) {
        notched = TRUE;
        GET_SKILL(ch, skillnum)++;
      }
      break;
    case SKILL_SUPRISE_ACCURACY:
      if (!use) {
        notched = TRUE;
        GET_SKILL(ch, skillnum)++;
      }
      break;
    case SKILL_COME_AND_GET_ME:
      if (!use) {
        notched = TRUE;
        GET_SKILL(ch, skillnum)++;
      }
      break;
    case SKILL_FEINT:
      if (!use) {
        notched = TRUE;
        GET_SKILL(ch, skillnum)++;
      }
      break;

      /* crafting skills */
    case SKILL_MINING:
      if (!craft) {
        notched = TRUE;
        GET_SKILL(ch, skillnum)++;
      }
      break;
    case SKILL_HUNTING:
      if (!craft) {
        notched = TRUE;
        GET_SKILL(ch, skillnum)++;
      }
      break;
    case SKILL_FORESTING:
      if (!craft) {
        notched = TRUE;
        GET_SKILL(ch, skillnum)++;
      }
      break;
    case SKILL_KNITTING:
      if (!craft) {
        notched = TRUE;
        GET_SKILL(ch, skillnum)++;
      }
      break;
    case SKILL_CHEMISTRY:
      if (!craft) {
        notched = TRUE;
        GET_SKILL(ch, skillnum)++;
      }
      break;
    case SKILL_ARMOR_SMITHING:
      if (!craft) {
        notched = TRUE;
        GET_SKILL(ch, skillnum)++;
      }
      break;
    case SKILL_WEAPON_SMITHING:
      if (!craft) {
        notched = TRUE;
        GET_SKILL(ch, skillnum)++;
      }
      break;
    case SKILL_JEWELRY_MAKING:
      if (!craft) {
        notched = TRUE;
        GET_SKILL(ch, skillnum)++;
      }
      break;
    case SKILL_LEATHER_WORKING:
      if (!craft) {
        notched = TRUE;
        GET_SKILL(ch, skillnum)++;
      }
      break;
    case SKILL_FAST_CRAFTER:
      if (!slow_craft) {
        notched = TRUE;
        GET_SKILL(ch, skillnum)++;
      }
      break;
    case SKILL_BONE_ARMOR:
      if (!craft) {
        notched = TRUE;
        GET_SKILL(ch, skillnum)++;
      }
      break;
    case SKILL_ELVEN_CRAFTING:
      if (!craft) {
        notched = TRUE;
        GET_SKILL(ch, skillnum)++;
      }
      break;
    case SKILL_MASTERWORK_CRAFTING:
      if (!craft) {
        notched = TRUE;
        GET_SKILL(ch, skillnum)++;
      }
      break;
    case SKILL_DRACONIC_CRAFTING:
      if (!craft) {
        notched = TRUE;
        GET_SKILL(ch, skillnum)++;
      }
      break;
    case SKILL_DWARVEN_CRAFTING:
      if (!craft) {
        notched = TRUE;
        GET_SKILL(ch, skillnum)++;
      }
      break;
      /* end crafting */

    case SKILL_LIGHTNING_REFLEXES:
      if (!pass) {
        notched = TRUE;
        GET_SKILL(ch, skillnum)++;
      }
      break;
    case SKILL_GREAT_FORTITUDE:
      if (!pass) {
        notched = TRUE;
        GET_SKILL(ch, skillnum)++;
      }
      break;
    case SKILL_IRON_WILL:
      if (!pass) {
        notched = TRUE;
        GET_SKILL(ch, skillnum)++;
      }
      break;
    case SKILL_EPIC_REFLEXES:
      if (!pass) {
        notched = TRUE;
        GET_SKILL(ch, skillnum)++;
      }
      break;
    case SKILL_EPIC_FORTITUDE:
      if (!pass) {
        notched = TRUE;
        GET_SKILL(ch, skillnum)++;
      }
      break;
    case SKILL_EPIC_WILL:
      if (!pass) {
        notched = TRUE;
        GET_SKILL(ch, skillnum)++;
      }
      break;
    case SKILL_SHIELD_SPECIALIST:
      if (!pass) {
        notched = TRUE;
        GET_SKILL(ch, skillnum)++;
      }
      break;
    default:
      if (!pass) {
        notched = TRUE;
        GET_SKILL(ch, skillnum)++;
      }
      return;
      ;
  }

  if (notched)
    send_to_char(ch, "\tMYou feel your skill in \tC%s\tM improve! Your skill at "
          "\tC%s\tM is now %d!\tn", spell_info[skillnum].name, spell_info[skillnum].name,
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
int rand_number(int from, int to) {
  /* error checking in case people call this incorrectly */
  if (from > to) {
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
float rand_float(float from, float to) {
  float ret;
  /* error checking in case people call this incorrectly */
  if (from > to) {
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
int dice(int num, int size) {
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
int MIN(int a, int b) {
  return (a < b ? a : b);
}

float FLOATMIN(float a, float b) {
  return (a < b ? a : b);
}

/** Return the larger number. Original note: Be wary of sign issues with this.
 * @param a The first number.
 * @param b The second number.
 * @retval int The larger of the two, a or b. */
int MAX(int a, int b) {
  return (a > b ? a : b);
}

float FLOATMAX(float a, float b) {
  return (a > b ? a : b);
}

/** Used to capitalize a string. Will not change any mud specific color codes.
 * @param txt The string to capitalize.
 * @retval char* Pointer to the capitalized string. */
char *CAP(char *txt) {
  char *p = txt;

  /* Skip all preceeding color codes and ANSI codes */
  while ((*p == '\t' && *(p + 1)) || (*p == '\x1B' && *(p + 1) == '[')) {
    if (*p == '\t') p += 2; /* Skip \t sign and color letter/number */
    else {
      p += 2; /* Skip the CSI section of the ANSI code */
      while (*p && !isalpha(*p)) p++; /* Skip until a 'letter' is found */
      if (*p) p++; /* Skip the letter */
    }
  }

  if (*p)
    *p = UPPER(*p);
  return (txt);
}

#if !defined(HAVE_STRLCPY)

/** A 'strlcpy' function in the same fashion as 'strdup' below. This copies up
 * to totalsize - 1 bytes from the source string, placing them and a trailing
 * NUL into the destination string. Returns the total length of the string it
 * tried to copy, not including the trailing NUL.  So a '>= totalsize' test
 * says it was truncated. (Note that you may have _expected_ truncation
 * because you only wanted a few characters from the source string.) Portable
 * function, in case your system does not have strlcpy. */
size_t strlcpy(char *dest, const char *source, size_t totalsize) {
  strncpy(dest, source, totalsize - 1); /* strncpy: OK (we must assume 'totalsize' is correct) */
  dest[totalsize - 1] = '\0';
  return strlen(source);
}
#endif

#if !defined(HAVE_STRDUP)

/** Create a duplicate of a string function. Portable. */
char *strdup(const char *source) {
  char *new_z;

  CREATE(new_z, char, strlen(source) + 1);
  return (strcpy(new_z, source)); /* strcpy: OK */
}
#endif

/** Strips "\r\n" from just the end of a string. Will not remove internal
 * "\r\n" values to the string.
 * @post Replaces any "\r\n" values at the end of the string with null.
 * @param txt The writable string to prune. */
void prune_crlf(char *txt) {
  int i = strlen(txt) - 1;

  while (txt[i] == '\n' || txt[i] == '\r')
    txt[i--] = '\0';
}

#ifndef str_cmp

/** a portable, case-insensitive version of strcmp(). Returns: 0 if equal, > 0
 * if arg1 > arg2, or < 0 if arg1 < arg2. Scan until strings are found
 * different or we reach the end of both. */
int str_cmp(const char *arg1, const char *arg2) {
  int chk, i;

  if (arg1 == NULL || arg2 == NULL) {
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
int strn_cmp(const char *arg1, const char *arg2, int n) {
  int chk, i;

  if (arg1 == NULL || arg2 == NULL) {
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
void basic_mud_vlog(const char *format, va_list args) {
  time_t ct = time(0);
  char *time_s = asctime(localtime(&ct));

  if (logfile == NULL) {
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
void basic_mud_log(const char *format, ...) {
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
int touch(const char *path) {
  FILE *fl;

  if (!(fl = fopen(path, "a"))) {
    log("SYSERR: %s: %s", path, strerror(errno));
    return (-1);
  } else {
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
void mudlog(int type, int level, int file, const char *str, ...) {
  char buf[MAX_STRING_LENGTH];
  struct descriptor_data *i;
  va_list args;

  if (str == NULL)
    return; /* eh, oh well. */

  if (file) {
    va_start(args, str);
    basic_mud_vlog(str, args);
    va_end(args);
  }

  if (level < 0)
    return;

  strcpy(buf, "[ "); /* strcpy: OK */
  va_start(args, str);
  vsnprintf(buf + 2, sizeof (buf) - 6, str, args);
  va_end(args);
  strcat(buf, " ]\tn\r\n"); /* strcat: OK */

  for (i = descriptor_list; i; i = i->next) {
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
size_t sprintbit(bitvector_t bitvector, const char *names[], char *result, size_t reslen) {
  size_t len = 0;
  int nlen;
  long nr;

  *result = '\0';

  for (nr = 0; bitvector && len < reslen; bitvector >>= 1) {
    if (IS_SET(bitvector, 1)) {
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
size_t sprinttype(int type, const char *names[], char *result, size_t reslen) {
  int nr = 0;

  while (type && *names[nr] != '\n') {
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
void sprintbitarray(int bitvector[], const char *names[], int maxar, char *result) {
  int nr, teller, found = FALSE, count = 0;

  *result = '\0';

  for (teller = 0; teller < maxar && !found; teller++) {
    for (nr = 0; nr < 32 && !found; nr++) {
      if (IS_SET_AR(bitvector, (teller * 32) + nr)) {
        if (*names[(teller * 32) + nr] != '\n') {
          if (*names[(teller * 32) + nr] != '\0') {
            strcat(result, names[(teller * 32) + nr]);
            strcat(result, " ");
            count++;
            if (count >= 8) {
              strcat(result, "\r\n");
              count = 0;
            }
          }
        } else {
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
struct time_info_data *real_time_passed(time_t t2, time_t t1) {
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
struct time_info_data *mud_time_passed(time_t t2, time_t t1) {
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
time_t mud_time_to_secs(struct time_info_data *now) {
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
struct time_info_data *age(struct char_data *ch) {
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
bool circle_follow(struct char_data *ch, struct char_data *victim) {
  struct char_data *k;

  for (k = victim; k; k = k->master) {
    if (k == ch)
      return (TRUE);
  }

  return (FALSE);
}

/* had to create this to get the "meat" out of stop_follower() for application
   in other scenarios */
void stop_follower_engine(struct char_data *ch) {
  struct follow_type *k = NULL;
  struct follow_type *j = NULL;
  
  if (ch->master->followers->follower == ch) { /* Head of follower-list? */
    k = ch->master->followers;
    ch->master->followers = k->next;
    free(k);
  } else { /* locate follower who is not head of list */
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
void stop_follower(struct char_data *ch) {

  /* Makes sure this function is not called when it shouldn't be called. */
  if (ch->master == NULL) {
    core_dump();
    return;
  }

  if (AFF_FLAGGED(ch, AFF_CHARM)) {
    act("You realize that you do not need to serve $N anymore!",
            FALSE, ch, 0, ch->master, TO_CHAR);
    act("$n is no longer serving you!", FALSE, ch, 0, ch->master, TO_VICT);
    
    /* hope i got everything - zusuk */
    if (affected_by_spell(ch, SPELL_CHARM))
      affect_from_char(ch, SPELL_CHARM);
    if (affected_by_spell(ch, SPELL_CHARM_ANIMAL))
      affect_from_char(ch, SPELL_CHARM_ANIMAL);
    if (affected_by_spell(ch, SPELL_DOMINATE_PERSON))
      affect_from_char(ch, SPELL_DOMINATE_PERSON);
    if (affected_by_spell(ch, SPELL_MASS_DOMINATION))
      affect_from_char(ch, SPELL_MASS_DOMINATION);
    
    if (GROUP(ch)) {
      leave_group(ch);            
    }    
    
  } else {
    act("You stop following $N.", FALSE, ch, 0, ch->master, TO_CHAR);
    act("$n stops following $N.", TRUE, ch, 0, ch->master, TO_NOTVICT);
    act("$n stops following you.", TRUE, ch, 0, ch->master, TO_VICT);
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
int num_followers_charmed(struct char_data *ch) {
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
void die_follower(struct char_data *ch) {
  struct follow_type *j, *k;

  if (ch->master)
    stop_follower(ch);

  for (k = ch->followers; k; k = j) {
    j = k->next;
    stop_follower(k->follower);
  }
}

/** Adds a new follower to a group.
 * @todo Maybe make circle_follow an inherent part of this function?
 * @pre Make sure to call circle_follow first. ch may also not already
 * be following anyone, otherwise core dump.
 * @param ch The character to follow.
 * @param leader The character to be followed. */
void add_follower(struct char_data *ch, struct char_data *leader) {
  struct follow_type *k;

  if (ch->master) {
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
int get_line(FILE *fl, char *buf) {
  char temp [READ_SIZE];
  int lines = 0;
  int sl;

  do {
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
int get_filename(char *filename, size_t fbufsize, int mode, const char *orig_name) {
  const char *prefix, *middle, *suffix;
  char name[PATH_MAX], *ptr;

  if (orig_name == NULL || *orig_name == '\0' || filename == NULL) {
    log("SYSERR: NULL pointer or empty string passed to get_filename(), %p or %p.",
            orig_name, filename);
    return (0);
  }

  switch (mode) {
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

  strlcpy(name, orig_name, sizeof (name));
  for (ptr = name; *ptr; ptr++)
    *ptr = LOWER(*ptr);

  switch (LOWER(*name)) {
    case 'a': case 'b': case 'c': case 'd': case 'e':
      middle = "A-E";
      break;
    case 'f': case 'g': case 'h': case 'i': case 'j':
      middle = "F-J";
      break;
    case 'k': case 'l': case 'm': case 'n': case 'o':
      middle = "K-O";
      break;
    case 'p': case 'q': case 'r': case 's': case 't':
      middle = "P-T";
      break;
    case 'u': case 'v': case 'w': case 'x': case 'y': case 'z':
      middle = "U-Z";
      break;
    default:
      middle = "ZZZ";
      break;
  }

  snprintf(filename, fbufsize, "%s%s"SLASH"%s.%s", prefix, middle, name, suffix);
  return (1);
}

/** Calculate the number of player characters (PCs) in the room. Any NPC (mob)
 * is not returned in the count.
 * @param room The room to check for PCs. */
int num_pc_in_room(struct room_data *room) {
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
void core_dump_real(const char *who, int line) {
  log("SYSERR: Assertion failed at %s:%d!", who, line);

#if 1	/* By default, let's not litter. */
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
int count_color_chars(char *string) {
  int i, len;
  int num = 0;

  if (!string || !*string)
    return 0;

  len = strlen(string);
  for (i = 0; i < len; i++) {
    while (string[i] == '\t') {
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
int count_non_protocol_chars(char * str) {
  int count = 0;
  char *string = str;

  while (*string) {
    if (*string == '\r' || *string == '\n') {
      string++;
      continue;
    }
    if (*string == '@' || *string == '\t') {
      string++;
      if (*string != '[' && *string != '<' && *string != '>' && *string != '(' && *string != ')')
        string++;
      else if (*string == '[') {
        while (*string && *string != ']')
          string++;
        string++;
      } else
        string++;
      continue;
    }
    count++;
    string++;
  }

  return count;
}

/* simple test to check if the given ch has infravision */
bool char_has_infra(struct char_data *ch) {
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
bool char_has_ultra(struct char_data *ch) {
  if (AFF_FLAGGED(ch, AFF_ULTRAVISION))
    return TRUE;

  if (HAS_FEAT(ch, FEAT_ULTRAVISION))
    return TRUE;

  if (GET_RACE(ch) == RACE_HALF_TROLL)
    return TRUE;
  if (GET_RACE(ch) == RACE_H_ORC)
    return TRUE;
  if (GET_RACE(ch) == RACE_DROW)
    return TRUE;
  if (GET_RACE(ch) == RACE_TRELUX)
    return TRUE;

  return FALSE;
}

/* test to see if a given room is "daylit", useful for dayblind races, etc
 rules detailed in code comments -zusuk */
bool room_is_daylit(room_rnum room) {
  if (!VALID_ROOM_RNUM(room)) {
    log("room_is_daylit: Invalid room rnum %d. (0-%d)", room, top_of_world);
    return (FALSE);
  }

  /* disqualifiers */
  /* sectors */
  if (SECT(room) == SECT_INSIDE)
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
bool room_is_dark(room_rnum room) {
  if (!VALID_ROOM_RNUM(room)) {
    log("room_is_dark: Invalid room rnum %d. (0-%d)", room, top_of_world);
    return (FALSE);
  }

  if (SECT(room) == SECT_INSIDE || SECT(room) == SECT_CITY)
    return (FALSE);

  if (ROOM_FLAGGED(room, ROOM_MAGICLIGHT))
    return (FALSE);

  if (ROOM_AFFECTED(room, RAFF_LIGHT))
    return (FALSE);

  /* magic-dark flag will over-ride lights */
  if (ROOM_FLAGGED(room, ROOM_MAGICDARK))
    return (TRUE);
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

/** Calculates the Levenshtein distance between two strings. Currently used
 * by the mud to make suggestions to the player when commands are mistyped.
 * This function is most useful when an index of possible choices are available
 * and the results of this function are constrained and used to help narrow
 * down the possible choices. For more information about Levenshtein distance,
 * recommend doing an internet or wikipedia search.
 * @param s1 The input string.
 * @param s2 The string to be compared to.
 * @retval int The Levenshtein distance between s1 and s2. */
int levenshtein_distance(const char *s1, const char *s2) {
  int **d, i, j;
  int s1_len = strlen(s1), s2_len = strlen(s2);

  CREATE(d, int *, s1_len + 1);

  for (i = 0; i <= s1_len; i++) {
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
void char_from_furniture(struct char_data *ch) {
  struct obj_data *furniture;
  struct char_data *tempch;

  if (!SITTING(ch))
    return;

  if (!(furniture = SITTING(ch))) {
    log("SYSERR: No furniture for char in char_from_furniture.");
    SITTING(ch) = NULL;
    NEXT_SITTING(ch) = NULL;
    return;
  }

  if (!(tempch = OBJ_SAT_IN_BY(furniture))) {
    log("SYSERR: Char from furniture, but no furniture!");
    SITTING(ch) = NULL;
    NEXT_SITTING(ch) = NULL;
    GET_OBJ_VAL(furniture, 1) = 0;
    return;
  }

  if (tempch == ch) {
    if (!NEXT_SITTING(ch)) {
      OBJ_SAT_IN_BY(furniture) = NULL;
    } else {
      OBJ_SAT_IN_BY(furniture) = NEXT_SITTING(ch);
    }
  } else {
    for (tempch = OBJ_SAT_IN_BY(furniture); tempch; tempch = NEXT_SITTING(tempch)) {
      if (NEXT_SITTING(tempch) == ch) {
        NEXT_SITTING(tempch) = NEXT_SITTING(ch);
      }
    }
  }
  GET_OBJ_VAL(furniture, 1) -= 1;
  SITTING(ch) = NULL;
  NEXT_SITTING(ch) = NULL;


  if (GET_OBJ_VAL(furniture, 1) < 1) {
    OBJ_SAT_IN_BY(furniture) = NULL;
    GET_OBJ_VAL(furniture, 1) = 0;
  }

  return;
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
void column_list(struct char_data *ch, int num_cols, const char **list, int list_length, bool show_nums) {
  int num_per_col, col_width, r, c, i, offset = 0, len = 0, temp_len, max_len = 0;
  char buf[MAX_STRING_LENGTH];

  /* Work out the longest list item */
  for (i = 0; i < list_length; i++)
    if (max_len < strlen(list[i]))
      max_len = strlen(list[i]);

  /* auto columns case */
  if (num_cols == 0) {
    num_cols = (IS_NPC(ch) ? 80 : GET_SCREEN_WIDTH(ch)) / (max_len + (show_nums ? 5 : 1));
  }

  /* Ensure that the number of columns is in the range 1-10 */
  num_cols = MIN(MAX(num_cols, 1), 10);

  /* Work out the longest list item */
  for (i = 0; i < list_length; i++)
    if (max_len < strlen(list[i]))
      max_len = strlen(list[i]);

  /* Calculate the width of each column */
  if (IS_NPC(ch)) col_width = 80 / num_cols;
  else col_width = (GET_SCREEN_WIDTH(ch)) / num_cols;

  if (show_nums) col_width -= 4;

  if (col_width < max_len)
    log("Warning: columns too narrow for correct output to %s in simple_column_list (utils.c)", GET_NAME(ch));

  /* Calculate how many list items there should be per column */
  num_per_col = (list_length / num_cols) + ((list_length % num_cols) ? 1 : 0);

  /* Fill 'buf' with the columnised list */
  for (r = 0; r < num_per_col; r++) {
    for (c = 0; c < num_cols; c++) {
      offset = (c * num_per_col) + r;
      if (offset < list_length) {
        if (show_nums)
          temp_len = snprintf(buf + len, sizeof (buf) - len, "%2d) %-*s", offset + 1, col_width, list[(offset)]);
        else
          temp_len = snprintf(buf + len, sizeof (buf) - len, "%-*s", col_width, list[(offset)]);
        len += temp_len;
      }
    }
    temp_len = snprintf(buf + len, sizeof (buf) - len, "\r\n");
    len += temp_len;
  }

  if (len >= sizeof (buf))
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
int get_flag_by_name(const char *flag_list[], char *flag_name) {
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
int file_head(FILE *file, char *buf, size_t bufsize, int lines_to_read) {
  /* Local variables */
  int lines_read = 0; /* The number of lines read so far. */
  char line[READ_SIZE]; /* Retrieval buffer for file. */
  size_t buflen; /* Amount of previous existing data in buffer. */
  int readstatus = 1; /* Are we at the end of the file? */
  int n = 0; /* Return value from snprintf. */
  const char *overflow = "\r\n**OVERFLOW**\r\n"; /* Appended if overflow. */

  /* Quick check for bad arguments. */
  if (lines_to_read <= 0) {
    return lines_to_read;
  }

  /* Initialize local variables not already initialized. */
  buflen = strlen(buf);

  /* Read from the front of the file. */
  rewind(file);

  while ((lines_read < lines_to_read) &&
          (readstatus > 0) && (buflen < bufsize)) {
    /* Don't use get_line to set lines_read because get_line will return
     * the number of comments skipped during reading. */
    readstatus = get_line(file, line);

    if (readstatus > 0) {
      n = snprintf(buf + buflen, bufsize - buflen, "%s\r\n", line);
      buflen += n;
      lines_read++;
    }
  }

  /* Check to see if we had a potential buffer overflow. */
  if (buflen >= bufsize) {
    /* We should never see this case, but... */
    if ((strlen(overflow) + 1) >= bufsize) {
      core_dump();
      snprintf(buf, bufsize, "%s", overflow);
    } else {
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
int file_tail(FILE *file, char *buf, size_t bufsize, int lines_to_read) {
  /* Local variables */
  int lines_read = 0; /* The number of lines read so far. */
  int total_lines = 0; /* The total number of lines in the file. */
  char c; /* Used to fast forward the file. */
  char line[READ_SIZE]; /* Retrieval buffer for file. */
  size_t buflen; /* Amount of previous existing data in buffer. */
  int readstatus = 1; /* Are we at the end of the file? */
  int n = 0; /* Return value from snprintf. */
  const char *overflow = "\r\n**OVERFLOW**\r\n"; /* Appended if overflow. */

  /* Quick check for bad arguments. */
  if (lines_to_read <= 0) {
    return lines_to_read;
  }

  /* Initialize local variables not already initialized. */
  buflen = strlen(buf);
  total_lines = file_numlines(file); /* Side effect: file is rewound. */

  /* Fast forward to the location we should start reading from */
  while (((lines_to_read + lines_read) < total_lines)) {
    do {
      c = fgetc(file);
    } while (c != '\n');

    lines_read++;
  }

  /* We reuse the lines_read counter. */
  lines_read = 0;

  /** From here on, we perform just like file_head */
  while ((lines_read < lines_to_read) &&
          (readstatus > 0) && (buflen < bufsize)) {
    /* Don't use get_line to set lines_read because get_line will return
     * the number of comments skipped during reading. */
    readstatus = get_line(file, line);

    if (readstatus > 0) {
      n = snprintf(buf + buflen, bufsize - buflen, "%s\r\n", line);
      buflen += n;
      lines_read++;
    }
  }

  /* Check to see if we had a potential buffer overflow. */
  if (buflen >= bufsize) {
    /* We should never see this case, but... */
    if ((strlen(overflow) + 1) >= bufsize) {
      core_dump();
      snprintf(buf, bufsize, "%s", overflow);
    } else {
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
size_t file_sizeof(FILE *file) {
  size_t numbytes = 0;

  rewind(file);

  /* It would be so much easier to do a byte count if an fseek SEEK_END and
   * ftell pair of calls was portable for text files, but all information
   * I've found says that getting a file size from ftell for text files is
   * not portable. Oh well, this method should be extremely fast for the
   * relatively small filesizes in the mud, and portable, too. */
  while (!feof(file)) {
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
int file_numlines(FILE *file) {
  int numlines = 0;
  char c;

  rewind(file);

  while (!feof(file)) {
    c = fgetc(file);
    if (c == '\n') {
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
IDXTYPE atoidx(const char *str_to_conv) {
  long int result;

  /* Check for errors */
  errno = 0;

  result = strtol(str_to_conv, NULL, 10);

  if (errno || (result > IDXTYPE_MAX) || (result < 0))
    return NOWHERE; /* All of the NO* settings should be the same */
  else
    return (IDXTYPE) result;
}

/*
   strfrmt (String Format) function
   Used by automap/map system
   Re-formats a string to fit within a particular size box.
   Recognises @ color codes, and if a line ends in one color, the
   next line will start with the same color.
   Ends every line with \tn to prevent color bleeds.
 */
char *strfrmt(char *str, int w, int h, int justify, int hpad, int vpad) {
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
  while (*sp) {
    /* eat leading space */
    while (*sp && isspace_ignoretabs(*sp)) sp++;
    /* word begins */
    wp = sp;
    wlen = 0;
    while (*sp) { /* Find the end of the word */
      if (isspace_ignoretabs(*sp)) break;
      if (*sp == '\\' && sp[1] && sp[1] == '\\') {
        if (sp != wp)
          break; /* Finish dealing with the current word */
        sp += 2; /* Eat the marker and any trailing space */
        while (*sp && isspace_ignoretabs(*sp)) sp++;
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
      } else if (*sp == '`' || *sp == '$' || *sp == '#') {
        if (sp[1] && (sp[1] == *sp))
          wlen++; /* One printable char here */
        sp += 2; /* Eat the whole code regardless */
      } else if (*sp == '\t' && sp[1]) {
        char MXPcode = (sp[1] == '[' ? ']' : sp[1] == '<' ? '>' : '\0');

        if (!MXPcode)
          last_color = sp[1];

        sp += 2; /* Eat the code */
        if (MXPcode) {
          while (*sp != '\0' && *sp != MXPcode)
            ++sp; /* Eat the rest of the code */
        }
      } else {
        wlen++;
        sp++;
      }
    }
    if (llen + wlen + (lp == line ? 0 : 1) > w) {
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
      if (last_color != 'n') {
        *lp++ = '\t'; /* restore previous color */
        *lp++ = last_color;
        new_line_started = TRUE;
      }
    }
    /* add word to line */
    if (lp != line && new_line_started != TRUE) {
      *lp++ = ' ';
      llen++;
    }
    new_line_started = FALSE;
    llen += wlen;
    for (; wp != sp; *lp++ = *wp++);
  }
  /* Copy over the last line */
  if (lp != line) {
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
  if (vpad) {
    while (lcount < h) {
      if (hpad) {
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
char *strpaste(char *str1, char *str2, char *joiner) {
  static char ret[MAX_STRING_LENGTH + 1];
  char *sp1 = str1;
  char *sp2 = str2;
  char *rp = ret;
  int jlen = strlen(joiner);

  while ((rp - ret) < MAX_STRING_LENGTH && (*sp1 || *sp2)) {
    /* Copy line from str1 */
    while ((rp - ret) < MAX_STRING_LENGTH && *sp1 && !ISNEWL(*sp1))
      *rp++ = *sp1++;
    /* Eat the newline */
    if (*sp1) {
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
    if (*sp2) {
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
struct char_data *is_playing(char *vict_name) {
  extern struct descriptor_data *descriptor_list;
  struct descriptor_data *i, *next_i;
  char name_copy[MAX_NAME_LENGTH + 1];

  /* We need to CAP the name, so make a copy otherwise original is damaged */
  snprintf(name_copy, MAX_NAME_LENGTH + 1, "%s", vict_name);

  for (i = descriptor_list; i; i = next_i) {
    next_i = i->next;
    if (i->connected == CON_PLAYING && !strcmp(i->character->player.name, CAP(name_copy)))
      return i->character;
  }
  return NULL;
}

/* for displaying large numbers with commas */
char *add_commas(long num) {
  int i, j = 0, len;
  int negative = (num < 0);
  char num_string[MAX_INPUT_LENGTH];
  static char commastring[MAX_INPUT_LENGTH];

  sprintf(num_string, "%ld", num);
  len = strlen(num_string);

  for (i = 0; num_string[i]; i++) {
    if ((len - i) % 3 == 0 && i && i - negative)
      commastring[j++] = ',';
    commastring[j++] = num_string[i];
  }
  commastring[j] = '\0';

  return commastring;
}

/* Create a blank affect struct */
void new_affect(struct affected_type *af) {
  int i;
  af->spell = 0;
  af->duration = 0;
  af->modifier = 0;
  af->location = APPLY_NONE;
  af->bonus_type = BONUS_TYPE_UNDEFINED;

  for (i = 0; i < AF_ARRAY_MAX; i++) af->bitvector[i] = 0;
}

/* Free an affect struct */
void free_affect(struct affected_type *af) {
  //struct damage_reduction_type *dr;
  if (af == NULL) return;
  free(af);
}

/* Handy function to get class ID number by name (abbreviations allowed) */
int get_class_by_name(char *classname) {
  int i;

  for (i = 0; i < NUM_CLASSES; i++)
    if (is_abbrev(classname, CLSLIST_NAME(i)))
      return (i);

  return (-1);
}

/* Handy function to get race ID number by name (abbreviations allowed) */
int get_race_by_name(char *racename) {
  int i;

  for (i = 0; i < NUM_RACES; i++)
    if (is_abbrev(racename, race_list[i].type))
      return (i);

  return (-1);
}

/* Handy function to get subrace ID number by name (abbreviations allowed) */
int get_subrace_by_name(char *racename) {
  int i;

  for (i = 0; i < NUM_SUB_RACES; i++)
    if (is_abbrev(racename, npc_subrace_types[i]))
      return (i);

  return (-1);
}

/* parse tabs function */
char *convert_from_tabs(char * string) {
  static char buf[MAX_STRING_LENGTH * 8];

  strcpy(buf, string);
  parse_tab(buf);
  return (buf);
}

/* this function takes old-system alignment and converts it to new */
int convert_alignment(int align) {
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

void set_alignment(struct char_data *ch, int alignment) {
  switch (alignment) {
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
char *get_align_by_num_cnd(int align) {
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
char *get_align_by_num(int align) {
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
int get_feat_value(struct char_data *ch, int featnum) {
  struct obj_data *obj;
  int i = 0, j = 0;
  int featval = 0;

  if ((featnum <= FEAT_UNDEFINED) || (featnum >= FEAT_LAST_FEAT)) {
    log("SYSERR: get_feat_value called with invalid featnum: %d", featnum);
    return 0;
  }

  /* Check for the feat. */
  if (IS_NPC(ch))
    featval = MOB_HAS_FEAT(ch, featnum);
  else if (AFF_FLAGGED(ch, AFF_WILD_SHAPE) && GET_DISGUISE_RACE(ch))
    featval = MOB_HAS_FEAT(ch, featnum);
  else {
    /* check if we got this feat equipped */
    for (j = 0; j < NUM_WEARS; j++) {
      if ((obj = GET_EQ(ch, j)) == NULL)
        continue;
      for (i = 0; i < MAX_OBJ_AFFECT; i++) {
        if (obj->affected[i].location == APPLY_FEAT && obj->affected[i].modifier == featnum) {
          featval++;
          break; /* capped at +1, sorry folks */
        }
      }
    }
    featval += HAS_REAL_FEAT(ch, featnum);
  }

  return featval;
}

int find_armor_type(int specType) {

  switch (specType) {

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
int savingthrow(struct char_data *ch, int save, int modifier, int dc) {
  int roll = dice(1, 20);

  /* 1 is an automatic failure. */
  if (roll == 1)
    return FALSE;

  /* 20 is an automatic success. */
  if (roll == 20)
    return TRUE;

  roll += compute_mag_saves(ch, save, modifier);

  if (roll >= dc)
    return TRUE;
  else
    return FALSE;

}

/* Utilities for managing daily use abilities for players. */

int get_daily_uses(struct char_data *ch, int featnum) {
  int daily_uses = 0;

  switch (featnum) {
    case FEAT_STUNNING_FIST:
      daily_uses += CLASS_LEVEL(ch, CLASS_MONK) + (GET_LEVEL(ch) - CLASS_LEVEL(ch, CLASS_MONK)) / 4;
      break;
    case FEAT_LAYHANDS:
      daily_uses += CLASS_LEVEL(ch, CLASS_PALADIN) / 2 + GET_CHA_BONUS(ch);
      break;
    case FEAT_TURN_UNDEAD:
      daily_uses += 3 + GET_CHA_BONUS(ch) + HAS_FEAT(ch, FEAT_EXTRA_TURNING) * 2;
      break;
    case FEAT_BARDIC_MUSIC:
      daily_uses += CLASS_LEVEL(ch, CLASS_BARD);
      break;
    case FEAT_REMOVE_DISEASE:
      daily_uses += HAS_FEAT(ch, FEAT_REMOVE_DISEASE);
      break;
    case FEAT_CRYSTAL_FIST:
    case FEAT_CRYSTAL_BODY:
    case FEAT_SLA_FAERIE_FIRE:
    case FEAT_SLA_LEVITATE:
    case FEAT_SLA_DARKNESS:
      daily_uses = 3;
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
    case FEAT_SMITE_EVIL:
    case FEAT_SMITE_GOOD:
    case FEAT_RAGE:
    case FEAT_DEFENSIVE_STANCE:
    case FEAT_QUIVERING_PALM:
    case FEAT_ARROW_OF_DEATH:
    case FEAT_SWARM_OF_ARROWS:
    case FEAT_WILD_SHAPE:
    case FEAT_ANIMATE_DEAD:
    case FEAT_VANISH:
      daily_uses += HAS_FEAT(ch, featnum);
      break;
    case FEAT_DRACONIC_HERITAGE_BREATHWEAPON:
      if (CLASS_LEVEL(ch, CLASS_SORCERER) >= 9) daily_uses++;
      if (CLASS_LEVEL(ch, CLASS_SORCERER) >= 17) daily_uses++;
      if (CLASS_LEVEL(ch, CLASS_SORCERER) >= 20) daily_uses++;
      break;
    case FEAT_DRACONIC_HERITAGE_CLAWS:
      daily_uses += 3 + GET_CHA_BONUS(ch);
      break;
  }

  return daily_uses;
}

/* Function to create an event, based on the mud_event passed in, that will either:
 * 1.) Create a new event with the proper sVariables
 * 2.) Update an existing event with a new sVariable value
 *
 * Returns the current number of uses on cooldown. */
int start_daily_use_cooldown(struct char_data *ch, int featnum) {
  struct mud_event_data * pMudEvent = NULL;
  int uses = 0, daily_uses = 0;
  char buf[128];
  event_id iId = 0;

  /* Transform the feat number to the event id for that ability. */
  if ((iId = feat_list[featnum].event) == eNULL) {
    log("SYSERR: in start_daily_use_cooldown, no cooldown event defined for %s!", feat_list[featnum].name);
    return (0);
  }

  if ((daily_uses = get_daily_uses(ch, featnum)) < 1) {
    /* ch has no uses of this ability at all!  Error! */
    log("SYSERR: in start_daily_use_cooldown, cooldown initiated for invalid ability (featnum: %d)!", featnum);
    return (0);
  }

  if ((pMudEvent = char_has_mud_event(ch, iId))) {
    /* Player is on cooldown for this ability - just update the event. */
    /* The number of uses is stored in the pMudEvent->sVariables field. */
    if (pMudEvent->sVariables == NULL) {
      /* This is odd - This field should always be populated for daily-use abilities,
       * maybe some legacy code or bad id. */
      log("SYSERR: sVariables field is NULL for daily-use-cooldown-event: %d", iId);
    } else {

      if (sscanf(pMudEvent->sVariables, "uses:%d", &uses) != 1) {
        log("SYSERR: In start_daily_use_cooldown, bad sVariables for daily-use-cooldown-event: %d", iId);
        uses = 0;
      }
      free(pMudEvent->sVariables);
    }
    uses++;

    if (uses > daily_uses)
      log("SYSERR: Daily uses exceeed maximum for %s, feat %s", GET_NAME(ch), feat_list[featnum].name);

    sprintf(buf, "uses:%d", uses);
    pMudEvent->sVariables = strdup(buf);
  } else {
    /* No event - so attach one. */
    uses = 1;
    attach_mud_event(new_mud_event(iId, ch, "uses:1"), (SECS_PER_MUD_DAY / daily_uses) RL_SEC);
  }

  return uses;
}

/* Function to return the number of daily uses remaining for a particular ability.
 * Returns the number of daily uses available or -1 if the feat is not a daily-use feat. */
int daily_uses_remaining(struct char_data *ch, int featnum) {
  struct mud_event_data * pMudEvent = NULL;
  int uses = 0;
  int uses_per_day = 0;
  event_id iId = 0;

  if ((iId = feat_list[featnum].event) == eNULL)
    return -1;

  if ((pMudEvent = char_has_mud_event(ch, iId))) {
    if (pMudEvent->sVariables == NULL) {
      /* This is odd - This field should always be populated for daily-use abilities,
       * maybe some legacy code or bad id. */
      log("SYSERR: sVariables field is NULL for daily-use-cooldown-event: %d", iId);
    } else {
      if (sscanf(pMudEvent->sVariables, "uses:%d", &uses) != 1) {
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
int start_armor_specab_daily_use_cooldown(struct obj_data *obj, int specab) {
  struct mud_event_data * pMudEvent = NULL;
  int uses = 0, daily_uses = 0;
  char buf[128];
  event_id iId = 0;

  /* Transform the feat number to the event id for that ability. */
  if ((iId = special_ability_info[specab].event) == eNULL) {
    log("SYSERR: in start_daily_use_cooldown, no cooldown event defined for %s!", special_ability_info[specab].name);
    return (0);
  }

  if ((daily_uses = special_ability_info[specab].daily_uses) < 1) {
    /* ch has no uses of this ability at all!  Error! */
    log("SYSERR: in start_daily_use_cooldown, cooldown initiated for invalid ability (specab: %d)!", specab);
    return (0);
  }

  if ((pMudEvent = obj_has_mud_event(obj, iId))) {
    /* Player is on cooldown for this ability - just update the event. */
    /* The number of uses is stored in the pMudEvent->sVariables field. */
    if (pMudEvent->sVariables == NULL) {
      /* This is odd - This field should always be populated for daily-use abilities,
       * maybe some legacy code or bad id. */
      log("SYSERR: sVariables field is NULL for daily-use-cooldown-event: %d", iId);
    } else {

      if (sscanf(pMudEvent->sVariables, "uses:%d", &uses) != 1) {
        log("SYSERR: In start_daily_use_cooldown, bad sVariables for daily-use-cooldown-event: %d", iId);
        uses = 0;
      }
      free(pMudEvent->sVariables);
    }
    uses++;

    if (uses > daily_uses)
      log("SYSERR: Daily uses exceeed maximum for %s, specab %s", obj->name, special_ability_info[specab].name);

    sprintf(buf, "uses:%d", uses);
    pMudEvent->sVariables = strdup(buf);
  } else {
    /* No event - so attach one. */
    uses = 1;
    attach_mud_event(new_mud_event(iId, obj, "uses:1"), (SECS_PER_MUD_DAY / daily_uses) RL_SEC);
  }

  return uses;
}

/* Function to return the number of daily uses remaining for a particular armor special ability.
 * Returns the number of daily uses available or -1 if the object does not have that special ability. */
int daily_armor_specab_uses_remaining(struct obj_data *obj, int specab) {
  struct mud_event_data * pMudEvent = NULL;
  int uses = 0;
  int uses_per_day = 0;
  event_id iId = 0;

  if ((iId = special_ability_info[specab].event) == eNULL)
    return -1;

  if ((pMudEvent = obj_has_mud_event(obj, iId))) {
    if (pMudEvent->sVariables == NULL) {
      /* This is odd - This field should always be populated for daily-use abilities,
       * maybe some legacy code or bad id. */
      log("SYSERR: sVariables field is NULL for daily-use-cooldown-event: %d", iId);
    } else {
      if (sscanf(pMudEvent->sVariables, "uses:%d", &uses) != 1) {
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
char* line_string(int length, char first, char second) {
  static char buf[MAX_STRING_LENGTH]; /* Note - static! */
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

void draw_line(struct char_data *ch, int length, char first, char second) {
  send_to_char(ch, "%s", line_string(length, first, second));
}

/*  text_line_string()
 *  Generate and return a string, as above, with text centered.
 */
char* text_line_string(char *text, int length, char first, char second) {
  int text_length, text_print_length, pre_length;
  int i = 0, j = 0;
  static char buf[MAX_STRING_LENGTH]; /* Note - static! */

  text_length = strlen(text);
  text_print_length = count_non_protocol_chars(text);

  pre_length = (length - (text_print_length)) / 2; /* (length - (text length + '[  ]'))/2 */
  //  pre_length = (length - (text_length + 4))/2; /* (length - (text length + '[  ]'))/2 */
  //  pre_length = 2;

  while (i < pre_length) {
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

void text_line(struct char_data *ch, char *text, int length, char first, char second) {
  send_to_char(ch, "%s", text_line_string(text, length, first, second));
}

/* Name:   calculate_cp
 * Author: Ornir
 * Param:  Object to calculate cp for
 * Return: cp value for the given object. */
int calculate_cp(struct obj_data *obj) {
  int current_cp = 0;
  struct obj_special_ability *specab;

  if (!obj)
    return 0;

  switch (GET_OBJ_TYPE(obj)) {
    case ITEM_WEAPON: /* obj is a weapon, use the cp calc for weapons. */
      current_cp += GET_ENHANCEMENT_BONUS(obj) * GET_ENHANCEMENT_BONUS(obj) * 2000;

      for (specab = obj->special_abilities; specab != NULL; specab = specab->next) {
        current_cp += (special_ability_info[specab->ability].cost * special_ability_info[specab->ability].cost * 2000);
      }

      break;
    default: /* No idea what this is. */
      break;
  }

  return 0;
}

bool paralysis_immunity(struct char_data *ch) {
  if (!ch) return FALSE;
  if (HAS_FEAT(ch, FEAT_DRACONIC_HERITAGE_POWER_OF_WYRMS)) return TRUE;
  return FALSE;
}

bool sleep_immunity(struct char_data *ch) {
  if (!ch) return FALSE;
  if (HAS_FEAT(ch, FEAT_DRACONIC_HERITAGE_POWER_OF_WYRMS)) return TRUE;
  return FALSE;
}
