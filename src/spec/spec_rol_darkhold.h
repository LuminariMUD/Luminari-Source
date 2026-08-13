/**
 * @file spec/spec_rol_darkhold.h
 * Source-profiled Darkhold adapters for the Realms of Luminari conversion.
 */

#ifndef LUMINARI_SPEC_ROL_DARKHOLD_H
#define LUMINARI_SPEC_ROL_DARKHOLD_H

#include <stdbool.h>
#include <stddef.h>

struct char_data;
struct obj_data;
struct spec_event_context;

enum rol_darkhold_object_kind
{
  ROL_DARKHOLD_OBJECT_NONE = 0,
  ROL_DARKHOLD_OBJECT_SUMMON_SKULL,
  ROL_DARKHOLD_OBJECT_PASSAGE_SKULL,
  ROL_DARKHOLD_OBJECT_NORTH_GEM,
  ROL_DARKHOLD_OBJECT_SOUTH_GEM
};

int rol_darkhold_object(struct char_data *ch, void *me, int cmd, const char *argument);
int rol_darkhold_object_typed(struct spec_event_context *context);
size_t rol_darkhold_object_profile_count(void);
bool rol_darkhold_object_profile(int object_vnum, enum rol_darkhold_object_kind *kind,
                                 int *room_vnum, int *destination_vnum);

bool rol_darkhold_monster_profile(int mobile_vnum, bool *shadow_fiend, bool *shadow_dragon);
int rol_darkhold_mobile_death(struct spec_event_context *context, struct char_data *ch);
int rol_darkhold_mobile_hit(struct spec_event_context *context, struct char_data *ch);
bool rol_darkhold_shadow_fiend_steal_roll_fires(int roll);
int rol_darkhold_shadow_fiend_cooldown_seconds(bool darkness);

bool rol_darkhold_warhammer_roll_fires(int roll);
int rol_darkhold_bastard_modifier(bool npc, int roll);
int rol_darkhold_weapon_hit(struct spec_event_context *context, struct char_data *ch,
                            struct obj_data *obj, struct char_data *victim, int slot,
                            bool warhammer);

#endif /* LUMINARI_SPEC_ROL_DARKHOLD_H */
