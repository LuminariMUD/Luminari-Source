/**
 * @file spec/spec_rol_conversion.h
 * Shared Realms of Luminari conversion special-procedure adapters.
 */

#ifndef LUMINARI_SPEC_ROL_CONVERSION_H
#define LUMINARI_SPEC_ROL_CONVERSION_H

#include <stdbool.h>

struct char_data;
struct obj_data;

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

bool rol_corpse_devourer_can_consume(const struct obj_data *obj);
int rol_poison_bite_roll_ceiling(int level);
int rol_umberhulk_proc_chance(int level);
int rol_planar_gate_cooldown_seconds(const struct char_data *ch);
bool rol_automatic_race_activity(struct char_data *ch);
void rol_automatic_race_combat_turn(struct char_data *ch);
bool rol_handle_conjured_death(struct char_data *ch);

#endif /* LUMINARI_SPEC_ROL_CONVERSION_H */
