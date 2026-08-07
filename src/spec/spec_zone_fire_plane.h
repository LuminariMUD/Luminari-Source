/**
 * @file spec/spec_zone_fire_plane.h
 * Public API for Fire Plane zone procedures.
 *
 * All rights reserved. See license for complete information.
 */

#ifndef LUMINARI_SPEC_ZONE_FIRE_PLANE_H
#define LUMINARI_SPEC_ZONE_FIRE_PLANE_H

struct char_data;

int imix(struct char_data *ch, void *me, int cmd, const char *argument);
int fp_invoker(struct char_data *ch, void *me, int cmd, const char *argument);

#endif /* LUMINARI_SPEC_ZONE_FIRE_PLANE_H */
