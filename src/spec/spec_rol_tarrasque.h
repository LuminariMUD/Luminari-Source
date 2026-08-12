/**
 * @file spec/spec_rol_tarrasque.h
 * Converted Realms of Luminari Tarrasque encounter procedures.
 */

#ifndef LUMINARI_SPEC_ROL_TARRASQUE_H
#define LUMINARI_SPEC_ROL_TARRASQUE_H

#include <stdbool.h>

struct char_data;
struct spec_event_context;

int rol_tarrasque(struct char_data *ch, void *me, int cmd, const char *argument);
int rol_tarrasque_typed(struct spec_event_context *context);

int rol_tarrasque_loot_vnum_for_roll(int roll);
bool rol_tarrasque_corpse_keyword(const char *argument, const char *aliases);

#endif /* LUMINARI_SPEC_ROL_TARRASQUE_H */
