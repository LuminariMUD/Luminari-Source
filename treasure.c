/* *************************************************************************
 *   File: spec_procs.c                                Part of LuminariMUD *
 *  Usage: constants for random treasure objects                           *
 *  Author: d20mud, ported to tba/luminari by Zusuk                        *
 ************************************************************************* */

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

// External Functions
SPECIAL(shop_keeper);
ACMD(do_split);
/* these function assigns pre-determined stats for weapons/armor */
void set_weapon_values(struct obj_data *obj, int type);
void set_armor_values(struct obj_data *obj, int type);
/* this function helps ensure items are capped by a system */
int set_object_level(struct obj_data *obj);
/* these two function is for handling d20mud coins and a d20mud memory leak*/
char * change_coins(int coins);
void convert_coins(struct char_data *ch);

/***************************************************************/
/***  utility functions ***/

/* some spells are not appropriate for expendable items, this simple
 function returns TRUE if the spell is OK, FALSE if not */
bool valid_item_spell(int spellnum) {

  /* just list exceptions */
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
    case SPELL_UNUSED271:
    case SPELL_UNUSED275:
    case SPELL_UNUSED285:
    case SPELL_BLADES:
    case SPELL_I_DARKNESS:
      return FALSE;
  }
  return TRUE;
}

/* simple function to give a random metal type */
int choose_metal_material(void) {
  int roll = dice(1, 9);

  if (roll == 1)
    return MATERIAL_GOLD;
  if (roll == 2)
    return MATERIAL_ADAMANTINE;
  if (roll == 3)
    return MATERIAL_MITHRIL;
  if (roll == 4)
    return MATERIAL_IRON;
  if (roll == 5)
    return MATERIAL_COPPER;
  if (roll == 6)
    return MATERIAL_PLATINUM;
  if (roll == 7)
    return MATERIAL_BRASS;
  if (roll == 8)
    return MATERIAL_BRONZE;
  else
    return MATERIAL_STEEL;
}

/* simple function to give a random cloth type */
int choose_cloth_material(void) {
  int roll = dice(1, 7);

  if (roll == 1)
    return MATERIAL_COTTON;
  if (roll == 2)
    return MATERIAL_SATIN;
  if (roll == 3)
    return MATERIAL_BURLAP;
  if (roll == 4)
    return MATERIAL_VELVET;
  if (roll == 5)
    return MATERIAL_WOOL;
  if (roll == 6)
    return MATERIAL_SILK;
  else
    return MATERIAL_HEMP;
}

/* a function to determine a random weapon type 
int determine_random_weapon_type(void) {
  int roll1;
  int roll2;
  int roll3;

  roll1 = dice(1, 100);
  roll2 = dice(1, 100);
  roll3 = dice(1, 100);

  if (roll3 <= 67) {
    roll1 = dice(1, NUM_WEAPON_TYPES);
    return roll1;
  }
  if (roll1 <= 70) {
    if (roll2 <= 4)
      return WEAPON_TYPE_DAGGER;
    else if (roll2 <= 14)
      return WEAPON_TYPE_GREAT_AXE;
    else if (roll2 <= 24)
      return WEAPON_TYPE_GREAT_SWORD;
    else if (roll2 <= 28)
      return WEAPON_TYPE_KAMA;
    else if (roll2 <= 41)
      return WEAPON_TYPE_LONG_SWORD;
    else if (roll2 <= 45)
      return WEAPON_TYPE_LIGHT_MACE;
    else if (roll2 <= 50)
      return WEAPON_TYPE_HEAVY_MACE;
    else if (roll2 <= 54)
      return WEAPON_TYPE_NUNCHAKU;
    else if (roll2 <= 57)
      return WEAPON_TYPE_QUARTERSTAFF;
    else if (roll2 <= 61)
      return WEAPON_TYPE_RAPIER;
    else if (roll2 <= 66)
      return WEAPON_TYPE_SCIMITAR;
    else if (roll2 <= 70)
      return WEAPON_TYPE_SHORTSPEAR;
    else if (roll2 <= 74)
      return WEAPON_TYPE_UNARMED;
    else if (roll2 <= 84)
      return WEAPON_TYPE_BASTARD_SWORD;
    else if (roll2 <= 89)
      return WEAPON_TYPE_SHORT_SWORD;
    else
      return WEAPON_TYPE_DWARVEN_WAR_AXE;
  } else if (roll1 <= 80) {
    if (roll2 <= 3)
      return WEAPON_TYPE_DOUBLE_AXE;
    else if (roll2 <= 7)
      return WEAPON_TYPE_BATTLE_AXE;
    else if (roll2 <= 10)
      return WEAPON_TYPE_SPIKED_CHAIN;
    else if (roll2 <= 12)
      return WEAPON_TYPE_CLUB;
    else if (roll2 <= 16)
      return WEAPON_TYPE_HAND_CROSSBOW;
    else if (roll2 <= 21) {
      if (dice(1, 2) - 1)
        return WEAPON_TYPE_HEAVY_REP_XBOW;
      else
        return WEAPON_TYPE_LIGHT_REP_XBOW;
    } else if (roll2 <= 23)
      return WEAPON_TYPE_FALCHION;
    else if (roll2 <= 26)
      return WEAPON_TYPE_DIRE_FLAIL;
    else if (roll2 <= 35)
      return WEAPON_TYPE_FLAIL;
    else if (roll2 <= 39)
      return WEAPON_TYPE_SIANGHAM;
    else if (roll2 <= 41)
      return WEAPON_TYPE_GLAIVE;
    else if (roll2 <= 43)
      return WEAPON_TYPE_GREAT_CLUB;
    else if (roll2 <= 45)
      return WEAPON_TYPE_GUISARME;
    else if (roll2 <= 48)
      return WEAPON_TYPE_HALBERD;
    else if (roll2 <= 51)
      return WEAPON_TYPE_UNARMED;
    else if (roll2 <= 54)
      return WEAPON_TYPE_HOOKED_HAMMER;
    else if (roll2 <= 56)
      return WEAPON_TYPE_LIGHT_HAMMER;
    else if (roll2 <= 58)
      return WEAPON_TYPE_HAND_AXE;
    else if (roll2 <= 61)
      return WEAPON_TYPE_KUKRI;
    else if (roll2 <= 65)
      return WEAPON_TYPE_LANCE;
    else if (roll2 <= 67)
      return WEAPON_TYPE_LONGSPEAR;
    else if (roll2 <= 70)
      return WEAPON_TYPE_MORNINGSTAR;
    else if (roll2 <= 72)
      return WEAPON_TYPE_NET;
    else if (roll2 <= 74)
      return WEAPON_TYPE_HEAVY_PICK;
    else if (roll2 <= 76)
      return WEAPON_TYPE_LIGHT_PICK;
    else if (roll2 <= 78)
      return WEAPON_TYPE_RANSEUR;
    else if (roll2 <= 80)
      return WEAPON_TYPE_SAP;
    else if (roll2 <= 82)
      return WEAPON_TYPE_SCYTHE;
    else if (roll2 <= 84)
      return WEAPON_TYPE_SHURIKEN;
    else if (roll2 <= 86)
      return WEAPON_TYPE_SICKLE;
    else if (roll2 <= 89)
      return WEAPON_TYPE_2_BLADED_SWORD;
    else if (roll2 <= 91)
      return WEAPON_TYPE_TRIDENT;
    else if (roll2 <= 94)
      return WEAPON_TYPE_DWARVEN_URGOSH;
    else if (roll2 <= 97)
      return WEAPON_TYPE_WARHAMMER;
    else
      return WEAPON_TYPE_WHIP;
  } else {
    if (roll2 <= 10)
      return WEAPON_TYPE_THROWING_AXE;
    else if (roll2 <= 25)
      return WEAPON_TYPE_HEAVY_CROSSBOW;
    else if (roll2 <= 35)
      return WEAPON_TYPE_LIGHT_CROSSBOW;
    else if (roll2 <= 39)
      return WEAPON_TYPE_DART;
    else if (roll2 <= 41)
      return WEAPON_TYPE_JAVELIN;
    else if (roll2 <= 46)
      return WEAPON_TYPE_SHORT_BOW;
    else if (roll2 <= 61)
      return WEAPON_TYPE_COMPOSITE_SHORTBOW;
    else if (roll2 <= 65)
      return WEAPON_TYPE_SLING;
    else if (roll2 <= 75)
      return WEAPON_TYPE_LONG_BOW;
    else
      return WEAPON_TYPE_COMPOSITE_LONGBOW;
  }

  return WEAPON_TYPE_UNARMED;
}
 */

/* function to create a random crystal */
void get_random_crystal(struct char_data *ch, int level) {
  struct obj_data *obj = NULL;
  int val = 0;
  int val2 = 0;
  int desc = -1;
  int color_1 = -1;
  int color_2 = -1;
  int i = 0;
  int roll = dice(1, 100);
  int size = 0;
  char buf[200];

  obj = read_object(64100, VIRTUAL);

  if (!obj)
    return;

  GET_OBJ_TYPE(obj) = ITEM_CRYSTAL;
  GET_OBJ_COST(obj) = 0;
  GET_OBJ_LEVEL(obj) = level;
  GET_OBJ_MATERIAL(obj) = MATERIAL_CRYSTAL;

  switch (dice(1, 13)) {
    case 1:
      val = APPLY_AC;
      break;
    case 2:
      val = APPLY_HITROLL;
      break;
    case 3:
      val = APPLY_DAMROLL;
      break;
    case 4:
      val = APPLY_STR;
      break;
    case 5:
      val = APPLY_CON;
      break;
    case 6:
      val = APPLY_DEX;
      break;
    case 7:
      val = APPLY_INT;
      break;
    case 8:
      val = APPLY_WIS;
      break;
    case 9:
      val = APPLY_CHA;
      break;
    case 10:
      val = APPLY_MOVE;
      break;
    case 11:
      val = APPLY_SAVING_FORT;
      break;
    case 12:
      val = APPLY_SAVING_REFL;
      break;
    case 13:
      val = APPLY_SAVING_WILL;
      break;
  }

  GET_OBJ_VAL(obj, 0) = val;
  GET_OBJ_VAL(obj, 1) = val2;

  i = 0;
  while (*(colors + i++)) {
    /* counting array */
  }
  size = i;
  color_1 = MAX(1, dice(1, (int) size) - 1);
  color_2 = MAX(1, dice(1, (int) size) - 1);
  while (color_2 == color_1)
    color_2 = MAX(1, dice(1, (int) size) - 1);
  i = 0;
  while (*(crystal_descs + i++)) {
    /* counting array */
  }
  size = i;
  desc = MAX(1, dice(1, (int) size) - 1);

  roll = dice(1, 100);

  if (roll >= 91) { // two colors and descriptor
    sprintf(buf, "crystal %s %s %s", colors[color_1 - 1], colors[color_2 - 1], crystal_descs[desc - 1]);
    obj->name = strdup(buf);
    sprintf(buf, "a  %s, %s and %s crystal", crystal_descs[desc - 1], colors[color_1 - 1], colors[color_2 - 1]);
    obj->short_description = strdup(buf);
    sprintf(buf, "A %s, %s and %s crystal lies here.", crystal_descs[desc - 1], colors[color_1 - 1], colors[color_2 - 1]);
    obj->description = strdup(buf);

  } else if (roll >= 66) { // one color and descriptor
    sprintf(buf, "crystal %s %s", colors[color_1 - 1], crystal_descs[desc - 1]);
    obj->name = strdup(buf);
    sprintf(buf, "a %s %s crystal", crystal_descs[desc - 1], colors[color_1 - 1]);
    obj->short_description = strdup(buf);
    sprintf(buf, "A %s %s crystal lies here.", crystal_descs[desc - 1], colors[color_1 - 1]);
    obj->description = strdup(buf);
  } else if (roll >= 41) { // two colors no descriptor
    sprintf(buf, "crystal %s %s", colors[color_1 - 1], colors[color_2 - 1]);
    obj->name = strdup(buf);
    sprintf(buf, "a %s and %s crystal", colors[color_1 - 1], colors[color_2 - 1]);
    obj->short_description = strdup(buf);
    sprintf(buf, "A %s and %s crystal lies here.", colors[color_1 - 1], colors[color_2 - 1]);
    obj->description = strdup(buf);
  } else if (roll >= 21) {// one color no descriptor
    sprintf(buf, "crystal %s", colors[color_1 - 1]);
    obj->name = strdup(buf);
    sprintf(buf, "a %s crystal", colors[color_1 - 1]);
    obj->short_description = strdup(buf);
    sprintf(buf, "A %s crystal lies here.", colors[color_1 - 1]);
    obj->description = strdup(buf);
  } else {// descriptor only
    sprintf(buf, "crystal %s", crystal_descs[desc - 1]);
    obj->name = strdup(buf);
    sprintf(buf, "a %s crystal", crystal_descs[desc - 1]);
    obj->short_description = strdup(buf);
    sprintf(buf, "A %s crystal lies here.", crystal_descs[desc - 1]);
    obj->description = strdup(buf);
  }

  obj_to_char(obj, ch);

  if (!(IS_NPC(ch) && IS_MOB(ch) && GET_MOB_SPEC(ch) == shop_keeper)) {
    send_to_char(ch, "\tYYou have found %s.\tn\r\n", obj->short_description);
    act("\tY$n has found $p.\tn", true, ch, obj, 0, TO_ROOM);
  }
}

/* function to create a random essence */
void get_random_essence(struct char_data *ch, int level) {
  struct obj_data *obj = NULL;
  int roll = dice(1, 1000);
  int max = 1000 + (level * 2);

  roll += level * 20;

  if (roll <= (max / 2))
    obj = read_object(64100, VIRTUAL); // minor power essence
  else if (roll <= (70 * max / 100))
    obj = read_object(64101, VIRTUAL); // lesser power essence
  else if (roll <= (85 * max / 100))
    obj = read_object(64102, VIRTUAL); // medium power essence
  else if (roll <= (94 * max / 100))
    obj = read_object(64103, VIRTUAL); // greater power essence
  else if (roll <= (98 * max / 100))
    obj = read_object(64104, VIRTUAL); // major power essence
  else
    get_random_essence(ch, level);

  if (obj != NULL) {
    obj_to_char(obj, ch);
    if (!(IS_NPC(ch) && IS_MOB(ch) && GET_MOB_SPEC(ch) == shop_keeper)) {
      act("\tYYou have found $p.\tn", FALSE, ch, obj, 0, TO_CHAR);
      act("$n \tYhas found $p.\tn", FALSE, ch, obj, 0, TO_ROOM);
    }
  }
}

/* when groupped, determine random recipient from group */
struct char_data * find_treasure_recipient(struct char_data *killer) {

  struct char_data *k = killer;
  struct follow_type *f;
  int num_people = 1;
  int roll = 0;
  int i = 0;

  if (!AFF_FLAGGED(killer, AFF_GROUP))
    return killer;

  if (killer->master)
    k = killer->master;

  for (f = k->followers; f; f = f->next) {
    if (AFF_FLAGGED(f->follower, AFF_GROUP) && IN_ROOM(killer) == IN_ROOM(f->follower))
      num_people++;
  }

  roll = dice(1, num_people);

  if (roll == 1)
    return k;

  i = 1;

  for (f = k->followers; f; f = f->next) {
    if (AFF_FLAGGED(f->follower, AFF_GROUP) && IN_ROOM(killer) == IN_ROOM(f->follower))
      i++;
    if (i == roll)
      return f->follower;
  }

  return killer;
}

/***************************************************************/
/***  primary functions ***/

/* function to determine if target should get a random 'crafting component'
   used, for example, before make_corpse() in fight.c */
void determine_crafting_component_treasure(struct char_data *ch, struct char_data *mob) {
  int level = 0;
  int roll = dice(1, 100);

  if (!mob)
    return;

  if (!IS_NPC(mob))
    return;

  level = GET_LEVEL(mob);

  if (roll > 2)
    return;

  if (roll <= 1) {
    get_random_crystal(ch, level);
    return;
  }

  get_random_essence(ch, level);
}

/* this function determines whether the character will get treasure or not
 *   for example, called before make_corpse() when killing a mobile */
void determine_treasure(struct char_data *ch, struct char_data *mob) {
  int roll = 0;
  int factor = 30;
  int gold = 0;
  int level = GET_LEVEL(mob);
  char gold_buf[MAX_STRING_LENGTH];
  char buf[MAX_STRING_LENGTH];
  int grade = GRADE_MUNDANE;

  if (IS_NPC(ch) && AFF_FLAGGED(ch, AFF_CHARM) && ch->master)
    ch = ch->master;

  gold = dice(GET_LEVEL(mob) * 2, GET_LEVEL(mob) * 5);

  if (!IS_NPC(mob))
    return;

  roll = dice(1, 100);

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

  if (dice(1, 1000) <= (factor)) {
    award_magic_item(1, ch, GET_LEVEL(mob), grade);

    if (!(IS_NPC(ch) && IS_MOB(ch) && GET_MOB_SPEC(ch) == shop_keeper)) {
      sprintf(buf, "\tYYou have found %s coins on $N's corpse!\tn", change_coins(gold));
      act(buf, FALSE, ch, 0, mob, TO_CHAR);
      sprintf(buf, "$n \tYhas has found %s coins on $N's corpse!\tn", change_coins(gold));
      act(buf, FALSE, ch, 0, mob, TO_NOTVICT);
      GET_GOLD(ch) += gold;
      if (IS_AFFECTED(ch, AFF_GROUP) && (gold > 0) &&
              PRF_FLAGGED(ch, PRF_AUTOSPLIT)) {
        sprintf(gold_buf, "%d", gold);
        do_split(ch, gold_buf, 0, 0);
      }
    }
  }
}

/* character should get treasure, roll dice for what items to give out */
void award_magic_item(int number, struct char_data *ch, int level, int grade) {
  int i = 0;

  for (i = 0; i <= number; i++) {
    if (dice(1, 1000) <= 1)
      award_special_magic_item(ch);
    if (dice(1, 100) <= 60)
      award_expendable_item(ch, grade, TYPE_POTION);
    if (dice(1, 100) <= 30)
      award_expendable_item(ch, grade, TYPE_SCROLL);
    if (dice(1, 100) <= 20)
      award_expendable_item(ch, grade, TYPE_WAND);
    if (dice(1, 100) <= 10)
      award_expendable_item(ch, grade, TYPE_STAFF);
    if (dice(1, 100) <= 10)
      award_magic_weapon(ch, grade, level);
    if (dice(1, 100) <= 20)
      award_misc_magic_item(ch, grade, level);
    if (dice(1, 100) <= 10)
      award_magic_armor(ch, grade, level);
  }
}

/* awards potions or scroll or wand or staff */

/* reminder, stock:
   obj value 0:  spell level
   obj value 1:  potion/scroll - spell #1; staff/wand - max charges
   obj value 2:  potion/scroll - spell #2; staff/wand - charges remaining
   obj value 3:  potion/scroll - spell #3; staff/wand - spell #1
 */
void award_expendable_item(struct char_data *ch, int grade, int type) {
  int class = CLASS_UNDEFINED, spell_level = 1, spell_num = SPELL_RESERVED_DBC;
  int color1 = 0, color2 = 0, desc = 0, roll = dice(1, 100), i = 0;
  struct obj_data *obj = NULL;
  char keywords[100] = {'\0'}, buf[MAX_STRING_LENGTH] = {'\0'};

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
    default:
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
      spell_level = rand_number(15, 18);
      break;
  }

  /* Loop through spell list randomly! conditions of exit:
     - invalid spell (sub-function)
     - does not match class
     - does not match level
   */
  do {
    spell_num = rand_number(1, NUM_SPELLS - 1);
  } while (spell_level < spell_info[spell_num].min_level[class] ||
          !valid_item_spell(spell_num));

  /* first assign two random colors for usage */
  color1 = rand_number(0, NUM_A_COLORS);
  color2 = rand_number(0, NUM_A_COLORS);
  /* make sure they are not the same colors */
  while (color2 == color1)
    color2 = rand_number(0, NUM_A_COLORS);

  /* load prototype */
  obj = read_object(ITEM_PROTOTYPE, VIRTUAL);

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
      GET_OBJ_COST(obj) = spell_level * spell_level * 30;
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
      GET_OBJ_VAL(obj, 1) = dice(1, 10);
      GET_OBJ_VAL(obj, 2) = 10;
      GET_OBJ_VAL(obj, 3) = spell_num;

      GET_OBJ_COST(obj) = 50;
      for (i = 1; i < spell_level; i++)
        GET_OBJ_COST(obj) *= 3;
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
      GET_OBJ_VAL(obj, 1) = dice(1, 8);
      GET_OBJ_VAL(obj, 2) = 8;
      GET_OBJ_VAL(obj, 3) = spell_num;

      GET_OBJ_COST(obj) = 110;
      for (i = 1; i < spell_level; i++)
        GET_OBJ_COST(obj) *= 3;
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

      GET_OBJ_COST(obj) = 10;
      for (i = 1; i < spell_level; i++)
        GET_OBJ_COST(obj) *= 3;
      GET_OBJ_MATERIAL(obj) = MATERIAL_PAPER;
      GET_OBJ_TYPE(obj) = ITEM_SCROLL;
      GET_OBJ_LEVEL(obj) = dice(1, spell_level);
      break;
  }

  SET_BIT_AR(GET_OBJ_EXTRA(obj), ITEM_MAGIC);

  obj_to_char(obj, ch);

  send_to_char(ch, "\tYYou have found %s in a nearby lair!\tn\r\n", obj->short_description);
  sprintf(buf, "$n \tYhas found %s in a nearby lair!\tn", obj->short_description);
  act(buf, FALSE, ch, 0, ch, TO_NOTVICT);
}

/* give away random magic armor */

/*
 method:
 * 1)  determine armor
 * 2)  determine material
 * 3)  assign description
 * 4)  determine modifier (if applicable)
 * 5)  determine amount (if applicable)
 */
void award_magic_armor(struct char_data *ch, int grade, int moblevel) {
  struct obj_data *obj = NULL;
  int vnum = -1, material = MATERIAL_BRONZE, roll = 0;
  int rare_grade = 0, color1 = 0, color2 = 0;
  char desc[MEDIUM_STRING] = {'\0'}, *armor_name = NULL;
  char buf[MAX_STRING_LENGTH] = {'\0'};


  /* determine if rare or not */
  roll = dice(1, 100);
  if (roll == 1) {
    rare_grade = 3;
    sprintf(desc, "\tM[Mythical]\tn");
  } else if (roll <= 6) {
    rare_grade = 2;
    sprintf(desc, "\tY[Legendary]\tn");
  } else if (roll <= 16) {
    rare_grade = 1;
    sprintf(desc, "\tG[Rare]\tn");
  }

  /* find a random piece of armor
   * assign base material
   * and last but not least, give appropriate start of description
   *  */
  switch (dice(1, NUM_ARMOR_MOLDS)) {

      /* body pieces */
    case 1:
      vnum = PLATE_BODY;
      material = MATERIAL_BRONZE;
      sprintf(desc, "%s a suit of", desc);
      armor_name = "plate mail armor";
      break;
    case 2:
      vnum = HALFPLATE_BODY;
      material = MATERIAL_BRONZE;
      sprintf(desc, "%s a suit of", desc);
      armor_name = "half plate armor";
      break;
    case 3:
      vnum = SPLINT_BODY;
      material = MATERIAL_BRONZE;
      sprintf(desc, "%s a suit of", desc);
      armor_name = "splint mail armor";
      break;
    case 4:
      vnum = BREASTPLATE_BODY;
      material = MATERIAL_BRONZE;
      sprintf(desc, "%s a suit of", desc);
      armor_name = "breastplate armor";
      break;
    case 5:
      vnum = CHAIN_BODY;
      material = MATERIAL_BRONZE;
      sprintf(desc, "%s a suit of", desc);
      armor_name = "chain mail armor";
      break;
    case 6:
      vnum = STUD_LEATHER_BODY;
      material = MATERIAL_LEATHER;
      sprintf(desc, "%s a suit of", desc);
      armor_name = "studded leather armor";
      break;
    case 7:
      vnum = LEATHER_BODY;
      material = MATERIAL_LEATHER;
      sprintf(desc, "%s a suit of", desc);
      armor_name = "leather armor";
      break;
    case 8:
      vnum = PADDED_BODY;
      material = MATERIAL_COTTON;
      sprintf(desc, "%s a suit of", desc);
      armor_name = "padded armor";
      break;
    case 9:
      vnum = CLOTH_BODY;
      material = MATERIAL_COTTON;
      armor_name = "robes";
      break;

      /* head pieces */
    case 10:
      vnum = PLATE_HELM;
      material = MATERIAL_BRONZE;
      sprintf(desc, "%s a", desc);
      armor_name = "plate mail helm";
      break;
    case 11:
      vnum = HALFPLATE_HELM;
      material = MATERIAL_BRONZE;
      sprintf(desc, "%s a", desc);
      armor_name = "half plate helm";
      break;
    case 12:
      vnum = SPLINT_HELM;
      material = MATERIAL_BRONZE;
      sprintf(desc, "%s a", desc);
      armor_name = "splint mail helm";
      break;
    case 13:
      vnum = PIECEPLATE_HELM;
      material = MATERIAL_BRONZE;
      sprintf(desc, "%s a", desc);
      armor_name = "piece plate helm";
      break;
    case 14:
      vnum = CHAIN_HELM;
      material = MATERIAL_BRONZE;
      sprintf(desc, "%s a", desc);
      armor_name = "chain mail helm";
      break;
    case 15:
      vnum = STUD_LEATHER_HELM;
      material = MATERIAL_LEATHER;
      sprintf(desc, "%s a", desc);
      armor_name = "studded leather helm";
      break;
    case 16:
      vnum = LEATHER_HELM;
      material = MATERIAL_LEATHER;
      sprintf(desc, "%s a", desc);
      armor_name = "leather helm";
      break;
    case 17:
      vnum = PADDED_HELM;
      material = MATERIAL_COTTON;
      sprintf(desc, "%s a", desc);
      armor_name = "padded armor helm";
      break;
    case 18:
      vnum = CLOTH_HELM;
      material = MATERIAL_COTTON;
      sprintf(desc, "%s a", desc);
      armor_name = "cloth hood";
      break;

      /* arm pieces */
    case 19:
      vnum = PLATE_ARMS;
      material = MATERIAL_BRONZE;
      sprintf(desc, "%s a set of", desc);
      armor_name = "plate mail vambraces";
      break;
    case 20:
      vnum = HALFPLATE_ARMS;
      material = MATERIAL_BRONZE;
      sprintf(desc, "%s a set of", desc);
      armor_name = "half plate vambraces";
      break;
    case 21:
      vnum = SPLINT_ARMS;
      material = MATERIAL_BRONZE;
      sprintf(desc, "%s a set of", desc);
      armor_name = "splint mail vambraces";
      break;
    case 22:
      vnum = CHAIN_ARMS;
      material = MATERIAL_BRONZE;
      sprintf(desc, "%s a set of", desc);
      armor_name = "chain mail sleeves";
      break;
    case 23:
      vnum = STUD_LEATHER_ARMS;
      material = MATERIAL_LEATHER;
      sprintf(desc, "%s a set of", desc);
      armor_name = "studded leather sleeves";
      break;
    case 24:
      vnum = LEATHER_ARMS;
      material = MATERIAL_LEATHER;
      sprintf(desc, "%s a set of", desc);
      armor_name = "leather sleeves";
      break;
    case 25:
      vnum = PADDED_ARMS;
      material = MATERIAL_COTTON;
      sprintf(desc, "%s a set of", desc);
      armor_name = "padded armor sleeves";
      break;
    case 26:
      vnum = CLOTH_ARMS;
      material = MATERIAL_COTTON;
      sprintf(desc, "%s a set of", desc);
      armor_name = "cloth sleeves";
      break;

      /* leg pieces */
      /* arm pieces */
    case 27:
      vnum = PLATE_LEGS;
      material = MATERIAL_BRONZE;
      sprintf(desc, "%s a set of", desc);
      armor_name = "plate mail greaves";
      break;
    case 28:
      vnum = HALFPLATE_LEGS;
      material = MATERIAL_BRONZE;
      sprintf(desc, "%s a set of", desc);
      armor_name = "half plate greaves";
      break;
    case 29:
      vnum = SPLINT_LEGS;
      material = MATERIAL_BRONZE;
      sprintf(desc, "%s a set of", desc);
      armor_name = "splint mail greaves";
      break;
    case 30:
      vnum = CHAIN_LEGS;
      material = MATERIAL_BRONZE;
      sprintf(desc, "%s a set of", desc);
      armor_name = "chain mail leggings";
      break;
    case 31:
      vnum = STUD_LEATHER_LEGS;
      material = MATERIAL_LEATHER;
      sprintf(desc, "%s a set of", desc);
      armor_name = "studded leather leggings";
      break;
    case 32:
      vnum = LEATHER_LEGS;
      material = MATERIAL_LEATHER;
      sprintf(desc, "%s a set of", desc);
      armor_name = "leather leggings";
      break;
    case 33:
      vnum = PADDED_LEGS;
      material = MATERIAL_COTTON;
      sprintf(desc, "%s a set of", desc);
      armor_name = "padded armor leggings";
      break;
    case 34:
      vnum = CLOTH_LEGS;
      material = MATERIAL_COTTON;
      sprintf(desc, "%s a set of", desc);
      armor_name = "cloth pants";
      break;

      /* shields */
    case 35:
      vnum = SHIELD_MEDIUM;
      material = MATERIAL_WOOD;
      sprintf(desc, "%s a", desc);
      armor_name = "medium shield";
      break;
    case 36:
      vnum = SHIELD_LARGE;
      material = MATERIAL_WOOD;
      sprintf(desc, "%s a", desc);
      armor_name = "large shield";
      break;
    case 37:
      vnum = SHIELD_TOWER;
      material = MATERIAL_WOOD;
      sprintf(desc, "%s a", desc);
      armor_name = "tower shield";
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
    case MATERIAL_LEATHER:
      switch (grade) {
        case GRADE_MUNDANE:
        case GRADE_MINOR:
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

  // Find out if there's an armor special adjective in the desc
  roll = dice(1, 3);
  if (roll == 3)
    sprintf(desc, "%s %s", desc,
          armor_special_descs[rand_number(0, NUM_A_ARMOR_SPECIAL_DESCS)]);

  // Find out if there's a color describer in the desc
  roll = dice(1, 5);
  // There's one color describer in the desc so find out which one
  if (roll >= 4)
    sprintf(desc, "%s %s", desc, colors[color1]);

    // There's two colors describer in the desc so find out which one
  else if (roll == 3)
    sprintf(desc, "%s %s and %s", desc, colors[color1], colors[color2]);

  // Insert the material type
  sprintf(desc, "%s %s", desc, material_name[material]);

  // Insert the armor type
  sprintf(desc, "%s %s", desc, armor_name);

  // Find out if the armor has any crests or symbols
  roll = dice(1, 8);

  // It has a crest so find out which and set the desc
  if (roll >= 7)
    sprintf(desc, "%s with %s %s crest", desc,
          AN(armor_crests[rand_number(0, NUM_A_ARMOR_CRESTS)]),
          armor_crests[rand_number(0, NUM_A_ARMOR_CRESTS)]);

    // It has a symbol so find out which and set the desc
  else if (roll >= 5)
    sprintf(desc, "%s covered in symbols of %s %s", desc,
          AN(armor_crests[rand_number(0, NUM_A_ARMOR_CRESTS)]),
          armor_crests[rand_number(0, NUM_A_ARMOR_CRESTS)]);


  // Set descriptions
  // keywords
  obj->name = strdup(desc);
  obj->short_description = strdup(desc);
  desc[0] = toupper(desc[0]);
  sprintf(desc, "%s is lying here.", desc);
  obj->description = strdup(desc);
  GET_OBJ_LEVEL(obj) = MAX(1, set_object_level(obj));

  obj->affected[0].modifier += (rare_grade * 10);
  GET_OBJ_COST(obj) = 100 + GET_OBJ_LEVEL(obj) * 50 * MAX(1, GET_OBJ_LEVEL(obj) - 1);
  GET_OBJ_COST(obj) = GET_OBJ_COST(obj) * (3 + (rare_grade * 2)) / 3;
  GET_OBJ_RENT(obj) = GET_OBJ_COST(obj) / 25;

  if (grade > GRADE_MUNDANE)
    SET_BIT_AR(GET_OBJ_EXTRA(obj), ITEM_MAGIC);

  roll = dice(1, 100);
  while (roll == 100) {
    roll = dice(1, 100);
    obj->affected[0].modifier += 10;
  }

  obj_to_char(obj, ch);

  if (!(IS_NPC(ch) && IS_MOB(ch) && GET_MOB_SPEC(ch) == shop_keeper)) {
    send_to_char(ch, "@YYou have found %s in a nearby lair!@n\r\n", obj->short_description);

    sprintf(buf, "@Y$n has found %s in a nearby lair!@n", obj->short_description);
    act(buf, FALSE, ch, 0, ch, TO_NOTVICT);
  }
}

/* gives out a random magic weapon */
/*
void award_magic_weapon(struct char_data *ch, int grade, int moblevel) {
  int i = 0;
  int roll = 0;
  int size = 0;
  int hit_bonus = 0;
  int dmg_bonus = 0;
  struct obj_data *obj;
  char buf[MAX_STRING_LENGTH];
  int special_roll = 0;
  int head_color_roll = 0;
  int hilt_color_roll = 0;
  char special[100];
  char head_color[100];
  char hilt_color[100];
  int rare = dice(1, 100);
  int raregrade = 0;

  if (grade == GRADE_MUNDANE) {
    hit_bonus = 1;
    dmg_bonus = 0;
  } else if (grade == GRADE_MINOR) {
    roll = dice(1, 100);

    if (roll >= 86) {
      award_magic_weapon(ch, grade, moblevel);
      return;
    } else if (roll >= 71) {
      hit_bonus = 2;
      dmg_bonus = 2;
    } else if (roll >= 51) {
      hit_bonus = dice(1, 2);
      dmg_bonus = dice(1, 2);
    } else {
      hit_bonus = 1;
      dmg_bonus = 1;
    }
  } else if (grade == GRADE_MEDIUM) {
    roll = dice(1, 100);

    if (roll >= 69) {
      award_magic_weapon(ch, grade, moblevel);
      return;
    } else if (roll >= 63) {
      award_magic_weapon(ch, grade, moblevel);
      return;
    } else if (roll >= 59) {
      hit_bonus = 4;
      dmg_bonus = 4;
    } else if (roll >= 51) {
      hit_bonus = dice(1, 2) + 2;
      dmg_bonus = dice(1, 2) + 2;
    } else if (roll >= 21) {
      hit_bonus = 3;
      dmg_bonus = 3;
    } else if (roll >= 16) {
      hit_bonus = dice(1, 2) + 1;
      dmg_bonus = dice(1, 2) + 1;
    } else if (roll >= 11) {
      hit_bonus = 2;
      dmg_bonus = 2;
    } else if (roll >= 6) {
      hit_bonus = dice(1, 2);
      dmg_bonus = dice(1, 2);
    } else {
      hit_bonus = 1;
      dmg_bonus = 1;
    }
  } else {
    roll = dice(1, 100);

    if (roll >= 64) {
      award_magic_weapon(ch, grade, moblevel);
      return;
    } else if (roll >= 50) {
      award_magic_weapon(ch, grade, moblevel);
      return;
    } else if (roll >= 39) {
      hit_bonus = 5;
      dmg_bonus = 5;
    } else if (roll >= 31) {
      hit_bonus = dice(1, 2) + 3;
      dmg_bonus = dice(1, 2) + 3;
    } else if (roll >= 21) {
      hit_bonus = 4;
      dmg_bonus = 4;
    } else if (roll >= 11) {
      hit_bonus = dice(1, 2) + 2;
      dmg_bonus = dice(1, 2) + 2;
    } else {
      hit_bonus = 3;
      dmg_bonus = 3;
    }
    moblevel -= 20;
    if (moblevel > 0) {
      hit_bonus += moblevel / 4;
      dmg_bonus += moblevel / 4;
    }
  }

  obj = read_object(30020, VIRTUAL);

  roll = dice(1, 100);

  if (roll <= 50) {
    for (i = 0; i < 10; i++)
      if (ch->player_specials->wishlist[i][0] == 1)
        roll = 0;
    if (roll == 0) {
      roll = dice(1, 10) - 1;
      while (ch->player_specials->wishlist[roll][0] != 1) {
        roll = dice(1, 10) - 1;
      }
      roll = ch->player_specials->wishlist[roll][1];
    }
  } else
    roll = determine_random_weapon_type();

  set_weapon_values(obj, roll);

  if (roll == WEAPON_TYPE_UNARMED)
    GET_OBJ_MATERIAL(obj) = MATERIAL_LEATHER;

  if (GET_OBJ_MATERIAL(obj) == MATERIAL_STEEL) {
    roll = dice(1, 100);

    if (roll >= 97) {
      GET_OBJ_MATERIAL(obj) = MATERIAL_ADAMANTINE;
    } else if (roll >= 90) {
      GET_OBJ_MATERIAL(obj) = MATERIAL_MITHRIL;
    } else if (roll >= 81) {
      GET_OBJ_MATERIAL(obj) = MATERIAL_ALCHEMAL_SILVER;
    } else if (roll >= 71) {
      GET_OBJ_MATERIAL(obj) = MATERIAL_COLD_IRON;
    } else {
      GET_OBJ_MATERIAL(obj) = MATERIAL_STEEL;
    }
  } else if (GET_OBJ_MATERIAL(obj) == MATERIAL_WOOD) {
    roll = dice(1, 100);

    if (roll >= 90) {
      GET_OBJ_MATERIAL(obj) = MATERIAL_DARKWOOD;
    } else {
      GET_OBJ_MATERIAL(obj) = MATERIAL_WOOD;
    }
  }

  if (rare == 1) {
    raregrade = 3;
  } else if (rare <= 6) {
    raregrade = 2;
  } else if (rare <= 16) {
    raregrade = 1;
  }

  char desc[50];
  if (raregrade == 0)
    sprintf(desc, "@n");
  else if (raregrade == 1)
    sprintf(desc, "@G[Rare]@n ");
  else if (raregrade == 2)
    sprintf(desc, "@Y[Legendary]@n ");
  else if (raregrade == 3)
    sprintf(desc, "@M[Mythical]@n ");

  i = 0;
  while (*(colors + i++)) {
  }
  size = i;
  head_color_roll = MAX(0, dice(1, (int) size) - 2);
  hilt_color_roll = MAX(0, dice(1, (int) size) - 2);
  sprintf(head_color, "%s", colors[head_color_roll]);
  sprintf(hilt_color, "%s", colors[hilt_color_roll]);

  if (IS_SET(weapon_list[GET_OBJ_VAL(obj, 0)].damageTypes, DAMAGE_TYPE_SLASHING)) {
    i = 0;
    while (*(blade_descs + i++)) {
    }
    size = i;
    special_roll = MAX(0, dice(1, (int) size) - 2);
    sprintf(special, "%s%s", desc, blade_descs[special_roll]);
  } else if (IS_SET(weapon_list[GET_OBJ_VAL(obj, 0)].damageTypes, DAMAGE_TYPE_PIERCING)) {
    i = 0;
    while (*(piercing_descs + i++)) {
    }
    size = i;
    special_roll = MAX(0, dice(1, (int) size) - 2);
    sprintf(special, "%s%s", desc, piercing_descs[special_roll]);
  } else {
    i = 0;
    while (*(blunt_descs + i++)) {
    }
    size = i;
    special_roll = MAX(0, dice(1, (int) size) - 2);
    sprintf(special, "%s%s", desc, blunt_descs[special_roll]);
  }

  roll = dice(1, 100);

  // special, head color, hilt color
  if (roll >= 91) {

    sprintf(buf, "%s %s-%s %s %s %s %s", special,
            head_color, head_types[weapon_list[GET_OBJ_VAL(obj, 0)].head_type],
            material_names[GET_OBJ_MATERIAL(obj)], weapon_list[GET_OBJ_VAL(obj, 0)].name,
            hilt_color,
            handle_types[weapon_list[GET_OBJ_VAL(obj, 0)].handle_type]);
    obj->name = strdup(buf);

    sprintf(buf, "%s %s, %s-%s %s %s with %s %s %s", a_or_an(special), special,
            head_color, head_types[weapon_list[GET_OBJ_VAL(obj, 0)].head_type],
            material_names[GET_OBJ_MATERIAL(obj)], weapon_list[GET_OBJ_VAL(obj, 0)].name,
            a_or_an(hilt_color), hilt_color,
            handle_types[weapon_list[GET_OBJ_VAL(obj, 0)].handle_type]);
    obj->short_description = strdup(buf);

    sprintf(buf, "%s %s, %s-%s %s %s with %s %s %s lies here.", a_or_an(special),
            special,
            head_color, head_types[weapon_list[GET_OBJ_VAL(obj, 0)].head_type],
            material_names[GET_OBJ_MATERIAL(obj)], weapon_list[GET_OBJ_VAL(obj, 0)].name,
            a_or_an(hilt_color), hilt_color,
            handle_types[weapon_list[GET_OBJ_VAL(obj, 0)].handle_type]);
 *buf = UPPER(*buf);
    obj->description = strdup(buf);

  }    // special, head color
  else if (roll >= 81) {

    sprintf(buf, "%s %s-%s %s %s", special,
            head_color, head_types[weapon_list[GET_OBJ_VAL(obj, 0)].head_type],
            material_names[GET_OBJ_MATERIAL(obj)], weapon_list[GET_OBJ_VAL(obj, 0)].name);
    obj->name = strdup(buf);

    sprintf(buf, "%s %s, %s-%s %s %s", a_or_an(special), special,
            head_color, head_types[weapon_list[GET_OBJ_VAL(obj, 0)].head_type],
            material_names[GET_OBJ_MATERIAL(obj)], weapon_list[GET_OBJ_VAL(obj, 0)].name);
    obj->short_description = strdup(buf);

    sprintf(buf, "%s %s, %s-%s %s %s lies here.", a_or_an(special),
            special,
            head_color, head_types[weapon_list[GET_OBJ_VAL(obj, 0)].head_type],
            material_names[GET_OBJ_MATERIAL(obj)], weapon_list[GET_OBJ_VAL(obj, 0)].name);
 *buf = UPPER(*buf);
    obj->description = strdup(buf);

  }    // special, hilt color
  else if (roll >= 71) {

    sprintf(buf, "%s %s %s %s %s", special,
            material_names[GET_OBJ_MATERIAL(obj)], weapon_list[GET_OBJ_VAL(obj, 0)].name,
            hilt_color,
            handle_types[weapon_list[GET_OBJ_VAL(obj, 0)].handle_type]);
    obj->name = strdup(buf);

    sprintf(buf, "%s %s %s %s with %s %s %s", a_or_an(special), special,
            material_names[GET_OBJ_MATERIAL(obj)], weapon_list[GET_OBJ_VAL(obj, 0)].name,
            a_or_an(hilt_color), hilt_color,
            handle_types[weapon_list[GET_OBJ_VAL(obj, 0)].handle_type]);
    obj->short_description = strdup(buf);

    sprintf(buf, "%s %s %s %s with %s %s %s lies here.", a_or_an(special),
            special,
            material_names[GET_OBJ_MATERIAL(obj)], weapon_list[GET_OBJ_VAL(obj, 0)].name,
            a_or_an(hilt_color), hilt_color,
            handle_types[weapon_list[GET_OBJ_VAL(obj, 0)].handle_type]);
 *buf = UPPER(*buf);
    obj->description = strdup(buf);


  }    // head color, hilt color
  else if (roll >= 41) {

    sprintf(buf, "%s-%s %s %s %s %s",
            head_color, head_types[weapon_list[GET_OBJ_VAL(obj, 0)].head_type],
            material_names[GET_OBJ_MATERIAL(obj)], weapon_list[GET_OBJ_VAL(obj, 0)].name,
            hilt_color,
            handle_types[weapon_list[GET_OBJ_VAL(obj, 0)].handle_type]);
    obj->name = strdup(buf);

    sprintf(buf, "%s %s-%s %s %s with %s %s %s", a_or_an(head_color),
            head_color, head_types[weapon_list[GET_OBJ_VAL(obj, 0)].head_type],
            material_names[GET_OBJ_MATERIAL(obj)], weapon_list[GET_OBJ_VAL(obj, 0)].name,
            a_or_an(hilt_color), hilt_color,
            handle_types[weapon_list[GET_OBJ_VAL(obj, 0)].handle_type]);
    obj->short_description = strdup(buf);

    sprintf(buf, "%s %s-%s %s %s with %s %s %s lies here.", a_or_an(head_color),
            head_color, head_types[weapon_list[GET_OBJ_VAL(obj, 0)].head_type],
            material_names[GET_OBJ_MATERIAL(obj)], weapon_list[GET_OBJ_VAL(obj, 0)].name,
            a_or_an(hilt_color), hilt_color,
            handle_types[weapon_list[GET_OBJ_VAL(obj, 0)].handle_type]);
 *buf = UPPER(*buf);
    obj->description = strdup(buf);

  }    // head color
  else if (roll >= 31) {

    sprintf(buf, "%s-%s %s %s",
            head_color, head_types[weapon_list[GET_OBJ_VAL(obj, 0)].head_type],
            material_names[GET_OBJ_MATERIAL(obj)], weapon_list[GET_OBJ_VAL(obj, 0)].name);
    obj->name = strdup(buf);

    sprintf(buf, "%s %s-%s %s %s", a_or_an(head_color),
            head_color, head_types[weapon_list[GET_OBJ_VAL(obj, 0)].head_type],
            material_names[GET_OBJ_MATERIAL(obj)], weapon_list[GET_OBJ_VAL(obj, 0)].name);
    obj->short_description = strdup(buf);

    sprintf(buf, "%s %s-%s %s %s lies here.", a_or_an(head_color),
            head_color, head_types[weapon_list[GET_OBJ_VAL(obj, 0)].head_type],
            material_names[GET_OBJ_MATERIAL(obj)], weapon_list[GET_OBJ_VAL(obj, 0)].name);
 *buf = UPPER(*buf);
    obj->description = strdup(buf);


  }    // hilt color
  else if (roll >= 21) {

    sprintf(buf, "%s %s %s %s",
            material_names[GET_OBJ_MATERIAL(obj)], weapon_list[GET_OBJ_VAL(obj, 0)].name,
            hilt_color,
            handle_types[weapon_list[GET_OBJ_VAL(obj, 0)].handle_type]);
    obj->name = strdup(buf);

    sprintf(buf, "%s %s %s with %s %s %s", a_or_an((char *) material_names[GET_OBJ_MATERIAL(obj)]),
            material_names[GET_OBJ_MATERIAL(obj)], weapon_list[GET_OBJ_VAL(obj, 0)].name,
            a_or_an(hilt_color), hilt_color,
            handle_types[weapon_list[GET_OBJ_VAL(obj, 0)].handle_type]);
    obj->short_description = strdup(buf);

    sprintf(buf, "%s %s %s with %s %s %s lies here.",
            a_or_an((char *) material_names[GET_OBJ_MATERIAL(obj)]),
            material_names[GET_OBJ_MATERIAL(obj)], weapon_list[GET_OBJ_VAL(obj, 0)].name,
            a_or_an(hilt_color), hilt_color,
            handle_types[weapon_list[GET_OBJ_VAL(obj, 0)].handle_type]);
 *buf = UPPER(*buf);
    obj->description = strdup(buf);

  }    // special
  else if (roll >= 11) {

    sprintf(buf, "%s %s %s", special,
            material_names[GET_OBJ_MATERIAL(obj)], weapon_list[GET_OBJ_VAL(obj, 0)].name);
    obj->name = strdup(buf);

    sprintf(buf, "%s %s %s %s", a_or_an(special), special,
            material_names[GET_OBJ_MATERIAL(obj)], weapon_list[GET_OBJ_VAL(obj, 0)].name);
    obj->short_description = strdup(buf);

    sprintf(buf, "%s %s %s %s lies here.", a_or_an(special), special,
            material_names[GET_OBJ_MATERIAL(obj)], weapon_list[GET_OBJ_VAL(obj, 0)].name);
 *buf = UPPER(*buf);
    obj->description = strdup(buf);

  }    // none
  else {

    sprintf(buf, "%s %s",
            material_names[GET_OBJ_MATERIAL(obj)], weapon_list[GET_OBJ_VAL(obj, 0)].name);
    obj->name = strdup(buf);

    sprintf(buf, "%s %s %s", a_or_an((char *) material_names[GET_OBJ_MATERIAL(obj)]),
            material_names[GET_OBJ_MATERIAL(obj)], weapon_list[GET_OBJ_VAL(obj, 0)].name);
    obj->short_description = strdup(buf);

    sprintf(buf, "%s %s %s lies here.",
            a_or_an((char *) material_names[GET_OBJ_MATERIAL(obj)]),
            material_names[GET_OBJ_MATERIAL(obj)], weapon_list[GET_OBJ_VAL(obj, 0)].name);
 *buf = UPPER(*buf);
    obj->description = strdup(buf);

  }

  obj->name = strdup(replace_string(obj->name, "unarmed", "gauntlet"));
  obj->short_description = strdup(replace_string(obj->short_description, "unarmed", "gauntlet"));
  obj->description = strdup(replace_string(obj->description, "unarmed", "gauntlet"));

  SET_BIT_AR(GET_OBJ_EXTRA(obj), ITEM_UNIQUE_SAVE);

  if (dmg_bonus) {
    obj->affected[0].location = APPLY_DAMROLL;
    obj->affected[0].modifier = dmg_bonus;
  }

  if (hit_bonus) {
    obj->affected[1].location = APPLY_HITROLL;
    obj->affected[1].modifier = hit_bonus;
  }

  GET_OBJ_LEVEL(obj) = MAX(1, set_object_level(obj));

  if (GET_OBJ_LEVEL(obj) >= CONFIG_LEVEL_CAP) {
    award_magic_weapon(ch, grade, moblevel);
    return;
  }

  obj->affected[0].modifier += raregrade;
  obj->affected[1].modifier += raregrade;

  GET_OBJ_COST(obj) = 250 + GET_OBJ_LEVEL(obj) * 50 * MAX(1, GET_OBJ_LEVEL(obj) - 1);
  GET_OBJ_COST(obj) = GET_OBJ_COST(obj) * (3 + (raregrade * 2)) / 3;
  GET_OBJ_RENT(obj) = GET_OBJ_COST(obj) / 25;

  if (grade > GRADE_MUNDANE)
    SET_BIT_AR(GET_OBJ_EXTRA(obj), ITEM_MAGIC);

  obj_to_char(obj, ch);

  if (!(IS_NPC(ch) && IS_MOB(ch) && GET_MOB_SPEC(ch) == shop_keeper)) {
    send_to_char(ch, "@YYou have found %s in a nearby lair!@n\r\n", obj->short_description);

    sprintf(buf, "@Y$n has found %s in a nearby lair!@n", obj->short_description);
    act(buf, FALSE, ch, 0, ch, TO_NOTVICT);
  }
}
 */

/* gives out random armor pieces (outside of body-armor/shield) */
/*
void award_misc_magic_item(struct char_data *ch, int grade, int moblevel) {
  byte bonus = 0;
  byte roll = 0;
  byte roll2 = 0;
  int type = 0;
  sbyte jewelry = FALSE;
  int affect = 0;
  int subval = 0;
  byte misc_desc_roll_1 = 0;
  byte misc_desc_roll_2 = 0;
  byte misc_desc_roll_3 = 0;
  byte misc_desc_roll_4 = 0;
  int material = 0;
  struct obj_data *obj;
  int i = 0;
  int size = 0;
  char desc[200];
  int rare = dice(1, 100);
  int raregrade = 0;
  int val = 0;

  // Figure out which item grade power it is and then find out the bonus of the effect on the item
  roll = dice(1, 100);

  switch (grade) {
    case GRADE_MUNDANE:
      bonus = 1;
      break;
    case GRADE_MINOR:
      if (roll <= 70)
        bonus = 1;
      else
        bonus = 2;
      break;
    case GRADE_MEDIUM:
      if (roll <= 60)
        bonus = 2;
      else if (roll <= 90)
        bonus = 3;
      else
        bonus = 4;
      break;
    case GRADE_MAJOR:
      if (roll <= 40)
        bonus = 3;
      else if (roll <= 70)
        bonus = 4;
      else if (roll <= 90)
        bonus = 5;
      else
        bonus = 6;
      if ((moblevel - 20) > 0)
        bonus += MAX(0, moblevel - 20) / 3;
      break;
  }

  // Find out what type the item is, where it will be worn
  roll = dice(1, 100);

  if (roll <= 15) {
    type = ITEM_WEAR_FINGER;
    obj = read_object(30085, VIRTUAL);
  } else if (roll <= 30) {
    type = ITEM_WEAR_WRIST;
    obj = read_object(30087, VIRTUAL);
  } else if (roll <= 45) {
    type = ITEM_WEAR_NECK;
    obj = read_object(30086, VIRTUAL);
  } else if (roll <= 55) {
    type = ITEM_WEAR_FEET;
    obj = read_object(30091, VIRTUAL);
  } else if (roll <= 65) {
    type = ITEM_WEAR_HEAD;
    obj = read_object(30092, VIRTUAL);
  } else if (roll <= 75) {
    type = ITEM_WEAR_HANDS;
    obj = read_object(30090, VIRTUAL);
  } else if (roll <= 85) {
    type = ITEM_WEAR_ABOUT;
    obj = read_object(30088, VIRTUAL);
  } else {
    type = ITEM_WEAR_WAIST;
    obj = read_object(30103, VIRTUAL);
  }

  // Decide whether the item is of type jewelry or not
  switch (type) {
    case ITEM_WEAR_FINGER:
    case ITEM_WEAR_WRIST:
    case ITEM_WEAR_NECK:
      jewelry = TRUE;
      break;
  }

  roll = dice(1, 100);
  roll2 = dice(1, 100);

  if (roll <= 5)
    affect = APPLY_STR;
  else if (roll <= 10)
    affect = APPLY_DEX;
  else if (roll <= 15)
    affect = APPLY_INT;
  else if (roll <= 20)
    affect = APPLY_WIS;
  else if (roll <= 25)
    affect = APPLY_CON;
  else if (roll <= 30)
    affect = APPLY_CHA;
  else if (roll <= 90) {
    affect = APPLY_MOVE;
    bonus *= 100;
  } else {
  }

  if (rare == 1) {
    raregrade = 3;
  } else if (rare <= 6) {
    raregrade = 2;
  } else if (rare <= 16) {
    raregrade = 1;
  }

  char rdesc[50];
  if (raregrade == 0)
    sprintf(rdesc, "@n");
  else if (raregrade == 1)
    sprintf(rdesc, "@G[Rare]@n ");
  else if (raregrade == 2)
    sprintf(rdesc, "@Y[Legendary]@n ");
  else if (raregrade == 3)
    sprintf(rdesc, "@M[Mythical]@n ");


  if (type == ITEM_WEAR_FINGER) {
    material = choose_metal_material();
    i = 0;
    while (*(ring_descs + i++)) {
    }
    size = i;
    misc_desc_roll_1 = MAX(0, dice(1, (int) size) - 2);
    i = 0;
    while (*(gemstones + i++)) {
    }
    size = i;
    misc_desc_roll_2 = MAX(0, dice(1, (int) size) - 2);
    sprintf(desc, "%s%s %s %s set with %s %s gemstone", rdesc, AN(material_names[material]), material_names[material],
            ring_descs[misc_desc_roll_1], AN(gemstones[misc_desc_roll_2]), gemstones[misc_desc_roll_2]);
    obj->name = strdup(desc);
    obj->short_description = strdup(desc);
    sprintf(desc, "%s%s %s %s set with %s %s gemstone lies here.", rdesc, AN(material_names[material]), material_names[material],
            ring_descs[misc_desc_roll_1], AN(gemstones[misc_desc_roll_2]), gemstones[misc_desc_roll_2]);
    obj->description = strdup(CAP(desc));
  } else if (type == ITEM_WEAR_WRIST) {
    material = choose_metal_material();
    i = 0;
    while (*(wrist_descs + i++)) {
    }
    size = i;
    misc_desc_roll_1 = MAX(0, dice(1, (int) size) - 2);
    i = 0;
    while (*(gemstones + i++)) {
    }
    size = i;
    misc_desc_roll_2 = MAX(0, dice(1, (int) size) - 2);
    sprintf(desc, "%s%s %s %s set with %s %s gemstone", rdesc, AN(material_names[material]), material_names[material],
            wrist_descs[misc_desc_roll_1], AN(gemstones[misc_desc_roll_2]), gemstones[misc_desc_roll_2]);
    obj->name = strdup(desc);
    obj->short_description = strdup(desc);
    sprintf(desc, "%s%s %s %s set with %s %s gemstone lies here.", rdesc, AN(material_names[material]), material_names[material],
            wrist_descs[misc_desc_roll_1], AN(gemstones[misc_desc_roll_2]), gemstones[misc_desc_roll_2]);
    obj->description = strdup(CAP(desc));
  } else if (type == ITEM_WEAR_NECK) {
    material = choose_metal_material();
    i = 0;
    while (*(neck_descs + i++)) {
    }
    size = i;
    misc_desc_roll_1 = MAX(0, dice(1, (int) size) - 2);
    i = 0;
    while (*(gemstones + i++)) {
    }
    size = i;
    misc_desc_roll_2 = MAX(0, dice(1, (int) size) - 2);
    sprintf(desc, "%s%s %s %s set with %s %s gemstone", rdesc, AN(material_names[material]), material_names[material],
            neck_descs[misc_desc_roll_1], AN(gemstones[misc_desc_roll_2]), gemstones[misc_desc_roll_2]);
    obj->name = strdup(desc);
    obj->short_description = strdup(desc);
    sprintf(desc, "%s%s %s %s set with %s %s gemstone lies here.", rdesc, AN(material_names[material]), material_names[material],
            neck_descs[misc_desc_roll_1], AN(gemstones[misc_desc_roll_2]), gemstones[misc_desc_roll_2]);
    obj->description = strdup(CAP(desc));
  } else if (type == ITEM_WEAR_FEET) {
    material = MATERIAL_LEATHER;
    i = 0;
    while (*(boot_descs + i++)) {
    }
    size = i;
    misc_desc_roll_1 = MAX(0, dice(1, (int) size) - 2);
    i = 0;
    while (*(colors + i++)) {
    }
    size = i;
    misc_desc_roll_2 = MAX(0, dice(1, (int) size) - 2);
    i = 0;
    while (*(armor_special_descs + i++)) {
    }
    size = i;
    misc_desc_roll_3 = MAX(0, dice(1, (int) size) - 2);
    sprintf(desc, "%sa pair of %s %s leather %s", rdesc, armor_special_descs[misc_desc_roll_3], colors[misc_desc_roll_2],
            boot_descs[misc_desc_roll_1]);
    obj->name = strdup(desc);
    obj->short_description = strdup(desc);
    sprintf(desc, "%sA pair of %s %s leather %s lie here.", rdesc, armor_special_descs[misc_desc_roll_3], colors[misc_desc_roll_2],
            boot_descs[misc_desc_roll_1]);
    obj->description = strdup(desc);
  } else if (type == ITEM_WEAR_HANDS) {
    material = MATERIAL_LEATHER;
    i = 0;
    while (*(hands_descs + i++)) {
    }
    size = i;
    misc_desc_roll_1 = MAX(0, dice(1, (int) size) - 2);
    i = 0;
    while (*(colors + i++)) {
    }
    size = i;
    misc_desc_roll_2 = MAX(0, dice(1, (int) size) - 2);
    i = 0;
    while (*(armor_special_descs + i++)) {
    }
    size = i;
    misc_desc_roll_3 = MAX(0, dice(1, (int) size) - 2);
    sprintf(desc, "%sa pair of %s %s leather %s", rdesc, armor_special_descs[misc_desc_roll_3], colors[misc_desc_roll_2],
            hands_descs[misc_desc_roll_1]);
    obj->name = strdup(desc);
    obj->short_description = strdup(desc);
    sprintf(desc, "%sA pair of %s %s leather %s lie here.", rdesc, armor_special_descs[misc_desc_roll_3], colors[misc_desc_roll_2],
            hands_descs[misc_desc_roll_1]);
    obj->description = strdup(desc);
  } else if (type == ITEM_WEAR_WAIST) {
    material = MATERIAL_LEATHER;
    i = 0;
    while (*(waist_descs + i++)) {
    }
    size = i;
    misc_desc_roll_1 = MAX(0, dice(1, (int) size) - 2);
    i = 0;
    while (*(colors + i++)) {
    }
    size = i;
    misc_desc_roll_2 = MAX(0, dice(1, (int) size) - 2);
    i = 0;
    while (*(armor_special_descs + i++)) {
    }
    size = i;
    misc_desc_roll_3 = MAX(0, dice(1, (int) size) - 2);
    sprintf(desc, "%s%s %s %s leather %s", rdesc, AN(armor_special_descs[misc_desc_roll_3]), armor_special_descs[misc_desc_roll_3], colors[misc_desc_roll_2],
            waist_descs[misc_desc_roll_1]);
    obj->name = strdup(desc);
    obj->short_description = strdup(desc);
    sprintf(desc, "%s%s %s %s leather %s lie here.", rdesc, AN(armor_special_descs[misc_desc_roll_3]), armor_special_descs[misc_desc_roll_3], colors[misc_desc_roll_2],
            waist_descs[misc_desc_roll_1]);
    obj->description = strdup(desc);
  } else if (type == ITEM_WEAR_ABOUT) {
    material = choose_cloth_material();
    i = 0;
    while (*(cloak_descs + i++)) {
    }
    size = i;
    misc_desc_roll_1 = MAX(0, dice(1, (int) size) - 2);
    i = 0;
    while (*(colors + i++)) {
    }
    size = i;
    misc_desc_roll_2 = MAX(0, dice(1, (int) size) - 2);
    i = 0;
    while (*(armor_special_descs + i++)) {
    }
    size = i;
    misc_desc_roll_3 = MAX(0, dice(1, (int) size) - 2);
    i = 0;
    while (*(armor_crests + i++)) {
    }
    size = i;
    misc_desc_roll_4 = MAX(0, dice(1, (int) size) - 2);
    sprintf(desc, "%s%s %s %s %s bearing the crest of %s %s", rdesc, AN(colors[misc_desc_roll_2]), colors[misc_desc_roll_2],
            material_names[material], cloak_descs[misc_desc_roll_1], AN(armor_crests[misc_desc_roll_4]),
            armor_crests[misc_desc_roll_4]);
    obj->name = strdup(desc);
    obj->short_description = strdup(desc);
    sprintf(desc, "%s%s %s %s %s bearing the crest of %s %s", rdesc, AN(colors[misc_desc_roll_2]), colors[misc_desc_roll_2],
            material_names[material], cloak_descs[misc_desc_roll_1], AN(armor_crests[misc_desc_roll_4]),
            armor_crests[misc_desc_roll_4]);
    obj->description = strdup(CAP(desc));
  } else if (type == ITEM_WEAR_ABOVE) {
    material = MATERIAL_GEMSTONE;
    i = 0;
    while (*(crystal_descs + i++)) {
    }
    size = i;
    misc_desc_roll_1 = MAX(0, dice(1, (int) size) - 2);
    i = 0;
    while (*(colors + i++)) {
    }
    size = i;
    misc_desc_roll_2 = MAX(0, dice(1, (int) size) - 2);
    sprintf(desc, "%sa %s %s ioun stone", rdesc, crystal_descs[misc_desc_roll_1], colors[misc_desc_roll_2]);
    obj->name = strdup(desc);
    obj->short_description = strdup(desc);
    sprintf(desc, "%sA %s %s ioun stone hovers just above the ground here.", rdesc, crystal_descs[misc_desc_roll_1], colors[misc_desc_roll_2]);
    obj->description = strdup(desc);
  }

  GET_OBJ_MATERIAL(obj) = material;
  SET_BIT_AR(GET_OBJ_EXTRA(obj), ITEM_UNIQUE_SAVE);

  val = affect;

  if (val >= APPLY_SPELL_LVL_0 && val <= APPLY_SPELL_LVL_9)
    bonus = MAX(1, bonus / 3);
  if (val == APPLY_AC_DEFLECTION || val == APPLY_AC_SHIELD || val == APPLY_AC_NATURAL || val == APPLY_AC_ARMOR || val == APPLY_AC_DODGE)
    bonus *= 10;
  if (val == APPLY_HIT)
    bonus *= 5;
  if (val == APPLY_KI)
    bonus *= 5;
  if (val == APPLY_MOVE)
    bonus *= 100;


  obj->affected[0].location = affect;
  obj->affected[0].modifier = bonus;
  obj->affected[0].specific = subval;

  GET_OBJ_LEVEL(obj) = MAX(1, set_object_level(obj));

  if (GET_OBJ_LEVEL(obj) >= CONFIG_LEVEL_CAP) {
    award_misc_magic_item(ch, grade, moblevel);
    return;
  }

  bonus = raregrade;
  if (val >= APPLY_SPELL_LVL_0 && val <= APPLY_SPELL_LVL_9)
    bonus = MAX(1, bonus / 3);
  if (val == APPLY_AC_DEFLECTION || val == APPLY_AC_SHIELD || val == APPLY_AC_NATURAL || val == APPLY_AC_ARMOR || val == APPLY_AC_DODGE)
    bonus *= 10;
  if (val == APPLY_HIT)
    bonus *= 5;
  if (val == APPLY_KI)
    bonus *= 5;
  if (val == APPLY_MOVE)
    bonus *= 100;
  obj->affected[0].modifier += bonus;


  GET_OBJ_COST(obj) = 250 + GET_OBJ_LEVEL(obj) * 50 * MAX(1, GET_OBJ_LEVEL(obj) - 1);
  GET_OBJ_COST(obj) = GET_OBJ_COST(obj) * (3 + (raregrade * 2)) / 3;
  GET_OBJ_RENT(obj) = GET_OBJ_COST(obj) / 25;

  if (grade > GRADE_MUNDANE)
    SET_BIT_AR(GET_OBJ_EXTRA(obj), ITEM_MAGIC);

  obj_to_char(obj, ch);

  if (!(IS_NPC(ch) && IS_MOB(ch) && GET_MOB_SPEC(ch) == shop_keeper)) {
    send_to_char(ch, "@YYou have found %s in a nearby lair!@n\r\n", obj->short_description);

    sprintf(desc, "@Y$n has found %s in a nearby lair!@n", obj->short_description);
    act(desc, FALSE, ch, 0, ch, TO_NOTVICT);
  }
}
 */

/* awards very rare magical items */
void award_special_magic_item(struct char_data *ch) {
  struct obj_data *obj = NULL;
  char buf[200];

  // Ring of Three Wishes  
  obj = read_object(30188, VIRTUAL);

  SET_BIT_AR(GET_OBJ_EXTRA(obj), ITEM_MAGIC);

  obj_to_char(obj, ch);

  if (!(IS_NPC(ch) && IS_MOB(ch) && GET_MOB_SPEC(ch) == shop_keeper)) {
    send_to_char(ch, "@YYou have found %s in a nearby lair!@n\r\n", obj->short_description);
    sprintf(buf, "@Y$n has found %s in a nearby lair!@n", obj->short_description);
    act(buf, FALSE, ch, 0, ch, TO_NOTVICT);
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

  award_magic_item(number, ch, GET_LEVEL(ch), grade);
}



