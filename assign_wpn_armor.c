 /*****************************************************************************
 ** assign_wpn_armor.c
  *
  * Assigning weapon and armor values for respective types                   **
 ** Initial code by Paladine (Stephen Squires), Ported by Ornir to Luminari  **
 *****************************************************************************/

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "assign_wpn_armor.h"
#include "craft.h"

/* global */
struct armor_table armor_list[NUM_SPEC_ARMOR_TYPES];
struct weapon_table weapon_list[NUM_WEAPON_TYPES];
const char *weapon_type[NUM_WEAPON_TYPES];

/* some utility functions necessary for our piecemail armor system, everything
 * is up for changes since this is highly experimental system */

/* Armor types */ /*
#define ARMOR_TYPE_NONE     0
#define ARMOR_TYPE_LIGHT    1
#define ARMOR_TYPE_MEDIUM   2
#define ARMOR_TYPE_HEAVY    3
#define ARMOR_TYPE_SHIELD   4
#define ARMOR_TYPE_TOWER_SHIELD   5
#define NUM_ARMOR_TYPES     6 */
/* we have to be strict here, some classes such as monk require armor_type
   check, we are going to return the lowest armortype-value that the given
   ch is wearing */
int compute_gear_armor_type(struct char_data *ch) {
  int armor_type = ARMOR_TYPE_NONE, armor_compare = ARMOR_TYPE_NONE, i;
  struct obj_data *obj = NULL;

  for (i = 0; i < NUM_WEARS; i++) {
    obj = GET_EQ(ch, i);
    if (obj && GET_OBJ_TYPE(obj) == ITEM_ARMOR) {
      /* ok we have an armor piece... */
      armor_compare = armor_list[GET_OBJ_VAL(obj, 1)].armorType;
      if (armor_compare < ARMOR_TYPE_SHIELD && armor_compare > armor_type) {
        armor_type = armor_compare;
      }
    }
  }

  return armor_type;
}

int compute_gear_shield_type(struct char_data *ch) {
  int shield_type = ARMOR_TYPE_NONE;
  struct obj_data *obj = GET_EQ(ch, WEAR_SHIELD);

  if (obj) {
    shield_type = armor_list[GET_OBJ_VAL(obj, 1)].armorType;
    if (shield_type != ARMOR_TYPE_SHIELD && shield_type != ARMOR_TYPE_TOWER_SHIELD) {
      shield_type = ARMOR_TYPE_NONE;
    }
  }

  return shield_type;
}

/* enhancement bonus + material bonus */
int compute_gear_enhancement_bonus(struct char_data *ch) {
  struct obj_data *obj = NULL;
  int enhancement_bonus = 0, material_bonus = 0, i, count = 0;

  for (i = 0; i < NUM_WEARS; i++) {
    /* exit slots */
    switch (i) {
      default:
        break;
    }
    obj = GET_EQ(ch, i);
    if (obj && GET_OBJ_TYPE(obj) == ITEM_ARMOR &&
        (i == WEAR_BODY || i == WEAR_HEAD || i == WEAR_LEGS || i == WEAR_ARMS) ) {
      count++;
      /* ok we have an armor piece... */
      enhancement_bonus += GET_OBJ_VAL(obj, 4);
      switch (GET_OBJ_MATERIAL(obj)) {
        case MATERIAL_ADAMANTINE:
        case MATERIAL_MITHRIL:
        case MATERIAL_DRAGONHIDE:
        case MATERIAL_DIAMOND:
        case MATERIAL_DARKWOOD:
          material_bonus++;
          break;
        default:
          break;
      }
    }
  }

  if (count) {/* divide by zero! :p */
    enhancement_bonus = enhancement_bonus / count;
    enhancement_bonus += MAX(0, material_bonus / count);
  }

  return enhancement_bonus;
}

/* should return a percentage */
int compute_gear_spell_failure(struct char_data *ch) {
  int spell_failure = 0, i, count = 0;
  struct obj_data *obj = NULL;

  for (i = 0; i < NUM_WEARS; i++) {
    obj = GET_EQ(ch, i);
    if (obj && GET_OBJ_TYPE(obj) == ITEM_ARMOR &&
        (i == WEAR_BODY || i == WEAR_HEAD || i == WEAR_LEGS || i == WEAR_ARMS)) {
      count++;
      /* ok we have an armor piece... */
      spell_failure += armor_list[GET_OBJ_VAL(obj, 1)].spellFail;
    }
  }

  if (count) {
    spell_failure = spell_failure / count;
  }

  if (spell_failure < 0)
    spell_failure = 0;
  if (spell_failure > 100)
    spell_failure = 100;

  return spell_failure;
}

/* for doing (usually) dexterity based tasks */
int compute_gear_armor_penalty(struct char_data *ch) {
  int armor_penalty = 0, i, count = 0;

  struct obj_data *obj = NULL;

  for (i = 0; i < NUM_WEARS; i++) {
    obj = GET_EQ(ch, i);
    if (obj && GET_OBJ_TYPE(obj) == ITEM_ARMOR &&
        (i == WEAR_BODY || i == WEAR_HEAD || i == WEAR_LEGS || i == WEAR_ARMS)) {
      count++;
      /* ok we have an armor piece... */
      armor_penalty += armor_list[GET_OBJ_VAL(obj, 1)].armorCheck;
    }
  }

  if (count) {
    armor_penalty = armor_penalty / count;
  }

  if (armor_penalty > 0)
    armor_penalty = 0;
  if (armor_penalty < -10)
    armor_penalty = -10;

  return armor_penalty;
}

/* maximum dexterity bonus */
int compute_gear_max_dex(struct char_data *ch) {
  int dexterity_cap = 0;

  return dexterity_cap;
}

/* end utility, start base set/load/init functions for weapons/armor */

void setweapon(int type, char *name, int numDice, int diceSize, int critRange, int critMult,
        int weaponFlags, int cost, int damageTypes, int weight, int range, int weaponFamily, int size,
        int material, int handle_type, int head_type) {
  weapon_type[type] = strdup(name);
  weapon_list[type].name = name;
  weapon_list[type].numDice = numDice;
  weapon_list[type].diceSize = diceSize;
  weapon_list[type].critRange = critRange;
  if (critMult == 2)
    weapon_list[type].critMult = CRIT_X2;
  else if (critMult == 3)
    weapon_list[type].critMult = CRIT_X3;
  else if (critMult == 4)
    weapon_list[type].critMult = CRIT_X4;
  else if (critMult == 5)
    weapon_list[type].critMult = CRIT_X5;
  else if (critMult == 6)
    weapon_list[type].critMult = CRIT_X6;
  weapon_list[type].weaponFlags = weaponFlags;
  weapon_list[type].cost = cost / 100;
  weapon_list[type].damageTypes = damageTypes;
  weapon_list[type].weight = weight;
  weapon_list[type].range = range;
  weapon_list[type].weaponFamily = weaponFamily;
  weapon_list[type].size = size;
  weapon_list[type].material = material;
  weapon_list[type].handle_type = handle_type;
  weapon_list[type].head_type = head_type;
}

void initialize_weapons(int type) {
  weapon_list[type].name = "unused weapon";
  weapon_list[type].numDice = 1;
  weapon_list[type].diceSize = 1;
  weapon_list[type].critRange = 0;
  weapon_list[type].critMult = 1;
  weapon_list[type].weaponFlags = 0;
  weapon_list[type].cost = 0;
  weapon_list[type].damageTypes = 0;
  weapon_list[type].weight = 0;
  weapon_list[type].range = 0;
  weapon_list[type].weaponFamily = 0;
  weapon_list[type].size = 0;
  weapon_list[type].material = 0;
  weapon_list[type].handle_type = 0;
  weapon_list[type].head_type = 0;
}

void load_weapons(void) {
  int i = 0;

  for (i = 0; i < NUM_WEAPON_TYPES; i++)
    initialize_weapons(i);

  /*	(weapon num, name, numDamDice, sizeDamDice, critRange, critMult, weapon flags, cost, damageType, weight, range, weaponFamily, Size, material, handle, head) */
  setweapon(WEAPON_TYPE_UNARMED, "unarmed", 1, 3, 0, 2, WEAPON_FLAG_SIMPLE, 200,
          DAMAGE_TYPE_BLUDGEONING, 1, 0, WEAPON_FAMILY_MONK, SIZE_SMALL, MATERIAL_ORGANIC,
          HANDLE_TYPE_GLOVE, HEAD_TYPE_FIST);
  setweapon(WEAPON_TYPE_DAGGER, "dagger", 1, 4, 1, 2, WEAPON_FLAG_THROWN |
          WEAPON_FLAG_SIMPLE, 200, DAMAGE_TYPE_PIERCING, 1, 10, WEAPON_FAMILY_SMALL_BLADE, SIZE_TINY,
          MATERIAL_STEEL, HANDLE_TYPE_HILT, HEAD_TYPE_BLADE);
  setweapon(WEAPON_TYPE_LIGHT_MACE, "light mace", 1, 6, 0, 2, WEAPON_FLAG_SIMPLE, 500,
          DAMAGE_TYPE_BLUDGEONING, 4, 0, WEAPON_FAMILY_CLUB, SIZE_SMALL, MATERIAL_STEEL,
          HANDLE_TYPE_HANDLE, HEAD_TYPE_HEAD);
  setweapon(WEAPON_TYPE_SICKLE, "sickle", 1, 6, 0, 2, WEAPON_FLAG_SIMPLE, 600,
          DAMAGE_TYPE_SLASHING, 2, 0, WEAPON_FAMILY_SMALL_BLADE, SIZE_SMALL, MATERIAL_STEEL,
          HANDLE_TYPE_HANDLE, HEAD_TYPE_BLADE);
  setweapon(WEAPON_TYPE_CLUB, "club", 1, 6, 0, 2, WEAPON_FLAG_SIMPLE, 10,
          DAMAGE_TYPE_BLUDGEONING, 3, 0, WEAPON_FAMILY_CLUB, SIZE_SMALL, MATERIAL_WOOD,
          HANDLE_TYPE_HANDLE, HEAD_TYPE_HEAD);
  setweapon(WEAPON_TYPE_HEAVY_MACE, "heavy mace", 1, 8, 0, 2, WEAPON_FLAG_SIMPLE, 1200,
          DAMAGE_TYPE_BLUDGEONING, 8, 0, WEAPON_FAMILY_CLUB, SIZE_MEDIUM, MATERIAL_STEEL,
          HANDLE_TYPE_HANDLE, HEAD_TYPE_HEAD);
  setweapon(WEAPON_TYPE_MORNINGSTAR, "morningstar", 1, 8, 0, 2, WEAPON_FLAG_SIMPLE, 800,
          DAMAGE_TYPE_BLUDGEONING | DAMAGE_TYPE_PIERCING, 6, 0, WEAPON_FAMILY_FLAIL, SIZE_MEDIUM,
          MATERIAL_STEEL, HANDLE_TYPE_HANDLE, HEAD_TYPE_HEAD);
  setweapon(WEAPON_TYPE_SHORTSPEAR, "shortspear", 1, 6, 0, 2, WEAPON_FLAG_SIMPLE |
          WEAPON_FLAG_THROWN, 100, DAMAGE_TYPE_PIERCING, 3, 20, WEAPON_FAMILY_SPEAR, SIZE_MEDIUM,
          MATERIAL_WOOD, HANDLE_TYPE_SHAFT, HEAD_TYPE_POINT);
  setweapon(WEAPON_TYPE_LONGSPEAR, "longspear", 1, 8, 0, 3, WEAPON_FLAG_SIMPLE |
          WEAPON_FLAG_REACH, 500, DAMAGE_TYPE_PIERCING, 9, 0, WEAPON_FAMILY_SPEAR, SIZE_LARGE,
          MATERIAL_WOOD, HANDLE_TYPE_SHAFT, HEAD_TYPE_POINT);
  setweapon(WEAPON_TYPE_QUARTERSTAFF, "quarterstaff", 1, 6, 0, 2, WEAPON_FLAG_SIMPLE,
          10, DAMAGE_TYPE_BLUDGEONING, 4, 0, WEAPON_FAMILY_MONK, SIZE_LARGE,
          MATERIAL_WOOD, HANDLE_TYPE_SHAFT, HEAD_TYPE_HEAD);
  setweapon(WEAPON_TYPE_SPEAR, "spear", 1, 8, 0, 3, WEAPON_FLAG_SIMPLE |
          WEAPON_FLAG_THROWN | WEAPON_FLAG_REACH, 200, DAMAGE_TYPE_PIERCING, 6, 20, WEAPON_FAMILY_SPEAR, SIZE_LARGE,
          MATERIAL_WOOD, HANDLE_TYPE_SHAFT, HEAD_TYPE_POINT);
  setweapon(WEAPON_TYPE_HEAVY_CROSSBOW, "heavy crossbow", 1, 10, 1, 2, WEAPON_FLAG_SIMPLE
          | WEAPON_FLAG_SLOW_RELOAD | WEAPON_FLAG_RANGED, 5000, DAMAGE_TYPE_PIERCING, 8, 120,
          WEAPON_FAMILY_CROSSBOW, SIZE_LARGE, MATERIAL_WOOD, HANDLE_TYPE_HANDLE, HEAD_TYPE_BOW);
  setweapon(WEAPON_TYPE_LIGHT_CROSSBOW, "light crossbow", 1, 8, 1, 2, WEAPON_FLAG_SIMPLE
          | WEAPON_FLAG_SLOW_RELOAD | WEAPON_FLAG_RANGED, 3500, DAMAGE_TYPE_PIERCING, 4, 80,
          WEAPON_FAMILY_CROSSBOW, SIZE_MEDIUM, MATERIAL_WOOD, HANDLE_TYPE_HANDLE, HEAD_TYPE_BOW);
  /*	(weapon num, name, numDamDice, sizeDamDice, critRange, critMult, weapon flags, cost, damageType, weight, range, weaponFamily, Size, material, handle, head) */
  setweapon(WEAPON_TYPE_DART, "dart", 1, 4, 0, 2, WEAPON_FLAG_SIMPLE | WEAPON_FLAG_THROWN
          | WEAPON_FLAG_RANGED, 50, DAMAGE_TYPE_PIERCING, 1, 20, WEAPON_FAMILY_THROWN, SIZE_TINY,
          MATERIAL_WOOD, HANDLE_TYPE_SHAFT, HEAD_TYPE_POINT);
  setweapon(WEAPON_TYPE_JAVELIN, "javelin", 1, 6, 0, 2, WEAPON_FLAG_SIMPLE |
          WEAPON_FLAG_THROWN | WEAPON_FLAG_RANGED, 100, DAMAGE_TYPE_PIERCING, 2, 30,
          WEAPON_FAMILY_SPEAR, SIZE_MEDIUM, MATERIAL_WOOD, HANDLE_TYPE_SHAFT, HEAD_TYPE_POINT);
  setweapon(WEAPON_TYPE_SLING, "sling", 1, 4, 0, 2, WEAPON_FLAG_SIMPLE |
          WEAPON_FLAG_RANGED, 10, DAMAGE_TYPE_BLUDGEONING, 1, 50, WEAPON_FAMILY_THROWN, SIZE_SMALL,
          MATERIAL_LEATHER, HANDLE_TYPE_STRAP, HEAD_TYPE_POUCH);
  setweapon(WEAPON_TYPE_THROWING_AXE, "throwing axe", 1, 6, 0, 2, WEAPON_FLAG_MARTIAL |
          WEAPON_FLAG_THROWN, 800, DAMAGE_TYPE_SLASHING, 2, 10, WEAPON_FAMILY_AXE, SIZE_SMALL,
          MATERIAL_STEEL, HANDLE_TYPE_HANDLE, HEAD_TYPE_BLADE);
  setweapon(WEAPON_TYPE_LIGHT_HAMMER, "light hammer", 1, 4, 0, 2, WEAPON_FLAG_MARTIAL |
          WEAPON_FLAG_THROWN, 100, DAMAGE_TYPE_BLUDGEONING, 2, 20, WEAPON_FAMILY_HAMMER, SIZE_SMALL,
          MATERIAL_STEEL, HANDLE_TYPE_HANDLE, HEAD_TYPE_HEAD);
  setweapon(WEAPON_TYPE_HAND_AXE, "hand axe", 1, 6, 0, 3, WEAPON_FLAG_MARTIAL, 600,
          DAMAGE_TYPE_SLASHING, 3, 0, WEAPON_FAMILY_AXE, SIZE_SMALL, MATERIAL_STEEL, HANDLE_TYPE_HANDLE,
          HEAD_TYPE_BLADE);
  setweapon(WEAPON_TYPE_KUKRI, "kukri", 1, 4, 2, 2, WEAPON_FLAG_MARTIAL, 800,
          DAMAGE_TYPE_SLASHING, 2, 0, WEAPON_FAMILY_SMALL_BLADE, SIZE_SMALL, MATERIAL_STEEL,
          HANDLE_TYPE_HILT, HEAD_TYPE_BLADE);
  setweapon(WEAPON_TYPE_LIGHT_PICK, "light pick", 1, 4, 0, 4, WEAPON_FLAG_MARTIAL, 400,
          DAMAGE_TYPE_PIERCING, 3, 0, WEAPON_FAMILY_PICK, SIZE_SMALL, MATERIAL_STEEL,
          HANDLE_TYPE_HANDLE, HEAD_TYPE_HEAD);
  setweapon(WEAPON_TYPE_SAP, "sap", 1, 6, 0, 2, WEAPON_FLAG_MARTIAL |
          WEAPON_FLAG_NONLETHAL, 100, DAMAGE_TYPE_BLUDGEONING | DAMAGE_TYPE_NONLETHAL, 2, 0,
          WEAPON_FAMILY_CLUB, SIZE_SMALL, MATERIAL_LEATHER, HANDLE_TYPE_HANDLE, HEAD_TYPE_HEAD);
  setweapon(WEAPON_TYPE_SHORT_SWORD, "short sword", 1, 6, 1, 2, WEAPON_FLAG_MARTIAL,
          1000, DAMAGE_TYPE_PIERCING, 2, 0, WEAPON_FAMILY_SMALL_BLADE, SIZE_SMALL, MATERIAL_STEEL,
          HANDLE_TYPE_HILT, HEAD_TYPE_BLADE);
  setweapon(WEAPON_TYPE_BATTLE_AXE, "battle axe", 1, 8, 0, 3, WEAPON_FLAG_MARTIAL, 1000,
          DAMAGE_TYPE_SLASHING, 6, 0, WEAPON_FAMILY_AXE, SIZE_MEDIUM, MATERIAL_STEEL,
          HANDLE_TYPE_HANDLE, HEAD_TYPE_BLADE);
  /*	(weapon num, name, numDamDice, sizeDamDice, critRange, critMult, weapon flags, cost, damageType, weight, range, weaponFamily, Size, material, handle, head) */
  setweapon(WEAPON_TYPE_FLAIL, "flail", 1, 8, 0, 2, WEAPON_FLAG_MARTIAL, 800,
          DAMAGE_TYPE_BLUDGEONING, 5, 0, WEAPON_FAMILY_FLAIL, SIZE_MEDIUM, MATERIAL_STEEL,
          HANDLE_TYPE_HANDLE, HEAD_TYPE_HEAD);
  setweapon(WEAPON_TYPE_LONG_SWORD, "long sword", 1, 8, 1, 2, WEAPON_FLAG_MARTIAL, 1500,
          DAMAGE_TYPE_SLASHING, 4, 0, WEAPON_FAMILY_MEDIUM_BLADE, SIZE_MEDIUM, MATERIAL_STEEL,
          HANDLE_TYPE_HILT, HEAD_TYPE_BLADE);
  setweapon(WEAPON_TYPE_HEAVY_PICK, "heavy pick", 1, 6, 0, 4, WEAPON_FLAG_MARTIAL, 800,
          DAMAGE_TYPE_PIERCING, 6, 0, WEAPON_FAMILY_PICK, SIZE_MEDIUM, MATERIAL_STEEL,
          HANDLE_TYPE_HANDLE, HEAD_TYPE_HEAD);
  setweapon(WEAPON_TYPE_RAPIER, "rapier", 1, 6, 2, 2, WEAPON_FLAG_MARTIAL |
          WEAPON_FLAG_BALANCED, 2000, DAMAGE_TYPE_PIERCING, 2, 0, WEAPON_FAMILY_SMALL_BLADE,
          SIZE_SMALL, MATERIAL_STEEL, HANDLE_TYPE_HILT, HEAD_TYPE_BLADE);
  setweapon(WEAPON_TYPE_SCIMITAR, "scimitar", 1, 6, 2, 2, WEAPON_FLAG_MARTIAL, 1500,
          DAMAGE_TYPE_SLASHING, 4, 0, WEAPON_FAMILY_MEDIUM_BLADE, SIZE_MEDIUM, MATERIAL_STEEL,
          HANDLE_TYPE_HILT, HEAD_TYPE_BLADE);
  setweapon(WEAPON_TYPE_TRIDENT, "trident", 1, 8, 0, 2, WEAPON_FLAG_MARTIAL |
          WEAPON_FLAG_THROWN, 1500, DAMAGE_TYPE_PIERCING, 4, 0, WEAPON_FAMILY_SPEAR, SIZE_MEDIUM,
          MATERIAL_STEEL, HANDLE_TYPE_SHAFT, HEAD_TYPE_POINT);
  setweapon(WEAPON_TYPE_WARHAMMER, "warhammer", 1, 8, 0, 3, WEAPON_FLAG_MARTIAL, 1200,
          DAMAGE_TYPE_BLUDGEONING, 5, 0, WEAPON_FAMILY_HAMMER, SIZE_MEDIUM, MATERIAL_STEEL,
          HANDLE_TYPE_HANDLE, HEAD_TYPE_HEAD);
  setweapon(WEAPON_TYPE_FALCHION, "falchion", 2, 4, 2, 2, WEAPON_FLAG_MARTIAL, 7500,
          DAMAGE_TYPE_SLASHING, 8, 0, WEAPON_FAMILY_LARGE_BLADE, SIZE_LARGE, MATERIAL_STEEL,
          HANDLE_TYPE_HILT, HEAD_TYPE_BLADE);
  setweapon(WEAPON_TYPE_GLAIVE, "glaive", 1, 10, 0, 3, WEAPON_FLAG_MARTIAL |
          WEAPON_FLAG_REACH, 800, DAMAGE_TYPE_SLASHING, 10, 0, WEAPON_FAMILY_POLEARM, SIZE_LARGE,
          MATERIAL_STEEL, HANDLE_TYPE_SHAFT, HEAD_TYPE_BLADE);
  setweapon(WEAPON_TYPE_GREAT_AXE, "great axe", 1, 12, 0, 3, WEAPON_FLAG_MARTIAL, 2000,
          DAMAGE_TYPE_SLASHING, 12, 0, WEAPON_FAMILY_AXE, SIZE_LARGE, MATERIAL_STEEL,
          HANDLE_TYPE_HANDLE, HEAD_TYPE_BLADE);
  setweapon(WEAPON_TYPE_GREAT_CLUB, "great club", 1, 10, 0, 2, WEAPON_FLAG_MARTIAL, 500,
          DAMAGE_TYPE_BLUDGEONING, 8, 0, WEAPON_FAMILY_CLUB, SIZE_LARGE, MATERIAL_WOOD,
          HANDLE_TYPE_SHAFT, HEAD_TYPE_HEAD);
  /*	(weapon num, name, numDamDice, sizeDamDice, critRange, critMult, weapon flags, cost, damageType, weight, range, weaponFamily, Size, material, handle, head) */
  setweapon(WEAPON_TYPE_HEAVY_FLAIL, "heavy flail", 1, 10, 1, 2, WEAPON_FLAG_MARTIAL,
          1500, DAMAGE_TYPE_BLUDGEONING, 10, 0, WEAPON_FAMILY_FLAIL, SIZE_LARGE, MATERIAL_STEEL,
          HANDLE_TYPE_HANDLE, HEAD_TYPE_HEAD);
  setweapon(WEAPON_TYPE_GREAT_SWORD, "great sword", 2, 6, 1, 2, WEAPON_FLAG_MARTIAL,
          5000, DAMAGE_TYPE_SLASHING, 8, 0, WEAPON_FAMILY_LARGE_BLADE, SIZE_LARGE, MATERIAL_STEEL,
          HANDLE_TYPE_HILT, HEAD_TYPE_BLADE);
  setweapon(WEAPON_TYPE_GUISARME, "guisarme", 2, 4, 0, 3, WEAPON_FLAG_MARTIAL |
          WEAPON_FLAG_REACH, 900, DAMAGE_TYPE_SLASHING, 12, 0, WEAPON_FAMILY_POLEARM, SIZE_LARGE,
          MATERIAL_STEEL, HANDLE_TYPE_SHAFT, HEAD_TYPE_BLADE);
  setweapon(WEAPON_TYPE_HALBERD, "halberd", 1, 10, 0, 3, WEAPON_FLAG_MARTIAL |
          WEAPON_FLAG_REACH, 1000, DAMAGE_TYPE_SLASHING | DAMAGE_TYPE_PIERCING, 12, 0,
          WEAPON_FAMILY_POLEARM, SIZE_LARGE, MATERIAL_STEEL, HANDLE_TYPE_SHAFT, HEAD_TYPE_BLADE);
  setweapon(WEAPON_TYPE_LANCE, "lance", 1, 8, 0, 3, WEAPON_FLAG_MARTIAL |
          WEAPON_FLAG_REACH | WEAPON_FLAG_CHARGE, 1000, DAMAGE_TYPE_PIERCING, 10, 0,
          WEAPON_FAMILY_POLEARM, SIZE_LARGE, MATERIAL_STEEL, HANDLE_TYPE_SHAFT, HEAD_TYPE_POINT);
  setweapon(WEAPON_TYPE_RANSEUR, "ranseur", 2, 4, 0, 3, WEAPON_FLAG_MARTIAL |
          WEAPON_FLAG_REACH, 1000, DAMAGE_TYPE_PIERCING, 10, 0, WEAPON_FAMILY_POLEARM, SIZE_LARGE,
          MATERIAL_STEEL, HANDLE_TYPE_SHAFT, HEAD_TYPE_POINT);
  setweapon(WEAPON_TYPE_SCYTHE, "scythe", 2, 4, 0, 4, WEAPON_FLAG_MARTIAL, 1800,
          DAMAGE_TYPE_SLASHING | DAMAGE_TYPE_PIERCING, 10, 0, WEAPON_FAMILY_POLEARM, SIZE_LARGE,
          MATERIAL_STEEL, HANDLE_TYPE_SHAFT, HEAD_TYPE_BLADE);
  setweapon(WEAPON_TYPE_LONG_BOW, "long bow", 1, 8, 0, 3, WEAPON_FLAG_MARTIAL |
          WEAPON_FLAG_RANGED, 7500, DAMAGE_TYPE_PIERCING, 3, 100, WEAPON_FAMILY_BOW, SIZE_MEDIUM,
          MATERIAL_WOOD, HANDLE_TYPE_STRING, HEAD_TYPE_BOW);
  setweapon(WEAPON_TYPE_COMPOSITE_LONGBOW, "composite long bow", 1, 8, 0, 3,
          WEAPON_FLAG_MARTIAL | WEAPON_FLAG_RANGED, 10000, DAMAGE_TYPE_PIERCING, 3, 110,
          WEAPON_FAMILY_BOW, SIZE_MEDIUM, MATERIAL_WOOD, HANDLE_TYPE_STRING, HEAD_TYPE_BOW);
  setweapon(WEAPON_TYPE_SHORT_BOW, "short bow", 1, 6, 0, 3, WEAPON_FLAG_MARTIAL |
          WEAPON_FLAG_RANGED, 3000, DAMAGE_TYPE_PIERCING, 2, 60, WEAPON_FAMILY_BOW, SIZE_SMALL,
          MATERIAL_WOOD, HANDLE_TYPE_STRING, HEAD_TYPE_BOW);
  setweapon(WEAPON_TYPE_COMPOSITE_SHORTBOW, "composite short bow", 1, 6, 0, 3,
          WEAPON_FLAG_MARTIAL | WEAPON_FLAG_RANGED, 7500, DAMAGE_TYPE_PIERCING, 2, 70,
          WEAPON_FAMILY_BOW, SIZE_SMALL, MATERIAL_WOOD, HANDLE_TYPE_STRING, HEAD_TYPE_BOW);
  /*	(weapon num, name, numDamDice, sizeDamDice, critRange, critMult, weapon flags, cost, damageType, weight, range, weaponFamily, Size, material, handle, head) */
  setweapon(WEAPON_TYPE_KAMA, "kama", 1, 6, 0, 2, WEAPON_FLAG_EXOTIC, 200,
          DAMAGE_TYPE_SLASHING, 2, 0, WEAPON_FAMILY_MONK, SIZE_SMALL, MATERIAL_STEEL,
          HANDLE_TYPE_HANDLE, HEAD_TYPE_BLADE);
  setweapon(WEAPON_TYPE_NUNCHAKU, "nunchaku", 1, 6, 1, 2, WEAPON_FLAG_EXOTIC, 200,
          DAMAGE_TYPE_BLUDGEONING, 2, 0, WEAPON_FAMILY_MONK, SIZE_SMALL, MATERIAL_WOOD,
          HANDLE_TYPE_HANDLE, HEAD_TYPE_HEAD);
  setweapon(WEAPON_TYPE_SAI, "sai", 1, 4, 1, 2, WEAPON_FLAG_EXOTIC | WEAPON_FLAG_THROWN,
          100, DAMAGE_TYPE_BLUDGEONING, 1, 10, WEAPON_FAMILY_MONK, SIZE_SMALL, MATERIAL_STEEL,
          HANDLE_TYPE_HANDLE, HEAD_TYPE_POINT);
  setweapon(WEAPON_TYPE_SIANGHAM, "siangham", 1, 6, 1, 2, WEAPON_FLAG_EXOTIC, 300,
          DAMAGE_TYPE_PIERCING, 1, 0, WEAPON_FAMILY_MONK, SIZE_SMALL, MATERIAL_STEEL,
          HANDLE_TYPE_HANDLE, HEAD_TYPE_POINT);
  setweapon(WEAPON_TYPE_BASTARD_SWORD, "bastard sword", 1, 10, 1, 2, WEAPON_FLAG_EXOTIC,
          3500, DAMAGE_TYPE_SLASHING, 6, 0, WEAPON_FAMILY_MEDIUM_BLADE, SIZE_MEDIUM, MATERIAL_STEEL,
          HANDLE_TYPE_HILT, HEAD_TYPE_BLADE);
  setweapon(WEAPON_TYPE_DWARVEN_WAR_AXE, "dwarven war axe", 1, 10, 0, 3,
          WEAPON_FLAG_EXOTIC, 3000, DAMAGE_TYPE_SLASHING, 8, 0, WEAPON_FAMILY_AXE, SIZE_MEDIUM,
          MATERIAL_STEEL, HANDLE_TYPE_HANDLE, HEAD_TYPE_BLADE);
  setweapon(WEAPON_TYPE_WHIP, "whip", 1, 3, 0, 2, WEAPON_FLAG_EXOTIC | WEAPON_FLAG_REACH
          | WEAPON_FLAG_DISARM | WEAPON_FLAG_TRIP, 100, DAMAGE_TYPE_SLASHING, 2, 0, WEAPON_FAMILY_WHIP,
          SIZE_MEDIUM, MATERIAL_LEATHER, HANDLE_TYPE_HANDLE, HEAD_TYPE_CORD);
  setweapon(WEAPON_TYPE_SPIKED_CHAIN, "spiked chain", 2, 4, 0, 2, WEAPON_FLAG_EXOTIC |
          WEAPON_FLAG_REACH | WEAPON_FLAG_DISARM | WEAPON_FLAG_TRIP, 2500, DAMAGE_TYPE_PIERCING, 10, 0,
          WEAPON_FAMILY_WHIP, SIZE_LARGE, MATERIAL_STEEL, HANDLE_TYPE_GRIP, HEAD_TYPE_CHAIN);
  setweapon(WEAPON_TYPE_DOUBLE_AXE, "double-headed axe", 1, 8, 0, 3, WEAPON_FLAG_EXOTIC |
          WEAPON_FLAG_DOUBLE, 6500, DAMAGE_TYPE_SLASHING, 15, 0, WEAPON_FAMILY_DOUBLE, SIZE_LARGE,
          MATERIAL_STEEL, HANDLE_TYPE_SHAFT, HEAD_TYPE_BLADE);
  setweapon(WEAPON_TYPE_DIRE_FLAIL, "dire flail", 1, 8, 0, 2, WEAPON_FLAG_EXOTIC |
          WEAPON_FLAG_DOUBLE, 9000, DAMAGE_TYPE_BLUDGEONING, 10, 0, WEAPON_FAMILY_DOUBLE, SIZE_LARGE,
          MATERIAL_STEEL, HANDLE_TYPE_SHAFT, HEAD_TYPE_HEAD);
  setweapon(WEAPON_TYPE_HOOKED_HAMMER, "hooked hammer", 1, 6, 0, 4, WEAPON_FLAG_EXOTIC |
          WEAPON_FLAG_DOUBLE, 2000, DAMAGE_TYPE_PIERCING | DAMAGE_TYPE_BLUDGEONING, 6, 0,
          WEAPON_FAMILY_DOUBLE, SIZE_LARGE, MATERIAL_STEEL, HANDLE_TYPE_SHAFT, HEAD_TYPE_HEAD);
  /*	(weapon num, name, numDamDice, sizeDamDice, critRange, critMult, weapon flags, cost, damageType, weight, range, weaponFamily, Size, material, handle, head) */
  setweapon(WEAPON_TYPE_2_BLADED_SWORD, "two-bladed sword", 1, 8, 1, 2,
          WEAPON_FLAG_EXOTIC | WEAPON_FLAG_DOUBLE, 10000, DAMAGE_TYPE_SLASHING, 10, 0,
          WEAPON_FAMILY_DOUBLE, SIZE_LARGE, MATERIAL_STEEL, HANDLE_TYPE_SHAFT, HEAD_TYPE_BLADE);
  setweapon(WEAPON_TYPE_DWARVEN_URGOSH, "dwarven urgosh", 1, 7, 0, 3, WEAPON_FLAG_EXOTIC
          | WEAPON_FLAG_DOUBLE, 5000, DAMAGE_TYPE_PIERCING | DAMAGE_TYPE_SLASHING, 12, 0,
          WEAPON_FAMILY_DOUBLE, SIZE_LARGE, MATERIAL_STEEL, HANDLE_TYPE_SHAFT, HEAD_TYPE_BLADE);
  setweapon(WEAPON_TYPE_HAND_CROSSBOW, "hand crossbow", 1, 4, 1, 2, WEAPON_FLAG_EXOTIC |
          WEAPON_FLAG_RANGED, 10000, DAMAGE_TYPE_PIERCING, 2, 30, WEAPON_FAMILY_CROSSBOW, SIZE_SMALL,
          MATERIAL_WOOD, HANDLE_TYPE_HANDLE, HEAD_TYPE_BOW);
  setweapon(WEAPON_TYPE_HEAVY_REP_XBOW, "heavy repeating crossbow", 1, 10, 1, 2,
          WEAPON_FLAG_EXOTIC | WEAPON_FLAG_RANGED | WEAPON_FLAG_REPEATING, 40000, DAMAGE_TYPE_PIERCING, 12, 120,
          WEAPON_FAMILY_CROSSBOW, SIZE_LARGE, MATERIAL_WOOD, HANDLE_TYPE_HANDLE, HEAD_TYPE_BOW);
  setweapon(WEAPON_TYPE_LIGHT_REP_XBOW, "light repeating crossbow", 1, 8, 1, 2,
          WEAPON_FLAG_EXOTIC | WEAPON_FLAG_RANGED, 25000, DAMAGE_TYPE_PIERCING, 6, 80,
          WEAPON_FAMILY_CROSSBOW, SIZE_MEDIUM, MATERIAL_WOOD, HANDLE_TYPE_HANDLE, HEAD_TYPE_BOW);
  setweapon(WEAPON_TYPE_BOLA, "bola", 1, 4, 0, 2, WEAPON_FLAG_EXOTIC | WEAPON_FLAG_THROWN
          | WEAPON_FLAG_TRIP, 500, DAMAGE_TYPE_BLUDGEONING, 2, 10, WEAPON_FAMILY_THROWN, SIZE_MEDIUM,
          MATERIAL_LEATHER, HANDLE_TYPE_GRIP, HEAD_TYPE_CORD);
  setweapon(WEAPON_TYPE_NET, "net", 1, 1, 0, 1, WEAPON_FLAG_EXOTIC | WEAPON_FLAG_THROWN |
          WEAPON_FLAG_ENTANGLE, 2000, DAMAGE_TYPE_BLUDGEONING, 6, 10, WEAPON_FAMILY_THROWN, SIZE_LARGE,
          MATERIAL_LEATHER, HANDLE_TYPE_GRIP, HEAD_TYPE_MESH);
  setweapon(WEAPON_TYPE_SHURIKEN, "shuriken", 1, 2, 0, 2, WEAPON_FLAG_EXOTIC |
          WEAPON_FLAG_THROWN, 20, DAMAGE_TYPE_PIERCING, 1, 10, WEAPON_FAMILY_MONK, SIZE_SMALL,
          MATERIAL_STEEL, HANDLE_TYPE_GRIP, HEAD_TYPE_BLADE);
}

void setarmor(int type, char *name, int armorType, int cost, int armorBonus,
              int dexBonus, int armorCheck, int spellFail, int thirtyFoot,
              int twentyFoot, int weight, int material, int wear) {
  armor_list[type].name = name;
  armor_list[type].armorType = armorType;
  armor_list[type].cost = cost;
  armor_list[type].armorBonus = armorBonus;
  armor_list[type].dexBonus = dexBonus;
  armor_list[type].armorCheck = armorCheck;
  armor_list[type].spellFail = spellFail;
  armor_list[type].thirtyFoot = thirtyFoot;
  armor_list[type].twentyFoot = twentyFoot;
  armor_list[type].weight = weight;
  armor_list[type].material = material;
  armor_list[type].wear = wear;
}

void initialize_armor(int type) {
  armor_list[type].name = "unused armor";
  armor_list[type].armorType = 0;
  armor_list[type].cost = 0;
  armor_list[type].armorBonus = 0;
  armor_list[type].dexBonus = 0;
  armor_list[type].armorCheck = 0;
  armor_list[type].spellFail = 0;
  armor_list[type].thirtyFoot = 0;
  armor_list[type].twentyFoot = 0;
  armor_list[type].weight = 0;
  armor_list[type].material = 0;
  armor_list[type].wear = ITEM_WEAR_TAKE;
}

void load_armor(void) {
  int i = 0;

  for (i = 0; i <= NUM_SPEC_ARMOR_TYPES; i++)
    initialize_armor(i);

  /* (armor, name, type,
   *    cost, AC, dexBonusCap, armorCheckPenalty, spellFailChance, (move)30ft, (move)20ft,
   *    weight, material, wear) */
  setarmor(SPEC_ARMOR_TYPE_CLOTHING, "body clothing", ARMOR_TYPE_NONE,
    10, 0, 999, 0, 0, 30, 20,
    1, MATERIAL_COTTON, ITEM_WEAR_BODY);
  setarmor(SPEC_ARMOR_TYPE_CLOTHING_HEAD, "clothing hood", ARMOR_TYPE_NONE,
    10, 0, 999, 0, 0, 30, 20,
    1, MATERIAL_COTTON, ITEM_WEAR_HEAD);
  setarmor(SPEC_ARMOR_TYPE_CLOTHING_ARMS, "cloth sleeves", ARMOR_TYPE_NONE,
    10, 0, 999, 0, 0, 30, 20,
    1, MATERIAL_COTTON, ITEM_WEAR_ARMS);
  setarmor(SPEC_ARMOR_TYPE_CLOTHING_LEGS, "cloth leggings", ARMOR_TYPE_NONE,
    10, 0, 999, 0, 0, 30, 20,
    1, MATERIAL_COTTON, ITEM_WEAR_LEGS);

  setarmor(SPEC_ARMOR_TYPE_PADDED, "padded body armor", ARMOR_TYPE_LIGHT,
    50, 7, 8, 0, 5, 30, 20,
    7, MATERIAL_COTTON, ITEM_WEAR_BODY);
  setarmor(SPEC_ARMOR_TYPE_PADDED_HEAD, "padded armor helm", ARMOR_TYPE_LIGHT,
    50, 1, 8, 0, 5, 30, 20,
    1, MATERIAL_COTTON, ITEM_WEAR_HEAD);
  setarmor(SPEC_ARMOR_TYPE_PADDED_ARMS, "padded armor sleeves", ARMOR_TYPE_LIGHT,
    50, 1, 8, 0, 5, 30, 20,
    1, MATERIAL_COTTON, ITEM_WEAR_ARMS);
  setarmor(SPEC_ARMOR_TYPE_PADDED_LEGS, "padded armor leggings", ARMOR_TYPE_LIGHT,
    50, 1, 8, 0, 5, 30, 20,
    1, MATERIAL_COTTON, ITEM_WEAR_LEGS);

  setarmor(SPEC_ARMOR_TYPE_LEATHER, "leather armor", ARMOR_TYPE_LIGHT,
    100, 11, 6, 0, 10, 30, 20,
    9, MATERIAL_LEATHER, ITEM_WEAR_BODY);
  setarmor(SPEC_ARMOR_TYPE_LEATHER_HEAD, "leather helm", ARMOR_TYPE_LIGHT,
    100, 3, 6, 0, 10, 30, 20,
    2, MATERIAL_LEATHER, ITEM_WEAR_HEAD);
  setarmor(SPEC_ARMOR_TYPE_LEATHER_ARMS, "leather sleeves", ARMOR_TYPE_LIGHT,
    100, 3, 6, 0, 10, 30, 20,
    2, MATERIAL_LEATHER, ITEM_WEAR_ARMS);
  setarmor(SPEC_ARMOR_TYPE_LEATHER_LEGS, "leather leggings", ARMOR_TYPE_LIGHT,
    100, 3, 6, 0, 10, 30, 20,
    2, MATERIAL_LEATHER, ITEM_WEAR_LEGS);

  /* (armor, name, type,
   *    cost, AC, dexBonusCap, armorCheckPenalty, spellFailChance, (move)30ft, (move)20ft,
   *    weight, material, wear) */
  setarmor(SPEC_ARMOR_TYPE_STUDDED_LEATHER, "studded leather armor", ARMOR_TYPE_LIGHT,
    250, 15, 5, -1, 15, 30, 20,
    11, MATERIAL_LEATHER, ITEM_WEAR_BODY);
  setarmor(SPEC_ARMOR_TYPE_STUDDED_LEATHER_HEAD, "studded leather helm", ARMOR_TYPE_LIGHT,
    250, 5, 5, -1, 15, 30, 20,
    3, MATERIAL_LEATHER, ITEM_WEAR_HEAD);
  setarmor(SPEC_ARMOR_TYPE_STUDDED_LEATHER_ARMS, "studded leather sleeves", ARMOR_TYPE_LIGHT,
    250, 5, 5, -1, 15, 30, 20,
    3, MATERIAL_LEATHER, ITEM_WEAR_ARMS);
  setarmor(SPEC_ARMOR_TYPE_STUDDED_LEATHER_LEGS, "studded leather leggings", ARMOR_TYPE_LIGHT,
    250, 5, 5, -1, 15, 30, 20,
    3, MATERIAL_LEATHER, ITEM_WEAR_LEGS);

  setarmor(SPEC_ARMOR_TYPE_LIGHT_CHAIN, "light chainmail armor", ARMOR_TYPE_LIGHT,
    1000, 19, 4, -2, 20, 30, 20,
    13, MATERIAL_STEEL, ITEM_WEAR_BODY);
  setarmor(SPEC_ARMOR_TYPE_LIGHT_CHAIN_HEAD, "light chainmail helm", ARMOR_TYPE_LIGHT,
    1000, 7, 4, -2, 20, 30, 20,
    4, MATERIAL_STEEL, ITEM_WEAR_HEAD);
  setarmor(SPEC_ARMOR_TYPE_LIGHT_CHAIN_ARMS, "light chainmail sleeves", ARMOR_TYPE_LIGHT,
    1000, 7, 4, -2, 20, 30, 20,
    4, MATERIAL_STEEL, ITEM_WEAR_ARMS);
  setarmor(SPEC_ARMOR_TYPE_LIGHT_CHAIN_LEGS, "light chainmail leggings", ARMOR_TYPE_LIGHT,
    1000, 7, 4, -2, 20, 30, 20,
    4, MATERIAL_STEEL, ITEM_WEAR_LEGS);

  setarmor(SPEC_ARMOR_TYPE_HIDE, "hide armor", ARMOR_TYPE_MEDIUM,
    150, 19, 4, -3, 20, 20, 15,
    13, MATERIAL_LEATHER, ITEM_WEAR_BODY);
  setarmor(SPEC_ARMOR_TYPE_HIDE_HEAD, "hide helm", ARMOR_TYPE_MEDIUM,
    150, 7, 4, -3, 20, 20, 15,
    4, MATERIAL_LEATHER, ITEM_WEAR_HEAD);
  setarmor(SPEC_ARMOR_TYPE_HIDE_ARMS, "hide sleeves", ARMOR_TYPE_MEDIUM,
    150, 7, 4, -3, 20, 20, 15,
    4, MATERIAL_LEATHER, ITEM_WEAR_ARMS);
  setarmor(SPEC_ARMOR_TYPE_HIDE_LEGS, "hide leggings", ARMOR_TYPE_MEDIUM,
    150, 7, 4, -3, 20, 20, 15,
    4, MATERIAL_LEATHER, ITEM_WEAR_LEGS);

  /* (armor, name, type,
   *    cost, AC, dexBonusCap, armorCheckPenalty, spellFailChance, (move)30ft, (move)20ft,
   *    weight, material, wear) */
  setarmor(SPEC_ARMOR_TYPE_SCALE, "scale armor", ARMOR_TYPE_MEDIUM,
    500, 23, 3, -4, 25, 20, 15,
    15, MATERIAL_STEEL, ITEM_WEAR_BODY);
  setarmor(SPEC_ARMOR_TYPE_SCALE_HEAD, "scale helm", ARMOR_TYPE_MEDIUM,
    500, 9, 3, -4, 25, 20, 15,
    5, MATERIAL_STEEL, ITEM_WEAR_HEAD);
  setarmor(SPEC_ARMOR_TYPE_SCALE_ARMS, "scale sleeves", ARMOR_TYPE_MEDIUM,
    500, 9, 3, -4, 25, 20, 15,
    5, MATERIAL_STEEL, ITEM_WEAR_ARMS);
  setarmor(SPEC_ARMOR_TYPE_SCALE_LEGS, "scale leggings", ARMOR_TYPE_MEDIUM,
    500, 9, 3, -4, 25, 20, 15,
    5, MATERIAL_STEEL, ITEM_WEAR_LEGS);

  setarmor(SPEC_ARMOR_TYPE_CHAINMAIL, "chainmail armor", ARMOR_TYPE_MEDIUM,
    1500, 27, 2, -5, 30, 20, 15,
    27, MATERIAL_STEEL, ITEM_WEAR_BODY);
  setarmor(SPEC_ARMOR_TYPE_CHAINMAIL_HEAD, "chainmail helm", ARMOR_TYPE_MEDIUM,
    1500, 11, 2, -5, 30, 20, 15,
    11, MATERIAL_STEEL, ITEM_WEAR_HEAD);
  setarmor(SPEC_ARMOR_TYPE_CHAINMAIL_ARMS, "chainmail sleeves", ARMOR_TYPE_MEDIUM,
    1500, 11, 2, -5, 30, 20, 15,
    11, MATERIAL_STEEL, ITEM_WEAR_ARMS);
  setarmor(SPEC_ARMOR_TYPE_CHAINMAIL_LEGS, "chainmail leggings", ARMOR_TYPE_MEDIUM,
    1500, 11, 2, -5, 30, 20, 15,
    11, MATERIAL_STEEL, ITEM_WEAR_LEGS);

  setarmor(SPEC_ARMOR_TYPE_PIECEMEAL, "piecemeal armor", ARMOR_TYPE_MEDIUM,
    2000, 25, 3, -4, 25, 20, 15,
    19, MATERIAL_STEEL, ITEM_WEAR_BODY);
  setarmor(SPEC_ARMOR_TYPE_PIECEMEAL_HEAD, "piecemeal helm", ARMOR_TYPE_MEDIUM,
    2000, 10, 3, -4, 25, 20, 15,
    7, MATERIAL_STEEL, ITEM_WEAR_HEAD);
  setarmor(SPEC_ARMOR_TYPE_PIECEMEAL_ARMS, "piecemeal sleeves", ARMOR_TYPE_MEDIUM,
    2000, 10, 3, -4, 25, 20, 15,
    7, MATERIAL_STEEL, ITEM_WEAR_ARMS);
  setarmor(SPEC_ARMOR_TYPE_PIECEMEAL_LEGS, "piecemeal leggings", ARMOR_TYPE_MEDIUM,
    2000, 10, 3, -4, 25, 20, 15,
    7, MATERIAL_STEEL, ITEM_WEAR_LEGS);

  /* (armor, name, type,
   *    cost, AC, dexBonusCap, armorCheckPenalty, spellFailChance, (move)30ft, (move)20ft,
   *    weight, material, wear) */
  setarmor(SPEC_ARMOR_TYPE_SPLINT, "splint mail armor", ARMOR_TYPE_HEAVY,
    2000, 31, 0, -7, 40, 20, 15,
    21, MATERIAL_STEEL, ITEM_WEAR_BODY);
  setarmor(SPEC_ARMOR_TYPE_SPLINT_HEAD, "splint mail helm", ARMOR_TYPE_HEAVY,
    2000, 13, 0, -7, 40, 20, 15,
    8, MATERIAL_STEEL, ITEM_WEAR_HEAD);
  setarmor(SPEC_ARMOR_TYPE_SPLINT_ARMS, "splint mail sleeves", ARMOR_TYPE_HEAVY,
    2000, 13, 0, -7, 40, 20, 15,
    8, MATERIAL_STEEL, ITEM_WEAR_ARMS);
  setarmor(SPEC_ARMOR_TYPE_SPLINT_LEGS, "splint mail leggings", ARMOR_TYPE_HEAVY,
    2000, 13, 0, -7, 40, 20, 15,
    8, MATERIAL_STEEL, ITEM_WEAR_LEGS);

  setarmor(SPEC_ARMOR_TYPE_BANDED, "banded mail armor", ARMOR_TYPE_HEAVY,
    2500, 31, 1, -6, 35, 20, 15,
    17, MATERIAL_STEEL, ITEM_WEAR_BODY);
  setarmor(SPEC_ARMOR_TYPE_BANDED_HEAD, "banded mail helm", ARMOR_TYPE_HEAVY,
    2500, 13, 1, -6, 35, 20, 15,
    6, MATERIAL_STEEL, ITEM_WEAR_HEAD);
  setarmor(SPEC_ARMOR_TYPE_BANDED_ARMS, "banded mail sleeves", ARMOR_TYPE_HEAVY,
    2500, 13, 1, -6, 35, 20, 15,
    6, MATERIAL_STEEL, ITEM_WEAR_ARMS);
  setarmor(SPEC_ARMOR_TYPE_BANDED_LEGS, "banded mail leggings", ARMOR_TYPE_HEAVY,
    2500, 13, 1, -6, 35, 20, 15,
    6, MATERIAL_STEEL, ITEM_WEAR_LEGS);

  setarmor(SPEC_ARMOR_TYPE_HALF_PLATE, "half plate armor", ARMOR_TYPE_HEAVY,
    6000, 35, 1, -6, 40, 20, 15,
    23, MATERIAL_STEEL, ITEM_WEAR_BODY);
  setarmor(SPEC_ARMOR_TYPE_HALF_PLATE_HEAD, "half plate helm", ARMOR_TYPE_HEAVY,
    6000, 15, 1, -6, 40, 20, 15,
    9, MATERIAL_STEEL, ITEM_WEAR_HEAD);
  setarmor(SPEC_ARMOR_TYPE_HALF_PLATE_ARMS, "half plate sleeves", ARMOR_TYPE_HEAVY,
    6000, 15, 1, -6, 40, 20, 15,
    9, MATERIAL_STEEL, ITEM_WEAR_ARMS);
  setarmor(SPEC_ARMOR_TYPE_HALF_PLATE_LEGS, "half plate leggings", ARMOR_TYPE_HEAVY,
    6000, 15, 1, -6, 40, 20, 15,
    9, MATERIAL_STEEL, ITEM_WEAR_LEGS);

  /* (armor, name, type,
   *    cost, AC, dexBonusCap, armorCheckPenalty, spellFailChance, (move)30ft, (move)20ft,
   *    weight, material, wear) */
  setarmor(SPEC_ARMOR_TYPE_FULL_PLATE, "full plate armor", ARMOR_TYPE_HEAVY,
    15000, 39, 1, -6, 35, 20, 15,
    23, MATERIAL_STEEL, ITEM_WEAR_BODY);
  setarmor(SPEC_ARMOR_TYPE_FULL_PLATE_HEAD, "full plate helm", ARMOR_TYPE_HEAVY,
    15000, 17, 1, -6, 35, 20, 15,
    9, MATERIAL_STEEL, ITEM_WEAR_HEAD);
  setarmor(SPEC_ARMOR_TYPE_FULL_PLATE_ARMS, "full plate sleeves", ARMOR_TYPE_HEAVY,
    15000, 17, 1, -6, 35, 20, 15,
    9, MATERIAL_STEEL, ITEM_WEAR_ARMS);
  setarmor(SPEC_ARMOR_TYPE_FULL_PLATE_LEGS, "full plate leggings", ARMOR_TYPE_HEAVY,
    15000, 17, 1, -6, 35, 20, 15,
    9, MATERIAL_STEEL, ITEM_WEAR_LEGS);

  /* (armor, name, type,
   *    cost, AC, dexBonusCap, armorCheckPenalty, spellFailChance, (move)30ft, (move)20ft,
   *    weight, material, wear) */
  setarmor(SPEC_ARMOR_TYPE_BUCKLER, "buckler shield", ARMOR_TYPE_SHIELD,
    150, 10, 99, -1, 5, 999, 999,
    5, MATERIAL_WOOD, ITEM_WEAR_SHIELD);
  setarmor(SPEC_ARMOR_TYPE_SMALL_SHIELD, "small shield", ARMOR_TYPE_SHIELD,
    90, 10, 99, -1, 5, 999, 999,
    6, MATERIAL_WOOD, ITEM_WEAR_SHIELD);
  setarmor(SPEC_ARMOR_TYPE_LARGE_SHIELD, "heavy shield", ARMOR_TYPE_SHIELD,
    200, 20, 99, -2, 15, 999, 999,
    13, MATERIAL_WOOD, ITEM_WEAR_SHIELD);
  setarmor(SPEC_ARMOR_TYPE_TOWER_SHIELD, "tower shield", ARMOR_TYPE_TOWER_SHIELD,
    300, 40, 2, -10, 50, 999, 999,
    45, MATERIAL_WOOD, ITEM_WEAR_SHIELD);
}
