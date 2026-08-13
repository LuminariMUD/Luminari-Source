/**
 * @file spec/spec_rol_utility_objects.h
 * Converted Realms of Luminari utility-object special procedures.
 */

#ifndef LUMINARI_SPEC_ROL_UTILITY_OBJECTS_H
#define LUMINARI_SPEC_ROL_UTILITY_OBJECTS_H

#include <stdbool.h>
#include <stddef.h>

struct char_data;
struct spec_event_context;

int rol_utility_object(struct char_data *ch, void *me, int cmd, const char *argument);
int rol_utility_object_typed(struct spec_event_context *context);

bool rol_utility_loot_blockable_container(const struct obj_data *obj);
bool rol_utility_plague_eligible(struct char_data *ch, const struct obj_data *obj);
int rol_utility_loot_sweep_interval_seconds(void);
int rol_utility_orchid_decay_hours(void);

bool rol_utility_sacrifice_keyword(const char *argument);
bool rol_utility_sacrifice_command_name(const char *command);
const char *rol_utility_necro_child_message(int roll);
bool rol_utility_monocle_room(int room_vnum);
bool rol_utility_spiderhaunt_maggots_trigger(const char *argument);
bool rol_utility_spiderhaunt_altar_trigger(const char *command, int position);
bool rol_utility_acheron_roaming_room_allowed(int room_vnum);
bool rol_utility_acheron_platform_room_allowed(int current_vnum, int destination_vnum);
size_t rol_utility_called_profile_count(void);
bool rol_utility_called_profile(int object_vnum, const char **phrase, int *cooldown_hours,
                                const char **description);

#endif /* LUMINARI_SPEC_ROL_UTILITY_OBJECTS_H */
