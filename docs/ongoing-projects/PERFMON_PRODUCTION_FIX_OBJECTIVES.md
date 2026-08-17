# PERFMON Production Analysis and Fix Objectives

Status: active engineering plan

Analysis date: 2026-08-17

Evidence source: the production-port `perfmon all` capture retained in
[`todo-now.md`](todo-now.md). The measurement window began after boot and copyover
recovery, as intended by the `PERF_reset()` call in `src/comm.c:745-750`.

This document separates observations, strong inferences, and items that still need
instrumentation. It is not proof that every proposed cause is responsible for every
slow pulse. The first implementation phase deliberately improves attribution before
changing persistence or game behavior.

## Executive conclusion

The best fix angles, in order, are:

1. Remove the synchronous one-minute persistence burst. The capture contains 185
   pulses over one second in a 185.5-minute window, almost exactly one per minute.
   The heartbeat has an unprofiled one-minute save block, and its character save path
   performs a large amount of redundant account SQL.
2. Explain and control live mobile growth before treating the memory result as a
   conventional allocator leak. The observed entity deltas account for nearly all of
   the heap increase using only the build's base structure sizes.
3. Reduce work proportional to all characters, objects, or rooms. At the captured
   population, known full-list sweeps consume about 43.8% of heartbeat time, with
   `mobile_activity` alone consuming 29.4%.
4. Improve PERFMON around tail attribution. Its averages are useful, but most critical
   sections do not collect percentiles, the slow-pulse log is rate-limited, and the
   report does not identify which scheduled tasks coincided on a bad pulse.
5. Investigate the combat-event tail after persistence is isolated. Combat callbacks
   dominate event callback time and reached 623 ms, but the event queue itself remained
   bounded.

The data does not support prioritizing input, output, vessels, the event queue depth,
or pending extraction cleanup as primary causes of current production latency.

## What the capture says

### Derived operating summary

| Signal | Derived result | Meaning |
| --- | ---: | --- |
| Measurement window | 11,129 seconds | 3h 05m 29s since reset |
| Expected 100 ms slots | 111,290 | Ten slots per second |
| Logged outer loops | 108,563 | 2,727 fewer loops because slow work crossed slots |
| Executed heartbeats | 111,280 | Catch-up replay recovered all but 10 logical heartbeats |
| Pulses over 100 ms | 343 (0.32%) | Exactly matches the 343 catch-up passes |
| Pulses over 500 ms | 188 (0.17%) | Severe user-visible stalls |
| Pulses over 1 second | 185 (0.17%) | 0.997 per minute, a strong periodic signal |
| Worst pulse | 4.197 seconds | Entirely inside `heartbeat()` at current granularity |
| Main-loop busy time | 688.66 seconds (6.19%) | Average capacity is acceptable; the tail is not |
| Heartbeat time | 678.44 seconds (6.10%) | 98.5% of measured main-loop work |
| Database executions | 338,189 | 30.39 queries/sec, or 1,823 queries/minute |

`PERF_log_pulse()` measures one outer-loop work interval against the 100 ms budget in
`src/comm.c:1133-1144`. A slow interval becomes `missed_pulses` in
`src/comm.c:1221-1236`, then bounded heartbeat replay runs in
`src/comm.c:1475-1512`. Therefore the 343 over-budget pulses and 343 catch-up passes
are two views of the same incidents, not 686 separate incidents.

The catch-up report says `remaining_backlog=10`, but the code deliberately discards
that remainder instead of carrying it forward. Operationally this means 2,717 missed
heartbeats were replayed and 10 were dropped. PERFMON should use that terminology.

### The once-per-minute signature

The strongest inference in the capture is:

```text
185 pulses over 1 second / 185.48 elapsed minutes = 0.997 per minute
```

The heartbeat runs the following work on the same one-minute boundary:

- General minute maintenance and a memory sample at `src/comm.c:1730-1736`.
- `save_player_pets()`, `save_chars()`, and `artifact_save_if_dirty()` at
  `src/comm.c:1814-1820`.
- Periodic crash/house saves and `update_player_last_on()` at
  `src/comm.c:1863-1878`.

The report does not profile the first two groups or `update_player_last_on()` as
separate sections, so it cannot name the owner of the regular one-second stall.
Nevertheless, the save implementation gives this inference high confidence:

- `save_chars()` saves every connected playing character, whether or not the
  character is dirty (`src/handler.c:3973-3990`).
- `save_char_checked()` calls `save_account()` on each save
  (`src/players.c:4050-4074`).
- `save_account()` performs an account upsert, an update for every account character,
  50 race upserts, 50 class upserts, and two unlock reload queries for each active
  descriptor on that account (`src/account.c:963-1067`). This happens even when the
  account did not change.
- `save_char_pets()` starts a transaction and deletes two owner data sets before
  committing for every player, even when the player has no persistent pets
  (`src/players.c:6595-6718`).
- `update_player_last_on()` adds one synchronous update per descriptor
  (`src/players.c:5789-5875`).
- Character serialization also strips and restores equipment and affects and flushes
  a player file synchronously. It only logs individual saves slower than 260 ms
  (`src/players.c:2272-2335`, `src/players.c:4076-4189`).

With ten playing characters, `save_account()` alone has a lower bound of roughly one
thousand SQL executions per minute before account-character updates, pet rows, object
rows, or unrelated game queries are counted. That is consistent with the observed
1,823 executions per minute.

The 15-minute crash-save path makes the tail worse:

- Twelve `Crash_save_all()` calls consumed 5.49 seconds total.
- Its maximum single call was 2.868 seconds.
- The other eleven calls averaged about 239 ms.
- `House_save_all()` peaked at only 60 ms.
- The worst heartbeat was 4.197 seconds. Subtracting the worst crash save still leaves
  about 1.329 seconds, consistent with the regular minute work stacking on the same
  boundary.

This does not prove that the maximum crash save and maximum heartbeat were the same
incident, because PERFMON has no slow-pulse correlation record. It is the leading
hypothesis to verify.

### Memory growth is strongly explained by live entities

The capture reports:

| Metric | Since reset |
| --- | ---: |
| RSS | +75.32 MiB |
| Anonymous RSS | +73.94 MiB |
| Heap in use | +69.57 MiB |
| Mobiles | +4,756 |
| Objects | +4,001 |
| NPC followers | +478, including +43 charmed |
| Affect nodes | +51 |
| Active events | -4 |
| Pending extractions | 0 |

In the current development binary, GDB reports these build-dependent base sizes:

| Structure | Size |
| --- | ---: |
| `struct char_data` | 14,384 bytes |
| `struct obj_data` | 696 bytes |
| `struct affected_type` | 72 bytes |
| `struct follow_type` | 16 bytes |

Applying only those base sizes to the production deltas gives a conservative estimate:

| Delta | Base allocation estimate |
| --- | ---: |
| 4,756 mobiles | 65.24 MiB |
| 4,001 objects | 2.66 MiB |
| 478 follower nodes and 51 affects | 0.01 MiB |
| Minimum explained allocation | 67.91 MiB |

The 67.91 MiB minimum is about 97.6% of the reported 69.57 MiB heap increase. Mobile
and object strings, scripts, inventories, and other owned allocations are not included,
so the exact production allocation will differ. The direction is still decisive: this
window looks primarily like retained live-entity growth, not invisible heap growth.

The capture does not reveal whether that growth is expected warm-up, a reset maximum
being approached, runaway spawning, or a lifecycle defect. Boot-time world allocations
are excluded because PERFMON resets after boot. Zone resets can create mobiles and
objects (`src/db.c:5082-5132`), but many DG, summon, encounter, vessel, quest, and
special-procedure call sites also call `read_mobile()` or `read_object()`.

Extraction is not visibly stuck: 1,035 pending characters were processed, the maximum
pending batch was 180, and the final pending count was zero. Event depth also ended
slightly below its starting point. The missing information is creation and destruction
by source, prototype, and zone.

### Recurring global sweeps are the main steady-state scaling risk

The following distinct heartbeat sections together used about 297.38 seconds, or 43.8%
of all heartbeat time:

| Section | Total | Average invocation | Current scaling shape |
| --- | ---: | ---: | --- |
| `mobile_activity` | 199.29 s | 1.791 ms per heartbeat | Visits the character population across each six-second cycle |
| `pulse_luminari` | 45.27 s | 20.347 ms per five-second call | Walks all characters |
| `update_damage_and_effects_over_time` | 19.45 s | 10.491 ms per six-second call | Walks all characters |
| `script_trigger_check` | 15.14 s | 17.690 ms per 13-second call | Walks all characters, objects, and 91,729 rooms |
| `proc_update` | 10.35 s | 5.580 ms per six-second call | Walks all 53,512 objects |
| `affect_update` | 7.88 s | 4.249 ms per combat pulse | Walks all characters, then skips most unaffected NPCs |

`mobile_activity_pulse()` already spreads one six-second population pass across pulses
(`src/mob/mob_act.c:586-628`). That is a good latency-control direction, but its cost
still grows with the number of live characters. The entity-growth objective therefore
improves both memory and CPU.

Two lower-risk indexing opportunities stand out:

- `proc_update()` scans every object just to find `ITEM_AUTOPROC` objects
  (`src/comm.c:1571-1588`). Maintain an extraction-safe registry of eligible objects.
- `script_trigger_check()` scans all characters, all objects, and every room just to
  find owners with relevant random triggers (`src/dgscript/dg_scripts.c:667-714`).
  Maintain trigger-owner registries updated by script attach/detach and extraction.

Character-wide mechanical updates need more care because changing cadence changes game
behavior. First split their telemetry into visited, eligible, and acted-on counts. Then
introduce state-specific registries or cursor budgets with production-linked tests that
prove cadence and extraction safety.

`zone_update` is cheap most of the time but reached 296.148 ms. Because it calls at most
one eligible `reset_zone()` per invocation and resets create world entities, per-zone
reset timing and create/extract deltas can connect this latency tail with the population
growth finding. If a reset can exceed its budget after unnecessary loads are repaired,
stage its commands across pulses without exposing players to a partially reset zone.

### Event processing is healthy in volume but has a combat tail

The queue itself is not growing:

- Depth changed from 78 to 71; maximum before processing was 274.
- 97,114 callbacks ran and 26,999 events were created during callbacks.
- `event_process` median was 2 us, p95 was 241 us, and p99 was 1.491 ms.
- Its maximum was 764.956 ms.

Callback attribution points to two owners:

| Callback | Total | Share of `event_process` | Average | Maximum |
| --- | ---: | ---: | ---: | ---: |
| Combat Round | 30.93 s | 66.2% | 6.309 ms | 623.296 ms |
| `trig_wait_event` | 12.07 s | 25.8% | 404.9 us | 21.390 ms |

Together they explain about 92% of event-process time. `event_combat_round()` delegates
to the action queue and `perform_violence()` (`src/combat/fight.c:16037-16104`), so the
current callback label is too coarse to optimize safely. The event tail deserves a
slow-callback recorder and phase-level combat profiling, but only after the periodic
save stall is separated.

One concrete combat-tail candidate is `rune_scimitar`: it accumulated 4.38 seconds and
had a 461.818 ms maximum. Its proc can generate four to seven bonus `hit()` calls
(`src/spec/spec_objects.c:2578-2629`). Correlated slow-callback records should establish
whether this proc or a nested special chain owns the Combat Round maximum before changing
its mechanics. Combat execution also needs a bounded attack/proc-chain guard independent
of this one item.

Vessel work is not a leading target in this capture. The combined vessel tick averaged
147 us per half-second invocation, and its profiled children had low p99 values.

## Prioritized engineering objectives

### P0. Attribute and eliminate the one-minute persistence stall

#### Instrument first

Add sampled PERFMON sections around every top-level minute task:

- `minute.maintenance`
- `minute.memory_sample`
- `minute.pet_save`
- `minute.character_save`
- `minute.artifact_save`
- `minute.crash_save`
- `minute.house_save`
- `minute.last_online`

Add nested measurements for `save_char_checked`, `save_account`, file serialization,
affect/equipment normalization, pet row preparation, pet SQL, object crash save, and
character crash save. Record calls, total, p50, p95, p99, max, failures, bytes, objects,
and queries for each save class.

#### Remove redundant work

1. Decouple account persistence from ordinary periodic character serialization.
   `save_char_checked()` must not rewrite account unlock tables unless account state is
   dirty.
2. Add account dirty generations for core account data, character membership, race
   unlocks, and class unlocks. Persist only changed sets and update in-memory descriptors
   directly after a successful write instead of re-querying unchanged rows.
3. Batch set persistence in transactions or prepared multi-row statements. Never issue
   100 fixed per-slot upserts for every character every minute.
4. Replace the all-character minute burst with a durable incremental scheduler. Bound
   both elapsed microseconds and saves per pulse, preserve fairness, and retain explicit
   save-on-quit/copyover/shutdown behavior.
5. Use the existing `Crash_save_incremental()` foundation
   (`src/obj/objsave.c:1903-1955`) from production scheduling, but extend its contract so
   the caller can distinguish no work, progress, completion, and failure.
6. Add pet dirty tracking. Do not run a delete-and-reinsert transaction for a player
   whose pet state did not change. Spread dirty owners across pulses.
7. Save houses only when dirty, or queue dirty houses under the same elapsed-time budget.
8. Batch `last_online` updates or move them to a lower-frequency, noncritical path while
   preserving operational semantics.

#### Persistence acceptance gates

- No persistence task may monopolize more than 50 ms of a normal pulse.
- Persistence should normally consume no more than 20 ms of a pulse, leaving headroom
  for combat and world activity.
- A 24-hour representative run has zero pulses over 500 ms attributable to scheduled
  persistence and no once-per-minute latency signature.
- Every dirty connected player and pet reaches durable storage within the documented
  maximum interval, including under continuous churn.
- Disconnect, reconnect, descriptor reorder, copyover, partial write, database failure,
  and retry tests prove cursor safety and eventual completion.
- Account and pet query volume scales with changed data, not with fixed array capacity or
  total connected characters.

### P0. Add slow-pulse correlation telemetry

Maintain a bounded in-memory ring of the slowest or most recent over-budget pulses. Each
record should contain:

- Wall and monotonic timestamps, pulse number, and total duration.
- Which heartbeat schedule classes fired: 1 s, 3 s, 5 s, 6 s, 13 s, 30 s, 60 s,
  75 s, autosave, and other long intervals.
- Per-top-level section elapsed time for that pulse.
- Queries executed and SQL time on the main thread during the pulse.
- Event callbacks processed and the slowest event callback.
- Character, mobile, object, descriptor, event, and pending-extraction counts.
- Requested, replayed, and dropped heartbeats.

Expose it as a concise `perfmon slow [count]` report and CSV. Keep identifiers bounded
and staff-safe; do not retain SQL text, credentials, player file contents, or arbitrary
command arguments.

This flight recorder is more valuable than rate-limited log snapshots. Current severity
logging suppresses most incidents and reports whichever sections ran on a logged pulse,
but it cannot preserve correlations across all 185 one-second events.

### P0. Identify and bound entity creation

Add counters for mobile and object creation and destruction, not only current inventory.
At minimum report:

- Created, extracted, and net count since reset.
- Top mobile and object VNUMs by positive net delta.
- Top zones by positive net delta and reset count.
- Creation reason: boot/reset, DG script, spell/summon, encounter, quest/mission, vessel,
  pet restore, special procedure, staff command, or unknown.
- Temporary-entity count, expiry count, and overdue temporary entities.
- Zone reset duration plus mobiles/objects created and removed by that reset.
- Current and delta counts for followers by ownership/lifetime class.

Instrument central constructors and extractors so all call paths are covered. Prefer an
explicit creation-reason enum propagated from major call sites; keep an `unknown` bucket
and make reducing it part of the rollout.

#### Entity acceptance gates

- A post-boot idle and normal-activity time series shows whether mobile and object counts
  plateau.
- Every sustained positive net-growth cohort has an identified source and documented
  lifetime policy.
- Reset-loaded prototypes do not exceed their intended global or room maxima.
- Temporary summons, encounters, helpers, and restored pets have tested extraction paths.
- Anonymous RSS and heap growth flatten when live-entity counts flatten. Only residual
  growth after that point should trigger allocation tracing.

### P1. Replace broad scans with eligible-owner registries

Implement in this order:

1. An `ITEM_AUTOPROC` object registry for `proc_update()`.
2. Random-trigger owner registries for mobile, object, and room DG scripts.
3. An affected-character registry for affect work that currently scans all characters.
4. State-specific character registries or bounded cursors for hazard, regeneration,
   affliction, and other Luminari pulse work.
5. Further mobile-activity eligibility buckets only after creation growth is controlled.

All registries require attach, detach, extraction, OLC replacement, copyover, and zone
reset tests. Add debug validation that periodically compares registry membership with a
full scan without using the full scan as the production hot path.

#### Scan acceptance gates

- PERFMON reports visited, eligible, and acted-on counts for each sweep.
- Cost follows the eligible population rather than all 65,000+ mobiles or all 91,000+
  rooms where semantics permit.
- No cadence, ordering, extraction, or trigger behavior changes in production-linked
  tests.
- Representative-load p99 remains within the assigned per-pulse budget as entity counts
  approach their operational cap.

### P1. Isolate the combat-event tail

Add bounded timing inside `event_combat_round()` and `perform_violence()` for action
queue work, attack generation, specials/procs, scripts, damage resolution, follower or
group fan-out, and output generation. Event callback telemetry should gain rolling p50,
p95, and p99, not only average and maximum.

For slow callbacks, retain event identity plus safe context such as PC/NPC class, mobile
VNUM, room VNUM, participant count, attack count, and proc count. Do not retain free-form
player text.

Acceptance target: no single ordinary combat callback exceeds 100 ms under representative
combat. Explicitly designed mass-combat cases need a documented budget and bounded work
strategy rather than an exemption from measurement.

### P1. Make SQL cost visible and bounded

The process-wide counter proves volume but not ownership or latency. Extend the central
`luminari_mysql_query()` wrapper in `src/mysql.c:63-80` with low-overhead telemetry:

- Main-thread versus worker-thread calls.
- Total, p50, p95, p99, and maximum latency.
- Errors and reconnects.
- Query class and normalized table family without retaining values or full SQL.
- Per-pulse query count and elapsed SQL time for slow-pulse correlation.
- Top call-site category, using explicit subsystem tags where practical.

Acceptance target: routine idle query volume is explained by named subsystems, periodic
persistence query volume is proportional to dirty records, and no synchronous main-loop
query can silently dominate a pulse.

## PERFMON product improvements

### Make the default report decision-oriented

`perfmon all` currently emits fifteen pages in registration order, mostly tiny command
and special-procedure rows. Add these views:

- `perfmon top total [limit]`
- `perfmon top max [limit]`
- `perfmon top p99 [limit]`
- `perfmon slow [count]`
- `perfmon saves`
- `perfmon entities`
- `perfmon sql`

Keep a raw exhaustive view for diagnostics, but make `perfmon all` start with a one-page
health summary and ranked objectives.

### Correct or clarify metric semantics

1. Show the measurement start timestamp, elapsed duration, pulse budget, logged outer
   loops, expected slots, executed heartbeats, replayed heartbeats, and dropped
   heartbeats.
2. Rename `remaining_backlog` to `dropped_missed` unless catch-up behavior is changed to
   retain a real backlog.
3. Label `Total %` as inclusive elapsed-wall percentage. Nested rows such as Main Loop,
   heartbeat, and event callbacks cannot be added as exclusive CPU shares.
4. Print `n/a` for median, p95, and p99 when sampling is disabled. A displayed `0.00`
   currently looks like a measured zero.
5. State that sampled percentiles cover the most recent 16,384 calls while total, average,
   and maximum are cumulative since reset. Prefer bounded histograms if cumulative
   percentiles are required.
6. Do not truncate distinct section names to the same 24-character display prefix.
7. Add milliseconds beside pulse percentages; `4196.80%` is less immediately useful than
   `4196.8 ms (4196.8%)`.
8. Label interval rows as rolling completed windows so partial minute/hour behavior is
   clear.

### Sample the sections that matter

Sampling is currently enabled for `event_process`, vessel sections, vessel schedules,
and pending extraction, but not for Main Loop, heartbeat, mobile activity, global update
sweeps, saves, or SQL. This is why most high-impact rows show zero percentiles.

Enable bounded sampling or histograms for:

- Main Loop and heartbeat.
- Every top-level scheduled heartbeat block.
- Mobile activity and the major global sweeps.
- Character, account, pet, crash, and house persistence.
- Zone reset.
- Main-thread SQL.
- Combat Round and other top event callbacks.

Report PERFMON's own memory overhead: section count, sampled section count, sample bytes,
event registry bytes, and slow-pulse ring bytes. Sample buffers are allocated lazily after
reset and otherwise appear as unexplained heap growth.

### Retain a real memory time series

The current assessment divides net growth since reset by total elapsed time
(`src/perfmon.c:1643-1680`, `src/perfmon.c:1797-1804`). One baseline and one current
sample cannot show a plateau, distinguish warm-up from steady growth, or correlate
changes in time.

Keep a bounded ring sampled every minute with at least:

- RSS, anonymous RSS, heap in use, arena, mmap, and swap.
- PCs, mobiles, objects, followers, affects, events, descriptors, and pending
  extractions.
- Created and extracted counts by major reason.
- Queries and heartbeat tail counters for the interval.

Report short, medium, and long slopes plus entity-normalized residuals. Reuse the current
sample when calculating rates: `PERF_memory_periodic_check()` currently samples once and
then `PERF_memory_growth_rate()` samples all entity lists again
(`src/perfmon.c:1683-1713`).

Use these status meanings:

- `GROWING WITH ENTITIES`: memory growth is substantially explained by live cohorts.
- `WARMING`: counts are still moving toward a known cap.
- `STABLE`: recent slopes and counts remain within bounds.
- `UNEXPLAINED GROWTH`: memory rises after entity counts stabilize.
- `CRITICAL HEADROOM`: projected use approaches an operational memory limit, regardless
  of whether the allocation is technically a leak.

## Recommended delivery sequence

### Phase 1: attribution

- Add minute-task, save, SQL, and critical-section sampling.
- Add the slow-pulse ring and correct catch-up terminology.
- Add mobile/object create-extract counters and a one-minute memory/entity ring.
- Run a representative development load and confirm the one-minute owner.

### Phase 2: persistence repair

- Stop unconditional account rewrites from character saves.
- Add dirty account and pet persistence.
- Schedule character, crash-object, pet, and house work incrementally under a time budget.
- Verify durability, fairness, recovery, and latency gates.

### Phase 3: entity lifecycle repair

- Rank net growth by VNUM, zone, and creation reason.
- Repair the dominant unintended cohort or document and enforce its cap and lifetime.
- Demonstrate an entity and memory plateau over a representative long run.

### Phase 4: steady-state scaling

- Add autoproc and DG trigger-owner registries.
- Convert other broad sweeps only where measurements justify the complexity.
- Profile and bound the combat-event tail.

## Definition of done

This production-health project is complete only when retained evidence demonstrates all
of the following:

- The once-per-minute one-second stall is gone.
- Persistence is incremental, dirty-driven, durable, and bounded per pulse.
- SQL volume and latency are attributable and no longer scale with fixed account array
  capacity on every character save.
- Mobile and object creation sources are known, operational caps are enforced, and live
  counts plateau under representative use.
- Anonymous RSS and heap usage plateau with live entities, or any remaining growth has a
  separately identified and bounded owner.
- Broad-scan and combat tails meet documented budgets without behavior regressions.
- PERFMON can identify the owner and schedule context of every future over-budget pulse
  without requiring a debugger or an unbounded log stream.
