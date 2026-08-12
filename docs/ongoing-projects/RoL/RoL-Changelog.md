# Realms of Luminari Project Changelog
**Previous Changelog entries can be found in changelog-archive/**

This file records completed milestones removed from the active
[feature-first conversion plan](REALMS_OF_LUMINARI_FEATURE_FIRST_CONVERSION_PLAN.md)
and [zone conversion scope](REALMS_OF_LUMINARI_ZONE_CONVERSION_SCOPE.md). The plans
retain only forward-looking requirements, decisions, phases, and acceptance gates.

## 2026-08-12 - Phase 6 guild-family adapters

Status: Completed checkpoint; Phase 6 direct-binding reconciliation in progress

### Delivered

- Closed eight source handler families and eight active direct bindings in one shared-system
  batch.
- Reused `RoL Guild Guard` for six Bloodstone class guards. Converted room rules preserve
  the source warrior/Antipaladin, cleric/Shaman, assassin/rogue, sorcerer/specialist mage,
  thief/rogue, and necromancer/lich admission groups through target class identities.
- Preserved Bloodstone's active-passage behavior: accepted characters are moved to the
  exact converted destination even when the reset-controlled entrance door is closed.
  Rejected classes remain blocked, and all six guards retain guild-guardian combat
  protection.
- Added the room-owned `RoL Bard Guild Room` and reused it for `guild_bard` and
  `guild_battlechanter`. Both source families map to target Bard eligibility under the
  multiclass model while delegating practice, training, and boosts to the native guild
  service.
- Extended converter persistence, registry and OLC visibility, builder and database help,
  manual testing, focused conversion checks, and production characterization.
- Reconciliation now resolves 831 of 1,147 active direct bindings and 302 of 562 source
  handlers; 316 bindings and 260 handlers remain. The independent `ACT_SPEC` checkpoint
  advances to 747 resolved / 101 pending.
- Archived the thirty-fifth Phase 6 delivery session. The remaining irregularity keeps the
  conservative 7-14 session Phase 6 envelope and the 55-90 session forecast for Phases
  6-8, or 110-360 focused engineering hours.

### Acceptance evidence

```text
Delivery commit: 99cb8aed
Reconciliation path: lib/rol-conversion/runs/phase6-special-20260812-guild-families
Reconciliation run: rol-phase6-special-33a4bb8a0371811c
Active direct bindings: 1,147
Direct bindings resolved: 831
Direct bindings pending: 316
Source handlers resolved: 302
Source handlers pending: 260
Additional handler families resolved: 8
Additional direct bindings resolved: 8
Native adapted bindings: 437
Native adapted composable bindings: 131
ACT_SPEC records resolved: 747
ACT_SPEC records pending: 101
Complete world-tool suite: 291 passed
Focused conversion suite: 87 passed
Production-linked CuTest suite: 645 passed
Documentation findings: 0 errors, 0 warnings, 0 info
Help SQL checks: 4 passed after applying the migration twice
Warning-free Autotools build, test, and install: passed
Root-level circle artifact: absent
Live target writes: 0
```

Phase 6 continues with the remaining 316 direct bindings across 260 source handlers.
The bulk classifier will keep grouping compatible source mechanics into shared native or
strict generated adapters, with focused checks inside each batch and full repository gates
only at checkpoint boundaries.

## 2026-08-12 - Phase 6 high-fanout special adapters

Status: Completed checkpoint; Phase 6 direct-binding reconciliation in progress

### Delivered

- Closed three source handler families and 11 active direct bindings selected from the
  pending inventory by binding fan-out and compatible source shape.
- Reused `RoL Alert Caller` for four Elemental Tower callers. Converted-VNUM profiles
  preserve each caller's authored message, three designated helper identities,
  same-zone helper checks, and once-per-fight behavior.
- Added the object-owned `RoL Portal Door` for four converted portals. It preserves
  `LOOK IN`, exact-object selection, destination remapping, the player level-20 gate,
  good/evil race rejection, immortal bypass, arena-boundary parity, and room-visible
  transport. Target destination safety replaces the source death-room extraction path,
  and invalid destinations remain explicit staff-visible diagnostics.
- Added the mobile-owned `RoL Fixed Bodyguard` for three Icecrag bodyguards. Converted
  profiles preserve the exact protected-mobile assignments and rescue an assigned,
  colocated mobile when it has an attacker.
- Extended converter persistence, registry and OLC visibility, builder and database
  help, manual testing, focused conversion checks, and production characterization.
- Reconciliation now resolves 823 of 1,147 active direct bindings and 294 of 562 source
  handlers; 324 bindings and 268 handlers remain. The independent `ACT_SPEC` checkpoint
  advances to 741 resolved / 107 pending.
- Archived the thirty-fourth Phase 6 delivery session. The remaining irregularity keeps
  the conservative 7-14 session Phase 6 envelope and the 55-90 session forecast for
  Phases 6-8, or 110-360 focused engineering hours.

### Acceptance evidence

```text
Delivery commit: 1108fb56
Reconciliation path: lib/rol-conversion/runs/phase6-special-20260812-high-fanout
Reconciliation run: rol-phase6-special-cedd3394da53d442
Active direct bindings: 1,147
Direct bindings resolved: 823
Direct bindings pending: 324
Source handlers resolved: 294
Source handlers pending: 268
Additional handler families resolved: 3
Additional direct bindings resolved: 11
Native adapted bindings: 429
Native adapted composable bindings: 131
ACT_SPEC records resolved: 741
ACT_SPEC records pending: 107
Complete world-tool suite: 291 passed
Focused conversion suite: 87 passed
Production-linked CuTest suite: 645 passed
Documentation findings: 0 errors, 0 warnings, 0 info
Help SQL checks: 4 passed after applying the migration twice
Warning-free Autotools build, test, and install: passed
Root-level circle artifact: absent
Live target writes: 0
```

Phase 6 continues with the remaining 324 direct bindings across 268 source handlers.
The bulk classifier will keep prioritizing high-fanout compatible shapes, with focused
checks inside each batch and full repository gates only at checkpoint boundaries.

## 2026-08-12 - Phase 6 cross-zone source-periodic expansion

Status: Completed checkpoint; Phase 6 direct-binding reconciliation in progress

### Delivered

- Closed eight additional source handler families and nine active direct bindings in one
  cross-zone generator expansion through the existing mobile-owned
  `RoL Source Periodic` procedure.
- Bulk-screened pending `PROC_INITIALIZE` mobile callbacks across the active source tree,
  then admitted only regular random speech, social, and room-action profiles. Command,
  combat, teleport, quest, economy, and other mixed-mechanics callbacks remain pending.
- Expanded the generated adapter from 82 to 90 source families and from 86 to 95
  converted mobile profiles. The source-hashed tables now preserve 354 random outcomes
  and 588 ordered speech or room-visible actions across Bloodstone, Icecrag, Menden,
  Fun, Mobile, and Realm source files.
- Made the awake precondition profile-specific. Fun mobile 2001230, jester 2003069, and
  cricket 2014048 retain their source behavior without an awake gate; all previously
  selected profiles retain their prior gates. Combat suppression also remains
  profile-specific.
- Updated converter reconciliation, production characterization, generated-table tests,
  builder and database help, and the manual test matrix.
- Reconciliation now resolves 812 of 1,147 active direct bindings and 291 of 562 source
  handlers; 335 bindings and 271 handlers remain. The independent `ACT_SPEC` checkpoint
  advances to 738 resolved / 110 pending.
- Archived the thirty-third Phase 6 delivery session. The 271 remaining handlers retain
  the conservative 7-14 session Phase 6 envelope at 20-45 related families per batch;
  the Phases 6-8 forecast remains 55-90 sessions, or 110-360 focused engineering hours.

### Acceptance evidence

```text
Delivery commit: 2844472f
Reconciliation path: lib/rol-conversion/runs/phase6-special-20260812-periodic-expansion
Reconciliation run: rol-phase6-special-ebf492f80a4ac682
Active direct bindings: 1,147
Direct bindings resolved: 812
Direct bindings pending: 335
Source handlers resolved: 291
Source handlers pending: 271
Additional handler families resolved: 8
Additional direct bindings resolved: 9
Generated profiles: 95
Generated outcomes: 354
Generated room-visible actions: 588
Native adapted bindings: 418
Native adapted composable bindings: 131
ACT_SPEC records resolved: 738
ACT_SPEC records pending: 110
Complete world-tool suite: 290 passed
Focused conversion/generator suite: 6 passed
Production-linked CuTest suite: 643 passed
Documentation findings: 0 errors, 0 warnings, 0 info
Help SQL checks: 4 passed after applying the migration twice
Warning-free Autotools build, test, and install: passed
Root-level circle artifact: absent
Live target writes: 0
```

Phase 6 continues with the remaining 335 direct bindings across 271 source handlers.
The bulk classifier will continue grouping compatible mechanics before generator or
shared-runtime implementation; full repository gates remain checkpoint-level work.

## 2026-08-12 - Phase 6 generated state-aware Waterdeep profiles

Status: Completed checkpoint; Phase 6 direct-binding reconciliation in progress

### Delivered

- Closed 26 state-aware Waterdeep source handler families and direct bindings in one
  bulk-generated workstream through the persistent, mobile-owned
  `RoL Stateful Periodic` procedure.
- Added an explicit source manifest and strict generator for idle and fighting tables in
  `specs.waterdeep.c`. The checked-in table carries a digest of its assessed source inputs,
  and sorted mobile and outcome tables support binary runtime lookup.
- Preserved 206 source random outcomes and 210 ordered speech or room-visible actions,
  including per-state dice distributions, fall-through, action order, source text, and
  visibility settings.
- Repaired an obvious source branch-ordering defect: while a converted mobile is fighting,
  the explicitly authored fighting table is selected before the target standing-position
  gate. Idle tables still require an awake, standing mobile. Guildmaster 2003020 remains
  quiet in combat because it has no authored fighting table.
- Classified `rogue_one` as source-inert rather than inventing behavior: it registered only
  for `NPC_HIT`, whose supplied victim caused its immediate source return. Mixed-mechanics
  guards and the independently rolled `casino_four` callback remain pending for faithful
  treatment.
- Extended conversion persistence, the builder registry, both build manifests, builder and
  database help, manual testing, focused generator checks, converter fixtures, and
  production-linked characterization tests.
- Reconciliation now resolves 803 of 1,147 active direct bindings and 283 of 562 source
  handlers; 344 bindings and 279 handlers remain. The independent `ACT_SPEC` checkpoint
  advances to 730 resolved / 118 pending.
- Archived the thirty-second Phase 6 delivery session. At the forward bulk target of 20-45
  related families, the 279 remaining handlers give a 7-14 session Phase 6 envelope and a
  55-90 session forecast for Phases 6-8, or 110-360 focused engineering hours.

### Acceptance evidence

```text
Delivery commit: a67c1bfa
Reconciliation path: lib/rol-conversion/runs/phase6-special-20260812-state-periodic
Reconciliation run: rol-phase6-special-cb19c31118c1ce47
Active direct bindings: 1,147
Direct bindings resolved: 803
Direct bindings pending: 344
Source handlers resolved: 283
Source handlers pending: 279
Selected generated handler families resolved: 26
Selected source-inert handler families resolved: 1
Generated outcomes: 206
Generated room-visible actions: 210
Native adapted bindings: 409
Native adapted composable bindings: 131
ACT_SPEC records resolved: 730
ACT_SPEC records pending: 118
Complete world-tool suite: 290 passed
Focused conversion/generator suite: 66 passed
Production-linked CuTest suite: 643 passed
Documentation findings: 0 errors, 0 warnings, 0 info
Help SQL checks: 4 passed after applying the migration twice
Clean Autotools build, test, and install: passed
Root-level circle artifact: absent
Live target writes: 0
```

Phase 6 continues with the remaining 344 direct bindings across 279 source handlers.
Regular shapes will continue through bulk classification and generation; irregular
mechanics remain dependency-complete shared-runtime workstreams with full gates only at
substantial checkpoints.

## 2026-08-12 - Phase 6 generated source-periodic profiles

Status: Completed checkpoint; Phase 6 direct-binding reconciliation in progress

### Delivered

- Closed 82 source handler families and 86 active direct bindings in one bulk-generated
  workstream through the persistent, mobile-owned `RoL Source Periodic` procedure.
- Added an explicit source manifest and strict generator for the regular awake activity
  profiles in `specs.bloodstone.c`, `specs.icecrag.c`, and `specs.menden.c`. The checked-in
  table carries a digest of its assessed source inputs and a reproducibility check.
- Preserved all 327 selected random outcomes and 561 ordered speech or room-visible
  actions, including source random bounds, case fall-through, social room text, direct
  action visibility, and per-profile combat gates. Menden magus 2088806 deliberately
  remains active in combat; all other selected profiles pause while fighting.
- Sorted the generated mobile and outcome tables and used binary runtime lookup, avoiding
  linear scans on each mobile activity pulse.
- Extended conversion persistence, the builder registry, both build manifests, OLC/help
  documentation, manual testing, focused generator checks, converter fixtures, and
  production-linked characterization tests.
- Reconciliation now resolves 776 of 1,147 active direct bindings and 256 of 562 source
  handlers; 371 bindings and 306 handlers remain. The independent `ACT_SPEC` checkpoint
  advances to 703 resolved / 145 pending.
- Archived the thirty-first Phase 6 delivery session. This batch proved an 82-family
  mechanically regular workstream; discounting the remaining mixed mechanics to a
  conservative 20-45 related families per batch gives 7-16 remaining Phase 6 sessions
  and 55-92 total sessions for Phases 6-8, or 110-368 focused engineering hours.

### Acceptance evidence

```text
Delivery commit: c16e0fe9
Reconciliation path: lib/rol-conversion/runs/phase6-special-20260812-source-periodic
Reconciliation run: rol-phase6-special-5d67954ff0a9adbc
Active direct bindings: 1,147
Direct bindings resolved: 776
Direct bindings pending: 371
Source handlers resolved: 256
Source handlers pending: 306
Selected handler families resolved: 82
Selected direct bindings resolved: 86
Generated outcomes: 327
Generated room-visible actions: 561
Native adapted bindings: 383
Native adapted composable bindings: 131
ACT_SPEC records resolved: 703
ACT_SPEC records pending: 145
Complete world-tool suite: 286 passed
Focused conversion/generator suite: 88 passed
Production-linked CuTest suite: 642 passed
Documentation findings: 0 errors, 0 warnings, 0 info
Help SQL checks: 4 passed after applying the migration twice
Clean Autotools build, test, and install: passed
Root-level circle artifact: absent
Live target writes: 0
```

Phase 6 continues with the remaining 371 direct bindings across 306 source handlers.
Regular shapes will be classified and generated in bulk; irregular mechanics remain
dependency-complete shared-runtime workstreams with full gates only at substantial
checkpoints.

## 2026-08-12 - Phase 6 expanded Waterdeep ambient batch

Status: Completed checkpoint; Phase 6 direct-binding reconciliation in progress

### Delivered

- Closed 21 additional source handler families and 22 active direct bindings through
  the existing persistent, mobile-owned `RoL Waterdeep Ambient` procedure.
- Added data-driven profiles for tailor, shopper, assassin, brigand, fisherman, sailor,
  seaman, naval-worker, seabird, commoner, and Waterdeep-guard families. The runtime
  remains one reusable adapter keyed by converted mobile VNUM rather than one procedure
  per source function.
- Preserved the source two-d5 outcome distributions, authored speech and room-action
  ordering, and multi-message outcomes. Source text is retained exactly, including its
  original spelling and punctuation.
- Preserved the standing-position gate for every added profile and the source combat
  suppression for Waterdeep guard profiles. The first guard family covers converted
  mobiles 2003059 and 2003070; the second covers 2003035.
- Extended conversion/reconciliation mappings, builder help, database-first help,
  manual test coverage, converter fixtures, and production-linked characterization
  tests without adding another registry entry or source file.
- Reconciliation now resolves 690 of 1,147 active direct bindings and 174 of 562 source
  handlers; 457 bindings and 388 handlers remain. The independent `ACT_SPEC` checkpoint
  advances to 622 resolved / 226 pending.
- Archived the thirtieth bounded Phase 6 delivery session since the Phase 5 closeout.
  At the measured throughput of 15-30 handler families per batch, the conservative
  forward-looking estimate is now 13-26 Phase 6 sessions and 61-102 total sessions for
  Phases 6-8, or 122-408 focused engineering hours at 2-4 hours per session. The next
  workstream will measure a larger bulk-conversion cadence before replacing this basis.

### Acceptance evidence

```text
Delivery commit: 30132767
Reconciliation path: lib/rol-conversion/runs/phase6-special-20260812-waterdeep-ambient-2
Reconciliation run: rol-phase6-special-af17e0481a21298e
Active direct bindings: 1,147
Direct bindings resolved: 690
Direct bindings pending: 457
Source handlers resolved: 174
Source handlers pending: 388
Selected handler families resolved: 21
Selected direct bindings resolved: 22
Native adapted bindings: 297
Native adapted composable bindings: 131
ACT_SPEC records resolved: 622
ACT_SPEC records pending: 226
Complete world-tool suite: 282 passed
Focused conversion/reconciliation suite: 84 passed
Production-linked CuTest suite: 641 passed
Documentation findings: 0 errors, 0 warnings, 0 info
Help SQL checks: 4 passed after applying the migration twice
Clean Autotools build, test, and install: passed
Root-level circle artifact: absent
Live target writes: 0
```

Phase 6 continues with the remaining 457 direct bindings across 388 source handlers,
using larger dependency-complete workstreams, shared data-driven adapters, focused
checks during implementation, and full release gates only at substantial milestones.

## 2026-08-12 - Phase 6 batched Waterdeep ambient citizens

Status: Completed checkpoint; Phase 6 direct-binding reconciliation in progress

### Delivered

- Closed 23 source handler families and 34 active direct bindings in one
  dependency-complete batch through the persistent, mobile-owned
  `RoL Waterdeep Ambient` procedure.
- Added data-driven profiles for the active wanderer, drunk, homeless, cat, merchant,
  farmer, baker, mage, cleric, artillery, warrior, mercenary, casino-player, and youth
  families. Multiple converted mobiles share a profile without duplicating runtime
  code, while the target VNUM table prevents the procedure from affecting unrelated
  mobiles.
- Preserved the source two-die outcome distributions: most profiles use two-d5,
  casino player 2003204 uses two-d7, and casino player 2003205 uses two-d6. Authored
  speech and room actions retain their outcome ordering, including multi-message rolls
  and the source switch fall-through on casino player 2003205.
- Preserved the source standing-position gate and the special harbor-room restriction
  for merchant 2005310, whose dialog runs only in converted room 2005400.
- Registered the procedure and persistence/index contracts, taught conversion and
  reconciliation all 23 canonical mappings, and updated builder help, database-first
  help, manual testing, OLC inventories, converter fixtures, and production-linked
  characterization tests.
- Reconciliation now resolves 668 of 1,147 active direct bindings and 153 of 562 source
  handlers; 479 bindings and 409 handlers remain. The independent `ACT_SPEC` checkpoint
  advances to 600 resolved / 248 pending.
- Archived the twenty-ninth bounded Phase 6 delivery session since the Phase 5
  closeout. At the current target throughput of 15-30 handler families per batch, the
  forward-looking estimate is now 14-28 Phase 6 sessions and 62-104 total sessions for
  Phases 6-8, or 124-416 focused engineering hours at 2-4 hours per session.

### Acceptance evidence

```text
Delivery commit: 27bad343
Reconciliation path: lib/rol-conversion/runs/phase6-special-20260812-waterdeep-ambient
Reconciliation run: rol-phase6-special-7d3a624a62a104e5
Active direct bindings: 1,147
Direct bindings resolved: 668
Direct bindings pending: 479
Source handlers resolved: 153
Source handlers pending: 409
Selected handler families resolved: 23
Selected direct bindings resolved: 34
Native adapted bindings: 275
Native adapted composable bindings: 131
ACT_SPEC records resolved: 600
ACT_SPEC records pending: 248
Complete world-tool suite: 282 passed
Focused conversion/reconciliation suite: 84 passed
Production-linked CuTest suite: 641 passed
Documentation findings: 0 errors, 0 warnings, 0 info
Help SQL checks: 4 passed after applying the migration twice
Clean Autotools build, test, and install: passed
Root-level circle artifact: absent
Live target writes: 0
```

Phase 6 continues with the remaining 479 direct bindings across 409 source handlers,
using dependency-complete batches and shared data-driven adapters.

## 2026-08-12 - Phase 6 batched combat and death procedures

Status: Completed checkpoint; Phase 6 direct-binding reconciliation in progress

### Delivered

- Replaced the one-family delivery cadence with dependency-complete procedure batches.
  This first wider batch closed 19 source handler families and 28 active bindings in one
  reviewable checkpoint, using shared data-driven profiles where behavior differed only
  by caller, helper set, message, or converted mobile identity.
- Closed seven alert bindings across Demogorgon, Drisinil, Tukra, Imix, the Imix pet,
  and Yancbin. Five callers use the persistent `RoL Alert Caller` procedure; Imix and
  Yancbin compose the alert beside their existing fire and lightning breath procedures.
  Each caller shouts once per fight and sends only its eligible same-zone helpers after
  the source sound, state, reachability, and damage gates. One inactive Demogorgon helper
  reference absent from the active source map was omitted.
- Closed all five active `yggdrasil_branch` bindings with the persistent
  `RoL Yggdrasil Branch` procedure. The adapter preserves the source target weighting,
  50 percent attempt cadence, Reflex save with the source -10 modifier, four-to-twelve
  round entangle, timed release, and current-movement halving.
- Closed 16 no-corpse bindings across tentacle, treant, phantom-steed, dark-shade,
  mephit, and elemental death handlers. Converted-VNUM death profiles preserve the
  source-family messages and suppress ordinary corpses without consuming a named
  procedure slot or adding family-specific flags.
- Registered the two builder-visible procedures, retained stable existing event IDs,
  taught conversion and reconciliation about all composable profiles, and updated
  builder help, database-first help, manual tests, registry inventories, converter
  fixtures, and production-linked characterization tests.
- Reconciliation now resolves 634 of 1,147 active direct bindings and 130 of 562 source
  handlers; 513 bindings and 432 handlers remain. The independent `ACT_SPEC` checkpoint
  remains 568 resolved / 280 pending because these records were resolved through
  companion direct bindings and runtime composition.
- Archived the twenty-eighth bounded Phase 6 delivery session since the Phase 5
  closeout. At a target throughput of 15-30 handler families per dependency-complete
  batch, the forward-looking estimate is now 15-29 Phase 6 sessions and 63-105 total
  sessions for Phases 6-8, or 126-420 focused engineering hours at 2-4 hours per session.

### Acceptance evidence

```text
Delivery commit: 03111649
Reconciliation path: lib/rol-conversion/runs/phase6-special-20260812-batched-combat-death
Reconciliation run: rol-phase6-special-bcb867fc0cb376eb
Active direct bindings: 1,147
Direct bindings resolved: 634
Direct bindings pending: 513
Source handlers resolved: 130
Source handlers pending: 432
Selected handler families resolved: 19
Selected direct bindings resolved: 28
Native adapted bindings: 241
Native adapted composable bindings: 131
ACT_SPEC records resolved: 568
ACT_SPEC records pending: 280
Complete world-tool suite: 281 passed
Focused reconciliation suite: 60 passed
Production-linked CuTest suite: 640 passed
Documentation findings: 0 errors, 0 warnings, 0 info
Help SQL checks: 4 passed after applying the migration twice
Clean Autotools build, test, and install: passed
Root-level circle artifact: absent
Live target writes: 0
```

Phase 6 continues with the remaining 513 direct bindings across 432 source handlers,
using dependency-complete batches and shared data-driven adapters.

## 2026-08-12 - Phase 6 converted Waterdeep guild rooms

Status: Completed checkpoint; Phase 6 direct-binding reconciliation in progress

### Delivered

- Closed all twelve active `waterdeep_guild_one` through
  `waterdeep_guild_twelve` room bindings with the persistent, room-owned
  `RoL Waterdeep Guild Room` procedure.
- Preserved each source wrapper's exact-class or class-family admission gate for
  target rooms 2002956, 2003044, 2003061, 2003073, 2003289, 2005505, 2005512,
  2005524, 2005537, 2005544, 2005568, and 2005581. Any matching class in a target
  multiclass build is sufficient.
- Delegated accepted `practice`, `train`, and `boosts` commands to the current target
  guild service instead of restoring obsolete source practice mechanics. Unrelated
  commands remain available to the room's ordinary command flow.
- Adapted the source-only Mercenary guild at room 2005512 to the target Warrior class;
  Paladin, Monk, Bard, Ranger, Druid, Rogue, mage-family, warrior-family, and
  cleric-family rooms retain their corresponding target gates.
- Registered the procedure and persistence/index contracts, taught the converter all
  twelve canonical mappings, and updated builder help, database-first help, manual
  testing, OLC inventories, converter fixtures, and characterization tests.
- Reconciliation now resolves 606 of 1,147 active direct bindings and 111 of 562 source
  handlers; 541 bindings and 451 handlers remain. The independent `ACT_SPEC` checkpoint
  remains 568 resolved / 280 pending because these are room-owned bindings.
- Archived the twenty-seventh bounded Phase 6 delivery session since the Phase 5
  closeout. The forward-looking estimate is now 21-53 Phase 6 sessions and 69-129 total
  sessions for Phases 6-8, or 138-516 focused engineering hours at 2-4 hours per session.

### Acceptance evidence

```text
Delivery commit: 08fcf107
Reconciliation path: lib/rol-conversion/runs/phase6-special-20260812-waterdeep-guild
Reconciliation run: rol-phase6-special-667bf4274a2fd6dd
Active direct bindings: 1,147
Direct bindings resolved: 606
Direct bindings pending: 541
Source handlers resolved: 111
Source handlers pending: 451
Waterdeep-guild bindings resolved: 12
Native adapted bindings: 231
ACT_SPEC records resolved: 568
ACT_SPEC records pending: 280
Complete world-tool suite: 279 passed
Production-linked CuTest suite: 638 passed
Documentation findings: 0 errors, 0 warnings, 0 info
Help SQL checks: 4 passed after applying the migration twice
Clean Autotools build, test, and install: passed
Root-level circle artifact: absent
Live target writes: 0
```

Phase 6 continues with the remaining 541 direct bindings across 451 source handlers.

## 2026-08-12 - Phase 6 converted Bloodstone portals

Status: Completed checkpoint; Phase 6 direct-binding reconciliation in progress

### Delivered

- Closed all four active `bs_portal` bindings through the persistent, object-owned
  `RoL Bloodstone Portal` procedure: objects 2007147 and 2007148 lead to room 2007250,
  object 2007149 leads to room 2007109, and object 2022491 leads to room 2022569.
- Preserved the awake-only, exact visible-object `enter` command contract and taught
  the converter to remap each source destination in object value 0 through its Phase 2
  room identity.
- Adapted source admission to the target's safe teleport checks. Invalid, forbidden,
  or unloaded destinations consume the matched portal command without moving the actor.
- Preserved mortal portal stress: 1-20 hit points and 1-30 movement points, with movement
  floored at zero. Staff are immune, exactly -10 hit points survives, and only a result
  below -10 is fatal.
- Registered the procedure and persistence/index contracts, and updated builder help,
  database-first help, manual testing, converter fixtures, and characterization tests.
- Reconciliation now resolves 594 of 1,147 active direct bindings and 99 of 562 source
  handlers; 553 bindings and 463 handlers remain. The independent `ACT_SPEC` checkpoint
  remains 568 resolved / 280 pending because these are object-owned bindings.
- Archived the twenty-sixth bounded Phase 6 delivery session since the Phase 5 closeout.
  The forward-looking estimate is now 22-54 Phase 6 sessions and 70-130 total sessions
  for Phases 6-8, or 140-520 focused engineering hours at 2-4 hours per session.

### Acceptance evidence

```text
Delivery commit: 2635c21c
Reconciliation path: lib/rol-conversion/runs/phase6-special-20260812-bloodstone-portal
Reconciliation run: rol-phase6-special-f629c37b68cdad7d
Active direct bindings: 1,147
Direct bindings resolved: 594
Direct bindings pending: 553
Source handlers resolved: 99
Source handlers pending: 463
Bloodstone-portal bindings resolved: 4
Native adapted bindings: 219
ACT_SPEC records resolved: 568
ACT_SPEC records pending: 280
Complete world-tool suite: 278 passed
Production-linked CuTest suite: 637 passed
Documentation findings: 0 errors, 0 warnings, 0 info
Help SQL checks: 4 passed after applying the migration twice
Clean Autotools build, test, and install: passed
Root-level circle artifact: absent
Live target writes: 0
```

Phase 6 continues with the remaining 553 direct bindings across 463 source handlers.

## 2026-08-12 - Phase 6 converted Ethereal floating pools

Status: Completed checkpoint; Phase 6 direct-binding reconciliation in progress

### Delivered

- Closed all four active `floating_pool` bindings for converted Ethereal objects
  2022706, 2022707, 2022710, and 2022711 through the persistent, object-owned
  `RoL Floating Pool` procedure and required `ITEM_AUTOPROC` gateway.
- Implemented the source-documented 12 percent movement chance rather than retaining
  the source implementation's inverted predicate, which moved the pools 88 percent of
  the time. This is an explicit repair of an obvious source defect under the locked
  conversion policy.
- Preserved random selection among eligible north, east, south, west, up, and down
  exits. Invalid, closed, hidden, blocked, and `ROOM_NOMOB` destinations are excluded,
  with source-style departure and arrival presentation around the object move.
- Corrected the shared object auto-pulse gateway so room and contained objects dispatch
  once per pulse with no actor; equipped and carried objects retain their existing
  worn-first and carried-fallback contract.
- Registered the procedure and persistence/index contracts, taught the converter its
  canonical target name and required flag, and updated builder help, database-first
  help, manual testing, OLC inventories, converter fixtures, and characterization tests.
- Reconciliation now resolves 590 of 1,147 active direct bindings and 98 of 562 source
  handlers; 557 bindings and 464 handlers remain. The independent `ACT_SPEC` checkpoint
  remains 568 resolved / 280 pending because these are object-owned bindings.
- Archived the twenty-fifth bounded Phase 6 delivery session since the Phase 5 closeout.
  The forward-looking estimate is now 23-55 Phase 6 sessions and 71-131 total sessions
  for Phases 6-8, or 142-524 focused engineering hours at 2-4 hours per session.

### Acceptance evidence

```text
Delivery commit: e3600f16
Reconciliation path: lib/rol-conversion/runs/phase6-special-20260812-floating-pool
Reconciliation run: rol-phase6-special-c7ae6f16963a5f16
Active direct bindings: 1,147
Direct bindings resolved: 590
Direct bindings pending: 557
Source handlers resolved: 98
Source handlers pending: 464
Floating-pool bindings resolved: 4
Native adapted bindings: 215
ACT_SPEC records resolved: 568
ACT_SPEC records pending: 280
Complete world-tool suite: 277 passed
Production-linked CuTest suite: 636 passed
Documentation findings: 0 errors, 0 warnings, 0 info
Help SQL checks: 4 passed after applying the migration twice
Autotools build, test, and install: passed
Root-level circle artifact: absent
Live target writes: 0
```

Phase 6 continues with the remaining 557 direct bindings across 464 source handlers.

## 2026-08-12 - Phase 6 converted Icecrag designated followers

Status: Completed checkpoint; Phase 6 direct-binding reconciliation in progress

### Delivered

- Closed all five active `follow_that_mob` bindings for converted Icecrag mobiles
  2097009, 2097018-2097019, and 2097036-2097037 through the persistent, mobile-owned
  `RoL Designated Follower` procedure and required `MOB_SPEC` activity gateway.
- Preserved the source follower-to-leader identities: 2097009 follows 2097012,
  2097018-2097019 follow 2097020, and 2097036-2097037 follow 2097035.
- Preserved the awake, unassigned, and colocated attachment gates. Once attached, the
  target follower system carries the mobile with its leader, and the adapter invokes
  target combat assistance when the designated NPC leader is fighting.
- Preserved source docile and no-kill suppression and inert behavior for unrelated
  prototypes, absent leaders, sleeping followers, or followers already assigned to a
  different master.
- Registered the procedure and persistence/index contracts, taught the converter its
  canonical target name and required flag, and updated builder help, database-first
  help, manual testing, OLC inventories, converter fixtures, and characterization tests.
- Reconciliation now resolves 586 of 1,147 active direct bindings and 97 of 562 source
  handlers; 561 bindings and 465 handlers remain. The independent `ACT_SPEC` checkpoint
  advances to 568 resolved / 280 pending.
- Archived the twenty-fourth bounded Phase 6 delivery session since the Phase 5 closeout.
  The forward-looking estimate is now 24-56 Phase 6 sessions and 72-132 total sessions
  for Phases 6-8, or 144-528 focused engineering hours at 2-4 hours per session.

### Acceptance evidence

```text
Delivery commit: 9de8ed76
Reconciliation path: lib/rol-conversion/runs/phase6-special-20260812-designated-follower
Reconciliation run: rol-phase6-special-2c7939775b253510
Active direct bindings: 1,147
Direct bindings resolved: 586
Direct bindings pending: 561
Source handlers resolved: 97
Source handlers pending: 465
Designated-follower bindings resolved: 5
Native adapted bindings: 211
ACT_SPEC records resolved: 568
ACT_SPEC records pending: 280
Complete world-tool suite: 276 passed
Production-linked CuTest suite: 635 passed
Documentation findings: 0 errors, 0 warnings, 0 info
Help SQL checks: 4 passed after applying the migration twice
Autotools build, test, and install: passed
Root-level circle artifact: absent
Live target writes: 0
```

Phase 6 continues with the remaining 561 direct bindings across 465 source handlers.

## 2026-08-12 - Phase 6 converted directional item blockers

Status: Completed checkpoint; Phase 6 direct-binding reconciliation in progress

### Delivered

- Closed all six active `item_block` bindings for converted ATD objects
  2000891-2000896 through the persistent, object-owned `RoL Item Blocker`
  procedure.
- Preserved each object's authored north, east, south, west, up, or down direction in
  object value 0. While an aggressive NPC occupies the room, the object intercepts
  mortal player and pet movement in that direction and matching door-unlock attempts.
- Preserved the source exemptions for ordinary non-pet NPCs and trusted staff, the
  source morphed-character exception, the hidden or blocked exit boundary, and inert
  behavior when no aggressive NPC is present.
- Registered the procedure and persistence/index contracts, taught the converter its
  canonical target name, and updated builder help, database-first help, manual testing,
  OLC inventories, converter fixtures, and production-linked characterization tests.
- Reconciliation now resolves 581 of 1,147 active direct bindings and 96 of 562 source
  handlers; 566 bindings and 466 handlers remain. The independent `ACT_SPEC` checkpoint
  remains 563 resolved / 285 pending because these are object-owned bindings.
- Archived the twenty-third bounded Phase 6 delivery session since the Phase 5 closeout.
  The forward-looking estimate is now 25-57 Phase 6 sessions and 73-133 total sessions
  for Phases 6-8, or 146-532 focused engineering hours at 2-4 hours per session.

### Acceptance evidence

```text
Delivery commit: acb858c1
Reconciliation path: lib/rol-conversion/runs/phase6-special-20260812-item-blocker
Reconciliation run: rol-phase6-special-d11cc60b4d4cd56e
Active direct bindings: 1,147
Direct bindings resolved: 581
Direct bindings pending: 566
Source handlers resolved: 96
Source handlers pending: 466
Item-blocker bindings resolved: 6
Native adapted bindings: 206
ACT_SPEC records resolved: 563
ACT_SPEC records pending: 285
Complete world-tool suite: 275 passed
Production-linked CuTest suite: 634 passed
Documentation findings: 0 errors, 0 warnings, 0 info
Help SQL checks: 4 passed after applying the migration twice
Autotools build, test, and install: passed
Root-level circle artifact: absent
Live target writes: 0
```

Phase 6 continues with the remaining 566 direct bindings across 466 source handlers.

## 2026-08-12 - Phase 6 Bloodstone critter behavior

Status: Completed checkpoint; Phase 6 direct-binding reconciliation in progress

### Delivered

- Closed all four active `bs_critter` bindings for converted Bloodstone mobiles
  2007110-2007112 and 2007141 through the persistent, mobile-owned
  `RoL Bloodstone Critter` procedure and required `MOB_SPEC` activity gateway.
- Preserved the source 2-in-81 ambient cadence: roll zero uses the current target
  `snarl` social, roll one uses `growl`, and all other outcomes are inert.
- Preserved the awake-and-idle boundary. Sleeping or fighting critters do not run the
  ambient behavior, and the normal loaded target socials provide current presentation.
- Registered the procedure and persistence/index contracts, taught the converter its
  canonical target name and required flag, and updated builder help, database-first
  help, manual testing, OLC inventories, converter fixtures, and characterization tests.
- Reconciliation now resolves 575 of 1,147 active direct bindings and 95 of 562 source
  handlers; 572 bindings and 467 handlers remain. The independent `ACT_SPEC` checkpoint
  advances to 563 resolved / 285 pending.
- Archived the twenty-second bounded Phase 6 delivery session since the Phase 5 closeout.
  The forward-looking estimate is now 26-58 Phase 6 sessions and 74-134 total sessions
  for Phases 6-8, or 148-536 focused engineering hours at 2-4 hours per session.

### Acceptance evidence

```text
Delivery commit: b2e7bf40
Reconciliation path: lib/rol-conversion/runs/phase6-special-20260812-bloodstone-critter
Reconciliation run: rol-phase6-special-bdf567929b95b5d7
Active direct bindings: 1,147
Direct bindings resolved: 575
Direct bindings pending: 572
Source handlers resolved: 95
Source handlers pending: 467
Bloodstone critter bindings resolved: 4
Native adapted bindings: 200
ACT_SPEC records resolved: 563
ACT_SPEC records pending: 285
Complete world-tool suite: 274 passed
Production-linked CuTest suite: 633 passed
Documentation findings: 0 errors, 0 warnings, 0 info
Help SQL checks: 4 passed after applying the migration twice
Autotools build, test, and install: passed
Root-level circle artifact: absent
Live target writes: 0
```

Phase 6 continues with the remaining 572 direct bindings across 467 source handlers.

## 2026-08-12 - Phase 6 Bloodstone undead death behavior

Status: Completed checkpoint; Phase 6 direct-binding reconciliation in progress

### Delivered

- Closed all four active `bs_undead_die` bindings for Bloodstone mobiles 7119, 7162,
  7167, and 7197 through the converter-owned `RoL-Black-Vapor-Death` mobile flag.
- Preserved composition with each mobile's other source procedure instead of consuming
  or replacing the target's single named SpecProc slot.
- Replaced Luminari's generic undead crumble presentation with the source black-vapor
  message while retaining the target's native no-corpse policy for converted undead.
- Extended the mobile-flag constants manifest, converter, runtime death hook, builder
  documentation, generated web guide, database-first help, manual test matrix, and
  production-linked characterization coverage.
- Reconciliation now resolves 571 of 1,147 active direct bindings and 94 of 562 source
  handlers; 576 bindings and 468 handlers remain. The independent `ACT_SPEC` checkpoint
  remains 559 resolved / 289 pending because these composable bindings do not consume
  the named-procedure gateway.
- Archived the twenty-first bounded Phase 6 delivery session since the Phase 5 closeout.
  The forward-looking estimate is now 27-59 Phase 6 sessions and 75-135 total sessions
  for Phases 6-8, or 150-540 focused engineering hours at 2-4 hours per session.

### Acceptance evidence

```text
Delivery commit: 27b5ba59
Reconciliation path: lib/rol-conversion/runs/phase6-special-20260812-bloodstone-vapor
Reconciliation run: rol-phase6-special-fffdce44e3e15b73
Active direct bindings: 1,147
Direct bindings resolved: 571
Direct bindings pending: 576
Source handlers resolved: 94
Source handlers pending: 468
Bloodstone undead death bindings resolved: 4
Native adapted composable bindings: 113
ACT_SPEC records resolved: 559
ACT_SPEC records pending: 289
Complete world-tool suite: 273 passed
Production-linked CuTest suite: 632 passed
Documentation findings: 0 errors, 0 warnings, 0 info
Help SQL checks: 4 passed after applying the migration twice
Autotools build, test, and install: passed
Root-level circle artifact: absent
Live target writes: 0
```

Phase 6 continues with the remaining 576 direct bindings across 468 source handlers.

## 2026-08-12 - Phase 6 converted Sister Knight reinforcements

Status: Completed checkpoint; Phase 6 direct-binding reconciliation in progress

### Delivered

- Closed all five active `sister_knight` mobile bindings in the Moonshae package
  through the persistent, mobile-owned `RoL Sister Knight` procedure and the required
  `MOB_SPEC` activity and combat-turn gateways.
- Preserved the source zone-wide attack shout and bounded helper family. Awake, idle,
  reachable converted Sisters within 100 rooms in the same zone begin pursuing the
  caller's opponent; fighting, hunting, charmed, protected, unreachable, out-of-range,
  and cross-zone candidates remain ineligible.
- Preserved soundproof-room, silence, paralysis, sleep, and casting suppression. The
  source justice witness guard maps to the target's per-combat `PROC_FIRED` state so a
  caller shouts once during an encounter and resets after combat ends.
- Registered the procedure and event contract, taught the converter its canonical
  target name and required action flag, and updated builder help, database-first help,
  manual testing, converter fixtures, registry persistence and OLC inventories, and
  production-linked characterization tests.
- Reconciliation now resolves 567 of 1,147 active direct bindings and 93 of 562 source
  handlers; 580 bindings and 469 handlers remain. The independent `ACT_SPEC` checkpoint
  advances to 559 resolved / 289 pending.
- Archived the twentieth bounded Phase 6 delivery session since the Phase 5 closeout.
  The forward-looking estimate is now 28-60 Phase 6 sessions and 76-136 total sessions
  for Phases 6-8, or 152-544 focused engineering hours at 2-4 hours per session.

### Acceptance evidence

```text
Delivery commit: 4fa18daf
Reconciliation path: lib/rol-conversion/runs/phase6-special-20260812-sister-knight
Reconciliation run: rol-phase6-special-3aa909fd9793606b
Active direct bindings: 1,147
Direct bindings resolved: 567
Direct bindings pending: 580
Source handlers resolved: 93
Source handlers pending: 469
Sister Knight bindings resolved: 5
ACT_SPEC records resolved: 559
ACT_SPEC records pending: 289
Complete world-tool suite: 272 passed, 52 subtests passed
Production-linked CuTest suite: 632 passed
Documentation findings: 0 errors, 0 warnings, 0 info
Help SQL checks: 4 passed after applying the migration twice
Autotools build, test, and install: passed
Root-level circle artifact: absent
Live target writes: 0
```

Phase 6 continues with the remaining 580 direct bindings across 469 source handlers.

## 2026-08-12 - Phase 6 converted class-family guild rooms

Status: Completed checkpoint; Phase 6 direct-binding reconciliation in progress

### Delivered

- Closed all ten active `guild_classtype_mage`, `guild_classtype_thief`,
  `guild_classtype_warrior`, and `guild_classtype_cleric` room bindings through four
  persistent, room-owned RoL class-family guild procedures.
- Preserved the source rule that a mismatched class family cannot use the guild while
  adapting its single-class check to the target multiclass model. Any matching class
  level admits the character; NPCs and unrelated commands retain ordinary current
  guild behavior.
- Delegated accepted `practice`, `train`, and `boosts` commands to Luminari's current
  training service instead of reviving the source's obsolete skill-practice and spell
  copying implementation.
- Registered the four procedures and their command contracts, taught the converter
  their canonical target names, and updated builder help, database-first help, manual
  testing, converter fixtures, registry persistence and OLC inventories, and
  production-linked characterization tests.
- Reconciliation now resolves 562 of 1,147 active direct bindings and 92 of 562 source
  handlers; 585 bindings and 470 handlers remain. The independent `ACT_SPEC` checkpoint
  remains 554 resolved / 294 pending.
- Archived the nineteenth bounded Phase 6 delivery session since the Phase 5 closeout.
  The forward-looking estimate is now 29-61 Phase 6 sessions and 77-137 total sessions
  for Phases 6-8, or 154-548 focused engineering hours at 2-4 hours per session.

### Acceptance evidence

```text
Delivery commit: f4eefda6
Reconciliation path: lib/rol-conversion/runs/phase6-special-20260812-class-guilds
Reconciliation run: rol-phase6-special-607369f4f87e0a4a
Active direct bindings: 1,147
Direct bindings resolved: 562
Direct bindings pending: 585
Source handlers resolved: 92
Source handlers pending: 470
Class-family guild bindings resolved: 10
ACT_SPEC records resolved: 554
ACT_SPEC records pending: 294
Complete world-tool suite: 271 passed, 52 subtests passed
Production-linked CuTest suite: 631 passed
Documentation findings: 0 errors, 0 warnings, 0 info
Help SQL checks: 4 passed after applying the migration twice
Autotools build, test, and install: passed
Root-level circle artifact: absent
Live target writes: 0
```

Phase 6 continues with the remaining 585 direct bindings across 470 source handlers.

## 2026-08-12 - Phase 6 converted lich energy drain

Status: Completed checkpoint; Phase 6 direct-binding reconciliation in progress

### Delivered

- Closed all six active `lich_energy_drain` mobile bindings through the persistent,
  mobile-owned `RoL Lich Energy Drain` procedure and the required `MOB_SPEC` activity
  and combat-turn gateways.
- Preserved room-list targeting of the current opponent and its party, the independent
  one-in-five eligibility checks, the first-success rule, casting suppression, and the
  source callback's deliberate fall-through to ordinary NPC activity.
- Preserved the full-current-hit-point life transfer, Blackmantle healing suppression,
  over-maximum healing, and cumulative two-combat-round stun. Target Death Ward maps
  the unavailable source protection-from-undead spell and leaves the victim at zero
  rather than minus five hit points.
- Registered the procedure and event contract, taught the converter its canonical
  target name and required action flag, and updated builder help, database-first help,
  manual testing, converter fixtures, registry persistence and OLC inventories, and
  production-linked characterization tests.
- Reconciliation now resolves 552 of 1,147 active direct bindings and 88 of 562 source
  handlers; 595 bindings and 474 handlers remain. The independent `ACT_SPEC` checkpoint
  advances to 554 resolved / 294 pending.
- Archived the eighteenth bounded Phase 6 delivery session since the Phase 5 closeout.
  The forward-looking estimate is now 30-62 Phase 6 sessions and 78-138 total sessions
  for Phases 6-8, or 156-552 focused engineering hours at 2-4 hours per session.

### Acceptance evidence

```text
Delivery commit: 72ba7c8e
Reconciliation path: lib/rol-conversion/runs/phase6-special-20260812-lich-energy-drain
Reconciliation run: rol-phase6-special-8eaf63b5965118f6
Active direct bindings: 1,147
Direct bindings resolved: 552
Direct bindings pending: 595
Source handlers resolved: 88
Source handlers pending: 474
Lich-energy-drain bindings resolved: 6
ACT_SPEC records resolved: 554
ACT_SPEC records pending: 294
Complete world-tool suite: 270 passed, 52 subtests passed
Production-linked CuTest suite: 630 passed
Documentation findings: 0 errors, 0 warnings, 0 info
Help SQL checks: 4 passed after applying the migration twice
Autotools build, test, and install: passed
Root-level circle artifact: absent
Live target writes: 0
```

Phase 6 continues with the remaining 595 direct bindings across 474 source handlers.

## 2026-08-12 - Phase 6 converted trade bandits

Status: Completed checkpoint; Phase 6 direct-binding reconciliation in progress

### Delivered

- Closed all seven active `bandit` mobile bindings through the persistent,
  mobile-owned `RoL Trade Bandit` procedure and the required `MOB_SPEC` command and
  combat-turn gateway.
- Preserved one-player capture, movement/flee/get interception, source demand variants,
  ordinary give-command processing, underpayment followed by attack, successful
  payment followed by disappearance, the repeat-attempt attack chance, and the lazy
  ten-MUD-hour expiry when the bandit is alone.
- Mapped source platinum demands to ten target gold per platinum and valued carried
  resources plus cargo in the player's owned wagon. The wagon-seizure variant uses the
  target `ITEM_WAGON` ownership model; a missing required wagon safely becomes an attack
  instead of reproducing the source null extraction defect.
- Registered the procedure and event contract, taught the converter its canonical
  target name and required action flag, and updated builder help, database-first help,
  manual testing, converter fixtures, registry persistence and OLC inventories, and
  production-linked characterization tests.
- Reconciliation now resolves 546 of 1,147 active direct bindings and 87 of 562 source
  handlers; 601 bindings and 475 handlers remain. The independent `ACT_SPEC` checkpoint
  remains 552 resolved / 296 pending.
- Archived the seventeenth bounded Phase 6 delivery session since the Phase 5 closeout.
  The forward-looking estimate is now 31-63 Phase 6 sessions and 79-139 total sessions
  for Phases 6-8, or 158-556 focused engineering hours at 2-4 hours per session.

### Acceptance evidence

```text
Delivery commit: 7693ce00
Reconciliation path: lib/rol-conversion/runs/phase6-special-20260812-bandit
Reconciliation run: rol-phase6-special-62a0531d50e3b71d
Active direct bindings: 1,147
Direct bindings resolved: 546
Direct bindings pending: 601
Source handlers resolved: 87
Source handlers pending: 475
Trade-bandit bindings resolved: 7
ACT_SPEC records resolved: 552
ACT_SPEC records pending: 296
Complete world-tool suite: 269 passed, 52 subtests passed
Production-linked CuTest suite: 629 passed
Documentation findings: 0 errors, 0 warnings, 0 info
Help SQL checks: 4 passed after applying the migration twice
Autotools build, test, and install: passed
Root-level circle artifact: absent
Live target writes: 0
```

Phase 6 continues with the remaining 601 direct bindings across 475 source handlers.

## 2026-08-12 - Phase 6 converted major beholders

Status: Completed checkpoint; Phase 6 direct-binding reconciliation in progress

### Delivered

- Closed all eight active `major_beholder` mobile bindings through the persistent,
  mobile-owned `RoL Major Beholder` procedure and the required `MOB_SPEC` combat-turn
  gateway.
- Preserved all ten independent eye identities, the source one-in-three check for every
  ready eye, three-combat-turn per-eye cooldowns, multiple rays in one turn, room combat
  retargeting, and redirection from a selected charmed pet to its present master.
- Mapped the eye suite onto target-native fireball, acid arrow, slow, ray of
  enfeeblement plus feeblemind, wither, room-wide dispel, prismatic spray, hold monster,
  harm, and finger of death effects. The source-only all-unused-eyes weapon-critical
  burst is explicitly unavailable because the target special gateway does not expose a
  source weapon-critical event.
- Registered the procedure and combat contract, taught the converter its canonical
  target name and required action flag, and updated builder help, database-first help,
  manual testing, converter fixtures, registry persistence and OLC inventories, and
  production-linked characterization tests.
- Reconciliation now resolves 539 of 1,147 active direct bindings and 86 of 562 source
  handlers; 608 bindings and 476 handlers remain. The independent `ACT_SPEC` checkpoint
  remains 552 resolved / 296 pending.
- Archived the sixteenth bounded Phase 6 delivery session since the Phase 5 closeout.
  The forward-looking estimate is now 32-64 Phase 6 sessions and 80-140 total sessions
  for Phases 6-8, or 160-560 focused engineering hours at 2-4 hours per session.

### Acceptance evidence

```text
Delivery commit: 5536e463
Reconciliation path: lib/rol-conversion/runs/phase6-special-20260812-major-beholder
Reconciliation run: rol-phase6-special-e2050f070b43faf9
Active direct bindings: 1,147
Direct bindings resolved: 539
Direct bindings pending: 608
Source handlers resolved: 86
Source handlers pending: 476
Major-beholder bindings resolved: 8
ACT_SPEC records resolved: 552
ACT_SPEC records pending: 296
Complete world-tool suite: 268 passed, 52 subtests passed
Production-linked CuTest suite: 628 passed
Documentation findings: 0 errors, 0 warnings, 0 info
Help SQL checks: 4 passed after applying the migration twice
Autotools build, test, and install: passed
Root-level circle artifact: absent
Live target writes: 0
```

Phase 6 continues with the remaining 608 direct bindings across 476 source handlers.

## 2026-08-12 - Phase 6 converted shaman totems

Status: Completed checkpoint; Phase 6 direct-binding reconciliation in progress

### Delivered

- Closed all 21 active `shaman_totem` object bindings and all 21 corresponding spirit
  death handlers through the persistent, object-owned `RoL Shaman Totem` procedure and
  the converter-owned `RoL-Totem-Spirit` mobile identity.
- Preserved permanent player/object bonding, good-versus-evil source-race admission,
  the mapped Cleric level-21 summon unlock, one active spirit, peaceful-room refusal,
  and three consumed attempts per seven MUD days. The missing source shaman skill maps
  to a documented Cleric-level and Wisdom progression that remains fallible until its
  bounded maximum.
- Preserved bounded companion scaling at ten Cleric levels below the summoner, target
  autorolling, the source 25-percent hit-point bonus, zero reward, charm/follower
  ownership, existing follower combat assistance, and all animal-specific corpse-free
  fade messages.
- Registered the persistent procedure and command/identify contracts, taught the
  converter all 21 target VNUM mappings, and updated both build manifests, constants,
  builder help, database-first help, manual testing, generated references, converter
  fixtures, registry inventories, and production-linked tests.
- Reconciliation now resolves 531 of 1,147 active direct bindings and 85 of 562 source
  handlers; 616 bindings and 477 handlers remain. The independent `ACT_SPEC` checkpoint
  remains 552 resolved / 296 pending.
- Archived the fifteenth bounded Phase 6 delivery session since the Phase 5 closeout.
  The forward-looking estimate is now 33-65 Phase 6 sessions and 81-141 total sessions
  for Phases 6-8, or 162-564 focused engineering hours at 2-4 hours per session.

### Acceptance evidence

```text
Delivery commit: 8159562d
Reconciliation path: lib/rol-conversion/runs/phase6-special-20260812-shaman-totem
Reconciliation run: rol-phase6-special-5b3c2b758537ad3a
Active direct bindings: 1,147
Direct bindings resolved: 531
Direct bindings pending: 616
Source handlers resolved: 85
Source handlers pending: 477
Shaman-totem object bindings resolved: 21
Totem-spirit death handlers resolved: 21
ACT_SPEC records resolved: 552
ACT_SPEC records pending: 296
Complete world-tool suite: 267 passed, 52 subtests passed
Production-linked CuTest suite: 627 passed
Documentation findings: 0 errors, 0 warnings, 0 info
Help SQL checks: 4 passed after applying the migration twice
Autotools build, test, and install: passed
Root-level circle artifact: absent
Live target writes: 0
```

Phase 6 continues with the remaining 616 direct bindings across 477 source handlers.

## 2026-08-12 - Phase 6 converted guild guards

Status: Completed checkpoint; Phase 6 direct-binding reconciliation in progress

### Delivered

- Closed all 47 active `guild_guard` mobile bindings through the persistent,
  mobile-owned `RoL Guild Guard` procedure.
- Reduced the source's 710-line global room switch to the 45 gate rules reached by
  active converted guards. The adapter covers 44 distinct converted load rooms, one
  two-direction entrance, class and race admission, unconditional barriers, and the
  source rule that a displaced guard no longer controls its entrance.
- Mapped the active source class families onto the target's multiclass model and mapped
  Grey Elf/Half-Elf admission onto target Elf/Half-Elf identities. Immortals and target
  town guards keep their movement bypasses.
- Preserved protection behavior in all 33 active protected-room rules: mortal attackers
  lose at most level times 5,000 experience without falling below two experience, take
  the source dispel/curse/poison/blind/slow suite, are reduced to one hit point, leave
  combat, and move to a safe random loaded room in the same zone.
- Registered the procedure with command and mobile-combat contracts, taught the
  converter its canonical target name, and made conversion add `MOB_SPEC`, which the
  target combat-turn gateway requires.
- Updated database-first help, builder and manual-test references, converter fixtures,
  registry persistence and OLC inventories, and production-linked tests. Reconciliation
  now resolves 489 of 1,147 active direct bindings and 63 of 562 handlers; 658 bindings
  and 499 handlers remain. The independent `ACT_SPEC` checkpoint advances to 552
  resolved / 296 pending.
- Archived the fourteenth bounded Phase 6 delivery session since the Phase 5 closeout.
  The forward-looking estimate is now 34-66 Phase 6 sessions and 82-142 total sessions
  for Phases 6-8, or 164-568 focused engineering hours at 2-4 hours per session.

### Acceptance evidence

```text
Delivery commit: 7102d82d
Reconciliation path: lib/rol-conversion/runs/phase6-special-20260812-guild-guard
Reconciliation run: rol-phase6-special-082fb35d02d05212
Active direct bindings: 1,147
Direct bindings resolved: 489
Direct bindings pending: 658
Source handlers resolved: 63
Source handlers pending: 499
Guild-guard bindings resolved: 47
ACT_SPEC records resolved: 552
ACT_SPEC records pending: 296
Complete world-tool suite: 265 passed
Production-linked CuTest suite: 626 passed
Documentation findings: 0 errors, 0 warnings, 0 info
Help SQL checks: 4 passed
Autotools build, test, and install: passed
Root-level circle artifact: absent
Live target writes: 0
```

Phase 6 continues with the remaining 658 direct bindings across 499 source handlers.

## 2026-08-12 - Phase 6 converted ship system

Status: Completed checkpoint; Phase 6 direct-binding reconciliation in progress

### Delivered

- Closed all 57 active bindings for the shared RoL ship subsystem: seven hull objects,
  seven control panels, seven exit rooms, 29 lookout rooms, and seven navigators across
  Chionthar, Realms Master, Silver Lady, Gloom, Mirar, Captain's Fancy, and Spirit Raven.
- Added a bounded fixed-interior adapter rather than forcing the converted 37-room ships
  through the target Greyhawk system's 20-room interior limit. Boarding, capacity,
  instruments, speed, directional sailing and repeat movement, firing, ramming, ship
  docking, lookout, disembarking, hull damage, and sinking retain the source contracts.
- Preserved all seven two-way authored routes at the source 2.5-second cadence, including
  game-hour departure windows, navigator presence requirements, and departure, arrival,
  lost-route, and blocked-route announcements.
- Added navigator order protection and composition-safe combat-turn crew calls. Repaired
  the source Silver Lady helper list, which incorrectly named the Realms Master crew, to
  use the corresponding Silver Lady mobile family.
- Registered five persistent owner-aware procedures, taught the converter their canonical
  target names, and made the converter add `MOB_SPEC` to all seven navigators because the
  target combat-turn gateway requires it.
- Updated database-first help, builder and manual-test references, both build manifests,
  converter fixtures, registry persistence and OLC inventories, and production-linked
  tests. Reconciliation now resolves 442 of 1,147 active direct bindings and 62 of 562
  handlers; 705 bindings and 500 handlers remain. The independent `ACT_SPEC` checkpoint
  advances to 521 resolved / 327 pending.
- Archived the thirteenth bounded Phase 6 delivery session since the Phase 5 closeout. The
  forward-looking estimate is now 35-67 Phase 6 sessions and 83-143 total sessions for
  Phases 6-8, or 166-572 focused engineering hours at 2-4 hours per session.

### Acceptance evidence

```text
Delivery commit: 215c0f13
Reconciliation path: lib/rol-conversion/runs/phase6-special-20260812-ships
Reconciliation run: rol-phase6-special-490933ef03fa1db0
Active direct bindings: 1,147
Direct bindings resolved: 442
Direct bindings pending: 705
Source handlers resolved: 62
Source handlers pending: 500
Ship-family bindings resolved: 57
ACT_SPEC records resolved: 521
ACT_SPEC records pending: 327
Complete world-tool suite: 264 passed
Production-linked CuTest suite: 625 passed
Documentation findings: 0 errors, 0 warnings, 0 info
Help SQL idempotency checks: 4 passed twice
Autotools build, test, and install: passed
Root-level circle artifact: absent
Live target writes: 0
```

Phase 6 continues with the remaining 705 direct bindings across 500 source handlers.

## 2026-08-12 - Phase 6 shadow-giant procedure

Status: Completed checkpoint; Phase 6 direct-binding reconciliation in progress

### Delivered

- Closed all eight active `shadow_giant` mobile bindings in the `abandon` package.
  Reconciliation now resolves 385 of 1,147 active direct bindings and 57 of 562
  handlers; 762 bindings and 505 handlers remain.
- Added the mobile-owned `RoL Shadow Giant` procedure on the source periodic activity
  cadence. While fighting, its 1-in-21 trigger applies level-30 `spook` behavior to
  every player and charmed pet in the room: 25d8 mental damage, a Will save for half,
  and the source level-based chance for a one-to-three-round stun.
- Preserved the source immunity list for undead, dragons, demons, devils, and angels.
  Added the converter-owned `RoL-Angel` identity flag because source angels otherwise
  collapse into the target's broader outsider race and cannot be distinguished from
  other outsider pets.
- Repaired the source face-removal messages to use the actual fighting target rather
  than the source callback's null local pointer. Ordinary non-pet NPCs remain excluded.
- Updated the registry, owner-aware OLC, converter, constants manifest, database-first
  help, builder references, manual-test guidance, and production-linked tests. The
  independent `ACT_SPEC` checkpoint remains 517 resolved / 331 pending because none of
  these eight assignment-table consumers carried the source `ACT_SPEC` flag.
- Archived the twelfth bounded Phase 6 delivery session since the Phase 5 closeout.
  The forward-looking estimate is now 36-68 Phase 6 sessions and 84-144 total sessions
  for Phases 6-8, or 168-576 focused engineering hours at 2-4 hours per session.

### Acceptance evidence

```text
Delivery commit: 96785da1
Reconciliation path: lib/rol-conversion/runs/phase6-special-20260812-shadow-giant
Reconciliation run: rol-phase6-special-989f5d0c8b5ac5c6
Active direct bindings: 1,147
Direct bindings resolved: 385
Direct bindings pending: 762
Source handlers resolved: 57
Source handlers pending: 505
Shadow-giant bindings resolved: 8
ACT_SPEC records resolved: 517
ACT_SPEC records pending: 331
Complete world-tool suite: 262 passed
Production-linked CuTest suite: 624 passed
Documentation findings: 0 errors, 0 warnings, 0 info
Help SQL idempotency checks: 4 passed twice
Autotools build, test, and install: passed
Root-level circle artifact: absent
Live target writes: 0
```

Phase 6 continues with the remaining 762 direct bindings across 505 source handlers.

## 2026-08-12 - Phase 6 source-preprocessor binding correction

Status: Completed checkpoint; Phase 6 direct-binding reconciliation in progress

### Delivered

- Corrected the Phase 6 denominator by running the source assignment table through the
  C preprocessor with RoL's checked-in configuration. Of 1,234 discovered candidates,
  87 are compiled out; the active inventory is 1,147 direct bindings across 562 source
  handlers.
- Added a separate immutable ledger for every preprocessor exclusion. It accounts for
  disabled jail and witness systems, `#if 0` item switches and experiments, inactive
  alternate branches, and five bindings previously counted as resolved.
- Updated Phase 1 discovery to apply the same preprocessor gate on future runs. This
  prevents disabled assignments from entering pilot selection or later corpus builds;
  all 91 bindings in the existing five-package pilot were independently confirmed
  active, so its staged output is unchanged.
- Recomputed composition and `ACT_SPEC` evidence from active assignments only. The
  direct ledger is 377 resolved / 770 pending, automatic-race composition is 22 beside
  direct procedures / 225 implicit-only, and `ACT_SPEC` is 517 resolved / 331 pending.
- Archived the eleventh bounded Phase 6 delivery session since the Phase 5 closeout.
  The forward-looking estimate is now 37-69 Phase 6 sessions and 85-145 total sessions
  for Phases 6-8, or 170-580 focused engineering hours at 2-4 hours per session.

### Acceptance evidence

```text
Delivery commit: 47d12583
Reconciliation path: lib/rol-conversion/runs/phase6-special-20260812-preprocessor
Reconciliation run: rol-phase6-special-bbb3db160a0636aa
Discovered direct-binding candidates: 1,234
Source-preprocessor exclusions: 87
Active direct bindings: 1,147
Direct bindings resolved: 377
Direct bindings pending: 770
Source handlers resolved: 56
Source handlers pending: 506
Automatic-race bindings resolved: 247
ACT_SPEC records resolved: 517
ACT_SPEC records pending: 331
Complete world-tool suite: 261 passed
Production-linked CuTest suite: 623 passed
Documentation findings: 0 errors, 0 warnings, 0 info
Autotools build, test, and install: passed
Root-level circle artifact: absent
Existing pilot inactive bindings: 0 of 91
Live target writes: 0
```

Phase 6 continues with the remaining 770 direct bindings across 506 source handlers.

## 2026-08-12 - Phase 6 auto-distributor room procedure

Status: Completed checkpoint; Phase 6 direct-binding reconciliation in progress

### Delivered

- Closed all 22 active direct bindings for the shared `autoDistributor` room handler.
  Reconciliation now resolves 382 of 1,234 direct bindings and 56 of 605 handlers;
  852 bindings and 549 handlers remain.
- Added the room-owned `RoL Auto Distributor` named procedure. Any command from a
  non-staff character is intercepted and moves that character to a uniformly selected
  loaded room in the same zone; staff remain exempt and an empty or invalid zone stops
  safely with a diagnostic.
- Preserved the source callback's effective command behavior without adding a periodic
  scheduler. The source requested initialization pulses, but every pulse passed a null
  character and returned without action.
- Updated registry, owner-aware OLC, database-first help, converter, reconciliation, and
  runtime tests. The independent `ACT_SPEC` cross-check remains at 500 of 848 resolved
  because this family is room-owned.
- Archived the tenth bounded Phase 6 delivery session since the Phase 5 closeout. The
  forward-looking estimate is now 38-70 Phase 6 sessions and 86-146 total sessions for
  Phases 6-8, or 172-584 focused engineering hours at 2-4 hours per session.

### Acceptance evidence

```text
Delivery commit: ee096702
Reconciliation path: lib/rol-conversion/runs/phase6-special-20260812-auto-distributor
Reconciliation run: rol-phase6-special-053b6c0d19db7fdc
Direct bindings resolved: 382
Direct bindings pending: 852
Source handlers resolved: 56
Source handlers pending: 549
Auto-distributor bindings resolved: 22
ACT_SPEC records resolved: 500
ACT_SPEC records pending: 348
Complete world-tool suite: 260 passed
Production-linked CuTest suite: 623 passed
Documentation findings: 0 errors, 0 warnings, 0 info
Help SQL idempotency checks: 4 passed twice
Autotools build, test, and install: passed
Root-level circle artifact: absent
Live target writes: 0
```

Phase 6 continues with the remaining 852 direct bindings across 549 source handlers.

## 2026-08-12 - Phase 6 magic-pool conversion

Status: Completed checkpoint; Phase 6 direct-binding reconciliation in progress

### Delivered

- Closed all 13 active direct bindings for the shared `magic_pool` object handler.
  Reconciliation now resolves 360 of 1,234 direct bindings and 55 of 605 handlers;
  874 bindings and 550 handlers remain.
- Added the object-owned `RoL Magic Pool` named procedure. Entering a matching pool
  preserves its authored fixed damage, transition messages, and destination transport;
  invalid destinations stop safely and identify the object in the server log.
- Extended native-binding metadata with explicit object-value references. The converter
  now remaps each pool's room destination in value 0 while retaining fixed damage in
  value 1; all 12 distinct active destinations have Phase 2 identity mappings.
- Updated registry, OLC, database-first help, converter, reconciliation, and runtime
  tests. The independent `ACT_SPEC` cross-check remains at 500 of 848 resolved because
  magic pools are object procedures rather than mobile flags.

### Acceptance evidence

```text
Delivery commit: 4c084ea1
Reconciliation path: lib/rol-conversion/runs/phase6-special-20260812-magic-pool
Reconciliation run: rol-phase6-special-1b3f0ef7ec095814
Direct bindings resolved: 360
Direct bindings pending: 874
Source handlers resolved: 55
Source handlers pending: 550
Magic-pool bindings resolved: 13
Distinct destination identities resolved: 12 of 12
ACT_SPEC records resolved: 500
ACT_SPEC records pending: 348
Complete world-tool suite: 259 passed
Production-linked CuTest suite: 622 passed
Documentation findings: 0 errors, 0 warnings, 0 info
Help SQL idempotency checks: 4 passed twice
Autotools build, test, and install: passed
Root-level circle artifact: absent
Live target writes: 0
```

Phase 6 continues with the remaining 874 direct bindings across 550 source handlers.

## 2026-08-12 - Phase 6 home-reset room behavior

Status: Completed checkpoint; Phase 6 direct-binding reconciliation in progress

### Delivered

- Closed all 44 direct bindings for the shared `home_reset` room handler. The
  reconciliation now resolves 347 of 1,234 direct bindings and 54 of 605 handlers;
  887 bindings and 551 handlers remain.
- Added persistent `ROOM_ROL_HOME_RESET` conversion metadata and a composition-safe
  successful-movement hook. An NPC that walks out of a marked room now remembers the
  destination as its home without consuming the room's named SpecProc slot.
- Repaired the source callback's failed-movement edge case: blocked, invalid, or
  trigger-rejected movement no longer changes the NPC's home before it has moved.
- Extended room emission, constants extraction, generated builder documentation, and
  reconciliation evidence. The independent `ACT_SPEC` cross-check remains at 500 of
  848 resolved because `home_reset` is a room procedure rather than a mobile flag.

### Acceptance evidence

```text
Delivery commit: 2849e0a7
Reconciliation path: lib/rol-conversion/runs/phase6-special-20260812-home-reset
Reconciliation run: rol-phase6-special-1d2f58fe08372b1e
Direct bindings resolved: 347
Direct bindings pending: 887
Source handlers resolved: 54
Source handlers pending: 551
Home-reset bindings resolved: 44
ACT_SPEC records resolved: 500
ACT_SPEC records pending: 348
Complete world-tool suite: 258 passed
Production-linked CuTest suite: 620 passed
Documentation findings: 0 errors, 0 warnings, 0 info
Constants and generated-guide drift checks: passed
Autotools build and install: passed
Root-level circle artifact: absent
Live target writes: 0
```

Phase 6 continues with the remaining 887 direct bindings across 551 source handlers.

## 2026-08-12 - Phase 6 shared combat and conjured-death procedures

Status: Completed checkpoint; Phase 6 direct-binding reconciliation in progress

### Delivered

- Closed 72 direct bindings across ten reusable source handlers. The reconciliation
  now resolves 303 of 1,234 direct bindings and 53 of 605 handlers; 931 bindings and
  552 handlers remain.
- Registered seven named mobile combat procedures: acid and lightning single-target
  breath attacks plus fire, cold, acid, gas, and lightning room-wide breath weapons.
  Each preserves the source four-turn cadence; attack variants use half-level damage,
  while weapon variants use the target's corresponding full-level area spell.
- Added three persistent, composition-safe conjured-death flags for 25 familiars, ten
  mounts, and ten summoned monsters. The flags preserve each source fade message and
  suppress corpse creation without consuming the mobile's named SpecProc slot.
- Extended converter emission and preserved-mobile patching to merge the new action
  flags with existing flags, automatic-race behavior, trigger attachments, and named
  procedures. The reconciler records these callbacks as
  `NATIVE_ADAPTED_COMPOSABLE` rather than inventing a second persisted SpecProc slot.
- Updated the owner-aware registry, builder documentation, generated web guide,
  constants manifest, and database-first help source. The independent `ACT_SPEC`
  cross-check now resolves 500 of 848 records and leaves 348 pending.

### Acceptance evidence

```text
Delivery commit: d447a10b
Reconciliation path: lib/rol-conversion/runs/phase6-special-20260812-shared-combat-death
Reconciliation run: rol-phase6-special-dd7798f8ea4681cf
Direct bindings resolved: 303
Direct bindings pending: 931
Source handlers resolved: 53
Source handlers pending: 552
Conjured-death bindings resolved: 45
Breath-procedure bindings resolved: 27
ACT_SPEC records resolved: 500
ACT_SPEC records pending: 348
Complete world-tool suite: 257 passed
Production-linked CuTest suite: 620 passed
Documentation findings: 0 errors, 0 warnings, 0 info
Help SQL idempotency checks: 4 passed twice
Autotools build and install: passed
Root-level circle artifact: absent
Live target writes: 0
```

Phase 6 continues with the remaining 931 direct bindings across 552 source handlers.

## 2026-08-12 - Phase 6 automatic race procedures

Status: Completed checkpoint; Phase 6 direct-binding reconciliation in progress

### Delivered

- Implemented all 247 active boot-time race procedures independently of the ordinary
  persisted special-procedure slot: 134 demons, 101 devils, and 12 umber hulks. This
  preserves composition for the 23 prototypes that also have a direct assignment.
- Added persistent `RoL-Demon`, `RoL-Devil`, and `RoL-Umberhulk` mobile flags and
  converter-owned initialization affects. Demon and devil prototypes receive
  infravision and aggregate elemental protection; umber hulks receive aggregate
  elemental protection.
- Added composition-safe activity and combat hooks. Recognized demons and devils use
  source-derived gate chances, cooldowns, follower rules, control restrictions,
  `nogate` templates, temporary followers, combat joining, and four-game-hour expiry.
  Complex source branches that could create several independent groups are represented
  by one bounded mixed group of at most five creatures.
- Added the umber-hulk claw initialization, level-scaled combat chance, confusion gaze,
  and extra mandible attack. The canonical claw prototype is found by its exact source
  alias rather than a hard-coded converted VNUM.
- Extended preserved-mobile staging so all four action-flag and all four affect-flag
  chunks can be merged without clobbering existing flags, named procedures, trigger
  attachments, or local content. This closes the eight automatic-race records whose
  record action is `KEEP`; the refreshed Hulburg pilot proves six of those patches.
- Reconciled all 247 implicit bindings to `NATIVE_ADAPTED_COMPOSABLE`. The independent
  `ACT_SPEC` cross-check now resolves 495 of 848 records and leaves 353 records, all of
  which are tied to still-pending direct assignments.
- Documented the new builder-visible flags and automatic behavior in the mobile flag,
  OLC special-procedure, generated web, and database-first help sources.

### Acceptance evidence

```text
Delivery commits: b58faaea, e5b81b14, 2b55b265
Reconciliation path: lib/rol-conversion/runs/phase6-special-20260812-automatic-race
Reconciliation run: rol-phase6-special-519936c88c94c0da
KEEP-patch stage: lib/rol-conversion/runs/phase6-special-20260812-race-keep-stage
KEEP-patch run: rol-phase4-build-174249e9cd9cc337
Active implicit race bindings: 247
Implicit race bindings resolved: 247
Implicit race bindings pending: 0
Preserved automatic-race prototypes: 8
Refreshed pilot preserved-mobile patches: 79
Direct bindings resolved: 231
Direct bindings pending: 1,003
ACT_SPEC records resolved: 495
ACT_SPEC records pending: 353
Complete world-tool suite: 256 passed
Production-linked CuTest suite: 620 passed
Documentation findings: 0 errors, 0 warnings, 0 info
Help SQL idempotency checks: 4 passed
Autotools build and install: passed
Root-level circle artifact: absent
Staged new active errors: 0
Reset observations: passed
Walkthroughs: passed
Live target writes: 0
```

Phase 6 continues with the remaining 1,003 direct bindings across 562 source handlers.

## 2026-08-12 - Phase 6 implicit race-binding correction

Status: Completed evidence checkpoint; Phase 6 runtime reconciliation in progress

### Delivered

- Corrected the Phase 6 evidence model to inventory source boot-time race procedures
  independently of `ACT_SPEC` and direct assignment tables. The active corpus has 247
  implicit bindings: 134 `standardDemon`, 101 `standardDevil`, and 12
  `standardUmberhulk` bindings.
- Proved that 23 implicit race bindings coexist with direct mobile assignments; the
  other 224 are implicit-only. The source supports both callbacks on one prototype, so
  the target port must use a composition-safe runtime path rather than consuming or
  replacing the target's single persistent special-procedure slot.
- Added a deterministic `automatic-race-ledger.jsonl` artifact with source definition
  hashes, record and destination identities, direct-binding relationships, dispositions,
  and explicit pending status for all 247 bindings.
- Corrected the `ACT_SPEC` cross-check without changing its total status counts. Of the
  386 pending records, 343 have direct assignments only, 10 combine direct and implicit
  race procedures, and 33 expose an implicit race procedure without a direct assignment.
  The remaining 214 implicit race procedures are active despite lacking `ACT_SPEC` in
  authored source data.
- Bumped the reconciliation evidence schema to version 2 and added production-corpus
  regression coverage for the race, composition, definition, and artifact counts.

### Acceptance evidence

```text
Delivery commit: ae867c47
Reconciliation path: lib/rol-conversion/runs/phase6-special-20260812-race-composition
Reconciliation run: rol-phase6-special-caf72346b7ac8119
Active direct bindings: 1,234
Active implicit race bindings: 247
Implicit bindings alongside direct assignments: 23
Implicit-only bindings: 224
Implicit race handlers located: 3 of 3
ACT_SPEC records resolved: 462
ACT_SPEC records pending: 386
Complete world-tool suite: 254 passed
Live target writes: 0
```

The earlier Phase 6 checkpoints' count of 33 automatic procedures described only the
pending `ACT_SPEC` subset. This checkpoint supersedes that count for full active-binding
coverage; their direct-binding and handler counts remain valid.

## 2026-08-12 - Phase 6 shared mobile procedures

Status: Completed checkpoint; Phase 6 special-procedure reconciliation in progress

### Delivered

- Traced and closed 46 direct bindings across four reusable mobile families. The
  reconciliation now resolves 231 of 1,234 direct bindings and 43 of 605 handlers;
  1,003 bindings and 562 handlers remain.
- Classified all 17 source `cityguard` bindings as source-inert. The callback returns
  before its disabled aggression code, so conversion emits neither an active target
  city guard nor `MOB_SPEC`. Mapping it to the target's active city guard would have
  invented behavior.
- Added a shared RoL corpse-devourer adapter for 11 bindings. It consumes food and
  non-player corpses, preserves player corpses, and spills corpse contents before
  extraction as the source does.
- Added a shared RoL poison-bite adapter for 15 bindings. It preserves the source
  level-scaled `1 / (62 - level)` proc chance rather than using the target snake's
  different `1 / (level + 1)` chance.
- Added a shared RoL thief adapter for three bindings. It attempts the source theft
  path for every eligible mortal player on each activity call, respects peaceful
  rooms, and uses the converted target gold economy.
- Registered all three adapters as owner-aware, world-persistable mobile procedures,
  added them to both supported build manifests, and documented the expanded registry.
  The `ACT_SPEC` cross-check now resolves 462 of 848 records and leaves 386 pending.

### Acceptance evidence

```text
Delivery commit: 960f5602
Reconciliation path: lib/rol-conversion/runs/phase6-special-20260812-shared-mobile
Reconciliation run: rol-phase6-special-0f4f1274d95a2941
Active direct bindings: 1,234
Direct bindings resolved: 231
Direct bindings pending: 1,003
Distinct source handlers: 605
Source handlers resolved: 43
Source handlers pending: 562
ACT_SPEC records resolved: 462
ACT_SPEC records pending: 386
Complete world-tool suite: 254 passed
Production-linked CuTest suite: 620 passed
Documentation findings: 0 errors, 0 warnings, 0 info
Autotools build and install: passed
Root-level circle artifact: absent
Live target writes: 0
```

The next pass continues with automatic demon/devil race procedures and the next
high-reuse shared families.

## 2026-08-12 - Phase 6 inventory and shared-service reconciliation

Status: Completed checkpoint; Phase 6 special-procedure reconciliation in progress

### Delivered

- Added a deterministic Phase 6 reconciliation bundle that verifies its Phase 1,
  Phase 2, and Phase 5 inputs before accounting for all 1,234 active direct bindings,
  all 605 distinct source handlers, and all 848 active `ACT_SPEC` records. Every
  source handler definition is located and hashed; the bundle performs zero live
  target writes.
- Separated direct assignment-table truth from the mobile `ACT_SPEC` cross-check. Of
  the 848 flagged mobiles, source boot clears 444 because no callback is assigned,
  33 receive automatic demon or devil race procedures and remain Phase 6 work, and
  371 have direct assignment-table bindings.
- Closed 185 direct bindings: 108 native or DG bindings established by the Phase 4
  families, 72 shared guild, janitor, pet-shop, or receptionist bindings, and five
  source `dump` bindings whose callback returns before its unreachable behavior.
  Binding the target's active Dump procedure would have invented behavior.
- Added persistent named room-binding output to the converter and registered
  `RoL Guild Room` as a room-owned canonical procedure backed by the current target
  training service. The mobile-owned `Guild` contract remains unchanged.
- Updated the builder guide and database-first `SPECIALS` help source with the
  current registry counts, owner-aware selection rules, and the distinction between
  `Guild` and `RoL Guild Room`.
- Removed the completed Phase 6 startup instruction from the active zone scope. The
  active plans retain only the remaining 1,049 direct bindings, 566 source handlers,
  360 directly assigned `ACT_SPEC` mobiles, 33 automatic race procedures, and later
  phase gates.

### Acceptance evidence

```text
Delivery commit: 368adc90
Reconciliation path: lib/rol-conversion/runs/phase6-special-20260812-inventory-v3
Reconciliation run: rol-phase6-special-7e0556903754990d
Active direct bindings: 1,234
Direct bindings resolved: 185
Direct bindings pending: 1,049
Distinct source handlers: 605
Source handler definitions located: 605
Source handlers resolved: 39
Source handlers pending: 566
ACT_SPEC records: 848
ACT_SPEC records resolved: 455
ACT_SPEC records pending: 393
Complete world-tool suite: 253 passed
Production-linked CuTest suite: 619 passed
Documentation findings: 0 errors, 0 warnings, 0 info
Autotools build and install: passed
Root-level circle artifact: absent
Live target writes: 0
```

Phase 6 is not complete. The next pass continues with reusable generic mobile
families and the automatic race assignments before moving into consuming-package
specific procedures.
