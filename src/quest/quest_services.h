/**
 * @file quest_services.h
 * Public callback API for quest support services.
 */

#ifndef LUMINARI_QUEST_SERVICES_H
#define LUMINARI_QUEST_SERVICES_H

struct char_data;

int replace_quest_item(struct char_data *ch, void *me, int cmd, const char *argument);

#endif /* LUMINARI_QUEST_SERVICES_H */
