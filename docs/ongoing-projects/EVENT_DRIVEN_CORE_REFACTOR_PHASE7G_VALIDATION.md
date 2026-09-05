# Event-Driven Core Refactor Phase 7G Point-Update Validation

**Status:** Pass
**Date:** 2026-08-31
**Branch:** `event-driven-core-refactor`
**Scope:** Seventh Phase 7 slice, mixed mud-hour point-update work

## Specification Audit

| Requirement | Disposition |
|---|---|
| Deadline ownership | One service-owned event is aligned to each 75-second mud-hour boundary. It marks one dispatch due rather than owning gameplay pointers or creating one event per player/object. |
| Genuine global work | Happy-hour and staff-event maintenance remain one global phase. Minute-based activated-item recharge and DG trigger-local pending damage were not part of `point_update()` and retain their established cadence and owner. |
| Player ownership | Every PC in `character_list` is linked into an intrusive registry. NPC condition work was already a no-op and is omitted; autonomous NPC activity remains owned by the accepted active-world and character-periodic systems. |
| Object ownership | Only live objects with positive ordinary/spec timers, timer triggers, imbued-missile state, decay, or corpse state are linked. Dormant objects receive no mud-hour visit. |
| Gameplay parity | The established point-update helpers still own hunger, thirst, sobriety, inventory timers, artifact burn, idle void/rent, special cooldown recovery messages, missile expiry, timer triggers, portal/item decay, and corpse decay/content release. |
| Cadence and ordering | The service uses the exact shared mud-hour boundary. Weather and DG time triggers still run first; point work remains global, then players, then objects. A delayed scheduler budget dispatches the due point work once without catch-up bursts. |
| Lifecycle | Character create, disk-loaded player entry/copyover, extract/direct-free, and object create/load/place/timer/flag/trigger mutation, OLC propagation, unique-item editing, DG transformation, extraction/direct-free, and shutdown update ownership directly. Current-owner extraction cannot invalidate iteration. |
| Bounds and failure | Scheduled mode owns one queue entry regardless of owner population. Registries allocate no per-owner event or heap node and are bounded by existing live PC/object populations; no lossy admission ceiling is needed. Mandatory service admission failure selects legacy mode for the whole subsystem. |
| Diagnostics | `perfmon entities` reports mode, player/object membership, validation mismatch, service/dispatch callbacks, and executed owner work on five labeled rows. Every row is tested at no more than 80 columns. |
| Rollback | `LUMINARI_POINT_UPDATE_EVENTS=legacy` exclusively restores the former whole-list heartbeat traversal. Scheduled and legacy paths never execute together. |

## Gameplay Boundary

This slice changes how the game finds players and objects that need mud-hour
work. It does not change hunger, thirst, sobriety, idle rent, item cooldowns,
imbued arrows, timer scripts, decaying portals/items, or corpse behavior.

All players remain eligible, whether actively commanding or idle. Objects
sleep only when they have no mud-hour responsibility. Giving an object a timer,
adding a timer trigger, marking it for decay, creating a corpse, or starting a
legacy item cooldown admits it immediately. Clearing its final responsibility
removes it after the current phase. Cross-object traversal order is no longer
a gameplay contract, but each object retains its internal timer-trigger-decay
order and equal deadlines remain deterministic.

## Focused Coverage

The production-linked suite proves:

- one exact aligned service event and no side effect before explicit dispatch;
- global, player-condition, ordinary timer, and special-timer execution once
  per mud hour;
- dormant object exclusion and automatic removal after the final timer expires;
- direct mutation admission, editor-copy exclusion, registry validation, and
  exclusive legacy rollback;
- registration of disk-loaded players at the shared live-entry boundary;
- extraction-safe iteration while multiple current decay objects free
  themselves;
- startup fallback when the mandatory service event cannot be admitted; and
- required diagnostic labels with each point-update row within 80 columns.

## Validation Evidence

The final Phase 7G tree passed all of the following gates:

- warning-clean CMake production and test builds and all 993
  production-linked C tests;
- the authoritative Autotools `make test-all` gate, including deployment,
  supervision, protocol, memory, world-tooling, rename, and install checks;
- all 993 tests against a disposable `luminari_test` MariaDB database;
- eight direct syntax boots covering scheduled/legacy point work,
  scheduler/legacy event backends, and libevent/select I/O drivers;
- AddressSanitizer and UndefinedBehaviorSanitizer with all 993 tests;
- strict Valgrind with all 993 test entries, zero errors, and no definite,
  indirect, or possible leaks; and
- a logged live scheduler/libevent session on port 4101 as Ornir at level 34,
  using an 80-column client, observing one service callback, one dispatch, and
  one player execution with zero registry mismatches, inspecting `perfmon
  entities`, restoring preferences, quitting, and shutting down cleanly.

The first live pass exposed that disk-loaded PCs did not travel through the
constructor registration hook. The shared `enter_player_game()` path now
registers them for normal login and copyover recovery. A second live pass
proved `players=1` and `work: players=1` for connected Ornir. Its widest
point-update diagnostic row was 35 columns. The temporary local-test login
credential was never logged, and the original account hash was restored
byte-for-byte before the harness succeeded.

The final nested Valgrind gate also exposed five room trail-list headers that
were allocated in every world mode but freed only in wilderness mode 2. Room
cleanup now releases that room-owned list unconditionally. The repeated nested
syntax boot and parent suite both report zero Valgrind errors and zero definite,
indirect, or possible leaks.

Logs are retained under `.ci-runtime/phase7g-*`. Disposable database and live
runtime state were restored or removed after validation.

## Rollback And Next Slice

Restart with `LUMINARI_POINT_UPDATE_EVENTS=legacy` to restore the complete old
point-update heartbeat path. Live mode switching is unsupported.

The next tranche adds a dedicated immortal event-debug command with readable,
paginated 80-column output and an explicit 120-column ceiling. Phase 8
encounter-level combat compatibility follows that observability gate.
