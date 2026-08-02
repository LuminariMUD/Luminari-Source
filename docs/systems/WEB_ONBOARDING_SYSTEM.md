# Structured Web Onboarding System

Last verified: 2026-07-29

## Purpose and Status

Luminari-Source can publish a structured view of account login, the account
lobby, character creation, the selected-character menu, and the role-play
profile to a capable web client. The implementation is in
`src/net/onboarding.c` and
`src/net/onboarding.h`.

Protocol v1 covers the account and core character-creation flow. Protocol v2
adds the complete role-play identity suite and private multiline field
transfer. V1 is always available when negotiated. V2 is implemented but
defaults off at compile time and must be enabled deliberately.

This module is a presentation adapter, not another account system or character
creator. The authoritative workflow is documented in
[PLAYER_MANAGEMENT_SYSTEM.md](PLAYER_MANAGEMENT_SYSTEM.md).

## Non-Negotiable Boundaries

The system preserves these ownership rules:

- Luminari-Source owns authentication, available choices, unlocks,
  restrictions, validation, mutation, and persistence.
- `nanny()` remains the only account and character-creation state machine.
- The gateway validates browser messages and translates ordinary actions to
  exact server-provided line values.
- The browser owns presentation, responsive media, accessibility behavior, and
  the option to reveal the classic terminal.
- Terminal clients continue to use the existing text prompts.
- Images, audio, and video never travel through Telnet or MSDP.

The supported runtime boundary is:

```text
Browser UI
  |  typed JSON over the web gateway's WebSocket contract
  v
Luminari Web gateway
  |  runtime validation and Telnet/MSDP adaptation
  v
Luminari-Source descriptor
  |  structured view of nanny(), plus ordinary line input
  v
Account, character, player-index, and role-play persistence
```

The MUD descriptor is the authenticated session. A gateway must not read
`account_data`, compare password hashes, create characters directly, or mutate
player files.

## Design Rationale

The versioned MSDP adapter was chosen because it adds a structured presentation
without replacing the existing Telnet transport or `nanny()` workflow. It
therefore preserves ordinary Telnet clients, supports capability negotiation,
and has a tested terminal fallback when either repository is older.

The following approaches remain outside this design:

- Parsing colored terminal prompts in the gateway is not a protocol contract.
  Prompt wording, wrapping, help output, and color changes would make it
  brittle.
- Copying race, class, alignment, unlock, or persistence rules into TypeScript
  would create a second implementation that can drift from the game.
- Giving the gateway direct database or player-file access would bypass the
  authenticated descriptor and duplicate ownership, validation, and save
  behavior.
- Native MUD WebSockets or a new GMCP module family require their own transport,
  schema, coexistence, security, operations, migration, and rollback design.
  They are not implicit extensions of this adapter.

## Source Integration

The implementation touches four lifecycle boundaries:

1. `src/net/protocol.c` recognizes reserved capability and action MSDP variables.
2. `src/comm.c` initializes and resets onboarding state with the descriptor.
3. The main game loop calls `web_onboarding_tick()` once per pulse for every
   descriptor.
4. `nanny()` input marks the descriptor dirty after processing, including when
   validation leaves it in the same state.

Polling `d->connected` is intentional. A state transition cannot leave the web
screen stale merely because a caller forgot a presentation hook. A supported
state emits a new document; an unsupported state emits a handoff document and
returns presentation control to the terminal.

The descriptor stores only connection-local protocol state:

- negotiated onboarding version;
- monotonically increasing revision;
- last emitted connection state;
- dirty flag and bounded error enum;
- opaque v2 transfer, paging, result, and rate-limit state.

`web_onboarding_reset()` wipes and releases private transfer state on
descriptor initialization and close.

## Reserved MSDP Variables

These names are protocol control variables and must not be exposed as
user-remappable HUD settings.

| Variable | Direction | Purpose |
| --- | --- | --- |
| `LUMINARI_ONBOARDING_VERSION` | Client to source | Legacy capability; v1 sends `1` |
| `LUMINARI_ONBOARDING_VERSIONS` | Client to source | Ordered supported-version list such as `2,1` |
| `LUMINARI_ONBOARDING` | Source to client | Bounded structured state document |
| `LUMINARI_ONBOARDING_ACTION` | Client to source | Protocol-v2 private editor action envelopes |
| `LUMINARI_ONBOARDING_CONTENT` | Source to client | Protocol-v2 private editor content envelopes |

The client sends the v1 capability first so an old source can negotiate v1.
A new source then scans the supported-version list defensively and selects the
highest version that both sides support. It echoes the selected version in
every state document.

`web_onboarding_enabled()` also requires active MSDP negotiation. Advertising a
version without MSDP does not enable the structured path.

## Version Coverage

### Protocol v1

V1 maps 21 `CON_*` states:

- account name, confirmation, existing/new password, account lobby, and
  legacy-character linking;
- character name, confirmation, sex, race list/detail, class list/detail,
  premade/custom build, alignment, preferences, and role-play decision;
- MOTD and selected-character menu.

V1 ends structured presentation when the player enters an unsupported
role-play state or `CON_PLAYING`. The classic terminal remains live and
continues the workflow.

### Protocol v2

V2 includes all v1 states and adds 29 role-play states:

- the 14-item role-play hub;
- generated short-description feature, adjective, preview, and confirmation
  states;
- long description and background story;
- background catalog and confirmation;
- goals, personality, ideals, bonds, and flaws examples and editors;
- age, homeland, faction, hometown, and deity selection and confirmation.

The source owns hub status, mutability, required/locked state, catalog pages,
examples, selected details, and persistence results. A v1 client never receives
v2-only screens.

## State Document

`LUMINARI_ONBOARDING` carries one JSON document encoded as an MSDP string. The
source caps it at `WEB_ONBOARDING_MAX_PAYLOAD` (15,000 bytes), below
`MAX_VARIABLE_LENGTH` (16,384 bytes). The builder fails closed instead of
sending truncated JSON.

Every state contains:

| Field | Meaning |
| --- | --- |
| `version` | Negotiated protocol version |
| `flowId` | Connection-local descriptor/login identifier |
| `revision` | Increasing state revision |
| `mode` and `screen` | Stable presentation identifiers |
| `title` and `prompt` | Bounded server-authored display text |
| `inputKind` | `none`, `text`, `password`, `choice`, `confirm`, or `multiline` |
| `sensitiveInput` | Whether entered text must receive password handling |
| `persistence` | Core state-machine position: `none`, `draft`, `pending`, or `saved` |
| `choices` | Bounded enabled choices with exact wire values |
| `characters` | Account character summaries on the lobby screen |
| `selection` | Only the character choices already made |
| `actions` | Actions valid for the current presentation |

Depending on the state, v2 can also add `profile`, `page`, `detail`, `examples`,
`editor`, `blocking`, `controlWire`, `error`, and `persistenceResult`.

`flowId` and `revision` allow the gateway to reject a button or form submission
created for an older screen. Ordinary actions are still revalidated by the
current `nanny()` handler after the gateway translates them.

Entering `CON_PLAYING` or an unsupported state emits a minimal `mode: "play"`,
`screen: "handoff"` document with no choices or private state.

## Choice and Catalog Authority

The adapter builds catalogs from the same source structures and rule helpers as
the terminal:

- account cards load actual character records;
- races use `race_list[]`, player-race flags, `is_locked_race()`, and
  `has_unlocked_race()`;
- classes use `class_list[]`, in-game/prestige/lock flags,
  `has_unlocked_class()`, and `valid_class_race_alignment()`;
- alignments use `valid_align_by_class()` and `valid_align_by_race()`;
- role-play catalogs use source background, region, clan, city, deity, age,
  short-description, and example data.

Do not copy these rules into C adapter tables, the gateway, or TypeScript.
Catalog builders must call the source-owned filters.

Stable IDs and media keys are deliberately separate from display text. Race
and class media keys use explicit tables; role-play IDs use domain-specific
helpers. This protects the web asset contract from spelling corrections,
colored labels, runtime names, and internal enum tokens. Unknown or unsupported
keys resolve to a domain fallback.

Large v2 catalogs are paged in the source. Page controls are consumed before
ordinary `nanny()` input so a presentation-only page change cannot become a
game selection.

## Ordinary Actions

Most browser actions remain line-compatible:

- selecting a card sends its server-provided `wireValue`;
- confirmation and reselection use the exact values accepted by the current
  state;
- account-menu create, link, selection, and quit use existing menu values;
- v2 hub, catalog, example, and navigation actions use source-provided item or
  control wire values.

The gateway must validate the action against its latest state before sending
the line. The source then validates the line through the unchanged handler.
The adapter never calculates legality from a browser-provided ID.

Private multiline text is the exception. It must never use the command-line
path because embedded newlines and editor commands would break byte accounting
and widen the injection surface.

## Protocol-v2 Editor Transfer

V2 moves each private multiline field through a strict begin/chunk/commit
side-channel in both directions. An upload has this lifecycle:

```text
begin metadata
  -> ordered base64 chunks
  -> commit with matching transfer ID
  -> UTF-8 normalization and field validation
  -> checked character save
  -> newer state with a safe result
```

The source validates exact JSON keys, protocol version, flow, revision, current
state, field ID, transfer ID, declared size, chunk count, chunk ordering,
canonical base64, SHA-256 digest, timeout, rate budget, UTF-8, control
characters, and the field-specific byte limit before committing.

Frozen source bounds are:

| Bound | Value |
| --- | ---: |
| State payload | 15,000 bytes |
| Raw editor chunk | 6,144 bytes |
| Base64 chunk | 8,192 bytes |
| Global editor content | 49,152 bytes |
| Chunks per transfer | 8 |
| Transfer lifetime | 30,000 ms |
| ID length | 120 bytes |
| Commit budget | 12 per 60 seconds |
| Byte budget | 512 KiB per 60 seconds |
| Concurrent state | One inbound and one outbound transfer per descriptor |

The long-description field has its smaller source field limit; every other
field is also capped by `roleplay_text_field_max_bytes()`. The global limit
does not override a smaller field limit.

No character field changes until the full upload passes validation.
`roleplay_text_commit_checked()` normalizes line endings, applies the same tilde
handling as the terminal editor, calls `save_char_checked()`, and restores the
old value on failure. Existing private content follows the reverse transfer and
is not embedded in the hub or state document.

Transfer memory is overwritten and released on success, rejection, timeout,
state change, capability downgrade, fallback, disconnect, account switch, and
character switch. `src/net/protocol.c` also overwrites its inbound MSDP value buffer
before freeing it.

## Persistence Semantics

The core `persistence` field is derived from connection state:

- `draft` before alignment;
- `pending` while alignment is being chosen;
- `saved` after the alignment handler has crossed its legacy save boundary;
- `none` outside core creation.

This describes workflow position, not a checked write receipt. The alignment
handler currently calls result-discarding save wrappers.

V2 `persistenceResult` is different. It describes a checked role-play commit:

- `accepted`: reserved for a future in-memory intermediate result; current
  checked commits do not emit it;
- `saved`: the checked durable save succeeded;
- `failed`: validation or durable persistence failed and the previous value
  remains authoritative.

Clients must not infer save success from animation, a state transition, or
terminal text.

## Sensitive Input and Privacy

The adapter never emits a password, password hash, submitted password, or
secret. Password screens set `inputKind: "password"` and
`sensitiveInput: true`. Telnet ECHO negotiation remains an independent
server-side signal; the gateway and browser should hide input if either signal
says it is sensitive.

Sensitive values must bypass:

- command history and tab completion;
- aliases, triggers, and automation;
- persistence, browser storage, and service-worker caches;
- logs, analytics, traces, snapshots, screenshots, and fixtures.

Account names, character lists, and role-play text are also private data. Do
not log full onboarding payloads or terminal transcripts. Use synthetic
fixtures and bounded error codes. Clear gateway and browser onboarding state on
disconnect and account/character changes.

HTTPS/WSS protects only the browser-to-gateway leg. The gateway-to-MUD leg is
ordinary Telnet TCP unless deployment supplies another transport. Keep it on
loopback or a trusted private network, or protect it with an authenticated
encrypted tunnel. Do not describe the system as encrypted end to end while
credentials cross plain Telnet.

## Media Boundary

The MUD emits only semantic media keys such as `race/human`,
`class/wizard`, or `region/fallback`. The web deployment owns:

- file URLs, versions, formats, and responsive crops;
- preload, lazy-load, and cache behavior;
- animation layers and reduced-motion presentation;
- alt text and visual fallback assets;
- audio assets and opt-in playback controls;
- provenance and licensing records.

Missing media must degrade to a functional generic card. It must never block
authentication or character creation. The live inventory, delivery status,
derivative requirements, byte budgets, and asset records are maintained by the
web client in `luminariweb/docs/manifest.md`; do not duplicate that checklist
in this repository.

### Web Manifest Contract

The web-owned media manifest must:

- have a versioned schema and runtime validation;
- resolve every current source-emitted media key or its domain fallback;
- route unknown future race, class, region, faction, hometown, and deity keys
  to the correct generic fallback;
- tolerate runtime-authored catalogs rather than assuming a permanently closed
  asset list;
- render multiclass and prestige-class account cards without requiring
  dedicated prestige-class art;
- keep live names, MOTD text, rules, validation, and private profile data out of
  raster assets and static caches;
- treat catalog imagery as an illustrative possibility, not a prescribed
  character appearance or persistent player avatar.

### Performance Requirements

- Serve responsive modern formats with a broadly supported fallback.
- Provide desktop and mobile crops; do not make a phone download desktop art.
- Lazy-load non-current catalog art and preload only the current or likely-next
  screen.
- Show a poster or placeholder before expensive animation layers are ready.
- Enforce explicit initial-load and per-screen byte budgets.
- Version static media so caches can be refreshed and retired predictably.
- Pause expensive animation in background tabs and respect data-saver signals
  where practical.
- Never place image, audio, or video bytes in WebSocket state, Telnet, or MSDP
  messages.

### Accessibility Requirements

- Every visual choice must remain an ordinary keyboard- and touch-operable
  control with visible focus.
- Selection, validation, and progress must not depend on color, sound, or
  animation.
- Reduced-motion mode must replace major motion while preserving meaning.
- Meaningful images need authored alt text; decorative images need empty alt
  text.
- Audio must be opt-in, muted by default, separately controllable, and never
  required to understand a choice.
- Screen-reader status announcements must report server validation and
  progression without replaying decorative copy.
- The classic terminal must remain available as a user-selected presentation,
  not only as an error fallback.

### Asset Provenance

The web-owned manifest should record the source, license, stable semantic key,
alt text, formats, crops, and replacement path for each asset. Representation,
similarity, composition, contrast, responsive behavior, and reduced-motion
review belong in the asset acceptance process. An asset that lacks acceptable
evidence must use the generic fallback rather than block onboarding.

## Compatibility and Recovery

| Client | Source | Result |
| --- | --- | --- |
| Current web gateway | Current source | Full structured core and role-play flow |
| Current web gateway | Older v1-only source | Core UI; terminal RP fallback |
| Current web gateway | Older or unrelated MUD | Classic terminal |
| Ordinary Telnet client | Current source | Existing text menus |
| Malformed or unsupported client state | Current source | Terminal handoff; no forced disconnect |
| Current client with missing media | Current source | Structured flow with generic media fallback |

Classic terminal access is a compatibility path, a recovery path, and a
debugging surface. New screens must not discard terminal output.

## Build, Activation, and Rollback

Protocol v2 is compiled into every source build. The retired rollout switch is
explicitly rejected:

```c
#ifdef WEB_ONBOARDING_ENABLE_V2
#error "WEB_ONBOARDING_ENABLE_V2 was removed; protocol v2 is always available"
#endif
```

With Autotools:

```sh
make clean
./configure
make -j"$(nproc)"
make test
make install
```

The current gateway always advertises versions `2,1`, and the current source
always selects v2. Version negotiation remains only for compatibility with old
gateways, old sources, and ordinary Telnet clients. It is not an operator
switch. Roll back a bad release to the last known-good complete release; do not
disable the role-play product capability.

## Testing

The production-linked source suite is
`unittests/CuTest/test_web_onboarding.c`. It covers:

- negotiation, version skew, and mandatory v2 availability;
- representative v1 screens and every mapped v2 role-play state;
- authoritative catalog filtering, stable IDs, media keys, paging, and wire
  values;
- bounded JSON and overflow refusal;
- sensitive-state mapping and core persistence position;
- private editor download/upload, exact limits, ordering, digest, timeout,
  lifecycle, rate limits, and cleanup;
- checked save success, failure, and rollback;
- role-play selection commit and player-index rollback.

The focused protocol parser harness separately verifies that reserved
capability and action variables reach the production handlers.

Exercise the single production compile mode:

```sh
make clean
./configure
make test
make install

make -C unittests/CuTest protocol-parser
```

For sanitizer, Valgrind, fuzzing, and full-suite commands, see
[TESTING_GUIDE.md](../guides/TESTING_GUIDE.md).

Cross-repository changes also require gateway contract, parser, session,
reducer, component, real-listener, synthetic-MUD, responsive, and
version-skew tests. They must also exercise missing media, muted audio,
reduced-motion, data-saver, keyboard, screen-reader, focus, and partial-cache
behavior. Never use live account or character data as fixtures.

## Maintenance Checklist

When changing this protocol:

1. Trace the real `nanny()` transition and domain handler.
2. Keep the MUD authoritative; do not recreate rules in presentation code.
3. Add or update the screen mapping and state-exact source tests.
4. Use stable IDs and explicit media keys, never transformed display labels.
5. Keep state payloads below 15,000 bytes and private content out of them.
6. Preserve flow/revision checks and fail-to-terminal behavior.
7. Use checked persistence and rollback before exposing a durable-success
   result.
8. Keep passwords and profile content out of logs and ordinary command input.
9. Update the paired gateway contract and bump the protocol version for an
   incompatible change.
10. Test old client/new source, new client/old source, v2-to-v1 downgrade,
    classic Telnet, disconnect, and character/account switching.

If a new `.c` file is added, update both `Makefile.am` and `CMakeLists.txt`.

## Current Limitations

- V2 remains compile-time default-off.
- Password change and character deletion retain deliberate terminal
  presentation.
- Locked or incompatible race/class entries are omitted rather than emitted
  with structured reasons.
- The account lobby does not publish an explicit account-capacity field.
- Most core same-state validation failures re-emit the current state but expose
  their detailed reason only in terminal text; structured core errors currently
  cover character-name failures.
- Core creation persistence status is state-derived rather than a checked
  acknowledgement.
- Email verification, password recovery, MFA, and long-lived web sessions are
  outside this system.
- Plain Telnet on the gateway-to-MUD leg remains a deployment security concern
  until the deployment provides a trusted or encrypted route.

## Key Files

| File | Responsibility |
| --- | --- |
| `src/net/onboarding.h` | Versions, bounds, variables, errors, and public API |
| `src/net/onboarding.c` | Screens, catalogs, emission, transfers, and cleanup |
| `src/comm.c` | Per-pulse emission and descriptor lifecycle |
| `src/net/protocol.c` | Reserved MSDP capability and action dispatch |
| `src/interpreter.c` | Authoritative account and core creation state machine |
| `src/roleplay.c` | Role-play field IDs, limits, validation, and checked commits |
| `src/char_descs.c` | Generated short-description states and checked commit |
| `src/account.c` | Account data, unlocks, membership, and account menu |
| `src/structs.h` | Descriptor tracking and `CON_*` state definitions |
| `unittests/CuTest/test_web_onboarding.c` | Production-linked behavior and boundary tests |
