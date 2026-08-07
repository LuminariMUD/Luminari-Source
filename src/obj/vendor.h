/**
 * @file obj/vendor.h
 * Public API for commerce and item-service procedures.
 *
 * All rights reserved. See license for complete information.
 */

#ifndef LUMINARI_OBJ_VENDOR_H
#define LUMINARI_OBJ_VENDOR_H

struct char_data;
struct spec_event_context;

int bank(struct char_data *ch, void *me, int cmd, const char *argument);
int bank_typed(struct spec_event_context *context);
int bought_pet(struct char_data *ch, void *me, int cmd, const char *argument);
int buyarmor(struct char_data *ch, void *me, int cmd, const char *argument);
int buyweapons(struct char_data *ch, void *me, int cmd, const char *argument);
int identify_mob(struct char_data *ch, void *me, int cmd, const char *argument);
int pet_shops(struct char_data *ch, void *me, int cmd, const char *argument);

#endif /* LUMINARI_OBJ_VENDOR_H */
