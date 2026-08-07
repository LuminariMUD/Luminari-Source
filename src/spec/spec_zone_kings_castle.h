/**
 * @file spec/spec_zone_kings_castle.h
 * Public API for King's Castle assignment and mobile procedures.
 *
 * All rights reserved. See license for complete information.
 */

#ifndef LUMINARI_SPEC_ZONE_KINGS_CASTLE_H
#define LUMINARI_SPEC_ZONE_KINGS_CASTLE_H

struct char_data;

void assign_kings_castle(void);
int do_npc_rescue(struct char_data *ch, struct char_data *friend);

int CastleGuard(struct char_data *ch, void *me, int cmd, const char *argument);
int DicknDavid(struct char_data *ch, void *me, int cmd, const char *argument);
int James(struct char_data *ch, void *me, int cmd, const char *argument);
int cleaning(struct char_data *ch, void *me, int cmd, const char *argument);
int jerry(struct char_data *ch, void *me, int cmd, const char *argument);
int king_welmar(struct char_data *ch, void *me, int cmd, const char *argument);
int peter(struct char_data *ch, void *me, int cmd, const char *argument);
int tim(struct char_data *ch, void *me, int cmd, const char *argument);
int tom(struct char_data *ch, void *me, int cmd, const char *argument);
int training_master(struct char_data *ch, void *me, int cmd, const char *argument);

#endif /* LUMINARI_SPEC_ZONE_KINGS_CASTLE_H */
