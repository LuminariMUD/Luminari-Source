/*
 * File:   assign_wpn_armor.h
 * Author: Zusuk
 */

#ifndef ASSIGN_WPN_ARMOR_H
#define ASSIGN_WPN_ARMOR_H

struct weapon_table
{
  const char *name;
  const char *description;
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
  /* not implemented yet */
  ubyte stunNumDice;
  ubyte stunSizeDice;
  ubyte availability;
  ush_int ammo;
};

struct armor_table
{
  const char *name;
  const char *description;
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
int get_defending_weapon_bonus(struct char_data *ch, bool weapon);
bool is_using_ghost_touch_weapon(struct char_data *ch);
bool has_speed_weapon(struct char_data *ch);
int get_lucky_weapon_bonus(struct char_data *ch);
int get_agile_weapon_dex_bonus(struct char_data *ch);

struct obj_data *is_using_ranged_weapon(struct char_data *ch, bool silent_mode);
// bool *is_ranged_weapon(struct obj_data *obj);
bool is_using_double_weapon(struct char_data *ch);
bool is_using_light_weapon(struct char_data *ch, struct obj_data *wielded);

int is_proficient_with_weapon(struct char_data *ch, int weapon_type);
int is_proficient_with_armor(struct char_data *ch);
int is_proficient_with_shield(struct char_data *ch);
int is_proficient_with_body_armor(struct char_data *ch);
int is_proficient_with_helm(struct char_data *ch);
int is_proficient_with_sleeves(struct char_data *ch);
int is_proficient_with_leggings(struct char_data *ch);

bool is_reloading_weapon(struct char_data *ch, struct obj_data *wielded, bool silent);
bool can_fire_ammo(struct char_data *ch, bool silent);
bool has_ammo_in_pouch(struct char_data *ch, struct obj_data *wielded,
                       bool silent);
bool reload_weapon(struct char_data *ch, struct obj_data *wielded, bool silent);
bool auto_reload_weapon(struct char_data *ch, bool silent_mode);
bool weapon_is_loaded(struct char_data *ch, struct obj_data *wielded, bool silent);
bool is_bare_handed(struct char_data *ch);
bool monk_gear_ok(struct char_data *ch);
bool is_two_handed_ranged_weapon(struct obj_data *obj);
/**/

ACMD_DECL(do_weaponlist);
ACMD_DECL(do_weaponlist_old);
ACMD_DECL(do_armorlist);
ACMD_DECL(do_armorlist_old);

/**/

#define ARMOR_STYLE_NONE 0
#define ARMOR_STYLE_CLOTHING 1
#define ARMOR_STYLE_PADDED 2
#define ARMOR_STYLE_LEATHER 3
#define ARMOR_STYLE_STUDDED_LEATHER 4
#define ARMOR_STYLE_LIGHT_CHAINMAIL 5
#define ARMOR_STYLE_HIDE 6
#define ARMOR_STYLE_SCALE 7
#define ARMOR_STYLE_CHAINMAIL 8
#define ARMOR_STYLE_PIECEMEAL 9
#define ARMOR_STYLE_SPLINT 10
#define ARMOR_STYLE_BANDED 11
#define ARMOR_STYLE_HALF_PLATE 12
#define ARMOR_STYLE_PLATE_MAIL 13

#define NUM_ARMOR_STYLES 14

#ifdef __cplusplus
extern "C"
{
#endif

#ifdef __cplusplus
}
#endif

#endif /* ASSIGN_WPN_ARMOR_H */
