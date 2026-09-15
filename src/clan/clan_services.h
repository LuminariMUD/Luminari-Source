/**
 * @file clan_services.h
 * Public API for clan hall mobile services and access control.
 *
 * All rights reserved. See license for complete information.
 */

#ifndef LUMINARI_CLAN_SERVICES_H
#define LUMINARI_CLAN_SERVICES_H

struct char_data;

int clan_cleric(struct char_data *ch, void *me, int cmd, const char *argument);
int clan_guard(struct char_data *ch, void *me, int cmd, const char *argument);

#endif /* LUMINARI_CLAN_SERVICES_H */
