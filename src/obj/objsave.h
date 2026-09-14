/**
 * @file objsave.h
 * Public callback API for rent and cryogenic storage services.
 */

#ifndef LUMINARI_OBJ_OBJSAVE_H
#define LUMINARI_OBJ_OBJSAVE_H

struct char_data;

int cryogenicist(struct char_data *ch, void *me, int cmd, const char *argument);
int receptionist(struct char_data *ch, void *me, int cmd, const char *argument);


bool pet_save_objs(struct char_data *ch, struct char_data *owner, long int pet_idnum);
#endif /* LUMINARI_OBJ_OBJSAVE_H */
