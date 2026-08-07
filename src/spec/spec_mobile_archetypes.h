/**
 * @file spec/spec_mobile_archetypes.h
 * Public API for reusable legacy combat and companion mobile archetypes.
 *
 * All rights reserved. See license for complete information.
 */

#ifndef LUMINARI_SPEC_MOBILE_ARCHETYPES_H
#define LUMINARI_SPEC_MOBILE_ARCHETYPES_H

struct char_data;

int bonedancer(struct char_data *ch, void *me, int cmd, const char *argument);
int cityguard(struct char_data *ch, void *me, int cmd, const char *argument);
int dog(struct char_data *ch, void *me, int cmd, const char *argument);
int dracolich_mob(struct char_data *ch, void *me, int cmd, const char *argument);
int ethereal_pet(struct char_data *ch, void *me, int cmd, const char *argument);
int lich_mob(struct char_data *ch, void *me, int cmd, const char *argument);
int mercenary(struct char_data *ch, void *me, int cmd, const char *argument);
int phantom(struct char_data *ch, void *me, int cmd, const char *argument);
int planewalker(struct char_data *ch, void *me, int cmd, const char *argument);
int practice_dummy(struct char_data *ch, void *me, int cmd, const char *argument);
int shades(struct char_data *ch, void *me, int cmd, const char *argument);
int skeleton_zombie(struct char_data *ch, void *me, int cmd, const char *argument);
int solid_elemental(struct char_data *ch, void *me, int cmd, const char *argument);
int totemanimal(struct char_data *ch, void *me, int cmd, const char *argument);
int vampire(struct char_data *ch, void *me, int cmd, const char *argument);
int vampire_mob(struct char_data *ch, void *me, int cmd, const char *argument);
int wraith(struct char_data *ch, void *me, int cmd, const char *argument);
int wraith_elemental(struct char_data *ch, void *me, int cmd, const char *argument);

#endif /* LUMINARI_SPEC_MOBILE_ARCHETYPES_H */
