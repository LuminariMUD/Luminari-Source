/**
 * @file spec/spec_zone_battlemaze.h
 * Public API for Battlemaze zone procedures.
 *
 * All rights reserved. See license for complete information.
 */

#ifndef LUMINARI_SPEC_ZONE_BATTLEMAZE_H
#define LUMINARI_SPEC_ZONE_BATTLEMAZE_H

struct char_data;

int battlemaze_guard(struct char_data *ch, void *me, int cmd, const char *argument);

#endif /* LUMINARI_SPEC_ZONE_BATTLEMAZE_H */
