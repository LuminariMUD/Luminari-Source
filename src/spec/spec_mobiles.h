/**
 * @file spec/spec_mobiles.h
 * Public API for general legacy mobile special procedures.
 *
 * All rights reserved. See license for complete information.
 */

#ifndef LUMINARI_SPEC_MOBILES_H
#define LUMINARI_SPEC_MOBILES_H

struct char_data;

int mayor(struct char_data *ch, void *me, int cmd, const char *argument);
int snake(struct char_data *ch, void *me, int cmd, const char *argument);
int hound(struct char_data *ch, void *me, int cmd, const char *argument);
int thief(struct char_data *ch, void *me, int cmd, const char *argument);
int wizard(struct char_data *ch, void *me, int cmd, const char *argument);
int wall(struct char_data *ch, void *me, int cmd, const char *argument);
int puff(struct char_data *ch, void *me, int cmd, const char *argument);
int fido(struct char_data *ch, void *me, int cmd, const char *argument);
int janitor(struct char_data *ch, void *me, int cmd, const char *argument);

#endif /* LUMINARI_SPEC_MOBILES_H */
