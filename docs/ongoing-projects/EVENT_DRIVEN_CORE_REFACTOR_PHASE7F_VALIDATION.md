# Event-Driven Core Refactor Phase 7F Vessel Validation

**Status:** Pass
**Date:** 2026-08-31
**Branch:** `event-driven-core-refactor`
**Scope:** Sixth Phase 7 slice, vessel and converted RoL ship periodic work

## Specification Audit

| Requirement | Disposition |
|---|---|
| Vessel ownership | Every valid Greyhawk vessel owns at most one nearest-deadline event. The registry is capped at all 501 fleet slots and refills released capacity from registered owners. |
| Genuine global work | One service event owns vessel event reconciliation, trade restocking, MSDP refresh, and merchant reconciliation. No global vessel list is scanned merely to discover due per-vessel work. |
| Fixed RoL ships | Each loaded canonical RoL hull owns one 2.5-second event. Object create/extract hooks synchronize the seven fixed definitions directly, replacing the former `object_list` search. |
| Gameplay parity | Owner callbacks invoke extracted one-vessel forms of the established autopilot, hunter, combat, crew, upkeep, narrative, weather, encounter, and schedule routines. The service callback invokes the established global routines. |
| Cadence | Fast work remains aligned to the shared 0.5-second boundary, RoL movement to 2.5 seconds, and schedule/merchant work to the 75-second mud-hour boundary. Narrative, hazard, and encounter counters retain their former half-second tick basis. |
| Ordering | One vessel executes its due routines in the former subsystem order. Different vessel owners interleave deterministically by scheduler deadline and insertion sequence rather than in global subsystem-wide passes. The routines have no contract requiring cross-vessel pass ordering. |
| Lifecycle | Spawn, database reconstruction, edit replacement, sinking, purge, hunter/merchant/event retirement, reset relinking, object extraction, feature-toggle changes, shutdown, and generation reuse synchronize or cancel ownership directly. In-flight owner retirement prevents later routines in that callback from touching the retired hull. |
| Bounds and failure | Admission failure is counted and logged at a bounded rate. A released owner slot admits a waiting registered vessel. Generation exhaustion fails closed. If the event backend or mandatory service event is unavailable at boot, the whole vessel subsystem selects the legacy heartbeat rather than running a partial scheduled mode. |
| Diagnostics | `perfmon entities` reports mode, registry, validation, capacity, owner/service callbacks, fixed-RoL state, and fast/schedule work as labeled rows. Each vessel row is no wider than 80 columns. |
| Rollback | `LUMINARI_VESSEL_EVENTS=legacy` restores the former Greyhawk, global-service, schedule, merchant, and converted-RoL heartbeat paths as one exclusive boot-time selection. |

## Gameplay Boundary

This slice changes how the game finds a vessel whose work is due. It does not
change navigation, ship combat, crew pay, wear, trade, weather, encounters,
public schedules, merchant fleets, occupant messages, or client vessel state.
A ship continues moving and fighting without a player aboard because validity,
not player proximity, controls owner eligibility.

The old heartbeat ran one subsystem across the whole fleet before moving to
the next subsystem. Scheduled mode runs the same ordered routine chain for one
due hull at a time. Equal deadlines are deterministic, and every hull receives
the same boundary and exactly one execution. Cross-hull pass order is therefore
an implementation detail, while each hull's gameplay order remains preserved.

## Focused Coverage

The production-linked suite proves:

- exact half-second and mud-hour boundaries, including weapon reload cadence;
- one owner event per live vessel and exclusive scheduled/legacy execution;
- sink/extract cancellation, slot reuse, and stale-generation rejection;
- admission rejection with a one-owner limit followed by automatic refill;
- live vessel-feature disable/re-enable with immediate cancellation and rebuild;
- whole-subsystem startup fallback when the event backend is unavailable;
- direct lifecycle synchronization at every production spawn, load, replace,
  relink, sink, purge, and retirement boundary;
- fixed-RoL direct object lifecycle hooks, owner scheduling, callback counts,
  and registry validation; and
- complete vessel diagnostic labels with every row within 80 visible columns.

## Validation Evidence

The final Phase 7F tree passed all of the following gates:

- warning-clean CMake production and test builds and all 987
  production-linked C tests;
- the authoritative Autotools `make test-all` gate, including deployment,
  supervision, protocol, memory, world-tooling, help, rename, and install
  checks;
- all 987 tests against a disposable `luminari_test` MariaDB database;
- eight direct syntax boots covering scheduled/legacy vessel work,
  scheduler/legacy event backends, and libevent/select I/O drivers;
- AddressSanitizer and UndefinedBehaviorSanitizer with all 987 tests;
- strict Valgrind with all 987 test entries, zero errors, and no definite,
  indirect, or possible leaks; and
- a logged live scheduler/libevent session on port 4101 as Ornir at level 34,
  including an 80-column client setting, `perfmon entities`, preference
  restoration, in-game quit, and clean server shutdown.

The reduced local world contains no vessel prototypes, persisted vessels, or
fixed RoL hull objects. Its live report therefore correctly showed zero vessel
owners and zero registry mismatches; the vessel block's widest visible row was
35 columns. Real owner work is exercised by the production-linked tests, which
construct valid vessels and run the actual callbacks and lifecycle hooks.

Logs are retained under `.ci-runtime/phase7f-*`. Disposable database state and
temporary authentication state were restored or removed after validation.

## Rollback And Next Slice

Restart with `LUMINARI_VESSEL_EVENTS=legacy` to restore all vessel periodic
heartbeat work. Scheduled and legacy paths never run together, and live mode
switching is unsupported.

The next Phase 7 slice decomposes the mixed mud-hour `point_update()` scan into
real owner deadlines and genuinely global service work.
