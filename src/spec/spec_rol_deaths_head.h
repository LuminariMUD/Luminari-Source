/**
 * @file spec/spec_rol_deaths_head.h
 * Converted Undermountain Death's Head lifecycle.
 */

#ifndef LUMINARI_SPEC_ROL_DEATHS_HEAD_H
#define LUMINARI_SPEC_ROL_DEATHS_HEAD_H

#include <stdbool.h>
#include <stddef.h>

struct char_data;
struct spec_event_context;

enum rol_deaths_head_kind
{
  ROL_DEATHS_HEAD_NONE = 0,
  ROL_DEATHS_HEAD_SAPLING,
  ROL_DEATHS_HEAD_FRUIT,
  ROL_DEATHS_HEAD_YOUNG,
  ROL_DEATHS_HEAD_MATURE
};

int rol_deaths_head(struct char_data *ch, void *me, int cmd, const char *argument);
int rol_deaths_head_typed(struct spec_event_context *context);
long event_rol_deaths_head_seed(void *event_obj);

size_t rol_deaths_head_mobile_profile_count(void);
bool rol_deaths_head_mobile_profile(int mobile_vnum, enum rol_deaths_head_kind *kind);
bool rol_deaths_head_seed_profile(int object_vnum);
int rol_deaths_head_initial_head_min(enum rol_deaths_head_kind kind);
int rol_deaths_head_initial_head_max(enum rol_deaths_head_kind kind);
int rol_deaths_head_mature_regrowth_count(int current_heads, int random_heads);
bool rol_deaths_head_larger_tree(enum rol_deaths_head_kind current,
                                 enum rol_deaths_head_kind candidate);
bool rol_deaths_head_mature_wood_drop_enabled(void);
long rol_deaths_head_source_delay_pulses(int source_pulses);
int rol_deaths_head_seed_damage_min(int growth);
int rol_deaths_head_seed_damage_max(int growth);

#endif /* LUMINARI_SPEC_ROL_DEATHS_HEAD_H */
