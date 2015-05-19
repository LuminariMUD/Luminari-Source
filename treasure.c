/* *************************************************************************
 *  File:   treasure.c                                 Part of LuminariMUD *
 *  Usage:  functions for random treasure objects                          *
 *  Author: d20mud, ported to tba/luminari by Zusuk                        *
 ************************************************************************* *
 * This code is going through a rewrite, to make a more consistent system
 * across the mud for magic item generation (crafting, random treasure,
 * etc.)  Changes by Ornir.
 *************************************************************************/

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "interpreter.h"
#include "handler.h"
#include "db.h"
#include "spells.h"
#include "constants.h"
#include "dg_scripts.h"
#include "treasure.h"
#include "craft.h"

/***  utility functions ***/

/* some spells are not appropriate for expendable items, this simple
 function returns TRUE if the spell is OK, FALSE if not */
bool valid_item_spell(int spellnum) {
  /* just list exceptions NOTE if you add any exception here, better make
   sure wizards have another way of getting the spell
   * update: added 'research' so wizards have another way to acquire spells */
  switch (spellnum) {
    case SPELL_VENTRILOQUATE:
    case SPELL_MUMMY_DUST:
    case SPELL_DRAGON_KNIGHT:
    case SPELL_GREATER_RUIN:
    case SPELL_HELLBALL:
    case SPELL_EPIC_MAGE_ARMOR:
    case SPELL_EPIC_WARDING:
    case SPELL_FIRE_BREATHE:
    case SPELL_STENCH:
    case SPELL_ACID_SPLASH:
    case SPELL_RAY_OF_FROST:
    case SPELL_FSHIELD_DAM:
    case SPELL_CSHIELD_DAM:
    case SPELL_ASHIELD_DAM:
    case SPELL_DEATHCLOUD:
    case SPELL_ACID:
    case SPELL_INCENDIARY:
    case SPELL_FROST_BREATHE:
    case SPELL_LIGHTNING_BREATHE:
    case SPELL_UNUSED285:
    case SPELL_ANIMATE_DEAD:
    case SPELL_GREATER_ANIMATION:
    case SPELL_BLADES:
    case SPELL_CONTROL_WEATHER:
    case SPELL_I_DARKNESS:
      return FALSE;
  }
  return TRUE;
}

/* simple function to give a random metal type (currently unused) */
int choose_metal_material(void) {
  switch (dice(1, 12)) {
    case 1:
    case 2:
    case 3:
      return MATERIAL_BRONZE;
    case 4:
    case 5:
      return MATERIAL_STEEL;
    case 6:
      return MATERIAL_ALCHEMAL_SILVER;
    case 7:
      return MATERIAL_COLD_IRON;
    case 8:
      return MATERIAL_ADAMANTINE;
    case 9:
      return MATERIAL_MITHRIL;
    default:
      return MATERIAL_IRON;
  }
}

/* simple function to give a random precious metal type (currently unused) */
int choose_precious_metal_material(void) {
  switch (dice(1, 9)) {
    case 1:
    case 2:
      return MATERIAL_BRASS;
    case 3:
    case 4:
      return MATERIAL_SILVER;
    case 5:
      return MATERIAL_GOLD;
    case 6:
      return MATERIAL_PLATINUM;
    default:
      return MATERIAL_COPPER;
  }
}

/* simple function to give a random cloth type (currently unused) */
int choose_cloth_material(void) {
  switch (dice(1, 12)) {
    case 1:
    case 2:
    case 3:
      return MATERIAL_COTTON;
    case 4:
    case 5:
      return MATERIAL_WOOL;
    case 6:
      return MATERIAL_BURLAP;
    case 7:
      return MATERIAL_VELVET;
    case 8:
      return MATERIAL_SATIN;
    case 9:
      return MATERIAL_SILK;
    default:
      return MATERIAL_HEMP;
  }
}

/* determine appropriate stat bonus apply for this piece of gear */
int determine_stat_apply(int wear) {
  int stat = APPLY_NONE;

  switch (wear) {
    case WEAR_FINGER_R:
    case WEAR_FINGER_L:
      switch (rand_number(1, 7)) {
        case 1:
          stat = APPLY_WIS;
          break;
        case 2:
          stat = APPLY_SAVING_WILL;
          break;
        case 3:
          stat = APPLY_HIT;
          break;
        case 4:
          stat = APPLY_RES_FIRE;
          break;
        case 5:
          stat = APPLY_RES_PUNCTURE;
          break;
        case 6:
          stat = APPLY_RES_ILLUSION;
          break;
        case 7:
          stat = APPLY_RES_ENERGY;
          break;
      }
      break;
    case WEAR_NECK_1:
    case WEAR_NECK_2:
      switch (rand_number(1, 7)) {
        case 1:
          stat = APPLY_INT;
          break;
        case 2:
          stat = APPLY_SAVING_REFL;
          break;
        case 3:
          stat = APPLY_RES_COLD;
          break;
        case 4:
          stat = APPLY_RES_AIR;
          break;
        case 5:
          stat = APPLY_RES_FORCE;
          break;
        case 6:
          stat = APPLY_RES_MENTAL;
          break;
        case 7:
          stat = APPLY_RES_WATER;
          break;
      }
      break;
    case WEAR_WRIST_R:
    case WEAR_WRIST_L:
      switch (rand_number(1, 6)) {
        case 1:
          stat = APPLY_SAVING_FORT;
          break;
        case 2:
          stat = APPLY_MANA;
          break;
        case 3:
          stat = APPLY_RES_ELECTRIC;
          break;
        case 4:
          stat = APPLY_RES_UNHOLY;
          break;
        case 5:
          stat = APPLY_RES_SOUND;
          break;
        case 6:
          stat = APPLY_RES_LIGHT;
          break;
      }
      break;
    case WEAR_HOLD_1:
    case WEAR_HOLD_2:
    case WEAR_HOLD_2H:
      switch (rand_number(1, 3)) {
        case 1:
          stat = APPLY_INT;
          break;
        case 2:
          stat = APPLY_CHA;
          break;
        case 3:
          stat = APPLY_HIT;
          break;
      }
      break;
    case WEAR_FEET:
      switch (rand_number(1, 3)) {
        case 1:
          stat = APPLY_RES_POISON;
          break;
        case 2:
          stat = APPLY_DEX;
          break;
        case 3:
          stat = APPLY_MOVE;
          break;
      }
      break;
    case WEAR_HANDS:
      switch (rand_number(1, 3)) {
        case 1:
          stat = APPLY_RES_SLICE;
          break;
        case 2:
          stat = APPLY_STR;
          break;
        case 3:
          stat = APPLY_RES_DISEASE;
          break;
      }
      break;
    case WEAR_ABOUT:
      switch (rand_number(1, 3)) {
        case 1:
          stat = APPLY_RES_ACID;
          break;
        case 2:
          stat = APPLY_CHA;
          break;
        case 3:
          stat = APPLY_RES_NEGATIVE;
          break;
      }
      break;
    case WEAR_WAIST:
      switch (rand_number(1, 3)) {
        case 1:
          stat = APPLY_RES_HOLY;
          break;
        case 2:
          stat = APPLY_CON;
          break;
        case 3:
          stat = APPLY_RES_EARTH;
          break;
      }
      break;
    default:
      stat = APPLY_NONE;

  }

  return stat;
}

/* added this because the apply_X bonus is capped, stop it before
   it causes problems */
#define RANDOM_BONUS_CAP  127
/* function to adjust the bonus value based on the apply location */
/* called by random_bonus_value(), cp_modify_object_applies(), */
int adjust_bonus_value(int apply_location, int bonus) {
  int adjusted_bonus = bonus;
  switch (apply_location) {
    case APPLY_HIT:
    case APPLY_MOVE:
      adjusted_bonus = bonus * 12;
      break;
    /* no modifications */
    default:
      break;
  }
  return MIN(RANDOM_BONUS_CAP, adjusted_bonus);
}

/* assign bonus-types to the bonus */
/* called by: cp_modify_object_applies() */
int adjust_bonus_type(int apply_location) {
  switch (apply_location) {
    case APPLY_SAVING_FORT:
    case APPLY_SAVING_REFL:
    case APPLY_SAVING_WILL:
      return BONUS_TYPE_RESISTANCE;
    default:
      return BONUS_TYPE_ENHANCEMENT;
      break;
  }
}

/* function that returns bonus value based on apply-value and level */
/* called by award_random_crystal() */
int random_bonus_value(int apply_value, int level, int mod) {
  int bonus = MAX(1, (level / BONUS_FACTOR) + mod);
  return adjust_bonus_value(apply_value, bonus);
}

/* when grouped, determine random recipient from group */
struct char_data *find_treasure_recipient(struct char_data *ch) {
  struct group_data *group = NULL;
  struct char_data *target = NULL;

  /* assign group data */
  if ((group = ch->group) == NULL)
    return ch;

  /* pick random group member */
  target = random_from_list(group->members);

  /* same room? */
  if (IN_ROOM(ch) != IN_ROOM(target))
    target = ch;

  return (target);
}

/***  primary functions ***/

/* this function determines whether the character will get treasure or not
 *   for example, called before make_corpse() when killing a mobile */
void determine_treasure(struct char_data *ch, struct char_data *mob) {
  int roll = dice(1, 100);
  int gold = 0;
  int level = 0;
  char buf[MEDIUM_STRING] = {'\0'};
  int grade = GRADE_MUNDANE;

  if (IS_NPC(ch))
    return;
  if (!IS_NPC(mob))
    return;

  gold = dice(1, GET_LEVEL(mob)) * 10;
  level = GET_LEVEL(mob);

  if (level >= 20) {
    grade = GRADE_MAJOR;
  } else if (level >= 16) {
    if (roll >= 61)
      grade = GRADE_MAJOR;
    else
      grade = GRADE_MEDIUM;
  } else if (level >= 12) {
    if (roll >= 81)
      grade = GRADE_MAJOR;
    else if (roll >= 11)
      grade = GRADE_MEDIUM;
    else
      grade = GRADE_MINOR;
  } else if (level >= 8) {
    if (roll >= 96)
      grade = GRADE_MAJOR;
    else if (roll >= 31)
      grade = GRADE_MEDIUM;
    else
      grade = GRADE_MINOR;
  } else if (level >= 4) {
    if (roll >= 76)
      grade = GRADE_MEDIUM;
    else if (roll >= 16)
      grade = GRADE_MINOR;
    else
      grade = GRADE_MUNDANE;
  } else {
    if (roll >= 96)
      grade = GRADE_MEDIUM;
    else if (roll >= 41)
      grade = GRADE_MINOR;
    else
      grade = GRADE_MUNDANE;
  }

  if (dice(1, 100) <= MAX(TREASURE_PERCENT, HAPPY_TREASURE)) {
    award_magic_item(dice(1, 2), ch, level, grade);
    sprintf(buf, "\tYYou have found %d coins hidden on $N's corpse!\tn", gold);
    act(buf, FALSE, ch, 0, mob, TO_CHAR);
    sprintf(buf, "$n \tYhas has found %d coins hidden on $N's corpse!\tn", gold);
    act(buf, FALSE, ch, 0, mob, TO_NOTVICT);
    GET_GOLD(ch) += gold;
    /* does not split this gold, maybe change later */
  }
}

/* character should get treasure, roll dice for what items to give out */
void award_magic_item(int number, struct char_data *ch, int level, int grade) {
  int i = 0;
  int roll = 0;
  if (number <= 0)
    number = 1;

  if (number >= 50)
    number = 50;

 /* crystals drop 5% of the time.
 * scrolls/wands/potions/staves drop 40% of the time. (each 10%)
 * trinkets (bracelets, rings, etc. including cloaks, boots and gloves) drop 20% of the time.
 * armor (head, arms, legs, body) drop 25% of the time.
 * weapons drop 10% of the time. */
  for (i = 0; i < number; i++) {
    roll = dice(1, 100);
    if (roll <= 5)
      award_random_crystal(ch, level);
    else if (roll <= 15)
      award_magic_weapon(ch, grade, level);
    else if (roll <= 55) {
      switch (dice(1,4)) {
        case 1:
          award_expendable_item(ch, grade, TYPE_SCROLL);
          break;
        case 2:
          award_expendable_item(ch, grade, TYPE_POTION);
          break;
        case 3:
          award_expendable_item(ch, grade, TYPE_WAND);
          break;
        case 4:
          award_expendable_item(ch, grade, TYPE_STAFF);
          break;
      }
    } else if (roll <= 75)
        award_misc_magic_item(ch, grade, level);
      else
        award_magic_armor(ch, grade, level, -1);
  }
}

int random_apply_value(void) {
  int val = APPLY_NONE;

  /* There will be different groupings based on item type and wear location,
   * for example weapons will get hit/dam bonus (the +) and armor will get
   * ac_apply_new bonus (the +). */
  switch (dice(1, 12)) {
    case 1:
      val = APPLY_HIT;
      break;
    case 2:
      val = APPLY_STR;
      break;
    case 3:
      val = APPLY_CON;
      break;
    case 4:
      val = APPLY_DEX;
      break;
    case 5:
      val = APPLY_INT;
      break;
    case 6:
      val = APPLY_WIS;
      break;
    case 7:
      val = APPLY_CHA;
      break;
    case 8:
      val = APPLY_MOVE;
      break;
    case 9:
      val = APPLY_SAVING_FORT;
      break;
    case 10:
      val = APPLY_SAVING_REFL;
      break;
    case 11:
      val = APPLY_SAVING_WILL;
      break;
    default:
      switch (rand_number(1, 20)) {
        case 1:
          val = APPLY_RES_FIRE;
          break;
        case 2:
          val = APPLY_RES_COLD;
          break;
        case 3:
          val = APPLY_RES_AIR;
          break;
        case 4:
          val = APPLY_RES_EARTH;
          break;
        case 5:
          val = APPLY_RES_ACID;
          break;
        case 6:
          val = APPLY_RES_HOLY;
          break;
        case 7:
          val = APPLY_RES_ELECTRIC;
          break;
        case 8:
          val = APPLY_RES_UNHOLY;
          break;
        case 9:
          val = APPLY_RES_SLICE;
          break;
        case 10:
          val = APPLY_RES_PUNCTURE;
          break;
        case 11:
          val = APPLY_RES_FORCE;
          break;
        case 12:
          val = APPLY_RES_SOUND;
          break;
        case 13:
          val = APPLY_RES_POISON;
          break;
        case 14:
          val = APPLY_RES_DISEASE;
          break;
        case 15:
          val = APPLY_RES_NEGATIVE;
          break;
        case 16:
          val = APPLY_RES_ILLUSION;
          break;
        case 17:
          val = APPLY_RES_MENTAL;
          break;
        case 18:
          val = APPLY_RES_LIGHT;
          break;
        case 19:
          val = APPLY_RES_ENERGY;
          break;
        case 20:
          val = APPLY_RES_WATER;
          break;
      }
  }
  return val;
}

/* function to create a random crystal */
void award_random_crystal(struct char_data *ch, int level) {
  int color1 = -1, color2 = -1, desc = -1, roll = 0;
  struct obj_data *obj = NULL;
  char buf[MEDIUM_STRING] = {'\0'};

  if ((obj = read_object(CRYSTAL_PROTOTYPE, VIRTUAL)) == NULL) {
    log("SYSERR:  get_random_crystal read_object returned NULL");
    return;
  }

  /* this is just to make sure the item is set correctly */
  GET_OBJ_TYPE(obj) = ITEM_CRYSTAL;
  GET_OBJ_LEVEL(obj) = rand_number(6, level);
  level = GET_OBJ_LEVEL(obj); /* for determining bonus */
  GET_OBJ_COST(obj) = GET_OBJ_LEVEL(obj) * 100;
  GET_OBJ_MATERIAL(obj) = MATERIAL_CRYSTAL;

  /* set a random apply value */
  obj->affected[0].location = random_apply_value();
  /* determine bonus */
  /* this is deprecated, level determines modifier now (in craft.c) */
  obj->affected[0].modifier = level / 5 + rand_number(0, 2);
  obj->affected[0].bonus_type = adjust_bonus_type(obj->affected[0].location);

  /* random color(s) and description */
  color1 = rand_number(0, NUM_A_COLORS);
  color2 = rand_number(0, NUM_A_COLORS);
  while (color2 == color1)
    color2 = rand_number(0, NUM_A_COLORS);
  desc = rand_number(0, NUM_A_CRYSTAL_DESCS);

  roll = dice(1, 100);

  // two colors and descriptor
  if (roll >= 91) {
    sprintf(buf, "crystal %s %s %s", colors[color1], colors[color2],
            crystal_descs[desc]);
    obj->name = strdup(buf);
    sprintf(buf, "a %s, %s and %s crystal", crystal_descs[desc],
            colors[color1], colors[color2]);
    obj->short_description = strdup(buf);
    sprintf(buf, "A %s, %s and %s crystal lies here.", crystal_descs[desc],
            colors[color1], colors[color2]);
    obj->description = strdup(buf);

    // one color and descriptor
  } else if (roll >= 66) {
    sprintf(buf, "crystal %s %s", colors[color1], crystal_descs[desc]);
    obj->name = strdup(buf);
    sprintf(buf, "a %s %s crystal", crystal_descs[desc], colors[color1]);
    obj->short_description = strdup(buf);
    sprintf(buf, "A %s %s crystal lies here.", crystal_descs[desc],
            colors[color1]);
    obj->description = strdup(buf);

    // two colors no descriptor
  } else if (roll >= 41) {
    sprintf(buf, "crystal %s %s", colors[color1], colors[color2]);
    obj->name = strdup(buf);
    sprintf(buf, "a %s and %s crystal", colors[color1], colors[color2]);
    obj->short_description = strdup(buf);
    sprintf(buf, "A %s and %s crystal lies here.", colors[color1], colors[color2]);
    obj->description = strdup(buf);
  } else if (roll >= 21) {// one color no descriptor
    sprintf(buf, "crystal %s", colors[color1]);
    obj->name = strdup(buf);
    sprintf(buf, "a %s crystal", colors[color1]);
    obj->short_description = strdup(buf);
    sprintf(buf, "A %s crystal lies here.", colors[color1]);
    obj->description = strdup(buf);

    // descriptor only
  } else {
    sprintf(buf, "crystal %s", crystal_descs[desc]);
    obj->name = strdup(buf);
    sprintf(buf, "a %s crystal", crystal_descs[desc]);
    obj->short_description = strdup(buf);
    sprintf(buf, "A %s crystal lies here.", crystal_descs[desc]);
    obj->description = strdup(buf);
  }

  obj_to_char(obj, ch);
}

/* awards potions or scroll or wand or staff */
/* reminder, stock:
   obj value 0:  spell level
   obj value 1:  potion/scroll - spell #1; staff/wand - max charges
   obj value 2:  potion/scroll - spell #2; staff/wand - charges remaining
   obj value 3:  potion/scroll - spell #3; staff/wand - spell #1 */
void award_expendable_item(struct char_data *ch, int grade, int type) {
  int class = CLASS_UNDEFINED, spell_level = 1, spell_num = SPELL_RESERVED_DBC;
  int color1 = 0, color2 = 0, desc = 0, roll = dice(1, 100), i = 0;
  struct obj_data *obj = NULL;
  char keywords[MEDIUM_STRING] = {'\0'}, buf[MAX_STRING_LENGTH] = {'\0'};

  /* first determine which class the scroll will be,
   then determine what level spell the scroll will be */
  switch (rand_number(0, NUM_CLASSES - 1)) {
    case CLASS_CLERIC:
      class = CLASS_CLERIC;
      break;
    case CLASS_DRUID:
      class = CLASS_DRUID;
      break;
    case CLASS_SORCERER:
      class = CLASS_SORCERER;
      break;
    case CLASS_PALADIN:
      class = CLASS_PALADIN;
      break;
    case CLASS_RANGER:
      class = CLASS_RANGER;
      break;
    case CLASS_BARD:
      class = CLASS_BARD;
      break;
    default: /* we favor wizards heavily since they NEED scrolls */
      class = CLASS_WIZARD;
      break;
  }

  /* determine level */
  /* -note- that this is caster level, not spell-circle */
  switch (grade) {
    case GRADE_MUNDANE:
      spell_level = rand_number(1, 5);
      break;
    case GRADE_MINOR:
      spell_level = rand_number(6, 10);
      break;
    case GRADE_MEDIUM:
      spell_level = rand_number(11, 14);
      break;
    default:
      spell_level = rand_number(15, 20);
      break;
  }

  /* Loop through spell list randomly! conditions of exit:
     - invalid spell (sub-function)
     - does not match class
     - does not match level
   */
  int loop_counter = 0; // just in case
  do {
    spell_num = rand_number(1, NUM_SPELLS - 1);
    loop_counter++;

    if (loop_counter >= 999)
      return;
  } while (spell_level < spell_info[spell_num].min_level[class] ||
          !valid_item_spell(spell_num));

  /* first assign two random colors for usage */
  color1 = rand_number(0, NUM_A_COLORS);
  color2 = rand_number(0, NUM_A_COLORS);
  /* make sure they are not the same colors */
  while (color2 == color1)
    color2 = rand_number(0, NUM_A_COLORS);

  /* load prototype */

  if ((obj = read_object(ITEM_PROTOTYPE, VIRTUAL)) == NULL) {
    log("SYSERR:  award_expendable_item returned NULL");
    return;
  }

  switch (type) {
    case TYPE_POTION:
      /* assign a description (potions only) */
      desc = rand_number(0, NUM_A_POTION_DESCS);

      // two colors and descriptor
      if (roll >= 91) {
        sprintf(keywords, "potion-%s", spell_info[spell_num].name);
        for (i = 0; i < strlen(keywords); i++)
          if (keywords[i] == ' ')
            keywords[i] = '-';
        sprintf(buf, "vial potion %s %s %s %s %s", colors[color1], colors[color2],
                potion_descs[desc], spell_info[spell_num].name, keywords);
        obj->name = strdup(buf);
        sprintf(buf, "a glass vial filled with a %s, %s and %s liquid",
                potion_descs[desc], colors[color1], colors[color2]);
        obj->short_description = strdup(buf);
        sprintf(buf, "A glass vial filled with a %s, %s and %s liquid lies here.",
                potion_descs[desc], colors[color1], colors[color2]);
        obj->description = strdup(buf);

        // one color and descriptor
      } else if (roll >= 66) {
        sprintf(keywords, "potion-%s", spell_info[spell_num].name);
        for (i = 0; i < strlen(keywords); i++)
          if (keywords[i] == ' ')
            keywords[i] = '-';
        sprintf(buf, "vial potion %s %s %s %s", colors[color1],
                potion_descs[desc], spell_info[spell_num].name, keywords);
        obj->name = strdup(buf);
        sprintf(buf, "a glass vial filled with a %s %s liquid",
                potion_descs[desc], colors[color1]);
        obj->short_description = strdup(buf);
        sprintf(buf, "A glass vial filled with a %s and %s liquid lies here.",
                potion_descs[desc], colors[color1]);
        obj->description = strdup(buf);

        // two colors no descriptor
      } else if (roll >= 41) {
        sprintf(keywords, "potion-%s", spell_info[spell_num].name);
        for (i = 0; i < strlen(keywords); i++)
          if (keywords[i] == ' ')
            keywords[i] = '-';
        sprintf(buf, "vial potion %s %s %s %s", colors[color1], colors[color2],
                spell_info[spell_num].name, keywords);
        obj->name = strdup(buf);
        sprintf(buf, "a glass vial filled with a %s and %s liquid", colors[color1],
                colors[color2]);
        obj->short_description = strdup(buf);
        sprintf(buf, "A glass vial filled with a %s and %s liquid lies here.",
                colors[color1], colors[color2]);
        obj->description = strdup(buf);

        // one color no descriptor
      } else {
        sprintf(keywords, "potion-%s", spell_info[spell_num].name);
        for (i = 0; i < strlen(keywords); i++)
          if (keywords[i] == ' ')
            keywords[i] = '-';
        sprintf(buf, "vial potion %s %s %s", colors[color1],
                spell_info[spell_num].name, keywords);
        obj->name = strdup(buf);
        sprintf(buf, "a glass vial filled with a %s liquid",
                colors[color1]);
        obj->short_description = strdup(buf);
        sprintf(buf, "A glass vial filled with a %s liquid lies here.",
                colors[color1]);
        obj->description = strdup(buf);
      }

      GET_OBJ_VAL(obj, 0) = spell_level;
      GET_OBJ_VAL(obj, 1) = spell_num;

      GET_OBJ_MATERIAL(obj) = MATERIAL_GLASS;
      GET_OBJ_COST(obj) = MIN(1000, 5 * spell_level);
      GET_OBJ_LEVEL(obj) = dice(1, spell_level);
      break;

    case TYPE_WAND:
      sprintf(keywords, "wand-%s", spell_info[spell_num].name);
      for (i = 0; i < strlen(keywords); i++)
        if (keywords[i] == ' ')
          keywords[i] = '-';

      sprintf(buf, "wand wooden runes %s %s %s", colors[color1],
              spell_info[spell_num].name, keywords);
      obj->name = strdup(buf);
      sprintf(buf, "a wooden wand covered in %s runes", colors[color1]);
      obj->short_description = strdup(buf);
      sprintf(buf, "A wooden wand covered in %s runes lies here.", colors[color1]);
      obj->description = strdup(buf);

      GET_OBJ_VAL(obj, 0) = spell_level;
      GET_OBJ_VAL(obj, 1) = 10;
      GET_OBJ_VAL(obj, 2) = dice(1, 10);
      GET_OBJ_VAL(obj, 3) = spell_num;

      GET_OBJ_SIZE(obj) = SIZE_SMALL;
      GET_OBJ_COST(obj) = MIN(1000, 50 * spell_level);
      GET_OBJ_MATERIAL(obj) = MATERIAL_WOOD;
      GET_OBJ_TYPE(obj) = ITEM_WAND;
      SET_BIT_AR(GET_OBJ_WEAR(obj), ITEM_WEAR_HOLD);
      GET_OBJ_LEVEL(obj) = dice(1, spell_level);
      break;

    case TYPE_STAFF:
      sprintf(keywords, "staff-%s", spell_info[spell_num].name);
      for (i = 0; i < strlen(keywords); i++)
        if (keywords[i] == ' ')
          keywords[i] = '-';

      sprintf(buf, "staff wooden runes %s %s %s", colors[color1],
              spell_info[spell_num].name, keywords);
      obj->name = strdup(buf);
      sprintf(buf, "a wooden staff covered in %s runes", colors[color1]);
      obj->short_description = strdup(buf);
      sprintf(buf, "A wooden staff covered in %s runes lies here.",
              colors[color1]);
      obj->description = strdup(buf);

      GET_OBJ_VAL(obj, 0) = spell_level;
      GET_OBJ_VAL(obj, 1) = 8;
      GET_OBJ_VAL(obj, 2) = dice(1, 8);
      GET_OBJ_VAL(obj, 3) = spell_num;

      GET_OBJ_COST(obj) = MIN(1000, 110 * spell_level);
      GET_OBJ_MATERIAL(obj) = MATERIAL_WOOD;
      GET_OBJ_TYPE(obj) = ITEM_STAFF;
      SET_BIT_AR(GET_OBJ_WEAR(obj), ITEM_WEAR_HOLD);
      GET_OBJ_LEVEL(obj) = dice(1, spell_level);
      break;

    default: // Type-Scrolls
      sprintf(keywords, "scroll-%s", spell_info[spell_num].name);
      for (i = 0; i < strlen(keywords); i++)
        if (keywords[i] == ' ')
          keywords[i] = '-';

      sprintf(buf, "scroll ink %s %s %s", colors[color1],
              spell_info[spell_num].name, keywords);
      obj->name = strdup(buf);
      sprintf(buf, "a scroll written in %s ink", colors[color1]);
      obj->short_description = strdup(buf);
      sprintf(buf, "A scroll written in %s ink lies here.", colors[color1]);
      obj->description = strdup(buf);

      GET_OBJ_VAL(obj, 0) = spell_level;
      GET_OBJ_VAL(obj, 1) = spell_num;

      GET_OBJ_COST(obj) = MIN(1000, 10 * spell_level);
      GET_OBJ_MATERIAL(obj) = MATERIAL_PAPER;
      GET_OBJ_TYPE(obj) = ITEM_SCROLL;
      GET_OBJ_LEVEL(obj) = dice(1, spell_level);
      break;
  }

  REMOVE_BIT_AR(GET_OBJ_EXTRA(obj), ITEM_MOLD);
  SET_BIT_AR(GET_OBJ_EXTRA(obj), ITEM_MAGIC);

  obj_to_char(obj, ch);

  send_to_char(ch, "\tYYou have found %s in a nearby lair!\tn\r\n", obj->short_description);
  sprintf(buf, "$n \tYhas found %s in a nearby lair!\tn", obj->short_description);
  act(buf, FALSE, ch, 0, ch, TO_NOTVICT);
}

/* this is a very simplified version of this function, the original version was
 incomplete and creating some very strange gear with some crazy stats.  The
 original version is right below this */
void cp_modify_object_applies(struct char_data *ch, struct obj_data *obj,
        int rare_grade, int level, int cp_type) {
  int bonus_value = 0, bonus_location = APPLY_NONE;
  bool has_enhancement = FALSE;
  char buf[MAX_STRING_LENGTH] = {'\0'};

  level = (rand_number(1, level)); /* this is as random as it gets right now */
  bonus_value = level / 5; if (bonus_value <= 0) bonus_value = 1;

  /* items that will only get an enhancement bonus */
  if (CAN_WEAR(obj, ITEM_WEAR_WIELD) || CAN_WEAR(obj, ITEM_WEAR_SHIELD) ||
      CAN_WEAR(obj, ITEM_WEAR_HEAD) || CAN_WEAR(obj, ITEM_WEAR_BODY) ||
      CAN_WEAR(obj, ITEM_WEAR_LEGS) || CAN_WEAR(obj, ITEM_WEAR_ARMS)
      ) {
    GET_OBJ_VAL(obj, 4) = level / 5;
    has_enhancement = TRUE;
  }
  else if (CAN_WEAR(obj, ITEM_WEAR_FINGER)) {
    bonus_location = determine_stat_apply(WEAR_FINGER_R);
  }
  else if (CAN_WEAR(obj, ITEM_WEAR_NECK)) {
    bonus_location = determine_stat_apply(WEAR_NECK_1);
  }
  else if (CAN_WEAR(obj, ITEM_WEAR_FEET)) {
    bonus_location = determine_stat_apply(WEAR_FEET);
  }
  else if (CAN_WEAR(obj, ITEM_WEAR_HANDS)) {
    bonus_location = determine_stat_apply(WEAR_HANDS);
  }
  else if (CAN_WEAR(obj, ITEM_WEAR_ABOUT)) {
    bonus_location = determine_stat_apply(WEAR_ABOUT);
  }
  else if (CAN_WEAR(obj, ITEM_WEAR_WAIST)) {
    bonus_location = determine_stat_apply(WEAR_WAIST);
  }
  else if (CAN_WEAR(obj, ITEM_WEAR_WRIST)) {
    bonus_location = determine_stat_apply(WEAR_WRIST_R);
  }
  else if (CAN_WEAR(obj, ITEM_WEAR_HOLD)) {
    bonus_location = determine_stat_apply(WEAR_HOLD_1);
  }

  if (!has_enhancement) {
    obj->affected[0].location = bonus_location;
    obj->affected[0].modifier = adjust_bonus_value(bonus_location, bonus_value);
    obj->affected[0].bonus_type = adjust_bonus_type(bonus_location);
  }

  GET_OBJ_LEVEL(obj) = level;
  GET_OBJ_COST(obj) = GET_OBJ_LEVEL(obj) * 100;  // set value
  REMOVE_BIT_AR(GET_OBJ_EXTRA(obj), ITEM_MOLD);  // make sure not mold
  if (level >= 5)
    SET_BIT_AR(GET_OBJ_EXTRA(obj), ITEM_MAGIC);  // add magic tag

  obj_to_char(obj, ch); // deliver object

  send_to_char(ch, "\tYYou have found %s in a nearby lair!\tn\r\n", obj->short_description);
  sprintf(buf, "$n \tYhas found %s in a nearby lair!\tn", obj->short_description);
  act(buf, FALSE, ch, 0, ch, TO_NOTVICT);
}
/* this is ornir's original version, taken out until he has time to finish it */
/* Here is where the significant changes start (cp = Creation Points) - Ornir */
/*void cp_modify_object_applies(struct char_data *ch, struct obj_data *obj,
        int rare_grade, int level, int cp_type) {
  int max_slots = 0;
  int current_slot = 1;
  int current_cp = 0;
  int max_bonus = 0;
  int max_bonus_cp_cost = 0;
  int bonus_location = 0;
  int bonus_value = 0;
  int i = 0;
  bool duplicate_affect = FALSE;
  char buf[MAX_STRING_LENGTH] = {'\0'};

  switch (cp_type) {
    case CP_TYPE_MISC:
      max_slots = 1; // Trinkets
      break;
    case CP_TYPE_ARMOR:
      max_slots = 2; // AC_APPLY takes 1!
      break;
    case CP_TYPE_WEAPON:
      max_slots = 3; // TOHIT/TODAM takes 2!
      break;
    default:
      break;
  }

  // Get the base CP for the item based on the level.
  current_cp = CP_BASE_VALUE(level);

  // Add bonus CP and slots for rarity
  current_cp += rare_grade * 100;
  if (rare_grade >= 2)
    max_slots += 1;

  // DEBUG
  if (GET_LEVEL(ch) >= LVL_IMMORTAL)
    send_to_char(ch, "\tyItem created, level: %d CP: %d\tn\r\n", level, current_cp);

  // Add bonuses, one bonus to each slot.
  while (current_slot <= max_slots) {

    // Determine bonus location, check if first bonus too
    switch (cp_type) {
      case CP_TYPE_ARMOR:
        // Since this is armor, the first bonus is ALWAYS AC
        // Note: this is just a marker now, we have enhancement bonus instead
        if(current_slot == 1)
          bonus_location = APPLY_AC_NEW;
        else
          bonus_location = random_armor_apply_value();
        break;

      case CP_TYPE_WEAPON:
        // Since this is a weapon, the first 2 bonuses are TOHIT and TODAM
        // Note: this is just a marker now, we have enhancement bonus instead
        if(current_slot == 1)
          bonus_location = APPLY_HITROLL; // We Apply TODAM later...
        else
          bonus_location = random_weapon_apply_value();
        break;
      default: // misc types
        bonus_location = random_apply_value();
        break;
    }

    // Check for duplicate affects
    duplicate_affect = FALSE;
    for(i = 0; i > MAX_OBJ_AFFECT; i++){
      if(obj->affected[i].location == bonus_location){
        duplicate_affect = TRUE;
        break;
      }
    }
    // Only add the affect if it is not a duplicate affect location!
    if(duplicate_affect != TRUE) {

      // Based on CP remaining, how HIGH a bonus can we get here?
      switch (cp_type) {
        case CP_TYPE_ARMOR:
          max_bonus = TREASURE_MAX_BONUS;
          break;
        default:
          max_bonus = TREASURE_MAX_BONUS + 5;
          break;
      }
      max_bonus_cp_cost = CP_COST(max_bonus);

      while ((max_bonus > 0) && (max_bonus_cp_cost > current_cp)) {
        max_bonus--;
        max_bonus_cp_cost = CP_COST(max_bonus);
      }

      // If we CAN apply a bonus, based on CP, then determine value.
      if (max_bonus > 0) {
        // Choose a bonus value from 1 to that bonus amount.
        bonus_value = rand_number(1, max_bonus);
        current_cp -= CP_COST(bonus_value);
        if (cp_type == CP_TYPE_WEAPON && bonus_location == APPLY_HITROLL) {
          GET_OBJ_VAL(obj, 4) = adjust_bonus_value(APPLY_DAMROLL, bonus_value); // Set enhancement bonus
          current_slot++;
          // added this code to handle armor enhancement -zusuk
        } else if (cp_type == CP_TYPE_ARMOR && bonus_location == APPLY_AC_NEW) {
          // it doesn't matter we're sending APPLY_DAMROLL here
          GET_OBJ_VAL(obj, 4) = adjust_bonus_value(APPLY_DAMROLL, bonus_value); // Set enhancement bonus.
          //current_slot++;
        } else {
          obj->affected[current_slot - 1].location = bonus_location;
          obj->affected[current_slot - 1].modifier = adjust_bonus_value(bonus_location, bonus_value);
          obj->affected[current_slot - 1].bonus_type = adjust_bonus_type(bonus_location);
        }

      }
    }
    current_slot++;
  } // end while

  GET_OBJ_LEVEL(obj) = level;
  GET_OBJ_COST(obj) = GET_OBJ_LEVEL(obj) * 100;  // set value
  REMOVE_BIT_AR(GET_OBJ_EXTRA(obj), ITEM_MOLD);  // make sure not mold
  if (level >= 5)
    SET_BIT_AR(GET_OBJ_EXTRA(obj), ITEM_MAGIC);  // add magic tag

  obj_to_char(obj, ch); // deliver object

  send_to_char(ch, "\tYYou have found %s in a nearby lair!\tn\r\n", obj->short_description);
  sprintf(buf, "$n \tYhas found %s in a nearby lair!\tn", obj->short_description);
  act(buf, FALSE, ch, 0, ch, TO_NOTVICT);
}
*/

/* Give away random magic armor, new method by Ornir
 * (includes:  body/head/legs/arms/shield)
 * 1)  determine armor type
 * 2)  determine material
 * 3)  determine rarity
 * 3)  determine Creation Points
 * 4)  determine AC bonus (Always first stat...)
 * 5)  craft description based on object and bonuses */
void award_magic_armor(struct char_data *ch, int grade, int moblevel, int wear_slot) {
  struct obj_data *obj = NULL;
  int vnum = -1, material = MATERIAL_BRONZE, roll = 0, crest_num = 0;
  int rare_grade = 0, color1 = 0, color2 = 0, level = 0, found_slot = FALSE;
  char desc[MEDIUM_STRING] = {'\0'}, armor_name[MEDIUM_STRING] = {'\0'};
  char keywords[MEDIUM_STRING] = {'\0'};

  /* determine if rare or not */
  roll = dice(1, 100);
  if (roll == 1) {
    rare_grade = 3;
    sprintf(desc, "\tM[Mythical]\tn ");
  } else if (roll <= 6) {
    rare_grade = 2;
    sprintf(desc, "\tY[Legendary]\tn ");
  } else if (roll <= 16) {
    rare_grade = 1;
    sprintf(desc, "\tG[Rare]\tn ");
  }

  /* attempt to find wear_slot */
  switch (wear_slot) {
    case WEAR_BODY:
      switch (rand_number(1, 9)) {
          /* body pieces */
        case 1:
          vnum = PLATE_BODY;
          material = MATERIAL_BRONZE;
          sprintf(desc, "%sa suit of", desc);
          sprintf(armor_name, "plate mail armor");
          found_slot = TRUE;
          break;
        case 2:
          vnum = HALFPLATE_BODY;
          material = MATERIAL_BRONZE;
          sprintf(desc, "%sa suit of", desc);
          sprintf(armor_name, "half plate armor");
          found_slot = TRUE;
          break;
        case 3:
          vnum = SPLINT_BODY;
          material = MATERIAL_BRONZE;
          sprintf(desc, "%sa suit of", desc);
          sprintf(armor_name, "splint mail armor");
          found_slot = TRUE;
          break;
        case 4:
          vnum = BREASTPLATE_BODY;
          material = MATERIAL_BRONZE;
          sprintf(desc, "%sa suit of", desc);
          sprintf(armor_name, "breastplate armor");
          found_slot = TRUE;
          break;
        case 5:
          vnum = CHAIN_BODY;
          material = MATERIAL_BRONZE;
          sprintf(desc, "%sa suit of", desc);
          sprintf(armor_name, "chain mail armor");
          found_slot = TRUE;
          break;
        case 6:
          vnum = STUD_LEATHER_BODY;
          material = MATERIAL_LEATHER;
          sprintf(desc, "%sa suit of", desc);
          sprintf(armor_name, "studded armor");
          found_slot = TRUE;
          break;
        case 7:
          vnum = LEATHER_BODY;
          material = MATERIAL_LEATHER;
          sprintf(desc, "%sa suit of", desc);
          sprintf(armor_name, "armor");
          found_slot = TRUE;
          break;
        case 8:
          vnum = PADDED_BODY;
          material = MATERIAL_COTTON;
          sprintf(desc, "%sa suit of", desc);
          sprintf(armor_name, "padded armor");
          found_slot = TRUE;
          break;
        case 9:
          vnum = CLOTH_BODY;
          material = MATERIAL_COTTON;
          sprintf(desc, "%ssome", desc);
          sprintf(armor_name, "robes");
          found_slot = TRUE;
          break;
        default:
          found_slot = FALSE;
          break;
      }
      break;
    case WEAR_LEGS:
      switch (rand_number(27, 34)) {
          /* leg pieces */
        case 27:
          vnum = PLATE_LEGS;
          material = MATERIAL_BRONZE;
          sprintf(desc, "%sa set of", desc);
          sprintf(armor_name, "plate mail greaves");
          found_slot = TRUE;
          break;
        case 28:
          vnum = HALFPLATE_LEGS;
          material = MATERIAL_BRONZE;
          sprintf(desc, "%sa set of", desc);
          sprintf(armor_name, "half plate greaves");
          found_slot = TRUE;
          break;
        case 29:
          vnum = SPLINT_LEGS;
          material = MATERIAL_BRONZE;
          sprintf(desc, "%sa set of", desc);
          sprintf(armor_name, "splint mail greaves");
          found_slot = TRUE;
          break;
        case 30:
          vnum = CHAIN_LEGS;
          material = MATERIAL_BRONZE;
          sprintf(desc, "%sa set of", desc);
          sprintf(armor_name, "chain mail leggings");
          found_slot = TRUE;
          break;
        case 31:
          vnum = STUD_LEATHER_LEGS;
          material = MATERIAL_LEATHER;
          sprintf(desc, "%sa set of", desc);
          sprintf(armor_name, "studded leggings");
          found_slot = TRUE;
          break;
        case 32:
          vnum = LEATHER_LEGS;
          material = MATERIAL_LEATHER;
          sprintf(desc, "%sa set of", desc);
          sprintf(armor_name, "cuisses");
          found_slot = TRUE;
          break;
        case 33:
          vnum = PADDED_LEGS;
          material = MATERIAL_COTTON;
          sprintf(desc, "%sa set of", desc);
          sprintf(armor_name, "padded armor leggings");
          found_slot = TRUE;
          break;
        case 34:
          vnum = CLOTH_LEGS;
          material = MATERIAL_COTTON;
          sprintf(desc, "%sa set of", desc);
          sprintf(armor_name, "cloth pants");
          found_slot = TRUE;
          break;
        default:
          found_slot = FALSE;
          break;
      }
      break;
    case WEAR_ARMS:
      switch (rand_number(19, 26)) {
        case 19:
          vnum = PLATE_ARMS;
          material = MATERIAL_BRONZE;
          sprintf(desc, "%sa set of", desc);
          sprintf(armor_name, "plate mail vambraces");
          found_slot = TRUE;
          break;
        case 20:
          vnum = HALFPLATE_ARMS;
          material = MATERIAL_BRONZE;
          sprintf(desc, "%sa set of", desc);
          sprintf(armor_name, "half plate vambraces");
          found_slot = TRUE;
          break;
        case 21:
          vnum = SPLINT_ARMS;
          material = MATERIAL_BRONZE;
          sprintf(desc, "%sa set of", desc);
          sprintf(armor_name, "splint mail vambraces");
          found_slot = TRUE;
          break;
        case 22:
          vnum = CHAIN_ARMS;
          material = MATERIAL_BRONZE;
          sprintf(desc, "%sa set of", desc);
          sprintf(armor_name, "chain mail sleeves");
          found_slot = TRUE;
          break;
        case 23:
          vnum = STUD_LEATHER_ARMS;
          material = MATERIAL_LEATHER;
          sprintf(desc, "%sa set of", desc);
          sprintf(armor_name, "studded sleeves");
          found_slot = TRUE;
          break;
        case 24:
          vnum = LEATHER_ARMS;
          material = MATERIAL_LEATHER;
          sprintf(desc, "%sa set of", desc);
          sprintf(armor_name, "sleeves");
          found_slot = TRUE;
          break;
        case 25:
          vnum = PADDED_ARMS;
          material = MATERIAL_COTTON;
          sprintf(desc, "%sa set of", desc);
          sprintf(armor_name, "padded armor sleeves");
          found_slot = TRUE;
          break;
        case 26:
          vnum = CLOTH_ARMS;
          material = MATERIAL_COTTON;
          sprintf(desc, "%sa set of", desc);
          sprintf(armor_name, "cloth sleeves");
          found_slot = TRUE;
          break;
        default:
          found_slot = FALSE;
          break;
      }
      break;
    case WEAR_HEAD:
      switch (rand_number(10, 18)) {
        case 10:
          vnum = PLATE_HELM;
          material = MATERIAL_BRONZE;
          sprintf(desc, "%sa", desc);
          sprintf(armor_name, "plate mail helm");
          found_slot = TRUE;
          break;
        case 11:
          vnum = HALFPLATE_HELM;
          material = MATERIAL_BRONZE;
          sprintf(desc, "%sa", desc);
          sprintf(armor_name, "half plate helm");
          found_slot = TRUE;
          break;
        case 12:
          vnum = SPLINT_HELM;
          material = MATERIAL_BRONZE;
          sprintf(desc, "%sa", desc);
          sprintf(armor_name, "splint mail helm");
          found_slot = TRUE;
          break;
        case 13:
          vnum = PIECEPLATE_HELM;
          material = MATERIAL_BRONZE;
          sprintf(desc, "%sa", desc);
          sprintf(armor_name, "piece plate helm");
          found_slot = TRUE;
          break;
        case 14:
          vnum = CHAIN_HELM;
          material = MATERIAL_BRONZE;
          sprintf(desc, "%sa", desc);
          sprintf(armor_name, "coif");
          found_slot = TRUE;
          break;
        case 15:
          vnum = STUD_LEATHER_HELM;
          material = MATERIAL_LEATHER;
          sprintf(desc, "%sa", desc);
          sprintf(armor_name, "studded helm");
          found_slot = TRUE;
          break;
        case 16:
          vnum = LEATHER_HELM;
          material = MATERIAL_LEATHER;
          sprintf(desc, "%sa", desc);
          sprintf(armor_name, "helm");
          found_slot = TRUE;
          break;
        case 17:
          vnum = PADDED_HELM;
          material = MATERIAL_COTTON;
          sprintf(desc, "%sa", desc);
          sprintf(armor_name, "padded armor helm");
          found_slot = TRUE;
          break;
        case 18:
          vnum = CLOTH_HELM;
          material = MATERIAL_COTTON;
          sprintf(desc, "%sa", desc);
          sprintf(armor_name, "cloth hood");
          found_slot = TRUE;
          break;
        default:
          found_slot = FALSE;
          break;
      }
      break;
    case WEAR_SHIELD:
      switch (rand_number(35, 37)) {
          /* shields */
        case 35:
          vnum = SHIELD_MEDIUM;
          material = MATERIAL_WOOD;
          sprintf(desc, "%sa", desc);
          sprintf(armor_name, "medium shield");
          found_slot = TRUE;
          break;
        case 36:
          vnum = SHIELD_LARGE;
          material = MATERIAL_WOOD;
          sprintf(desc, "%sa", desc);
          sprintf(armor_name, "large shield");
          found_slot = TRUE;
          break;
        case 37:
          vnum = SHIELD_TOWER;
          material = MATERIAL_WOOD;
          sprintf(desc, "%sa", desc);
          sprintf(armor_name, "tower shield");
          found_slot = TRUE;
          break;
        default:
          found_slot = FALSE;
          break;

      }
      break;
    default:
      found_slot = FALSE;
      break;
  }

  /* find a random piece of armor
   * assign base material
   * and last but not least, give appropriate start of description
   *  */
  if (!found_slot) {
    switch (dice(1, NUM_ARMOR_MOLDS)) {
        /* body pieces */
      case 1:
        vnum = PLATE_BODY;
        material = MATERIAL_BRONZE;
        sprintf(desc, "%sa suit of", desc);
        sprintf(armor_name, "plate mail armor");
        break;
      case 2:
        vnum = HALFPLATE_BODY;
        material = MATERIAL_BRONZE;
        sprintf(desc, "%sa suit of", desc);
        sprintf(armor_name, "half plate armor");
        break;
      case 3:
        vnum = SPLINT_BODY;
        material = MATERIAL_BRONZE;
        sprintf(desc, "%sa suit of", desc);
        sprintf(armor_name, "splint mail armor");
        break;
      case 4:
        vnum = BREASTPLATE_BODY;
        material = MATERIAL_BRONZE;
        sprintf(desc, "%sa suit of", desc);
        sprintf(armor_name, "breastplate armor");
        break;
      case 5:
        vnum = CHAIN_BODY;
        material = MATERIAL_BRONZE;
        sprintf(desc, "%sa suit of", desc);
        sprintf(armor_name, "chain mail armor");
        break;
      case 6:
        vnum = STUD_LEATHER_BODY;
        material = MATERIAL_LEATHER;
        sprintf(desc, "%sa suit of", desc);
        sprintf(armor_name, "studded armor");
        break;
      case 7:
        vnum = LEATHER_BODY;
        material = MATERIAL_LEATHER;
        sprintf(desc, "%sa suit of", desc);
        sprintf(armor_name, "armor");
        break;
      case 8:
        vnum = PADDED_BODY;
        material = MATERIAL_COTTON;
        sprintf(desc, "%sa suit of", desc);
        sprintf(armor_name, "padded armor");
        break;
      case 9:
        vnum = CLOTH_BODY;
        material = MATERIAL_COTTON;
        sprintf(desc, "%ssome", desc);
        sprintf(armor_name, "robes");
        break;

        /* head pieces */
      case 10:
        vnum = PLATE_HELM;
        material = MATERIAL_BRONZE;
        sprintf(desc, "%sa", desc);
        sprintf(armor_name, "plate mail helm");
        break;
      case 11:
        vnum = HALFPLATE_HELM;
        material = MATERIAL_BRONZE;
        sprintf(desc, "%sa", desc);
        sprintf(armor_name, "half plate helm");
        break;
      case 12:
        vnum = SPLINT_HELM;
        material = MATERIAL_BRONZE;
        sprintf(desc, "%sa", desc);
        sprintf(armor_name, "splint mail helm");
        break;
      case 13:
        vnum = PIECEPLATE_HELM;
        material = MATERIAL_BRONZE;
        sprintf(desc, "%sa", desc);
        sprintf(armor_name, "piece plate helm");
        break;
      case 14:
        vnum = CHAIN_HELM;
        material = MATERIAL_BRONZE;
        sprintf(desc, "%sa", desc);
        sprintf(armor_name, "coif");
        break;
      case 15:
        vnum = STUD_LEATHER_HELM;
        material = MATERIAL_LEATHER;
        sprintf(desc, "%sa", desc);
        sprintf(armor_name, "studded helm");
        break;
      case 16:
        vnum = LEATHER_HELM;
        material = MATERIAL_LEATHER;
        sprintf(desc, "%sa", desc);
        sprintf(armor_name, "helm");
        break;
      case 17:
        vnum = PADDED_HELM;
        material = MATERIAL_COTTON;
        sprintf(desc, "%sa", desc);
        sprintf(armor_name, "padded armor helm");
        break;
      case 18:
        vnum = CLOTH_HELM;
        material = MATERIAL_COTTON;
        sprintf(desc, "%sa", desc);
        sprintf(armor_name, "cloth hood");
        break;

        /* arm pieces */
      case 19:
        vnum = PLATE_ARMS;
        material = MATERIAL_BRONZE;
        sprintf(desc, "%sa set of", desc);
        sprintf(armor_name, "plate mail vambraces");
        break;
      case 20:
        vnum = HALFPLATE_ARMS;
        material = MATERIAL_BRONZE;
        sprintf(desc, "%sa set of", desc);
        sprintf(armor_name, "half plate vambraces");
        break;
      case 21:
        vnum = SPLINT_ARMS;
        material = MATERIAL_BRONZE;
        sprintf(desc, "%sa set of", desc);
        sprintf(armor_name, "splint mail vambraces");
        break;
      case 22:
        vnum = CHAIN_ARMS;
        material = MATERIAL_BRONZE;
        sprintf(desc, "%sa set of", desc);
        sprintf(armor_name, "chain mail sleeves");
        break;
      case 23:
        vnum = STUD_LEATHER_ARMS;
        material = MATERIAL_LEATHER;
        sprintf(desc, "%sa set of", desc);
        sprintf(armor_name, "studded sleeves");
        break;
      case 24:
        vnum = LEATHER_ARMS;
        material = MATERIAL_LEATHER;
        sprintf(desc, "%sa set of", desc);
        sprintf(armor_name, "sleeves");
        break;
      case 25:
        vnum = PADDED_ARMS;
        material = MATERIAL_COTTON;
        sprintf(desc, "%sa set of", desc);
        sprintf(armor_name, "padded armor sleeves");
        break;
      case 26:
        vnum = CLOTH_ARMS;
        material = MATERIAL_COTTON;
        sprintf(desc, "%sa set of", desc);
        sprintf(armor_name, "cloth sleeves");
        break;

        /* leg pieces */
      case 27:
        vnum = PLATE_LEGS;
        material = MATERIAL_BRONZE;
        sprintf(desc, "%sa set of", desc);
        sprintf(armor_name, "plate mail greaves");
        break;
      case 28:
        vnum = HALFPLATE_LEGS;
        material = MATERIAL_BRONZE;
        sprintf(desc, "%sa set of", desc);
        sprintf(armor_name, "half plate greaves");
        break;
      case 29:
        vnum = SPLINT_LEGS;
        material = MATERIAL_BRONZE;
        sprintf(desc, "%sa set of", desc);
        sprintf(armor_name, "splint mail greaves");
        break;
      case 30:
        vnum = CHAIN_LEGS;
        material = MATERIAL_BRONZE;
        sprintf(desc, "%sa set of", desc);
        sprintf(armor_name, "chain mail leggings");
        break;
      case 31:
        vnum = STUD_LEATHER_LEGS;
        material = MATERIAL_LEATHER;
        sprintf(desc, "%sa set of", desc);
        sprintf(armor_name, "studded leggings");
        break;
      case 32:
        vnum = LEATHER_LEGS;
        material = MATERIAL_LEATHER;
        sprintf(desc, "%sa set of", desc);
        sprintf(armor_name, "cuisses");
        break;
      case 33:
        vnum = PADDED_LEGS;
        material = MATERIAL_COTTON;
        sprintf(desc, "%sa set of", desc);
        sprintf(armor_name, "padded armor leggings");
        break;
      case 34:
        vnum = CLOTH_LEGS;
        material = MATERIAL_COTTON;
        sprintf(desc, "%sa set of", desc);
        sprintf(armor_name, "cloth pants");
        break;

        /* shields */
      case 35:
        vnum = SHIELD_MEDIUM;
        material = MATERIAL_WOOD;
        sprintf(desc, "%sa", desc);
        sprintf(armor_name, "medium shield");
        break;
      case 36:
        vnum = SHIELD_LARGE;
        material = MATERIAL_WOOD;
        sprintf(desc, "%sa", desc);
        sprintf(armor_name, "large shield");
        break;
      case 37:
        vnum = SHIELD_TOWER;
        material = MATERIAL_WOOD;
        sprintf(desc, "%sa", desc);
        sprintf(armor_name, "tower shield");
        break;
    }
  }

  /* we already determined 'base' material, now
   determine whether an upgrade was achieved by item-grade */
  roll = dice(1, 100);
  switch (material) {
    case MATERIAL_BRONZE:
      switch (grade) {
        case GRADE_MUNDANE:
          if (roll <= 75)
            material = MATERIAL_BRONZE;
          else
            material = MATERIAL_IRON;
          break;
        case GRADE_MINOR:
          if (roll <= 75)
            material = MATERIAL_IRON;
          else
            material = MATERIAL_STEEL;
          break;
        case GRADE_MEDIUM:
          if (roll <= 50)
            material = MATERIAL_IRON;
          else if (roll <= 80)
            material = MATERIAL_STEEL;
          else if (roll <= 95)
            material = MATERIAL_COLD_IRON;
          else
            material = MATERIAL_ALCHEMAL_SILVER;
          break;
        default: // major grade
          if (roll <= 50)
            material = MATERIAL_COLD_IRON;
          else if (roll <= 80)
            material = MATERIAL_ALCHEMAL_SILVER;
          else if (roll <= 95)
            material = MATERIAL_MITHRIL;
          else
            material = MATERIAL_ADAMANTINE;
          break;
      }
      break;
    case MATERIAL_LEATHER:
      switch (grade) {
        case GRADE_MUNDANE:
          material = MATERIAL_LEATHER;
          break;
        case GRADE_MINOR:
          material = MATERIAL_LEATHER;
          break;
        case GRADE_MEDIUM:
          material = MATERIAL_LEATHER;
          break;
        default: // major grade
          if (roll <= 90)
            material = MATERIAL_LEATHER;
          else
            material = MATERIAL_DRAGONHIDE;
          break;
      }
      break;
    case MATERIAL_COTTON:
      switch (grade) {
        case GRADE_MUNDANE:
          if (roll <= 75)
            material = MATERIAL_HEMP;
          else
            material = MATERIAL_COTTON;
          break;
        case GRADE_MINOR:
          if (roll <= 50)
            material = MATERIAL_HEMP;
          else if (roll <= 80)
            material = MATERIAL_COTTON;
          else
            material = MATERIAL_WOOL;
          break;
        case GRADE_MEDIUM:
          if (roll <= 50)
            material = MATERIAL_COTTON;
          else if (roll <= 80)
            material = MATERIAL_WOOL;
          else if (roll <= 95)
            material = MATERIAL_VELVET;
          else
            material = MATERIAL_SATIN;
          break;
        default: // major grade
          if (roll <= 50)
            material = MATERIAL_WOOL;
          else if (roll <= 80)
            material = MATERIAL_VELVET;
          else if (roll <= 95)
            material = MATERIAL_SATIN;
          else
            material = MATERIAL_SILK;
          break;
      }
      break;
    case MATERIAL_WOOD:
      switch (grade) {
        case GRADE_MUNDANE:
        case GRADE_MINOR:
        case GRADE_MEDIUM:
          material = MATERIAL_WOOD;
          break;
        default: // major grade
          if (roll <= 80)
            material = MATERIAL_WOOD;
          else
            material = MATERIAL_DARKWOOD;
          break;
      }
      break;
  }

  /* determine level */
  switch (grade) {
    case GRADE_MUNDANE:
      level = rand_number(1, 8);
      break;
    case GRADE_MINOR:
      level = rand_number(9, 16);
      break;
    case GRADE_MEDIUM:
      level = rand_number(17, 24);
      break;
    default: // major grade
      level = rand_number(25, 30);
      break;
  }

  /* ok load object, set material */
  if ((obj = read_object(vnum, VIRTUAL)) == NULL) {
    log("SYSERR: award_magic_armor created NULL object");
    return;
  }
  GET_OBJ_MATERIAL(obj) = material;

  /* BEGIN DESCRIPTION SECTION */

  /* first assign two random colors for usage */
  color1 = rand_number(0, NUM_A_COLORS);
  color2 = rand_number(0, NUM_A_COLORS);
  /* make sure they are not the same colors */
  while (color2 == color1)
    color2 = rand_number(0, NUM_A_COLORS);
  crest_num = rand_number(0, NUM_A_ARMOR_CRESTS);

  /* start with keyword string */
  sprintf(keywords, "%s %s", keywords, armor_name);
  sprintf(keywords, "%s %s", keywords, material_name[material]);

  roll = dice(1, 3);
  if (roll == 3) { // armor spec adjective in desc?
    sprintf(desc, "%s %s", desc,
            armor_special_descs[rand_number(0, NUM_A_ARMOR_SPECIAL_DESCS)]);
    sprintf(keywords, "%s %s", keywords,
            armor_special_descs[rand_number(0, NUM_A_ARMOR_SPECIAL_DESCS)]);
  }

  roll = dice(1, 5);
  if (roll >= 4) { // color describe #1?
    sprintf(desc, "%s %s", desc, colors[color1]);
    sprintf(keywords, "%s %s", keywords, colors[color1]);
  } else if (roll == 3) { // two colors
    sprintf(desc, "%s %s and %s", desc, colors[color1], colors[color2]);
    sprintf(keywords, "%s %s and %s", keywords, colors[color1], colors[color2]);
  }

  // Insert the material type, then armor type
  sprintf(desc, "%s %s", desc, material_name[material]);
  sprintf(desc, "%s %s", desc, armor_name);

  roll = dice(1, 8);
  if (roll >= 7) { // crest?
    sprintf(desc, "%s with %s %s crest", desc,
            AN(armor_crests[crest_num]),
            armor_crests[crest_num]);
    sprintf(keywords, "%s with %s %s crest", keywords,
            AN(armor_crests[crest_num]),
            armor_crests[crest_num]);
  } else if (roll >= 5) { // or symbol?
    sprintf(desc, "%s covered in symbols of %s %s", desc,
            AN(armor_crests[crest_num]),
            armor_crests[crest_num]);
    sprintf(keywords, "%s covered in symbols of %s %s", keywords,
            AN(armor_crests[crest_num]),
            armor_crests[crest_num]);
  }

  // keywords
  obj->name = strdup(keywords);
  // Set descriptions
  obj->short_description = strdup(desc);
  desc[0] = toupper(desc[0]);
  sprintf(desc, "%s is lying here.", desc);
  obj->description = strdup(desc);

  /* END DESCRIPTION SECTION */

  /* BONUS SECTION */
  cp_modify_object_applies(ch, obj, rare_grade, level, CP_TYPE_ARMOR);
  /* END BONUS SECTION */
}

/* give away random magic weapon, method:
 * 1)  determine weapon
 * 2)  determine material
 * 3)  assign description
 * 4)  determine modifier (if applicable)
 * 5)  determine amount (if applicable)
 */
#define SHORT_STRING 80
void award_magic_weapon(struct char_data *ch, int grade, int moblevel) {
  struct obj_data *obj = NULL;
  int vnum = -1, material = MATERIAL_BRONZE, roll = 0, size = SIZE_MEDIUM;
  int rare_grade = 0, color1 = 0, color2 = 0, level = 0, roll2 = 0, roll3 = 0;
  char desc[MEDIUM_STRING] = {'\0'}, weapon_name[SHORT_STRING] = {'\0'};
  char hilt_color[SHORT_STRING] = {'\0'}, head_color[SHORT_STRING] = {'\0'};
  char special[SHORT_STRING] = {'\0'};
  char buf[MAX_STRING_LENGTH] = {'\0'};

  /* determine if rare or not */
  roll = dice(1, 100);
  if (roll == 1) {
    rare_grade = 3;
    sprintf(desc, "\tM[Mythical] \tn");
  } else if (roll <= 6) {
    rare_grade = 2;
    sprintf(desc, "\tY[Legendary] \tn");
  } else if (roll <= 16) {
    rare_grade = 1;
    sprintf(desc, "\tG[Rare] \tn");
  }

  /* find a random weapon
   * assign base material
   * assign size
   * and last but not least, give appropriate start of description
   *  */
  switch (dice(1, NUM_WEAPON_MOLDS)) {
      /* simple */
      /* light */
    case 1:
      vnum = DAGGER;
      material = MATERIAL_BRONZE;
      sprintf(weapon_name, "dagger");
      size = SIZE_SMALL;
      break;
    case 2:
      vnum = MACE;
      material = MATERIAL_BRONZE;
      sprintf(weapon_name, "mace");
      size = SIZE_SMALL;
      break;
    case 3:
      vnum = SICKLE;
      material = MATERIAL_BRONZE;
      sprintf(weapon_name, "sickle");
      size = SIZE_SMALL;
      break;
      /* one handed */
    case 4:
      vnum = CLUB;
      material = MATERIAL_WOOD;
      sprintf(weapon_name, "club");
      size = SIZE_MEDIUM;
      break;
    case 5:
      vnum = MORNINGSTAR;
      material = MATERIAL_BRONZE;
      sprintf(weapon_name, "morning star");
      size = SIZE_MEDIUM;
      break;
      /* two handed */
    case 6:
      vnum = SPEAR;
      material = MATERIAL_WOOD;
      sprintf(weapon_name, "spear");
      size = SIZE_LARGE;
      break;
    case 7:
      vnum = QUARTERSTAFF;
      material = MATERIAL_WOOD;
      sprintf(weapon_name, "quarterstaff");
      size = SIZE_LARGE;
      break;
      /* martial */
      /* light */
    case 8:
      vnum = HANDAXE;
      material = MATERIAL_BRONZE;
      sprintf(weapon_name, "hand axe");
      size = SIZE_SMALL;
      break;
    case 9:
      vnum = KUKRI;
      material = MATERIAL_BRONZE;
      sprintf(weapon_name, "kukri");
      size = SIZE_SMALL;
      break;
    case 10:
      vnum = SHORTSWORD;
      material = MATERIAL_BRONZE;
      sprintf(weapon_name, "short sword");
      size = SIZE_SMALL;
      break;
      /* one handed */
    case 11:
      vnum = BATTLEAXE;
      material = MATERIAL_BRONZE;
      sprintf(weapon_name, "battle axe");
      size = SIZE_MEDIUM;
      break;
    case 12:
      vnum = FLAIL;
      material = MATERIAL_BRONZE;
      sprintf(weapon_name, "flail");
      size = SIZE_MEDIUM;
      break;
    case 13:
      vnum = LONGSWORD;
      material = MATERIAL_BRONZE;
      sprintf(weapon_name, "long sword");
      size = SIZE_MEDIUM;
      break;
    case 14:
      vnum = RAPIER;
      material = MATERIAL_BRONZE;
      sprintf(weapon_name, "rapier");
      size = SIZE_MEDIUM;
      break;
    case 15:
      vnum = SCIMITAR;
      material = MATERIAL_BRONZE;
      sprintf(weapon_name, "scimitar");
      size = SIZE_MEDIUM;
      break;
    case 16:
      vnum = TRIDENT;
      material = MATERIAL_WOOD;
      sprintf(weapon_name, "trident");
      size = SIZE_MEDIUM;
      break;
    case 17:
      vnum = WARHAMMER;
      material = MATERIAL_BRONZE;
      sprintf(weapon_name, "war hammer");
      size = SIZE_MEDIUM;
      break;
      /* two handed */
    case 18:
      vnum = FALCHION;
      material = MATERIAL_BRONZE;
      sprintf(weapon_name, "falchion");
      size = SIZE_LARGE;
      break;
    case 19:
      vnum = GLAIVE;
      material = MATERIAL_BRONZE;
      sprintf(weapon_name, "glaive");
      size = SIZE_LARGE;
      break;
    case 20:
      vnum = GREATAXE;
      material = MATERIAL_BRONZE;
      sprintf(weapon_name, "great axe");
      size = SIZE_LARGE;
      break;
    case 21:
      vnum = GREATCLUB;
      material = MATERIAL_WOOD;
      sprintf(weapon_name, "great club");
      size = SIZE_LARGE;
      break;
    case 22:
      vnum = GREATSWORD;
      material = MATERIAL_BRONZE;
      sprintf(weapon_name, "great sword");
      size = SIZE_LARGE;
      break;
    case 23:
      vnum = HALBERD;
      material = MATERIAL_BRONZE;
      sprintf(weapon_name, "halberd");
      size = SIZE_LARGE;
      break;
    case 24:
      vnum = LANCE;
      material = MATERIAL_WOOD;
      sprintf(weapon_name, "lance");
      size = SIZE_LARGE;
      break;
    case 25:
      vnum = SCYTHE;
      material = MATERIAL_BRONZE;
      sprintf(weapon_name, "scythe");
      size = SIZE_LARGE;
      break;
      /* exotic */
      /* light */
    case 26:
      vnum = KAMA;
      material = MATERIAL_BRONZE;
      sprintf(weapon_name, "kama");
      size = SIZE_SMALL;
      break;
      /* one handed */
    case 27:
      vnum = BASTARDSWORD;
      material = MATERIAL_BRONZE;
      sprintf(weapon_name, "bastard sword");
      size = SIZE_MEDIUM;
      break;
    case 28:
      vnum = DWARVENWARAXE;
      material = MATERIAL_BRONZE;
      sprintf(weapon_name, "dwarven war axe");
      size = SIZE_MEDIUM;
      break;
      /* two handed */
    case 29:
      vnum = DIREFLAIL;
      material = MATERIAL_BRONZE;
      sprintf(weapon_name, "dire flail");
      size = SIZE_LARGE;
      break;
  }

  /* we already determined 'base' material, now
   determine whether an upgrade was achieved by item-grade */
  roll = dice(1, 100);
  switch (material) {
    case MATERIAL_BRONZE:
      switch (grade) {
        case GRADE_MUNDANE:
          if (roll <= 75)
            material = MATERIAL_BRONZE;
          else
            material = MATERIAL_IRON;
          break;
        case GRADE_MINOR:
          if (roll <= 75)
            material = MATERIAL_IRON;
          else
            material = MATERIAL_STEEL;
          break;
        case GRADE_MEDIUM:
          if (roll <= 50)
            material = MATERIAL_IRON;
          else if (roll <= 80)
            material = MATERIAL_STEEL;
          else if (roll <= 95)
            material = MATERIAL_COLD_IRON;
          else
            material = MATERIAL_ALCHEMAL_SILVER;
          break;
        default: // major grade
          if (roll <= 50)
            material = MATERIAL_COLD_IRON;
          else if (roll <= 80)
            material = MATERIAL_ALCHEMAL_SILVER;
          else if (roll <= 95)
            material = MATERIAL_MITHRIL;
          else
            material = MATERIAL_ADAMANTINE;
          break;
      }
      break;
    case MATERIAL_WOOD:
      switch (grade) {
        case GRADE_MUNDANE:
        case GRADE_MINOR:
        case GRADE_MEDIUM:
          material = MATERIAL_WOOD;
          break;
        default: // major grade
          if (roll <= 80)
            material = MATERIAL_WOOD;
          else
            material = MATERIAL_DARKWOOD;
          break;
      }
      break;
  }

  /* determine level */
  switch (grade) {
    case GRADE_MUNDANE:
      level = rand_number(1, 8);
      break;
    case GRADE_MINOR:
      level = rand_number(9, 16);
      break;
    case GRADE_MEDIUM:
      level = rand_number(17, 24);
      break;
    default: // major grade
      level = rand_number(25, 30);
      break;
  }

  /* ok load object, set material */
  if ((obj = read_object(vnum, VIRTUAL)) == NULL) {
    log("SYSERR: award_magic_armor created NULL object");
    return;
  }
  GET_OBJ_MATERIAL(obj) = material;

  // pick a pair of random colors for usage
  /* first assign two random colors for usage */
  color1 = rand_number(0, NUM_A_COLORS);
  color2 = rand_number(0, NUM_A_COLORS);
  /* make sure they are not the same colors */
  while (color2 == color1)
    color2 = rand_number(0, NUM_A_COLORS);

  sprintf(head_color, "%s", colors[color1]);
  sprintf(hilt_color, "%s", colors[color2]);
  if (IS_BLADE(obj))
    sprintf(special, "%s%s", desc, blade_descs[rand_number(0, NUM_A_BLADE_DESCS)]);
  else if (IS_PIERCE(obj))
    sprintf(special, "%s%s", desc, piercing_descs[rand_number(0, NUM_A_PIERCING_DESCS)]);
  else //blunt
    sprintf(special, "%s%s", desc, blunt_descs[rand_number(0, NUM_A_BLUNT_DESCS)]);

  roll = dice(1, 100);
  roll2 = rand_number(0, NUM_A_HEAD_TYPES);
  roll3 = rand_number(0, NUM_A_HANDLE_TYPES);

  // special, head color, hilt color
  if (roll >= 91) {
    sprintf(buf, "%s %s-%s %s %s %s %s", weapon_name,
            head_color, head_types[roll2],
            material_name[GET_OBJ_MATERIAL(obj)], special,
            hilt_color,
            handle_types[roll3]);
    obj->name = strdup(buf);
    sprintf(buf, "%s %s, %s-%s %s %s with %s %s %s", a_or_an(special), special,
            head_color, head_types[roll2],
            material_name[GET_OBJ_MATERIAL(obj)], weapon_name,
            a_or_an(hilt_color), hilt_color,
            handle_types[roll3]);
    obj->short_description = strdup(buf);
    sprintf(buf, "%s %s, %s-%s %s %s with %s %s %s lies here.", a_or_an(special),
            special, head_color, head_types[roll2],
            material_name[GET_OBJ_MATERIAL(obj)], weapon_name,
            a_or_an(hilt_color), hilt_color,
            handle_types[roll3]);
    *buf = UPPER(*buf);
    obj->description = strdup(buf);

    // special, head color
  } else if (roll >= 81) {
    sprintf(buf, "%s %s-%s %s %s", weapon_name,
            head_color, head_types[roll2],
            material_name[GET_OBJ_MATERIAL(obj)], special);
    obj->name = strdup(buf);
    sprintf(buf, "%s %s, %s-%s %s %s", a_or_an(special), special,
            head_color, head_types[roll2],
            material_name[GET_OBJ_MATERIAL(obj)], weapon_name);
    obj->short_description = strdup(buf);
    sprintf(buf, "%s %s, %s-%s %s %s lies here.", a_or_an(special),
            special, head_color, head_types[roll2],
            material_name[GET_OBJ_MATERIAL(obj)], weapon_name);
    *buf = UPPER(*buf);
    obj->description = strdup(buf);

    // special, hilt color
  } else if (roll >= 71) {
    sprintf(buf, "%s %s %s %s %s", weapon_name,
            material_name[GET_OBJ_MATERIAL(obj)], special, hilt_color,
            handle_types[roll3]);
    obj->name = strdup(buf);
    sprintf(buf, "%s %s %s %s with %s %s %s", a_or_an(special), special,
            material_name[GET_OBJ_MATERIAL(obj)], weapon_name,
            a_or_an(hilt_color), hilt_color,
            handle_types[roll3]);
    obj->short_description = strdup(buf);
    sprintf(buf, "%s %s %s %s with %s %s %s lies here.", a_or_an(special),
            special, material_name[GET_OBJ_MATERIAL(obj)], weapon_name,
            a_or_an(hilt_color), hilt_color,
            handle_types[roll3]);
    *buf = UPPER(*buf);
    obj->description = strdup(buf);

    // head color, hilt color
  } else if (roll >= 41) {
    sprintf(buf, "%s %s-%s %s %s %s",
            weapon_name, head_color, head_types[roll2],
            material_name[GET_OBJ_MATERIAL(obj)],
            hilt_color,
            handle_types[roll3]);
    obj->name = strdup(buf);
    sprintf(buf, "%s %s-%s %s %s with %s %s %s", a_or_an(head_color),
            head_color, head_types[roll2],
            material_name[GET_OBJ_MATERIAL(obj)], weapon_name,
            a_or_an(hilt_color), hilt_color,
            handle_types[roll3]);
    obj->short_description = strdup(buf);
    sprintf(buf, "%s %s-%s %s %s with %s %s %s lies here.", a_or_an(head_color),
            head_color, head_types[roll2],
            material_name[GET_OBJ_MATERIAL(obj)], weapon_name,
            a_or_an(hilt_color), hilt_color,
            handle_types[roll3]);
    *buf = UPPER(*buf);
    obj->description = strdup(buf);

    // head color
  } else if (roll >= 31) {
    sprintf(buf, "%s %s-%s %s", weapon_name,
            head_color, head_types[roll2],
            material_name[GET_OBJ_MATERIAL(obj)]);
    obj->name = strdup(buf);
    sprintf(buf, "%s %s-%s %s %s", a_or_an(head_color),
            head_color, head_types[roll2],
            material_name[GET_OBJ_MATERIAL(obj)], weapon_name);
    obj->short_description = strdup(buf);
    sprintf(buf, "%s %s-%s %s %s lies here.", a_or_an(head_color),
            head_color, head_types[roll2],
            material_name[GET_OBJ_MATERIAL(obj)], weapon_name);
    *buf = UPPER(*buf);
    obj->description = strdup(buf);

    // hilt color
  } else if (roll >= 21) {
    sprintf(buf, "%s %s %s %s",
            weapon_name, material_name[GET_OBJ_MATERIAL(obj)],
            hilt_color,
            handle_types[roll3]);
    obj->name = strdup(buf);
    sprintf(buf, "%s %s %s with %s %s %s", a_or_an((char *) material_name[GET_OBJ_MATERIAL(obj)]),
            material_name[GET_OBJ_MATERIAL(obj)], weapon_name,
            a_or_an(hilt_color), hilt_color,
            handle_types[roll3]);
    obj->short_description = strdup(buf);
    sprintf(buf, "%s %s %s with %s %s %s lies here.",
            a_or_an((char *) material_name[GET_OBJ_MATERIAL(obj)]),
            material_name[GET_OBJ_MATERIAL(obj)], weapon_name,
            a_or_an(hilt_color), hilt_color,
            handle_types[roll3]);
    *buf = UPPER(*buf);
    obj->description = strdup(buf);

    // special
  } else if (roll >= 11) {
    sprintf(buf, "%s %s %s", weapon_name,
            material_name[GET_OBJ_MATERIAL(obj)], special);
    obj->name = strdup(buf);
    sprintf(buf, "%s %s %s %s", a_or_an(special), special,
            material_name[GET_OBJ_MATERIAL(obj)], weapon_name);
    obj->short_description = strdup(buf);
    sprintf(buf, "%s %s %s %s lies here.", a_or_an(special), special,
            material_name[GET_OBJ_MATERIAL(obj)], weapon_name);
    *buf = UPPER(*buf);
    obj->description = strdup(buf);

    // none
  } else {
    sprintf(buf, "%s %s",
            weapon_name, material_name[GET_OBJ_MATERIAL(obj)]);
    obj->name = strdup(buf);
    sprintf(buf, "%s %s %s", a_or_an((char *) material_name[GET_OBJ_MATERIAL(obj)]),
            material_name[GET_OBJ_MATERIAL(obj)], weapon_name);
    obj->short_description = strdup(buf);
    sprintf(buf, "%s %s %s lies here.",
            a_or_an((char *) material_name[GET_OBJ_MATERIAL(obj)]),
            material_name[GET_OBJ_MATERIAL(obj)], weapon_name);
    *buf = UPPER(*buf);
    obj->description = strdup(buf);
  }

  /* object is fully described
   base object is taken care of including material, now set random stats, etc */
  cp_modify_object_applies(ch, obj, rare_grade, level, CP_TYPE_WEAPON);
}
#undef SHORT_STRING

/* give away random misc item
 * includes:  neck, about, waist, wrist, hands, rings, feet
 * method:
 * 1)  determine item
 * 2)  determine material
 * 3)  assign description
 * 4)  determine modifier (if applicable)
 * 5)  determine amount (if applicable)
 */
#define SHORT_STRING    80
void award_misc_magic_item(struct char_data *ch, int grade, int moblevel) {
  struct obj_data *obj = NULL;
  int vnum = -1, material = MATERIAL_BRONZE, roll = 0;
  int rare_grade = 0, level = 0;
  char desc[MEDIUM_STRING] = {'\0'}, armor_name[MEDIUM_STRING] = {'\0'};
  char keywords[MEDIUM_STRING] = {'\0'};
  char desc2[SHORT_STRING] = {'\0'}, desc3[SHORT_STRING] = {'\0'};

  /* determine if rare or not */
  roll = dice(1, 100);
  if (roll == 1) {
    rare_grade = 3;
    sprintf(desc, "\tM[Mythical] \tn");
  } else if (roll <= 6) {
    rare_grade = 2;
    sprintf(desc, "\tY[Legendary] \tn");
  } else if (roll <= 16) {
    rare_grade = 1;
    sprintf(desc, "\tG[Rare] \tn");
  }

  /* find a random piece of armor
   * assign base material
   * and last but not least, give appropriate start of description
   *  */
  switch (dice(1, NUM_MISC_MOLDS)) {
    case 1:
      vnum = RING_MOLD;
      material = MATERIAL_COPPER;
      sprintf(armor_name, ring_descs[rand_number(0, NUM_A_RING_DESCS)]);
      sprintf(desc2, gemstones[rand_number(0, NUM_A_GEMSTONES)]);
      break;
    case 2:
      vnum = NECKLACE_MOLD;
      material = MATERIAL_COPPER;
      sprintf(armor_name, neck_descs[rand_number(0, NUM_A_NECK_DESCS)]);
      sprintf(desc2, gemstones[rand_number(0, NUM_A_GEMSTONES)]);
      break;
    case 3:
      vnum = BOOTS_MOLD;
      material = MATERIAL_LEATHER;
      sprintf(armor_name, boot_descs[rand_number(0, NUM_A_BOOT_DESCS)]);
      sprintf(desc2, armor_special_descs[rand_number(0, NUM_A_ARMOR_SPECIAL_DESCS)]);
      sprintf(desc3, colors[rand_number(0, NUM_A_COLORS)]);
      break;
    case 4:
      vnum = GLOVES_MOLD;
      material = MATERIAL_LEATHER;
      sprintf(armor_name, hands_descs[rand_number(0, NUM_A_HAND_DESCS)]);
      sprintf(desc2, armor_special_descs[rand_number(0, NUM_A_ARMOR_SPECIAL_DESCS)]);
      sprintf(desc3, colors[rand_number(0, NUM_A_COLORS)]);
      break;
    case 5:
      vnum = CLOAK_MOLD;
      material = MATERIAL_COTTON;
      sprintf(armor_name, cloak_descs[rand_number(0, NUM_A_CLOAK_DESCS)]);
      sprintf(desc2, armor_crests[rand_number(0, NUM_A_ARMOR_CRESTS)]);
      sprintf(desc3, colors[rand_number(0, NUM_A_COLORS)]);
      break;
    case 6:
      vnum = BELT_MOLD;
      material = MATERIAL_LEATHER;
      sprintf(armor_name, waist_descs[rand_number(0, NUM_A_WAIST_DESCS)]);
      sprintf(desc2, armor_special_descs[rand_number(0, NUM_A_ARMOR_SPECIAL_DESCS)]);
      sprintf(desc3, colors[rand_number(0, NUM_A_COLORS)]);
      break;
    case 7:
      vnum = WRIST_MOLD;
      material = MATERIAL_COPPER;
      sprintf(armor_name, wrist_descs[rand_number(0, NUM_A_WRIST_DESCS)]);
      sprintf(desc2, gemstones[rand_number(0, NUM_A_GEMSTONES)]);
      break;
    case 8:
      vnum = HELD_MOLD;
      material = MATERIAL_ONYX;
      sprintf(armor_name, crystal_descs[rand_number(0, NUM_A_CRYSTAL_DESCS)]);
      sprintf(desc2, colors[rand_number(0, NUM_A_COLORS)]);
      break;
  }

  /* we already determined 'base' material, now
   determine whether an upgrade was achieved by item-grade */
  roll = dice(1, 100);
  switch (material) {
    case MATERIAL_COPPER:
      switch (grade) {
        case GRADE_MUNDANE:
          if (roll <= 75)
            material = MATERIAL_COPPER;
          else
            material = MATERIAL_BRASS;
          break;
        case GRADE_MINOR:
          if (roll <= 75)
            material = MATERIAL_BRASS;
          else
            material = MATERIAL_SILVER;
          break;
        case GRADE_MEDIUM:
          if (roll <= 50)
            material = MATERIAL_BRASS;
          else if (roll <= 80)
            material = MATERIAL_SILVER;
          else
            material = MATERIAL_GOLD;
          break;
        default: // major grade
          if (roll <= 50)
            material = MATERIAL_SILVER;
          else if (roll <= 80)
            material = MATERIAL_GOLD;
          else
            material = MATERIAL_PLATINUM;
          break;
      }
      break;
    case MATERIAL_LEATHER:
      switch (grade) {
        case GRADE_MUNDANE:
          material = MATERIAL_LEATHER;
          break;
        case GRADE_MINOR:
          material = MATERIAL_LEATHER;
          break;
        case GRADE_MEDIUM:
          material = MATERIAL_LEATHER;
          break;
        default: // major grade
          if (roll <= 90)
            material = MATERIAL_LEATHER;
          else
            material = MATERIAL_DRAGONHIDE;
          break;
      }
      break;
    case MATERIAL_COTTON:
      switch (grade) {
        case GRADE_MUNDANE:
          if (roll <= 75)
            material = MATERIAL_HEMP;
          else
            material = MATERIAL_COTTON;
          break;
        case GRADE_MINOR:
          if (roll <= 50)
            material = MATERIAL_HEMP;
          else if (roll <= 80)
            material = MATERIAL_COTTON;
          else
            material = MATERIAL_WOOL;
          break;
        case GRADE_MEDIUM:
          if (roll <= 50)
            material = MATERIAL_COTTON;
          else if (roll <= 80)
            material = MATERIAL_WOOL;
          else if (roll <= 95)
            material = MATERIAL_VELVET;
          else
            material = MATERIAL_SATIN;
          break;
        default: // major grade
          if (roll <= 50)
            material = MATERIAL_WOOL;
          else if (roll <= 80)
            material = MATERIAL_VELVET;
          else if (roll <= 95)
            material = MATERIAL_SATIN;
          else
            material = MATERIAL_SILK;
          break;
      }
      break;
      /* options:  crystal, obsidian, onyx, ivory, pewter*/
    case MATERIAL_ONYX:
      switch (dice(1, 5)) {
        case 1:
          material = MATERIAL_CRYSTAL;
          break;
        case 2:
          material = MATERIAL_OBSIDIAN;
          break;
        case 3:
          material = MATERIAL_IVORY;
          break;
        case 4:
          material = MATERIAL_PEWTER;
          break;
        default:
          material = MATERIAL_ONYX;
          break;
      }
      break;
  }

  /* determine level */
  switch (grade) {
    case GRADE_MUNDANE:
      level = rand_number(1, 8);
      break;
    case GRADE_MINOR:
      level = rand_number(9, 16);
      break;
    case GRADE_MEDIUM:
      level = rand_number(17, 24);
      break;
    default: // major grade
      level = rand_number(25, 30);
      break;
  }

  /* ok load object, set material */
  if ((obj = read_object(vnum, VIRTUAL)) == NULL) {
    log("SYSERR: award_magic_armor created NULL object");
    return;
  }
  GET_OBJ_MATERIAL(obj) = material;

  /* put together a descrip */
  switch (vnum) {
    case RING_MOLD:
    case NECKLACE_MOLD:
    case WRIST_MOLD:
      sprintf(keywords, "%s %s set with %s gemstone",
              armor_name, material_name[material], desc2);
      obj->name = strdup(keywords);
      sprintf(desc, "%s%s %s %s set with %s %s gemstone", desc,
              AN(material_name[material]), material_name[material],
              armor_name, AN(desc2), desc2);
      obj->short_description = strdup(desc);
      sprintf(desc, "%s %s %s set with %s %s gemstone lies here.",
              AN(material_name[material]), material_name[material],
              armor_name, AN(desc2), desc2);
      obj->description = strdup(CAP(desc));
      break;
    case BOOTS_MOLD:
    case GLOVES_MOLD:
      sprintf(keywords, "%s pair %s %s leather", armor_name, desc2, desc3);
      obj->name = strdup(keywords);
      sprintf(desc, "%sa pair of %s %s leather %s", desc, desc2, desc3,
              armor_name);
      obj->short_description = strdup(desc);
      sprintf(desc, "A pair of %s %s leather %s lie here.", desc2, desc3,
              armor_name);
      obj->description = strdup(desc);
      break;
    case CLOAK_MOLD:
      sprintf(keywords, "%s %s %s %s bearing crest", armor_name, desc2,
              material_name[material], desc3);
      obj->name = strdup(keywords);
      sprintf(desc, "%s%s %s %s %s bearing the crest of %s %s", desc, AN(desc3), desc3,
              material_name[material], armor_name, AN(desc2),
              desc2);
      obj->short_description = strdup(desc);
      sprintf(desc, "%s %s %s %s bearing the crest of %s %s is lying here.", AN(desc3), desc3,
              material_name[material], armor_name, AN(desc2),
              desc2);
      obj->description = strdup(CAP(desc));
      break;
    case BELT_MOLD:
      sprintf(keywords, "%s %s leather %s", armor_name, desc2, desc3);
      obj->name = strdup(keywords);
      sprintf(desc, "%s%s %s %s leather %s", desc, AN(desc2), desc2, desc3,
              armor_name);
      obj->short_description = strdup(desc);
      sprintf(desc, "%s %s %s leather %s lie here.", AN(desc2), desc2, desc3,
              armor_name);
      obj->description = strdup(desc);
      break;
    case HELD_MOLD:
      sprintf(keywords, "%s %s orb", armor_name, desc2);
      obj->name = strdup(keywords);
      sprintf(desc, "%sa %s %s orb", desc, desc2, armor_name);
      obj->short_description = strdup(desc);
      sprintf(desc, "A %s %s orb is lying here.", desc2, armor_name);
      obj->description = strdup(desc);
      break;
  }

  /* level, bonus and cost */
  cp_modify_object_applies(ch, obj, rare_grade, level, CP_TYPE_MISC);
}
#undef SHORT_STRING

/* Load treasure on a mob. -Ornir */
void load_treasure(char_data *mob) {
  int roll = dice(1, 100);
  int level = 0;
  int grade = GRADE_MUNDANE;

  if (!IS_NPC(mob))
    return;

  level = GET_LEVEL(mob);

  if (level >= 20) {
    grade = GRADE_MAJOR;
  } else if (level >= 16) {
    if (roll >= 61)
      grade = GRADE_MAJOR;
    else
      grade = GRADE_MEDIUM;
  } else if (level >= 12) {
    if (roll >= 81)
      grade = GRADE_MAJOR;
    else if (roll >= 11)
      grade = GRADE_MEDIUM;
    else
      grade = GRADE_MINOR;
  } else if (level >= 8) {
    if (roll >= 96)
      grade = GRADE_MAJOR;
    else if (roll >= 31)
      grade = GRADE_MEDIUM;
    else
      grade = GRADE_MINOR;
  } else if (level >= 4) {
    if (roll >= 76)
      grade = GRADE_MEDIUM;
    else if (roll >= 16)
      grade = GRADE_MINOR;
    else
      grade = GRADE_MUNDANE;
  } else {
    if (roll >= 96)
      grade = GRADE_MEDIUM;
    else if (roll >= 41)
      grade = GRADE_MINOR;
    else
      grade = GRADE_MUNDANE;
  }

  /* Give the mob one magic item. */
  award_magic_item(1, mob, level, grade);
}

/* staff tool to load random items */
ACMD(do_loadmagicspecific) {
  char arg1[MAX_STRING_LENGTH];
  char arg2[MAX_STRING_LENGTH];
  int type = 0;
  int grade = 0;

  two_arguments(argument, arg1, arg2);

  if (!*arg1) {
    send_to_char(ch, "Syntax: loadmagicspecific [mundane | minor | medium | major] "
            "[weapon | shield | body | legs | arms | head | misc]\r\n");
    return;
  }

  if (!*arg2) {
    send_to_char(ch, "The second argument must be either weapon/body/legs/arms/head/misc.\r\n");
    return;
  }

  if (is_abbrev(arg1, "mundane"))
    grade = GRADE_MUNDANE;
  else if (is_abbrev(arg1, "minor"))
    grade = GRADE_MINOR;
  else if (is_abbrev(arg1, "medium"))
    grade = GRADE_MEDIUM;
  else if (is_abbrev(arg1, "major"))
    grade = GRADE_MAJOR;
  else {
    send_to_char(ch, "Syntax: loadmagicspecific [mundane | minor | medium | major] "
            "[weapon | shield | body | legs | arms | head | misc]\r\n");
    return;
  }

  if (is_abbrev(arg2, "weapon"))
    type = 1;
  else if (is_abbrev(arg2, "body"))
    type = 2;
  else if (is_abbrev(arg2, "legs"))
    type = 3;
  else if (is_abbrev(arg2, "arms"))
    type = 4;
  else if (is_abbrev(arg2, "head"))
    type = 5;
  else if (is_abbrev(arg2, "misc"))
    type = 6;
  else if (is_abbrev(arg2, "shield"))
    type = 7;
  else {
    send_to_char(ch, "The second argument must be either weapon/shield/body/legs/arms/head/misc.\r\n");
    return;
  }

  switch (type) {
    case 1: /* weapon */
      award_magic_weapon(ch, grade, GET_LEVEL(ch));
      break;
    case 2: /* body */
      award_magic_armor(ch, grade, GET_LEVEL(ch), WEAR_BODY);
      break;
    case 3: /* legs */
      award_magic_armor(ch, grade, GET_LEVEL(ch), WEAR_LEGS);
      break;
    case 4: /* arms */
      award_magic_armor(ch, grade, GET_LEVEL(ch), WEAR_ARMS);
      break;
    case 5: /* head */
      award_magic_armor(ch, grade, GET_LEVEL(ch), WEAR_HEAD);
      break;
    case 6: /* misc */
      award_misc_magic_item(ch, grade, GET_LEVEL(ch));
      break;
    case 7: /* shield */
      award_magic_armor(ch, grade, GET_LEVEL(ch), WEAR_SHIELD);
      break;
    default:
      send_to_char(ch, "Syntax: loadmagicspecific [mundane | minor | medium | major] "
              "[weapon | body | legs | arms | head | misc]\r\n");
      break;
  }
}

/* staff tool to load random items */
ACMD(do_loadmagic) {
  char arg1[MAX_STRING_LENGTH];
  char arg2[MAX_STRING_LENGTH];
  int number = 1;
  int grade = 0;

  two_arguments(argument, arg1, arg2);

  if (!*arg1) {
    send_to_char(ch, "Syntax: loadmagic [mundane | minor | medium | major] [# of items]\r\n");
    return;
  }

  if (*arg2 && !isdigit(arg2[0])) {
    send_to_char(ch, "The second number must be an integer.\r\n");
    return;
  }

  if (is_abbrev(arg1, "mundane"))
    grade = GRADE_MUNDANE;
  else if (is_abbrev(arg1, "minor"))
    grade = GRADE_MINOR;
  else if (is_abbrev(arg1, "medium"))
    grade = GRADE_MEDIUM;
  else if (is_abbrev(arg1, "major"))
    grade = GRADE_MAJOR;
  else {
    send_to_char(ch, "Syntax: loadmagic [mundane | minor | medium | major] [# of items]\r\n");
    return;
  }

  if (*arg2)
    number = atoi(arg2);

  if (number <= 0)
    number = 1;

  if (number >= 50)
    number = 50;

  award_magic_item(number, ch, GET_LEVEL(ch), grade);
}

