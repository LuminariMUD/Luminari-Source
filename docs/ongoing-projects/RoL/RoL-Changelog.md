# Realms of Luminari Project Changelog
**Previous Changelog entries can be found in changelog-archive/**

This file records completed milestones removed from the active
[canonical conversion plan](REALMS_OF_LUMINARI_CANONICAL_CONVERSION_PLAN.md), which
retains the forward-looking requirements, decisions, phases, and acceptance gates. The
superseded [feature-first plan](plan-archive/REALMS_OF_LUMINARI_FEATURE_FIRST_CONVERSION_PLAN.md),
[Phase 6.5 plan](plan-archive/PHASE6_5_CANONICAL_VNUM_REBASE_PLAN.md), and
[zone conversion scope](plan-archive/REALMS_OF_LUMINARI_ZONE_CONVERSION_SCOPE.md) are
preserved in `plan-archive/`.

## 2026-08-13 - Phase 6 Undermountain Death's Head lifecycle

Status: Completed checkpoint; corrected Phase 6 binding reconciliation in progress

### Delivered

- Reconciled all five active Undermountain Death's Head bindings through one owner-aware
  `RoL Death's Head` runtime: sapling, fruit, young, and mature mobiles 2093013-2093016 and
  seed object 2093044.
- Preserved source head-count ranges, corpse-fed immature growth, larger-tree suppression,
  mature head regrowth, fruit shedding, directional cries, per-head bite chances, and ordinary
  corpse suppression. The source's always-eleven mature regrowth and unreachable wood drop
  remain explicit behavior rather than silently repaired intent.
- Preserved wandering-fruit implantation and bite behavior, including the actual source rule
  that a fruit buries a second seed only in a corpse already containing one seed. Tree bites
  retain the source's unrestricted victim handling; fruit bites remain limited to mortal PCs.
- Added lifecycle-safe object events for implanted-seed growth, source-clock translation,
  direct carrier damage, corpse germination, existing-tree cleanup, and the source quirk that a
  successful sprout leaves its seed for the next periodic pass to remove.
- Added reconciliation, transform, registry, OLC, persistence, profile, cadence, damage,
  germination-pulse, and death-flow regressions. No player helpfile changed because the batch
  adds no player command or syntax; the staff manual records the dependency-stage test matrix.
- Regenerated deterministic evidence. Resolution increases from 1,570 to 1,575 static bindings
  and from 667 to 670 direct handlers, leaving 146 bindings across 125 handlers in 27 source
  files. The independent `ACT_SPEC` cross-check advances to 816 resolved and 32 pending.
- Reforecast twenty-three corrected batches covering 329 bindings across 132 handlers. The
  remaining Phase 6 envelope remains 11-22 sessions, or 22-88 focused engineering hours; the
  full remaining project envelope remains 67-106 sessions, or 134-424 focused hours.

### Acceptance evidence

```text
Delivery commit: 55a19242
Reconciliation path: lib/rol-conversion/runs/phase6-special-20260813-deaths-head
Reconciliation run: rol-phase6-special-cc7c00546c5db049
Evidence tree SHA-256: 8977cf28391aa44303dd9c3f73ce5e19baed8fab6a52a1befe114f3791916509
Active direct bindings: 1,721
Direct bindings resolved: 1,575
Direct bindings pending: 146
Source handlers resolved: 670
Source handlers pending: 125
Additional handler families resolved: 3
Additional direct bindings resolved: 5
Native adapted bindings: 1,023
Native adapted composable bindings: 216
Source-inert exclusions: 38
ACT_SPEC records resolved: 816
ACT_SPEC records pending: 32
Complete world-tool suite: 354 passed
Production-linked CuTest suite: 688 passed
CMake build and CTest: 12 of 12 passed
Documentation findings: 0 errors, 0 warnings, 0 info
Warning-free Autotools build, test, and install: passed
Root-level circle artifact: absent
Installed ELF build ID: 4c453b2f86bffbfcd36dfdbd2ef08b840ae3b515
Installed SHA-256: 140b1cfca787d5d053f01375f95fe0d13bb83702e8217a16ce233eaaa8a8931d
Evidence artifact hashes: 8 verified
Repeat reconciliation generation: byte-identical
Live target writes: 0
```

Phase 6 continues with the next dependency-complete combat or utility family.

## 2026-08-13 - Phase 6 Undermountain drow conclave guards

Status: Completed checkpoint; corrected Phase 6 binding reconciliation in progress

### Delivered

- Reconciled all six active `um2_drowConclaveGuard` bindings through the existing exact-identity
  `RoL Monster Combat` runtime: converted mobiles 2093102, 2093108-2093112.
- Preserved the source's one-global-alarm-per-boot lifetime, visible mortal and staff gates, exact
  alarm line, and 11-room detect-invisibility sweep for the five authored guard identities.
- Preserved the first-three-guard deployment to converted rooms 2093146, 2093147, and 2093147,
  followed by the first later sergeant deployment to 2093155. Target-native room-path movement
  replaces source hunt events.
- Preserved all six combat lines and their rolls one through six of 20. The source declares an
  NPC-hit callback but implements no NPC-hit branch, so no unsupported hit behavior was invented.
- Repaired the source's wrong global-character-list advance by keeping barracks selection on the
  room list. This preserves its documented one-pass ordering without mutating unrelated rooms.
- Added reconciliation, profile, detection, destination, combat-line, alarm, redeployment, and
  one-shot runtime regressions. No player helpfile changed because the batch adds no player
  command or syntax; the staff manual records the dependency-stage test matrix.
- Regenerated deterministic evidence. Resolution increases from 1,564 to 1,570 static bindings
  and from 666 to 667 direct handlers, leaving 151 bindings across 128 handlers in 27 source
  files. The independent `ACT_SPEC` cross-check advances to 812 resolved and 36 pending.
- Reforecast twenty-two corrected batches covering 324 bindings across 129 handlers. The
  remaining Phase 6 envelope remains 11-22 sessions, or 22-88 focused engineering hours; the full
  remaining project envelope remains 67-106 sessions, or 134-424 focused hours.

### Acceptance evidence

```text
Delivery commit: eb084a07
Reconciliation path: lib/rol-conversion/runs/phase6-special-20260813-drow-conclave
Reconciliation run: rol-phase6-special-1b44c259bece3bf6
Evidence tree SHA-256: 9f4317c56c3cd001d99888ddfe4e8c6519fb1e67355ef7e7b52c54d9280ed15f
Active direct bindings: 1,721
Direct bindings resolved: 1,570
Direct bindings pending: 151
Source handlers resolved: 667
Source handlers pending: 128
Additional handler families resolved: 1
Additional direct bindings resolved: 6
Native adapted bindings: 1,018
Native adapted composable bindings: 216
Source-inert exclusions: 38
ACT_SPEC records resolved: 812
ACT_SPEC records pending: 36
Complete world-tool suite: 352 passed
Production-linked CuTest suite: 686 passed
CMake build and CTest: 12 of 12 passed
Documentation findings: 0 errors, 0 warnings, 0 info
Warning-free Autotools build, test, and install: passed
Root-level circle artifact: absent
Installed ELF build ID: c9fbcd9a13ed247d037965af399c749c94e42a49
Installed SHA-256: baa7fc90024241d7abf37edc586d2ff08b6c7bf6bb36e8f2155307e49b14f6eb
Evidence artifact hashes: 8 verified
Repeat reconciliation generation: byte-identical
Live target writes: 0
```

Phase 6 continues with the next dependency-complete combat or utility family.

## 2026-08-13 - Phase 6 paralysis gaze and venom tails

Status: Completed checkpoint; corrected Phase 6 binding reconciliation in progress

### Delivered

- Reconciled five active Dusk Road and Undermountain bindings across three handlers through the
  existing exact-identity `RoL Monster Combat` runtime.
- Added profiles for Dusk Road basilisks 2089793 and 2089794, Undermountain manscorpion 2093061,
  and wyverns 2093219 and 2094563.
- Preserved the basilisk source's level-derived one-in-four and one-in-two successful-hit chances,
  room-list scan order, victim eligibility and visibility, continue-after-resist behavior, exact
  save pressure, first-failed-target selection, and ten-round paralysis.
- Preserved critical-only manscorpion and wyvern tail effects, target-native paralysis immunity
  and Fortitude saves, random two-to-twelve-round manscorpion paralysis, and fatal wyvern venom.
  A fatal result invalidates the outer hit context so later riders cannot use the extracted target.
- Added reconciliation, exact-profile, gaze-order, save, duration, critical-gate, fatal-result,
  and production-runtime regressions. No player helpfile changed because the batch adds no player
  command or syntax; the staff manual records the dependency-stage test matrix.
- Regenerated deterministic evidence. Resolution increases from 1,559 to 1,564 static bindings
  and from 663 to 666 direct handlers, leaving 157 bindings across 129 handlers in 27 source
  files. The independent `ACT_SPEC` cross-check remains 811 resolved and 37 pending because none
  of these five active bindings has a corresponding action-table record.
- Reforecast twenty-one corrected batches covering 318 bindings across 128 handlers. The
  remaining Phase 6 envelope remains 11-22 sessions, or 22-88 focused engineering hours; the full
  remaining project envelope remains 67-106 sessions, or 134-424 focused hours.

### Acceptance evidence

```text
Delivery commit: 04d8d449
Reconciliation path: lib/rol-conversion/runs/phase6-special-20260813-paralysis-tails
Reconciliation run: rol-phase6-special-869dda48a01594d1
Evidence tree SHA-256: 000ac339b8fc6c0d040f2d98d0722f3f8fc4eab3977c9e66b2c7cd2408098bb6
Active direct bindings: 1,721
Direct bindings resolved: 1,564
Direct bindings pending: 157
Source handlers resolved: 666
Source handlers pending: 129
Additional handler families resolved: 3
Additional direct bindings resolved: 5
Native adapted bindings: 1,012
Native adapted composable bindings: 216
Source-inert exclusions: 38
ACT_SPEC records resolved: 811
ACT_SPEC records pending: 37
Complete world-tool suite: 351 passed
Production-linked CuTest suite: 685 passed
CMake build and CTest: 12 of 12 passed
Documentation findings: 0 errors, 0 warnings, 0 info
Warning-free Autotools build, test, and install: passed
Root-level circle artifact: absent
Installed ELF build ID: fa218a8ddb3eef5bab58340c15cd86c2e75de2fb
Installed SHA-256: ea0846c3eeddc008412661e8f81e1a7ac169be57244f237997bf5ddd809a3083
Evidence artifact hashes: 8 verified
Repeat reconciliation generation: byte-identical
Live target writes: 0
```

Phase 6 continues with the next dependency-complete combat or utility family.

## 2026-08-13 - Phase 6 Griffon's Nest non-Berserker aggression

Status: Completed checkpoint; corrected Phase 6 binding reconciliation in progress

### Delivered

- Reconciled all 15 active Griffon's Nest `aggroNonBarbarian` bindings through the existing
  exact-identity `RoL Monster Combat` runtime.
- Added profiles for converted guards 2010661, 2010744, 2010745, 2010749, 2010750, and
  2010754-2010763. Source Barbarian player-race eligibility maps to the target's multiclass
  Berserker role, so any Berserker level grants the authored exemption.
- Preserved command and periodic registration, first-visible-eligible room-list selection,
  immortal and NPC exemptions, immediate battle-cry attack, source visibility, and the false
  callback return that permits the incoming command to continue.
- Preserved occupied-guard behavior: a guard already fighting remembers an eligible target when
  `MOB_MEMORY` is set instead of switching opponents or repeating the battle cry.
- Added reconciliation, profile, eligibility, and production-runtime memory regressions. No
  player helpfile changed because the batch adds no player command or syntax; the staff manual
  records the complete dependency-stage test matrix.
- Regenerated deterministic evidence. Resolution increases from 1,544 to 1,559 static bindings
  and from 662 to 663 direct handlers, leaving 162 bindings across 132 handlers in 28 source
  files. The independent `ACT_SPEC` cross-check advances to 811 resolved and 37 pending.
- Reforecast twenty corrected batches covering 313 bindings across 125 handlers. The remaining
  Phase 6 envelope is 11-22 sessions, or 22-88 focused engineering hours; the full remaining
  project envelope is 67-106 sessions, or 134-424 focused hours.

### Acceptance evidence

```text
Delivery commit: 5fb8b1cb
Reconciliation path: lib/rol-conversion/runs/phase6-special-20260813-griffon-nonbarbarian
Reconciliation run: rol-phase6-special-443dfa53de680c8e
Evidence tree SHA-256: b9e0ce05e388a9cd5322310cb33e9d5036024d64e72f6981b79a50af85810737
Active direct bindings: 1,721
Direct bindings resolved: 1,559
Direct bindings pending: 162
Source handlers resolved: 663
Source handlers pending: 132
Additional handler families resolved: 1
Additional direct bindings resolved: 15
Native adapted bindings: 1,007
Native adapted composable bindings: 216
Source-inert exclusions: 38
ACT_SPEC records resolved: 811
ACT_SPEC records pending: 37
Complete world-tool suite: 350 passed
Production-linked CuTest suite: 684 passed
CMake build and CTest: 12 of 12 passed
Documentation findings: 0 errors, 0 warnings, 0 info
Warning-free Autotools build, test, and install: passed
Root-level circle artifact: absent
Installed ELF build ID: 3576341236ba90626de888c84de55c187865fc51
Installed SHA-256: 5514c62e61baa53032b27753e7cf1f44778f234261981c4f7cafbedf6f6ebfa4
Evidence artifact hashes: 8 verified
Repeat reconciliation generation: byte-identical
Live target writes: 0
```

Phase 6 continues with the next dependency-complete combat or utility family.

## 2026-08-13 - Phase 6 source death-effects family

Status: Completed checkpoint; corrected Phase 6 binding reconciliation in progress

### Delivered

- Reconciled eight active death bindings across seven source handlers from Trahern, Dobluth,
  and Undermountain through the existing composition-safe mobile-death gateway.
- Added exact converted profiles for both elemental-fire weevils 2020221 and 2020267, Lady
  Aleanrahel 2021783, Helmed Horror 2092062, Butcher Knife 2093017, gargoyle 2093018, crystal
  golem 2093020, and white pudding 2093301.
- Preserved each authored corpse outcome and death message. Lady Aleanrahel becomes banshee
  2021820 with carried and equipped items transferred; the horror and butcher drop mapped
  objects 2092091 and 2093048; the gargoyle and golem shatter without corpses; and the active
  white-pudding generation splits into exactly two mobiles 2093330.
- Preserved the weevil source's one shared `25d2` fire roll, ordinary corpse, area eligibility,
  and cumulative room-order protection halving rather than normalizing the mutable damage value.
- Added converter, reconciliation, exact-profile, damage-order, mapped-split, and production
  runtime coverage. No player helpfile changed because this batch adds no player command or
  syntax; the staff manual records the dependency-stage test matrix.
- Regenerated deterministic evidence. Resolution increases from 1,536 to 1,544 static bindings
  and from 655 to 662 direct handlers, leaving 177 bindings across 133 handlers in 29 source
  files. The independent `ACT_SPEC` cross-check advances to 809 resolved and 39 pending.
- Reforecast nineteen corrected batches covering 298 bindings across 124 handlers. The
  remaining Phase 6 envelope is 12-21 sessions, or 24-84 focused engineering hours; the full
  remaining project envelope is 68-105 sessions, or 136-420 focused hours.

### Acceptance evidence

```text
Delivery commit: 53b6a3e4
Reconciliation path: lib/rol-conversion/runs/phase6-special-20260813-source-death-effects
Reconciliation run: rol-phase6-special-59a0f3f782fc7f30
Evidence tree SHA-256: 3566db472325a01673827d7125561cec023d299d7248d3ac7bb568cba1ae9fcc
Active direct bindings: 1,721
Direct bindings resolved: 1,544
Direct bindings pending: 177
Source handlers resolved: 662
Source handlers pending: 133
Additional handler families resolved: 7
Additional direct bindings resolved: 8
Native adapted bindings: 992
Native adapted composable bindings: 216
Source-inert exclusions: 38
ACT_SPEC records resolved: 809
ACT_SPEC records pending: 39
Complete world-tool suite: 349 passed
Production-linked CuTest suite: 683 passed
CMake build and CTest: 12 of 12 passed
Documentation findings: 0 errors, 0 warnings, 0 info
Warning-free Autotools build, test, and install: passed
Root-level circle artifact: absent
Installed ELF build ID: 7ef432a8e35171926abd0b8def3677b31b80f863
Installed SHA-256: d2762087bdb743df847c69bd4e02aed1d0215b3fd4cc6d477418ce8b6f504f65
Evidence artifact hashes: 8 verified
Repeat reconciliation generation: byte-identical
Live target writes: 0
```

Phase 6 continues with the next dependency-complete combat or utility family.

## 2026-08-13 - Phase 6 Undermountain ambient and inert-stench family

Status: Completed checkpoint; corrected Phase 6 binding reconciliation in progress

### Delivered

- Reconciled 14 active Undermountain bindings across nine handlers. Eight exact mobile
  identities now use generated source-periodic profiles; all six registrations of the
  compile-disabled troglodyte stench callback are explicitly source-inert.
- Added source-derived profiles for the Mad Mage, Juris, Deriah, Talugen, the succubus,
  Sha'Tar, the imp, and Bhara'Tir at converted identities 2093012, 2093021-2093023,
  2093202, 2093211, 2093225, and 2093304.
- Preserved all 61 authored outcomes, the exact zero-to-100 random ranges, awake and
  not-fighting gates, speech versus room-action dispatch, source strings, and Talugen's
  expanded `frown` social. Unrelated mobile identities fail closed.
- Recorded troglodyte source identities 2093404-2093408 and 2093426 without a replacement
  callback because both event registration and the stench implementation are inside
  `#if 0`; adding target behavior would contradict the active source.
- Added generator, reconciliation, generated-table, and production-runtime coverage. No
  player helpfile changed because this batch adds no player command or syntax; the staff
  manual records the dependency-stage test matrix.
- Regenerated deterministic evidence. Resolution increases from 1,522 to 1,536 static
  bindings and from 646 to 655 direct handlers, leaving 185 bindings across 140 handlers in
  29 source files. The independent `ACT_SPEC` cross-check advances to 808 resolved and 40
  pending.
- Reforecast eighteen corrected batches covering 290 bindings across 117 handlers. The
  remaining Phase 6 envelope is 12-22 sessions, or 24-88 focused engineering hours; the full
  remaining project envelope is 68-106 sessions, or 136-424 focused hours.

### Acceptance evidence

```text
Delivery commit: fcfd2be6
Reconciliation path: lib/rol-conversion/runs/phase6-special-20260813-undermountain-socials
Reconciliation run: rol-phase6-special-b66f1c18acb2ce12
Evidence tree SHA-256: 44c456548a90cb1f09ff1264c25b37e318ce8cc9ff07e0ce9a0525f9e93a7664
Active direct bindings: 1,721
Direct bindings resolved: 1,536
Direct bindings pending: 185
Source handlers resolved: 655
Source handlers pending: 140
Additional handler families resolved: 9
Additional direct bindings resolved: 14
Native adapted bindings: 992
Native adapted composable bindings: 208
Source-inert exclusions: 38
ACT_SPEC records resolved: 808
ACT_SPEC records pending: 40
Complete world-tool suite: 347 passed
Production-linked CuTest suite: 682 passed
CMake build and CTest: 12 of 12 passed
Documentation findings: 0 errors, 0 warnings, 0 info
Warning-free Autotools build, test, and install: passed
Root-level circle artifact: absent
Installed ELF build ID: 363b5b57e55c2b2b6d45ca794e58b34fdbcf3274
Installed SHA-256: e8af98f53fc082f5d2a807f16b302fd0228d474b41ad4f8441767987b17dc784
Evidence artifact hashes: 8 verified
Repeat reconciliation generation: byte-identical
Live target writes: 0
```

Phase 6 continues with the next dependency-complete combat, death, or utility family.

## 2026-08-13 - Phase 6 drow-equipment decay family

Status: Completed checkpoint; corrected Phase 6 binding reconciliation in progress

### Delivered

- Reconciled all 16 active `genericDrowEq` object bindings in the Undermountain source family
  through one exact target runtime. The preprocessor-disabled seventeenth assignment remains in
  the excluded ledger and does not inflate live progress.
- Added the typed `RoL Drow Equipment` procedure and object event for converted identities
  2092080-2092082, 2092096, 2093081-2093085, 2093087, and 2093150-2093155. Conversion also adds
  `ITEM_AUTOPROC`, and unrelated object identities fail closed.
- Preserved the source hourly event lifecycle: the first event starts after one MUD hour,
  converted Underdark sectors stop decay, a later surface command restarts a stopped event, and
  continuing surface events retain the source +/-4-pulse jitter translated from its four-Hz
  clock to the target pulse rate.
- Preserved the source's always-true daybreak and daytime predicates, direct-sunlight
  acceleration, slower nested-object decay, no-sell marking, integer reductions to cost and
  weight, weapon-dice consolidation, armor reduction, first-two-affect mutation, owner-visible
  messages, and terminal extraction. The unreachable source level-above-50 maintenance command
  was not exposed in the target's level-34 staff model.
- Added exact-profile, sector-map, cadence, arithmetic, event, registry, persistence, OLC,
  converter, reconciliation, and production-runtime coverage. No player helpfile changed because
  the batch adds no player command or syntax; the staff manual records the dependency-stage test
  matrix.
- Regenerated deterministic evidence. Resolution increases from 1,506 to 1,522 static bindings
  and from 645 to 646 direct handlers, leaving 199 bindings across 149 handlers in 29 source
  files. The independent `ACT_SPEC` cross-check remains 806 resolved and 42 pending.
- Reforecast seventeen corrected batches covering 276 bindings across 108 handlers. The remaining
  Phase 6 envelope is 13-24 sessions, or 26-96 focused engineering hours; the full remaining
  project envelope is 69-108 sessions, or 138-432 focused hours.

### Acceptance evidence

```text
Delivery commit: be32b3d3
Reconciliation path: lib/rol-conversion/runs/phase6-special-20260813-drow-equipment
Reconciliation run: rol-phase6-special-477a41d7687e3e25
Evidence tree SHA-256: 05194e39763ecff2fde376be041e57d02e38189ce84751d672846f10e8e71ff4
Active direct bindings: 1,721
Direct bindings resolved: 1,522
Direct bindings pending: 199
Source handlers resolved: 646
Source handlers pending: 149
Additional handler families resolved: 1
Additional direct bindings resolved: 16
Native adapted bindings: 984
Native adapted composable bindings: 208
Source-inert exclusions: 32
ACT_SPEC records resolved: 806
ACT_SPEC records pending: 42
Complete world-tool suite: 345 passed
Production-linked CuTest suite: 682 passed
CMake build and CTest: 12 of 12 passed
Documentation findings: 0 errors, 0 warnings, 0 info
Warning-free Autotools build, test, and install: passed
Root-level circle artifact: absent
Installed ELF build ID: 779abfbd60c34b72f160eb8f9505030731d13379
Installed SHA-256: 89ab7ccc83a67f014455709be4691f32632004ea2cb8beab3c909998c0b4a8ff
Evidence artifact hashes: 8 verified
Repeat reconciliation generation: byte-identical
Live target writes: 0
```

Phase 6 continues with the next dependency-complete combat, death, or utility family.

## 2026-08-13 - Phase 6 Darkhold special family

Status: Completed checkpoint; corrected Phase 6 binding reconciliation in progress

### Delivered

- Reconciled all 16 active Darkhold bindings across nine source handlers: six summon-skull
  assignments, one passage skull, four passage gems, two weapon procs, two shadow-fiend hit
  callbacks, and one shadow-dragon death callback.
- Added the typed `RoL Darkhold Object` procedure for 11 exact object identities. The six summon
  skulls create mobile 2094500 without consumption; passage skull 2094504 opens the blocked north
  passage in room 2094666; and the four gems intercept exact-room drops to repair the authored
  south or north destinations.
- Extended `RoL Monster Combat` for shadow fiend 2094505 and shadow dragon 2094506. The fiend
  preserves independently cooling lit-room darkness and one-in-six mind steal, including staff
  exemption, source-pressure Will save, 45d10 source-untyped damage, and blackmantle-aware healing.
  The dragon reveals and unlocks the north exit of room 2094675 on death.
- Extended `RoL Weapon Proc` for warhammer 2094571 and bastard sword 2094566. The warhammer keeps
  its primary/two-handed one-in-21 30d10 ice proc; the sword keeps its critical-only paired
  zero-duration hit/damage penalties, exact NPC and PC ranges, and nonstacking continuation path.
- Added exact-profile, registry, conversion, passage, death, weapon-payload, reconciliation, and
  production-runtime coverage. No player helpfile changed because this batch adds no player
  command or syntax; the staff manual records the dependency-stage test matrix.
- Regenerated deterministic evidence. Resolution increases from 1,490 to 1,506 static bindings
  and from 636 to 645 direct handlers, leaving 215 bindings across 150 handlers in 30 source
  files. The independent `ACT_SPEC` cross-check advances to 806 resolved and 42 pending.
- Reforecast sixteen corrected batches covering 260 bindings across 107 handlers. The remaining
  Phase 6 envelope is 14-23 sessions, or 28-92 focused engineering hours; the full remaining
  project envelope is 70-107 sessions, or 140-428 focused hours.

### Acceptance evidence

```text
Delivery commit: 90adccdb
Reconciliation path: lib/rol-conversion/runs/phase6-special-20260813-darkhold
Reconciliation run: rol-phase6-special-2b80ad51d6738778
Evidence tree SHA-256: dfba4dbd1356e111bb74af08f7ca7734fd26fbc08b585238fa01d78350d5cd21
Active direct bindings: 1,721
Direct bindings resolved: 1,506
Direct bindings pending: 215
Source handlers resolved: 645
Source handlers pending: 150
Additional handler families resolved: 9
Additional direct bindings resolved: 16
Native adapted bindings: 968
Native adapted composable bindings: 208
Source-inert exclusions: 32
ACT_SPEC records resolved: 806
ACT_SPEC records pending: 42
Complete world-tool suite: 343 passed
Production-linked CuTest suite: 680 passed
CMake build: passed
Documentation findings: 0 errors, 0 warnings, 0 info
Warning-free Autotools build, test, and install: passed
Root-level circle artifact: absent
Installed ELF build ID: f674ff2371169a399a43a36bc2410681ccdff5bb
Installed SHA-256: 8d46eea11db53ab58e7952aa1500aea97776dcefe71a06ab5b16e2d22106eb8d
Evidence artifact hashes: 8 verified
Repeat reconciliation generation: byte-identical
Live target writes: 0
```

Phase 6 continues with the next dependency-complete combat, death, or utility family.

## 2026-08-13 - Phase 6 Zhentil periodic family

Status: Completed checkpoint; corrected Phase 6 binding reconciliation in progress

### Delivered

- Reconciled six active Zhentil Keep mobile bindings across six source handlers: minstrel,
  little girl, terrified merchant, visiting dignitary, Scornubian trader, and ugly prostitute.
- Extended `RoL Source Periodic` to 128 exact mobile identities across 118 source handlers, 466
  outcomes, and 729 actions. The six profiles preserve the source inclusive random ranges,
  awake and non-fighting gates, room acts, socials, hidden visibility, and ordered compound
  outcomes.
- Extended the source-table generator to accept a tightly validated zero-roll conditional in
  addition to random switches. It requires exactly one random-number call, an action-bearing
  braced branch, and a true return, preserving the one-in-11 little-girl wiggle and one-in-16
  terrified-merchant shudder without hand-authored tables.
- Kept gate guard 2081074 pending because its callback also delegates command handling and owns
  time-of-day gate repair and event rescheduling; adapting only its ambient branch would be an
  incomplete behavior port.
- Added generator, converter, reconciliation, exact-VNUM, roll-shape, gate, visibility, action,
  and production-runtime regression coverage. No player helpfile changed because the batch adds
  no player command or syntax; the staff manual records the dependency-stage test matrix.
- Regenerated deterministic evidence. Resolution increases from 1,484 to 1,490 static bindings
  and from 630 to 636 direct handlers, leaving 231 bindings across 159 handlers in 31 source
  files. The independent `ACT_SPEC` cross-check remains 805 resolved and 43 pending.
- Reforecast fifteen corrected batches covering 244 bindings across 98 handlers. The remaining
  Phase 6 envelope is 15-25 sessions, or 30-100 focused engineering hours; the full remaining
  project envelope is 71-109 sessions, or 142-436 focused hours.

### Acceptance evidence

```text
Delivery commit: afe6b68d
Reconciliation path: lib/rol-conversion/runs/phase6-special-20260813-zhentil-periodic
Reconciliation run: rol-phase6-special-da5452c48ee34106
Evidence tree SHA-256: 1959db856353624f80d24208b7620fb3cf56c250d8234fce7997ad1973d579e8
Active direct bindings: 1,721
Direct bindings resolved: 1,490
Direct bindings pending: 231
Source handlers resolved: 636
Source handlers pending: 159
Additional handler families resolved: 6
Additional direct bindings resolved: 6
Native adapted bindings: 952
Native adapted composable bindings: 208
Source-inert exclusions: 32
ACT_SPEC records resolved: 805
ACT_SPEC records pending: 43
Complete world-tool suite: 341 passed
Production-linked CuTest suite: 677 passed
Documentation findings: 0 errors, 0 warnings, 0 info
Warning-free Autotools build, test, and install: passed
Root-level circle artifact: absent
Installed ELF build ID: d5fd8bbb2958d56032d46f3f903f0e38a1306dfb
Installed SHA-256: a7a0c956fe2edcee95e8ede3d7ac62f5c3149f3446338fafd33e5c49c6f586e8
Evidence artifact hashes: 8 verified
Repeat reconciliation generation: byte-identical
Live target writes: 0
```

Phase 6 continues with the next dependency-complete combat, death, or utility family.

## 2026-08-13 - Phase 6 Scornubel family

Status: Completed checkpoint; corrected Phase 6 binding reconciliation in progress

### Delivered

- Reconciled all 18 active Scornubel bindings across 14 source handlers: 17 mobile assignments
  across 13 periodic callbacks and fiery mace object 2006084.
- Extended the generated `RoL Source Periodic` table with 17 exact mobile identities. The
  profiles preserve the source zero-to-15, zero-to-20, and zero-to-50 roll ranges, awake and
  non-fighting gates, speech, socials, room acts, hidden acts, and multi-action outcomes.
- Composed Parchimil 2006061 through the existing `RoL Guild Guard` procedure so its source
  periodic behavior and guild passage behavior share one persistent special-procedure slot.
  The converter also requires `MOB_SPEC` for this composed identity.
- Added fiery mace 2006084 to `RoL Weapon Proc`. Primary, two-handed, and offhand hits preserve
  the source one-in-36 fire-corona chance, fixed 100-point source-untyped damage, messages, and
  target invalidation safety.
- Added generator, converter, reconciliation, profile, composition, probability, damage, and
  exact-VNUM regression coverage. No player helpfile changed because the batch adds no player
  command or syntax; the staff manual records the dependency-stage test matrix.
- Regenerated deterministic evidence. Resolution increases from 1,466 to 1,484 static bindings
  and from 616 to 630 direct handlers, leaving 237 bindings across 165 handlers in 31 source
  files. The independent `ACT_SPEC` cross-check remains 805 resolved and 43 pending.
- Reforecast fourteen corrected batches covering 238 bindings across 92 handlers. The remaining
  Phase 6 envelope is 14-26 sessions, or 28-104 focused engineering hours; the full remaining
  project envelope is 70-110 sessions, or 140-440 focused hours.

### Acceptance evidence

```text
Delivery commit: 8a741c3c
Reconciliation path: lib/rol-conversion/runs/phase6-special-20260813-scornubel
Reconciliation run: rol-phase6-special-885945d3c68756af
Evidence tree SHA-256: 9dc278a34cede48237663907a9e32c1db067029f154513b4bc06233d38d24df1
Active direct bindings: 1,721
Direct bindings resolved: 1,484
Direct bindings pending: 237
Source handlers resolved: 630
Source handlers pending: 165
Additional handler families resolved: 14
Additional direct bindings resolved: 18
Native adapted bindings: 946
Native adapted composable bindings: 208
Source-inert exclusions: 32
ACT_SPEC records resolved: 805
ACT_SPEC records pending: 43
Complete world-tool suite: 339 passed
Production-linked CuTest suite: 676 passed
Documentation findings: 0 errors, 0 warnings, 0 info
Warning-free Autotools build, test, and install: passed
Root-level circle artifact: absent
Installed ELF build ID: 6eabc923cb6fe47fb81084befada3e24f94df3bc
Installed SHA-256: 1debdb362bdb492b4dc35d41cfc1b6a17a1cddfdd11246d71c680e39591c8f75
Evidence artifact hashes: 8 verified
Repeat reconciliation generation: byte-identical
Live target writes: 0
```

Phase 6 continues with the next dependency-complete combat, death, or utility family.

## 2026-08-13 - Phase 6 Avernus lifecycle

Status: Completed checkpoint; corrected Phase 6 binding reconciliation in progress

### Delivered

- Reconciled the remaining 21 active Avernus lifecycle bindings across 14 source handlers.
  Twenty bindings now use exact native mobile, object, or room profiles; object 2033006 is an
  explicit source-inert exclusion because `avernus_seal_unload` never parses an event and never
  registers its extraction callback.
- Added 15 exact Avernus mobile profiles through the persistent `RoL Monster Combat` procedure.
  Man 2032623 transforms into Kri'ik with his possessions; the Erinyes owns and restores its
  room-illusion text safely; prisoner 2032660 rewards its leader and departs at Coidon; deva
  2033005 delivers alignment-aware echoes; three Rogues rehide; and black altar 2033026 heals,
  removes blindness, and suppresses its corpse.
- Preserved the forward and reverse ring and citadel patrol tables and the prison patrol's door
  sequence. Unsafe source-global prison state is now per mobile, patrols recover toward their
  load room when displaced, and citadel patrols retain their intruder response.
- Preserved Bel 2033014's regeneration and bless maintenance, pet purge, combat door lock,
  shieldpunch interception, and guard-sacrifice replacement. The replacement inherits Bel's
  possessions at full health and reclaims the surviving guards.
- Added the typed `RoL Avernus Object` procedure. Rod 2032631 preserves its held-only exact
  bargain phrase, home restriction, Kri'ik, Cornugon, green orb, and self-destruction. Bel's
  sword 2033011 preserves return, punishment, devil restriction, damage degradation, and its
  one-in-six fire corona with native area safety and saves.
- Preserved both dancing daggers' one-in-ten high-level release, hidden object state, exact
  object-to-helper identity, helper weapon, owner combat, mortal order rejection, one-in-five
  return, loss recovery, and staff continual-light phrase. Owner identity is stable across
  simultaneous daggers rather than relying on an ambiguous follower VNUM.
- Added a typed room-activity gateway and the `RoL Avernus Garden` procedure. Only authored room
  2032672 is scheduled, and it scans the 16 authored garden rooms for save-gated calm or sleep,
  rage, escape-pool, no-sleep, pet, and staff exceptions without a world-wide room scan.
- Added converter, reconciliation, profile, route, event-contract, object-autoproc, garden-range,
  and lifecycle coverage for all 21 source bindings. No player helpfile changed because the
  batch adds no player command or syntax; the staff manual records the full dependency-stage
  test matrix.
- Regenerated deterministic evidence. Resolution increases from 1,445 to 1,466 static bindings
  and from 602 to 616 direct handlers, leaving 255 bindings across 179 handlers in 32 source
  files. The independent `ACT_SPEC` cross-check remains 805 resolved and 43 pending.
- Reforecast thirteen corrected batches covering 220 bindings across 78 handlers. The remaining
  Phase 6 envelope is 16-30 sessions, or 32-120 focused engineering hours; the full remaining
  project envelope is 72-114 sessions, or 144-456 focused hours.

### Acceptance evidence

```text
Delivery commit: c15a7aa9
Reconciliation path: lib/rol-conversion/runs/phase6-special-20260813-avernus-lifecycle
Reconciliation run: rol-phase6-special-80e5cc527bee919a
Evidence tree SHA-256: b5772333c34736713da2df0ddd84a548efdad7e86a69f66634844700f7b1d446
Active direct bindings: 1,721
Direct bindings resolved: 1,466
Direct bindings pending: 255
Source handlers resolved: 616
Source handlers pending: 179
Additional handler families resolved: 14
Additional direct bindings resolved: 21
Native adapted bindings: 928
Native adapted composable bindings: 208
Source-inert exclusions: 32
ACT_SPEC records resolved: 805
ACT_SPEC records pending: 43
Complete world-tool suite: 337 passed
Production-linked CuTest suite: 675 passed
Documentation findings: 0 errors, 0 warnings, 0 info
Warning-free Autotools build, test, and install: passed
Root-level circle artifact: absent
Installed ELF build ID: 96df5ad9fa3f6d1ac2bc3534ea06cce66078d441
Installed SHA-256: 077e66321f6cd4d42ce8cc385161a6ad8af8dc59303353664f918874e454ef21
Evidence artifact hashes: 8 verified
Repeat reconciliation generation: byte-identical
Live target writes: 0
```

Phase 6 continues with the next dependency-complete combat, death, or utility family.

## 2026-08-13 - Phase 6 Avernus devil combat

Status: Completed checkpoint; corrected Phase 6 binding reconciliation in progress

### Delivered

- Reconciled 22 Avernus bindings across seven source handlers: a Tiamat dragon alert, Barbazu
  reactive rage, Barbazu glaive wounds, Gelugon freezing tails, Meritos's caster-silencing bolt,
  Hanariel's disarm interception, and the Gelugon freezing spear.
- Added a typed mobile-was-hit gateway after successful weapon handling. Fourteen exact Barbazu
  identities now retain their source one-in-20 received-hit rage, five-tick doubled current
  hitroll and damroll, half-maximum-hit-point increase, and nonstacking affect marker.
- Composed dragon 2032622 with `RoL Guild Guard` and an exact alert profile. Its successful hit
  sends the Tiamat defense call once per fight, recruits helper 2036180 within the source 30-room
  distance, and resets its alert state after combat.
- Preserved critical-only Barbazu glaive behavior for objects 2032602 and 2033001. NPC targets
  receive the source 100-point wound; player targets receive independently stacked blood-loss
  events that remove 40 hit points every three violence pulses, floor at -9, persist while the
  victim is below -5, and resume after healing.
- Preserved both Gelugons' actual one-in-seven tail trigger despite the source's one-in-ten
  comment, with native paralysis immunity, Fortitude saves, and one-to-two-tick paralysis.
  Meritos preserves one-in-four first-caster targeting, resistance, the source +5 Will save, and
  four-tick silence while repairing the source null-target crash. Hanariel preserves the mortal
  disarm block, sitting trip, and three-round wait.
- Preserved spear 2033012's one-in-three, 2d4-tick slow and recurring bad-owner penalty. Invalid
  owners take 5-50 fire damage and the weapon degrades to 1d1; non-pet devil NPCs and staff are
  exempt.
- Added exact converter, reconciliation, event-contract, profile, probability, affect, event,
  alert-distance, and owner-restriction coverage. No player helpfile changed because the batch
  adds no player command or syntax; the staff manual records the dependency-stage test matrix.
- Regenerated deterministic evidence. Resolution increases from 1,423 to 1,445 static bindings
  and from 595 to 602 direct handlers, leaving 276 bindings across 193 handlers in 33 source
  files. The independent `ACT_SPEC` cross-check remains 805 resolved and 43 pending.
- Reforecast twelve corrected batches covering 199 bindings across 64 handlers. The remaining
  Phase 6 envelope is 17-37 sessions, or 34-148 focused engineering hours; the full remaining
  project envelope is 73-121 sessions, or 146-484 focused hours.

### Acceptance evidence

```text
Delivery commit: 31282808
Reconciliation path: lib/rol-conversion/runs/phase6-special-20260813-avernus-devil-combat
Reconciliation run: rol-phase6-special-bc771a97be7031ca
Evidence tree SHA-256: 88783b6e1f1dd358077f3e05643b552ade2994f47bc20637ad2b8d6e80bde3ed
Active direct bindings: 1,721
Direct bindings resolved: 1,445
Direct bindings pending: 276
Source handlers resolved: 602
Source handlers pending: 193
Additional handler families resolved: 7
Additional direct bindings resolved: 22
Native adapted bindings: 908
Native adapted composable bindings: 208
Source-inert exclusions: 31
ACT_SPEC records resolved: 805
ACT_SPEC records pending: 43
Complete world-tool suite: 336 passed
Production-linked CuTest suite: 674 passed
Documentation findings: 0 errors, 0 warnings, 0 info
Warning-free Autotools build, test, and install: passed
Root-level circle artifact: absent
Installed ELF build ID: 083d8d74db9e0c4dc5fbbd964af10140c29785da
Installed SHA-256: 0b84821aade13cddb8443dfc244154f283407c69fdbf2661930fb5ebc6ed35b3
Evidence artifact hashes: 8 verified
Repeat reconciliation generation: byte-identical
Live target writes: 0
```

Phase 6 continues with the remaining 13 Avernus bindings across 12 stateful handlers, followed
by the next dependency-complete combat, death, or utility family.

## 2026-08-13 - Phase 6 planar capture, charm, and Vrock dance

Status: Completed checkpoint; corrected Phase 6 binding reconciliation in progress

### Delivered

- Reconciled 11 bindings across five planar handlers on exact `RoL Monster Combat` identity
  profiles: Glabrezu grab, Marilith tail, Succubus charm, Succubus captive commands, and Vrock
  dance of ruin.
- Preserved the Glabrezu and Marilith one-in-11 trigger, strict raw-Dexterity half-stat evasion,
  1-85 raw-Constitution survival check, lethal crush, charmed-follower captivity, source command
  whitelist, and delayed ordinary-combat attack.
- Preserved male mortal targeting, one-in-four charm attempts, the source -2 save pressure,
  one-to-four-MUD-hour lethal kisses, combat deferral, and one-at-a-time captive deaths for
  Succubi. The source Antipaladin role maps to target Blackguards; target-native mind blank,
  no-charm equipment, charm immunity, spell resistance, and Will saves protect eligible PCs.
- Added the full five-member Vrock dance: three violence-pulse stages, disabled-state timer
  progress, leader recovery, below-five abort, one shared 20d10 lightning roll, target-native
  Reflex saves and area safety, and a one-MUD-day cohort cooldown.
- Repaired the source dance's wrong-variable defect. Abort and completion now clear all dancers
  instead of repeatedly clearing the callback owner and leaving peers permanently disabled.
- Added exact converter, source-inventory, profile, threshold, command, delay, dance, and typed
  command-gateway coverage. No player helpfile changed because the batch adds no player command
  or syntax; the staff manual records the dependency-stage test matrix.
- Regenerated deterministic evidence. Resolution increases from 1,412 to 1,423 static bindings
  and from 590 to 595 direct handlers, leaving 298 bindings across 200 handlers in 33 source
  files. The independent `ACT_SPEC` cross-check moves to 805 resolved and 43 pending.
- Reforecast eleven corrected batches covering 177 bindings across 57 handlers. The remaining
  Phase 6 envelope is 19-39 sessions, or 38-156 focused engineering hours; the full remaining
  project envelope is 75-123 sessions, or 150-492 focused hours.

### Acceptance evidence

```text
Delivery commit: ba20b88a
Reconciliation path: lib/rol-conversion/runs/phase6-special-20260813-planar-capture-charm-dance
Reconciliation run: rol-phase6-special-dd88e66849111ed1
Evidence tree SHA-256: d48594262af67ecc335a571ad43da69325523c44490e9c5c278ef2929c841b53
Active direct bindings: 1,721
Direct bindings resolved: 1,423
Direct bindings pending: 298
Source handlers resolved: 595
Source handlers pending: 200
Additional handler families resolved: 5
Additional direct bindings resolved: 11
Native adapted bindings: 887
Native adapted composable bindings: 207
Source-inert exclusions: 31
ACT_SPEC records resolved: 805
ACT_SPEC records pending: 43
Complete world-tool suite: 335 passed
Production-linked CuTest suite: 673 passed
Documentation findings: 0 errors, 0 warnings, 0 info
Warning-free Autotools build, test, and install: passed
Root-level circle artifact: absent
Installed ELF build ID: a164aca4acd785fb02a4eb3a4ae262cf086efb09
Installed SHA-256: c5a0b194c45fc2f9ac1d2e5b56b84818ec397b843cf50f6e18ef3904449aa83f
Evidence artifact hashes: 8 verified
Repeat reconciliation generation: byte-identical
Live target writes: 0
```

Phase 6 continues with the highest-value dependency-complete pending combat, death, or utility
family selected through source and target call-path tracing.

## 2026-08-13 - Phase 6 planar deaths, bursts, and Balor weapons

Status: Completed checkpoint; corrected Phase 6 binding reconciliation in progress

### Delivered

- Reconciled 18 planar bindings across eight source handlers. Manes, Balors, Vrocks, and
  Spinagons use exact identity profiles on `RoL Monster Combat`; Balor whip 2093227 and
  lightning sword 2093228 use `RoL Weapon Proc`.
- Preserved the Manes' 4d6 save-gated acid death burst and ordinary corpse, and the Balors'
  elemental protection, missing-weapon provisioning, abyss-forged weapon dissolution, and
  ordinary-corpse suppression through the typed mobile-death gateway.
- Preserved independent Vrock screech and spore paths: low-health PC-only Constitution checks,
  silence and soundproof gates, one-round stun, one-MUD-day screech cooldown, five-in-six 10d2
  poison spores, and a separate three-round spore cooldown.
- Preserved the Spinagon's five-in-six trigger, 2-5 target-native safe area targets, elemental
  protection save bonus, and three-round cooldown. The port uses the active source code's actual
  `20d2` fire roll instead of its contradictory `2d20` comment.
- Added demon-only Balor weapon ownership and automatic destruction for invalid carried or worn
  owners. Pets may retain the objects but cannot trigger their combat procs. The whip adds 8d6
  unavoidable force damage on each qualifying hit; sword criticals add 20d10 negative energy
  through either a one-in-five safe room burst or a direct eight-affect ability-penalty branch.
- Recorded both Chasme buzz bindings as source-inert. The callback tests the Chasme owner's demon
  race rather than the victim, so the authored sleep branch cannot run for either bound mobile.
- Added exact converter, reconciliation, profile, roll, cooldown, ownership, death, and weapon
  coverage. No player helpfile changed because the batch introduces no player command or syntax;
  the staff manual records the complete dependency-stage test matrix.
- Regenerated deterministic evidence. Resolution increases from 1,394 to 1,412 static bindings
  and from 582 to 590 direct handlers, leaving 309 bindings across 205 handlers in 34 source
  files. The independent `ACT_SPEC` cross-check moves to 801 resolved and 47 pending.
- Reforecast ten corrected batches covering 166 bindings across 52 handlers. The remaining
  Phase 6 envelope is 19-40 sessions, or 38-160 focused engineering hours; the full remaining
  project envelope is 75-124 sessions, or 150-496 focused hours.

### Acceptance evidence

```text
Delivery commit: 336ee930
Reconciliation path: lib/rol-conversion/runs/phase6-special-20260813-planar-death-bursts-weapons
Reconciliation run: rol-phase6-special-1f5bf8c5d82f76a4
Evidence tree SHA-256: 677be3adf7d8dae6bb3123e8ac9646effc9597ca31970861d4c1224e4c0b5a46
Active direct bindings: 1,721
Direct bindings resolved: 1,412
Direct bindings pending: 309
Source handlers resolved: 590
Source handlers pending: 205
Additional handler families resolved: 8
Additional direct bindings resolved: 18
Native adapted bindings: 876
Native adapted composable bindings: 207
Source-inert exclusions: 31
ACT_SPEC records resolved: 801
ACT_SPEC records pending: 47
Complete world-tool suite: 335 passed
Production-linked CuTest suite: 672 passed
Documentation findings: 0 errors, 0 warnings, 0 info
Warning-free Autotools build, test, and install: passed
Root-level circle artifact: absent
Installed ELF build ID: 1cf293861de5d4835aef3e2879252d332bacd337
Installed SHA-256: d87dbdd1c7ef5641ca35e0baef6f54da63960b43be52b9da5648fa87199619cb
Evidence artifact hashes: 8 verified
Repeat reconciliation generation: byte-identical
Live target writes: 0
```

Phase 6 continues with the remaining planar capture, charm, and Vrock-dance group, followed by
the next dependency-complete shared-runtime family.

## 2026-08-13 - Phase 6 Hive Skriaxit sandstorm

Status: Completed checkpoint; corrected Phase 6 binding reconciliation in progress

### Delivered

- Reconciled both `skriaxit_sandstorm` bindings on converted Hive mobiles 2043741 and
  2043742 through exact identity-owned profiles on the persistent `RoL Monster Combat`
  mobile procedure.
- Preserved the source three-round schedule while the mobile is idle, fighting, or disabled.
  Each eligible pulse reaches the current room and populated rooms through open north, east,
  south, west, up, and down exits; closed exits, invalid rooms, and peaceful rooms are skipped.
- Preserved mortal-player and player-pet targeting, incorporeal and air/earth elemental
  immunity, native area safety, spell resistance, a level-48 Will-save dispel attempt against
  each eligible spell affect, and removal of at most the first failed affect.
- Preserved the bound source's actual zero-damage behavior. Its room loop resets the counted
  Skriaxits before evaluating `3 * num`, so the target does not invent the scaling damage
  suggested by the source comments.
- Added exact binding-set, profile, cadence, source-damage, disabled-activity, converter,
  reconciler, and production-linked coverage. No player helpfile changed because the batch adds
  no command or syntax; the staff manual records the two profiles and their scheduled behavior.
- Regenerated the corrected Phase 6 evidence. Resolution increases from 1,392 to 1,394 static
  bindings and from 581 to 582 direct handlers, leaving 327 bindings across 213 handlers in 34
  source files. The independent `ACT_SPEC` cross-check remains 799 resolved and 49 pending.
- Reforecast the nine corrected batches, which closed 148 bindings across 44 handlers. The
  binding-count projection is about 20 additional batches and the handler-diversity projection
  about 44; the measured Phase 6 envelope is therefore 20-44 sessions, or 40-176 focused
  engineering hours. The full remaining project envelope is 76-128 sessions, or 152-512
  focused hours.

### Acceptance evidence

```text
Delivery commit: cb964e66
Reconciliation path: lib/rol-conversion/runs/phase6-special-20260813-hive-skriaxit-sandstorm
Reconciliation run: rol-phase6-special-337e1c4ccb66c3bc
Evidence tree SHA-256: a7c8ef2f7abfa6b8358ee8aaa1d2f2f33c96b6489785ac98a2f827caa0dbdcfe
Active direct bindings: 1,721
Direct bindings resolved: 1,394
Direct bindings pending: 327
Source handlers resolved: 582
Source handlers pending: 213
Additional handler families resolved: 1
Additional direct bindings resolved: 2
Native adapted bindings: 860
Native adapted composable bindings: 207
ACT_SPEC records resolved: 799
ACT_SPEC records pending: 49
Complete world-tool suite: 333 passed
Production-linked CuTest suite: 670 passed
Documentation findings: 0 errors, 0 warnings, 0 info
Warning-free Autotools build, test, and install: passed
Root-level circle artifact: absent
Installed ELF build ID: 98acdd1ad145d505b9a9d0c9ce94d6968c434a66
Installed SHA-256: d87d807bf5bd9f8a6234ace2e4ac1169a2a8c752f064c08432c686c2cafd0205
Evidence artifact hashes: 8 verified
Repeat reconciliation generation: byte-identical
Live target writes: 0
```

Phase 6 continues with another dependency-complete shared-runtime family. Reforecast after
another material batch or any inventory correction.

## 2026-08-13 - Phase 6 successful-hit area family

Status: Completed checkpoint; corrected Phase 6 binding reconciliation in progress

### Delivered

- Reconciled six successful-hit area handlers on converted mobiles 2021786, 2021820,
  2043705, 2096631, 2096670, and 2096672 through exact identity-owned profiles on the
  persistent `RoL Monster Combat` mobile procedure.
- Preserved Dobluth bladestorm's one-in-five weapon-damage sweep and banshee wail's one-in-four
  damage and stun path, the Hive sandstorm beast's one-in-16 damage and independent blindness
  path, the Greycloak banshee's one-in-six wail, Urgutha Forka's one-in-11 poison-fume attack,
  and Aralesh Tandar's one-in-11 opponent execution with its same-room pet-owner branch.
- Routed the source direct HP mutations through typed target damage, native saves, immunity and
  resistance handling, safe area targeting, blindness affects, and bounded stun. Bladestorm
  retains one aggregate weapon payload for every eligible target; successful saves cannot leak
  a reduced amount into later targets.
- Invalidated the remaining hit context whenever an area effect or execution removes the
  character struck by the outer attack, including Aralesh's pet-owner branch, so later critical
  and artifact riders cannot access an extracted target.
- Added exact binding-set, profile, roll-boundary, gateway, invalidation, registry, converter,
  reconciler, and production-linked behavioral coverage. No player helpfile changed because the
  batch adds no command or syntax; the staff manual now covers all six converted behaviors.
- Regenerated the corrected Phase 6 evidence. Resolution increases from 1,386 to 1,392 static
  bindings and from 575 to 581 direct handlers, leaving 329 bindings across 214 handlers in 34
  source files. The independent `ACT_SPEC` cross-check remains 799 resolved and 49 pending.
- Reforecast the eight corrected batches, which closed 146 bindings across 43 handlers. The
  binding-count projection is about 19 additional batches and the handler-diversity projection
  about 40; the measured Phase 6 envelope is therefore 20-40 sessions, or 40-160 focused
  engineering hours. The full remaining project envelope is 76-124 sessions, or 152-496
  focused hours.

### Acceptance evidence

```text
Delivery commit: 3c9f0afe
Reconciliation path: lib/rol-conversion/runs/phase6-special-20260813-successful-hit-area
Reconciliation run: rol-phase6-special-767041a4f1ffe023
Evidence tree SHA-256: 912ad77f9db500b0c4133a0293b2a3b0d9890337ae1fe379d7c2db7467b18c48
Active direct bindings: 1,721
Direct bindings resolved: 1,392
Direct bindings pending: 329
Source handlers resolved: 581
Source handlers pending: 214
Additional handler families resolved: 6
Additional direct bindings resolved: 6
Native adapted bindings: 858
Native adapted composable bindings: 207
ACT_SPEC records resolved: 799
ACT_SPEC records pending: 49
Complete world-tool suite: 332 passed
Production-linked CuTest suite: 669 passed
Documentation findings: 0 errors, 0 warnings, 0 info
Warning-free Autotools build, test, and install: passed
Root-level circle artifact: absent
Installed ELF build ID: 96079d64d19cdd877edc8d4e3c84e4b7899210a0
Installed SHA-256: 033a9239a4719a688b541286d39b6680b2930a248c5fde090de867d17e187a14
Evidence artifact hashes: 8 verified
Repeat reconciliation generation: byte-identical
Live target writes: 0
```

Phase 6 continues with another dependency-complete shared-runtime family. Reforecast after
another material batch or any inventory correction.

## 2026-08-13 - Phase 6 Hive manscorpion venom family

Status: Completed checkpoint; corrected Phase 6 binding reconciliation in progress

### Delivered

- Reconciled 16 Hive manscorpion bindings across `manscorpion_venom_light`,
  `manscorpion_venom_medium`, `manscorpion_venom_heavy`, and `manscorpion_king` through exact
  identity-owned profiles on the persistent `RoL Monster Combat` mobile procedure.
- Added the typed successful-mobile-hit gateway and invoked it only after a completed damaging
  hit. The gateway validates the attacker and randomly selected mortal room target, and it
  invalidates the remaining attack context if lethal king venom extracts that target.
- Preserved the source one-in-31, one-in-7, one-in-11, and one-in-25 trigger chances. Nonfatal
  venom uses target-native poison immunity and Fortitude saves, applies nonstacking -2
  Constitution for six, four, or two ticks, and king venom kills immediately unless RoL slow
  poison is active, in which case it follows the one-tick nonfatal path.
- Added exact binding-set, profile, affect, event-dispatch, gateway, registry, and
  production-linked behavioral coverage. No player helpfile changed because the batch adds no
  command or syntax; the staff manual now covers the converted Hive behaviors.
- Regenerated the corrected Phase 6 evidence. Resolution increases from 1,370 to 1,386 static
  bindings and from 571 to 575 direct handlers, leaving 335 bindings across 220 handlers in 34
  source files. The independent `ACT_SPEC` cross-check remains 799 resolved and 49 pending.
- Reforecast the seven corrected batches, which closed 140 bindings across 37 handlers. The
  binding-count projection is about 17 additional batches and the handler-diversity projection
  about 42; the measured Phase 6 envelope is therefore 18-42 sessions, or 36-168 focused
  engineering hours. The full remaining project envelope is 74-126 sessions, or 148-504
  focused hours.

### Acceptance evidence

```text
Delivery commit: 236296dd
Reconciliation path: lib/rol-conversion/runs/phase6-special-20260813-hive-manscorpion-venom
Reconciliation run: rol-phase6-special-6cfc16c58f7802e2
Evidence tree SHA-256: 3e600d5c5515e2e09ec64f882a98f2d647459466cbc7d44987381484319df8f1
Active direct bindings: 1,721
Direct bindings resolved: 1,386
Direct bindings pending: 335
Source handlers resolved: 575
Source handlers pending: 220
Additional handler families resolved: 4
Additional direct bindings resolved: 16
Native adapted bindings: 852
Native adapted composable bindings: 207
ACT_SPEC records resolved: 799
ACT_SPEC records pending: 49
Complete world-tool suite: 331 passed
Production-linked CuTest suite: 668 passed
Documentation findings: 0 errors, 0 warnings, 0 info
Warning-free Autotools build, test, and install: passed
Root-level circle artifact: absent
Installed ELF build ID: 7f003f9fda14f34566f8e5fcca02f0284bb16829
Installed SHA-256: 3ba54e3be21f60f0abed6a608e838ebe41e76f7ae7afce72685f9c127044e65d
Evidence artifact hashes: 8 verified
Repeat reconciliation generation: byte-identical
Live target writes: 0
```

Phase 6 continues with another dependency-complete shared-runtime family. Reforecast after
another material batch or any inventory correction.

## 2026-08-13 - Phase 6 Seelie faerie family

Status: Completed checkpoint; corrected Phase 6 binding reconciliation in progress

### Delivered

- Reconciled 38 Seelie Court bindings across `standard_faerie_prism`,
  `standard_faerie_ff`, and `faerie_search` on 18 converted mobiles. All use the persistent
  `RoL Monster Combat` mobile procedure with exact identity-owned capability profiles.
- Preserved the source prismatic cadence and beam multiplicity, eight distinct color outcomes,
  target-native saves and safety checks, faerie-fire cadence and cooldown, and hidden-target
  search/reveal/stun behavior. Seelie profiles retain their authored chance to act and recover
  while disabled without relaxing the disabled gate for unrelated mobiles.
- Added exact profile, binding-set, helper, gateway, and production-linked behavioral coverage.
  No player helpfile changed because the batch adds no command or syntax; the staff manual now
  covers the converted Seelie behaviors.
- Regenerated the corrected Phase 6 evidence. Resolution increases from 1,332 to 1,370 static
  bindings and from 568 to 571 direct handlers, leaving 351 bindings across 224 handlers in 34
  source files. The independent `ACT_SPEC` cross-check remains 799 resolved and 49 pending.
- Reforecast the six corrected batches, which closed 124 bindings across 33 handlers. The
  binding-count projection is about 17 additional batches and the handler-diversity projection
  about 41; the measured Phase 6 envelope is therefore 18-41 sessions, or 36-164 focused
  engineering hours. The full remaining project envelope is 74-125 sessions, or 148-500
  focused hours.

### Acceptance evidence

```text
Delivery commits: f945051b, f40a5e48
Reconciliation path: lib/rol-conversion/runs/phase6-special-20260813-seelie-faerie
Reconciliation run: rol-phase6-special-66858c11e3301a3e
Evidence tree SHA-256: c52763d7c464a3b18ac97026a9462e79d8b510a1c44cfc95b2757bf38033f2c8
Active direct bindings: 1,721
Direct bindings resolved: 1,370
Direct bindings pending: 351
Source handlers resolved: 571
Source handlers pending: 224
Additional handler families resolved: 3
Additional direct bindings resolved: 38
Native adapted bindings: 836
Native adapted composable bindings: 207
ACT_SPEC records resolved: 799
ACT_SPEC records pending: 49
Complete world-tool suite: 330 passed
Production-linked CuTest suite: 666 passed
Documentation findings: 0 errors, 0 warnings, 0 info
Warning-free Autotools build, test, and install: passed
Root-level circle artifact: absent
Installed ELF build ID: 7e5309c5e4da2c8daa72dd579d14fbf2ded461c4
Installed SHA-256: 30b3c1ce894fa329aa183d41c626076da859d16cea9161a75b70f5eb0e28a387
Evidence artifact hashes: 8 verified
Repeat reconciliation generation: byte-identical
Live target writes: 0
```

Phase 6 continues with another dependency-complete shared-runtime family. Reforecast after
another material batch or any inventory correction.

## 2026-08-13 - Phase 6 Darkhold elemental deaths

Status: Completed checkpoint; corrected Phase 6 binding reconciliation in progress

### Delivered

- Reconciled the `fire_die`, `air_die`, `water_die`, and `earth_die` callbacks on Darkhold
  source mobiles 94501-94504 through the existing composable death-profile runtime. The
  callbacks consume no persistent special-procedure slot and add no prototype flag.
- Preserved each authored crumbling message and mapped reward dependency: converted mobiles
  2094501-2094504 drop objects 2094508-2094511, respectively a ruby, diamond, aquamarine, and
  golden nugget. All four callbacks return to normal death handling, so ordinary corpses remain.
- Added exact transformation, handler-disposition, source-VNUM, message, reward-object, and
  corpse-policy regressions plus a production-linked end-to-end drop test for all four profiles.
  No player helpfile changed because the batch adds no command or syntax; the staff manual
  matrix records the death behavior.
- Regenerated the corrected Phase 6 evidence. Resolution increases from 1,328 to 1,332 static
  bindings and from 564 to 568 direct handlers, leaving 389 bindings across 227 handlers in 34
  source files. The independent `ACT_SPEC` cross-check remains 799 resolved and 49 pending.
- Reforecast the five corrected batches, which closed 86 bindings across 30 handlers. The
  binding-count projection is about 23 sessions and the handler-diversity projection about 38;
  the measured Phase 6 envelope is therefore 24-38 sessions, or 48-152 focused engineering
  hours. The full remaining project envelope is 80-122 sessions, or 160-488 focused hours.

### Acceptance evidence

```text
Delivery commit: 41ba7cea
Reconciliation path: lib/rol-conversion/runs/phase6-special-20260813-darkhold-elemental-deaths
Reconciliation run: rol-phase6-special-cc108f5e1415f677
Active direct bindings: 1,721
Direct bindings resolved: 1,332
Direct bindings pending: 389
Source handlers resolved: 568
Source handlers pending: 227
Additional handler families resolved: 4
Additional direct bindings resolved: 4
Native adapted composable bindings: 207
ACT_SPEC records resolved: 799
ACT_SPEC records pending: 49
Complete world-tool suite: 328 passed
Production-linked CuTest suite: 665 passed
Documentation findings: 0 errors, 0 warnings, 0 info
Warning-free Autotools build, test, and install: passed
Root-level circle artifact: absent
Installed ELF build ID: 5d7ea680d07384ff360d8d36354b42aaf9dd1fda
Installed SHA-256: d81c639065cf56913f175dc0ae46cf1cbfb7d488810d9bb4c26d6d0d24d0b4d0
Evidence artifact hashes: 8 verified
Repeat reconciliation generation: byte-identical
Live target writes: 0
```

Phase 6 continues with corrected batch six from a dependency-complete shared-runtime family.
Perform the scheduled forecast recalibration after that delivery.

## 2026-08-13 - Phase 6 planar mobile initializers

Status: Completed checkpoint; corrected Phase 6 binding reconciliation in progress

### Delivered

- Reconciled eight bindings across six planar initializer handlers. Bar-lgura gains the existing
  converted rogue-role action plus permanent hide; both Cambions gain that role plus permanent
  sneak; and Lemure and Nupperibo gain target-native charm immunity. These are prototype
  properties and consume no persistent special-procedure slot.
- Recorded three source-inert callbacks with code and prototype evidence. Alu-fiend regeneration
  is disabled by `#if 0`; Dretch only clears an absent wimpy action that automatic demon setup
  does not add; and Rutterkin registers no events and changes no state.
- Extended mobile conversion bindings with required affect bits. Fixed owner-level staging so
  multiple composable callbacks on one mobile union their action flags, affect flags, value
  references, and compatible persistent procedure instead of allowing the last callback to
  overwrite earlier requirements. Conflicting owners or procedure names now fail closed.
- Added exact transformation, emission, owner-composition, handler-disposition, source-VNUM, and
  production-ledger regression coverage. No player helpfile changed because the batch adds no
  command or syntax; the staff manual matrix covers prototype verification.
- Regenerated the corrected Phase 6 evidence. Resolution increases from 1,320 to 1,328 static
  bindings and from 558 to 564 direct handlers, leaving 393 bindings across 231 handlers in 34
  source files. The independent `ACT_SPEC` cross-check is now 799 resolved and 49 pending.
- Retained the measured 24-36-session Phase 6 envelope. The first four corrected batches closed
  82 bindings across 26 handlers, which projected approximately 19 binding-count sessions and
  36 handler-diversity sessions from the then-current remaining inventory.

### Acceptance evidence

```text
Delivery commit: 66c3d7a1
Reconciliation path: lib/rol-conversion/runs/phase6-special-20260813-planar-initializers
Reconciliation run: rol-phase6-special-49b79534c90b09d7
Active direct bindings: 1,721
Direct bindings resolved: 1,328
Direct bindings pending: 393
Source handlers resolved: 564
Source handlers pending: 231
Additional handler families resolved: 6
Additional direct bindings resolved: 8
Native adapted composable bindings: 203
Source-inert excluded bindings: 29
ACT_SPEC records resolved: 799
ACT_SPEC records pending: 49
Complete world-tool suite: 326 passed
Production-linked CuTest suite: 664 passed
Documentation findings: 0 errors, 0 warnings, 0 info
Warning-free Autotools build, test, and install: passed
Root-level circle artifact: absent
Installed ELF build ID: 8f263159cbe2b8bbf676a2634c7b770acfe51a27
Installed SHA-256: e99271feb0b795f94ba24a3e0b29b43adb439fd4f9fb8f27a98f470950068bd2
Evidence artifact hashes: 8 verified
Repeat reconciliation generation: byte-identical
Live target writes: 0
```

Phase 6 continues with a dependency-complete planar combat or death family and its
Undermountain aliases. Reforecast after corrected batch six or a material inventory change.

## 2026-08-13 - Phase 6 planar demon base behavior

Status: Completed checkpoint; corrected Phase 6 binding reconciliation in progress

### Delivered

- Reconciled all 25 active `abyssForgedWeapons` bindings. The converter marks only those
  prototypes with `MOB_ROL_ABYSS_FORGED`; their primary, off-hand, and two-handed wield slots
  dissolve before either special-death dispatch or ordinary corpse creation.
- Reconciled all eight directly authored `standardDemon` bindings. Each source mobile is already
  race X and therefore receives the complete composition-safe `MOB_ROL_DEMON` runtime through
  automatic race conversion; no duplicate persistent procedure or `MOB_SPEC` ownership is added.
- Added production-linked coverage for all three wield slots, unmarked-mobile exclusion,
  idempotence, and pre-corpse extraction. Added exact converter and reconciliation invariants for
  the 25 abyss-forged VNUMs and eight directly authored demon VNUMs.
- Added builder documentation for the conversion-only flag and a staff manual matrix for the
  death behavior. No player helpfile changed because this batch adds no command or syntax.
- Regenerated the corrected Phase 6 evidence. Resolution increases from 1,287 to 1,320 static
  bindings and from 556 to 558 direct handlers, leaving 401 bindings across 237 handlers in 34
  source files. The independent `ACT_SPEC` cross-check remains 798 resolved and 50 pending.
- Reforecast the remaining Phase 6 work from the first three corrected batches. They closed 74
  bindings across 20 handlers; the measured envelope is now 24-36 sessions, or 48-144 focused
  engineering hours. This replaces the provisional 18-30-session range and recognizes that 185
  of the remaining handlers are singletons.

### Acceptance evidence

```text
Delivery commit: c90ced37
Reconciliation path: lib/rol-conversion/runs/phase6-special-20260813-planar-base
Reconciliation run: rol-phase6-special-55c1c510a1bc029d
Active direct bindings: 1,721
Direct bindings resolved: 1,320
Direct bindings pending: 401
Source handlers resolved: 558
Source handlers pending: 237
Additional handler families resolved: 2
Additional direct bindings resolved: 33
Native adapted bindings: 798
Native adapted composable bindings: 198
ACT_SPEC records resolved: 798
ACT_SPEC records pending: 50
Complete world-tool suite: 323 passed
Production-linked CuTest suite: 664 passed
Documentation findings: 0 errors, 0 warnings, 0 info
Warning-free Autotools build, test, and install: passed
Root-level circle artifact: absent
Installed ELF build ID: 9717d20af0483684f7ca51ea93ba49390637b8ff
Installed SHA-256: 46d5ae2f8f717a89447ab0835d32705932d899ffc1471a3cf7bb4c43f9d33203
Evidence artifact hashes: 8 verified
Repeat reconciliation generation: byte-identical
Live target writes: 0
```

Phase 6 continues with the remaining planar-specific handlers and behavior-identical cross-zone
aliases. Reforecast again after another three corrected batches or a material inventory change.

## 2026-08-13 - Phase 6 exact-class guild family

Status: Completed checkpoint; corrected Phase 6 binding reconciliation in progress

### Delivered

- Reconciled all 37 active room bindings across the 14 exact-class source guild callbacks.
  Each source wrapper delegates to the same source training engine, so the conversion reuses the
  already production-tested target mage, thief, cleric, and warrior family room adapters.
- Mapped Conjurer, Elementalist, and Necromancer to the mage family; Thief and Assassin to the
  thief family; Cleric, Druid, and Shaman to the cleric family; and Warrior, Antipaladin,
  Mercenary, Monk, Paladin, and Ranger to the warrior family. This retains the authored role
  boundary while honoring the target multiclass model.
- Added exact transformation and reconciliation coverage for every callback. Room bindings add
  no prototype flag, no new persistent procedure identity, and no new runtime code.
- Expanded the manual guild matrix with all 37 converted destination rooms and their target
  family expectations. No player helpfile changed because the existing `practice`, `train`, and
  `boosts` commands and responses are unchanged.
- Regenerated the corrected Phase 6 evidence. Resolution increases from 1,250 to 1,287 static
  bindings and from 542 to 556 direct handlers, leaving 434 bindings across 239 handlers in 34
  source files. The independent `ACT_SPEC` cross-check remains 798 resolved and 50 pending.
- Retained the 18-30-session Phase 6 envelope. This is the first regular corrected-denominator
  family sample; reforecast follows corrected batch three.

### Acceptance evidence

```text
Delivery commit: b0e924b8
Reconciliation path: lib/rol-conversion/runs/phase6-special-20260813-exact-class-guilds
Reconciliation run: rol-phase6-special-be53e38737ea4fc8
Active direct bindings: 1,721
Direct bindings resolved: 1,287
Direct bindings pending: 434
Source handlers resolved: 556
Source handlers pending: 239
Additional handler families resolved: 14
Additional direct bindings resolved: 37
Native adapted bindings: 798
Native adapted composable bindings: 165
ACT_SPEC records resolved: 798
ACT_SPEC records pending: 50
Complete world-tool suite: 321 passed
Production-linked CuTest suite: 663 passed
Documentation findings: 0 errors, 0 warnings, 0 info
Warning-free Autotools build, test, and install: passed
Root-level circle artifact: absent
Installed ELF build ID: cd432fd38de79f5d39d8ca25b8426bf99d91a418
Installed SHA-256: 19c9f96768f9cce85e9129f3ccf5ce1c57a87ea14a09d4cf500da25d314f09de
Evidence artifact hashes: 7 verified
Repeat reconciliation generation: byte-identical
Live target writes: 0
```

Phase 6 continues with corrected batch three from a source-local or behavior-shared family. The
estimate will then be recalibrated from the first three corrected-denominator samples.

## 2026-08-13 - Phase 6 Tarrasque encounter

Status: Completed checkpoint; corrected Phase 6 binding reconciliation in progress

### Delivered

- Added the typed, builder-visible `RoL Tarrasque Encounter` procedure for source mobile 2601
  and objects 2604 and 2610. The encounter preserves periodic healing, pet execution, ordered
  swallow/tail-fling/tail-sweep combat, stomach acid, casting and preparation interruption,
  corpse entry, special death loot, and the return portal.
- Added a flow-bearing typed mobile-death event and invoked it from NPC death processing only
  when a registered procedure advertises the event. A successful handler suppresses the
  ordinary corpse and extracts the mobile after its replacement death behavior completes.
- Preserved the source weighted 6/6/6/2 loot distribution and adapted the source portal's old
  single-value destination to the target's normal portal schema. The return portal records the
  death-room VNUM in both target destination fields.
- Routed encounter damage, saves, acid resistance, stun eligibility, and random relocation
  through target-native safety contracts. The source random-teleport meaning is implemented by
  bounded valid-destination selection because the target `teleport` spell has different
  semantics.
- Added exact converter dispositions for `tarrasque_swallow_smack`, `tarrasque_die`,
  `tarrasque_stomache`, and `tarrasque_corpse_enter`. The converter supplies `MOB_SPEC` to the
  mobile and `ITEM_AUTOPROC` only to the stomach-acid object.
- Added registry, persistence, owner-aware OLC, event-contract, dispatch, loot-weight,
  corpse-alias, transformation, and reconciliation coverage. The production-linked suite now
  contains 663 tests and the world-tool suite contains 319 tests.
- Regenerated the corrected Phase 6 evidence. Resolution increases from 1,246 to 1,250 static
  bindings and from 538 to 542 direct handlers, leaving 471 bindings across 253 handlers in 35
  source files. The independent `ACT_SPEC` cross-check remains 798 resolved and 50 pending.
- Retained the 18-30-session Phase 6 envelope. This encounter-specific four-handler closure is
  the first corrected-denominator throughput sample; reforecast waits for at least three
  corrected batches.

### Acceptance evidence

```text
Delivery commit: bbdf893a
Reconciliation path: lib/rol-conversion/runs/phase6-special-20260813-tarrasque
Reconciliation run: rol-phase6-special-de980a28a3be846e
Active direct bindings: 1,721
Direct bindings resolved: 1,250
Direct bindings pending: 471
Source handlers resolved: 542
Source handlers pending: 253
Additional handler families resolved: 4
Additional direct bindings resolved: 4
Native adapted bindings: 761
Native adapted composable bindings: 165
ACT_SPEC records resolved: 798
ACT_SPEC records pending: 50
Complete world-tool suite: 319 passed
Production-linked CuTest suite: 663 passed
Documentation findings: 0 errors, 0 warnings, 0 info
Warning-free Autotools build, test, and install: passed
Root-level circle artifact: absent
Installed ELF build ID: 1735ea1cadc25a1776aa09ec56f0ef3c6afde2e6
Installed SHA-256: e303a922907e55964b8cde3d596fccff509d6f9cdf809c6224c4c31377f6c89e
Evidence artifact hashes: 7 verified
Repeat reconciliation generation: byte-identical
Live target writes: 0
```

Phase 6 continues with a second source-local or behavior-shared batch from the remaining
corrected inventory. The estimate will be recalibrated after at least three such batches.

## 2026-08-13 - Phase 6 special discovery repair

Status: Completed checkpoint; corrected Phase 6 binding reconciliation in progress

### Delivered

- Replaced the direct-file-only special extractor with active boot call-path traversal through
  all 53 reachable registration wrappers. Every emitted row retains its wrapper path, source
  line, original VNUM token, and literal or preprocessor resolution evidence.
- Resolved all 38 active numeric planar macros automatically and added regression fixtures for
  direct, wrapped, symbolic, preprocessor-excluded, and dynamic registrations.
- Added explicit dynamic ledgers for `assign_the_questers()` and
  `assign_the_shopkeepers()`. Their 5,078 quest and 453 shop binding instances are resolved
  through the target data-driven HLQuest and shop services rather than misclassified as unknown
  numeric bindings.
- Regenerated the dependent Phase 1, Phase 2, Phase 5, and Phase 6 ignored evidence chain. The
  corrected static denominator is 1,721 live bindings across 795 direct handlers; 1,246 bindings
  and 538 handlers are resolved, leaving 475 bindings across 257 handlers.
- Recomputed the independent `ACT_SPEC` cross-check to 798 resolved and 50 pending records.
  Automatic race reconciliation remains complete for all 247 implicit bindings.
- Reforecast the remaining Phase 6 work at 18-30 sessions from the corrected pending inventory,
  its 36 source files, 190 singleton handlers, and 67 multi-binding handlers. The previous
  1-3-session estimate is retired.

### Acceptance evidence

```text
Delivery commit: c2a677a8
Phase 1 path: lib/rol-conversion/runs/phase1-policy2-20260813-special-discovery
Phase 1 run: rol-phase1-237602d3ade48138
Phase 2 path: lib/rol-conversion/runs/phase2-policy2-20260813-special-discovery
Phase 2 run: rol-phase2-c93b8c4610b36d1e
Phase 5 path: lib/rol-conversion/runs/phase5-policy2-20260813-special-discovery-audit
Phase 5 run: rol-phase5-audit-cec58661a4f21a2a
Phase 6 path: lib/rol-conversion/runs/phase6-special-20260813-discovery-repair
Phase 6 run: rol-phase6-special-df585be75f0574e3
Static binding candidates: 1,813
Preprocessor-excluded bindings: 92
Active static bindings: 1,721
Static bindings resolved: 1,246
Static bindings pending: 475
Direct source handlers: 795
Direct source handlers resolved: 538
Direct source handlers pending: 257
Dynamic registration paths: 2
Active dynamic binding instances: 5,531
Total active binding instances: 7,252
Total source handlers: 797
ACT_SPEC records resolved: 798
ACT_SPEC records pending: 50
Complete world-tool suite: 317 passed
Production-linked CuTest suite: 661 passed
Documentation findings: 0 errors, 0 warnings, 0 info
Warning-free Autotools build, test, and install: passed
Root-level circle artifact: absent
Installed ELF build ID: 232dd588da93e455de32b496c2ed5a92efa08951
Installed SHA-256: 59f8510f6e1379df99e6e4fef44745397bdc735f8a512c6bed7c07a3bc5c4a3f
Evidence artifact hashes: 30 verified
Repeat Phase 6 generation: byte-identical
Live target writes: 0
```

Phase 6 continues with the dependency-complete Tarrasque encounter, then source-local and
behavior-shared batches from the corrected 475-binding pending inventory.

## 2026-08-12 - Phase 6 Waterdeep town crier

Status: Completed checkpoint; Phase 6 direct-binding reconciliation in progress

### Delivered

- Converted the active `crier_one` binding for northern Waterdeep mobile 2003008 through the
  existing builder-visible `RoL Scheduled Mobile` procedure and enforced `MOB_SPEC`.
- Preserved the standing-gated 2d42 ambient distribution, all 41 authored cases, ordered speech
  and room actions, zone shouts, and the deliberately silent 43-84 half of the roll range.
- Preserved both shared once-per-hour state gates and their source reset order: Moonshae and
  Calimport ship warnings, the hour-5 shop-opening warning, and the source-normal suppression of
  the hour-18 shop-closing warning until a reset or fresh load permits it.
- Preserved the source combat help shout and outdoor-only city cheering, plus the outdoor-only
  housewife response after the two welcome shouts. Target zone and indoor-room metadata replace
  the source descriptor filter without broadening the audience.
- Added converter, required-flag, schedule-boundary, exact-disposition, persistence, registry,
  OLC, plan, and manual-test coverage without adding a second named procedure.
- Reconciliation now resolves 1,112 of 1,147 active direct bindings and 538 of 562 source
  handlers; 35 bindings and 24 handlers remain. The independent `ACT_SPEC` checkpoint resolves
  830 records and leaves 18 pending.
- Archived the sixty-second Phase 6 delivery session. The remaining Phase 6 envelope remains 1-3
  sessions.

### Acceptance evidence

```text
Delivery commit: 6c64fba1
Reconciliation path: lib/rol-conversion/runs/phase6-special-20260812-waterdeep-crier
Reconciliation run: rol-phase6-special-e0e90cdd3f12895e
Active direct bindings: 1,147
Direct bindings resolved: 1,112
Direct bindings pending: 35
Source handlers resolved: 538
Source handlers pending: 24
Additional handler families resolved: 1
Additional direct bindings resolved: 1
Native adapted bindings: 654
Native adapted composable bindings: 159
Source-inert excluded bindings: 26
Source-unsafe excluded bindings: 18
ACT_SPEC records resolved: 830
ACT_SPEC records pending: 18
Special registry definitions: 111 total / 98 legacy / 13 typed
Compatibility names: 112
Complete world-tool suite: 316 passed
Production-linked CuTest suite: 661 passed
Documentation findings: 0 errors, 0 warnings, 0 info
Warning-free Autotools build, test, and install: passed
Root-level circle artifact: absent
Installed ELF build ID: cbffe163e2dd9066d30e31e862938eecc9cc3438
Installed SHA-256: e8782120146413ce2153376982b1c0b31995a7e247bde0a310be08677b373bf7
Evidence manifest hashes: verified
Live target writes: 0
```

Phase 6 continues with the remaining 35 direct bindings across 24 source handlers. Continue
using dependency-complete batches and reserve the full build/test/install gate for published
checkpoints.

## 2026-08-12 - Phase 6 lich rite

Status: Completed checkpoint; Phase 6 direct-binding reconciliation in progress

### Delivered

- Converted both active `lichConverter` mobile bindings through the new builder-visible
  `RoL Lich Rite` procedure and enforced the required `MOB_SPEC` flag.
- Preserved the exact case-sensitive `say immortality` trigger, maximum-mortal level gate,
  Necromancer admission, complete source narrative, both keeper-held converted offerings, and
  consumption of the offerings and keeper on success.
- Repaired the source equipped-offering defect by retaining concrete pointers for worn or carried
  offerings 2089471 and 2046999 and validating both before either is consumed.
- Applied the target's established safety contract for irreversible race conversion: the player
  must be ungrouped, neither following nor leading, then becomes the target Lich race and is rebuilt
  as a Wizard with zero experience and -1000 alignment through the current respec engine.
- Added registry, compatibility, OLC inventory, converter flag, exact-phrase, eligibility,
  offering-location, and reconciliation coverage plus builder and manual-test documentation.
- Reconciliation now resolves 1,111 of 1,147 active direct bindings and 537 of 562 source
  handlers; 36 bindings and 25 handlers remain. The independent `ACT_SPEC` checkpoint resolves
  829 records and leaves 19 pending.
- Archived the sixty-first Phase 6 delivery session. The remaining Phase 6 envelope remains 1-3
  sessions.

### Acceptance evidence

```text
Delivery commit: 7d28382f
Reconciliation path: lib/rol-conversion/runs/phase6-special-20260812-lich-rite
Reconciliation run: rol-phase6-special-ded69599851e733e
Active direct bindings: 1,147
Direct bindings resolved: 1,111
Direct bindings pending: 36
Source handlers resolved: 537
Source handlers pending: 25
Additional handler families resolved: 1
Additional direct bindings resolved: 2
Native adapted bindings: 653
Native adapted composable bindings: 159
Source-inert excluded bindings: 26
Source-unsafe excluded bindings: 18
ACT_SPEC records resolved: 829
ACT_SPEC records pending: 19
Special registry definitions: 111 total / 98 legacy / 13 typed
Compatibility names: 112
Complete world-tool suite: 316 passed
Production-linked CuTest suite: 661 passed
Documentation findings: 0 errors, 0 warnings, 0 info
Warning-free Autotools build, test, and install: passed
Root-level circle artifact: absent
Installed ELF build ID: 5c2cdc496d9dbd52d6b2be77ec1bea2294c7ed92
Installed SHA-256: 9fcff4b60681d2c4423b689aa8807c736ec9d9287741deb39e26314248e24ae2
Evidence manifest hashes: verified
Live target writes: 0
```

Phase 6 continues with the remaining 36 direct bindings across 25 source handlers. Continue
using dependency-complete batches and reserve the full build/test/install gate for published
checkpoints.

## 2026-08-12 - Phase 6 lost totem restorer

Status: Completed checkpoint; Phase 6 direct-binding reconciliation in progress

### Delivered

- Converted the active `lostTotemRestorer` binding through the new builder-visible
  `RoL Totem Restorer` mobile procedure in the existing converted-totem subsystem.
- Preserved the exact `say spiritworld` trigger, mapped source Shaman progression to the
  established target Cleric progression, retained the level-21 and saved-spirit-choice gates,
  and converted the source 1,000-platinum threshold to 10,000 target gold.
- Recreates the exact good or evil totem selected by persistent `GET_ROL_TOTEM_CHOICE`, binds
  it to the requesting character, and consumes the paid helper only after the object prototype
  has been validated and loaded. Invalid choices and missing prototypes fail without consuming
  the helper.
- Added converter `MOB_SPEC` enforcement, registry and compatibility coverage, eligibility and
  phrase regressions, reconciliation expectations, and manual testing instructions.
- Reconciliation now resolves 1,109 of 1,147 active direct bindings and 536 of 562 source
  handlers; 38 bindings and 26 handlers remain. The independent `ACT_SPEC` checkpoint remains
  at 828 resolved records and 20 pending because this direct binding has no source `ACT_SPEC`
  record.
- Archived the sixtieth Phase 6 delivery session. The remaining Phase 6 envelope remains 1-3
  sessions.

### Acceptance evidence

```text
Delivery commit: 3f773d78
Reconciliation path: lib/rol-conversion/runs/phase6-special-20260812-totem-restorer
Reconciliation run: rol-phase6-special-9139221a800d60a0
Active direct bindings: 1,147
Direct bindings resolved: 1,109
Direct bindings pending: 38
Source handlers resolved: 536
Source handlers pending: 26
Additional handler families resolved: 1
Additional direct bindings resolved: 1
Native adapted bindings: 651
Native adapted composable bindings: 159
Source-inert excluded bindings: 26
Source-unsafe excluded bindings: 18
ACT_SPEC records resolved: 828
ACT_SPEC records pending: 20
Special registry definitions: 110 total / 97 legacy / 13 typed
Compatibility names: 111
Complete world-tool suite: 315 passed
Production-linked CuTest suite: 660 passed
Documentation findings: 0 errors, 0 warnings, 0 info
Warning-free Autotools build, test, and install: passed
Root-level circle artifact: absent
Installed ELF build ID: 9e49f4696e6076b93d55806cc3ef2172379f6552
Installed SHA-256: b437baa2005d7652d34f0703fd3d888f4907063475c0eff0b7fe78bbc60b0913
Evidence manifest hashes: verified
Live target writes: 0
```

Phase 6 continues with the remaining 38 direct bindings across 26 source handlers. Continue
using dependency-complete batches and reserve the full build/test/install gate for published
checkpoints.

## 2026-08-12 - Phase 6 Menden fisherman

Status: Completed checkpoint; Phase 6 direct-binding reconciliation in progress

### Delivered

- Converted `menden_fisherman` through the existing source-hashed `RoL Source Periodic`
  gateway. No registry definition or storage schema changed.
- Preserved the awake gate, absence of fighting suppression, `number(1, 80)` selection,
  21 active outcomes, 40 generated actions, source message order, and exact room-visible text.
- Extended the periodic source parser, generated profile schema, and runtime to preserve targeted
  source socials. Named targets retain distinct room and victim messages, and `me` resolves to the
  acting mobile; the fisherman exercises wench, magus, and self targets across five actions.
- Preserved the source social-table boundary: `CMD_SIP` has no source action record and therefore
  contributes no room-visible action. Required `MOB_SPEC` activity is now enforced for persisted
  source-periodic bindings.
- Added generator, reconciliation, and production-linked profile regressions. The checked-in
  source-periodic and state-periodic tables remain reproducible from the assessed source tree.
- Reconciliation now resolves 1,108 of 1,147 active direct bindings and 535 of 562 source handlers;
  39 bindings and 27 handlers remain. The independent `ACT_SPEC` checkpoint resolves 828 records
  and leaves 20 pending.
- Archived the fifty-ninth Phase 6 delivery session. The remaining Phase 6 envelope remains 1-3
  sessions, leaving the Phases 6-8 forecast at 49-79 sessions, or 98-316 focused engineering hours.

### Acceptance evidence

```text
Delivery commit: 33965efc
Reconciliation path: lib/rol-conversion/runs/phase6-special-20260812-menden-fisherman
Reconciliation run: rol-phase6-special-aab827a742a51ca2
Active direct bindings: 1,147
Direct bindings resolved: 1,108
Direct bindings pending: 39
Source handlers resolved: 535
Source handlers pending: 27
Additional handler families resolved: 1
Additional direct bindings resolved: 1
Native adapted bindings: 650
Native adapted composable bindings: 159
Source-inert excluded bindings: 26
Source-unsafe excluded bindings: 18
ACT_SPEC records resolved: 828
ACT_SPEC records pending: 20
Special registry definitions: 109 total / 96 legacy / 13 typed
Compatibility names: 110
Complete world-tool suite: 315 passed
Production-linked CuTest suite: 660 passed
Documentation findings: 0 errors, 0 warnings, 0 info
Warning-free Autotools build, test, and install: passed
Root-level circle artifact: absent
Installed ELF build ID: 271844a0d4ad97d0293cd1d6cd20668485efd214
Installed SHA-256: fb560d3b5560bdce62c81b4eb9004872a821d832161955c2adfd08240b341f68
Evidence manifest hashes: verified
Live target writes: 0
```

Phase 6 continues with the remaining 39 direct bindings across 27 source handlers. Continue
grouping compatible irregular mechanics behind shared gateways, and preserve the full
build/test/install gate at each substantial commit boundary.

## 2026-08-12 - Phase 6 scheduled-mobile batch

Status: Completed checkpoint; Phase 6 direct-binding reconciliation in progress

### Delivered

- Converted four scheduled-mobile handlers through one shared, builder-visible legacy `RoL
  Scheduled Mobile` gateway. No storage schema changed.
- Preserved the Waterdeep and Gloomhaven gate guards' distinct opening, repair, and closing
  windows, gate-state correction, speeches, glare, and ambient tables. The inactive 19-21 hour
  interval is retained because neither source handler performs a corrective transition there.
- Preserved the lighthouse keeper's shared counter and the source hour-eight reset behavior,
  including its repeated first line during that hour and its later staged announcements.
- Preserved the naval combatant's source standing-before-fighting branch order and idle table.
  Its reachable defensive helper maps to the source helper's actual stoneskin operation; the
  post-loop disarm remains excluded because the source victim loop makes it unreachable.
- Added converter dispositions, exact reconciliation expectations, required mobile activity bits,
  registry coverage, public behavior helpers, and production-linked regressions.
- Reconciliation now resolves 1,107 of 1,147 active direct bindings and 534 of 562 source handlers;
  40 bindings and 28 handlers remain. The independent `ACT_SPEC` checkpoint resolves 827 records
  and leaves 21 pending.
- Archived the fifty-eighth Phase 6 delivery session. The remaining Phase 6 envelope remains 1-3
  sessions, leaving the Phases 6-8 forecast at 49-79 sessions, or 98-316 focused engineering hours.

### Acceptance evidence

```text
Delivery commit: c8704d86
Reconciliation path: lib/rol-conversion/runs/phase6-special-20260812-scheduled-mobiles
Reconciliation run: rol-phase6-special-c447dd6b4665cb7a
Active direct bindings: 1,147
Direct bindings resolved: 1,107
Direct bindings pending: 40
Source handlers resolved: 534
Source handlers pending: 28
Additional handler families resolved: 4
Additional direct bindings resolved: 4
Native adapted bindings: 649
Native adapted composable bindings: 159
Source-inert excluded bindings: 26
Source-unsafe excluded bindings: 18
ACT_SPEC records resolved: 827
ACT_SPEC records pending: 21
Special registry definitions: 109 total / 96 legacy / 13 typed
Compatibility names: 110
Complete world-tool suite: 314 passed
Production-linked CuTest suite: 660 passed
Documentation findings: 0 errors, 0 warnings, 0 info
Warning-free Autotools build, test, and install: passed
Root-level circle artifact: absent
Evidence manifest hashes: verified
Live target writes: 0
```

Phase 6 continues with the remaining 40 direct bindings across 28 source handlers. Continue
grouping compatible irregular mechanics behind shared gateways, and preserve the full
build/test/install gate at each substantial commit boundary.

## 2026-08-12 - Phase 6 utility-service batch

Status: Completed checkpoint; Phase 6 direct-binding reconciliation in progress

### Delivered

- Converted four utility-service handlers through typed `RoL Utility Object` and `RoL Utility
  Room` gateways. The room gateway adds one builder-visible registry and persistence name; the
  object gateway remains shared.
- Preserved the Black Plague reservoir's level gate, exact room ownership, drink/fill exposure,
  and disease immunity through target-native contagion. The source global plague toggle has no
  target equivalent and is recorded as an explicit compatibility boundary.
- Preserved the loot blocker's aggressive-NPC interception for room containers and non-player
  corpses while allowing carried containers and player corpses. Its exact 120-second corpse sweep
  maps the source 60-second decay request to one target MUD tick without adding persistent state.
- Preserved newbie-room east routing by converted source-race alignment and mapped the unavailable
  source birthplace model to the target saved load room with the mortal start as fallback.
- Preserved the weight trigger's 5,000-unit threshold, immortal exemption, transition state, and
  source messages. The source callback's door-effect branch is itself unimplemented.
- Added converter dispositions, exact reconciliation expectations, registry and profile coverage,
  item identification text, and production-linked behavior regressions.
- Reconciliation now resolves 1,103 of 1,147 active direct bindings and 530 of 562 source handlers;
  44 bindings and 32 handlers remain. The independent `ACT_SPEC` checkpoint remains 824 resolved /
  24 pending.
- Archived the fifty-seventh Phase 6 delivery session. The remaining Phase 6 envelope remains 1-3
  sessions, leaving the Phases 6-8 forecast at 49-79 sessions, or 98-316 focused engineering hours.

### Acceptance evidence

```text
Delivery commit: 9d40694b
Reconciliation path: lib/rol-conversion/runs/phase6-special-20260812-utility-services
Reconciliation run: rol-phase6-special-2c12ac866ad07db2
Active direct bindings: 1,147
Direct bindings resolved: 1,103
Direct bindings pending: 44
Source handlers resolved: 530
Source handlers pending: 32
Additional handler families resolved: 4
Additional direct bindings resolved: 4
Native adapted bindings: 645
Native adapted composable bindings: 159
Source-inert excluded bindings: 26
Source-unsafe excluded bindings: 18
ACT_SPEC records resolved: 824
ACT_SPEC records pending: 24
Special registry definitions: 108 total / 95 legacy / 13 typed
Compatibility names: 109
Complete world-tool suite: 312 passed
Production-linked CuTest suite: 659 passed
Documentation findings: 0 errors, 0 warnings, 0 info
Warning-free Autotools build, test, and install: passed
Root-level circle artifact: absent
Evidence manifest hashes: verified
Live target writes: 0
```

Phase 6 continues with the remaining 44 direct bindings across 32 source handlers. Continue
grouping compatible irregular mechanics behind shared typed gateways, and preserve the full
build/test/install gate at each substantial commit boundary.

## 2026-08-12 - Phase 6 object-service batch

Status: Completed checkpoint; Phase 6 direct-binding reconciliation in progress

### Delivered

- Converted five object-service handlers through the existing typed `RoL Utility Object` and
  `RoL Weapon Proc` gateways. No registry definition, persisted procedure name, or storage schema
  was added.
- Preserved Lathander's disc renewal, consumption, sleep, and stun behavior behind a new target
  `rub` gateway command. Preserved Llym's held-treasure offering, valuation, consumption, blessing,
  gold, summon, and object-reward paths with all Phase 2 identities verified by source kind.
- Preserved the smoke shield's one-in-ten punch stun and block discharge, including the source
  nonlethal guard and target-safe invalidation. Preserved the Crescent Moon's exact-case invocation
  and invisibility, and the Hellish Fury bow's ranged fire message and one-in-26 heavy fire proc.
- Mapped the source bow `FIREWEAPON` callback to the available successful ranged-hit gateway, the
  Crescent Moon's pulse object recharge to one actor combat-round wait, source vitality to target
  aid, and source coin varieties to the target's unified gold field.
- Classified `nuclear_bomb` as source-inert: its assigned initializer returns no event bits, so the
  destructive missile-hit body is unreachable through the active binding.
- Added converter dispositions, exact reconciliation expectations, utility registry-event coverage,
  object and weapon profile coverage, identify text, and the target command boundary.
- Reconciliation now resolves 1,099 of 1,147 active direct bindings and 526 of 562 source handlers;
  48 bindings and 36 handlers remain. The independent `ACT_SPEC` checkpoint remains 824 resolved /
  24 pending.
- Archived the fifty-sixth Phase 6 delivery session. The remaining Phase 6 envelope remains 1-3
  sessions, leaving the Phases 6-8 forecast at 49-79 sessions, or 98-316 focused engineering hours.

### Acceptance evidence

```text
Delivery commit: 1849d9ad
Reconciliation path: lib/rol-conversion/runs/phase6-special-20260812-object-services
Reconciliation run: rol-phase6-special-e50685fc20cfaf75
Active direct bindings: 1,147
Direct bindings resolved: 1,099
Direct bindings pending: 48
Source handlers resolved: 526
Source handlers pending: 36
Additional handler families resolved: 6
Additional direct bindings resolved: 6
Native adapted bindings: 641
Native adapted composable bindings: 159
Source-inert excluded bindings: 26
Source-unsafe excluded bindings: 18
ACT_SPEC records resolved: 824
ACT_SPEC records pending: 24
Special registry definitions: 107 total / 95 legacy / 12 typed
Compatibility names: 108
Complete world-tool suite: 311 passed
Production-linked CuTest suite: 658 passed
Documentation findings: 0 errors, 0 warnings, 0 info
Warning-free Autotools build, test, and install: passed
Root-level circle artifact: absent
Evidence manifest hashes: verified
Live target writes: 0
```

Phase 6 continues with the remaining 48 direct bindings across 36 source handlers. Continue
grouping compatible irregular mechanics behind shared typed gateways, and preserve the full
build/test/install gate at each substantial commit boundary.

## 2026-08-12 - Phase 6 called-effect object batch

Status: Completed checkpoint; Phase 6 direct-binding reconciliation in progress

### Delivered

- Converted eight called-effect object handlers through the existing typed, object-owned
  `RoL Utility Object` gateway. Identity profiles keep all behavior behind the single existing
  registry and persistence name.
- Preserved exact, case-sensitive source phrases; worn-item validation; 24-, 48-, and 72-hour
  instance cooldowns; target-native spell effects; combat target validation; and the Staff of
  Magius `shirak` and `dulak` light toggle.
- Preserved the basilisk legging stoneskin, charmed basilisk-snake summon, Dragon Cult elemental
  protection, Earthmother random elemental aid, Tyr favor, Ashentoris combat aid, and haste
  sleeves. Summon ownership is complete before load triggers that may extract the mobile.
- Extended item identification for this gateway so each converted item describes its invocation
  and cooldown. No registry definition, persisted procedure name, or storage schema was added.
- Added converter dispositions, exact reconciliation expectations, profile coverage, registry
  event-contract coverage, and a production-linked light-toggle behavior regression.
- Reconciliation now resolves 1,093 of 1,147 active direct bindings and 520 of 562 source
  handlers; 54 bindings and 42 handlers remain. The independent `ACT_SPEC` checkpoint remains
  824 resolved / 24 pending.
- Archived the fifty-fifth Phase 6 delivery session. The remaining Phase 6 envelope remains
  1-3 sessions, leaving the Phases 6-8 forecast at 49-79 sessions, or 98-316 focused engineering
  hours.

### Acceptance evidence

```text
Delivery commit: c334a648
Reconciliation path: lib/rol-conversion/runs/phase6-special-20260812-called-objects
Reconciliation run: rol-phase6-special-abe9fabc332abee0
Active direct bindings: 1,147
Direct bindings resolved: 1,093
Direct bindings pending: 54
Source handlers resolved: 520
Source handlers pending: 42
Additional handler families resolved: 8
Additional direct bindings resolved: 8
Native adapted bindings: 636
Native adapted composable bindings: 159
Source-inert excluded bindings: 25
ACT_SPEC records resolved: 824
ACT_SPEC records pending: 24
Special registry definitions: 107 total / 95 legacy / 12 typed
Compatibility names: 108
Complete world-tool suite: 311 passed
Production-linked CuTest suite: 658 passed
Documentation findings: 0 errors, 0 warnings, 0 info
Warning-free Autotools build, test, and install: passed
Root-level circle artifact: absent
Evidence manifest hashes: verified
Live target writes: 0
```

Phase 6 continues with the remaining 54 direct bindings across 42 source handlers. Continue
grouping compatible irregular mechanics behind shared typed gateways, and preserve the full
build/test/install gate at each substantial commit boundary.

## 2026-08-12 - Phase 6 residual mobile-procedure batch

Status: Completed checkpoint; Phase 6 direct-binding reconciliation in progress

### Delivered

- Converted seven residual mobile handlers across 13 active bindings through the existing
  typed, identity-profiled `RoL Monster Combat` gateway. The shared registry and persistence
  contract remain unchanged.
- Preserved delayed extraplanar vanishing, Beavis and Butthead social activity, Finn's idle
  and combat speech, faerie player selection and gold theft, six spell-cast interception
  assignments, and the ancient brownie's combat attack.
- Mapped destructive source behavior through the target's purge event, damage, spell,
  paralysis-immunity, and invalidation paths. This retains the intended mechanics without
  unsafe direct extraction, hit-point mutation, or stale-target use.
- Classified the active `clock_tower` object binding as source-inert. Its assigned direct
  callback returns no event bits during initialization, and the source tree contains no
  separate clock-tower event registration.
- Added both build-manifest entries, profile and converter coverage, explicit disposition
  tests, and production-linked profile inventory tests.
- Reconciliation now resolves 1,085 of 1,147 active direct bindings and 512 of 562 source
  handlers; 62 bindings and 50 handlers remain. The independent `ACT_SPEC` checkpoint is
  824 resolved / 24 pending.
- Archived the fifty-fourth Phase 6 delivery session. The remaining Phase 6 envelope remains
  1-3 sessions, leaving the Phases 6-8 forecast at 49-79 sessions, or 98-316 focused
  engineering hours.

### Acceptance evidence

```text
Delivery commit: 0c545b1c
Reconciliation path: lib/rol-conversion/runs/phase6-special-20260812-residual-mobiles
Reconciliation run: rol-phase6-special-c60c0d2b988fd49f
Active direct bindings: 1,147
Direct bindings resolved: 1,085
Direct bindings pending: 62
Source handlers resolved: 512
Source handlers pending: 50
Additional handler families resolved: 8
Additional direct bindings resolved: 14
Native adapted bindings: 628
Native adapted composable bindings: 159
Source-inert excluded bindings: 25
ACT_SPEC records resolved: 824
ACT_SPEC records pending: 24
Special registry definitions: 107 total / 95 legacy / 12 typed
Compatibility names: 108
Complete world-tool suite: 311 passed
Production-linked CuTest suite: 657 passed
Documentation findings: 0 errors, 0 warnings, 0 info
Warning-free Autotools build, test, and install: passed
Root-level circle artifact: absent
Evidence manifest hashes: verified
Live target writes: 0
```

Phase 6 continues with the remaining 62 direct bindings across 50 source handlers. Continue
grouping compatible irregular mechanics behind shared typed gateways, and preserve the full
build/test/install gate at each substantial commit boundary.
