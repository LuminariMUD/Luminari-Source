# Event-Driven Core Refactor Phase 11h Validation

**Status:** Accepted 2026-08-31
**Date:** 2026-08-31
**Branch:** `event-driven-core-refactor`
**Scope:** Offline cooldown, timed-state, and recoverable-use continuity

## 1. Requirement Correction

Ability cooldowns and recoverable uses continue against real elapsed time while
a player is logged out. A player returning after the last deadline has no
cooldown event and has all uses available. Returning partway through a cooldown
restores only its remaining time. Multi-use abilities recover every elapsed use
and retain one next deadline without executing a callback storm at login.

This explicitly supersedes Phase 5's offline-pause choice. It applies to normal
logout, full reboot, and copyover recovery. Copyover usually subtracts only its
brief handoff interval and otherwise preserves the same deadline.

## 2. Versioned Event Records

All 93 persisted character-event policies now write schema 2 with the
`MUD_EVENT_OFFLINE_ELAPSE` policy. Restore:

- validates type, stable player owner, schema, timestamp, duration, payload,
  duplicate state, and scheduler admission;
- accepts schema 1 records and applies the corrected elapsed-time policy;
- subtracts ten scheduler pulses for each elapsed real second;
- expires one-shot records whose deadline passed;
- catches up `uses:N` recovery arithmetically using the ability's normal
  per-use interval, then admits at most one next event;
- clears the coupled `SPELLBATTLE` marker if that event expired offline; and
- creates fresh process-local owner and scheduler identity for every admitted
  record.

The timestamp-free `Evnt` rollback format remains readable and writable. It
cannot know how long it was offline, so the reader resumes its stored duration
and the next default save migrates it to schema 2. This limitation is explicit
and confined to the operator-selected rollback format.

## 3. Saved Counter Recovery

Some older abilities use integer counters advanced by the connected-player or
six-second character cadence rather than MUD events. Player files now include a
`CkAt` wall-clock checkpoint. Load converts complete six-second intervals and
advances only persisted timing state:

- mission, forage, scrounge, spiritual weapon, irresistible magic, quick cast,
  spell recall, retainer, vampire cloak, eidolon, and Kapak saliva six-second
  countdowns, plus Fight to the Death's seconds-based countdown;
- staggered domain and generic bonus-slot recovery;
- staggered moon bonus-spell use recovery; and
- full-refresh Efreeti, dragon, pixie dust, laughing touch, fleeting glance,
  fey shadow walk, grave touch, grasp of the dead, and incorporeal form pools.

No periodic gameplay function is replayed. Login catch-up therefore cannot
apply damage, move an entity, clean a mission, auto-prepare a spell, emit a
recovery message, or run another world-dependent side effect. Older files use
their `Last` timestamp once and write `CkAt` on the next save. A timestamp more
than five minutes in the future is rejected; smaller backward clock skew
produces no elapsed work.

Nature's Wrath and Establish Camp are unchanged. Their Survival/Nature design
question remains explicitly deferred for a human gameplay decision.

The audit also corrected Fight to the Death's existing online cadence: its
600-second value had decremented only while the unrelated self-buff routine was
active and `cooldowns` displayed it as six times longer. It now advances by six
seconds on each established player-periodic callback and displays its stored
seconds directly, matching the ability's ten-minute definition and offline
recovery.

## 4. Validation Evidence

- Production-linked CuTest: 1,037/1,037 passed with the authored world fixture.
- The same 1,037/1,037 suite passed all four scheduler/legacy timed-backend and
  libevent/select I/O-driver combinations.
- Complete `make test-all`: 1,037 C tests, 504 world-tool tests, 29 protocol
  tests, 36 help tests, process-memory checks, character-rename checks, and
  installation passed.
- Four syntax boots covering scheduled/legacy runtime services and
  scheduler/legacy timed backends passed against the database-backed fixture.
- ASan and UBSan: 1,037/1,037 passed with leak detection and halt-on-error. The
  first pass exposed a post-free read in the newly extended test; the test was
  corrected to snapshot loaded state before cleanup and the definitive pass is
  clean.
- Valgrind: 1,037/1,037 passed with zero errors and no definite, indirect, or
  possible leaks.
- Event policy coverage proves all persisted records use elapsed schema 2.
- Focused tests cover schema 1 migration, partial and exact expiry, stale
  Spellbattle reconciliation, staggered multi-use catch-up, long offline
  recovery, future-clock handling, and both timed backends.
- The production player writer/loader test saves a ten-tick mission cooldown,
  verifies `CkAt`, backdates the checkpoint by two minutes, and observes no
  cooldown on the loaded character.
- Isolated live MUD on port 4104: level-34 Ornir created a real Treat Injury
  cooldown, logged out, then loaded after the schema 2 record and `CkAt` were
  backdated by 2,000 seconds. `cooldowns` no longer listed Treat Injury and the
  ability was immediately usable again.
- Real copyover with the renewed cooldown active preserved Ornir's descriptor
  and advanced the displayed duration naturally from 148 to 142 seconds. The
  replacement rebuilt scheduler/libevent services, removed its handoff file,
  closed the port on shutdown, and restored the pre-test Ornir database rows.

Acceptance logs are retained under `.ci-runtime/phase11h/`.

## 5. Residual Boundaries

This tranche advances only state already persisted and actively timed in the
current game. It does not invent recovery behavior for dormant fields with no
runtime producer or cadence. It also preserves existing online timing units,
including any historical balance values expressed through the six-second
player cadence; timing rebalance is separate gameplay work.
