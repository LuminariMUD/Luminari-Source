/*****************************************************************************
 * projectiles.h                           Part of LuminariMUD
 * Shared launcher and thrown-projectile state and selection.
 *****************************************************************************/

#ifndef PROJECTILES_H
#define PROJECTILES_H

#include "structs.h"

enum projectile_source
{
  PROJECTILE_SOURCE_NONE = 0,
  PROJECTILE_SOURCE_AMMO_POUCH,
  PROJECTILE_SOURCE_INVENTORY,
  PROJECTILE_SOURCE_WIELDED
};

enum projectile_disposition
{
  PROJECTILE_DISPOSITION_NONE = 0,
  PROJECTILE_DISPOSITION_TARGET_ROOM,
  PROJECTILE_DISPOSITION_TARGET_INVENTORY,
  PROJECTILE_DISPOSITION_ATTACKER_INVENTORY,
  PROJECTILE_DISPOSITION_ATTACKER_EQUIPMENT,
  PROJECTILE_DISPOSITION_ATTACKER_ROOM,
  PROJECTILE_DISPOSITION_DESTROYED
};

struct projectile_attack_context
{
  int attack_kind;
  struct obj_data *attack_weapon;
  struct obj_data *physical_projectile;
  enum projectile_source original_source;
  obj_vnum anchor_vnum;
  int anchor_wear_slot;
  room_rnum target_room;
  int target_x;
  int target_y;
  bool detached;
  bool snatched;
  enum projectile_disposition disposition;
};

bool is_launcher_attack(int attack_type);
bool is_thrown_attack(int attack_type);
bool is_ranged_weapon_attack(int attack_type);
bool has_physical_projectile(int attack_type);

bool is_launcher_weapon(const struct obj_data *obj);
bool is_throwable_weapon(struct char_data *ch, const struct obj_data *obj);
bool can_store_projectile_in_ammo_pouch(struct char_data *ch, const struct obj_data *obj);
bool ammo_pouch_has_capacity(const struct obj_data *ammo_pouch);

struct obj_data *find_equipped_launcher(struct char_data *ch, int *wear_slot);
struct obj_data *find_equipped_throwable(struct char_data *ch, int *wear_slot);
struct obj_data *find_compatible_launcher_ammo(struct char_data *ch,
                                               const struct obj_data *launcher);
bool is_compatible_launcher_ammo(const struct obj_data *launcher, const struct obj_data *ammo);

void clear_projectile_mode(struct char_data *ch);
void clear_launcher_projectile_mode(struct char_data *ch);
void set_launcher_projectile_mode(struct char_data *ch);
bool set_thrown_projectile_mode(struct char_data *ch, obj_vnum anchor_vnum, int wear_slot);
bool validate_thrown_projectile_mode(struct char_data *ch);
bool can_throw_projectile(struct char_data *ch, bool silent);

void initialize_projectile_attack_context(struct projectile_attack_context *context,
                                          int attack_kind);
bool prepare_launcher_projectile(struct char_data *ch, struct projectile_attack_context *context);
bool select_thrown_projectile(struct char_data *ch, obj_vnum anchor_vnum, int wear_slot,
                              struct projectile_attack_context *context);
void set_projectile_target(struct projectile_attack_context *context,
                           const struct char_data *target);
bool detach_physical_projectile(struct char_data *ch, struct projectile_attack_context *context);
bool projectile_object_is_live(const struct obj_data *obj);
void finalize_physical_projectile(struct projectile_attack_context *context,
                                  struct char_data *attacker, struct char_data *target,
                                  enum projectile_disposition disposition);

#endif /* PROJECTILES_H */
