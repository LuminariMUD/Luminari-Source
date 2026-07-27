# Web Account and Character Creation Experience

**Status:** Implemented through Phase 2; Phase 3 and rollout outstanding
**Date:** 2026-07-28
**Runtime code changed:** Yes (development environment)

## Implementation Status (2026-07-28)

The recommended architecture in this document has been built end to end across
both repositories. The classic terminal remains fully usable and is the
automatic fallback everywhere the structured protocol is unavailable.

### Luminari-Source

- `src/systems/web_client/onboarding.{c,h}` publishes a bounded, read-only view
  of the `nanny()` state machine as a compact JSON document in the reserved
  `LUMINARI_ONBOARDING` MSDP variable, capped well under the 16 KB variable
  limit.
- Capability negotiation uses the reserved `LUMINARI_ONBOARDING_VERSION` MSDP
  variable, handled in `ExecuteMSDPPair()`. A client that does not send it keeps
  the text menus unchanged.
- Emission is driven by a per-pulse poll of `d->connected` in the main loop
  rather than by hooking each transition individually, so a moved or added
  `nanny()` transition can never leave a stale web screen. States this adapter
  does not present (OLC, string editors, the role-play suite) emit an explicit
  clear and hand control back to the terminal.
- Catalogs are built by calling the same authoritative filters the terminal
  uses: `is_locked_race()`, `has_unlocked_race()`, `has_unlocked_class()`,
  `CLSLIST_INGAME`, `CLSLIST_PRESTIGE`, `valid_class_race_alignment()`,
  `valid_align_by_class()`, and `valid_align_by_race()`. No rule is duplicated.
- Stable media keys are held in explicit tables, so the corrected
  `race/tiefling` key is emitted regardless of the internal token spelling.
- Both `Makefile.am` and `CMakeLists.txt` were updated. The tree builds clean
  with no new warnings.

### Luminari Web

- `shared/onboarding.ts` defines the versioned contract, its bounds, and real
  runtime validation. Session phase is modelled separately from connection
  status.
- `server/telnet-parser.ts` now tracks Telnet ECHO and reports sensitive input;
  `server/mud-session.ts` validates every payload, enforces the size cap,
  rejects stale flows and revisions, rate-limits onboarding separately from
  ordinary commands, and clears all onboarding state on disconnect.
- Passwords are masked, cleared on send, and excluded from command history,
  aliases, triggers, tab completion, persistence, and the service-worker cache -
  in both the structured UI and the classic terminal field.
- `src/features/onboarding/` contains the cinematic experience: scene backdrop
  with client-driven parallax and particle motion, race and class galleries,
  alignment compass, forms, account lobby, running summary, audio controls, and
  a reducer that only presents the latest server revision.
- `scripts/process-onboarding-media.mjs` builds versioned responsive
  derivatives and regenerates the media manifest; see
  [manifest.md](manifest.md) for asset delivery state.
- `scripts/onboarding-preview-server.mjs` replays the flow with synthetic data
  for design review without a MUD or database.

### Verified

- 156 production-linked CuTest cases pass, including a new
  `unittests/CuTest/test_web_onboarding.c` suite covering capability handling,
  media-key bounds, payload structure, the sensitive-input mapping, the save
  boundary, and refusal to truncate. Writing it found and fixed a real
  NULL-dereference: the catalog builders assumed `d->character` was always
  attached, which would have crashed the server if a descriptor lost its
  character while the per-pulse poll ran.
- 216 Node tests pass, including new contract, session, reducer, and media
  manifest suites. Lint, formatting, type-check, and production build are clean.
- A browser walkthrough completes the full flow from account name through the
  role-play decision and hands off to the terminal, with no console errors.
- No horizontal overflow at 390 px or 360 px; reduced-motion mode removes
  parallax and particles rather than merely shortening them.
- Published media is measured against the manifest's byte budgets by an
  automated test.

### Outstanding

- Phase 3 role-play identity suite.
- Feature flags, version-skew drills, load testing, and the rollout runbook.
- Layered scene masters, remaining catalog art, and all human approval gates.

## Executive Finding

The vision is feasible, but it is not primarily a frontend reskin. A polished
account lobby and motion-rich character creator is a medium-to-large
cross-repository feature because the current browser only sees terminal text
during login and character creation. The MUD owns a large, stateful workflow,
and the web client has no typed representation of those states.

The recommended direction is:

1. Keep Luminari-Source authoritative for account authentication, available
   choices, validation, unlocks, restrictions, character mutation, and saving.
2. Keep the integrated Node proxy as the supported browser transport.
3. Add a small, versioned Luminari onboarding protocol over the existing
   Telnet/MSDP connection.
4. Have the proxy validate that protocol and expose typed onboarding messages
   over its existing `/ws` JSON contract.
5. Build the cinematic React experience as progressive enhancement, with the
   ordinary terminal flow always available as a compatibility and recovery
   path.
6. Treat original illustration, motion, audio, and content production as a
   parallel workstream with its own budget.

The first useful release should cover secure account login, the account lobby,
existing-character selection, and the core new-character path through
alignment. The optional role-play profile suite should follow after the
contract is proven.

Rough scope:

- Secure structured MVP: 8-14 engineering person-weeks.
- Full account, core creation, optional role-play, polish, and rollout:
  15-28 engineering person-weeks.
- Original media production: approximately 4-8 artist-weeks for a reusable
  visual system, or 10-20+ artist-weeks for bespoke race/class illustrations
  and richer animation.

Those are planning ranges, not commitments. They assume an experienced C and
TypeScript contributor, a designer or art director, and no simultaneous
rewrite of authentication or the network transport.

## Audit Baseline

This document was produced from a read-only trace of both repositories.

| Repository | Baseline | Relevant current shape |
|------------|----------|------------------------|
| `Luminari-Source` | `b59a8c4d` | C MUD, descriptor-driven `nanny()` login and creation flow, MariaDB accounts |
| `luminariweb` | `e9ce006` | React browser client, Node WebSocket-to-Telnet proxy, MSDP HUD |

The MUD environment reports itself as non-production. No credentials were
copied into this document, and no production or runtime files were changed.

### Key code evidence

- `src/comm.c:2515` starts a new descriptor in protocol negotiation or account
  name state.
- `src/interpreter.c:6712-9033` contains the authoritative `nanny()` state
  machine.
- `src/account.c:1108` builds the current account character menu.
- `src/interpreter.c:7480-8298` implements sex, race, class, build, alignment,
  initial save, and preference selection.
- `src/interpreter.c:9035` and `src/roleplay.c:1045-1623` implement the
  role-play profile menus and state handlers.
- `src/comm.c:4803-5013` implements normal MSDP updates and limits them to
  `CON_PLAYING`.
- `luminariweb/shared/mud.ts:254-291` defines the current browser/proxy message
  contract.
- `luminariweb/server/telnet-parser.ts:197-258` handles Telnet negotiation and
  MSDP but does not forward sensitive-input state to React.
- `luminariweb/src/App.tsx:758-869` handles all current WebSocket messages.
- `luminariweb/src/App.tsx:2305-2349` uses one ordinary command form for all
  MUD input.

## Product Definition

The desired experience can be framed as four connected surfaces:

1. **Arrival and authentication**
   A branded landing transition, account name and password forms, new-account
   creation, clear validation, and safe password handling.
2. **Account lobby**
   Character cards with identity, race, classes, level, status, and actions to
   play or create a character.
3. **Guided character creation**
   A step-based experience for name, sex, race, class, build style, alignment,
   and preferences. Race and class choices can use large art, animated
   transitions, mechanical summaries, and comparison details.
4. **Optional character identity**
   Background, descriptions, age, homeland, faction, hometown, deity, goals,
   personality, ideals, bonds, and flaws.

The cinematic layer should make the existing game easier to understand. It
must not become a second implementation of the game rules.

## Current Architecture

```text
React browser client
  |  JSON: connect, disconnect, input, resize, MSDP config
  v
Node Express/WebSocket proxy
  |  Telnet negotiation, text, and MSDP
  v
Luminari-Source descriptor
  |  nanny() connection state machine
  v
Account, character, player files, and MariaDB persistence
```

### Current web-client boundary

The web client currently supports these browser-to-proxy messages:

- `connect`
- `disconnect`
- `input`
- `resize`
- `msdp-config`

The proxy currently sends:

- `connection-status`
- `terminal`
- `state`

There is no account, authentication, menu, character-list, or creation message
in `shared/mud.ts`. All pre-game interaction is sent as ordinary `input` text
and received as terminal output.

The current `connected` status means only that the proxy opened the MUD TCP
socket. It is true before the player supplies an account name or password. A
structured client therefore needs a separate session phase such as
`negotiating`, `account`, `character`, and `playing`; it must not overload
network connection status to mean authentication.

The frontend is also concentrated in a very large component and stylesheet:

- `src/App.tsx`: 4,193 lines
- `src/App.css`: 3,202 lines

That is workable for the current terminal/HUD application, but a multi-screen
creation experience should not be added as another large conditional block in
`App.tsx`. Feature extraction is part of the scope.

The current media footprint is intentionally small:

- One approximately 13 KB `src/assets/hero.png`
- SVG favicon and icon assets
- No animation or media runtime dependency
- No audio or video experience
- A basic `prefers-reduced-motion` rule

The existing service worker caches only the static application shell. A larger
media experience will need deliberate asset versioning, preload, lazy-load,
and cache behavior.

### Current MUD structured-data boundary

The MUD negotiates MSDP before account login, so the transport foundation is
available early enough. However, the normal `msdp_update()` loop only emits
game state when the descriptor is `CON_PLAYING`. Account and character creation
states do not currently emit structured state.

This is why the web client can render a character HUD after entering the game
but cannot render an account lobby or creation wizard before then.

## Traced Account Flow

The primary flow is in `src/interpreter.c:nanny()`, with account persistence and
menu rendering in `src/account.c`.

| Stage | MUD states | Current behavior |
|-------|------------|------------------|
| Protocol negotiation | `CON_GET_PROTOCOL` | Negotiates client protocols, prints greeting |
| Account identity | `CON_ACCOUNT_NAME` | Treats the first entered name as an account name |
| New-account confirmation | `CON_ACCOUNT_NAME_CONFIRM` | Confirms spelling, checks site ban and creation lock |
| Existing-account password | `CON_PASSWORD` | Verifies the account password and limits bad attempts |
| New-account password | `CON_NEWPASSWD`, `CON_CNFPASSWD` | Creates and confirms the account password |
| Account lobby | `CON_ACCOUNT_MENU` | Lists characters and accepts character number, create, add, or quit |
| Add existing character | `CON_ACCOUNT_ADD`, `CON_ACCOUNT_ADD_PWD` | Links an old character after ownership/password checks |
| Character load | `CON_RMOTD`, `CON_MENU` | Shows MOTD and the selected character menu |
| Enter game | `CON_PLAYING` | Runs `enter_player_game()` and begins normal play |

The account can hold up to `MAX_CHARS_PER_ACCOUNT`, currently 100. The existing
account menu loads each character to display name, level, race, and class
summary. That data is suitable for character cards, but it must be emitted as
structured data instead of reconstructed from formatted terminal rows.

Important account-related scope boundaries:

- There is no web account/session system in `luminariweb`.
- There is no HTTP account API.
- Account passwords are owned and verified by the MUD.
- The `account_data` record has an email field, but this login flow does not
  provide modern email verification or password recovery.
- Linking an existing character is a separate, sensitive ownership flow.
- Character password change and character deletion are available from the
  selected-character menu, not the account lobby.

## Traced Core Character-Creation Flow

The core new-character sequence is:

| Order | MUD state | Server-owned rule |
|------:|-----------|-------------------|
| 1 | `CON_GET_NAME` | Parse, length-check, reserve-word check, uniqueness check |
| 2 | `CON_NAME_CNFRM` | Confirm spelling, repeat ban/creation-lock checks, duplicate check |
| 3 | `CON_QSEX` | Current prompt accepts male or female |
| 4 | `CON_QRACE` | Parse race, require playable/unlocked race |
| 5 | `CON_QRACE_HELP` | Show race details and confirm/reselect |
| 6 | `CON_QCLASS` | Require in-game, non-prestige, unlocked, race-compatible class |
| 7 | `CON_QCLASS_HELP` | Show class details and confirm/reselect |
| 8 | `CON_CONFIRM_PREMADE` | Choose premade or custom build |
| 9 | `CON_QALIGN` | Choose an alignment valid for both race and class |
| 10 | Persistence boundary | Initialize, save character, link account, update player index |
| 11 | `CON_SETPREFS` | Optionally enable recommended preference flags |
| 12 | `CON_CHAR_RP_DECIDE` | Non-role-player, enter role-play details, or defer |
| 13 | `CON_RMOTD`, `CON_MENU` | Continue to character menu and game entry |

The point-buy block is currently compiled out with
`CHARGEN_NO_STATISTICS`. Ability scores, feats, skills, and later build choices
therefore must not appear in the first web creator as though they are part of
the current creation contract. Expanding level-one study is a separate project.

The current flow supports confirmation and re-selection in specific places. It
does not support arbitrary wizard-style back navigation. Adding a universal
Back button would require server-side rollback rules for dependent choices,
unlock state, feats, and partially persisted characters. The browser must not
fake backtracking by editing a local draft.

### Persistence timing matters

The character is saved after alignment, before recommended preferences and the
role-play decision. A disconnect before that point and a disconnect after that
point have different recovery behavior.

A polished client needs explicit server states for:

- Draft not yet persisted
- Character persisted but onboarding incomplete
- Creation completed
- Server rejected save/link operation
- Reconnect must return to terminal/account flow

The UI should not show a successful completion animation until the MUD confirms
the relevant save.

## Traced Optional Role-Play Flow

If the player elects to enter role-play information, the MUD exposes:

| Choice | Current implementation |
|--------|------------------------|
| Short description | Multi-step generated descriptor/adjective flow; may be required before game entry |
| Long description | MUD string editor |
| Background story | MUD string editor |
| Background archetype | Named catalog, description, confirmation, mechanical feat |
| Goals | Examples plus string editor |
| Personality | Background-themed examples plus string editor |
| Ideals | Background-themed examples plus string editor |
| Bonds | Background-themed examples plus string editor |
| Flaws | Background-themed examples plus string editor |
| Age | Five categories with ability modifiers; one-time selection |
| Homeland region | Runtime-selectable regions with descriptions and language effect |
| Faction | Runtime clan list, descriptions, membership effects |
| Hometown | Campaign/runtime choices with recall and donation behavior |
| Deity | Runtime deity list, alignment, portfolio, description, confirmation |

Most of this is optional, but the short-description setup can be forced when
the introduction system is enabled. A complete web experience eventually
needs both the optional profile editor and the forced short-description gate.

## Existing Authoritative Content

The project already owns much of the text and mechanics needed for rich cards.
It should be exposed, not copied by hand into TypeScript.

| Domain | Existing source authority |
|--------|---------------------------|
| Races | `race_list[]`: stable name, label, description, size, ability modifiers, playable flag, unlock cost, allowed alignments, racial feats |
| Classes | `class_list[]`: name, description, primary attributes, hit die, BAB, saves, skills, unlock cost, in-game/prestige flags |
| Compatibility | `valid_class_race_alignment()`, `valid_align_by_class()`, `valid_align_by_race()` |
| Account unlocks | `has_unlocked_race()`, `has_unlocked_class()` |
| Backgrounds | `background_list[]` and role-play example tables |
| Regions | `regions[]`, `is_selectable_region()`, `get_region_info()` |
| Factions | Runtime clan data |
| Hometowns | `cities[]` plus current campaign/runtime rules |
| Deities | `deity_list[]`: name, pantheon, alignment, portfolio, symbol, description |

The current compile-time ceilings are 28 player-race slots and 38 class slots,
although runtime playable, in-game, prestige, lock, and compatibility filters
reduce what a new character may actually select. Media production should be
based on the live emitted catalog, not those array ceilings.

Some descriptive content is still embedded directly in terminal prompt code.
Part of the MUD work is to normalize display data enough that the terminal and
web presentations draw from the same values.

## Experience Proposal

### 1. Arrival

- Full-viewport art or layered illustrated environment
- Clear Luminari identity
- Connect status presented as part of the scene
- Accessible "Use classic terminal" path
- No autoplay audio

### 2. Account login and creation

- Account-name form
- Password field that reacts to Telnet sensitive-input state
- New-account confirmation and password confirmation
- Inline server validation without guessing server rules
- No password history, aliases, triggers, persistence, analytics, or logs

### 3. Account lobby

- Character card grid or carousel
- Name, level, race, class summary, and status
- Primary Play action
- Create Character action
- Advanced actions for linking an old character, returning to the terminal,
  or quitting
- Empty-account welcome state
- Full-account state

### 4. Name and identity

- Character naming guidance in a readable side panel
- Server uniqueness/validity response
- Confirmation step
- Sex choice rendered as an accessible control, using the server's current
  available values

### 5. Race gallery

- Large selectable race cards
- Stable media key derived from the server race slug
- Race description, size, ability modifiers, notable racial features, and
  level adjustment where relevant
- Locked state only when explicitly emitted by the server
- Unlock cost and reason when product policy chooses to reveal locked options
- Keyboard, touch, reduced-motion, and screen-reader equivalents
- Explicit confirmation matching the current MUD flow

### 6. Class gallery

- Cards filtered by the authoritative selected race and account unlocks
- Primary attributes, hit die, BAB, saves, role summary, and description
- Optional compare view
- Disabled/restricted reasons supplied by the server
- Explicit confirmation

### 7. Build and alignment

- Premade versus custom as two well-explained paths
- Alignment compass or grid containing only valid server choices
- Text labels and descriptions, not color alone
- Final core summary before the irreversible persistence boundary, if the MUD
  contract adds such a confirmation

### 8. Preferences and role-play choice

- Recommended settings bundle with an inspectable list
- Role-player, non-role-player, or decide later
- Optional role-play profile as its own later wizard

### 9. Handoff to play

- Server-confirmed save
- Short transition into the existing terminal/HUD play surface
- MOTD and any mandatory short-description setup remain accessible
- Terminal output is never discarded; it can be revealed for troubleshooting

## Architecture Options Considered

| Option | Initial effort | Long-term quality | Finding |
|--------|----------------|-------------------|---------|
| Parse terminal prompts in React or Node | Low | Poor | Reject except for a disposable prototype. Text, colors, wrapping, help output, and prompt wording will break it. |
| Duplicate creation rules and catalogs in TypeScript | Medium | Poor | Reject. Race/class/unlock/alignment behavior will drift from the game. |
| Add direct Node HTTP APIs against the MUD database | High | Risky | Reject. It duplicates authentication and bypasses descriptor, validation, player-file, and save behavior. |
| Replace the proxy with native MUD WebSockets | Very high | Unknown | Defer. Existing web ADR 0003 already requires a separate transport, security, operations, and rollback project. |
| Implement a complete GMCP module family now | High | Potentially good | Defer. Existing ADR 0002 requires module schemas, coexistence, parser, validation, fixtures, and migration design. |
| Add structured onboarding over current Telnet/MSDP and proxy | Medium | Good | Recommended. It reuses the tested transport and preserves terminal compatibility. |

## Recommended Technical Architecture

```text
Motion-rich React onboarding UI
  |  typed onboarding-action / onboarding-state JSON
  v
Existing Luminari Web `/ws` proxy
  |  runtime schema validation
  |  sensitive-input handling
  |  Luminari onboarding MSDP adapter
  v
Existing Telnet connection
  |  versioned structured onboarding state
  |  ordinary line input for initial action transport
  v
Luminari-Source `nanny()` state machine
  |  authoritative choices, validation, mutation, and save
  v
Existing account and character persistence
```

### Protocol shape

The exact field names need an ADR and tests, but the browser-facing state should
look conceptually like this:

```json
{
  "type": "onboarding-state",
  "version": 1,
  "flowId": "connection-local-id",
  "revision": 12,
  "mode": "character-creation",
  "screen": "race",
  "title": "Choose your race",
  "prompt": "Select a race to continue.",
  "sensitiveInput": false,
  "choices": [
    {
      "id": "human",
      "label": "Human",
      "enabled": true,
      "wireValue": "human",
      "mediaKey": "race/human",
      "summary": "Adaptable and skilled."
    }
  ],
  "selection": {
    "name": "Aelarin",
    "sex": "female"
  },
  "actions": ["select", "classic-terminal"]
}
```

This is the proxy-to-browser contract, not a requirement to build JSON by hand
inside C. The MUD-side payload can use bounded MSDP tables and arrays, and the
proxy can normalize that payload into this type.

Recommended protocol properties:

- Dedicated capability/version negotiation, not a hard-coded client-name
  check.
- Reserved control variable names that users cannot remap in the MSDP settings
  screen.
- A connection-local `flowId` and increasing `revision` so stale buttons are
  rejected after a state change.
- Stable screen and option IDs separate from display labels.
- Server-provided `wireValue` for the existing line-input path.
- Structured error codes plus human-readable text.
- Explicit sensitive-input state.
- Strict bounds on strings, choice counts, nesting, and total payload size.
- No password, password hash, private command, or secret in an emitted state.
- Clear/reset event on disconnect, character switch, and `CON_PLAYING`.

The source currently caps an MSDP variable at 16 KB. Full descriptions and an
entire catalog should not be packed into every state update. Use compact choice
lists and a selected-choice detail payload, or bounded catalog chunks. Images,
audio, and video must never be sent through MSDP.

### Browser actions

For the first implementation, a typed browser action can be validated against
the proxy's latest state and translated to the exact existing input line. The
MUD still validates it normally.

Examples:

- Select race card -> send the server-provided `wireValue`
- Confirm -> send `y`
- Reselect -> send `n`
- Choose alignment -> send its numeric value
- Submit account name -> send entered text

This minimizes changes to the MUD input path. A future bidirectional structured
protocol can be considered only if raw line translation becomes limiting.

### Compatibility behavior

- New web client plus new MUD: cinematic structured experience.
- New web client plus old/other MUD: classic terminal experience.
- Old Telnet client plus new MUD: current text menus.
- Protocol parse/version failure: show a recoverable warning and return to the
  terminal, without disconnecting the player.

This fallback matrix is essential because `luminariweb` currently presents
itself as compatible with multiple LuminariMUD-style servers.

## Luminari-Source Work

### Protocol and presentation adapter

Add a focused source module, for example:

- `src/systems/web_client/onboarding.c`
- `src/systems/web_client/onboarding.h`

If new source files are used, both `Makefile.am` and `CMakeLists.txt` must be
updated.

Responsibilities:

- Capability/version state for each descriptor
- Bounded structured payload builder
- Screen/state serializer
- Race, class, alignment, account-lobby, and role-play catalog builders
- Error/result emission
- Sensitive-input state emission
- Clear/fallback event
- No direct database ownership beyond calls to existing account/character
  functions

### State-machine integration

`nanny()` currently interleaves:

- Text output
- State changes
- Validation
- Data mutation
- Persistence

The safest implementation is not a second state machine. Add state-entry
helpers that render both the text presentation and structured presentation
from the same authoritative data.

The initial integration should cover:

- Account name and confirmation
- Existing/new password states
- Account menu and character summaries
- New character name and confirmation
- Sex
- Race list, detail, and confirmation
- Class list, detail, and confirmation
- Premade/custom
- Alignment
- Recommended preferences
- Role-play decision
- MOTD/character-menu handoff

Every direct state transition in this path needs an audit. Missing a transition
would leave the web screen stale even though the terminal continues correctly.

### Catalog and rule extraction

Expose read-only builders around the existing authoritative structures. Do not
move rules into the protocol layer.

Examples:

- `build_available_races(descriptor)`
- `build_available_classes(descriptor)`
- `build_available_alignments(descriptor)`
- `build_account_character_summaries(descriptor)`
- `build_roleplay_options(descriptor)`

Each builder should call the same existing lock and compatibility functions
used by `nanny()`.

### Persistence and edge cases

The MUD implementation must define and test:

- Account at character capacity
- Character name race between check and creation
- Duplicate active character
- Multiple descriptors on one account
- Site ban and creation lock
- Locked race/class changes during an active flow
- Save failure or unavailable database
- Disconnect before and after the alignment save boundary
- Partially created character recovery
- Add-existing-character ownership
- Deleted character behavior
- Mandatory short description before game entry

No web animation should conceal or reinterpret these results.

### Source tests

There is currently no focused automated suite for the full account/creation
state machine. Add production-linked CuTest coverage for:

- Choice catalog filtering
- Race/class/alignment compatibility
- Descriptor capability and revision handling
- Bounded serialization and escaping
- Sensitive-state mapping
- State transition emission
- Stale/invalid action rejection where applicable
- Payload size and malformed content

Database-mutating integration cases need an isolated development/test database,
unique fixtures, and cleanup. They must never point at production.

## Luminari Web Work

### Shared contracts

Create a separate shared module rather than expanding generic `MudState`, for
example:

- `shared/onboarding.ts`

It should define:

- Onboarding state and screen unions
- Session phase distinct from TCP/WebSocket connection status
- Choice/detail types
- Account-character summary types
- Sensitive-input state
- Browser action messages
- Server state/error messages
- Runtime type guards and size limits

The current browser `parseServerMessage()` performs only a shallow cast after
JSON parsing. Onboarding data includes identity and UI instructions, so the new
contract needs real runtime validation.

### Telnet parser and session

Changes are needed in:

- `server/telnet-parser.ts`
- `server/mud-session.ts`
- `server/index.ts`
- `shared/mud.ts`

Responsibilities:

- Report Telnet ECHO/sensitive-input negotiation changes to `MudSession`
- Recognize reserved onboarding MSDP payloads
- Validate and normalize payloads before sending them to React
- Track current flow/revision for typed actions
- Clear onboarding state on reconnect/disconnect
- Use a separate, deliberate rate limit for onboarding submissions if needed
- Preserve current command throttling and destination/origin protections
- Never log submitted credentials or onboarding free text

### Critical password safety gap

The proxy currently accepts Telnet ECHO negotiation but does not tell React
that input is sensitive. React always uses the ordinary command field, and the
normal dispatch path includes command history, aliases, and triggers.

Before presenting web login as a first-class feature:

- The input must become `type="password"` during sensitive states.
- The value must be cleared immediately after send.
- Sensitive values must bypass command history.
- Sensitive values must bypass aliases and triggers.
- Sensitive values must not be exported or persisted.
- Sensitive values must not be cached by the service worker.
- Sensitive values must not enter logs, analytics, test snapshots, or error
  details.
- Telnet ECHO and structured `sensitiveInput` should be cross-checked, with a
  fail-safe preference for hiding input.

This is Phase 0/1 work, not polish.

### React feature structure

Suggested extraction:

```text
src/features/onboarding/
  OnboardingShell.tsx
  AccountLoginScreen.tsx
  AccountLobbyScreen.tsx
  CharacterNameScreen.tsx
  RaceGallery.tsx
  ClassGallery.tsx
  BuildChoiceScreen.tsx
  AlignmentScreen.tsx
  PreferencesScreen.tsx
  RoleplayDecisionScreen.tsx
  RoleplayProfileScreen.tsx
  CreationSummary.tsx
  onboarding-reducer.ts
  media-manifest.ts
  onboarding.css
```

The reducer controls presentation of the latest server revision. It does not
calculate available classes, valid alignments, unlocks, or save success.

The top-level app should select among:

- Connection
- Classic terminal login
- Structured account/login
- Account lobby
- Structured creation/profile
- Selected-character menu
- Playing

### Frontend tests

Add:

- Runtime contract tests
- Reducer tests for new revision, stale revision, reconnect, and fallback
- Component tests for every core screen
- Password-mode history/automation regression tests
- Keyboard and focus tests
- Reduced-motion behavior tests
- Responsive checks at desktop, 390 px, and 360 px
- Browser end-to-end tests through a synthetic Telnet server
- Version-skew and malformed-payload tests

The current Node test suite provides useful parser and proxy patterns, but a
multi-screen React flow will benefit from a browser/component test layer.

## Media and Motion Workstream

### Recommended visual strategy

Start with a reusable art system:

- A small set of animated environment backdrops
- One emblem/silhouette or portrait per live race
- One emblem/hero image per selectable base class
- Shared particle, fog, light, rune, and parallax layers
- CSS or Web Animations for interface motion
- Short, non-blocking transitions between major steps

Layered still art with restrained motion can feel rich while loading far less
data than full-screen video.

### Media manifest

The complete production checklist is maintained in
[manifest.md](manifest.md).

The web repository should own a versioned media manifest keyed by stable server
IDs:

```ts
{
  "race/human": {
    "poster": "...",
    "layers": ["...", "..."],
    "alt": "Human adventurers overlooking Ashenport"
  },
  "class/wizard": {
    "poster": "...",
    "accent": "arcane",
    "alt": "A wizard shaping a field of blue-white runes"
  }
}
```

The MUD owns the semantic key. The web client owns file URLs, crops, formats,
animation layers, alt text, and presentation.

Missing media must fall back to a generic race/class card without breaking the
flow.

### Performance requirements

- Responsive AVIF/WebP with a PNG/JPEG fallback where needed
- Multiple crop sizes for desktop and mobile
- Lazy-load non-current race/class art
- Preload only the current and likely-next screen
- Poster/placeholder before animated layers
- No image bytes inside WebSocket/MSDP messages
- Explicit initial-load and per-screen byte budgets
- Versioned static caching, with old media removable on deploy
- Pause expensive animation in background tabs
- Respect data-saver signals where practical

### Accessibility requirements

- `prefers-reduced-motion` must replace major transitions, not merely shorten
  them.
- Every visual choice needs a normal button/list representation.
- Keyboard order and visible focus must remain clear.
- Selection cannot depend on animation or color.
- Decorative images need empty alt text; meaningful images need authored alt
  text.
- Audio is opt-in, muted by default, separately controllable, and never
  required to understand a choice.
- Screen-reader status announcements should describe server validation and
  progression without replaying decorative copy.

### Art and licensing

Before producing dozens of assets, the autonomous pipeline derives and
validates:

- Art direction and visual canon
- Aspect ratios and safe text zones
- Race/class naming and stable media keys
- Source/licensing record for every asset
- The manifest policy for generative assets and their automated checks
- Representation standards across races and genders
- Machine-readable acceptance thresholds and evidence
- Replacement/update workflow

Missing canon evidence is not a human gate. The pipeline uses the generic
manifest fallback, records the evidence gap, and continues. Content,
representation, similarity, license, composition, contrast, responsive, and
reduced-motion checks either accept an asset or trigger regeneration,
substitution, or quarantine.

A bespoke illustration for every live race and selectable class can easily
become the largest schedule item. A reusable backdrop plus emblem/silhouette
system is the recommended first release.

## Security and Privacy

### Keep authentication inside the MUD

The Node proxy should not query `account_data`, compare password hashes, or
create its own authenticated cookies/tokens for this feature. Doing so would
create a second security boundary and duplicate:

- Name validation
- Password verification and bad-attempt policy
- Site bans and creation locks
- Account unlocks
- Character ownership
- Duplicate-login handling
- Save and player-file behavior

The live MUD descriptor is the authenticated session.

### Transport

HTTPS/WSS protects browser-to-proxy traffic. The proxy-to-MUD leg is ordinary
Telnet TCP today. A first-party deployment should keep that hop on loopback or
a trusted private network, or secure it with an authenticated, encrypted tunnel
whose configuration passes automated transport tests.
Credentials should not cross an untrusted network as plain Telnet.

Modernizing the MUD's legacy password hashing is worthwhile but is a separate
security migration. This UI project must not silently change password formats
without a migration and rollback plan.

### Personal data

Structured onboarding introduces account names, character names, and character
lists into typed proxy messages. Requirements:

- No default payload logging
- No live account/character data in fixtures
- Sanitized error messages
- No terminal transcript persistence
- No service-worker caching for `/api` or `/ws`
- No analytics on entered names or role-play free text
- Clear memory/state on disconnect and account switch

### Password recovery is separate

A "full web account experience" often implies email verification, forgotten
password recovery, MFA, account settings, and long-lived web sessions. None of
that exists in the current browser client, and the current MUD login flow does
not provide it.

Those features should be scoped as a separate authentication project rather
than being implied by the visual login screen.

## Delivery Plan

The phases below are deliberately vertical. Each should leave the classic
terminal usable.

### Phase 0: Contract and safety - 1-2 person-weeks

- [ ] Apply the autonomous MVP defaults defined below
- [ ] Write onboarding protocol ADR
- [x] Define capability, version, revision, fallback, and size rules
- [x] Define password-sensitive behavior
- [ ] Encode locked-choice visibility from the default below
- [ ] Encode back-navigation behavior from the default below
- [ ] Establish feature flags in both repositories
- [x] Define synthetic fixtures and privacy rules
- [ ] Define art direction and media-key convention

Exit criteria: a machine-validated contract and test fixtures represent
account login, account lobby, race choice, errors, and fallback without live
private data.

### Phase 1: Secure structured vertical slice - 3-5 person-weeks

- [x] Add Telnet sensitive-input callback to the proxy
- [x] Prevent password history, aliases, triggers, persistence, and logs
- [x] Add MUD capability and bounded state emission
- [x] Add proxy runtime validation and typed WebSocket messages
- [x] Render account login/new-account forms
- [x] Render account lobby and existing-character cards
- [x] Select a character and hand off to the current MOTD/menu/play flow
- [x] Preserve classic terminal fallback
- [x] Add source, proxy, and browser tests

Exit criteria: an existing account can log in, choose a character, and enter
the game through the structured UI without exposing a password to browser
history or breaking a classic Telnet client.

### Phase 2: Core cinematic character creator - 4-7 person-weeks

- [x] Name and confirmation
- [x] Sex selection
- [x] Race catalog, detail, lock state, art, and confirmation
- [x] Class catalog, compatibility, detail, art, and confirmation
- [x] Premade/custom explanation
- [x] Alignment selection
- [x] Persistence-result handling
- [x] Recommended preferences
- [x] Role-play decision
- [x] Responsive and reduced-motion behavior
- [x] End-to-end creation fixtures

Exit criteria: a new character can complete the current core server path using
only server-emitted options and can enter the existing game surface.

### Phase 3: Role-play identity suite - 2-4 person-weeks

- [ ] Generated short-description flow
- [ ] Long description and background editors
- [ ] Background archetype
- [ ] Age
- [ ] Homeland, faction, hometown, and deity
- [ ] Goals, personality, ideals, bonds, and flaws
- [ ] Forced short-description gate
- [ ] One-time-selection and server-error handling

Exit criteria: all current character-options-menu functions have a structured
web presentation or a deliberate terminal fallback.

### Phase 4: Media polish and content completion - 3-6 engineering person-weeks

- [ ] Final media manifest and fallback coverage
- [ ] Animation system and reduced-motion equivalents
- [x] Asset preloading and cache policy
- [x] Optional audio controls
- [ ] Final copy edit and mechanical summaries
- [x] Mobile crops and performance budgets
- [x] Licensing and attribution record
- [ ] Automated visual QA across the live catalog

Art production runs in parallel and is not included in the engineering range.

### Phase 5: Hardening and rollout - 2-4 person-weeks

- [ ] Version-skew and malformed-protocol testing
- [ ] Reconnect and partial-creation recovery
- [ ] Load, rate-limit, and payload-size testing
- [ ] Automated security and privacy test suites
- [ ] Automated accessibility conformance suite
- [ ] Deployment and rollback runbook
- [ ] Feature-flagged development rollout
- [ ] Automated canary rollout with health thresholds and rollback triggers
- [ ] Classic-terminal rollback drill
- [ ] Automatic documentation promotion after all checks pass

## Estimate Summary

| Deliverable | Engineering | Media/content |
|-------------|-------------|---------------|
| Secure login plus account lobby vertical slice | 4-7 person-weeks including Phase 0 share | Minimal branded backdrop and character-card fallback |
| Core race/class creator through role-play decision | 8-14 cumulative person-weeks | Reusable visual system plus accepted race/class key art |
| Full optional role-play suite | 11-17 cumulative person-weeks | Additional location, faction, deity, and background art as desired |
| Production-polished full experience | 15-28 cumulative person-weeks | 4-8 artist-weeks reusable system; 10-20+ bespoke system |

With two engineers who can work concurrently across the C and TypeScript
boundaries, plus design/art support, a realistic calendar target for the full
experience is roughly 10-16 weeks after decisions and assets begin. A single
engineer doing both repositories serially is more likely a 4-7 month project.

## Primary Risks

| Risk | Impact | Mitigation |
|------|--------|------------|
| `nanny()` is a large interleaved switch | Missed transition produces stale UI | State-entry emitters, transition audit, source tests, terminal fallback |
| Browser duplicates server rules | Incorrect choices and save failures | Server emits all availability and validation |
| Password stays on ordinary command path | Credential exposure in UI/history/automation | Sensitive-input work before public login UI |
| Version skew between repositories | Blank or broken onboarding | Capability/version negotiation and fail-to-terminal behavior |
| Arbitrary Back navigation | Invalid dependent state or rollback bugs | Match current confirmation semantics first; scope rollback separately |
| Save occurs mid-onboarding | Confusing reconnect/partial characters | Explicit persistence status and recovery contract |
| MSDP payload exceeds 16 KB | Truncation or parser failure | Compact lists, selected detail, chunks, hard limits |
| Runtime catalogs differ from static art | Missing/wrong card imagery | Stable media keys plus generic fallback |
| Bespoke art scope dominates schedule | Delayed release | Reusable motion/backdrop system first |
| Motion harms accessibility/performance | Unusable mobile/low-motion experience | Reduced-motion alternative, byte budgets, lazy load |
| Plain Telnet proxy hop | Credential exposure on untrusted networks | Loopback/private hop or an authenticated encrypted tunnel that passes transport tests |
| Product implies modern account recovery | Scope and security expansion | Explicitly keep recovery/MFA/session work separate |

## Autonomous Defaults Applied Before Implementation

These defaults are authoritative unless repository evidence requires a safer
behavior. The pipeline records such evidence and applies the safe alternative
without waiting for sign-off.

1. **MVP boundary**
   Recommended: secure login, account lobby, existing-character selection, and
   core creation through role-play decision. Defer the full role-play suite.
2. **Locked races and classes**
   Recommended: show a locked card only when the MUD explicitly emits it with
   cost/reason; otherwise preserve current hidden behavior.
3. **Back navigation**
   Recommended: mirror current confirm/reselect behavior. Do not promise a
   universal Back button in the first release.
4. **Art depth**
   Recommended: reusable animated scenes plus one emblem/silhouette per
   race/class. Generate bespoke full illustrations incrementally.
5. **Audio**
   Recommended: omit from MVP. If added, make it opt-in and muted by default.
6. **Custom avatar**
   Recommended: do not add persistent avatars initially. Use race/class art.
   Persistent portraits require a new player-profile schema and moderation
   policy.
7. **Other MUD servers**
   Recommended: Luminari structured UI only, automatic classic-terminal
   fallback everywhere else.
8. **Account recovery**
   Recommended: explicitly out of scope for this project.
9. **Character linking, password change, and deletion**
   Recommended: keep terminal fallback in the first vertical slice, then add
   structured versions after core login/creation is stable.
10. **Server confirmation before save**
    Recommended: add only if the MUD also adds an authoritative pre-save
    confirmation state. Do not create a browser-only confirmation that suggests
    unsupported editing/backtracking.

## Acceptance Criteria for the Full Project

- [ ] Classic Telnet login and creation behavior still works.
- [ ] Old/other MUDs automatically use the web terminal flow.
- [ ] The MUD is the only authority for options, restrictions, validation, and
      save success.
- [ ] Passwords never enter history, aliases, triggers, storage, cache, logs,
      analytics, snapshots, or fixtures.
- [x] Account and character data is runtime-validated at the proxy boundary.
- [x] Every onboarding message is versioned and bounded.
- [x] Stale UI actions cannot mutate a later state.
- [x] Reconnect and disconnect clear private UI state.
- [ ] Core creation covers every current server step through role-play choice.
- [ ] Optional role-play screens either have structured support or a deliberate
      terminal handoff.
- [x] Missing media has a functional fallback.
- [x] Desktop, 390 px, and 360 px layouts pass.
- [ ] Keyboard-only and screen-reader flows pass.
- [x] Reduced-motion mode avoids major motion and preserves meaning.
- [x] Media stays within agreed load budgets.
- [ ] Source tests, web tests, lint, build, and integration tests pass.
- [ ] Development rollout, production feature flag, and rollback are
      documented and rehearsed.

## Recommended Next Step

Start Phase 0 with the autonomous MVP and art defaults above. The first
implementation artifact is the protocol ADR plus synthetic examples for these
five screens:

1. Sensitive account password
2. Account lobby with two characters
3. Race selection
4. Server validation error
5. Protocol fallback to terminal

Those fixtures will make the cross-repository boundary concrete before either
the MUD state machine or the React UI is substantially changed.
