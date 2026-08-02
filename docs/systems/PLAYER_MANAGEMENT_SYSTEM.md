# LuminariMUD Player Management System

Last verified: 2026-07-29

## Scope

Player management is an account-first, descriptor-driven workflow. The
`nanny()` state machine in `src/interpreter.c` owns account authentication,
character selection, character creation, the selected-character menu, and the
transition into play. `src/account.c` owns account persistence and account-wide
unlocks. `src/players.c` owns character loading, character saving, and the
player index.

The classic Telnet menus and the structured web experience are two
presentations of this same workflow. The web adapter does not authenticate
accounts or implement character rules. See
[WEB_ONBOARDING_SYSTEM.md](WEB_ONBOARDING_SYSTEM.md) for that presentation
layer.

## Data Ownership

| Data | Runtime owner | Durable owner |
| --- | --- | --- |
| Account name, password hash, email, experience | `struct account_data` | MariaDB `account_data` |
| Account character membership | `account->character_names[]` | MariaDB `player_data.account_id` |
| Unlocked races and classes | `account->races[]`, `account->classes[]` | MariaDB unlock tables |
| Character state | `struct char_data` | ASCII player file written by `save_char_checked()` |
| Character lookup metadata | `player_table` | Checked player-index file |
| Equipment and inventory | Character/object runtime structures | Player object files |

MariaDB is required for the account flow. Character files remain the primary
character record; the `player_data` table also supplies account membership,
last-online data, and database-backed character metadata. For the wider
persistence map, see [SAVE_SYSTEMS_BREAKDOWN.md](SAVE_SYSTEMS_BREAKDOWN.md).

## Account Model

`struct account_data` is declared in `src/structs.h`. Its active fields include:

- database ID and account name;
- password hash and failed-password count;
- up to `MAX_CHARS_PER_ACCOUNT` character names (currently 100);
- account experience;
- arrays of unlocked race and class IDs;
- optional email;
- account-level quit-survey completion.

`load_account()` loads the base row, character membership, and unlock tables.
`save_account()` upserts the account, updates character membership, saves
unlocks, and refreshes active descriptors that share the account ID.

An email field is not an email-authentication system. The login workflow does
not implement email verification, forgotten-password recovery, MFA, web
sessions, or long-lived authentication tokens.

## Connection and Account Flow

`init_descriptor()` starts a connection in `CON_GET_PROTOCOL` when protocol
negotiation is configured, otherwise in `CON_ACCOUNT_NAME`. Protocol
negotiation eventually returns to `CON_ACCOUNT_NAME`.

| Stage | State or states | Server behavior |
| --- | --- | --- |
| Protocol negotiation | `CON_GET_PROTOCOL` | Telnet capability negotiation and greeting |
| Account identity | `CON_ACCOUNT_NAME` | Validates the entered name and tries `load_account()` |
| New-account confirmation | `CON_ACCOUNT_NAME_CONFIRM` | Spelling, ban, and lock checks |
| Existing-account password | `CON_PASSWORD` | Checks the password and failed-attempt limit |
| New-account password | `CON_NEWPASSWD`, `CON_CNFPASSWD` | Creates and confirms password |
| Account lobby | `CON_ACCOUNT_MENU` | Selects, creates, links, or quits |
| Link character | `CON_ACCOUNT_ADD`, `CON_ACCOUNT_ADD_PWD` | Ownership/password checks |
| Load character | `CON_RMOTD`, `CON_MENU` | Duplicate/restriction checks; character menu |
| Enter the world | `CON_PLAYING` | Starts normal command processing |

The account menu loads character records to display level, race, and class
summary. A deleted character is not playable. Selecting a character also runs
duplicate-session checks before reaching the MOTD and character menu.

Linking an existing character is a distinct ownership flow:

- a character already associated with the same account can be restored without
  another password challenge;
- a character associated with a different account is refused;
- an unassociated legacy character requires its character password;
- full accounts, missing characters, and deleted characters are refused.

Password change and character deletion are selected-character operations under
`CON_MENU`; they are not account-lobby operations.

## Password Input

Transitions into password entry use `ProtocolNoEcho()` to control Telnet echo,
and their handlers restore echo when appropriate. The principal account,
legacy-link, password-change, and deletion entry points clear already queued
input so a command typed ahead of a sensitive prompt is not consumed as the
password.

These controls do not encrypt Telnet traffic. A browser gateway must also mask
and isolate sensitive input, and the gateway-to-MUD leg must stay on a trusted
path or use an authenticated encrypted tunnel. The password format is legacy
and must not be changed as part of a presentation-only feature without a
separate migration and rollback plan.

## Character Creation Flow

The current new-character sequence is:

| Order | State | Authoritative behavior |
| ---: | --- | --- |
| 1 | `CON_GET_NAME` | Parses and validates the name, checks reserved words, and checks uniqueness |
| 2 | `CON_NAME_CNFRM` | Confirms spelling; repeats ban, lock, and duplicate checks |
| 3 | `CON_QSEX` | Accepts male or female |
| 4 | `CON_QRACE` | Selects an available, account-unlocked player race |
| 5 | `CON_QRACE_HELP` | Shows race details and confirms or reselects |
| 6 | `CON_QCLASS` | Selects an available base class compatible with the race and account unlocks |
| 7 | `CON_QCLASS_HELP` | Shows class details and confirms or reselects |
| 8 | `CON_CONFIRM_PREMADE` | Selects a guided premade build or a custom build |
| 9 | `CON_QALIGN` | Selects an alignment valid for both race and class |
| 10 | Persistence boundary | Initializes and saves; links account; updates player index |
| 11 | `CON_SETPREFS` | Optionally applies the recommended preference bundle |
| 12 | `CON_CHAR_RP_DECIDE` | Selects role-player, non-role-player, or decide later |
| 13 | `CON_RMOTD`, `CON_MENU` | Shows the MOTD and selected-character menu |

The point-buy block is compiled out by `CHARGEN_NO_STATISTICS` in
`src/interpreter.c`. Ability scores, feats, skills, and level-one study are not
part of the active initial creation workflow.

Navigation follows the state machine's explicit confirmation and reselection
paths. There is no general Back operation. Adding one requires server-side
rollback rules for dependent race, class, alignment, unlock, and persistence
state; a client must not simulate it by editing a local draft.

## Choice Authority

Presentation code must consume the same source data and filters as the terminal
workflow:

- races come from `race_list[]`, player-race flags, `is_locked_race()`, and
  `has_unlocked_race()`;
- classes come from `class_list[]`, `CLSLIST_INGAME()`,
  `CLSLIST_PRESTIGE()`, `CLSLIST_LOCK()`, `has_unlocked_class()`, and
  `valid_class_race_alignment()`;
- alignments are filtered and validated with `valid_align_by_class()` and
  `valid_align_by_race()`;
- account cards are built by loading the account's actual character records.

Display labels are not stable protocol identifiers. External presentations use
explicit stable IDs and media keys while continuing to send the exact wire
values accepted by `nanny()`.

## Core Persistence Boundary

After a valid alignment is selected, `nanny()`:

1. creates a player-index entry when required;
2. calls `init_char()`;
3. copies the account password hash to an account-created character's legacy
   password slot;
4. adds the character name to the in-memory account;
5. calls `save_char()`;
6. ensures a `player_data` row exists when MariaDB is available;
7. calls `save_account()` and `save_player_index()`.

The recommended-preference and role-play-decision steps happen after this
boundary. The role-play decision saves the character again, and `CON_RMOTD`
performs a further safety save.

This timing matters on disconnect:

- before alignment completes, the character is an unpersisted draft;
- after alignment, a character may exist even though preferences and the
  role-play decision are incomplete;
- reconnect recovery follows the persisted account/character state, not a
  browser-side wizard draft.

The core alignment path uses legacy result-discarding save wrappers. A
presentation's core `persistence` value therefore identifies the state-machine
boundary; it is not a checked write receipt. Code that must promise durable
success should use checked save functions and explicit rollback, as the
protocol-v2 role-play commit path does.

## Role-Play Profile

After core creation, a player may skip, defer, or enter the role-play profile.
The character menu also exposes the profile later. It includes:

- generated short description;
- long description and background story;
- background archetype;
- goals, personality, ideals, bonds, and flaws;
- age;
- homeland region;
- faction;
- hometown;
- deity.

Most profile data is optional. When `CONFIG_USE_INTRO_SYSTEM` is enabled,
entering the world is blocked until the required short-description components
exist.

Background, age, homeland, faction, hometown, and deity selections have
domain-specific mutability rules. Their handlers use descriptor-local pending
values and checked commit helpers so a rejected or failed save can restore the
previous character and player-index state. The seven free-text fields share
field IDs, field-specific byte limits, UTF-8 validation, normalization, and
`save_char_checked()` through `src/roleplay.c`.

## Checked and Legacy Save APIs

`src/players.c` exposes both:

```c
bool save_char_checked(struct char_data *ch, int mode);
void save_char(struct char_data *ch, int mode);
```

`save_char_checked()` reports allocation, file, flush, close, and index
failures. `save_char()` is the legacy wrapper used by call sites that do not
consume a result. The player index follows the same pattern with
`save_player_index_checked()` and `save_player_index()`.

Use the checked functions when the caller:

- must not report success before durable storage succeeds;
- has already mutated state that must be rolled back on failure;
- is accepting structured input whose client expects an explicit result.

## Validation and Access Controls

The workflow includes:

- account and character name parsing, length checks, fill-word checks, reserved
  word checks, and character-name uniqueness checks;
- failed-password attempt limits;
- new-account and new-character site-ban checks;
- creation and staff-only restriction checks;
- duplicate active-character checks;
- account ownership checks when linking a character;
- confirmation before character deletion.

The MUD descriptor is the authenticated session. A web gateway must not query
account tables or compare hashes on the MUD's behalf; doing so would create a
second authentication and ownership boundary.

## Maintenance Rules

When changing account or character creation:

1. Trace every affected `nanny()` state and transition.
2. Keep validation, unlocks, mutation, and persistence in source-owned domain
   code.
3. Update the structured presentation map or intentionally retain terminal
   fallback.
4. Preserve password no-echo and queued-input clearing on every sensitive path.
5. Re-evaluate the alignment persistence boundary and disconnect behavior.
6. Add production-linked CuTest coverage for positive, negative, boundary, and
   rollback behavior.
7. Test classic Telnet, structured v1, structured v2 where applicable, and
   version-skew fallback.

## Key Files

| File | Responsibility |
| --- | --- |
| `src/interpreter.c` | `nanny()` account, creation, character-menu, and play transitions |
| `src/account.c` | Account load/save, membership, unlocks, and account menu |
| `src/players.c` | Character files, checked saves, loads, and player index |
| `src/roleplay.c` | Role-play field authority, pending selections, checked commits |
| `src/char_descs.c` | Generated short-description workflow |
| `src/comm.c` | Descriptor lifecycle, input queue, and game-loop dispatch |
| `src/net/protocol.c` | Telnet negotiation, no-echo, MSDP parsing |
| `src/net/onboarding.c` | Structured presentation adapter |
| `src/structs.h` | Account, descriptor, character, and connection-state definitions |
