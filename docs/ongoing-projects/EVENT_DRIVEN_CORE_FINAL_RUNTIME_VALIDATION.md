# Event-Driven Core Final Runtime Validation

**Date:** 2026-09-01

**Branch:** `event-driven-core-refactor`

## Result

The default-native product passed the complete tranche 7 runtime gate. The
copied production world runs one timing wheel with demand-driven owner events,
preserves live descriptors across copyover, exposes entity and script events to
immortals, and uses less CPU than the optimized rollback loop under the same
controlled workload.

## Production-World Evidence

The copied world loaded 762 zones, 91,735 rooms, 27,067 mobile prototypes, and
22,637 object prototypes. After boot and copyover, `eventdebug` reported:

- 41,960 live timed events and 272 sealed semantic types;
- 38,743 autonomous mobile agendas, including 36,912 wander agendas;
- 192,309 autonomous-agenda callbacks during the observed run;
- zero ready or overdue events;
- zero callback failures, admission rejections, owner mismatches, or stale
  owners; and
- zero late, skipped, missed, or coalesced callbacks.

The mobile agendas remain active when no player is nearby. A concrete owner
wakes only for its admitted responsibility, performs that responsibility, and
reschedules or retires. There is no normal-path sweep over every loaded mobile.

## Performance Correction

An optimized 60-second comparison initially measured the native product at
27.03% of one CPU core and the rollback loop at 3.33%. Per-type profiling showed
that gameplay callbacks were not responsible. The scheduler bridge was taking
two complete diagnostic snapshots per dispatch pass; each snapshot traversed
about 42,000 events and 42,000 owners.

The hot path now reads constant-time queue-depth counters. Full snapshots remain
available to explicit diagnostic commands. Repeating the same optimized
full-world measurement produced 2.70% for the native product, below the 3.33%
rollback baseline. The architecture gate requires the constant-time accessors
and rejects a return to full diagnostic scans during dispatch.

Native semantic callbacks now also feed the existing per-type performance
profiles. `eventdebug types` therefore reports live count, schedules, callbacks,
recurrences, and callback time for native types without repeatedly scanning the
whole wheel.

## Live MUD And Copyover

The live immortal session exercised the compact entity filters:

```text
eventdebug player <name> [limit]
eventdebug mob <name> [limit]
eventdebug object <name> [limit]
eventdebug room <here|vnum> [limit]
eventdebug scripts <player|mob|object|room> <target> [limit]
```

`eventdebug scripts mob Puff 2` selected exactly Puff's `dg.random_trigger`, and
`eventdebug mob Puff 2` showed the same owner event in the complete entity view.
Payload data stayed redacted, default output fit 80 columns, and the hard limit
remained 120 columns.

A real same-process copyover retained the test descriptor and resumed command
handling. Post-copyover `eventdebug` remained healthy, and the expected durable
copyover diagnostic and performance snapshots were written successfully.

## Automated Validation

- Normal and default-disabled rollback-quarantine builds: pass.
- Explicit rollback build: pass.
- Five syntax-boot build/runtime combinations: pass.
- CTest: 19/19 targets, including 1,052 production-linked CuTests.
- ASan plus UBSan: 1,052/1,052 tests, no findings.
- Strict Valgrind with child tracing: 33 process logs, no errors and no
  definite, indirect, or possible leaks.
- Native architecture, demand-driven architecture, legacy admission, and
  retired-PubSub source contracts: pass.

The temporary validation listener is stopped. Port 4103 remains intentionally
down at the maintainer's request.

## Remaining Work

Tranche 8 is the final adversarial source/spec/runtime audit and permanent
maintainer documentation. Physical deletion of rollback code and archival
PubSub schema remains outside this development acceptance until the external
stable-release and operator approval gate is satisfied.
