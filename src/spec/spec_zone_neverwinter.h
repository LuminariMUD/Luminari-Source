/**
 * @file spec_zone_neverwinter.h
 * Public API for Neverwinter control-puzzle procedures.
 */

#ifndef LUMINARI_SPEC_ZONE_NEVERWINTER_H
#define LUMINARI_SPEC_ZONE_NEVERWINTER_H

struct char_data;

int neverwinter_button_control(struct char_data *ch, void *me, int cmd, const char *argument);
int neverwinter_valve_control(struct char_data *ch, void *me, int cmd, const char *argument);

#endif /* LUMINARI_SPEC_ZONE_NEVERWINTER_H */
