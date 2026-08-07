/**
 * @file spec/spec_zone_kenjin_tower.h
 * Public API for Tower of Kenjin zone procedures.
 *
 * All rights reserved. See license for complete information.
 */

#ifndef LUMINARI_SPEC_ZONE_KENJIN_TOWER_H
#define LUMINARI_SPEC_ZONE_KENJIN_TOWER_H

struct char_data;

int kt_kenjin(struct char_data *ch, void *me, int cmd, const char *argument);
int kt_twister(struct char_data *ch, void *me, int cmd, const char *argument);

#endif /* LUMINARI_SPEC_ZONE_KENJIN_TOWER_H */
