/**
 * @file spec/spec_rol_drow.h
 * Source-profiled drow-equipment decay for the Realms of Luminari conversion.
 */

#ifndef LUMINARI_SPEC_ROL_DROW_H
#define LUMINARI_SPEC_ROL_DROW_H

#include <stdbool.h>
#include <stddef.h>

struct char_data;
struct obj_data;
struct spec_event_context;

int rol_drow_equipment(struct char_data *ch, void *me, int cmd, const char *argument);
int rol_drow_equipment_typed(struct spec_event_context *context);
long event_rol_drow_decay(void *event_obj);

size_t rol_drow_equipment_profile_count(void);
bool rol_drow_equipment_profile(int object_vnum);
bool rol_drow_decayable_sector(int sector_type);
int rol_drow_decay_modulus(bool inside_object, int hour, bool sunlight);
long rol_drow_decay_delay_pulses(int jitter_pulses);
bool rol_drow_reduce_object_value(struct obj_data *obj, int decay_modulus);

#endif /* LUMINARI_SPEC_ROL_DROW_H */
