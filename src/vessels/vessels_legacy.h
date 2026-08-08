/**
 * @file vessels_legacy.h
 * Public API for legacy route and Greyhawk vessel procedures.
 */

#ifndef LUMINARI_VESSELS_LEGACY_H
#define LUMINARI_VESSELS_LEGACY_H

struct char_data;

int alandor_ferry(struct char_data *ch, void *me, int cmd, const char *argument);
int chionthar_ferry(struct char_data *ch, void *me, int cmd, const char *argument);
int greyhawk_ship_commands(struct char_data *ch, void *me, int cmd, const char *argument);
int greyhawk_ship_object(struct char_data *ch, void *me, int cmd, const char *argument);
int md_carpet(struct char_data *ch, void *me, int cmd, const char *argument);

#endif /* LUMINARI_VESSELS_LEGACY_H */
