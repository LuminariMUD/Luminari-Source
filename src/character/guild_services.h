/**
 * @file character/guild_services.h
 * Public API for class guild training and entrance services.
 *
 * All rights reserved. See license for complete information.
 */

#ifndef LUMINARI_CHARACTER_GUILD_SERVICES_H
#define LUMINARI_CHARACTER_GUILD_SERVICES_H

struct char_data;

int guild(struct char_data *ch, void *me, int cmd, const char *argument);
int guild_guard(struct char_data *ch, void *me, int cmd, const char *argument);

#endif /* LUMINARI_CHARACTER_GUILD_SERVICES_H */
