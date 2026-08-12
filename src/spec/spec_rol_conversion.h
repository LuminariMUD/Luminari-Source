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

bool rol_corpse_devourer_can_consume(const struct obj_data *obj);
int rol_poison_bite_roll_ceiling(int level);

#endif /* LUMINARI_SPEC_ROL_CONVERSION_H */
