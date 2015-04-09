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


#ifdef	__cplusplus
extern "C" {
#endif




#ifdef	__cplusplus
}
#endif

#endif	/* ASSIGN_WPN_ARMOR_H */

