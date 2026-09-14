/**
 * @file objsave.h
 * Public callback API for rent and cryogenic storage services.
 */

#ifndef LUMINARI_OBJ_OBJSAVE_H
#define LUMINARI_OBJ_OBJSAVE_H

struct char_data;
struct obj_data;

int cryogenicist(struct char_data *ch, void *me, int cmd, const char *argument);
int receptionist(struct char_data *ch, void *me, int cmd, const char *argument);


bool pet_save_objs(struct char_data *ch, struct char_data *owner, long int pet_idnum);
int objsave_save_obj_record_db_sheath(struct obj_data *obj, struct char_data *ch,
                                      long int sheath_idnum, int sheath_slot);
#endif /* LUMINARI_OBJ_OBJSAVE_H */
