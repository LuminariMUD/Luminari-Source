/**************************************************************************
 *  File: onboarding.h                                                     *
 *  Usage: Structured account and character-creation state for web clients *
 *                                                                         *
 *  The MUD stays authoritative for every option, restriction, validation, *
 *  and save. It publishes a bounded view of nanny() and, in protocol v2,   *
 *  accepts only verified editor transfers through the shared checked save. *
 *  Nothing here ever emits a password, a password hash, or a secret.       *
 **************************************************************************/

#ifndef WEB_CLIENT_ONBOARDING_H
#define WEB_CLIENT_ONBOARDING_H

struct descriptor_data;
enum roleplay_commit_result;

/* Bump only with a matching change in the web client contract. */
#define WEB_ONBOARDING_PROTOCOL_VERSION 1

/*
 * Highest version this build can speak. v2 adds the role-play identity suite:
 * the profile hub, structured multiline editors with chunked transfer, and
 * checked persistence results. See luminariweb/docs/adr/0004.
 */
#define WEB_ONBOARDING_PROTOCOL_VERSION_MAX 2

/*
 * Master switch for protocol v2. Defaults to off so v2 is something an
 * operator opts into, and so rollback is a capability downgrade rather than a
 * redeploy. Build with -DWEB_ONBOARDING_ENABLE_V2=1 to offer it.
 */
#ifndef WEB_ONBOARDING_ENABLE_V2
#define WEB_ONBOARDING_ENABLE_V2 0
#endif

/* Reserved MSDP variable names. Players must not be able to remap these. */
#define WEB_ONBOARDING_MSDP_VARIABLE "LUMINARI_ONBOARDING"
#define WEB_ONBOARDING_CAPABILITY_VARIABLE "LUMINARI_ONBOARDING_VERSION"
/* Client-advertised list of supported versions, e.g. "2,1". v2 negotiation. */
#define WEB_ONBOARDING_VERSIONS_CAPABILITY_VARIABLE "LUMINARI_ONBOARDING_VERSIONS"
/* Reserved variable carrying v2 structured actions (editor transfer). */
#define WEB_ONBOARDING_ACTION_VARIABLE "LUMINARI_ONBOARDING_ACTION"
/* Reserved variable carrying source-to-client editor content chunks. */
#define WEB_ONBOARDING_CONTENT_VARIABLE "LUMINARI_ONBOARDING_CONTENT"

/* Keep well under protocol.h's MAX_VARIABLE_LENGTH of 16384. */
#define WEB_ONBOARDING_MAX_PAYLOAD 15000

/* Frozen protocol-v2 editor bounds. See luminariweb ADR 0004. */
#define WEB_ONBOARDING_EDITOR_MAX_CHUNK_BYTES 6144
#define WEB_ONBOARDING_EDITOR_MAX_BASE64_BYTES 8192
#define WEB_ONBOARDING_EDITOR_MAX_CONTENT_BYTES 49152
#define WEB_ONBOARDING_EDITOR_MAX_CHUNKS 8
#define WEB_ONBOARDING_EDITOR_TRANSFER_TIMEOUT_MS 30000
#define WEB_ONBOARDING_EDITOR_RATE_WINDOW_MS 60000
#define WEB_ONBOARDING_EDITOR_MAX_COMMITS_PER_WINDOW 12
#define WEB_ONBOARDING_EDITOR_MAX_BYTES_PER_WINDOW (512 * 1024)
#define WEB_ONBOARDING_EDITOR_MAX_ID_BYTES 120

/* Bounded, server-authored validation failures that may be shown to clients. */
enum web_onboarding_error
{
  WEB_ONBOARDING_ERROR_NONE = 0,
  WEB_ONBOARDING_ERROR_INVALID_NAME,
  WEB_ONBOARDING_ERROR_NAME_TAKEN,
  WEB_ONBOARDING_ERROR_EDITOR_INVALID_TRANSFER,
  WEB_ONBOARDING_ERROR_EDITOR_STALE,
  WEB_ONBOARDING_ERROR_EDITOR_TOO_LARGE,
  WEB_ONBOARDING_ERROR_EDITOR_INVALID_CONTENT,
  WEB_ONBOARDING_ERROR_EDITOR_RATE_LIMITED,
  WEB_ONBOARDING_ERROR_EDITOR_SAVE_FAILED,
  WEB_ONBOARDING_ERROR_EDITOR_LOAD_FAILED,
  WEB_ONBOARDING_ERROR_ROLEPLAY_INVALID_SELECTION,
  WEB_ONBOARDING_ERROR_ROLEPLAY_LOCKED,
  WEB_ONBOARDING_ERROR_ROLEPLAY_SAVE_FAILED
};

/* Record the client-declared protocol version during MSDP negotiation. */
void web_onboarding_set_capability(struct descriptor_data *d, const char *value);

/*
 * Record the client's full supported-version list ("2,1") and select the
 * highest version both sides can speak. An old client never sends this, so v1
 * negotiation is unaffected.
 */
void web_onboarding_set_version_list(struct descriptor_data *d, const char *value);

/* TRUE when this descriptor negotiated a supported structured onboarding. */
bool web_onboarding_enabled(struct descriptor_data *d);

/* The version negotiated for this descriptor, or 0 when structured UI is off. */
int web_onboarding_version(struct descriptor_data *d);

/* TRUE when this descriptor negotiated v2 and v2 is enabled in this build. */
bool web_onboarding_v2_enabled(struct descriptor_data *d);

/* Emit a new state if the connection state changed or was marked dirty. */
void web_onboarding_tick(struct descriptor_data *d);

/* Force the next tick to re-emit, e.g. after a validation failure. */
void web_onboarding_mark_dirty(struct descriptor_data *d);

/* Attach a bounded validation failure and force a same-state refresh. */
void web_onboarding_set_error(struct descriptor_data *d, enum web_onboarding_error error);

/* Publish a safe durable result for a checked non-text role-play commit. */
void web_onboarding_report_roleplay_commit(struct descriptor_data *d,
                                           enum roleplay_commit_result result);

/* Consume source-owned catalog paging controls without entering nanny data. */
bool web_onboarding_handle_catalog_control(struct descriptor_data *d, const char *value);

/*
 * Accept one strict JSON envelope from WEB_ONBOARDING_ACTION_VARIABLE.
 * Content is never logged and no character field changes before a complete,
 * current transfer has passed its size, UTF-8, ordering, and SHA-256 checks.
 */
void web_onboarding_handle_action(struct descriptor_data *d, const char *payload);

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

#ifdef LUMINARI_CUTEST
/* Deterministic clock and state inspection for bounded transfer tests. */
void web_onboarding_handle_action_at_for_test(struct descriptor_data *d, const char *payload,
                                              int64_t now_ms);
bool web_onboarding_has_active_transfer_for_test(struct descriptor_data *d);
bool web_onboarding_has_active_outbound_transfer_for_test(struct descriptor_data *d);
#endif

#endif /* WEB_CLIENT_ONBOARDING_H */
