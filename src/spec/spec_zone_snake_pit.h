/**
 * @file spec/spec_zone_snake_pit.h
 * Public API for Snake Pit zone procedures.
 *
 * All rights reserved. See license for complete information.
 */

#ifndef LUMINARI_SPEC_ZONE_SNAKE_PIT_H
#define LUMINARI_SPEC_ZONE_SNAKE_PIT_H

struct char_data;

int naga_golem(struct char_data *ch, void *me, int cmd, const char *argument);
int naga(struct char_data *ch, void *me, int cmd, const char *argument);

#endif /* LUMINARI_SPEC_ZONE_SNAKE_PIT_H */
