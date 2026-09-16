/**
 * @file combat_messages.h
 * Internal contract for rendering the text of a resolved combat action.
 *
 * Combat is split so that resolution and presentation change independently:
 *
 *   fight.c            - resolution: what hits, for how much, and what dies.
 *   combat_messages.c  - presentation: what the room reads about it.
 *
 * Resolution calls into presentation and never the other way round. Nothing
 * here decides an outcome, so retuning combat text cannot change a fight, and
 * a combat maths change cannot silently reword the room.
 *
 * This header is internal to the combat subsystem. Only fight.c and
 * combat_messages.c include it.
 *
 * Ownership and lifetime
 * ----------------------
 * Both functions borrow every pointer for the duration of the call and store
 * none of them. They do not free, extract, or reposition any participant, and
 * they never take ownership of the projectile.
 *
 * The one allocation is internal: skill_message_with_projectile() may
 * read_object() a stand-in claw object to name a natural attack, and it
 * extracts that object before returning. Callers see no allocation.
 *
 * Nullability
 * -----------
 * ch and victim/vict must be non-NULL; the callers resolve them before an
 * attack lands. projectile is optional and NULL means "not a ranged attack
 * with a distinct missile".
 *
 * Reentrancy and thread safety
 * ----------------------------
 * Not thread-safe and not reentrant. Text is assembled in file-static
 * scratch buffers, so a returned string stays valid only until the next call
 * into this module. Callers must send it before invoking either function
 * again. Both run on the game loop thread only.
 *
 * Side effects
 * ------------
 * Presentation is not purely cosmetic here: the skill-message path reports
 * defensive reactions to the special-procedure gateway
 * (spec_gateway_defense_reaction), because a shieldblock, parry, glance, or
 * dodge becomes observable at the moment it is described. That call is a
 * notification; it does not feed back into the resolved outcome.
 */

#ifndef COMBAT_MESSAGES_H
#define COMBAT_MESSAGES_H

#include "core/structs.h"

/* Render a hit or miss using the message tables loaded from lib/misc/messages.
 * Returns SKILL_MESSAGE_MISS_FAIL when no message matches the attack type, in
 * which case the caller falls back to dam_message(). projectile may be NULL. */
int skill_message_with_projectile(int dam, struct char_data *ch, struct char_data *vict,
                                  int attacktype, int attack_mode, struct obj_data *projectile);

/* skill_message_with_projectile() for attacks that carry no distinct missile. */
int skill_message(int dam, struct char_data *ch, struct char_data *vict, int attacktype,
                  int attack_mode);

/* Generic weapon damage text, scaled by the fraction of the victim's health
 * removed. This is the fallback used when no skill message matches, and it
 * always produces output. projectile may be NULL. */
void dam_message(int dam, struct char_data *ch, struct char_data *victim, int w_type,
                 int attack_type, struct obj_data *projectile);

#endif /* COMBAT_MESSAGES_H */
