/**************************************************************************
 *  File: handler.c                                         Part of LuminariMUD *
 *  Usage: Internal funcs: moving and finding chars/objs.                  *
 *                                                                         *
 *  All rights reserved.  See license for complete information.            *
 *                                                                         *
 *  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
 *  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
 ************************************************************************ */

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "db.h"
#include "handler.h"
#include "screen.h"
#include "interpreter.h"
#include "spells.h"
#include "dg_scripts.h"
#include "act.h"
#include "class.h"
#include "fight.h"
#include "quest.h"
#include "mud_event.h"
#include "wilderness.h"
#include "actionqueues.h"
#include "constants.h"
#include "spec_abilities.h"


/* local file scope variables */
static int extractions_pending = 0;

/* local file scope functions */
static void update_object(struct obj_data *obj, int use);

/* find the first word in a string buffer */
char *fname(const char *namelist) {
  static char holder[READ_SIZE];
  char *point;

  for (point = holder; isalpha(*namelist); namelist++, point++)
    *point = *namelist;

  *point = '\0';

  return (holder);
}

/* Leave this here even if you put in a newer isname().  Used for OasisOLC. */
int is_name(const char *str, const char *namelist) {
  const char *curname, *curstr;

  if (!str || !namelist || !*str || !*namelist)
    return (0);

  curname = namelist;
  for (;;) {
    for (curstr = str;; curstr++, curname++) {
      if (!*curstr && !isalpha(*curname))
        return (1);

      if (!*curname)
        return (0);

      if (!*curstr || *curname == ' ')
        break;

      if (LOWER(*curstr) != LOWER(*curname))
        break;
    }

    /* skip to next name */
    for (; isalpha(*curname); curname++);
    if (!*curname)
      return (0);
    curname++; /* first char of new name */
  }
}

/* allow abbreviations */
#define WHITESPACE " \t"
#define KEYWORDJOIN "-"
int isname_tok(const char *str, const char *namelist) {
  char *newlist;
  char *curtok;
  char *saveptr = NULL;

  if (!str || !*str || !namelist || !*namelist)
    return 0;

  if (!strcmp(str, namelist)) /* the easy way */
    return 1;

  newlist = strdup(namelist); /* make a copy since strtok 'modifies' strings */
  for (curtok = strtok_r(newlist, WHITESPACE, &saveptr); curtok; curtok = strtok_r(NULL, WHITESPACE, &saveptr))
    if (curtok && is_abbrev(str, curtok)) {
      /* Don't allow abbreviated numbers. - Sryth */
      if (isdigit(*str) && (atoi(str) != atoi(curtok)))
        return 0;
      free(newlist);
      return 1;
    }
  free(newlist);
  return 0;
}

int isname (const char *str, const char *namelist)
{
  char *strlist = NULL;
  char *substr = NULL;
  char *saveptr = NULL;

  if (!str || !*str || !namelist || !*namelist)
    return 0;

  if (!strcmp (str, namelist))	/* the easy way */
    return 1;

    strlist = strdup(str);
    for (substr = strtok_r(strlist, KEYWORDJOIN, &saveptr); substr; substr = strtok_r(NULL, KEYWORDJOIN, &saveptr))
    {
        if (!substr) continue;
        if (!isname_tok(substr, namelist)) return 0;
    }
    free(strlist);
    /* If we didn't fail, assume we succeeded because every token was matched */
    return 1;
}



/* modify a character's given apply-type (loc) by value */
void aff_apply_modify(struct char_data *ch, byte loc, sbyte mod, char *msg) {

  switch (loc) {

    case APPLY_STR:
      (ch)->aff_abils.str += mod;
      break;
    case APPLY_DEX:
      (ch)->aff_abils.dex += mod;
      break;
    case APPLY_INT:
      GET_INT(ch) += mod;
      break;
    case APPLY_WIS:
      GET_WIS(ch) += mod;
      break;
    case APPLY_CON:
      (ch)->aff_abils.con += mod;
      break;
    case APPLY_CHA:
      GET_CHA(ch) += mod;
      break;

    case APPLY_CHAR_WEIGHT:
      GET_WEIGHT(ch) += mod;
      break;
    case APPLY_CHAR_HEIGHT:
      GET_HEIGHT(ch) += mod;
      break;

    case APPLY_PSP:
      GET_MAX_PSP(ch) += mod;
      break;
    case APPLY_HIT:
      GET_MAX_HIT(ch) += mod;
      break;
    case APPLY_MOVE:
      GET_MAX_MOVE(ch) += mod;
      break;

    case APPLY_AC:
      (ch)->points.armor += mod;
      break;
    case APPLY_AC_NEW: // new APPLY_AC for 3.5E armor class -Nashak
      (ch)->points.armor += mod * 10;
      break;

    case APPLY_HITROLL:
      GET_HITROLL(ch) += mod;
      break;
    case APPLY_DAMROLL:
      GET_DAMROLL(ch) += mod;
      break;

    case APPLY_SPELL_RES:
      GET_SPELL_RES(ch) += mod;
      break;

    case APPLY_SIZE:
      (ch)->points.size += mod;
      break;

    case APPLY_SAVING_FORT:
      GET_SAVE(ch, SAVING_FORT) += mod;
      break;
    case APPLY_SAVING_REFL:
      GET_SAVE(ch, SAVING_REFL) += mod;
      break;
    case APPLY_SAVING_WILL:
      GET_SAVE(ch, SAVING_WILL) += mod;
      break;
    case APPLY_SAVING_POISON:
      GET_SAVE(ch, SAVING_POISON) += mod;
      break;
    case APPLY_SAVING_DEATH:
      GET_SAVE(ch, SAVING_DEATH) += mod;
      break;

    case APPLY_RES_FIRE:
      GET_RESISTANCES(ch, DAM_FIRE) += mod;
      break;
    case APPLY_RES_COLD:
      GET_RESISTANCES(ch, DAM_COLD) += mod;
      break;
    case APPLY_RES_AIR:
      GET_RESISTANCES(ch, DAM_AIR) += mod;
      break;
    case APPLY_RES_EARTH:
      GET_RESISTANCES(ch, DAM_EARTH) += mod;
      break;
    case APPLY_RES_ACID:
      GET_RESISTANCES(ch, DAM_ACID) += mod;
      break;
    case APPLY_RES_HOLY:
      GET_RESISTANCES(ch, DAM_HOLY) += mod;
      break;
    case APPLY_RES_ELECTRIC:
      GET_RESISTANCES(ch, DAM_ELECTRIC) += mod;
      break;
    case APPLY_RES_UNHOLY:
      GET_RESISTANCES(ch, DAM_UNHOLY) += mod;
      break;
    case APPLY_RES_SLICE:
      GET_RESISTANCES(ch, DAM_SLICE) += mod;
      break;
    case APPLY_RES_PUNCTURE:
      GET_RESISTANCES(ch, DAM_PUNCTURE) += mod;
      break;
    case APPLY_RES_FORCE:
      GET_RESISTANCES(ch, DAM_FORCE) += mod;
      break;
    case APPLY_RES_SOUND:
      GET_RESISTANCES(ch, DAM_SOUND) += mod;
      break;
    case APPLY_RES_POISON:
      GET_RESISTANCES(ch, DAM_POISON) += mod;
      break;
    case APPLY_RES_DISEASE:
      GET_RESISTANCES(ch, DAM_DISEASE) += mod;
      break;
    case APPLY_RES_NEGATIVE:
      GET_RESISTANCES(ch, DAM_NEGATIVE) += mod;
      break;
    case APPLY_RES_ILLUSION:
      GET_RESISTANCES(ch, DAM_ILLUSION) += mod;
      break;
    case APPLY_RES_MENTAL:
      GET_RESISTANCES(ch, DAM_MENTAL) += mod;
      break;
    case APPLY_RES_LIGHT:
      GET_RESISTANCES(ch, DAM_LIGHT) += mod;
      break;
    case APPLY_RES_ENERGY:
      GET_RESISTANCES(ch, DAM_ENERGY) += mod;
      break;
    case APPLY_RES_WATER:
      GET_RESISTANCES(ch, DAM_WATER) += mod;
      break;
    case APPLY_DR:
      /* This needs to be updated. */
      break;
      /* Do Not Use. */
    case APPLY_FEAT:
      break;
    case APPLY_AGE:
      //ch->player.time.birth -= (mod * SECS_PER_MUD_YEAR);
      break;
    case APPLY_CLASS:
      break;
    case APPLY_LEVEL:
      break;
    case APPLY_GOLD:
      break;
    case APPLY_EXP:
      break;
    case APPLY_NONE:
      break;
      /* end Do Not Use */

    default:
      log("SYSERR: Unknown apply adjust %d attempt (%s, affect_modify).", loc, __FILE__);
      break;

  } /* switch */
}

void affect_modify_ar(struct char_data * ch, byte loc, sbyte mod, int bitv[],
        bool add) {
  int i, j;

  if (add) {
    for (i = 0; i < AF_ARRAY_MAX; i++)
      for (j = 0; j < 32; j++)
        if (IS_SET_AR(bitv, (i * 32) + j))
          SET_BIT_AR(AFF_FLAGS(ch), (i * 32) + j);
  } else {
    for (i = 0; i < AF_ARRAY_MAX; i++)
      for (j = 0; j < 32; j++)
        if (IS_SET_AR(bitv, (i * 32) + j))
          REMOVE_BIT_AR(AFF_FLAGS(ch), (i * 32) + j);
    mod = -mod;
  }

  aff_apply_modify(ch, loc, mod, "affect_modify_ar");
}

int calculate_best_mod(struct char_data *ch, int location, int bonus_type, int except_eq, int except_spell) {
  struct affected_type *af = NULL;
  int i = 0, j = 0;
  int best = 0;
  int modifier = 0;

  /* Skip stackable bonus types and bonus types without a modifier. */
  if ((location == APPLY_NONE) ||
      (location == APPLY_DR) ||
      (BONUS_TYPE_STACKS(bonus_type)) )
    return 0;

  /* Check affect structures */
  for (af = ch->affected; af; af = af->next) {
    /* Skip affects that are not on this location and have a different type. */
    if ((af->bonus_type != bonus_type) || (af->location != location))
      continue;
    if (af->spell == except_spell)
      continue;

    modifier = af->modifier;

    if (af->location == location && modifier > best)
      best = modifier;
  }

  /* Check gear. */
  for (i = 0; i < NUM_WEARS; i++) {
    if (GET_EQ(ch, i)) {
//      noeffect = FALSE;
//      for (k = 0; k < NUM_NO_AFFECT_EQ; k++)
//        if (OBJWEAR_FLAGGED(GET_EQ(ch, i), no_affect_eq[k]))
//          noeffect = TRUE;
//      if (noeffect)
//        continue;
      if (i == except_eq)
        continue;

      for (j = 0; j < MAX_OBJ_AFFECT; j++) {
        if ((GET_EQ(ch, i)->affected[j].bonus_type == bonus_type) &&
            (GET_EQ(ch, i)->affected[j].location   == location  )) {
          modifier = GET_EQ(ch, i)->affected[j].modifier;
          if (modifier > best)
            best = modifier;
        }
      }
    }
  }
  return best;
}

/* this will take a character's modified 'points' and reset it
 to their 'real points' */
void reset_char_points(struct char_data *ch) {
  int i = 0;
  //struct damage_reduction_type *damreduct;
  //struct dr_bypass_type *dr_bypass;

  ch->points.max_psp = ch->real_points.max_psp;
  ch->points.max_hit = ch->real_points.max_hit;
  ch->points.max_move = ch->real_points.max_move;
  ch->points.armor = ch->real_points.armor;
  ch->points.spell_res = ch->real_points.spell_res;
  ch->points.hitroll = ch->real_points.hitroll;
  ch->points.damroll = ch->real_points.damroll;
  ch->points.size = ch->real_points.size;
  for (i = 0; i < NUM_OF_SAVING_THROWS; i++)
    ch->points.apply_saving_throw[i] = ch->real_points.apply_saving_throw[i];
  for (i = 0; i < NUM_DAM_TYPES; i++)
    ch->points.resistances[i] = ch->real_points.resistances[i];

  /* Reset damage reduction */
//  for (damreduct = ch->points.damage_reduction;
//       damreduct != NULL;
//       damreduct = damreduct->next)
//  {
    /* We have a damage reduction record.  Clear it out. */
//    for (dr_bypass = damreduct->bypass;
//         dr_bypass != NULL;
//         dr_bypass = dr_bypass->next)
//    {

//    }
//  }
}

#define BASE_STAT_CAP 8
#define HP_CAP 200
#define PSP_CAP 300
#define MOVE_CAP 999
#define HITDAM_CAP 10
#define AC_CAP -140
#define SAVE_CAP 10
#define RESIST_CAP 100
void compute_char_cap(struct char_data *ch) {
  int hp_cap, psp_cap, move_cap, hit_cap, dam_cap, ac_cap,
          save_cap, resist_cap, class, class_level = 0;
  int str_cap, dex_cap, con_cap, wis_cap, int_cap, cha_cap;
  int rage_bonus = 0;

  /* values are between 1..stat-cap, not < 1 and not > stat-cap */
  (ch)->aff_abils.dex = MAX(1, MIN(GET_DEX(ch), STAT_CAP));
  GET_INT(ch) = MAX(1, MIN(GET_INT(ch), STAT_CAP));
  GET_WIS(ch) = MAX(1, MIN(GET_WIS(ch), STAT_CAP));
  (ch)->aff_abils.con = MAX(1, MIN(GET_CON(ch), STAT_CAP));
  GET_CHA(ch) = MAX(1, MIN(GET_CHA(ch), STAT_CAP));
  (ch)->aff_abils.str = MAX(1, MIN(GET_STR(ch), STAT_CAP));

  (ch)->points.size = MAX(SIZE_FINE, MIN(GET_SIZE(ch), SIZE_COLOSSAL));

  /* can add more restrictions to npc's above this if we like */
  if (IS_NPC(ch))
    return;

  /*****************/
  /* PC Cap System */

  /* start with base */
  str_cap = BASE_STAT_CAP + GET_REAL_STR(ch);
  dex_cap = BASE_STAT_CAP + GET_REAL_DEX(ch);
  con_cap = BASE_STAT_CAP + GET_REAL_CON(ch);
  wis_cap = BASE_STAT_CAP + GET_REAL_WIS(ch);
  int_cap = BASE_STAT_CAP + GET_REAL_INT(ch);
  cha_cap = BASE_STAT_CAP + GET_REAL_CHA(ch);
  hp_cap = HP_CAP + GET_REAL_MAX_HIT(ch);
  psp_cap = PSP_CAP + GET_REAL_MAX_PSP(ch);
  move_cap = MOVE_CAP + GET_REAL_MAX_MOVE(ch);
  hit_cap = HITDAM_CAP + GET_REAL_HITROLL(ch);
  dam_cap = HITDAM_CAP + GET_REAL_DAMROLL(ch);
  resist_cap = RESIST_CAP + GET_REAL_SPELL_RES(ch);
  ac_cap = AC_CAP;
  save_cap = SAVE_CAP;

  /* here for reference
  "Wizard"       int, dex, wis
  "Cleric"       wis, str, cha
  "MysticTheurge"int, wis, cha
  "Rogue"        dex,           hitroll, damroll, (str / int)
  "Warrior"      str, con,      hitroll, damroll
  "WeaponMaster" str, dex,      hitroll, damroll
  "Monk"         wis, dex,      hitroll, damroll
  "Druid"        wis, str, dex
  "Berserker"    str, con,      hitroll, damroll
  "Sorcerer"     cha, dex, int
  "Paladin"      cha, str,      hitroll, damroll
  "Ranger"       dex,           hitroll, damroll, (str / wis)
  "Bard"         cha, dex,      (hitroll, damroll, str, int)
   */

  /* here is the actual class modifiers */
  for (class = 0; class < MAX_CLASSES; class++) {
    if ((class_level = CLASS_LEVEL(ch, class)) > 0) {
      switch (class) {
        case CLASS_WIZARD:
          int_cap += class_level / 4 + 1;
          dex_cap += class_level / 4 + 1;
          wis_cap += class_level / 4 + 1;
          break;
        case CLASS_CLERIC:
          str_cap += class_level / 4 + 1;
          cha_cap += class_level / 4 + 1;
          wis_cap += class_level / 4 + 1;
          break;
        case CLASS_MYSTIC_THEURGE:
          int_cap += class_level / 4 + 1;
          cha_cap += class_level / 4 + 1;
          wis_cap += class_level / 4 + 1;
          break;          
        case CLASS_ROGUE:
          str_cap += class_level / 8 + 1;
          dex_cap += class_level / 4 + 1;
          int_cap += class_level / 8 + 1;
          hit_cap += class_level / 3;
          dam_cap += class_level / 3;
          break;
        case CLASS_ARCANE_ARCHER:
          dex_cap += class_level / 4 + 1;
          int_cap += class_level / 8 + 1;
          cha_cap += class_level / 8 + 1;
          hit_cap += class_level / 3;
          dam_cap += class_level / 3;
          break;
        case CLASS_WARRIOR:
          str_cap += class_level / 4 + 1;
          con_cap += class_level / 4 + 1;
          hit_cap += class_level / 3;
          dam_cap += class_level / 3;
          break;
        case CLASS_WEAPON_MASTER:
          str_cap += class_level / 4 + 1;
          dex_cap += class_level / 4 + 1;
          hit_cap += class_level / 3;
          dam_cap += class_level / 3;
          break;
        case CLASS_MONK:
          wis_cap += class_level / 4 + 1;
          dex_cap += class_level / 4 + 1;
          hit_cap += class_level / 3;
          dam_cap += class_level / 3;
          break;
        case CLASS_SHIFTER:
          str_cap += class_level / 4 + 1;
          dex_cap += class_level / 4 + 1;
          wis_cap += class_level / 4 + 1;
          break;
        case CLASS_DRUID:
          str_cap += class_level / 4 + 1;
          dex_cap += class_level / 4 + 1;
          wis_cap += class_level / 4 + 1;
          break;
        case CLASS_BERSERKER: /*rage*/
          if (affected_by_spell(ch, SKILL_RAGE)) {
            if (class_level < 11) /*normal*/
              rage_bonus = 4;
            else if (class_level < 20) /*greater*/
              rage_bonus = 6;
            else if (class_level < 27) /*mighty*/
              rage_bonus = 9;
            else if (class_level >= 27) /*indomitable*/
              rage_bonus = 12;
          }
          
          str_cap += class_level / 4 + 1 + rage_bonus;
          con_cap += class_level / 4 + 1 + rage_bonus;
          hit_cap += class_level / 3;
          dam_cap += class_level / 3;
          break;
        case CLASS_STALWART_DEFENDER: /*defensive stance*/
          if (affected_by_spell(ch, SKILL_DEFENSIVE_STANCE)) {
            if (class_level > 0) /*normal*/
              rage_bonus = 4;
          }
          
          str_cap += class_level / 4 + 1 + rage_bonus;
          con_cap += class_level / 4 + 1 + rage_bonus;
          hit_cap += class_level / 3;
          dam_cap += class_level / 3;
          break;
        case CLASS_SORCERER:
          int_cap += class_level / 4 + 1;
          dex_cap += class_level / 4 + 1;
          cha_cap += class_level / 4 + 1;
          break;
        case CLASS_PALADIN:
          str_cap += class_level / 4 + 1;
          cha_cap += class_level / 4 + 1;
          hit_cap += class_level / 3;
          dam_cap += class_level / 3;
          break;
        case CLASS_RANGER:
          dex_cap += class_level / 4 + 1;
          wis_cap += class_level / 8 + 1;
          str_cap += class_level / 8 + 1;
          hit_cap += class_level / 3;
          dam_cap += class_level / 3;
          break;
        case CLASS_BARD:
          dex_cap += class_level / 4 + 1;
          cha_cap += class_level / 4 + 1;
          str_cap += class_level / 8 + 1;
          int_cap += class_level / 8 + 1;
          hit_cap += class_level / 6;
          dam_cap += class_level / 6;
          break;
      }
    }
  }

  /* cap stats according to adjustments */
  /* note zusuk added another +3 to the cap just to accomodate berserkers */
  (ch)->aff_abils.dex = MIN(dex_cap + 4, GET_DEX(ch));
  GET_INT(ch) = MIN(int_cap + 4, GET_INT(ch));
  GET_WIS(ch) = MIN(wis_cap + 4, GET_WIS(ch));
  (ch)->aff_abils.con = MIN(con_cap + 4, GET_CON(ch));
  GET_CHA(ch) = MIN(cha_cap + 4, GET_CHA(ch));
  (ch)->aff_abils.str = MIN(str_cap + 4, GET_STR(ch));

  GET_HITROLL(ch) = MIN(hit_cap, GET_HITROLL(ch));
  GET_DAMROLL(ch) = MIN(dam_cap, GET_DAMROLL(ch));
}
#undef STAT_CAP
#undef BASE_STAT_CAP
#undef HP_CAP
#undef PSP_CAP
#undef MOVE_CAP
#undef HITDAM_CAP
#undef AC_CAP
#undef SAVE_CAP
#undef RESIST_CAP

/* this is just affect-total, the 'subtracting' portion of the function */
/* returns armor class of character after unaffected */
int affect_total_sub(struct char_data *ch) {
  struct affected_type *af;
  int i, j, at_armor = 100;
  int modifier = 0;
  int empty_bits[AF_ARRAY_MAX];

  for(i = 0; i > AF_ARRAY_MAX; i++)
    empty_bits[i] = 0;

  /* subtract affects with gear */
  for (i = 0; i < NUM_WEARS; i++) {
    if (GET_EQ(ch, i)) {
      for (j = 0; j < MAX_OBJ_AFFECT; j++) {
        if (BONUS_TYPE_STACKS(GET_EQ(ch, i)->affected[j].bonus_type)) {
          affect_modify_ar(ch, GET_EQ(ch, i)->affected[j].location,
              GET_EQ(ch, i)->affected[j].modifier,
              GET_OBJ_AFFECT(GET_EQ(ch, i)), FALSE);
        } else {
          affect_modify_ar(ch, GET_EQ(ch, i)->affected[j].location,
              0,//GET_EQ(ch, i)->affected[j].modifier,
              GET_OBJ_AFFECT(GET_EQ(ch, i)), FALSE);
        }
      }
    }
  }

  /* remove affects based on 'nekked' char */
  for (af = ch->affected; af; af = af->next) {
    //affect_modify_ar(ch, af->location, af->modifier, af->bitvector, FALSE);
    if (BONUS_TYPE_STACKS(af->bonus_type))
      affect_modify_ar(ch, af->location, af->modifier, af->bitvector, FALSE);
    else
      affect_modify_ar(ch, af->location, 0, af->bitvector, FALSE);
  }

  /* Adjust the modifiers to APPLY_ fields. */
  for (i = 0; i < NUM_APPLIES; i++) {
    modifier = 0;
    for (j = 0; j < NUM_BONUS_TYPES; j++) {
      modifier += calculate_best_mod(ch, i, j, -1, -1);
    }
    aff_apply_modify(ch, i, -modifier, "affect_total_sub");
    //affect_modify_ar(ch, i, modifier, empty_bits, FALSE);
  }

  /* any stats that are not an APPLY_ need to be stored */
  at_armor = GET_AC(ch);

  /* reset stats - everything should be at 0 now */
  ch->aff_abils = ch->real_abils;
  reset_char_points(ch);

  return at_armor;
}

/* this is just affect-total, the 're-adding' portion of the function */
void affect_total_plus(struct char_data *ch, int at_armor) {
  struct affected_type *af;
  int i, j;
  int empty_bits[AF_ARRAY_MAX];
  int modifier = 0;

  for (i = 0; i > AF_ARRAY_MAX; i++)
    empty_bits[i] = 0;

  /* restore stored stats */
  if (!(IS_NPC(ch))) (ch)->points.armor = at_armor;

  /* add gear back on */
  for (i = 0; i < NUM_WEARS; i++) {
    if (GET_EQ(ch, i)) {
      for (j = 0; j < MAX_OBJ_AFFECT; j++) {
        if (BONUS_TYPE_STACKS(GET_EQ(ch, i)->affected[j].bonus_type)) {
          affect_modify_ar(ch, GET_EQ(ch, i)->affected[j].location,
                  GET_EQ(ch, i)->affected[j].modifier,
                  GET_OBJ_AFFECT(GET_EQ(ch, i)), TRUE);
        } else {
          affect_modify_ar(ch, GET_EQ(ch, i)->affected[j].location,
                  0, //GET_EQ(ch, i)->affected[j].modifier,
                  GET_OBJ_AFFECT(GET_EQ(ch, i)), TRUE);
        }
      }
    }
  }

  /* re-apply affects based on 'regeared' char */
  for (af = ch->affected; af; af = af->next) {
    //affect_modify_ar(ch, af->location, af->modifier, af->bitvector, TRUE);
    if (BONUS_TYPE_STACKS(af->bonus_type))
      affect_modify_ar(ch, af->location, af->modifier, af->bitvector, TRUE);
    else
      affect_modify_ar(ch, af->location, 0, af->bitvector, TRUE);
  }

  /* Adjust the modifiers to APPLY_ fields. */
  for (i = 0; i < NUM_APPLIES; i++) {
    modifier = 0;
    for (j = 0; j < NUM_BONUS_TYPES; j++)
      modifier += calculate_best_mod(ch, i, j, -1, -1);
    aff_apply_modify(ch, i, modifier, "affect_total_plus");
    //affect_modify_ar(ch, i, modifier, empty_bits, TRUE);
  }

  /* cap character */
  compute_char_cap(ch);

  /* any dynamic stats need to be modified? (example, con -> hps) */
  GET_MAX_HIT(ch) += ((GET_CON(ch) - GET_REAL_CON(ch)) / 2 * GET_LEVEL(ch));
}

void cleanup_disguise(struct char_data *ch) {
  if (GET_DISGUISE_RACE(ch) == 0) {
    if (AFF_FLAGGED(ch, AFF_WILD_SHAPE))
      REMOVE_BIT_AR(AFF_FLAGS(ch), AFF_WILD_SHAPE);
    set_bonus_attributes(ch, 0, 0, 0, 0);
  }
  //if (!AFF_FLAGGED(ch, AFF_WILD_SHAPE)) {
  //  GET_DISGUISE_RACE(ch) = 0;
  //  set_bonus_attributes(ch, 0, 0, 0, 0);
  //}
}
void update_msdp_affects(struct char_data *ch) {
  char msdp_buffer[MAX_STRING_LENGTH];
  struct affected_type *af, *next;
  bool first = TRUE;
  int i = 0;

  /* MSDP */
  
  msdp_buffer[0] = '\0';
  if (ch && ch->desc) { 
    /* Open up the AFFECTS table */
    char buf2[4000];
    sprintf(buf2, "%c"
                  "%c%s%c"
                  "%c",
              (char)MSDP_TABLE_OPEN,
              (char)MSDP_VAR, "AFFECTED_BY", (char)MSDP_VAL, 
              (char)MSDP_ARRAY_OPEN);
        strcat(msdp_buffer, buf2);   
    for (i = 0; i < NUM_AFF_FLAGS; i++) {
      if (IS_SET_AR(AFF_FLAGS(ch), i)) {
        char buf[4000];
        sprintf(buf, "%c%c"
                   "%c%s%c%s"
                   "%c%s%c%s"                   
                         "%c",
            (char)MSDP_VAL, 
              (char)MSDP_TABLE_OPEN,
                (char)MSDP_VAR, "NAME", (char)MSDP_VAL, affected_bits[i],
                (char)MSDP_VAR, "DESC", (char)MSDP_VAL,affected_bit_descs[i],                
              (char)MSDP_TABLE_CLOSE);
        strcat(msdp_buffer, buf);      
      }
    }
    sprintf(buf2, "%c"
                  "%c%s%c"
                   "%c",
              (char)MSDP_ARRAY_CLOSE,
              (char)MSDP_VAR, "SPELL_LIKE_AFFECTS", (char)MSDP_VAL, 
              (char)MSDP_ARRAY_OPEN);
    strcat(msdp_buffer, buf2);   
    for (af = ch->affected; af; af = next) {
      char buf[4000]; // Buffer for building the affect table for MSDP    
      next = af->next;
      sprintf(buf, "%c%c"
                   "%c%s%c%s"
                   "%c%s%c%s"
                   "%c%s%c%d"
                   "%c%s%c%s"
                   "%c%s%c%d"
                         "%c",
            (char)MSDP_VAL, 
              (char)MSDP_TABLE_OPEN,
                (char)MSDP_VAR, "NAME", (char)MSDP_VAL, skill_name(af->spell),
                (char)MSDP_VAR, "LOCATION", (char)MSDP_VAL, apply_types[(int) af->location],
                (char)MSDP_VAR, "MODIFIER", (char)MSDP_VAL, af->modifier,
                (char)MSDP_VAR, "TYPE",     (char)MSDP_VAL, bonus_types[af->bonus_type], 
                (char)MSDP_VAR, "DURATION", (char)MSDP_VAL, af->duration,
              (char)MSDP_TABLE_CLOSE);
      strcat(msdp_buffer, buf);
      first = FALSE;
    }
    sprintf(buf2, "%c"
                  "%c",
              (char)MSDP_ARRAY_CLOSE,
              (char)MSDP_TABLE_CLOSE);
    strcat(msdp_buffer, buf2);

    //send_to_char(ch, "%s", msdp_buffer); 
    
    MSDPSetString(ch->desc, eMSDP_AFFECTS, msdp_buffer);
    MSDPFlush(ch->desc, eMSDP_AFFECTS);
  }
}

/* This updates a character by subtracting everything he is affected by
 * restoring original abilities, and then affecting all again. */
void affect_total(struct char_data *ch) {
  int at_armor = 100;
 
  /* cleanup for disguise system */
  cleanup_disguise(ch);

  /* this will subtract all affects and reset stats
     at_armor stores character's AC after being unaffected (like armor-apply) */
  at_armor = affect_total_sub(ch);

  /* this will re-add all affects, cap the char, and modify any dynamics */
  affect_total_plus(ch, at_armor);
  
  /* MSDP */
  update_msdp_affects(ch);
}

/* Insert an affect_type in a char_data structure. Automatically sets
 * appropriate bits and apply's */
void affect_to_char(struct char_data *ch, struct affected_type *af) {
  int i;
  struct affected_type *affected_alloc;
  int empty_bits[AF_ARRAY_MAX];

  for(i = 0; i > AF_ARRAY_MAX; i++)
    empty_bits[i] = 0;

  CREATE(affected_alloc, struct affected_type, 1);

  *affected_alloc = *af;
  affected_alloc->next = ch->affected;
  ch->affected = affected_alloc;

  /*affect_modify_ar(ch, af->location, af->modifier, af->bitvector, TRUE);*/
  affect_modify_ar(ch, af->location, 0, af->bitvector, TRUE);

  if (BONUS_TYPE_STACKS(af->bonus_type)) {
    affect_modify_ar(ch, af->location, af->modifier, af->bitvector, TRUE);
  } else if (af->modifier >
          calculate_best_mod(ch, af->location, af->bonus_type, -1, af->spell)) {
    aff_apply_modify(ch, af->location, -calculate_best_mod(ch, af->location,
            af->bonus_type, -1, af->spell), "affect_to_char");
    /*affect_modify_ar(ch, af->location, calculate_best_mod(ch, af->location,
             af->bonus_type, -1, af->spell), empty_bits, FALSE);*/
    affect_modify_ar(ch, af->location, af->modifier, af->bitvector, TRUE);
  }

  affect_total(ch);
  
}

/* Remove an affected_type structure from a char (called when duration reaches
 * zero). Pointer *af must never be NIL!  Frees mem and calls
 * affect_location_apply */
void affect_remove(struct char_data *ch, struct affected_type *af) {
  int i;
  struct affected_type *temp;
  int empty_bits[AF_ARRAY_MAX];

  for(i = 0; i > AF_ARRAY_MAX; i++)
    empty_bits[i] = 0;

  if (ch->affected == NULL) {
    core_dump();
    return;
  }

  affect_modify_ar(ch, af->location, 0, af->bitvector, FALSE);

  if (BONUS_TYPE_STACKS(af->bonus_type)) {
    affect_modify_ar(ch, af->location, af->modifier, af->bitvector, FALSE);
  } else if (af->modifier > calculate_best_mod(ch, af->location, af->bonus_type, -1, af->spell)) {
    aff_apply_modify(ch, af->location, calculate_best_mod(ch, af->location, af->bonus_type, -1, af->spell), "affect_remove");
    //affect_modify_ar(ch, af->location, calculate_best_mod(ch, af->location, af->bonus_type, -1, af->spell), empty_bits, TRUE);
   // affect_modify_ar(ch, af->location, af->modifier, af->bitvector, TRUE);
  }

  /* Check if we have anything that is 'nonstandard' from this affect */
  if (af->location == APPLY_DR) {
    /* Remove the dr. */
    struct damage_reduction_type *temp, *dr; /* Used by REMOVE_FROM_LIST */
    for(dr = GET_DR(ch); dr != NULL; dr = dr->next) {
      if (dr->spell == af->spell) {
        REMOVE_FROM_LIST(dr, GET_DR(ch), next);
      }
    }
  }
  
  REMOVE_FROM_LIST(af, ch->affected, next);

  free_affect(af);

  affect_total(ch);
  /* added by zusuk to address an issue with calculation not coming out correct
   on first run of affect_total() ?  unknown, need to trace and figure it out
   eventually though */
  //affect_total(ch);
}

/* Call affect_remove with every affect from the bitvector "type" */
void affect_type_from_char(struct char_data *ch, int type) {
  struct affected_type *hjp, *next;

  for (hjp = ch->affected; hjp; hjp = next) {
    next = hjp->next;
    if (hjp->bitvector[type])
        affect_remove(ch, hjp);
  }
}

/* Call affect_remove with every affect from the spell "spell" */
void affect_from_char(struct char_data *ch, int spell) {
  struct affected_type *hjp, *next;

  for (hjp = ch->affected; hjp; hjp = next) {
    next = hjp->next;
    if (hjp->spell == spell)
      affect_remove(ch, hjp);
  }
}

/* Return TRUE if a char is affected by a spell (SPELL_XXX), FALSE indicates
 * not affected. */
bool affected_by_spell(struct char_data *ch, int type) {
  struct affected_type *hjp;

  for (hjp = ch->affected; hjp; hjp = hjp->next)
    if (hjp->spell == type)
      return (TRUE);

  return (FALSE);
}

/* primary entry point to adding an affection to a player
   @in: character, affection structure, (on same affects:)
        Add duration? Avg duration? Add mod? Avg mod?*/
void affect_join(struct char_data *ch, struct affected_type *af,
        bool add_dur, bool avg_dur, bool add_mod, bool avg_mod) {
  struct affected_type *hjp = NULL, *next = NULL;
  bool found = FALSE;

  /* increment through all the affections on the character, check for matches */
  for (hjp = ch->affected; !found && hjp; hjp = next) {
    next = hjp->next;

    /* matching spell-number AND affection location matches? */
    if ((hjp->spell == af->spell) && (hjp->location == af->location)) {
      if (add_dur)
        af->duration += hjp->duration;
      else if (avg_dur)
        af->duration = (af->duration + hjp->duration) / 2;
      if (add_mod)
        af->modifier += hjp->modifier;
      else if (avg_mod)
        af->modifier = (af->modifier + hjp->modifier) / 2;

      /* replace! */
      affect_remove(ch, hjp);
      affect_to_char(ch, af);
      found = TRUE;
    }
  }
  
  /* no matches in our current affection list, so throw the affection on */
  if (!found)
    affect_to_char(ch, af);
}

/* function to update number of lights in a room */
void check_room_lighting(room_rnum room, struct char_data *ch, bool enter) {
  int i, value = 0, val2 = 0;
  struct obj_data *obj, *next_obj = NULL;

  /* check for light items */
  if (GET_EQ(ch, WEAR_LIGHT))
    if (GET_OBJ_TYPE(GET_EQ(ch, WEAR_LIGHT)) == ITEM_LIGHT)
      if (GET_OBJ_VAL(GET_EQ(ch, WEAR_LIGHT), 2)) /* Light ON */
        value++;

  /* check for 'magic lights' on worn gear */
  for (i = 0; i < NUM_WEARS; i++) {
    obj = GET_EQ(ch, i);
    if (obj)
      if (OBJ_FLAGGED(obj, ITEM_MAGLIGHT))
        value++;
  }
  /* check for 'magic lights' in inventory */
  for (obj = ch->carrying; obj; obj = next_obj) {
    next_obj = obj->next_content;
    if (obj)
      if (OBJ_FLAGGED(obj, ITEM_MAGLIGHT))
        value++;
  }

  /* check for 'glowing' on worn gear */
  for (i = 0; i < NUM_WEARS; i++) {
    obj = GET_EQ(ch, i);
    if (obj)
      if (OBJ_FLAGGED(obj, ITEM_GLOW))
        value++;
  }
  /* check for 'glowing' in inventory */
  for (obj = ch->carrying; obj; obj = next_obj) {
    next_obj = obj->next_content;
    if (obj)
      if (OBJ_FLAGGED(obj, ITEM_GLOW))
        value++;
  }

  /* lit-up mobiles (like fire elementals) */
  if (IS_NPC(ch) && MOB_FLAGGED(ch, MOB_LIT))
    value++;

  /* magically lit individuals */
  if (AFF_FLAGGED(ch, AFF_MAGE_FLAME))
    value++;

  /* done with lights, check globes */
  //  if (has_globe)
  //    val2++;

  /* modify lights in room */
  if (value > 0) {
    if (enter) {
      world[room].light += value;
      world[room].globe += val2;
    } else {
      world[room].light -= value;
      world[room].globe -= val2;
      if (world[room].light < 0)
        world[room].light = 0;
      if (world[room].globe < 0)
        world[room].globe = 0;
    }
  }
}

/* move a player out of a room */
void char_from_room(struct char_data *ch) {
  struct char_data *temp;

  if (ch == NULL) {
    log("SYSERR: NULL character in %s, char_from_room, shutting game down!", __FILE__);
    exit(1);
  }

  if (IN_ROOM(ch) == NOWHERE) {
    log("SYSERR: NOWHERE in %s, char_from_room, shutting game down!", __FILE__);
    exit(1);
  }

  if (FIGHTING(ch) != NULL)
    stop_fighting(ch);

  char_from_furniture(ch);

  /* checks for light, globes of darkness, etc */
  check_room_lighting(IN_ROOM(ch), ch, FALSE);

  REMOVE_FROM_LIST(ch, world[IN_ROOM(ch)].people, next_in_room);
  IN_ROOM(ch) = NOWHERE;
  ch->next_in_room = NULL;
}

/* place a char at the specified coord location in the wilderness specified. */
void char_to_coords(struct char_data *ch, int x, int y, int wilderness) {
  room_rnum room = NOWHERE;

  if (ch == NULL)
    log("SYSERR: Illegal value(s) passed to char_to_coords. ((x, y): (%d,%d) Ch: %p)",
      x, y, ch);
  else {
    room = find_room_by_coordinates(x, y);
    if (room == NOWHERE) {
      room = find_available_wilderness_room();
      if(room == NOWHERE) {
        return ;
      }
      /* Must set the coords, etc in the going_to room. */
      assign_wilderness_room(room, x, y);
    }
  }

  X_LOC(ch) = x;
  Y_LOC(ch) = y;

  char_to_room(ch, room);

}

/* place a character in a room */
void char_to_room(struct char_data *ch, room_rnum room) {

  if (ch == NULL || room == NOWHERE || room > top_of_world)
    log("SYSERR: Illegal value(s) passed to char_to_room. (Room: %d/%d Ch: %p)",
          room, top_of_world, ch);
  else {

    /* If this is a wilderness room, set coords. */
    if(ZONE_FLAGGED(GET_ROOM_ZONE(room), ZONE_WILDERNESS)) {
      if((X_LOC(ch) != world[room].coords[0]) || (Y_LOC(ch) != world[room].coords[1])) {
        room = find_room_by_coordinates(X_LOC(ch), Y_LOC(ch));
        if (room == NOWHERE) {
          room = find_available_wilderness_room();
          if(room == NOWHERE) {
            return ;
          }
          /* Must set the coords, etc in the going_to room. */
          assign_wilderness_room(room, X_LOC(ch), Y_LOC(ch));
        }

      }
      /* Set occupied flag */

      //log("room: %d real_room(worroom): %d top_of_world: %d is_dynamic(room) %d\n", room, real_room(room), top_of_world, IS_DYNAMIC(room));

      if (real_room(world[room].number) != NOWHERE &&  IS_DYNAMIC(room) ) {
        //log("Setting occupied bit to room: %d", room); /* spams syslogs */
        SET_BIT_AR(ROOM_FLAGS(room), ROOM_OCCUPIED);
        /* Create the event to clear the flag, if it is not already set. */
        if(!room_has_mud_event(&world[room], eCHECK_OCCUPIED))
          NEW_EVENT(eCHECK_OCCUPIED, &world[room].number, NULL, 10 RL_SEC);
      } else {
      }
    }

    ch->next_in_room = world[room].people;
    world[room].people = ch;
    IN_ROOM(ch) = room;


    autoquest_trigger_check(ch, 0, 0, AQ_ROOM_FIND);
    autoquest_trigger_check(ch, 0, 0, AQ_MOB_FIND);

    /* checks for light, globes of darkness, etc */
    check_room_lighting(room, ch, TRUE);

    /* Stop fighting now, if we left. */
    if (FIGHTING(ch) && IN_ROOM(ch) != IN_ROOM(FIGHTING(ch))) {
      stop_fighting(FIGHTING(ch));
      stop_fighting(ch);
    }

    /* falling */
    if (char_should_fall(ch, FALSE) && !char_has_mud_event(ch, eFALLING)) {
      /* the svariable value of 20 is just a rough number for feet */
      attach_mud_event(new_mud_event(eFALLING, ch, "20"), 5);
      send_to_char(ch, "Suddenly your realize you are falling!\r\n");
      act("$n has just realized $e has no visible means of support!",
              FALSE, ch, 0, 0, TO_ROOM);
    }
  // Send new MSDP data.
  update_msdp_room(ch);
  if (ch->desc)
    MSDPFlush(ch->desc, eMSDP_ROOM);
  }
}

/* Give an object to a char. */
void obj_to_char(struct obj_data *object, struct char_data *ch) {
  if (object && ch) {
    object->next_content = ch->carrying;
    ch->carrying = object;
    object->carried_by = ch;
    IN_ROOM(object) = NOWHERE;
    IS_CARRYING_W(ch) += GET_OBJ_WEIGHT(object);
    IS_CARRYING_N(ch)++;

    autoquest_trigger_check(ch, NULL, object, AQ_OBJ_FIND);

    /* set flag for crash-save system, but not on mobs! */
    if (!IS_NPC(ch))
      SET_BIT_AR(PLR_FLAGS(ch), PLR_CRASH);
    
    if (ch->desc) {
      update_msdp_inventory(ch);
      MSDPFlush(ch->desc, eMSDP_INVENTORY);
    }
  } else
    log("SYSERR: NULL obj (%p) or char (%p) passed to obj_to_char.", object, ch);
}

/* take an object from a char */
void obj_from_char(struct obj_data *object) {
  struct obj_data *temp;
  struct char_data *ch;

  ch = object->carried_by;
  
  if (object == NULL) {
    log("SYSERR: NULL object passed to obj_from_char.");
    return;
  }
  REMOVE_FROM_LIST(object, object->carried_by->carrying, next_content);

  /* set flag for crash-save system, but not on mobs! */
  if (!IS_NPC(object->carried_by))
    SET_BIT_AR(PLR_FLAGS(object->carried_by), PLR_CRASH);

  IS_CARRYING_W(object->carried_by) -= GET_OBJ_WEIGHT(object);
  IS_CARRYING_N(object->carried_by)--;
  object->carried_by = NULL;
  object->next_content = NULL;

  if (ch->desc) {
    update_msdp_inventory(ch);
    MSDPFlush(ch->desc, eMSDP_INVENTORY);
  }
}

/* Return the effect of a piece of armor in position eq_pos */
int apply_ac(struct char_data *ch, int eq_pos) {
  int factor = 1;

  if (GET_EQ(ch, eq_pos) == NULL) {
    core_dump();
    return (0);
  }

  if (!(GET_OBJ_TYPE(GET_EQ(ch, eq_pos)) == ITEM_ARMOR))
    return (0);

  switch (eq_pos) {

    default:
      factor = 1;
      break;
  }

  return (factor * GET_OBJ_VAL(GET_EQ(ch, eq_pos), 0));
}

int invalid_align(struct char_data *ch, struct obj_data *obj) {
  if (OBJ_FLAGGED(obj, ITEM_ANTI_EVIL) && IS_EVIL(ch))
    return TRUE;
  if (OBJ_FLAGGED(obj, ITEM_ANTI_GOOD) && IS_GOOD(ch))
    return TRUE;
  if (OBJ_FLAGGED(obj, ITEM_ANTI_NEUTRAL) && IS_NEUTRAL(ch))
    return TRUE;
  return FALSE;
}

int invalid_prof(struct char_data *ch, struct obj_data *obj) {
  if (IS_NPC(ch)) // npc's don't have restrictions at this stage -zusuk
    return FALSE;

  switch (GET_OBJ_PROF(obj)) {
    case ITEM_PROF_NONE:
      return FALSE;
    case ITEM_PROF_MINIMAL:
      return FALSE;
    case ITEM_PROF_BASIC:
      if (GET_SKILL(ch, SKILL_PROF_BASIC))
        return FALSE;
      else
        return TRUE;
    case ITEM_PROF_ADVANCED:
      if (GET_SKILL(ch, SKILL_PROF_ADVANCED))
        return FALSE;
      else
        return TRUE;
    case ITEM_PROF_MASTER:
      if (GET_SKILL(ch, SKILL_PROF_MASTER))
        return FALSE;
      else
        return TRUE;
    case ITEM_PROF_EXOTIC:
      if (GET_SKILL(ch, SKILL_PROF_EXOTIC))
        return FALSE;
      else
        return TRUE;
    case ITEM_PROF_LIGHT_A:
      if (GET_SKILL(ch, SKILL_PROF_LIGHT_A))
        return FALSE;
      else
        return TRUE;
    case ITEM_PROF_MEDIUM_A:
      if (GET_SKILL(ch, SKILL_PROF_MEDIUM_A))
        return FALSE;
      else
        return TRUE;
    case ITEM_PROF_HEAVY_A:
      if (GET_SKILL(ch, SKILL_PROF_HEAVY_A))
        return FALSE;
      else
        return TRUE;
    case ITEM_PROF_SHIELDS:
      if (GET_SKILL(ch, SKILL_PROF_SHIELDS))
        return FALSE;
      else
        return TRUE;
    case ITEM_PROF_T_SHIELDS:
      if (GET_SKILL(ch, SKILL_PROF_T_SHIELDS))
        return FALSE;
      else
        return TRUE;
    default:
      return TRUE;
  }
  return TRUE;
}

void equip_char(struct char_data *ch, struct obj_data *obj, int pos) {
  int i, j;
  int empty_bits[AF_ARRAY_MAX];
  room_rnum r_rnum = NOWHERE;

  for(i = 0; i > AF_ARRAY_MAX; i++)
    empty_bits[i] = 0;

  if (pos < 0 || pos >= NUM_WEARS) {
    core_dump();
    return;
  }

  if (GET_EQ(ch, pos)) {
    r_rnum = IN_ROOM(ch);
      
    log("SYSERR: Char/Loc [%d][%d] is already equipped: %s, %s", GET_MOB_VNUM(ch), GET_ROOM_VNUM(r_rnum), GET_NAME(ch),
            obj->short_description);
    return;
  }
  if (obj->carried_by) {
    log("SYSERR: EQUIP: Obj is carried_by when equip.");
    return;
  }
  if (IN_ROOM(obj) != NOWHERE) {
    log("SYSERR: EQUIP: Obj is in_room when equip.");
    return;
  }
  /*  Changed this - proficiencies are handles in the places where they apply penalties.
   *  You can wear whatever you want. - Ornir */
/* if (invalid_align(ch, obj) || invalid_class(ch, obj) || invalid_prof(ch, obj)) { */
  if (invalid_align(ch, obj) || invalid_class(ch, obj)) {
    act("You try to use $p, but fumble it and let go.", FALSE, ch, obj, 0, TO_CHAR);
    act("$n tries to use $p, but can't seem to and lets go.", FALSE, ch, obj, 0, TO_ROOM);
    /* Changed to drop in inventory instead of the ground. */
    obj_to_char(obj, ch);
    return;
  }

  GET_EQ(ch, pos) = obj;
  obj->worn_by = ch;
  obj->worn_on = pos;

  /* Object special abilities, process for ACTMTD_WEAR */
  process_item_abilities(obj, ch, NULL, ACTMTD_WEAR, NULL);

  /*  Modified this to use the NEW ac system - AC starts at 10 and is modified by armor.
   *  09/09/14 : Ornir */
  if (GET_OBJ_TYPE(obj) == ITEM_ARMOR)
    (ch)->points.armor += apply_ac(ch, pos);

  if (IN_ROOM(ch) != NOWHERE) {
    if (pos == WEAR_LIGHT && GET_OBJ_TYPE(obj) == ITEM_LIGHT)
      if (GET_OBJ_VAL(obj, 2)) /* if light is ON */
        world[IN_ROOM(ch)].light++;
  } else
    log("SYSERR: IN_ROOM(ch) = NOWHERE when equipping char %s.", GET_NAME(ch));

  /* apply_obj_affects(ch, obj);*/
  for (j = 0; j < MAX_OBJ_AFFECT; j++) {
    /* Here is where we need to see if these affects ACTUALLY apply,
     * based on the bonus types. */

    affect_modify_ar(ch, obj->affected[j].location,
          0, //obj->affected[j].modifier,
          GET_OBJ_AFFECT(obj), TRUE);

    if ((obj->affected[j].modifier) < 0) {
      affect_modify_ar(ch, obj->affected[j].location, obj->affected[j].modifier, GET_OBJ_AFFECT(obj), TRUE);
    } else if ((obj->affected[j].modifier) > calculate_best_mod(ch, obj->affected[j].location, obj->affected[j].bonus_type, pos, -1)) {
      affect_modify_ar(ch, obj->affected[j].location, obj->affected[j].modifier, GET_OBJ_AFFECT(obj), TRUE);
      aff_apply_modify(ch, obj->affected[j].location, -calculate_best_mod(ch, obj->affected[j].location, obj->affected[j].bonus_type, pos, -1), "equip_char");
      //affect_modify_ar(ch, obj->affected[j].location, calculate_best_mod(ch, obj->affected[j].location, obj->affected[j].bonus_type, pos, -1), empty_bits, FALSE);
    }

  }

  affect_total(ch);
}

struct obj_data *unequip_char(struct char_data *ch, int pos) {
  int i, j;
  struct obj_data *obj;
  int empty_bits[AF_ARRAY_MAX];

  for(i = 0; i > AF_ARRAY_MAX; i++)
    empty_bits[i] = 0;

  if ((pos < 0 || pos >= NUM_WEARS) || GET_EQ(ch, pos) == NULL) {
    core_dump();
    return (NULL);
  }

  obj = GET_EQ(ch, pos);
  obj->worn_by = NULL;
  obj->worn_on = -1;

  if (GET_OBJ_TYPE(obj) == ITEM_ARMOR)
    (ch)->points.armor -= apply_ac(ch, pos);

  if (IN_ROOM(ch) != NOWHERE) {
    if (pos == WEAR_LIGHT && GET_OBJ_TYPE(obj) == ITEM_LIGHT)
      if (GET_OBJ_VAL(obj, 2)) /* if light is ON */
        world[IN_ROOM(ch)].light--;
  } else
    log("SYSERR: IN_ROOM(ch) = NOWHERE when unequipping char %s.", GET_NAME(ch));

  GET_EQ(ch, pos) = NULL;

  for (j = 0; j < MAX_OBJ_AFFECT; j++) {
    /* Here is where we need to see if these affects ACTUALLY apply,
     * based on the bonus types. */
    affect_modify_ar(ch, obj->affected[j].location,
          0, //obj->affected[j].modifier,
          GET_OBJ_AFFECT(obj), FALSE);

    if ((obj->affected[j].modifier) < 0) {
      affect_modify_ar(ch, obj->affected[j].location, obj->affected[j].modifier, GET_OBJ_AFFECT(obj), FALSE);
    } else if ((obj->affected[j].modifier) > calculate_best_mod(ch, obj->affected[j].location, obj->affected[j].bonus_type, pos, -1)) {
      affect_modify_ar(ch, obj->affected[j].location, obj->affected[j].modifier, GET_OBJ_AFFECT(obj), FALSE);
      aff_apply_modify(ch, obj->affected[j].location, calculate_best_mod(ch, obj->affected[j].location, obj->affected[j].bonus_type, pos, -1), "equip_char");
      //affect_modify_ar(ch, obj->affected[j].location, calculate_best_mod(ch, obj->affected[j].location, obj->affected[j].bonus_type, pos, -1), empty_bits, TRUE);
    }
  }
  affect_total(ch);

  return (obj);
}

int get_number(char **name) {
  int i, retval;
  char *ppos, *namebuf;
  char number[MAX_INPUT_LENGTH];

  *number = '\0';

  retval = 1; /* Default is '1' */

  /* Make a working copy of name */
  namebuf = strdup(*name);

  if ((ppos = strchr(namebuf, '.')) != NULL) {

    *ppos++ = '\0';
    strlcpy(number, namebuf, sizeof (number));
    strcpy(*name, ppos); /* strcpy: OK (always smaller) */

    for (i = 0; *(number + i); i++)
      if (!isdigit(*(number + i)))
        retval = 0;

    retval = atoi(number);
  }

  free(namebuf);

  return retval;
}

/* Search a given list for an object number, and return a ptr to that obj */
struct obj_data *get_obj_in_list_num(int num, struct obj_data *list) {
  struct obj_data *i;

  for (i = list; i; i = i->next_content)
    if (GET_OBJ_RNUM(i) == num)
      return (i);

  return (NULL);
}

/* search the entire world for an object number, and return a pointer  */
struct obj_data *get_obj_num(obj_rnum nr) {
  struct obj_data *i;

  for (i = object_list; i; i = i->next)
    if (GET_OBJ_RNUM(i) == nr)
      return (i);

  return (NULL);
}

/* search a room for a char, and return a pointer if found..  */
struct char_data *get_char_room(char *name, int *number, room_rnum room) {
  struct char_data *i;
  int num;

  if (!number) {
    number = &num;
    num = get_number(&name);
  }

  if (*number == 0)
    return (NULL);

  for (i = world[room].people; i && *number; i = i->next_in_room)
    if (isname(name, i->player.name))
      if (--(*number) == 0)
        return (i);

  return (NULL);
}

/* search all over the world for a char num, and return a pointer if found */
struct char_data *get_char_num(mob_rnum nr) {
  struct char_data *i;

  for (i = character_list; i; i = i->next)
    if (GET_MOB_RNUM(i) == nr)
      return (i);

  return (NULL);
}

/* put an object in a room */
void obj_to_room(struct obj_data *object, room_rnum room) {
  if (!object || room == NOWHERE || room > top_of_world)
    log("SYSERR: Illegal value(s) passed to obj_to_room. (Room #%d/%d, obj %p)",
          room, top_of_world, object);
  else {
    object->next_content = world[room].contents;
    world[room].contents = object;
    IN_ROOM(object) = room;
    object->carried_by = NULL;
    if (ROOM_FLAGGED(room, ROOM_HOUSE))
      SET_BIT_AR(ROOM_FLAGS(room), ROOM_HOUSE_CRASH);

    /* falling check */
    if (obj_should_fall(object))
      ; // fall event for objects
  }
}

/* Take an object from a room */
void obj_from_room(struct obj_data *object) {
  struct obj_data *temp;
  struct char_data *t, *tempch;

  if (!object || IN_ROOM(object) == NOWHERE) {
    log("SYSERR: NULL object (%p) or obj not in a room (%d) passed to obj_from_room",
            object, IN_ROOM(object));
    return;
  }

  /* if people are sitting in it, toss their butt to the ground */
  if (OBJ_SAT_IN_BY(object) != NULL) {
    for (tempch = OBJ_SAT_IN_BY(object); tempch; tempch = t) {
      t = NEXT_SITTING(tempch);
      SITTING(tempch) = NULL;
      NEXT_SITTING(tempch) = NULL;
    }
  }

  REMOVE_FROM_LIST(object, world[IN_ROOM(object)].contents, next_content);

  if (ROOM_FLAGGED(IN_ROOM(object), ROOM_HOUSE))
    SET_BIT_AR(ROOM_FLAGS(IN_ROOM(object)), ROOM_HOUSE_CRASH);
  IN_ROOM(object) = NOWHERE;
  object->next_content = NULL;
}

/* put an object in an object (quaint)  */
void obj_to_obj(struct obj_data *obj, struct obj_data *obj_to) {
  struct obj_data *tmp_obj;

  if (!obj || !obj_to || obj == obj_to) {
    log("SYSERR: NULL object (%p) or same source (%p) and target (%p) obj passed to obj_to_obj.",
            obj, obj, obj_to);
    return;
  }

  obj->next_content = obj_to->contains;
  obj_to->contains = obj;
  obj->in_obj = obj_to;
  tmp_obj = obj->in_obj;

  /* Add weight to container, unless unlimited. */
  if (GET_OBJ_VAL(obj->in_obj, 0) > 0) {
    for (tmp_obj = obj->in_obj; tmp_obj->in_obj; tmp_obj = tmp_obj->in_obj)
      GET_OBJ_WEIGHT(tmp_obj) += GET_OBJ_WEIGHT(obj);

    /* top level object.  Subtract weight from inventory if necessary. */
    GET_OBJ_WEIGHT(tmp_obj) += GET_OBJ_WEIGHT(obj);
    if (tmp_obj->carried_by)
      IS_CARRYING_W(tmp_obj->carried_by) += GET_OBJ_WEIGHT(obj);
  }
}

/* remove an object from an object */
void obj_from_obj(struct obj_data *obj) {
  struct obj_data *temp, *obj_from;

  if (obj->in_obj == NULL) {
    log("SYSERR: (%s): trying to illegally extract obj from obj.", __FILE__);
    return;
  }
  obj_from = obj->in_obj;
  temp = obj->in_obj;
  REMOVE_FROM_LIST(obj, obj_from->contains, next_content);

  /* Subtract weight from containers container unless unlimited. */
  if (GET_OBJ_VAL(obj->in_obj, 0) > 0) {
    for (temp = obj->in_obj; temp->in_obj; temp = temp->in_obj)
      GET_OBJ_WEIGHT(temp) -= GET_OBJ_WEIGHT(obj);

    /* Subtract weight from char that carries the object */
    GET_OBJ_WEIGHT(temp) -= GET_OBJ_WEIGHT(obj);
    if (temp->carried_by)
      IS_CARRYING_W(temp->carried_by) -= GET_OBJ_WEIGHT(obj);
  }
  obj->in_obj = NULL;
  obj->next_content = NULL;
}

/* Set all carried_by to point to new owner */
void object_list_new_owner(struct obj_data *list, struct char_data *ch) {
  if (list) {
    object_list_new_owner(list->contains, ch);
    object_list_new_owner(list->next_content, ch);
    list->carried_by = ch;
  }
}

/* Extract an object from the world */
void extract_obj(struct obj_data *obj) {
  struct char_data *ch = NULL, *next = NULL;
  struct obj_data *temp = NULL;

  if (obj->worn_by != NULL)
    if (unequip_char(obj->worn_by, obj->worn_on) != obj)
      log("SYSERR: Inconsistent worn_by and worn_on pointers!!");
  if (IN_ROOM(obj) != NOWHERE)
    obj_from_room(obj);
  else if (obj->carried_by)
    obj_from_char(obj);
  else if (obj->in_obj)
    obj_from_obj(obj);

  if (OBJ_SAT_IN_BY(obj)) {
    for (ch = OBJ_SAT_IN_BY(obj); OBJ_SAT_IN_BY(obj); ch = next) {
      if (!NEXT_SITTING(ch))
        OBJ_SAT_IN_BY(obj) = NULL;
      else
        OBJ_SAT_IN_BY(obj) = (next = NEXT_SITTING(ch));
      SITTING(ch) = NULL;
      NEXT_SITTING(ch) = NULL;
    }
  }

  /* Get rid of the contents of the object, as well. */
  while (obj->contains)
    extract_obj(obj->contains);

  REMOVE_FROM_LIST(obj, object_list, next);

  if (GET_OBJ_RNUM(obj) != NOTHING)
    (obj_index[GET_OBJ_RNUM(obj)].number)--;

  if (SCRIPT(obj))
    extract_script(obj, OBJ_TRIGGER);

  if (obj->events != NULL) {
    if (obj->events->iSize > 0) {
      struct event * pEvent;

      while ((pEvent = simple_list(obj->events)) != NULL)
        event_cancel(pEvent);
    }
    free_list(obj->events);
    obj->events = NULL;
  }

  if (GET_OBJ_RNUM(obj) == NOTHING || obj->proto_script != obj_proto[GET_OBJ_RNUM(obj)].proto_script)
    free_proto_script(obj, OBJ_TRIGGER);

  free_obj(obj);
}

static void update_object(struct obj_data *obj, int use) {
  /* dont update objects with a timer trigger */
  if (!SCRIPT_CHECK(obj, OTRIG_TIMER) && (GET_OBJ_TIMER(obj) > 0))
    GET_OBJ_TIMER(obj) -= use;
  if (obj->contains)
    update_object(obj->contains, use);
  if (obj->next_content)
    update_object(obj->next_content, use);
}

void update_char_objects(struct char_data *ch) {
  int i;

  if (GET_EQ(ch, WEAR_LIGHT) != NULL)
    if (GET_OBJ_TYPE(GET_EQ(ch, WEAR_LIGHT)) == ITEM_LIGHT)
      if (GET_OBJ_VAL(GET_EQ(ch, WEAR_LIGHT), 2) > 0) {
        i = --GET_OBJ_VAL(GET_EQ(ch, WEAR_LIGHT), 2);
        if (i == 2) {
          send_to_char(ch, "Your \tYlight\tn is beginning to \tDwane\tn.\r\n");
          act("$n's light is beginning to wane.", FALSE, ch, 0, 0, TO_ROOM);
        } else if (i == 1) {
          send_to_char(ch, "Your \tYlight\tn begins to \tDflicker and fade\tn.\r\n");
          act("$n's light begins to flicker and fade.", FALSE, ch, 0, 0, TO_ROOM);
        } else if (i == 0) {
          send_to_char(ch, "Your \tYlight\tn sputters out and \tDdies\tn.\r\n");
          act("$n's light sputters out and dies.", FALSE, ch, 0, 0, TO_ROOM);
          world[IN_ROOM(ch)].light--;
          obj_to_char(unequip_char(ch, WEAR_LIGHT), ch);
        }
      }

  for (i = 0; i < NUM_WEARS; i++)
    if (GET_EQ(ch, i))
      update_object(GET_EQ(ch, i), 2);

  if (ch->carrying)
    update_object(ch->carrying, 1);
}

/* Extract a ch completely from the world, and leave his stuff behind */
void extract_char_final(struct char_data *ch) {
  struct char_data *k, *temp;
  struct descriptor_data *d;
  struct obj_data *obj;
  struct char_data *tch = NULL;
  int i;

  if (IN_ROOM(ch) == NOWHERE) {
    log("SYSERR: NOWHERE extracting char %s. (%s, extract_char_final)",
            GET_NAME(ch), __FILE__);
    exit(1);
  }

  /* We're booting the character of someone who has switched so first we need
   * to stuff them back into their own body.  This will set ch->desc we're
   * checking below this loop to the proper value. */
  if (!IS_NPC(ch) && !ch->desc) {
    for (d = descriptor_list; d; d = d->next)
      if (d->original == ch) {
        do_return(d->character, NULL, 0, 0);
        break;
      }
  }

  if (ch->desc) {
    /* This time we're extracting the body someone has switched into (not the
     * body of someone switching as above) so we need to put the switcher back
     * to their own body. If this body is not possessed, the owner won't have a
     * body after the removal so dump them to the main menu. */
    if (ch->desc->original)
      do_return(ch, NULL, 0, 0);
    else {
      /* Now we boot anybody trying to log in with the same character, to help
       * guard against duping.  CON_DISCONNECT is used to close a descriptor
       * without extracting the d->character associated with it, for being
       * link-dead, so we want CON_CLOSE to clean everything up. If we're
       * here, we know it's a player so no IS_NPC check required. */
      for (d = descriptor_list; d; d = d->next) {
        if (d == ch->desc)
          continue;
        if (d->character && GET_IDNUM(ch) == GET_IDNUM(d->character))
          STATE(d) = CON_CLOSE;
      }
      STATE(ch->desc) = CON_MENU;
      write_to_output(ch->desc, "%s", CONFIG_MENU);
    }
  }

  /* On with the character's assets... */
  if (ch->followers || ch->master)
    die_follower(ch);

  /* clear riding */
  if (RIDING(ch) || RIDDEN_BY(ch))
    dismount_char(ch);

  /* Check to see if we are grouped! */
  if (GROUP(ch))
    leave_group(ch);

  /* reset guard */
  GUARDING(ch) = NULL;
  for (tch = character_list; tch; tch = tch->next) {
    if (GUARDING(tch) == ch)
      GUARDING(tch) = NULL;
  }

  /* transfer objects to room, if any */
  while (ch->carrying) {
    obj = ch->carrying;
    obj_from_char(obj);
    obj_to_room(obj, IN_ROOM(ch));
  }

  /* transfer equipment to room, if any */
  for (i = 0; i < NUM_WEARS; i++)
    if (GET_EQ(ch, i))
      obj_to_room(unequip_char(ch, i), IN_ROOM(ch));

  /* stop any fighting */
  if (FIGHTING(ch))
    stop_fighting(ch);
  FIRING(ch) = 0;

  for (k = combat_list; k; k = temp) {
    temp = k->next_fighting;
    if (FIGHTING(k) == ch)
      stop_fighting(k);
  }

  /* Clear the action queue */
  clear_action_queue(GET_QUEUE(ch));

  /* Wipe character from the memory of hunters and other intelligent NPCs... */
  for (temp = character_list; temp; temp = temp->next) {
    /* PCs can't use MEMORY, and don't use HUNTING() */
    if (!IS_NPC(temp))
      continue;
    /* If "temp" is hunting our extracted char, stop the hunt. */
    if (HUNTING(temp) == ch)
      HUNTING(temp) = NULL;
    /* If "temp" has allocated memory data and our ch is a PC, forget the
     * extracted character (if he/she is remembered) */
    if (!IS_NPC(ch) && GET_POS(ch) == POS_DEAD && MEMORY(temp))
      forget(temp, ch); /* forget() is safe to use without a check. */
  }

  char_from_room(ch);

  if (IS_NPC(ch)) {
    if (GET_MOB_RNUM(ch) != NOTHING) /* prototyped */
      mob_index[GET_MOB_RNUM(ch)].number--;
    clearMemory(ch);

    if (SCRIPT(ch))
      extract_script(ch, MOB_TRIGGER);

    if (SCRIPT_MEM(ch))
      extract_script_mem(SCRIPT_MEM(ch));
  } else { // !IS_NPC
    /* do NOT save events here, the value of 1 for the 2nd parameter of
       save_char was setup for this express goal */
    /* note - this is deprecated, switched value back to 0 */
    save_char(ch, 0);

    Crash_delete_crashfile(ch);
  }

  /* If there's a descriptor, they're in the menu now. */
  if (IS_NPC(ch) || !ch->desc)
    free_char(ch);
}

/* Why do we do this? Because trying to iterate over the character list with
 * 'ch = ch->next' does bad things if the current character happens to die. The
 * trivial workaround of 'vict = next_vict' doesn't work if the _next_ person
 * in the list gets killed, for example, by an area spell. Why do we leave them
 * on the character_list? Because code doing 'vict = vict->next' would get
 * really confused otherwise. */
void extract_char(struct char_data *ch) {

  char_from_furniture(ch);

  /* We want to save events, this will be last legitimate save including
     events before extract_char_final(), we have to make sure in
     extract_char_final() we DO NOT save events
  save_char(ch, 0);
   */

  /* this is a solution to a whirlwind bug where while in the menu
     after death while whirlwinding, the event would try to search
     through the room to find targets, but you had no room
     this is the recommended solution below by vatiken, i chanaged it
     inside the event_whirlwind to just make sure the player is
     IS_PLAYING() or the event will not continue ...  the positive
     side effect of my solution is this:  saving events becomes a lot
     easier task */

  if (IS_NPC(ch))
    SET_BIT_AR(MOB_FLAGS(ch), MOB_NOTDEADYET);
  else
    SET_BIT_AR(PLR_FLAGS(ch), PLR_NOTDEADYET);

  extractions_pending++;
}

/* I'm not particularly pleased with the MOB/PLR hoops that have to be jumped
 * through but it hardly calls for a completely new variable. Ideally it would
 * be its own list, but that would change the '->next' pointer, potentially
 * confusing some code. -gg This doesn't handle recursive extractions. */
void extract_pending_chars(void) {
  struct char_data *vict, *next_vict, *prev_vict;

  if (extractions_pending < 0)
    log("SYSERR: Negative (%d) extractions pending.", extractions_pending);

  for (vict = character_list, prev_vict = NULL; vict && extractions_pending; vict = next_vict) {
    next_vict = vict->next;

    if (MOB_FLAGGED(vict, MOB_NOTDEADYET))
      REMOVE_BIT_AR(MOB_FLAGS(vict), MOB_NOTDEADYET);
    else if (PLR_FLAGGED(vict, PLR_NOTDEADYET))
      REMOVE_BIT_AR(PLR_FLAGS(vict), PLR_NOTDEADYET);
    else {
      /* Last non-free'd character to continue chain from. */
      prev_vict = vict;
      continue;
    }

    extract_char_final(vict);
    extractions_pending--;

    if (prev_vict)
      prev_vict->next = next_vict;
    else
      character_list = next_vict;
  }

  if (extractions_pending > 0)
    log("SYSERR: Couldn't find %d extractions as counted.", extractions_pending);

  extractions_pending = 0;
}

/* Here follows high-level versions of some earlier routines, ie functions
 * which incorporate the actual player-data */
struct char_data *get_player_vis(struct char_data *ch, char *name, int *number, int inroom) {
  struct char_data *i;
  int num;

  if (!number) {
    number = &num;
    num = get_number(&name);
  }

  for (i = character_list; i; i = i->next) {
    if (IS_NPC(i))
      continue;
    if (inroom == FIND_CHAR_ROOM && IN_ROOM(i) != IN_ROOM(ch))
      continue;
    if (str_cmp(i->player.name, name)) /* If not same, continue */
      continue;
    if (!CAN_SEE(ch, i))
      continue;
    if (--(*number) != 0)
      continue;
    return (i);
  }

  return (NULL);
}

struct char_data *get_char_room_vis(struct char_data *ch, char *name, int *number) {
  struct char_data *i;
  int num;

  if (!number) {
    number = &num;
    num = get_number(&name);
  }

  /* JE */
  if (!str_cmp(name, "self") || !str_cmp(name, "me"))
    return (ch);

  /* 0.<name> means PC with name */
  if (*number == 0)
    return (get_player_vis(ch, name, NULL, FIND_CHAR_ROOM));

  for (i = world[IN_ROOM(ch)].people; i && *number; i = i->next_in_room) {
    /* we have to handle disguises */
    if (GET_DISGUISE_RACE(i)) {
      if (is_abbrev(name, race_list[GET_DISGUISE_RACE(i)].name)) {
        return (i);
      }
    }

    if (isname(name, i->player.name)) {
      if (CAN_SEE(ch, i) || CAN_INFRA(ch, i)) {
        if (--(*number) == 0) {
          return (i);
        }
      }
    }
  }

  return (NULL);
}

struct char_data *get_char_world_vis(struct char_data *ch, char *name, int *number) {
  struct char_data *i;
  int num;

  if (!number) {
    number = &num;
    num = get_number(&name);
  }

  if ((i = get_char_room_vis(ch, name, number)) != NULL)
    return (i);

  if (*number == 0)
    return (get_player_vis(ch, name, NULL, 0));

  for (i = character_list; i && *number; i = i->next) {
    if (IN_ROOM(ch) == IN_ROOM(i))
      continue;
    if (!isname(name, i->player.name))
      continue;
    if (!CAN_SEE(ch, i))
      continue;
    if (--(*number) != 0)
      continue;

    return (i);
  }
  return (NULL);
}

struct char_data *get_char_vis(struct char_data *ch, char *name, int *number, int where) {
  if (where == FIND_CHAR_ROOM)
    return get_char_room_vis(ch, name, number);
  else if (where == FIND_CHAR_WORLD)
    return get_char_world_vis(ch, name, number);
  else
    return (NULL);
}

struct obj_data *get_obj_in_list_vis(struct char_data *ch, char *name, int *number, struct obj_data *list) {
  struct obj_data *i;
  int num;

  if (!number) {
    number = &num;
    num = get_number(&name);
  }

  if (*number == 0)
    return (NULL);

  for (i = list; i && *number; i = i->next_content)
    if (isname(name, i->name))
      if (CAN_SEE_OBJ(ch, i))
        if (--(*number) == 0)
          return (i);

  return (NULL);
}

/* search the entire world for an object, and return a pointer  */
struct obj_data *get_obj_vis(struct char_data *ch, char *name, int *number) {
  struct obj_data *i;
  int num;

  if (!number) {
    number = &num;
    num = get_number(&name);
  }

  if (*number == 0)
    return (NULL);

  /* scan items carried */
  if ((i = get_obj_in_list_vis(ch, name, number, ch->carrying)) != NULL)
    return (i);

  /* scan room */
  if ((i = get_obj_in_list_vis(ch, name, number, world[IN_ROOM(ch)].contents)) != NULL)
    return (i);

  /* ok.. no luck yet. scan the entire obj list   */
  for (i = object_list; i && *number; i = i->next)
    if (isname(name, i->name))
      if (CAN_SEE_OBJ(ch, i))
        if (--(*number) == 0)
          return (i);

  return (NULL);
}

struct obj_data *get_obj_in_equip_vis(struct char_data *ch, char *arg, int *number, struct obj_data *equipment[]) {
  int j, num;

  if (!number) {
    number = &num;
    num = get_number(&arg);
  }

  if (*number == 0)
    return (NULL);

  for (j = 0; j < NUM_WEARS; j++)
    if (equipment[j] && CAN_SEE_OBJ(ch, equipment[j]) && isname(arg, equipment[j]->name))
      if (--(*number) == 0)
        return (equipment[j]);

  return (NULL);
}

int get_obj_pos_in_equip_vis(struct char_data *ch, char *arg, int *number, struct obj_data *equipment[]) {
  int j, num;

  if (!number) {
    number = &num;
    num = get_number(&arg);
  }

  if (*number == 0)
    return (-1);

  for (j = 0; j < NUM_WEARS; j++)
    if (equipment[j] && CAN_SEE_OBJ(ch, equipment[j]) && isname(arg, equipment[j]->name))
      if (--(*number) == 0)
        return (j);

  return (-1);
}

const char *money_desc(int amount) {
  int cnt;

  struct {
    int limit;
    const char *description;
  } money_table[] = {
    { 1, "a gold coin"},
    { 10, "a tiny pile of gold coins"},
    { 20, "a handful of gold coins"},
    { 75, "a little pile of gold coins"},
    { 200, "a small pile of gold coins"},
    { 1000, "a pile of gold coins"},
    { 5000, "a big pile of gold coins"},
    { 10000, "a large heap of gold coins"},
    { 20000, "a huge mound of gold coins"},
    { 75000, "an enormous mound of gold coins"},
    { 150000, "a small mountain of gold coins"},
    { 250000, "a mountain of gold coins"},
    { 500000, "a huge mountain of gold coins"},
    { 1000000, "an enormous mountain of gold coins"},
    { 0, NULL},
  };

  if (amount <= 0) {
    log("SYSERR: Try to create negative or 0 money (%d).", amount);
    return (NULL);
  }

  for (cnt = 0; money_table[cnt].limit; cnt++)
    if (amount <= money_table[cnt].limit)
      return (money_table[cnt].description);

  return ("an absolutely colossal mountain of gold coins");
}

struct obj_data *create_money(int amount) {
  struct obj_data *obj = NULL;
  struct extra_descr_data *new_descr = NULL;
  char buf[200] = {'\0'};
  int y = 0;

  if (amount <= 0) {
    log("SYSERR: Try to create negative or 0 money. (%d)", amount);
    return (NULL);
  }
  obj = create_obj();
  CREATE(new_descr, struct extra_descr_data, 1);

  if (amount == 1) {
    obj->name = strdup("coin gold");
    obj->short_description = strdup("a gold coin");
    obj->description = strdup("One miserable gold coin is lying here.");
    new_descr->keyword = strdup("coin gold");
    new_descr->description = strdup("It's just one miserable little gold coin.");
  } else {
    obj->name = strdup("coins gold");
    obj->short_description = strdup(money_desc(amount));
    snprintf(buf, sizeof (buf), "%s is lying here.", money_desc(amount));
    obj->description = strdup(CAP(buf));

    new_descr->keyword = strdup("coins gold");
    if (amount < 10)
      snprintf(buf, sizeof (buf), "There are %d coins.", amount);
    else if (amount < 100)
      snprintf(buf, sizeof (buf), "There are about %d coins.", 10 * (amount / 10));
    else if (amount < 1000)
      snprintf(buf, sizeof (buf), "It looks to be about %d coins.", 100 * (amount / 100));
    else if (amount < 100000)
      snprintf(buf, sizeof (buf), "You guess there are, maybe, %d coins.",
            1000 * ((amount / 1000) + rand_number(0, (amount / 1000))));
    else
      strcpy(buf, "There are a LOT of coins."); /* strcpy: OK (is < 200) */
    new_descr->description = strdup(buf);
  }

  new_descr->next = NULL;
  obj->ex_description = new_descr;

  GET_OBJ_TYPE(obj) = ITEM_MONEY;
  for (y = 0; y < TW_ARRAY_MAX; y++)
    obj->obj_flags.wear_flags[y] = 0;
  SET_BIT_AR(GET_OBJ_WEAR(obj), ITEM_WEAR_TAKE);
  GET_OBJ_VAL(obj, 0) = amount;
  GET_OBJ_COST(obj) = amount;
  obj->item_number = NOTHING;

  return (obj);
}

/* Generic Find, designed to find any object or character.
 *  *arg     is the pointer containing the string to be searched for.
 *           This string doesn't have to be a single word, the routine
 *           extracts the next word itself.
 *  bitv..   All those bits that you want to "search through".
 *           Bit found will be result of the function
 *  *ch      This is the person that is trying to "find"
 *  **tar_ch Will be NULL if no character was found, otherwise points
 * **tar_obj Will be NULL if no object was found, otherwise points
 *
 * The routine used to return a pointer to the next word in *arg (just
 * like the one_argument routine), but now it returns an integer that
 * describes what it filled in. */
int generic_find(char *arg, bitvector_t bitvector, struct char_data *ch,
        struct char_data **tar_ch, struct obj_data **tar_obj) {
  int i, found, number;
  char name_val[MAX_INPUT_LENGTH];
  char *name = name_val;

  *tar_ch = NULL;
  *tar_obj = NULL;

  one_argument(arg, name);

  if (!*name)
    return (0);
  if (!(number = get_number(&name)))
    return (0);

  if (IS_SET(bitvector, FIND_CHAR_ROOM)) { /* Find person in room */
    if ((*tar_ch = get_char_room_vis(ch, name, &number)) != NULL)
      return (FIND_CHAR_ROOM);
  }

  if (IS_SET(bitvector, FIND_CHAR_WORLD)) {
    if ((*tar_ch = get_char_world_vis(ch, name, &number)) != NULL)
      return (FIND_CHAR_WORLD);
  }

  if (IS_SET(bitvector, FIND_OBJ_EQUIP)) {
    for (found = FALSE, i = 0; i < NUM_WEARS && !found; i++)
      if (GET_EQ(ch, i) && isname(name, GET_EQ(ch, i)->name) && --number == 0) {
        *tar_obj = GET_EQ(ch, i);
        found = TRUE;
      }
    if (found)
      return (FIND_OBJ_EQUIP);
  }

  if (IS_SET(bitvector, FIND_OBJ_INV)) {
    if ((*tar_obj = get_obj_in_list_vis(ch, name, &number, ch->carrying)) != NULL)
      return (FIND_OBJ_INV);
  }

  if (IS_SET(bitvector, FIND_OBJ_ROOM)) {
    if ((*tar_obj = get_obj_in_list_vis(ch, name, &number, world[IN_ROOM(ch)].contents)) != NULL)
      return (FIND_OBJ_ROOM);
  }

  if (IS_SET(bitvector, FIND_OBJ_WORLD)) {
    if ((*tar_obj = get_obj_vis(ch, name, &number)))
      return (FIND_OBJ_WORLD);
  }

  return (0);
}

/* a function to scan for "all" or "all.x" */
int find_all_dots(char *arg) {
  if (!strcmp(arg, "all"))
    return (FIND_ALL);
  else if (!strncmp(arg, "all.", 4)) {
    strcpy(arg, arg + 4); /* strcpy: OK (always less) */
    return (FIND_ALLDOT);
  } else
    return (FIND_INDIV);
}

/* Group Handlers */
struct group_data * create_group(struct char_data * leader) {
  struct group_data * new_group;

  /* Allocate Group Memory & Attach to Group List*/
  CREATE(new_group, struct group_data, 1);
  add_to_list(new_group, group_list);

  /* Allocate Members List */
  new_group->members = create_list();

  /* Clear Data */
  new_group->group_flags = 0;

  /* Assign Data */
  SET_BIT(GROUP_FLAGS(new_group), GROUP_OPEN);

  if (IS_NPC(leader))
    SET_BIT(GROUP_FLAGS(new_group), GROUP_NPC);

  join_group(leader, new_group);

  return (new_group);

  update_msdp_group(leader);
  if (leader->desc) 
    MSDPFlush(leader->desc, eMSDP_GROUP);
}

void free_group(struct group_data * group) {
  struct char_data *tch;
  struct iterator_data Iterator;

  if (group->members->iSize) {
    for (tch = (struct char_data *) merge_iterator(&Iterator, group->members);
            tch;
            tch = next_in_list(&Iterator))
      leave_group(tch);

    remove_iterator(&Iterator);
  }

  free_list(group->members);
  remove_from_list(group, group_list);
  free(group);
}

void leave_group(struct char_data *ch) {
  struct group_data *group;
  struct char_data *tch;
  struct iterator_data Iterator;
  bool found_pc = FALSE;

  if ((group = ch->group) == NULL)
    return;

  //  if (group->members->iSize == 0)
  send_to_group(NULL, group, "%s has left the group.\r\n", GET_NAME(ch));

  remove_from_list(ch, group->members);
  ch->group = NULL;

  if (group->members->iSize) {
    for (tch = (struct char_data *) merge_iterator(&Iterator, group->members);
            tch; tch = next_in_list(&Iterator))
      if (!IS_NPC(tch))
        found_pc = TRUE;

    remove_iterator(&Iterator);
  }

  if (!found_pc)
    SET_BIT(GROUP_FLAGS(group), GROUP_NPC);

  if (GROUP_LEADER(group) == ch && group->members->iSize) {
    group->leader = (struct char_data *) random_from_list(group->members);
    send_to_group(NULL, group, "%s has assumed leadership of the group.\r\n", GET_NAME(GROUP_LEADER(group)));
  } else if (group->members->iSize == 0)
    free_group(group);
 
  update_msdp_group(ch);
  if (ch->desc)
    MSDPFlush(ch->desc, eMSDP_GROUP);
}

void join_group(struct char_data *ch, struct group_data *group) {
  add_to_list(ch, group->members);

  if (group->leader == NULL)
    group->leader = ch;

  ch->group = group;

  if (IS_SET(group->group_flags, GROUP_NPC) && !IS_NPC(ch))
    REMOVE_BIT(GROUP_FLAGS(group), GROUP_NPC);

  if (group->leader == ch)
    send_to_group(NULL, group, "%s becomes leader of the group.\r\n", GET_NAME(ch));
  else
    send_to_group(NULL, group, "%s joins the group.\r\n", GET_NAME(ch));

  update_msdp_group(ch);
  if (ch->desc)
    MSDPFlush(ch->desc, eMSDP_GROUP);
}

/* mount related stuff */
void dismount_char(struct char_data *ch) {
  if (RIDING(ch)) {
    RIDDEN_BY(RIDING(ch)) = NULL;
    RIDING(ch) = NULL;
  }

  if (RIDDEN_BY(ch)) {
    RIDING(RIDDEN_BY(ch)) = NULL;
    RIDDEN_BY(ch) = NULL;
  }
}

void mount_char(struct char_data *ch, struct char_data *mount) {
  RIDING(ch) = mount;
  RIDDEN_BY(mount) = ch;
}

