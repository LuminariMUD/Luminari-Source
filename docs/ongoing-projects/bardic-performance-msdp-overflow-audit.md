# Bardic Performance and MSDP Overflow Audit

Status: Repairs in progress; reported overflow path and state lifecycle verified

Date: 2026-08-04

## Purpose

This document began as an investigation of a Mudlet structured-data decoder
failure observed while repeatedly performing `song of healing`. It now records
a source-level audit of the complete bardic performance subsystem, including:

- The active player `perform` command and recurring verse engine.
- All thirteen base performances.
- Performance state initialization, start, stop, failure, and disconnect paths.
- Group and foe targeting, affect creation, persistence, and timing.
- Instrument, action-economy, feat, help-file, and status-display integration.
- Spellsinger and Warchanter perks that read or modify performance state.
- The legacy NPC performance implementation.
- The MSDP `AFFECTS` serializer and descriptor output path involved in the
  reported overflow.

The initial diagnosis was a static source audit plus execution of the focused
protocol harness. Repair work is now active in the development tree. The first
production-linked state and command batch passed all 372 root CuTests. The
second overflow repair batch passes all 375 root CuTests and all 20 focused
protocol tests. Both batches were installed with `make install` on 2026-08-04.

## Reported Incident

The visible symptoms included:

- `**OVERFLOW**` appearing during a performance verse.
- Mudlet reporting an invalid JSON character inside a string.
- Fragments such as `"NAME":"Cold-Shielde` and
  `"DESC":"You are surro` immediately before the overflow marker.
- ANSI prompt bytes, displayed by Mudlet as `\027[...]`, appearing inside the
  malformed structured value.

The defect is server-side. Bard messages do not construct JSON and are not the
malformed data. Bardic affect churn causes the server to enqueue many complete
copies of the structured `AFFECTS` value. The descriptor buffer then truncates
one Telnet subnegotiation and inserts an in-band overflow marker into the
unterminated frame.

## Executive Summary

The original MSDP diagnosis is accurate. On a normally initialized Song of
Healing path, the first verse sends fifteen complete `AFFECTS` updates per
target and every later verse sends sixteen. Each update contains the target's
complete affected-bit and spell-like-affect state.

The usable descriptor output capacity is 24,143 bytes. A reported `AFFECTS`
payload only needs to be roughly 1.5 KiB for sixteen frames to exhaust that
capacity in one verse. A character with several active buffs can readily exceed
that threshold. Once the queue fills, `vwrite_to_output()` truncates the current
frame at an arbitrary byte. `process_output()` appends `**OVERFLOW**`; if the
truncation removed `IAC SE`, the marker and later prompt bytes remain inside the
same structured value.

The expanded audit found additional critical and high-severity defects:

- Songweaver reuses the affect initialization loop variable and leaves seven
  affect records uninitialized. This is undefined behavior.
- The secondary-performance slot initializes to zero, which is the valid index
  for Song of Healing. A Master of Motifs character can therefore execute Song
  of Healing twice per pulse without ever starting a second song.
- Master of Motifs cannot normally add a second song, and failure of either slot
  tears down shared state without consistently clearing the other slot.
- Efficient Performance is checked too late to bypass the interpreter's
  standard-action gate.
- Heightened Harmony first grants `+0`, then can accumulate duplicate `+5`
  affects while also being counted a second time by `compute_ability()`.
- Protective Chorus and Aria of Stasis consult the wrong character or omit the
  required performer/song relationship.
- Frostbite Refrain applies its Tier I cold rider with attacker and victim
  reversed, damaging the bard. Its damage is also already added through the
  generic weapon-damage path.
- Commanding Cadence and Winter's War March reverse the caster and victim in
  `savingthrow()` and pass a positive value as though it were a save DC even
  though that parameter is a bonus to the saving character.
- Several advertised capstone effects are placeholders or have no call sites.
- The perk design assumes a performance-round resource pool, but the active
  performance engine has no such resource. Base performances are already free
  and indefinite until stopped, interrupted, or failed.

These issues are not one repair. Memory safety, state invariants, affect
batching, and atomic protocol framing are now repaired and dynamically covered.
Song mechanics, resource costs, perk contracts, and conflicting player-facing
text remain in the next implementation batches.

The first repair batch establishes explicit absent sentinels, validates
performance indexes before table access, makes command transitions atomic,
enables Master of Motifs and Efficient Performance transitions, separates
primary and secondary failure, initializes every Songweaver affect record, and
cleans active state on disconnect or link loss. The second batch emits one
final `AFFECTS` state for a logical performance mutation, queues structured
Telnet frames atomically with retryable backpressure, applies only meaningful
affect slots, and makes affect serialization bounded and fail closed. Song
mechanics and performance-linked perk repairs remain.

## Accuracy Check of the Earlier Partial Audit

The following earlier conclusions were re-traced and remain correct:

- The Song of Healing mutation counts are fifteen on the first normal verse and
  sixteen on each later normal verse, per affected target.
- `LARGE_BUFSIZE` is 24,144 bytes and the writer reserves the terminating byte,
  leaving 24,143 usable queued bytes.
- `Cold-Shielded`, its description, and the exact `**OVERFLOW**` marker identify
  the `AFFECTS` payload and descriptor overflow path.
- `MSDPSend()` validates only its local 16 KiB frame buffer, not the descriptor's
  remaining cumulative capacity.
- The focused protocol harness now passes twenty tests, including a full-queue
  rejection followed by a complete-frame retry. A production-linked web
  onboarding test separately exercises the real descriptor queue limit.
- The Songweaver nested-loop defect is reachable and is more serious than the
  output overflow because it is undefined behavior.

Two scope clarifications are important:

- `update_msdp_affects()` currently returns unless `bMSDP` is enabled, so the
  reported `AFFECTS` incident is specifically an MSDP path. The same non-atomic
  `Write()` path is also used to emit GMCP frames, so the framing defect is not
  intrinsically limited to MSDP.
- The original Master of Motifs finding was incomplete. In addition to the
  unreachable second-song branch, the zero-initialized secondary slot can cause
  a duplicate Song of Healing, and per-slot failures corrupt shared state.

## Active System Map

```text
perform <name>
  -> interpreter dispatch without a fixed action gate
  -> do_perform()
       -> normalize transient state
       -> trim and resolve input before mutation
       -> exact or unambiguous case-insensitive performance match
       -> can_perform()
       -> select move or standard action from the character's feats
       -> start, replace, add, or stop an explicit named slot

global verse pulse
  -> pulse_bardic_performance()
       -> iterate the complete character list
       -> clear stale active state from linkless player characters
       -> bardic_performance_engine(primary slot)
       -> bardic_performance_engine(secondary slot), if present
       -> keep the surviving slot when the other slot fails
       -> performance-linked pulse perks
            -> Dirge of Dissonance
            -> Symphonic Resonance placeholder
            -> Endless Refrain placeholder

bardic_performance_engine()
  -> can_perform()
  -> two Perform checks and instrument adjustments
  -> process_performance()
       -> select group, foes, or room targets
       -> performance_effects()
            -> remove every affect with the same performance skill number
            -> execute instantaneous behavior
            -> join all eight affect slots
            -> affect_total()
            -> update_msdp_affects()
            -> immediate MSDPFlush()
```

NPC bards do not use this recurring player path. They call the older
`perform_perform()` implementation, apply a single `SKILL_PERFORM` buff, and use
`ePERFORM` as a long cooldown marker.

## Confirmed Overflow Failure Chain

```text
pulse_bardic_performance()
  -> bardic_performance_engine()
    -> process_performance()
      -> performance_effects()
        -> remove old effects for this performance
        -> join all eight BARD_AFFECTS slots
          -> affect_remove() and/or affect_to_char()
            -> affect_total()
              -> update_msdp_affects()
                -> rebuild the complete AFFECTS value
                -> MSDPFlush()
                  -> enqueue another complete protocol frame
                    -> descriptor output reaches 24,143 bytes
                      -> truncate the current frame
                      -> report success and clear the dirty flag
                      -> append **OVERFLOW**
                      -> omit the original IAC SE terminator
                        -> later ANSI prompt enters the structured value
                          -> Mudlet decoder rejects the value
```

The strings in the incident identify the payload conclusively:

- `Cold-Shielded` is `affected_bits[AFF_CSHIELD]` in `src/constants.c`.
- `You are surrounded by a shield of swirling snow.` is the corresponding
  entry in `affected_bit_descs[]`.
- The exact unadorned `**OVERFLOW**` marker is appended by
  `process_output()` in `src/comm.c` when descriptor space reaches zero.

## Finding Summary

| ID | Severity | Finding |
|----|----------|---------|
| BP-001 | Critical | A verse serializes and flushes the complete affect list up to sixteen times per target. |
| BP-002 | Critical | Songweaver leaves seven affect records uninitialized. |
| BP-003 | High | MSDP and GMCP frames are passed to a truncating text queue instead of being queued atomically. |
| BP-004 | High | All performances join eight slots even when most or all are no-op affects. |
| BP-005 | Medium | Affect serialization does not validate indexes or detect truncation. |
| BP-006 | Medium | Performance validation indexes the table before checking the index. |
| BP-007 | High | Master of Motifs cannot normally start a second song. |
| BP-008 | Medium | Stop/reset state and the disabled event implementation are incomplete. |
| BP-009 | Critical | The secondary slot initializes as Song of Healing and can double a verse. |
| BP-010 | High | Dual-song success and failure are not modeled independently. |
| BP-011 | High | Input matching and switch behavior can stop a valid song on invalid input. |
| BP-012 | High | Efficient Performance cannot reliably use its advertised move-action path. |
| BP-013 | Medium | Verse, duration, Lingering Performance, and help-file timing disagree. |
| BP-014 | High | Eligibility, immunity, hearing, ownership, and source rules are incomplete. |
| BP-015 | High | Connected-player-only pulsing and legacy NPC cooldown state create stale room conflicts. |
| BP-016 | High | Multiple base performances disagree with their descriptions or standard mechanics. |
| BP-017 | Critical to Medium | Spellsinger perk integrations contain incorrect, partial, and placeholder behavior. |
| BP-018 | Critical to Medium | Performance-linked Warchanter behavior contains reversed damage/save calls and missing ally effects. |
| BP-019 | High design blocker | Runtime code and documentation assume incompatible performance resource and perk contracts. |

## Implementation Progress

Last updated: 2026-08-04 after repair batch 2 verification.

Status meanings:

- `Verified`: implemented and covered by the production-linked root suite.
- `Implemented`: source repair is present, but finding-specific dynamic or
  sanitizer coverage is still pending.
- `Partial`: part of the finding is repaired and the remaining scope is named.
- `Pending`: no source repair has been made for that finding.

| ID | Status | Current evidence and remaining work |
|----|--------|-------------------------------------|
| BP-001 | Verified | Nested affect batching recalculates live state but emits only the final `AFFECTS` value; root coverage proves one frame for a two-affect batch and zero frames for an unchanged replacement. |
| BP-002 | Implemented | All eight records now receive `new_affect()` and deterministic fields; add direct Songweaver and sanitizer coverage. |
| BP-003 | Verified | MSDP, GMCP, MSSP, and MXP frames use an atomic raw queue operation with reserved headroom; focused and production-linked tests prove no partial frame and retry retention under backpressure. |
| BP-004 | Verified | Each performance marks only meaningful affect slots; root coverage proves Healing creates zero affects, Protection creates three, and refreshed Flight retains exactly one flying affect. |
| BP-005 | Verified | The bounded serializer validates metadata, preserves the prior value on invalid or oversized input, and checks protocol results; root tests cover invalid indexes and aggregate overflow. |
| BP-006 | Verified | Bounds are checked before table access; root tests cover negative, maximum, and oversized indexes. |
| BP-007 | Verified | Master of Motifs can add a distinct secondary song; the root suite preserves both slots. |
| BP-008 | Partial | Central reset/slot teardown, spell interruption, disconnect cleanup, and the retired duplicate event path are repaired; add direct disconnect and Harmonic Casting tests. |
| BP-009 | Verified | `clear_char()` and player initialization use `PERFORMANCE_NONE`; the root suite proves both slots begin absent. |
| BP-010 | Implemented | Per-slot engine failure preserves or promotes the surviving song; direct helper coverage is present and forced engine-failure coverage remains. |
| BP-011 | Verified | Input is trimmed and resolved before mutation; root tests cover whitespace, unknown, capitalized, ambiguous, duplicate, and unavailable replacements, and listing scans only valid feat indexes. |
| BP-012 | Verified | `perform` has no fixed interpreter action gate; command preflight selects move or standard action and root tests cover both paths plus immediate list/stop. |
| BP-013 | Pending | Timing constants, real-time behavior, Lingering Performance, and help text still need reconciliation. |
| BP-014 | Pending | Eligibility, immunity, hearing, source ownership, and iterator work remains. |
| BP-015 | Implemented | Pulsing uses `character_list`, clears linkless player state, processes active NPC state, and ignores legacy `ePERFORM` cooldowns as room conflicts; direct active-NPC pulse coverage remains. |
| BP-016 | Pending | Base performance mechanics matrix remains unresolved. |
| BP-017 | Pending | Spellsinger behavior and placeholders remain unresolved. |
| BP-018 | Pending | Warchanter call-order and missing ally effects remain unresolved. |
| BP-019 | Pending | Resource and documentation contract still requires an explicit implementation decision. |

### Repair Checkpoints

- `a9f6eb5a` recorded the complete audit, documentation index entry, and the
  synchronized 2.5038-beta development version before source repairs began.
- Repair batch 1 covers BP-002 and BP-006 through BP-012, plus the lifecycle
  portion of BP-015, and publishes as development version 2.5039-beta.
  Verification: warning-clean GNU C23 build, `make test` with 372/372 passing
  tests, and `make install`.
- Repair batch 2 covers BP-001 and BP-003 through BP-005 and publishes as
  development version 2.5040-beta. Verification: warning-clean GNU C23 build,
  `make test` with 375/375 passing tests, `make protocol-parser` with 20/20
  passing tests, a 10-second ASan/UBSan protocol fuzz run, and `make install`.

## Detailed Findings

### BP-001: A Verse Flushes the Complete Affect List Repeatedly

Severity: Critical

`performance_effects()` initializes an array of eight `affected_type` records.
After executing the song-specific behavior, it passes every record to
`affect_join()`, whether or not that slot represents a real effect.

`affect_join()` identifies an existing affect by spell and location. A match is
replaced by calling `affect_remove()` followed by `affect_to_char()`. Both paths
call `affect_total()`, which calls `update_msdp_affects()`. That function
immediately calls `MSDPFlush()`.

For Song of Healing, all eight normal slots use the same spell and `APPLY_NONE`
location:

- The first slot is added: one full update.
- Each of the remaining seven slots replaces the preceding slot: two full
  updates per slot.
- Total on the first verse: `1 + (7 * 2) = 15` full updates.
- A later verse first removes the old marker: one additional update.
- Total on every later verse: `1 + 15 = 16` full updates.

More generally, a correctly initialized recurring performance produces sixteen
affect mutations regardless of how many distinct locations its eight slots use.
Old distinct effects are removed, and all eight slots are then added or
replaced. The Songweaver and duplicate-secondary paths are not correctly
initialized normal paths and can behave differently or fail outright.

This work occurs once per eligible group member or foe. It is wasteful even if
the client is not reporting `AFFECTS`; when reporting is enabled, every
intermediate list is serialized and sent.

Repair: Affect batches keep in-memory totals current after every mutation but
defer MSDP serialization until the outer batch ends. `performance_effects()`
wraps replacement and insertion in one batch. The root suite verifies one
final frame for a changed batch and no frame when the final value is unchanged.

Relevant code:

- `src/bardic_performance.c:502-524` - creation of eight affect records.
- `src/bardic_performance.c:526-531` - removal of prior song effects.
- `src/bardic_performance.c:800-809` - unconditional join of all slots.
- `src/handler.c:1023-1106` - full serialization and immediate flush.
- `src/handler.c:1131-1158` - affect insertion and total recalculation.
- `src/handler.c:1217-1280` - affect removal and total recalculation.
- `src/handler.c:1381-1415` - remove-then-add replacement behavior.

### BP-002: Songweaver Leaves Seven Affect Records Uninitialized

Severity: Critical

The initialization loop uses variable `i`. Inside that loop, the Songweaver
duration code starts another loop using the same `i`:

```c
for (i = 0; i < BARD_AFFECTS; i++)
{
  new_affect(&(af[i]));
  /* initialize af[i] */

  if (songweaver_bonus > 0)
  {
    for (i = 0; i < BARD_AFFECTS; i++)
      af[i].duration += songweaver_bonus;
  }
}
```

When the bonus is positive:

1. `af[0]` is initialized.
2. The nested loop reads and writes `duration` in uninitialized `af[1]`
   through `af[7]`.
3. The nested loop leaves `i == BARD_AFFECTS`.
4. The outer loop terminates without initializing the remaining records.
5. Resonant Voice reads `af[6].location`.
6. The final loop passes all eight records to `affect_join()`.

Uninitialized fields include `spell`, `location`, `bonus_type`, both bitvector
arrays, `specific`, and `next`. Downstream code directly indexes
`apply_types[af->location]` and `bonus_types[af->bonus_type]`, so this can cause
an out-of-bounds read while building `AFFECTS`, corrupt affect state, or crash.

The bonus must be computed once before initialization. Each record must be
fully initialized before its duration is assigned. Songweaver's advertised
potency scaling also needs a separate design and implementation; the only
current integration is duration.

Repair: Songweaver's duration bonus is computed before the initialization loop,
and every affect record is passed through `new_affect()` before any song logic
can read it. Direct Songweaver and sanitizer coverage remain tracked before this
finding is promoted from Implemented to Verified.

Relevant code:

- `src/bardic_performance.c:503-524` - reused loop variable.
- `src/bardic_performance.c:784-798` - read of `af[6]`.
- `src/bardic_performance.c:800-809` - use of all records.
- `src/utils.c:4670-4686` - fields normally initialized by `new_affect()`.
- `src/handler.c:1090-1094` - unguarded location and bonus-type indexing.

### BP-003: Protocol Frames Are Not Queued Atomically

Severity: High

`MSDPSend()` checks whether one raw frame fits its 16 KiB local buffer. It does
not check whether the complete frame fits the remaining descriptor queue. It
calls `Write()`, which returns no status and delegates to the general text
writer.

`vwrite_to_output()` shortens incoming text to the remaining queue capacity.
That behavior is tolerable for display text but invalid for Telnet
subnegotiations because it can remove `IAC SE`. `MSDPSend()` still returns
success, so `MSDPFlush()` or `MSDPUpdate()` clears the variable's dirty flag and
does not retry the lost frame.

`process_output()` then appends `**OVERFLOW**` whenever `bufspace == 0`,
including when the output is marked out-of-band. Suppressing only the marker is
not sufficient; the frame would remain truncated and unterminated.

Protocol emission must be atomic. If a processed frame plus required headroom
does not fit, the server should append none of it, return
`PROTOCOL_ERROR_BUFFER_FULL`, retain the dirty flag, and retry after queued
output drains.

Repair: Encoded structured frames now bypass the truncating display-text path
and enter the descriptor queue through an all-or-nothing raw append with 512
bytes of reserved headroom. Queue rejection does not advance onboarding
transfers or clear an MSDP dirty value. The focused harness proves a rejected
frame appends zero bytes and a later retry ends with `IAC SE`; the root suite
exercises the production queue at capacity.

Relevant code:

- `src/net/protocol.c:36-50` - status-less `Write()` wrapper.
- `src/net/protocol.c:1507-1555` - dirty state and flush behavior.
- `src/net/protocol.c:1558-1632` - individual frame construction.
- `src/comm.c:2435-2470` - arbitrary cumulative truncation.
- `src/comm.c:2758-2804` - overflow marker and prompt emission.
- `src/structs.h:5616-5621` - descriptor capacity definitions.
- `src/net/onboarding.c:4357-4375` - an existing capacity-checking pattern for
  web onboarding frames.

### BP-004: All Performances Apply Unused Affect Slots

Severity: High

Most performances use fewer than eight meaningful records. Song of Healing
performs an instantaneous heal and leaves all eight records as identical
`APPLY_NONE` markers. Every slot is nevertheless joined.

This creates no-op affects, recalculates the character repeatedly, and makes
the list depend on replacement order instead of an explicit effect count.

The implementation should use an active-slot count or mask. If a persistent
marker is needed to identify or refresh a song, it should add one explicit
marker. Instantaneous songs should not manufacture eight identical records.

Repair: `performance_effects()` now records an explicit active-slot mask for
each song and joins only those slots. Conditional flag effects set their mask
only when applied, instantaneous songs use no marker, and Resonant Voice marks
its additional slot explicitly. Root tests cover zero-, one-, and three-slot
paths plus replacement of an existing one-slot Flight effect.

### BP-005: Affect Serialization Does Not Fail Closed

Severity: Medium

`update_msdp_affects()` uses 100-, 200-, and 400-byte temporary buffers plus a
49,152-byte aggregate buffer. It does not check any `snprintf()` or `strlcat()`
result. It ignores `MSDPSetString()` and `MSDPFlush()` results. It also indexes
`apply_types` and `bonus_types` without validating affect fields.

The current static names and descriptions fit their local buffers, so this did
not initiate the reported incident. The construction is still brittle. A
longer future value, corrupt affect, or aggregate over `MAX_VARIABLE_LENGTH`
can produce a truncated local object or leave a stale protocol value.

Use a bounded writer that records overflow. Validate `spell`, `location`, and
`bonus_type` before lookup. On failure, log the condition and send nothing.

Repair: The serializer writes directly into a `MAX_VARIABLE_LENGTH`-bounded
buffer through a checked append helper, validates every metadata index and
pointer before lookup, reserves frame space, and checks both set and flush
results. Invalid and oversized states leave the prior protocol value unchanged
and queue no output in production-linked tests.

### BP-006: Performance Index Validation Reads Before Validating

Severity: Medium

`is_valid_performance()` accesses
`performance_info[performance_num][PERFORMANCE_SKILLNUM]` without first checking
that `performance_num` is in `[0, MAX_PERFORMANCES)`. No range check exists in
the function intended to validate the index, so it can perform the invalid read
itself.

Normal command lookup supplies a valid index, but primary and secondary pulse
state are raw integers and receive no upper-bound check.

Relevant code: `src/bardic_performance.c:134-184`.

### BP-007: Master of Motifs Cannot Normally Start a Second Song

Severity: High

`can_perform()` is intended to let a Master of Motifs character add a second
performance. `do_perform()` first stops and clears every current performance
whenever any non-empty argument is entered. The later branch requiring
`IS_PERFORMING(ch)` is therefore unreachable in the normal command flow.

Even if the early clear were removed, the function unconditionally writes the
new number to `GET_PERFORMING(ch)` after assigning the secondary slot. That
would overwrite the primary. It also does not reject selecting the same song
twice.

Relevant code:

- `src/bardic_performance.c:259-280` - intended dual allowance.
- `src/bardic_performance.c:383-405` - early shared-state clear.
- `src/bardic_performance.c:430-460` - unreachable branch followed by primary
  overwrite.

### BP-008: Stop/Reset State and the Disabled Event Path Are Incomplete

Severity: Medium

- A no-argument stop clears only `IS_PERFORMING`. It leaves the primary,
  secondary, and Crescendo variables stale.
- Spellcasting and several engine failure paths implement their own slightly
  different copies of performance teardown.
- The disabled `EVENT_RAN` command path formats undeclared variable `i`; it
  likely intended `performance_num`.
- The `EVENT_RAN` preprocessor layout excludes the closing brace of
  `event_bardic_performance()` when enabled. The event build therefore has more
  than the one undeclared-variable error.
- `VERSE_INTERVAL` in `bardic_performance.h` duplicates the active
  `PULSE_VERSE_INTERVAL` definition in `structs.h`.
- `save_char()` is called before and after the effect switch for every player
  target on every verse, creating unnecessary persistence work.
- User-facing strings include `abrupted`, `stikes`, `$n hs lost`, and `$n fly`.

One `stop_bardic_performance()` helper should own all state cleanup and be used
by commands, spell interruption, validation failure, stutter, disconnect, and
future event code.

### BP-009: The Secondary Slot Initializes as Song of Healing

Severity: Critical

Player initialization sets all ten `performance_vars` entries to zero, then
sets only the primary slot to `-1`. The secondary slot remains zero. Performance
index zero is Song of Healing.

When a Master of Motifs character starts Song of Healing, `do_perform()` only
sets the secondary slot to `-1` if its current value differs from the selected
song. Zero equals the selected index, so the secondary remains zero. The next
pulse runs both primary and secondary as Song of Healing.

Consequences per target on a normal, non-Songweaver path are:

- First pulse after start: 15 affect updates for the primary plus 16 for the
  duplicate secondary, or 31 total.
- Later pulses: 16 plus 16, or 32 total.
- Instantaneous healing and other verse side effects execute twice.

The no-argument stop defect can recreate the same condition with stale
secondary state. All absent performance slots must initialize and reset to a
single named sentinel such as `PERFORMANCE_NONE`.

Relevant code:

- `src/players.c:743-745` - zero initialization and primary-only sentinel.
- `src/bardic_performance.c:453-460` - equality-based secondary reset.
- `src/bardic_performance.c:1225-1233` - both slots processed.

### BP-010: Dual-Song Success and Failure Are Not Independent

Severity: High

`bardic_performance_engine()` receives only a performance number, not the slot
being processed. Any validation failure, process failure, or stutter clears
the primary number and global `IS_PERFORMING`, even if it occurred while
processing the secondary. The secondary number is not consistently cleared.

If the primary fails, the secondary is skipped. If the primary succeeds and
the secondary fails, the successful primary is torn down while stale secondary
state remains. There is no way to report which song failed or allow the other
song to continue.

The raw indexes also have unrelated meanings packed into magic array positions:

- `0`: active flag.
- `1`: primary performance.
- `2`: secondary performance.
- `3`: Crescendo-used flag.
- `4`: pending Crescendo damage dice.

Use named fields or at least named index constants. Model primary and secondary
slot validation and failure separately, then derive the global active state
from whether any valid slot remains.

### BP-011: Input Matching and Switching Destroy Valid State

Severity: High

Any non-empty `perform` argument stops the current song before whitespace is
trimmed, the requested name is resolved, ownership is checked, or start
conditions are validated. A typo, wrong capitalization, unknown song,
unavailable feat, silence, room conflict, or other rejected replacement thus
stops a valid current performance.

Name matching uses case-sensitive `strncmp(argument, skill_name, len)` and
accepts the first prefix in table order:

- A whitespace-only argument becomes length zero after the early stop and
  matches Song of Healing because `strncmp(..., 0)` succeeds.
- Short ambiguous prefixes such as `s` select the first matching song rather
  than reporting ambiguity.
- Capitalized names need not match lower-case skill names.

Resolve and validate a requested transition first, then commit it atomically.
Use the normal case-insensitive command abbreviation rules and reject ambiguous
prefixes.

### BP-012: Efficient Performance Is Gated as a Standard Action

Severity: High

The command table declares `perform` as `ACTION_STANDARD`. The interpreter
checks standard-action availability before `do_perform()` runs and queues the
command if the standard action is unavailable. Only inside `do_perform()` does
Efficient Performance choose `USE_MOVE_ACTION()`.

Therefore a character with a move action available but a standard action on
cooldown cannot start the performance immediately, contrary to the feat. The
same command-table gate also applies to no-cost list and stop operations, so a
stop request can be queued instead of taking effect immediately.

The command needs a preflight/action-selection hook or must declare an action
requirement compatible with the feat and handle start versus list/stop paths
explicitly.

Relevant code:

- `src/interpreter.c:3133` - fixed standard-action declaration.
- `src/interpreter.c:6289-6310` - pre-command availability gate and queue.
- `src/bardic_performance.c:467-470` - late action selection.
- `src/actions.h:34-37` - move action falls back to a standard action.

### BP-013: Verse and Affect Timing Disagree With Documentation

Severity: Medium

The active pulse runs every eleven seconds. New performances do not execute an
immediate verse, so the first effect arrives zero to eleven seconds after the
start command depending on the global pulse phase.

Most performance affects use duration `3`. Affect duration decrements every
six-second combat pulse, so the nominal duration is three rounds, not one.
Because an affect is removed on the update after it reaches zero, wall-clock
lifetime is approximately eighteen to twenty-four seconds depending on pulse
phase. The eleven-second refresh already overlaps without Lingering
Performance.

Lingering Performance adds another `3`, producing a nominal six rounds and an
approximately thirty-six to forty-two second wall-clock lifetime, not a change
from six to twelve seconds. Songweaver adds one affect round per rank; it does
not increase effectiveness or potency. Song of Flight overwrites its flying
slot with duration `30`, so the normal Songweaver duration assignment does not
extend that slot.

Current text says:

- The verse repeats approximately every seven seconds.
- Effects normally last one round or six seconds.
- Lingering Performance is what makes effects overlap.
- Songweaver increases duration and potency.

Performance-linked perks also use inconsistent clocks: Dirge is processed by
the eleven-second verse pulse while advertised per round, and Sustaining Melody
is checked by the five-second Luminari pulse while advertised per combat round.

Relevant code:

- `src/structs.h:5587-5593` - six-second affect and eleven-second verse pulses.
- `src/comm.c:1598-1606` and `src/comm.c:1623-1627` - active scheduling.
- `src/bardic_performance.c:508`, `src/bardic_performance.c:687`, and
  `src/bardic_performance.c:803-805` - durations.
- `src/character/feats.c:1914-1918` and
  `lib/text/help/help.hlp:17353-17356` - stale timing text.

### BP-014: Eligibility, Targeting, Immunity, and Effect Ownership Are Incomplete

Severity: High

Eligibility and instruments:

- The help file says each performance requires a minimum trained Perform
  ability. `can_perform()` checks the associated feat but no ability threshold.
- Most performance feat descriptions say the ideal instrument is mandatory.
  The engine deliberately permits a wrong instrument or no instrument with an
  effectiveness penalty.
- `INSTRUMENT_SKILLNUM` is populated in `performance_info` but never used. No
  player instrument proficiency is checked.
- Heroism and Revelation upgrades use total character level rather than bard
  level or performance effectiveness.

Targets and defenses:

- Audio performance types stop if the performer is silenced or the room is
  soundproof, but recipients are not checked for deafness or ability to hear.
- Fear checks a few bespoke protections, but Fear, Rooting, Forgetfulness,
  Magi, and Deafening do not consistently use the standard mind-affecting,
  condition-immunity, `can_deafen()`, or saving-throw pipelines.
- Direct bard healing calls `process_healing()` without the golem immunity in
  `mag_points()`, so magical performance healing can bypass the normal construct
  restriction.

Ownership and iteration:

- `affected_type` records carry a spell number but no source identity.
  `affect_from_char(tch, spellnum)` removes every matching performance affect,
  including lingering effects from another bard.
- The one-bard-per-room restriction reduces but does not eliminate collisions:
  performers and affected targets can move, and lingering state can meet a new
  bard later.
- Group targeting uses global, non-reentrant `simple_list()` state while calling
  the complex `performance_effects()` path inside the loop. `lists.c` explicitly
  warns not to nest or transitively reuse this iterator. Use `merge_iterator()`.
- Verse messages are broadcast to the room before group membership, `aoeOK()`,
  immunity, or random success is known. Non-recipients can see text stating that
  they feel the effect.

### BP-015: Pulse Lifecycle and Legacy NPC State Create Stale Conflicts

Severity: High

`pulse_bardic_performance()` iterates playing descriptors, not the character
list. A linkless performer retains `IS_PERFORMING` but receives no pulses and no
continued validation. No disconnect cleanup was found. That stale character can
continue blocking another bard under the one-bard-per-room check.

NPC bards use `perform_perform()` instead of the recurring system. It attaches
`ePERFORM` as a long cooldown after applying a one-shot buff. `can_perform()`
treats any other character's `ePERFORM` as an active conflicting performance.
Consequently an NPC bard on cooldown can prevent a player from performing even
though the NPC is no longer executing verses.

Other lifecycle drift:

- An NPC forced into the new `IS_PERFORMING` state will not pulse because it has
  no playing descriptor.
- Room status display checks only `eBARDIC_PERFORMANCE`, the disabled event path,
  so active pulse-based performers are not labeled `(performing)`.
- Legacy `ePERFORM`, disabled `eBARDIC_PERFORMANCE`, and active
  `IS_PERFORMING` represent three different concepts but are compared as though
  they were equivalent.

Relevant code:

- `src/bardic_performance.c:312-333` - room conflict check.
- `src/bardic_performance.c:1212-1298` - descriptor-only pulse.
- `src/act.other.c:1581-1668` - legacy one-shot NPC performance and cooldown.
- `src/mob/mob_class.c:138-149` - NPC caller.
- `src/act.informative.c:1148-1149` - event-only status display.

### BP-016: Base Performance Mechanics and Text Need Reconciliation

Severity: High overall; individual rows range from text drift to probable
mechanical defects.

| Performance | Confirmed implementation | Gap or decision required |
|-------------|--------------------------|--------------------------|
| Song of Healing | Heals each in-room group member every verse, then joins eight no-op slots. | Instrument is optional despite "must hold a lyre". Healing bypasses the golem check. Displayed healing is emitted before clamping. |
| Dance of Protection | Grants new AC, Will save, and damage reduction. | Description promises armor and spell resistance; no spell resistance is applied. |
| Song of Focused Mind | Grants INT, WIS, and CHA and accelerates preparation when the song affect is present. Targets the group. | Description says anyone in the room and discusses preparation speed only. |
| Song of Heroism | Grants hit, damage, STR, DEX, CON, and haste when the performer's total level is at least 10. | Extra attack is tied to total level, not the advertised performance proficiency. Instrument is optional. |
| Oratory of Rejuvenation | Restores HP and movement and may remove poison. | Broadly matches its description, apart from optional instrument and generic targeting/serialization issues. |
| Song of Flight | Applies flying for 30 affect rounds and restores movement. | Songweaver does not extend the overwritten flying duration. Instrument is optional. |
| Song of Revelation | Adds five detection flags at total levels 1, 5, 10, 15, and 20. | Uses total level rather than bard level or proficiency. Instrument is optional. |
| Song of Fear | Randomly applies fear and a hit penalty after a few bespoke immunity checks. | Does not use the standard mind-affecting immunity and save pipeline. |
| Skit of Forgetfulness | On a random success, clears NPC memory and stops the NPC's side of combat. It does nothing to player targets. | The performer's `FIGHTING` pointer remains, so combat is not fully disengaged and may resume. No standard save or immunity is used. |
| Song of Rooting | Randomly applies entangle, slow, damage penalty, and AC penalty. | No standard save/immunity; room text says paralyzed, which is not the applied condition. |
| Song of Dragons | Always grants AC, five saves, a large CON bonus, and maximum HP. | Description says each verse chooses among three effects, including healing, Reflex, and AC. The implemented fixed eight-effect package is materially different. |
| Song of the Magi | Randomly penalizes Will and spell resistance but also grants positive INT, WIS, and CHA to every affected foe. | The mental-stat bonuses contradict a foe debuff and are probable sign errors. No standard save is used. |
| Deafening Song | Unconditionally applies deafness and an AC penalty to eligible foes. | Does not call `can_deafen()` or grant a save; user-facing text contains a typo. |

Resonant Voice adds another base-song inconsistency. It uses slot six only if
that slot is `APPLY_NONE`, so it is silently absent from Song of Dragons. It
grants a generic competence bonus to all Will saves, not a bonus limited to
mind-affecting effects as advertised.

### BP-017: Spellsinger Perk Integration Is Partial or Incorrect

Severity: Critical to Medium

| Perk | Confirmed behavior | Gap or defect |
|------|--------------------|---------------|
| Songweaver I/II | The combined rank helper is used only to add affect duration. | BP-002 is undefined behavior. No potency or effectiveness scaling exists. |
| Resonant Voice | Adds a competence Will bonus in slot six on group performances. | Applies to all Will saves, disappears when slot six is occupied, and has no source-specific ownership. |
| Harmonic Casting | A bard spell has a 50% chance not to stop all active songs. | Text describes conserving a performance round, but no round resource exists. On failure both songs are cleared. |
| Crescendo | If the song survives Harmonic Casting, the first bard spell sets +2 DC and one pending d6 of sonic damage. | Crescendo requires Harmonic Casting, so the advertised first spell receives no benefit on the 50% interruption path. A non-damaging first spell leaves the sonic die for a later damage spell. `GET_DC_BONUS` is consumed by the first save check, not reliably every target of an area spell. |
| Sustaining Melody | While fighting and performing, the five-second Luminari pulse has a 20% chance to restore a bard slot. | Advertised per combat round. It also depends on mutable `GET_CASTING_CLASS` being Bard rather than simply checking Bard ownership/slots. |
| Master of Motifs | Pulse code can process a primary and secondary integer. | BP-007, BP-009, and BP-010 prevent a safe two-song state. Source and design-doc prerequisites also disagree. |
| Dirge of Dissonance | Deals 1d6 sonic to eligible room foes on each eleven-second verse. PC concentration checks scan for an enemy performing bard and take -2. | Advertised per six-second round. NPC concentration checks receive no penalty. |
| Heightened Harmony | A metamagic bard cast adds an `APPLY_SKILL` affect with duration three rounds. | The modifier is obtained from a helper that returns +5 only when the affect is already active, so the first proc grants +0. Later casts append duplicate +5 affects. `compute_ability()` sums those affects and then adds the helper's +5 again. Duration is about 18 seconds, not one minute. |
| Protective Chorus | The perk owner always receives +2 saves and +2 dodge AC through global calculations. | No active performance is required, allies are not located, and AC applies against all attacks rather than attacks of opportunity. |
| Spellsong Maestra | The +2 spell DC helper is called while the owner performs. | Caster-level and free-metamagic helpers have no call sites. The DC call checks only that the character has a Bard level and is casting a spell, so a multiclass non-Bard spell can qualify. Runtime text and `BARD_PERKS.md` describe different capstones. |
| Aria of Stasis | Attack calculations can penalize a foe attacking a grouped ally of an Aria owner. | Runtime registration describes a passive aura; `BARD_PERKS.md` describes a timed active room control. Save code checks the hostile caster and gives its victim +4 when the caster owns Aria. No performance is required. Movement penalty and slow immunity are not wired. |
| Symphonic Resonance | Before an Enchantment/Illusion spell resolves, code attempts to daze non-NPC room enemies for one round. | Temporary HP is a message-only TODO. The daze skips every NPC; on a no-PK server `aoeOK()` then rejects PCs, leaving no targets. It has no save, ignores the 20-foot helper, and can occur before the triggering spell succeeds. |
| Endless Refrain | The verse pulse prints a spell-reserve message. | Slot regeneration is a TODO. The resource-conservation helper is unused, and every base song is already free and indefinite. |

### BP-018: Performance-Linked Warchanter Behavior Is Incorrect or Missing

Severity: Critical to Medium

| Perk | Confirmed behavior | Gap or defect |
|------|--------------------|---------------|
| Battle Hymn I/II | Rank bonuses are added to the perk owner's generic melee weapon damage. | No active performance or Inspire Courage affect is required. Allies receiving Inspire Courage get nothing. |
| Drummer's Rhythm I/II | Adds melee to-hit to the owner while `IS_PERFORMING`. | This personal portion matches the source description, subject to the unreliable shared performance state. |
| Frostbite Refrain I/II | Rank damage is added through generic weapon damage while performing; natural 20 debuffs are applied to the target. | The generic damage is not typed cold. `damage_shield_check()` then attempts an extra Tier I cold rider as `damage(victim, ch, ...)`, damaging the bard and attributing it to the target. The Tier I amount is therefore both included in outgoing generic damage and reflected back as cold damage. |
| Warbeat | Helper functions exist. | No call site implements the first-turn extra attack or ally damage buff. |
| Anthem of Fortitude | Maximum-HP calculation gives the performing perk owner +10%. | Allies are not found. The +2 Fortitude helper has no call site. |
| Commanding Cadence | A successful melee hit attempts a Will save and applies target cooldown state. | It calls `savingthrow(victim, ch, ...)`, making the bard the saving character. Its `save_dc` is passed in the victim-modifier position, where a positive number improves the save. The perk does not require a song despite its cadence framing. |
| Steel Serenade | The owner receives natural AC while performing. | The 10% physical damage-resistance helper has no call site. |
| Banner Verse | Helper functions exist. | No command, room object, or bonus call site implements the advertised standard. |
| Warchanter's Dominance | The owner gains generic hit, damage, and AC while any performance is active. | The bonuses are not attached to Inspire Courage or Warbeat recipients, and no ally effects are implemented. |
| Winter's War March | While performing, a melee hit can proc against that one target, with per-target affect cooldowns. | It is not an activated room-wide anthem. It reverses `savingthrow()` caster/victim and misuses the modifier as a DC. It subtracts `GET_HIT` directly instead of using `damage()`, bypassing cold resistance, damage hooks, attribution, and the normal death/update pipeline. Its "slow" is a STR penalty rather than a slow condition. |

The reversed calls are especially important because the signature is
`savingthrow(caster, victim, type, victim_modifier, ...)`. Higher
`victim_modifier` values improve the victim's chance to resist; they are not an
absolute DC.

### BP-019: Resource and Documentation Contracts Are Incompatible

Severity: High design blocker

The active engine has no performance rounds, daily uses, or shared song pool.
Starting a song consumes an action, after which verses continue without a
resource until a stutter, interruption, invalid state, or explicit stop.

This makes several runtime descriptions impossible as written:

- Harmonic Casting cannot "save a performance round".
- Master of Motifs cannot share a pool that does not exist.
- Spellsong Maestra cannot remove a second-song cost that does not exist.
- Endless Refrain cannot make an already-free performance free.

Documentation also disagrees at several levels:

- `help perform` says seven-second verses, six-second effects, trained Perform
  thresholds, and Lingering-based overlap.
- Base performance feat descriptions often require instruments that code makes
  optional.
- `docs/systems/perks/BARD_PERKS.md` and `define_bard_perks()` disagree on
  Spellsong Maestra, Aria of Stasis, available Spellsinger capstones, and some
  prerequisites.
- Source comments, perk registration text, and call sites sometimes describe
  three different versions of the same perk.

Do not resolve these conflicts by choosing whichever text is easiest to edit.
First decide whether the intended system has a finite performance resource and
which perk specification is authoritative. Then update implementation, runtime
perk descriptions, help files, and design documentation together.

## Direct String Audit

No unsafe direct string construction was found in the active bard performance
implementation:

- Performance messages are string literals sent through `send_to_char()` or
  `act()`.
- There is no JSON construction in `src/bardic_performance.c`.
- There is no `sprintf()`, `strcpy()`, `strcat()`, or manually managed dynamic
  string buffer in the active implementation.
- The one `snprintf()` uses a 128-byte local buffer in the disabled event path;
  its size is safe, although the variable is undeclared and the event branch
  has a separate brace/preprocessor error.
- `You perform without an instrument...  ` intentionally lacks a newline so
  the following verse message appears on the same line. This is not the
  overflow source.

The malformed structured string is an output-framing failure caused by affect
update volume, not a missing quote or escape in a bard message.

## Recommended Repair Sequence

### 1. Fix Memory Safety and State Invariants

- Validate a performance index before every table access.
- Initialize all absent performance slots to one named sentinel.
- Fix the Songweaver initialization loop.
- Replace magic performance-array indexes with named fields or constants.
- Add one teardown helper and one validated transition helper.
- Make primary and secondary slot failure independent.

These repairs should not wait on gameplay design decisions.

### 2. Apply Only Real Effects and Batch Affect Changes

- Replace unconditional traversal of all eight slots with an active count or
  mask.
- Define whether each instantaneous performance needs one explicit marker or no
  marker.
- Remove old performance effects and add replacements as one logical
  transaction.
- Recalculate totals, persist if necessary, and notify the client only after
  the final affect list is stable.
- Remove the two per-target `save_char()` calls unless a demonstrated
  persistence contract requires them.

### 3. Coalesce and Atomically Emit Structured Updates

- Stop immediately flushing `eMSDP_AFFECTS` after every intermediate mutation.
  The once-per-second `msdp_update()` path can coalesce dirty values.
- Add a descriptor-capacity API that accepts or rejects one complete processed
  OOB frame.
- On insufficient capacity, append nothing, preserve dirty state, and retry
  after output drains.
- Never insert `**OVERFLOW**` inside an OOB frame.
- Harden the affect serializer with validated indexes and checked writes.

Increasing `LARGE_BUFSIZE` is not a complete fix; another sufficiently large
burst could still split a frame.

### 4. Repair Command and Action Transitions

- Trim and resolve input before mutating current state.
- Use case-insensitive, ambiguity-aware matching.
- Separate list, stop, start, replace-primary, add-secondary, and stop-one-song
  operations.
- Select the required action before the interpreter availability check so
  Efficient Performance works with only a move action.
- Make list and stop operations immediate unless design explicitly assigns
  them an action cost.

### 5. Define Target, Source, and Defense Contracts

- Decide whether each performance is audible, visual, language-dependent,
  mind-affecting, magical, and subject to saves or condition immunities.
- Use standard hearing, `can_deafen()`, mind-affecting, save, golem-healing, and
  damage pipelines as appropriate.
- Track the producing bard or another ownership key for refresh and removal.
- Replace the stateful group list loop with an explicit iterator.
- Emit success messages only to actual recipients after eligibility succeeds.

### 6. Reconcile Every Base Song

Use the BP-016 matrix as an acceptance checklist. In particular, decide the
intended mechanics for Dance of Protection, Song of Dragons, Song of the Magi,
instrument requirements, level scaling, and offensive saves before editing
their descriptions.

### 7. Reconcile Performance-Linked Perks

- Repair Heightened Harmony, Frostbite Refrain, Commanding Cadence, and Winter's
  War March before enabling or advertising them as complete.
- Route ally perks from a qualifying performer to actual recipients.
- Implement or remove placeholder and unused capstone effects.
- Decide whether a performance resource exists, then implement Harmonic
  Casting, Master of Motifs, and Endless Refrain against that contract.
- Ensure Bard-only spell perks check the actual casting class.

### 8. Update Player-Facing Documentation Last

After mechanics are selected and tested, update together:

- `lib/text/help/help.hlp`.
- Performance and Lingering Performance text in `src/character/feats.c`.
- Bard perk registration text in `src/character/perks.c`.
- `docs/systems/perks/BARD_PERKS.md`.
- Any status, action, and instrument documentation affected by the final
  contract.

## Regression Test Requirements

### Production-Linked State and Command Tests

Production-linked bard behavior now lives in the existing registered
`unittests/CuTest/test_bardic_performance.c` suite. The following list remains
the acceptance checklist as later repair batches add mechanics coverage.

Required cases:

1. All performance slots initialize to the absent sentinel except the active
   flag.
2. Starting Song of Healing never populates the secondary slot implicitly.
3. Master of Motifs can add two distinct songs without replacing the primary.
4. Failure or stutter of one slot has the explicitly selected effect on the
   other slot.
5. Stop, spell interruption, disconnect cleanup, and validation failure leave
   identical clean state.
6. Whitespace-only, unknown, capitalized, and ambiguous input cannot
   accidentally select Song of Healing or stop a valid song.
7. A failed replacement leaves the current valid performance unchanged.
8. Efficient Performance starts with a move action when the standard action is
   unavailable; normal performers cannot.
9. Listing and stopping do not queue behind an unavailable standard action.
10. Linkless characters do not retain a room-blocking active state.
11. An NPC's legacy cooldown does not count as an active conflicting song.

### Production-Linked Effect Tests

1. Song of Healing heals an eligible target and creates no more than the
   intended marker count.
2. A later verse refreshes the stable final state without sixteen protocol
   flushes.
3. Songweaver initializes every record and changes only the intended duration
   and potency values.
4. Group members are each processed exactly once; nonmembers are not told they
   received the effect.
5. Two bards' source-tagged lingering effects follow the selected stacking and
   removal rule.
6. Deaf, immune, construct, and saving targets follow each song's declared
   contract.
7. Each row in BP-016 has an explicit mechanics test, including Song of Dragons
   and Song of the Magi.
8. Default, Songweaver, Lingering, and Flight durations match documented real
   time across verse boundaries.

Run the Songweaver and affect-index cases under AddressSanitizer and
UndefinedBehaviorSanitizer where available.

### Performance-Linked Perk Tests

1. Heightened Harmony grants exactly +5 on the first trigger, does not double
   count, refreshes instead of accumulating, and lasts the documented time.
2. Harmonic Casting and Crescendo follow the selected interruption/resource
   contract for damaging, non-damaging, single-target, and area spells.
3. Protective Chorus, Aria of Stasis, Anthem of Fortitude, Battle Hymn, and
   Dominance affect the intended allies only while their required song is
   active.
4. Spellsong Maestra modifies only Bard casts and implements all advertised
   components.
5. Symphonic Resonance targets NPC and player foes according to PK rules, uses
   a save, and runs only after a successful triggering spell.
6. Frostbite Refrain damages the defender once with the selected damage type
   and never damages the attacker.
7. Commanding Cadence and Winter's War March pass caster and victim in the
   correct order and test the intended DC.
8. Winter's War March uses the normal damage/death/resistance path and affects
   the selected room targets exactly once.
9. Every advertised helper has either a behavioral call-site test or is removed
   from the advertised perk.

### Focused Protocol Tests

Extend protocol/output coverage with the real cumulative-capacity behavior:

1. Pre-fill a descriptor so a complete OOB frame does not fit.
2. Attempt to send a reported structured variable.
3. Assert that no prefix of the frame was appended.
4. Assert that the variable remains dirty for retry.
5. Drain the descriptor and retry.
6. Assert that the result contains the opening bytes and final `IAC SE`.
7. Assert that `**OVERFLOW**` never occurs between those boundaries.
8. Repeat for both MSDP and GMCP emission.
9. Build an invalid affect and assert that serialization logs and sends nothing
   without indexing outside lookup tables.

The focused protocol harness passed all twenty current tests on 2026-08-04:

```text
....................

OK (20 tests)
```

The harness now stubs both text and atomic raw output, models the descriptor
capacity, and covers a cumulative full-queue rejection and retry. The
production-linked web onboarding suite separately exercises the real atomic
descriptor append at the cumulative limit. MSDP retry is covered; a dedicated
GMCP backpressure case remains in the protocol acceptance checklist.

## Temporary Operational Mitigation

Until the development repair is deployed, an MSDP client can avoid this
specific decoder failure by not reporting the `AFFECTS` variable. Disabling
MSDP also avoids this path at the cost of other GUI data.

This is only a client-side workaround. It does not fix Songweaver undefined
behavior, duplicate Song of Healing, or the general ability to split another
OOB frame after a sufficiently large output burst.

## Validation Commands After Implementation

```bash
make clean
make -j$(nproc)
make test
make install

cd unittests/CuTest
make protocol-parser
./protocol_parser_tests
```

After `make test`, always run `make install` so the root-level `circle` build
artifact is installed as `bin/circle` and removed from the project root.

## Files Reviewed or in Repair Scope

Core performance and state:

- `src/bardic_performance.c`
- `src/bardic_performance.h`
- `src/players.c`
- `src/interpreter.c`
- `src/actions.c`
- `src/actions.h`
- `src/structs.h`
- `src/utils.h`
- `src/lists.c`

Affects, protocol, and lifecycle:

- `src/handler.c`
- `src/comm.c`
- `src/net/protocol.c`
- `src/net/protocol.h`
- `src/net/onboarding.c`
- `src/limits.c`
- `src/act.informative.c`
- `src/act.other.c`
- `src/mob/mob_class.c`

Songs, spells, combat, and perks:

- `src/magic/magic.c`
- `src/magic/spell_parser.c`
- `src/magic/spell_prep.c`
- `src/combat/fight.c`
- `src/spec_procs.c`
- `src/character/class.c`
- `src/character/feats.c`
- `src/character/perks.c`
- `src/character/perks.h`
- `src/constants.c`
- `src/utils.c`

Player-facing text and tests:

- `lib/text/help/help.hlp`
- `docs/systems/perks/BARD_PERKS.md`
- `unittests/CuTest/test_protocol_parser.c`
- `unittests/CuTest/test_bardic_performance.c`

## Audit Outcome

The Mudlet error is not client-only. Bard affect churn was the known trigger,
and non-atomic server output created the malformed data. Both layers are now
repaired: batching prevents the known burst, while atomic OOB emission prevents
the same corruption class elsewhere.

The wider performance subsystem still needs mechanics and perk reconciliation.
The immediate Songweaver, secondary-slot, state-transition, affect-volume, and
framing risks are contained in the first two repair batches. Later batches will
select and implement one authoritative contract for timing, targets, base
songs, performance resources, and performance-linked perks, then align all
player-facing documentation with those tested mechanics.
