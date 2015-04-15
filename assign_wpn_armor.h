/*
 * File:   assign_wpn_armor.h
 * Author: Zusuk
 */

#ifndef ASSIGN_WPN_ARMOR_H
#define	ASSIGN_WPN_ARMOR_H


struct weapon_table
{
  char *name;
  sbyte numDice;
  ubyte diceSize;
  sbyte critRange;
  sbyte critMult;
  ush_int weaponFlags;
  ush_int cost;
  ush_int damageTypes;
  ush_int weight;
  ubyte range;
  ush_int weaponFamily;
  byte size;
  ubyte material;
  ubyte handle_type;
  ubyte head_type;
  ubyte stunNumDice;
  ubyte stunSizeDice;
  ubyte availability;
  ush_int ammo;
};

struct armor_table
{
  char *name;
  ubyte armorType;
  ush_int cost;
  ubyte armorBonus;
  ubyte dexBonus;
  byte armorCheck;
  ubyte spellFail;
  ubyte thirtyFoot;
  ubyte twentyFoot;
  ush_int weight;
  ubyte material;
  ubyte availability;
  ubyte fort_bonus;
  int wear;
};

extern struct weapon_table weapon_list[NUM_WEAPON_TYPES];
extern const char *weapon_type[NUM_WEAPON_TYPES];

extern struct armor_table armor_list[NUM_SPEC_ARMOR_TYPES];
extern const char *armor_type[NUM_SPEC_ARMOR_TYPES];

/* functions available through assign_wpn_armor.c */
int compute_gear_max_dex(struct char_data *ch);
int compute_gear_enhancement_bonus(struct char_data *ch);
int compute_gear_spell_failure(struct char_data *ch);
int compute_gear_armor_penalty(struct char_data *ch);
int compute_gear_armor_type(struct char_data *ch);
int compute_gear_shield_type(struct char_data *ch);
bool is_using_double_weapon(struct char_data *ch);
bool is_using_light_weapon(struct char_data *ch, struct obj_data *wielded);
int is_proficient_with_weapon(struct char_data *ch, int weapon_type);
int is_proficient_with_armor(struct char_data *ch);
bool monk_gear_ok(struct char_data *ch);


#ifdef	__cplusplus
extern "C" {
#endif




#ifdef	__cplusplus
}
#endif

#endif	/* ASSIGN_WPN_ARMOR_H */

