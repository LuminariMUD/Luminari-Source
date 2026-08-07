/**
 * @file spec/spec_zone_crimson_flame.h
 * Public API for Crimson Flame zone procedures.
 *
 * All rights reserved. See license for complete information.
 */

#ifndef LUMINARI_SPEC_ZONE_CRIMSON_FLAME_H
#define LUMINARI_SPEC_ZONE_CRIMSON_FLAME_H

struct char_data;

int cf_converter(int value);
int cf_alathar(struct char_data *ch, void *me, int cmd, const char *argument);
int cf_trainingmaster(struct char_data *ch, void *me, int cmd, const char *argument);

#endif /* LUMINARI_SPEC_ZONE_CRIMSON_FLAME_H */
