/**
 * @file spec/spec_zone_jot.h
 * Public API for Jot invasion, encounter, and object procedures.
 *
 * All rights reserved. See license for complete information.
 */

#ifndef LUMINARI_SPEC_ZONE_JOT_H
#define LUMINARI_SPEC_ZONE_JOT_H

#include <stdbool.h>

struct char_data;

extern bool jot_inv_check;
extern int fg_pos[];
extern int sb_pos[];
extern int frost_pos[];

int jot_converter(int value);
void jot_invasion(void);

int jot_invasion_loader(struct char_data *ch, void *me, int cmd, const char *argument);
int thrym(struct char_data *ch, void *me, int cmd, const char *argument);
int ymir(struct char_data *ch, void *me, int cmd, const char *argument);
int planetar(struct char_data *ch, void *me, int cmd, const char *argument);
int gatehouse_guard(struct char_data *ch, void *me, int cmd, const char *argument);
int ymir_cloak(struct char_data *ch, void *me, int cmd, const char *argument);
int mistweave(struct char_data *ch, void *me, int cmd, const char *argument);
int frostbite(struct char_data *ch, void *me, int cmd, const char *argument);
int vaprak_claws(struct char_data *ch, void *me, int cmd, const char *argument);
int fake_twilight(struct char_data *ch, void *me, int cmd, const char *argument);
int twilight(struct char_data *ch, void *me, int cmd, const char *argument);
int valkyrie_sword(struct char_data *ch, void *me, int cmd, const char *argument);
int planetar_sword(struct char_data *ch, void *me, int cmd, const char *argument);
int giantslayer(struct char_data *ch, void *me, int cmd, const char *argument);

#endif /* LUMINARI_SPEC_ZONE_JOT_H */
