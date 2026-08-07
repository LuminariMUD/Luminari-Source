/**
 * @file spec/spec_zone_fire_giant.h
 * Public API for Fire Giant invasion procedures.
 *
 * All rights reserved. See license for complete information.
 */

#ifndef LUMINARI_SPEC_ZONE_FIRE_GIANT_H
#define LUMINARI_SPEC_ZONE_FIRE_GIANT_H

struct char_data;

int fg_invasion_loader(struct char_data *ch, void *me, int cmd, const char *argument);
int flamekissed_instrument(struct char_data *ch, void *me, int cmd, const char *argument);

#endif /* LUMINARI_SPEC_ZONE_FIRE_GIANT_H */
