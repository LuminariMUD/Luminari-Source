/**
 * @file spec/spec_zone_orc_ruins.h
 * Public API for Orc Ruins zone procedures.
 *
 * All rights reserved. See license for complete information.
 */

#ifndef LUMINARI_SPEC_ZONE_ORC_RUINS_H
#define LUMINARI_SPEC_ZONE_ORC_RUINS_H

struct char_data;

int shar_heart(struct char_data *ch, void *me, int cmd, const char *argument);
int shar_statue(struct char_data *ch, void *me, int cmd, const char *argument);

#endif /* LUMINARI_SPEC_ZONE_ORC_RUINS_H */
