/**
 * @file spec/spec_zone_celestial_leviathan.h
 * Public API for Celestial Leviathan encounter procedures.
 *
 * All rights reserved. See license for complete information.
 */

#ifndef LUMINARI_SPEC_ZONE_CELESTIAL_LEVIATHAN_H
#define LUMINARI_SPEC_ZONE_CELESTIAL_LEVIATHAN_H

struct char_data;

int rejuv_celestial_leviathan(struct char_data *ch);
int celestial_leviathan_attacks(struct char_data *ch);
int celestial_leviathan(struct char_data *ch, void *me, int cmd, const char *argument);

#endif /* LUMINARI_SPEC_ZONE_CELESTIAL_LEVIATHAN_H */
