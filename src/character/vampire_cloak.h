/**
 * @file character/vampire_cloak.h
 * Typed vampire-cloak procedure and legacy callback-slot adapter.
 */

#ifndef LUMINARI_CHARACTER_VAMPIRE_CLOAK_H
#define LUMINARI_CHARACTER_VAMPIRE_CLOAK_H

struct char_data;
struct spec_event_context;

int vampire_cloak(struct char_data *ch, void *me, int cmd, const char *argument);
int vampire_cloak_typed(struct spec_event_context *context);

#endif /* LUMINARI_CHARACTER_VAMPIRE_CLOAK_H */
