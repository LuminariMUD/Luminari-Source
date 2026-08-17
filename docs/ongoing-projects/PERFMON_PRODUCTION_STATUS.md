# PERFMON Production Status and Next Actions

Status: major latency repair validated for 77 minutes; production acceptance remains open

Analysis date: 2026-08-17

This document consolidates the original production baseline and the first two production
captures after the PERFMON and persistence changes. It replaces the raw capture in
`todo-now.md` and the implementation-oriented
`PERFMON_PRODUCTION_FIX_OBJECTIVES.md` plan.

The implementation is complete in the codebase. The remaining work is operational:
correct two registry discrepancies, remove the remaining bounded latency owners, control
zone-driven entity growth, and retain a representative complete inter-copyover production capture.

## Executive assessment

The primary repair is working.

- The original once-per-minute one-second stall has not appeared during 77.6 minutes of
  post-change measurement.
- No post-change pulse exceeded 500 ms. The old maximum was 4.197 seconds; the new
  cumulative maximum is 369.7 ms.
- Heartbeat busy time fell from 6.10% to 3.73%.
- No heartbeat has been dropped after deployment.
- SQL volume fell from 30.39 queries/second to 1.17 queries/second cumulatively, and to
  0.47 queries/second after the initial 16-minute warm-up.
- Incremental persistence completed all 77 cycles with no failures or queued work.
- Combat and event processing no longer show the old 623-765 ms tail.

The project is not finished. Four findings prevent final acceptance:

1. The first post-change capture found two autoproc registry discrepancies and six
   affected-character registry discrepancies. The later pasted report ended before the
   validation rows, so correction is not yet demonstrated.
2. A zone update produced a 349.2 ms section and a 369.7 ms pulse.
3. Dirty crash-object persistence still performs a roughly 200 ms
   `DELETE player_save_objs` query and can produce a 250-340 ms pulse.
4. Mobiles and objects have not plateaued. Zone resets account for nearly all retained
   growth, with zones 20222 and 20230 providing clear high-growth cohorts.

The practical player-facing result should be a substantially smoother game without the
regular multi-second freeze. Rare quarter-second hitches remain possible during a large
dirty inventory save or an expensive zone reset. A roughly 120 ms scheduling collision
also remains visible every five minutes.

## Evidence windows

All percentages use a 100 ms pulse budget. The post-change captures belong to the same
PERFMON run, which began at 2026-08-17 18:01:49 UTC. Capture B is cumulative; the
"A to B interval" values below were derived by subtracting Capture A counters from
Capture B counters.

| Window | Duration | Purpose |
| --- | ---: | --- |
| Original baseline | 11,129 seconds (185.5 minutes) | Pre-change production behavior |
| Capture A | 966 seconds (16.1 minutes) | First post-change warm-up and initial crash-save cycle |
| Capture B | 4,654.5 seconds (77.6 minutes) | Longer cumulative post-change observation |
| A to B interval | 3,688.5 seconds (61.5 minutes) | Best available approximation of post-warm-up behavior |

The post-change population was initially lower than the old capture, so small reductions
in population-scaled work cannot be assigned entirely to code changes. The disappearance
of multi-second pulses and the SQL reduction are much larger than the population
difference.

## Implemented controls represented in the captures

The consolidated status depends on the following completed code changes:

- A fair incremental persistence scheduler processes one player, pet, artifact, crash
  inventory, or dirty house unit per pulse and retains explicit quit, copyover, and
  shutdown durability paths.
- Account core data, membership, race unlocks, and class unlocks have independent dirty
  generations. Changed sets are batched instead of rewriting fixed-capacity arrays on
  every character save.
- Pet persistence uses state fingerprints to skip unchanged owners.
- Main-thread and worker-thread SQL telemetry records bounded percentiles, errors,
  reconnects, normalized query families, and slow-pulse correlation without retaining
  query values.
- A 128-record slow-pulse ring retains schedule, section, SQL, event, entity, extraction,
  and catch-up context.
- Mobile/object lifecycle counters attribute creation and extraction by reason, VNUM,
  and zone, while reset telemetry records duration and entity deltas.
- Extraction-safe autoproc, DG random-owner, and affected-character registries replace
  selected full-population scans and expose validation counts.
- Combat phase telemetry and 128-attack/128-proc guards bound pathological chains.
- A one-day minute memory ring reports short, medium, and long slopes plus
  entity-normalized residual growth.

Capture B reports about 10.0 MiB of bounded PERFMON storage: 6.0 MB of section samples,
4.1 MB for the event registry, 136 KiB for slow pulses, and 78 KiB for SQL telemetry.
This is expected monitoring capacity, not an unbounded growth source.

## Before-and-after results

### Pulse and catch-up health

| Signal | Original baseline | Capture A | Capture B | Assessment |
| --- | ---: | ---: | ---: | --- |
| Main-loop inclusive time | 6.19% | 3.99% | 3.79% | 38.8% below baseline |
| Heartbeat inclusive time | 6.10% | 3.92% | 3.73% | 38.9% below baseline |
| Maximum pulse | 4,196.8 ms | 340.5 ms | 369.7 ms | 91.2% lower |
| Pulses over 100 ms | 343 (0.32%) | 14 (0.15%) | 30 (0.06%) | Cumulative incidence down 79.6% |
| Pulses over 250 ms | 234 (0.22%) | 3 (0.03%) | 5 (0.01%) | Severe tail is now rare |
| Pulses over 500 ms | 188 | 0 | 0 | No recurrence |
| Pulses over one second | 185 | 0 | 0 | Old minute signature absent |
| Requested missed slots | 2,727 | 23 | 43 | See normalized rates below |
| Replayed missed slots | 2,717 | 23 | 43 | All post-change misses recovered |
| Dropped missed slots | 10 | 0 | 0 | No post-change loss |

The original run requested 14.70 catch-up slots per minute. Capture B averages 0.55 per
minute, and the A to B interval averages 0.33 per minute. The latter is a 97.8% reduction
from baseline.

During the A to B interval, only 16 of 36,865 outer loops exceeded 100 ms (0.043%), and
only two exceeded 250 ms. No pulse exceeded 500 ms.

### Persistence and SQL

The incremental scheduler is healthy at the cycle level:

| Scheduler signal | Capture A | Capture B | A to B addition |
| --- | ---: | ---: | ---: |
| Cycles started/completed | 16/16 | 77/77 | 61/61 |
| Operations | 461 | 2,170 | 1,709 |
| Failures | 0 | 0 | 0 |
| Operations over 20 ms target | 86 | 394 | 308 |
| Operations over 50 ms diagnostic limit | 8 | 9 | 1 |
| Maximum operation | 339.5 ms | 339.5 ms | No new cumulative maximum |

Ordinary character persistence remains below the 50 ms diagnostic limit in Capture B:

| Character-save percentile | Duration |
| --- | ---: |
| Median | 20.44 ms |
| P95 | 35.46 ms |
| P99 | 41.73 ms |
| Maximum | 45.32 ms |

The 20 ms target is therefore a useful optimization target rather than a currently
enforced operation deadline. The scheduler limits work to one unit per pulse, but it
cannot preempt an individual synchronous operation.

SQL volume shows the largest sustained improvement:

| Window | Queries | Rate | Change from baseline |
| --- | ---: | ---: | ---: |
| Original baseline | 338,189 | 30.39/sec | - |
| Capture A | 3,703 | 3.83/sec | 87.4% lower |
| Capture B cumulative | 5,443 | 1.17/sec | 96.2% lower |
| A to B interval | 1,740 | 0.47/sec | 98.4% lower |

There were no SQL errors or reconnects. Fixed 50-slot race and class unlock rewrites no
longer dominate the query list, ordinary character saves do not blindly rewrite account
sets, and unchanged pet state is usually skipped.

The remaining SQL tail is narrowly identified:

| Query family | Calls | Average | Maximum | Total |
| --- | ---: | ---: | ---: | ---: |
| `crash_object:delete.player_save_objs` | 10 | 199.4 ms | 223.2 ms | 1.994 sec |
| `crash_object:insert.player_save_objs` | 2,672 | 101.6 us | 258 us | 271.4 ms |

Only one crash-object delete occurred during the additional 61 minutes, demonstrating
that dirty selection works. When it does run, wholesale deletion and many individual
row inserts still make one player save too large for the pulse budget.

Before changing the query, confirm the production table has an effective index on
`player_save_objs.name`. Then prefer a bounded snapshot replacement, batched insert, or
worker-safe design that preserves transactional durability without blocking the main
loop for 200-340 ms.

### Remaining slow-pulse owners

The flight recorder is doing its job: every displayed slow pulse has schedule, section,
SQL, event, and entity context.

#### Zone update

The largest Capture B pulse was:

```text
pulse duration:          369.694 ms
zone_update:             349.205 ms
extract_pending_chars:    17.735 ms
SQL:                       0.000 ms
```

This is the next isolated latency owner. Identify the reset zone associated with that
pulse, then determine whether the cost belongs to reset commands, extraction work,
scripts, or reset-queue handling. If an otherwise-correct reset cannot fit its budget,
stage its work without exposing players to a partially reset zone.

#### Five-minute schedule collision

Pulses 36,000, 39,000, 42,000, and 45,000 were 118-125 ms. They coincide with most of
the long-period schedules:

- Old skool tick: about 23-28 ms.
- `pulse_luminari`: about 18-20 ms.
- Incremental character save: about 12-13 ms.
- Damage and effect update: about 9-10 ms.
- Memory sampling: about 6-9 ms.
- Mobile activity, event callbacks, and occasional script or command work.

No single section owns the overrun. Staggering safe maintenance schedules across nearby
pulses should remove most of this repeatable tail without changing task cadence.

### Global sweeps and steady-state work

The eligible-owner registries greatly reduced candidate populations in Capture A:

| Sweep | Old population proxy | Registry members | Approximate scan reduction |
| --- | ---: | ---: | ---: |
| ITEM_AUTOPROC objects | 53,512 objects | 441 | 99.2% |
| DG random mobile owners | 65,893 mobiles | 2,206 | 96.7% |
| DG random object owners | 53,512 objects | 2 | More than 99.99% |
| DG random room owners | 91,729 rooms | 194 | 99.8% |
| Affected characters | 65,893 mobiles plus PCs | 22 | More than 99.9% |

DG mobile, object, and room validation reported zero discrepancies. Autoproc validation
reported two discrepancies. The affected registry contained 22 members while the same
capture reported 28 characters with affects, producing six discrepancies. Because these
registries control behavior, nonzero validation is a correctness defect, not merely a
monitoring warning. Run `perfmon entities` again and repair attach, detach, flag-change,
or extraction paths until every mismatch is zero.

Measured recurring work now looks like this:

| Section | Original average | Capture B average | Result |
| --- | ---: | ---: | --- |
| `script_trigger_check` | 17.69 ms | 9.85 ms | 44.3% lower |
| `mobile_activity` | 1.79 ms/heartbeat | 1.66 ms/heartbeat | 7.3% lower |
| `pulse_luminari` | 20.35 ms | 20.63 ms | Essentially unchanged |
| `update_damage_and_effects_over_time` | 10.49 ms | 10.75 ms | Essentially unchanged |

Further character-state registries or bounded cursors should be measurement-driven and
must preserve cadence, ordering, extraction, and gameplay behavior.

### Combat and events

The original event tail was dominated by combat:

- `event_process` reached 765.0 ms.
- Combat Round reached 623.3 ms.
- Combat Round averaged 6.31 ms.

Capture B reports:

- `event_process` average 203.7 us, p95 1.14 ms, p99 2.01 ms, maximum 26.96 ms.
- 2,891 combat callbacks.
- Zero callbacks over 100 ms.
- Zero attack or proc chain-limit activations.
- Zero rejected attacks or procs.

This is a strong provisional pass. The 128-attack and 128-proc guards did not alter
ordinary observed combat.

## Entity growth and memory

### Entity lifecycle attribution

The old report showed growth but could not identify its source. The new counters show
that zone resets own nearly all retained growth.

| Signal | Original baseline | Capture A | Capture B |
| --- | ---: | ---: | ---: |
| Net mobiles | +4,756 | +877 | +2,969 |
| Net objects | +4,001 | +400 | +1,903 |
| Zone-reset mobile net | Unknown | +877 | +2,963 |
| Zone-reset object net | Unknown | +345 | +1,832 |
| Pending extractions at report | 0 | 0 | Not present in pasted ending; slow samples were 0-1 |

The A to B interval added 2,092 mobiles (34.0/minute) and 1,503 objects
(24.5/minute). Mobile growth slowed from 54.5/minute during Capture A but remains above
the old run's average of 25.6/minute. Object growth has not materially slowed.

Two cohorts are already actionable:

#### Zone 20222: invasion mobiles

- 16 resets created 560 net mobiles with no matching extraction in the report.
- VNUMs 2022270-2022274 account for the complete 560-mobile cohort.
- The zone file loads 35 invasion mobs per successful reset.
- Each prototype uses a global maximum of 500, permitting a large retained population
  before the reset commands naturally stop loading more.

This may be intentional invasion behavior, but the intended lifetime and operational
cap must be documented. If accumulation is unintended, use correct global or room
maxima and verify extraction when the invasion ends.

#### Zone 20230: room fixture objects

- Six resets produced a cumulative object net of +294.
- Object VNUM 2023000 alone reached +288.
- The zone places this object in 48 rooms while using a global maximum of 5,000.

This looks like a room-fixture maximum problem: each reset can add another copy to each
room until the very high global cap is reached. Confirm intended semantics and enforce
one appropriate instance per room where applicable.

Do not add a generic global mobile/object cap. Summons, quests, encounters, vessels,
pets, and reset-loaded fixtures have different lifetime contracts. Apply caps or expiry
rules to measured positive-net cohorts and test their intended gameplay behavior.

### Memory interpretation

Capture A was the only post-change excerpt containing the full memory dashboard:

- RSS increased 16.81 MiB in 16.1 minutes.
- Heap in use increased 13.43 MiB.
- Base entity sizes explained 12,630.9 KiB, or 91.8% of heap growth.
- Entity-normalized residual heap growth was about 40-70 KiB/minute.
- Only 17 one-minute samples existed, so short, medium, and long slopes represented
  nearly the same warm-up interval.

The original baseline similarly had at least 97.6% of heap growth explained by base
mobile and object sizes. The evidence therefore continues to favor retained live
entities over an invisible allocator leak. It does not prove a plateau. Capture B was
truncated before its memory dashboard, and entity counts were still rising.

Only investigate a conventional allocation leak after live cohorts flatten and the
entity-normalized residual remains positive over a representative interval.

## Acceptance status

| Requirement | Current status | Evidence or gap |
| --- | --- | --- |
| Once-per-minute one-second stall removed | Provisional pass | Zero pulses over one second in 77.6 minutes |
| No persistence pulse over 500 ms | Provisional pass | Zero total pulses over 500 ms; full inter-copyover proof pending |
| Incremental cycles finish and retry safely | Provisional pass | 77/77 complete, zero failures; fault-path tests exist, production failure not observed |
| Ordinary persistence remains bounded | Partial | Character saves stay below 50 ms; crash-object saves do not |
| Query volume follows changed state | Provisional pass | Post-warm-up rate is 98.4% below baseline |
| Every slow pulse is attributable | Pass for displayed records | Flight recorder identified persistence, zone update, and schedule collision owners |
| Combat callbacks remain below 100 ms | Provisional pass | 2,891 callbacks, zero slow or limited callbacks |
| Registry validation is exact | Fail/unverified | Autoproc mismatch 2 and affected mismatch 6 in Capture A; Capture B rows unavailable |
| Entity cohorts plateau with documented lifetimes | Fail | +2,969 mobiles and +1,903 objects; dominant reset cohorts identified |
| Memory plateaus with entities | Unverified | Capture B memory rows unavailable and entities still growing |
| Broad-scan costs meet budgets | Partial | DG work improved; Luminari and damage/effect passes remain population-scaled |
| `perfmon all` is decision-oriented | Partial | Summary is useful, but the zone list expanded the report to 27 pages |

## Prioritized next actions

### P0: correctness and remaining severe tails

1. Re-run registry validation and repair all autoproc and affected-character
   discrepancies. Add regression coverage for the production lifecycle that creates
   each missing or stale member.
2. Identify the zone behind the 349.2 ms `zone_update`; profile its reset phases and
   repair the specific reset, extraction, or script owner.
3. Verify the production `player_save_objs.name` index and redesign dirty inventory
   replacement so one owner cannot block a pulse for 200-340 ms.
4. Confirm and enforce intended lifetime/cap policy for zones 20222 and 20230, then
   continue ranking positive-net VNUMs and zones.

### P1: tail smoothing and scaling

1. Stagger compatible five-minute maintenance tasks while retaining their cadence.
2. Reduce `pulse_luminari` and damage/effect work only with state-specific registries or
   bounded cursors backed by behavior tests.
3. Continue observing combat under representative mass-combat scenarios; document any
   intentionally larger budget rather than exempting it from measurement.

### P2: PERFMON report usability

`perfmon all` grew from 15 to 27 pages because it prints every zone with reset or
lifecycle activity. Keep the concise health summary, but rank and cap zone rows by net
growth and reset duration. Put the exhaustive zone list behind an explicit raw or CSV
view. This preserves the evidence while making the default report operationally useful.

## Required production follow-up

The nightly automatic copyover normally runs at 08:00 UTC and recovery starts a new
PERFMON window. Immediately before each accepted copyover, the server atomically
overwrites `log/perfmon-pre-copyover.txt` with the complete current snapshot. A failed
snapshot leaves the previous complete file intact and does not block copyover. Retain
that file after a representative full inter-copyover window covering idle time, player
activity, combat, zone resets, and scheduled saves.

Use:

- `perfmon slow 128 csv` for all retained slow-pulse correlations.
- `perfmon saves` for cycle completion, failures, and budget overruns.
- `perfmon sql csv` for volume, latency, and query ownership.
- `perfmon entities csv` for registry validation and positive-net VNUM/zone cohorts.
- `perfmon combat 64 csv` for callback tails and chain limiting.
- `perfmon top p99 20` and `perfmon top max 20` for ranked section tails.
- `perfmon csv` for a complete machine-readable start/end record.

Final acceptance requires a representative complete inter-copyover run, normally just
under 24 hours, demonstrating:

- No recurrence of the once-per-minute stall and no persistence-attributed pulse over
  500 ms.
- Completed persistence cycles, zero unexplained failures, and a documented maximum
  dirty-owner durability interval.
- Zero registry validation mismatches.
- Documented caps/lifetimes for every sustained positive-net entity cohort and an
  observed entity plateau.
- Flat anonymous RSS and heap after entity stabilization, or a separately identified
  and bounded residual owner.
- Ordinary combat callbacks below 100 ms with no unexplained chain truncation.
- Attribution for every over-budget pulse retained by the flight recorder.
