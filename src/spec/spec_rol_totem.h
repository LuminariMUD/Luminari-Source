/**
 * @file spec/spec_rol_totem.h
 * Converted Realms of Luminari shaman-totem behavior.
 */

#ifndef LUMINARI_SPEC_ROL_TOTEM_H
#define LUMINARI_SPEC_ROL_TOTEM_H

#include <stdbool.h>
#include <time.h>

struct char_data;

int rol_shaman_totem(struct char_data *ch, void *me, int cmd, const char *argument);
int rol_totem_restorer(struct char_data *ch, void *me, int cmd, const char *argument);

int rol_shaman_totem_choice(int target_vnum);
int rol_shaman_totem_vnum(int choice);
bool rol_shaman_totem_race_allowed(int target_vnum, int race);
int rol_shaman_totem_success_chance(struct char_data *ch);
bool rol_shaman_totem_consume_weekly_use(struct char_data *ch, time_t now);
bool rol_totem_restorer_requirements(const struct char_data *ch, int keeper_gold, int *totem_vnum);
const char *rol_totem_spirit_death_message(int target_vnum);

#endif /* LUMINARI_SPEC_ROL_TOTEM_H */
