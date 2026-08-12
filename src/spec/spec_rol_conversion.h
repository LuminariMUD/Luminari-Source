/**
 * @file spec/spec_rol_conversion.h
 * Shared Realms of Luminari conversion special-procedure adapters.
 */

#ifndef LUMINARI_SPEC_ROL_CONVERSION_H
#define LUMINARI_SPEC_ROL_CONVERSION_H

#include <stdbool.h>

struct char_data;
struct obj_data;

enum rol_bandit_demand
{
  ROL_BANDIT_DEMAND_PASS = -1,
  ROL_BANDIT_DEMAND_ATTACK = -2,
  ROL_BANDIT_DEMAND_TAKE_WAGON = -3
};

int rol_corpse_devourer(struct char_data *ch, void *me, int cmd, const char *argument);
int rol_poison_bite(struct char_data *ch, void *me, int cmd, const char *argument);
int rol_thief(struct char_data *ch, void *me, int cmd, const char *argument);
int rol_breath_weapon_fire(struct char_data *ch, void *me, int cmd, const char *argument);
int rol_breath_weapon_cold(struct char_data *ch, void *me, int cmd, const char *argument);
int rol_breath_weapon_acid(struct char_data *ch, void *me, int cmd, const char *argument);
int rol_breath_weapon_gas(struct char_data *ch, void *me, int cmd, const char *argument);
int rol_breath_weapon_lightning(struct char_data *ch, void *me, int cmd, const char *argument);
int rol_breath_attack_acid(struct char_data *ch, void *me, int cmd, const char *argument);
int rol_breath_attack_lightning(struct char_data *ch, void *me, int cmd, const char *argument);
int rol_magic_pool(struct char_data *ch, void *me, int cmd, const char *argument);
int rol_auto_distributor(struct char_data *ch, void *me, int cmd, const char *argument);
int rol_shadow_giant(struct char_data *ch, void *me, int cmd, const char *argument);
int rol_guild_guard(struct char_data *ch, void *me, int cmd, const char *argument);
int rol_major_beholder(struct char_data *ch, void *me, int cmd, const char *argument);
int rol_bandit(struct char_data *ch, void *me, int cmd, const char *argument);

bool rol_corpse_devourer_can_consume(const struct obj_data *obj);
int rol_poison_bite_roll_ceiling(int level);
int rol_umberhulk_proc_chance(int level);
int rol_shadow_giant_spook_damage(bool save_succeeded);
bool rol_shadow_giant_spook_immune(struct char_data *target);
bool rol_shadow_giant_stun_succeeds(int level, int chance_roll, int penalty_roll);
bool rol_guild_guard_allows(int room_vnum, int direction, const struct char_data *ch);
bool rol_guild_guard_protects(int room_vnum);
int rol_major_beholder_eye_spell(int eye);
int rol_major_beholder_eye_cooldown(int state, int eye);
int rol_major_beholder_advance_cooldowns(int state, unsigned int fired_eye_mask);
int rol_bandit_cargo_value(struct char_data *ch);
int rol_bandit_fee_gold(int target_vnum, int cargo_value, int alignment, int carried_gold);
int rol_planar_gate_cooldown_seconds(const struct char_data *ch);
bool rol_automatic_race_activity(struct char_data *ch);
void rol_automatic_race_combat_turn(struct char_data *ch);
bool rol_handle_conjured_death(struct char_data *ch);
bool rol_update_mobile_home_after_move(struct char_data *ch, int source_room, int destination_room);

#endif /* LUMINARI_SPEC_ROL_CONVERSION_H */
