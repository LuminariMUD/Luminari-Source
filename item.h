/*
 * File:   item.h
 * Author: Zusuk
 *
 */

#ifndef ITEM_H
#define ITEM_H

#ifdef __cplusplus
extern "C"
{
#endif

#define ITEM_STAT_MODE_IMMORTAL 0
#define ITEM_STAT_MODE_IDENTIFY_SPELL 1
#define ITEM_STAT_MODE_LORE_SKILL 2
#define ITEM_STAT_MODE_G_LORE 3

    void do_stat_object(struct char_data *ch, struct obj_data *j, int mode);
    void display_item_object_values(struct char_data *ch, struct obj_data *item, int mode);

#ifdef __cplusplus
}
#endif

#endif /* ITEM_H */
