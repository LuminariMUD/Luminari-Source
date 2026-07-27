/**************************************************************************
 *  File: onboarding.h                                                     *
 *  Usage: Structured account and character-creation state for web clients *
 *                                                                         *
 *  The MUD stays authoritative for every option, restriction, validation, *
 *  and save. This module only publishes a bounded, read-only view of the  *
 *  nanny() state machine so a capable client can present it graphically.  *
 *  Nothing here ever emits a password, a password hash, or a secret.      *
 **************************************************************************/

#ifndef WEB_CLIENT_ONBOARDING_H
#define WEB_CLIENT_ONBOARDING_H

struct descriptor_data;

/* Bump only with a matching change in the web client contract. */
#define WEB_ONBOARDING_PROTOCOL_VERSION 1

/* Reserved MSDP variable names. Players must not be able to remap these. */
#define WEB_ONBOARDING_MSDP_VARIABLE "LUMINARI_ONBOARDING"
#define WEB_ONBOARDING_CAPABILITY_VARIABLE "LUMINARI_ONBOARDING_VERSION"

/* Keep well under protocol.h's MAX_VARIABLE_LENGTH of 16384. */
#define WEB_ONBOARDING_MAX_PAYLOAD 15000

/* Record the client-declared protocol version during MSDP negotiation. */
void web_onboarding_set_capability(struct descriptor_data *d, const char *value);

/* TRUE when this descriptor negotiated a supported structured onboarding. */
bool web_onboarding_enabled(struct descriptor_data *d);

/* Emit a new state if the connection state changed or was marked dirty. */
void web_onboarding_tick(struct descriptor_data *d);

/* Force the next tick to re-emit, e.g. after a validation failure. */
void web_onboarding_mark_dirty(struct descriptor_data *d);

/* Clear per-connection onboarding tracking. */
void web_onboarding_reset(struct descriptor_data *d);

/* Stable media keys, deliberately independent of display text. */
const char *web_onboarding_race_media_key(int race);
const char *web_onboarding_class_media_key(int chclass);

/*
 * Build the payload for the descriptor's current state without sending it.
 * Returns FALSE when the state has no structured presentation or when the
 * document would not fit, in which case the buffer holds an empty string.
 * Exposed so the payload can be asserted directly in tests.
 */
bool web_onboarding_build_payload(struct descriptor_data *d, char *buf, size_t buf_size);

#endif /* WEB_CLIENT_ONBOARDING_H */
