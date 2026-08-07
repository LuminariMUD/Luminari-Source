/**
 * @file spec/spec_zone_prisoner.h
 * Public API for The Prisoner encounter procedures.
 *
 * All rights reserved. See license for complete information.
 */

#ifndef LUMINARI_SPEC_ZONE_PRISONER_H
#define LUMINARI_SPEC_ZONE_PRISONER_H

#include <stdbool.h>

struct char_data;

extern int prisoner_heads;
extern bool eq_loaded;

int check_heads(struct char_data *ch);
void move_items(struct char_data *ch, struct char_data *lich);
void prisoner_on_death(struct char_data *ch);
int rejuv_prisoner(struct char_data *ch);
int prisoner_attacks(struct char_data *ch);
void prisoner_gear_loading(struct char_data *ch);

int prisoner_dracolich(struct char_data *ch, void *me, int cmd, const char *argument);
int the_prisoner(struct char_data *ch, void *me, int cmd, const char *argument);
int tia_rapier(struct char_data *ch, void *me, int cmd, const char *argument);

#endif /* LUMINARI_SPEC_ZONE_PRISONER_H */
