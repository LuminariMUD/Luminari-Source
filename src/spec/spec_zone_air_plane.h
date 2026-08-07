/**
 * @file spec/spec_zone_air_plane.h
 * Public API for Air Plane zone procedures.
 *
 * All rights reserved. See license for complete information.
 */

#ifndef LUMINARI_SPEC_ZONE_AIR_PLANE_H
#define LUMINARI_SPEC_ZONE_AIR_PLANE_H

#include <stdbool.h>

struct char_data;

bool yan_yell(struct char_data *ch);
void yan_maelstrom(struct char_data *ch);
void yan_windgust(struct char_data *ch);
bool chan_yell(struct char_data *ch);
int yan(struct char_data *ch, void *me, int cmd, const char *argument);
int chan(struct char_data *ch, void *me, int cmd, const char *argument);

#endif /* LUMINARI_SPEC_ZONE_AIR_PLANE_H */
