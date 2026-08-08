/**
 * @file objsave.h
 * Public callback API for rent and cryogenic storage services.
 */

#ifndef LUMINARI_OBJ_OBJSAVE_H
#define LUMINARI_OBJ_OBJSAVE_H

struct char_data;

int cryogenicist(struct char_data *ch, void *me, int cmd, const char *argument);
int receptionist(struct char_data *ch, void *me, int cmd, const char *argument);

#endif /* LUMINARI_OBJ_OBJSAVE_H */
