/**
 * @file spec/spec_zone_ttf.h
 * Public API for the Temple of Twisted Flesh procedures.
 *
 * All rights reserved. See license for complete information.
 */

#ifndef LUMINARI_SPEC_ZONE_TTF_H
#define LUMINARI_SPEC_ZONE_TTF_H

struct char_data;

extern int ttf_path[];

int ttf_monstrosity(struct char_data *ch, void *me, int cmd, const char *argument);
int ttf_abomination(struct char_data *ch, void *me, int cmd, const char *argument);
int ttf_rotbringer(struct char_data *ch, void *me, int cmd, const char *argument);
int ttf_patrol(struct char_data *ch, void *me, int cmd, const char *argument);

#endif /* LUMINARI_SPEC_ZONE_TTF_H */
