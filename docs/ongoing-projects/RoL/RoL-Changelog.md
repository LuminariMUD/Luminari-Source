# Realms of Luminari Project Changelog

This file records completed milestones removed from the active
[feature-first conversion plan](REALMS_OF_LUMINARI_FEATURE_FIRST_CONVERSION_PLAN.md)
and [zone conversion scope](REALMS_OF_LUMINARI_ZONE_CONVERSION_SCOPE.md). The plans
retain only forward-looking requirements, decisions, phases, and acceptance gates.

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

## 2026-08-12 - Phase 5 shop compatibility and completion

Status: Phase 5 completed; Phase 6 special-procedure reconciliation in progress

### Delivered

- Reconciled the active source inventory to 453 shops: 450 fixed-room shops and three
  roaming-only shops. The earlier figure of 458 describes a provisional/raw inventory,
  not the grammar-selected active records emitted by the final audit.
- Preserved products, accepted item types, messages, keepers, resolved rooms, hours,
  fixed and roaming operation, the load-bearing `v3.0` header, and exact source price
  formulas. Player buy prices use `GREED / 100`; player sell prices use
  `GREED / (100 + PROFIT)`.
- Mapped authored `HATES` alignment, class, race, and NPC groups to target customer
  restrictions. Closely related source-only identities use explicit bounded target
  equivalents; real source identities without a target counterpart remain named losses.
  Invalid source tokens such as the observed `PZ` and `NP` are recorded as source-inert
  instead of being assigned invented behavior.
- Added an optional persistent `R <mask>~` shop record for the source `CHEATS` contract.
  Matching customers pay twice the normal buy price and receive half the normal sell
  price. Boot, lookup, shop save, and world export paths preserve the record.
- Added converter-owned magic-policy bits. Native shops remain unchanged; converted
  shops permit `cast`, `recite`, and `use` only when the source authored `CASTING`.
  `KILLABLE` and `ROAMING` use the existing target shopkeeper and roaming contracts.
- Traced `DEADBEAT` as source-inert, bounded `OFFENSE` to the target refusal policy, and
  retained source-only open, close, and bigotry messages as explicit conversion evidence.
- Regenerated the full-corpus audit and five-zone pilot with zero generic capability
  gaps, zero unmapped symbolic observations, zero transform exceptions, and zero live
  target writes. The remaining 848 special-binding diagnostics are owned by Phase 6;
  the 804 record-specific reference diagnostics are owned by Phase 7 batches.
- Removed completed Phase 5 and shop requirements from the active plan and scope. The
  remaining forecast is 96-156 sessions, or 192-624 focused engineering hours at the
  defined 2-4 hour session size.

### Acceptance evidence

```text
Delivery commits: ec1a8cd8, fe38a56e
Full-corpus audit path: lib/rol-conversion/runs/phase5-shop-20260812-audit
Full-corpus audit run: rol-phase5-audit-3fb8de2d9afd067b
Pilot build path: lib/rol-conversion/runs/phase5-shop-20260812-pilot
Pilot build run: rol-phase4-build-35c9c879af63b8d1
Active source records: 71,680
Convertible records emitted: 69,920
Active shops emitted: 453
Emitted target bytes: 42,099,580
Converter exceptions: 0
Unmapped symbolic observations: 0
Generic capability gaps: 0
Pilot selected actions: 3,001
Pilot active staged errors: 79 inherited, 0 new
Pilot reset-reference and walkthrough gates: passed
Complete world-tool suite: 248 passed
Production-linked CuTest suite: 619 passed
Documentation findings: 0 errors, 0 warnings, 0 info
Autotools build and install: passed
Root-level circle artifact: absent
Live target writes: 0
```

Phase 6 starts from the already owned special-binding inventory; no generic shared
capability remains as a prerequisite.

## 2026-08-12 - Phase 5 exit-trap compatibility

Status: Completed sub-milestone; Phase 5 implementation in progress

### Delivered

- Added a strict persistent room record for all 34 valid active RoL exit-trap payloads,
  including direction, state, source type, damage range, area behavior, hardness, and
  load percentage. The one malformed `scorn_fe` row is excluded with a source-located
  diagnostic rather than guessed into target data.
- Added loader, writer, room-copy, OLC-copy, free, validation, and lookup support for the
  record so converted traps survive normal world-data lifecycles.
- Adapted the source blade, poison, rock, fire, lightning, random, and falling families.
  Runtime behavior preserves flat damage ranges, area targeting, poison, and falling
  characters and non-floating room objects through a valid down exit.
- Integrated mortal detection and disabling plus open and lock-pick triggering without
  changing native close, unlock, object-trap, NPC, or immortal behavior. Source exit
  traps remain reusable rather than becoming one-shot target traps.
- Preserved boot load probability and mapped the source door-reset rearm bit. Rearming
  clears detected, disarmed, and triggered state before rerolling the authored load
  percentage.
- Rebuilt the full-corpus audit and five-zone pilot with zero live target writes. The
  pilot contains an actual converted area rock trap in room 2026051 and continues to
  add zero active staged errors while passing reset and walkthrough gates.

### Acceptance evidence

```text
Delivery commit: c647c5f4
Full-corpus audit path: lib/rol-conversion/runs/phase5-exit-traps-20260812-audit
Full-corpus audit run: rol-phase5-audit-c6c7050ef434f7b8
Pilot build path: lib/rol-conversion/runs/phase5-exit-traps-20260812-pilot
Pilot build run: rol-phase4-build-7e8fa263dff52098
Active source records: 71,680
Convertible records emitted: 69,920
Emitted target bytes: 42,095,793
Converter exceptions: 0
Unmapped symbolic observations: 0
Valid exit-trap payloads adapted: 34
Malformed exit-trap payloads excluded: 1
Door-reset rearm rows mapped: 33
Pilot selected actions: 3,001
Pilot active staged errors: 79 inherited, 0 new
Pilot reset-reference and walkthrough gates: passed
Complete world-tool suite: 246 passed
Production-linked CuTest suite: 617 passed
Documentation findings: 0 errors, 0 warnings, 0 info
Autotools build and install: passed
Root-level circle artifact: absent
Live target writes: 0
```

The next Phase 5 pass resolves reconciled shop behavior and remaining reusable shared
capability gaps.

## 2026-08-12 - Phase 5 reset mobile-chain compatibility

Status: Completed sub-milestone; Phase 5 implementation in progress

### Delivered

- Traced all active source reset shapes and corrected the semantic mismatch between
  source boolean dependencies and the target result-offset queue.
- Added the converter-owned `RoL-Reset-Compat` zone flag. In flagged zones, all `E`
  and `G` commands bind to the most recent successful `M` command even when an earlier
  equipment or inventory load fails. Native zones retain their existing queue behavior.
- Normalized every non-`F` source dependency to its actual boolean contract. The three
  active non-boolean source values now emit as `1` with explicit diagnostics; `F`
  continues to preserve its source follow-mode overload.
- Regenerated the constants manifest and documented the new zone flag. Added converter
  fixtures and a production-linked predicate test covering source and native behavior.
- Rebuilt the full-corpus audit and five-zone staged pilot with zero live target writes.
  The pilot still has no new active errors and passes reset observations and scripted
  walkthroughs.

### Acceptance evidence

```text
Delivery commit: afeea9d7
Full-corpus audit path: lib/rol-conversion/runs/phase5-reset-chain-20260812-audit
Full-corpus audit run: rol-phase5-audit-ed84cba825215e4f
Pilot build path: lib/rol-conversion/runs/phase5-reset-chain-20260812-pilot
Pilot build run: rol-phase4-build-0036becbb939e3ad
Active source records: 71,680
Convertible records emitted: 69,920
Emitted target bytes: 42,094,939
Converter exceptions: 0
Unmapped symbolic observations: 0
Pilot selected actions: 3,001
Pilot active staged errors: 79 inherited, 0 new
Pilot reset-reference and walkthrough gates: passed
Complete world-tool suite: 243 passed
Production-linked CuTest suite: 615 passed
Documentation findings: 0 errors, 0 warnings, 0 info
Autotools build and install: passed
Root-level circle artifact: absent
Live target writes: 0
```

The next Phase 5 pass persists the 34 valid active exit-trap payloads and their reset
rearming contract, then continues with reconciled shop behavior and remaining shared
capability gaps.

## 2026-08-12 - Phase 5 mobile-action compatibility

Status: Completed sub-milestone; Phase 5 implementation in progress

### Delivered

- Completed explicit dispositions for all 15,243 active mobile-action observations.
  The full-corpus audit now reports zero unmapped symbolic observations across all
  71,680 active source records.
- Added target behavior for nice thieves, same-sector wandering, delayed hunting,
  one-room archery, independent psionic/divine/arcane/rogue/warrior roles, and
  aggression toward source-good or source-evil races.
- Converted source protector behavior to the existing helper plus adjacent-combat
  listener behaviors. Converted break-charm and outcast aggression to documented
  bounded target equivalents.
- Corrected class-zero conversion to a warrior baseline and inferred the primary target
  class from source role flags while retaining every independent role as a flag.
- Assigned source `ACT_SPEC` to the Phase 6 special-binding reconciler instead of
  treating it as an unowned generic gap. Source relationship-only `ACT_SAVE` and the
  unconsumed `ACT_SPEC_DIE` now have explicit logged source-only dispositions.
- Rebuilt the five-zone staged pilot with zero live target writes. The pilot provides
  concrete archer, protector, class-role, race-aggression, and nice-thief test
  candidates while preserving its structural, reset-reference, and walkthrough gates.

### Acceptance evidence

```text
Delivery commit: 1f6020de
Full-corpus audit path: lib/rol-conversion/runs/phase5-mobile-actions-20260812-v2
Full-corpus audit run: rol-phase5-audit-fc1c1ddc402d3800
Pilot build path: lib/rol-conversion/runs/phase5-mobile-actions-20260812-pilot
Pilot build run: rol-phase4-build-f11ba7e2f3909645
Active source records: 71,680
Convertible records emitted: 69,920
Emitted target bytes: 42,089,791
Converter exceptions: 0
Previous unmapped symbolic observations: 15,243
Current unmapped symbolic observations: 0
Reduction: 15,243
Pilot selected actions: 3,001
Pilot active staged errors: 79 inherited, 0 new
Pilot retained native/adapted specials: 46/45
Pilot reset-reference and walkthrough gates: passed
Complete world-tool suite: 242 passed
Production-linked CuTest suite: 614 passed
Documentation findings: 0 errors, 0 warnings, 0 info
Autotools build and install: passed
Root-level circle artifact: absent
Live target writes: 0
```

The next Phase 5 pass addresses reset transforms, reconciled shop behavior, and the
remaining reusable shared capability gaps.

## 2026-08-12 - Phase 5 object-apply and affect compatibility

Status: Completed sub-milestone; Phase 5 implementation in progress

### Delivered

- Completed dispositions for every active object apply, object item type, object wear
  flag, mobile affect, object affect, and sector value. The full-corpus audit now has
  only mobile-action observations remaining in its unmapped symbolic inventory.
- Mapped source agility and power applies to target Dexterity and Wisdom. Mapped the
  six maximum-stat apply identities to their target attributes and converted the four
  active multiplicative race-factor tuples to bounded D20-scale stat modifiers.
- Traced source Karma and Luck apply fields and confirmed that no active source mechanic
  consumes their modified values; they are now omitted with explicit source-only
  diagnostics.
- Corrected object grammar ownership so only the two numeric rows immediately before
  object extensions can be affect masks. Numeric content following an extension is
  diagnosed and ignored instead of becoming a false object affect.
- Added target secondary affects for RoL slow poison and docile behavior. Slow poison
  halves positive poison damage with a minimum of one; docile mobiles no longer assist
  as helpers or guards. Source meditation maps to the bounded rapid-preparation affect.
- Explicitly omitted transient or source-inert prototype affects after tracing their
  source call sites. Repaired the single malformed mountain sector, one shifted object
  type, and two source-corrupt wear bits with record-specific diagnostics.
- Rebuilt the five-zone staged pilot without live target writes. All 3,001 selected
  actions remain disposed, generated syntax is complete, and reset-reference and
  walkthrough gates still pass.

### Acceptance evidence

```text
Delivery commit: f7eabca4
Full-corpus audit path: lib/rol-conversion/runs/phase5-object-applies-affects-20260812
Full-corpus audit run: rol-phase5-audit-ae0fdf51ee3707ee
Pilot build path: lib/rol-conversion/runs/phase5-object-applies-affects-20260812-pilot
Pilot build run: rol-phase4-build-87b9c7b1b8e214bd
Previous unmapped symbolic observations: 15,731
Current unmapped symbolic observations: 15,243
Reduction: 488
Remaining unmapped symbolic families: mobile action only
Convertible records emitted: 69,920
Emitted target bytes: 42,083,358
Converter exceptions: 0
Pilot selected actions: 3,001
Pilot active staged errors: 79 inherited, 0 new
Pilot retained native/adapted specials: 46/45
Pilot reset-reference and walkthrough gates: passed
Complete world-tool suite: 241 passed
Production-linked CuTest suite: 613 passed
Documentation findings: 0 errors, 0 warnings, 0 info
Autotools build and install: passed
Root-level circle artifact: absent
Live target writes: 0
```

The next Phase 5 pass resolves the measured mobile-action family, then continues with
reset, shop, and other shared capability gaps.

## 2026-08-12 - Phase 5 object-property compatibility

Status: Completed sub-milestone; Phase 5 implementation in progress

### Delivered

- Completed dispositions for all 1,556 active source object-extra-flag observations.
  Every observed flag now maps to target behavior or has a traced source-inert
  disposition.
- Added nine explicit RoL compatibility flags for good-race and evil-race equipment
  restrictions, identify resistance, summon protection, sleep protection, charm
  protection, forced two-handed use, whole-body armor, and whole-head equipment.
- Enforced the flags through equipment and scroll restrictions; identify, mass
  identify, lore, charm, sleep, summon, group summon, hand-use, combat modifier, and
  overlapping armor paths. Immortals retain the source administrative bypass for
  identify and restricted scroll use.
- Mapped source `NOSHOW` to the target hidden-object flag. Traced source `DARK` through
  its light-recalculation call sites and confirmed that the source light counters never
  consume it; it is omitted explicitly instead of inventing target darkness.
- Extended OEdit constants, generated constants, builder documentation, converter
  fixtures, and production-linked behavior tests. The four-chunk object bit array has
  capacity for all 125 target flags.
- Rebuilt the five-zone staged pilot. It contains testable no-identify, two-handed,
  race-restricted, whole-body, and no-summon objects while preserving all earlier
  structural, reset, and walkthrough gates.

### Acceptance evidence

```text
Delivery commit: ca037b15
Full-corpus audit path: lib/rol-conversion/runs/phase5-object-flags-e5b998bd
Full-corpus audit run: rol-phase5-audit-fb713f798161a78b
Pilot build path: lib/rol-conversion/runs/phase5-object-flags-e5b998bd-pilot
Pilot build run: rol-phase4-build-4e6f5f9a132e06cc
Active source object-extra observations: 1,556
Unmapped object-extra observations: 0
Previous unmapped symbolic observations: 17,287
Current unmapped symbolic observations: 15,731
Reduction: 1,556
Convertible records emitted: 69,920
Emitted target bytes: 42,078,773
Converter exceptions: 0
Pilot selected actions: 3,001
Pilot active staged errors: 79 inherited, 0 new
Pilot reset-reference and walkthrough gates: passed
Complete world-tool suite: 237 passed
Production-linked CuTest suite: 607 passed
Documentation findings: 0 errors, 0 warnings, 0 info
Autotools build and install: passed
Root-level circle artifact: absent
Live target writes: 0
```

The next Phase 5 pass addresses mobile actions, object applies, the remaining item-type
and wear rows, the one malformed sector, and explicit dispositions for the remaining
transient affects.

## 2026-08-12 - Phase 5 room, zone, and affect compatibility

Status: Completed sub-milestone; Phase 5 implementation in progress

### Delivered

- Completed symbolic ownership for every active source room flag and zone flag. All
  53,987 room-flag observations and all 83 zone-flag observations now have traced
  target behavior or an explicit source-only disposition.
- Preserved source zone restrictions on each emitted room, including peaceful,
  soundproof, no-teleport, no-magic, no-recall, and no-summon behavior. A source
  `MAGIC_OK` room removes an inherited no-magic restriction.
- Added bounded room compatibility for underwater sectors, arenas, no-precipitation
  rooms, the RoL jail marker, psionic-regeneration rooms, healing rooms, and solid or
  fire fog. Arena flags participate in arena death rules and PvP without widening the
  target's legacy PvP VNUM range; no-precipitation rooms suppress weather without
  suppressing daylight; psionic-regeneration rooms double positive PSP tick gain.
- Corrected the high RoL affect-number offset and resolved 3,517 previously unmapped
  mobile and object affect observations. The remaining 177 observations are confined
  to seven transient or source-only affect identities awaiting explicit dispositions.
- Rebuilt the five-zone pilot from the new converter. All 3,001 selected actions,
  73 preserved-mobile patches, 14 shops, 181 SOC triggers, 13 special triggers, and
  46 retained native bindings remain represented with zero live target writes.

### Acceptance evidence

```text
Delivery commit: f0fb9d8f
Full-corpus audit path: lib/rol-conversion/runs/phase5-room-zone-df35cb1f
Full-corpus audit run: rol-phase5-audit-719a67acc4cb6b01
Pilot build path: lib/rol-conversion/runs/phase5-room-flags-f0fb9d8f
Pilot build run: rol-phase4-build-e5d61111edbfcd9d
Convertible records emitted: 69,920
Emitted target bytes: 42,078,584
Converter exceptions: 0
Previous unmapped symbolic observations: 26,006
Current unmapped symbolic observations: 17,287
Reduction: 8,719
Unmapped room-flag observations: 0
Unmapped zone-flag observations: 0
Unmapped mobile and object affect observations: 177
Pilot selected actions: 3,001
Pilot active staged errors: 79 inherited, 0 new
Pilot reset-reference and walkthrough gates: passed
Complete world-tool suite: 236 passed
Production-linked CuTest suite: 607 passed
Documentation findings: 0 errors, 0 warnings, 0 info
make install: passed
Root-level circle artifact: absent
Live target writes: 0
```

The next recorded Phase 5 pass completed object-property compatibility; its evidence is
the milestone immediately above.

## 2026-08-12 - Phase 5 full-corpus capability audit

Status: Completed sub-milestone; Phase 5 implementation in progress

### Delivered

- Added `rol-capability-audit`, a deterministic, non-mutating pass over every active
  record and every Phase 2 disposition. It reparses the authoritative physical source,
  verifies the Phase 2 bundle, emits every non-SOC record that is not already excluded,
  inventories symbolic values, and records transform diagnostics and exceptions.
- Emitted all 69,920 convertible records, representing 42,075,289 bytes of target text,
  with zero converter exceptions and zero live target writes.
- Reconciled the source quest runtime's optional second `R:I` integer against all 1,467
  active item-reward directions. No active direction supplies a random-object range;
  every active item reward is a fixed VNUM. The unsupported hypothetical form therefore
  blocks no active content.
- Quantified 26,006 as-yet-unmapped symbolic observations across room, mobile, object,
  affect, apply, item-type, wear, and sector families. These are now exact owned Phase 5
  work rather than unmeasured pilot risk.
- Repaired the Automake/CMake world-tool source manifests to include the Phase 4 build,
  SOC, special-procedure, transform-test, and pilot-build modules added by earlier
  checkpoints, and kept the new audit modules synchronized in both build systems.

### Acceptance evidence

```text
Delivery commit: 51b7bd13
Audit run: rol-phase5-audit-13d7727804344e9a
Active source records: 71,680
Final Phase 2 dispositions: 71,680
Convertible records emitted: 69,920
SOC records deferred to the Phase 6 reconciler: 1,754
Locked malformed record exclusions: 6
Emitted target bytes: 42,075,289
Converter exceptions: 0
Active qst R:I directions: 1,467
Active random item-reward ranges: 0
Unmapped symbolic observations: 26,006
Complete world-tool suite: 234 passed
Documentation findings: 0 errors, 0 warnings, 0 info
Live target writes: 0
```

The next Phase 5 pass resolves the measured symbolic families by traced equivalence,
bounded compatibility adapters, or explicit smallest-unit dispositions. Record-specific
missing references remain attached to their Phase 7 packages rather than being mistaken
for generic converter failures.

## 2026-08-12 - Phase 5 object-trap compatibility

Status: Completed sub-milestone; Phase 5 implementation in progress

### Delivered

- Traced the source object-trap loader, all trigger call sites, damage and status
  behavior, random-type mutation, charge persistence, detection, disarming, and
  same-zone teleport behavior.
- Added deterministic conversion of the source six-field object `T` payload into
  target object values 10-15 plus `ITEM_TRAPPED`. This avoids colliding with the
  target `T` DG-trigger attachment record and uses object fields already preserved by
  prototype, player, house, pet, and sheath saves.
- Added boot-time and world-tool validation for the compatibility payload. Invalid
  effect masks, damage types, charges, levels, or dice fail closed instead of creating
  an accidental live hazard.
- Added runtime triggers for directional movement, get and put, open, and lock-pick
  events; single-target and room-area delivery; physical, venom, explosion, sleep,
  same-zone teleport, fire, cold, acid, lightning, and source random damage families;
  finite and unlimited charges; and staff/NPC bypass behavior.
- Extended `detecttrap <object>` and `disabletrap <object>`, immortal object stat
  output, authoritative database help entries, and the local file fallback. The source
  search-trigger bit is preserved for inspection but remains inert because the source
  defines `checksearch()` without calling it from any command path.
- Audited every active source object-trap record. All 29 valid payloads convert and
  parse as target objects. Four empty source `T` rows are explicitly omitted as
  inactive/malformed smallest units.

### Acceptance evidence

```text
Delivery commit: 6a1ddb6d
Active source object T records: 33
Valid payloads converted: 29
Empty/inactive source rows omitted with diagnostics: 4
Supported source event families: move, get/put, open, pick
Supported damage/status families: 15
Complete world-tool suite: 232 passed
Production CuTest suite: 605 passed
Documentation findings: 0 errors, 0 warnings, 0 info
Compatibility pilot run: rol-phase4-build-1dc8a681fa1595d5
Pilot output change: none; the five pilots contain no active object T payload
Selected pilot actions disposed: 3,001
Staged active errors: 79 inherited, 0 new
Live target writes: 0
Autotools build and install: passed
Root-level circle build artifact after install: absent
```

The next Phase 5 compatibility pass begins with the full-corpus symbolic-value audit.

## 2026-08-12 - Phase 5 rare room extension compatibility

Status: Completed sub-milestone; Phase 5 implementation in progress

### Delivered

- Added the optional target room `R <minimum> <maximum>` record with strict loader,
  canonical writer, REdit, `stat room`, world-tool parser, lookup, semantic validation,
  fixture, and builder-documentation coverage. `-1` is the canonical unrestricted
  bound.
- Enforced room entry ranges for physical movement, portal users and followers, mortal
  teleport destinations, and mounts. Immortal player characters may bypass a finite
  maximum but not a minimum; NPCs and mounts do not receive that bypass.
- Converted all three active source level-range records. Source maxima 35 and 39 are
  normalized to unrestricted because the target player-level ceiling is 34; finite
  minimum bounds remain effective.
- Traced the one active source `F` fall-chance extension from loader through runtime and
  confirmed that the source stores and validates it without consuming it. It is omitted
  with an explicit source-inert diagnostic.
- Traced all 36 active source `M` mana extensions and confirmed that the source marks
  them obsolete and never consumes them. They are omitted with explicit diagnostics;
  target room `M` remains reserved for moving-room data.
- Added a safe manual-test matrix for the completed Phase 4 pilot and restaged that
  pilot with the Phase 5 quest and room compatibility checkpoints. No development-world
  files were changed.

### Acceptance evidence

```text
Delivery commit: e100bdff
Active rare room-extension rows inspected: 40
Source R rows converted: 3
Source-inert F rows omitted with diagnostics: 1
Obsolete M rows omitted with diagnostics: 36
Targeted room/parser/semantic/transform tests: 44 passed
Complete world-tool suite: 227 passed
Production CuTest suite: 602 passed
Documentation findings: 0 errors, 0 warnings, 0 info
Compatibility pilot run: rol-phase4-build-1dc8a681fa1595d5
Selected pilot actions disposed: 3,001
Staged active errors: 79 inherited, 0 new
Live target writes: 0
Autotools build and install: passed
Root-level circle build artifact after install: absent
```

The remaining Phase 5 shared-extension work begins with source object `T` traps and
random quest item reward `R:I` ranges. A capability-complete full-corpus bundle is not
yet ready for development-world application.

## 2026-08-12 - Phase 5 quest reward and SOC terminator compatibility

Status: Completed sub-milestone; Phase 5 implementation in progress

### Delivered

- Extended the existing target HLQuest system with appended `P` quest-point and `E`
  experience output commands. Existing command indexes `0..11` and their persisted
  codes are unchanged.
- Added loader, runtime, display, editor, canonical writer, source-derived constant,
  validator, fixture, and builder-documentation coverage for both commands.
- Adapted source reward direction `R:E` to the normal target quest-mode experience
  path, as required by the locked conversion policy, instead of retaining the disabled
  source implementation.
- Adapted signed `R:P` prestige deltas to the target's persistent quest-point economy.
  The target balance safely saturates at `0..100000000`.
- Mapped all 29 spell/skill identities used by active `R:S` rows. Twenty-seven have a
  target teachable equivalent; source-only ultrablast and sandstorm rewards are omitted
  individually with explicit diagnostics because the HLQuest teaching contract has no
  equivalent.
- Added the argument-free `R:A` attack reward and made the SOC compiler recognize
  `LISTDONE` as an explicit list terminator.

### Acceptance evidence

```text
Delivery commit: 7bbfba3d
Active source qst records inspected: 5,078
R:A rows: 10
R:E rows: 56
R:P rows: 12
R:S rows: 33 across 29 unique source identities
Mapped R:S identities: 29 of 29
Target-equivalent R:S identities: 27
Explicit source-only R:S identities: 2
Targeted Python tests: 80 passed
Production CuTest suite: 601 passed
Source-derived constants: current
Documentation findings: 0 errors, 0 warnings, 0 info
Autotools build and install: passed
Root-level circle build artifact after install: absent
```

This sub-milestone does not close the broader Phase 5 quest gate. Full-corpus
conversion still has to resolve random item-reward ranges and prove every emitted host
block against the complete identity map. Room `M`, room `F`, and object `T` are the
next unimplemented shared grammar rows.

## 2026-08-12 - Phase 4 representative vertical pilot completed

Status: Completed; Phase 5 is the active phase

### Delivered

- Completed all 3,001 selected record actions across `swamp_two`, `hulburg`, `muspel`,
  `theswamp`, and `cemetery`: 1,408 `ADD`, 1,465 `KEEP`, 127 `MERGE`, and one
  smallest-record `EXCLUDE`.
- Added the deterministic `rol-pilot-build` pipeline. It reparses the source oracle,
  transforms selected records, applies only explicit patches/appends to a disposable
  copy of the target, updates indexes, and writes a hashed evidence bundle without
  changing the live development world.
- Generated 25 new world files, patched 73 preserved target mobiles, and appended 14
  shops. The compiler disposed all 91 selected special bindings: 46 persisted native
  bindings and 45 DG adaptations compiled into 13 triggers.
- Combined the special triggers with 181 SOC triggers for 194 generated DG triggers.
  Added the bounded runtime adapters and native handlers needed by the pilot before the
  generated data was booted.
- Added semantic source-to-target magic-item spell mapping, capped source spell levels
  at the target limit, and explicitly disabled `mud to rock`, for which the target has
  no equivalent. Unknown future source spell IDs fail closed to a logged disabled slot.
- Corrected alphabetic bit-vector decoding at bit 31 so signed `int` promotion cannot
  create false high bits, and made RoL mobile-removal resets ignore characters already
  pending extraction.
- Added a machine-readable pilot runtime contract. It verifies all reset references,
  missing exit targets, connected-component traversal roots, and representative routes
  for every selected target zone.
- Completed the evidence-based reforecast and removed Phase 4 from both active plans.
  The active project now begins at Phase 5.

### Acceptance evidence

```text
Final integration commits: 694cf84f, 3fe7015f
Build run directory: lib/rol-conversion/runs/phase4-build-e6ea7982
Build run ID: rol-phase4-build-a2c341dfaa743b26
Selected actions disposed: 3,001 of 3,001
Generated world files: 25
Preserved target mobiles patched: 73
Shops appended: 14
Special bindings disposed: 91 of 91
SOC DG triggers: 181
Special DG triggers: 13
Generated overlay parse complete: yes
New staged active errors: 0
Implicit target overwrites: 0
Live target writes: 0
Reset-reference observations: 5 of 5 zones passed
Walkthrough coverage: 1,160 of 1,160 rooms passed
Focused world-tool tests: 224 passed
Production CuTest suite: 601 passed
Autotools build and install: passed
Root-level circle build artifact after install: absent
Isolated syntax boot: passed with no pilot-related SYSERR
Isolated MariaDB behavioral boot: entered game loop and terminated normally
Eligible boot-time pilot resets observed: zones 1591 and 20586
Pilot-related spell/reference/reset/trigger/extraction boot errors: 0
```

The staged validator still reports 79 active errors inherited from the authoritative
target baseline, including pre-existing Hulburg HLQ references and incomplete
unselected HLQ files. Therefore global `staged_parse_complete` remains false. The pilot
added zero active errors, and its generated overlay parses completely. Phase 7 package
batches retain the requirement to repair selected/touched lineage findings before
project completion; Phase 4 completion does not waive the final Definition of Done.

### Measured reforecast

The pilot covered 3,001 of 71,680 active records (4.19 percent), five of 252 active
physical packages (1.98 percent), and 77 of 89 capability rows (86.52 percent). It was
deliberately lineage-heavy: its action mix was 48.82 percent `KEEP`, while the complete
plan is 95.07 percent `ADD`. After the pilot, 68,679 records and 247 physical packages
remain for broad batching.

Special procedures dominate uncertainty. The pilot covered 91 of 1,234 active source
bindings and 34 of 605 source handler names. The remaining 1,143 bindings and 571
handler names are why Phase 6 expands beyond its provisional range even though Phase 5
shrinks after broad capability coverage in the pilot.

| Phase | Evidence-based sessions | Focused hours at 2-4 hours/session |
|------:|------------------------:|-----------------------------------:|
| 5 | 8-14 | 16-56 |
| 6 | 48-80 | 96-320 |
| 7 | 42-66 | 84-264 |
| 8 | 6-10 | 12-40 |
| **Remaining total** | **104-170** | **208-680** |

These are engineering planning envelopes, not calendar promises. Reforecast again
after Phase 5 resolves the 12 capability rows absent from the pilot and inventories
full-corpus symbolic value variants.

## 2026-08-11 - Phase 4 pilot SOC and native special capabilities

Status: Completed sub-milestone; Phase 4 implementation in progress

### Delivered

- Added deterministic compilation for all 245 selected source SOC rows across all five
  source modes and all five special action codes. The compiler emits 181 target DG
  triggers, a 26.122449 percent reduction from safe grouping of equivalent actions.
- Preserved command identity, chance, delay, actor restrictions, command arguments,
  paths, flags, all-zone echo, and termination behavior. Added bounded
  `mrolzoneecho` and `mrolwalkto` adapters for behavior DG could not otherwise express
  faithfully.
- Added 24 VNUM-independent native special-procedure families covering 46 of the 91
  selected bindings: six mobile combat/command families and 18 object command, hit,
  defense, and pulse families.
- Registered the new handlers through the persisted named special-procedure registry,
  including exact owner/event contracts, required prototype flags, OLC inventory, and
  legacy compatibility accessors. No source VNUM is hardcoded in the native handlers.
- Kept the source handler names as canonical persisted identities so generated world
  bindings remain directly traceable to the source assignment inventory.
- Removed completed SOC compilation and native special-family implementation from the
  active plan and scope. The active special-binding compiler must persist the 46 native
  bindings and DG-compile the remaining 45 bindings in seven VNUM-dependent behavior
  families.

### Acceptance evidence

```text
Delivery commits: 56393f8c, 8136c71b
Selected SOC rows compiled: 245 of 245
Generated target DG triggers: 181
SOC modes covered: 5 of 5
SOC special action codes covered: 5 of 5
VNUM-independent special families implemented: 24
Selected bindings covered by native families: 46 of 91
Special registry definitions: 52 total; 50 legacy; 2 typed
Focused SOC compiler tests: 21 passed
Production CuTest suite: 600 passed
Autotools build and install: passed
Root-level circle build artifact after install: absent
Live target writes: 0
```

## 2026-08-11 - Phase 4 pilot shop and quest capabilities

Status: Completed sub-milestone; Phase 4 implementation in progress

### Delivered

- Added semantic conversion for all 15 selected source shops: 14 Hulburg records and
  one Muspel record. Products, keepers, rooms, buy types, prices, messages, hours, and
  source killability/roaming behavior emit as parser-clean native target shops.
- Added a native roaming-shop flag with runtime room-access, OLC persistence, format
  documentation, and production tests. Source-only shop AI fields remain explicit
  conversion diagnostics rather than silent losses.
- Added source `.qst` to canonical target HLQ compilation for all 57 selected
  non-reused quest hosts. Ask and completion entries, item/coin inputs, item/coin
  rewards, room object/mobile loads, disappearance, list-prepend ordering, and the
  mandatory `!` approval marker are preserved.
- Adapted native HLQ completion to consume the exact configured coin amount, accept
  coin values through `MAX_GOLD`, require every repeated copy of a duplicate item
  input, and extract consumed items as the source completion contract does.
- Preserved the text of source disappearance messages by folding it into the target
  completion reply before executing the native disappear output. Unsupported broader
  quest directions remain explicit smallest-direction exclusions and were not needed
  by the locked pilot.
- Removed the completed pilot shop and quest capability work from the active plan and
  scope. SOC behavior, special bindings, deterministic bundling, staged validation,
  walkthroughs, and reforecasting remain active.

### Acceptance evidence

```text
Delivery commits: 10a30cf2, d5609b1c
Selected shops emitted and parsed: 15 of 15
Selected added quest hosts emitted and parsed: 57 of 57
Quest entries emitted pre-approved: 100 percent
Focused Python conversion/semantic tests: 23 passed
Production CuTest suite after shops: 599 passed
Production CuTest suite after quests: 600 passed
Autotools build and install: passed
Root-level circle build artifact after install: absent
Live target writes: 0
```

## 2026-08-11 - Phase 4 pilot record and reset capabilities

Status: Completed sub-milestone; Phase 4 implementation in progress

### Delivered

- Added semantic target emitters for selected room, mobile, object, and zone records,
  including target-syntax parsing and structural validation fixtures.
- Corrected the source reset oracle so numeric comment text cannot be interpreted as
  reset arguments.
- Added conversion and native runtime support for source base resets, legacy door
  bitmasks and chance, follow/group/mount relationships, mobile removal, calendar
  predicates, and chance-qualified object removal.
- Added blocked-exit persistence and movement enforcement, plus OLC save, display,
  export, and prototype-renumbering support for the conversion reset commands.
- Preserved unsupported source tail equipment slots and legacy door-trap extensions as
  explicit smallest-instruction exclusions with diagnostics instead of emitting unsafe
  or semantically false target data.
- Removed the completed record/reset capability work from the active Phase 4 plan and
  scope. Shops, quests, SOC behavior, special bindings, pilot bundling, staged runtime
  validation, walkthroughs, and the evidence-based reforecast remain active.
- Regenerated Phase 1, Phase 2, and pilot-selection evidence after the reset-oracle
  correction. All package, record, reference, binding, coverage, and action totals
  remained unchanged.

### Acceptance evidence

```text
Delivery commits: 3a6b7602, 72c2ef24
Focused conversion and parser tests: 44 passed
Production CuTest suite: 598 passed
Autotools build and install: passed
Root-level circle build artifact after install: absent
Corrected Phase 1 run: rol-phase1-1c287d5073293f7c
Corrected Phase 2 run: rol-phase2-39a6d6d253950dff
Corrected selection run: rol-phase4-select-6f7ae16e5df665ec
Regenerated artifact hash failures: 0
Live target writes: 0
```

## 2026-08-11 - Phase 4 representative pilot selection

Status: Completed sub-milestone; Phase 4 implementation in progress

### Delivered

- Selected `swamp_two`, `hulburg`, `muspel`, `theswamp`, and `cemetery` as the locked
  five-package pilot from measured Phase 1 and Phase 2 evidence.
- Covered a compact conventional-reset oracle, a confirmed-lineage settlement with
  shops and quests, all five SOC modes and all five special SOC action codes, all three
  custom `F`, `T`, and `X` reset families, uncommon object and room extensions, and
  significant special-procedure dependencies.
- Selected 3,001 records with 14,103 typed references and 91 active source special
  bindings. Their record actions are 1,408 `ADD`, 1,465 `KEEP`, 127 `MERGE`, and one
  `EXCLUDE`.
- Produced a deterministic source oracle plus complete pilot record, identity,
  reference, action, binding, and coverage artifacts without writing the live target.
- Removed the completed selection requirements from the two active plans; Phase 4 now
  contains only pilot conversion, validation, and reforecast work.

### Acceptance evidence

```text
Delivery commits: fe523532, a0b54009
Evidence refresh revision: e6ea7982
Run directory: lib/rol-conversion/runs/phase4-select-e6ea7982
Run ID: rol-phase4-select-6f7ae16e5df665ec
Packages selected: 5
Records selected and planned: 3,001 of 3,001
Selection coverage checks: 9 of 9
SOC modes covered: 5 of 5
SOC special action codes covered: 5 of 5
Custom reset families covered: F, T, X
Live target writes: 0
```

This run supersedes `rol-phase4-select-821d842548312f9f`. The corrected source reset
arguments changed the canonical record/action payload hashes but did not change the
locked packages, selected records, reference totals, binding totals, coverage checks,
or action counts.

## 2026-08-11 - Phases 1-3 conversion foundation and walking skeleton

Status: Completed; Phase 4 not started

### Phase 1 delivered

- Added grammar-aware parsers for every active `zon`, `wld`, `mob`, `obj`, `shp`,
  `qst`, and `soc` source record, including loader defaults, compact syntax, typed
  references, known source defects, and smallest-unit exclusions.
- Parsed 71,680 active records, classified 420,124 directives, and extracted 355,042
  typed references. Source parsing completed with 113 warnings and six explicit
  malformed-record exclusions rather than silent data loss.
- Produced a complete owned dependency graph: 239,051 active-source resolutions,
  112,708 exact target resolutions, 100 target-lineage candidates, 2,547 source-command
  resolutions, 237 sentinels, 25 excluded-source resolutions, and 374 explicit
  smallest-dependent-instruction exclusions for otherwise unresolved references.
- Inventoried 1,017 commands, five SOC special action codes, 1,234 active source
  special-procedure bindings, 50 numeric persistent VNUM columns containing 6,286
  distinct values, and 89 capability rows. Every capability and reference received an
  engineering-owned disposition.
- Reconciled all seven source aggregates byte for byte and accepted the two semantic
  tail differences only through their exact Phase 0 loss evidence. Confirmed three
  prior-lineage packages through a documented seed plus broad formula and normalized
  identity evidence.

### Phase 1 acceptance evidence

```text
Delivery commits: e6101445, 0c3753ee
Evidence refresh revision: e6ea7982
Run directory: lib/rol-conversion/runs/phase1-e6ea7982
Run ID: rol-phase1-1c287d5073293f7c
Active records with candidate or explicit absence: 71,680 of 71,680
Dependency closure complete: yes
Source parse complete: yes
Source aggregate semantics reconciled: yes
Persistent bindings captured: yes
Capability dispositions owned: 89 of 89
Target parse complete: no, preserved pre-existing baseline
```

### Phase 2 delivered

- Added verified Phase 2 action planning with canonical identity maps, capability rows,
  apply-oriented change plans, schemas, collision hard failures, deterministic reserved
  allocations, and zero live target writes.
- Assigned a final action to all 71,680 active records: 68,146 `ADD`, 2,458 `KEEP`,
  1,070 `MERGE`, zero `PATCH`, and six `EXCLUDE` actions.
- Preserved ambiguous target candidates and allocated them in the reserved range rather
  than patching a possible lineage match. Confirmed lineage was limited to the three
  packages meeting the measured evidence threshold.

### Phase 2 acceptance evidence

```text
Delivery commit: 0c3753ee
Evidence refresh revision: e6ea7982
Run directory: lib/rol-conversion/runs/phase2-e6ea7982
Run ID: rol-phase2-39a6d6d253950dff
Discovery input: rol-phase1-1c287d5073293f7c
Records planned: 71,680 of 71,680
Final actions: 71,680 of 71,680
Emission-ready KEEP/EXCLUDE records: 2,464
Live target writes: 0
```

### Phase 3 delivered

- Added `wtool rol-skeleton` and exercised the complete inventory, parse,
  reconciliation, identity, bundle, staging, validation, and apply path for the
  smallest confirmed prior-lineage zone slice.
- Selected source zone 960 from the Jotun package and preserved confirmed target zone
  1960 with a real hash-guarded `KEEP`. No `ADD` was needed or attempted.
- Copied the complete development world into isolated staging, validated the selected
  package with the same target grammar configuration, and proved zero new findings.
- Applied the action twice. Both applies performed zero writes, retained the exact
  target file hash, and left the 3,808-file authoritative tree hash unchanged.
- Repeated the operational run independently with the same controlled creation time.
  Both manifests and all 12 hashed artifacts were byte-identical and produced the same
  deterministic run ID.

### Phase 3 acceptance evidence

```text
Delivery commit: a5419818
Run directories: lib/rol-conversion/runs/phase3-a5419818-a
                 lib/rol-conversion/runs/phase3-a5419818-b
Run ID: rol-phase3-11336f1832d8765c
Plan input: rol-phase2-befefd85d1ceee35
Action: KEEP zon/1960.zon
First apply writes: 0; no-op: yes
Second apply writes: 0; no-op: yes
Authoritative target tree unchanged: yes
Staged validation equals target validation: yes
New validation findings: 0
Independent-run artifacts byte-identical: 12 of 12
Builder-owned files written: 0
Python suite: 198 tests passed
Documentation findings: 0 errors, 0 warnings, 0 info
```

The selected target baseline remains parse-incomplete with five pre-existing errors
and 112 warnings. Those findings occur equally in the authoritative and staged results;
the walking skeleton does not waive, hide, or add to them.

### Active boundary after this milestone

The active plans now begin at Phase 4. No representative vertical pilot, runtime
capability rollout, special-procedure port, broad corpus batch, or live world mutation
was started as part of Phases 1-3.

## 2026-08-11 - Phase 0 baseline evidence foundation

Status: Completed milestone; typed dependency closure subsequently completed in Phase 1

### Delivered

- Added `wtool rol-baseline` and versioned `rol-conversion-policy-1` behavior,
  identity, staging, bundle, apply, encoding, and quest-approval rules.
- Added deterministic target inventory for all eight indexed Luminari world kinds,
  including index hashes, data hashes, missing index entries, and orphaned files.
- Reproduced all seven RoL source aggregates byte for byte in active manifest order.
  The implementation mirrors the source C reader's unterminated-final-line behavior
  and records every dropped fragment rather than silently normalizing it.
- Added typed collision evidence for the candidate entity and zone ranges across
  target definitions, VNUM-related source bindings, and 50 numeric database VNUM
  columns without exposing database credentials or identity.
- Captured the exact development `validate --all` result, including finding
  identities and incomplete parse state, instead of relying on process status.
- Added a unique ignored run directory with artifact hashes and committed source and
  target revisions. The tool never modifies the RoL source or target world.
- Added permanent CLI/testing/changelog documentation, synchronized Autotools and
  CMake manifests, and bumped `wtool` to 0.4.0.

### Acceptance evidence

```text
Committed target revision: 1619ccd869934b8e0eeadd1effca91f3088e347c
Source revision: 3f57e70c45327335187fd123c991388e8bab2661
Run ID: rol-phase0-02a84b2da28503c1
Target files: 3,738 total; 3,352 indexed; 386 orphaned; 0 missing
Source aggregates: 7 of 7 byte-identical
Candidate entity range 2000000-2999999: reserved; 0 collisions
Candidate zone range 20000-29999: reserved; 0 collisions
Database evidence: 50 numeric VNUM columns checked; 0 collision rows
Baseline findings: 3,849 errors; 37,413 warnings; 206 info; parse incomplete
Python suite: 183 tests passed
make test-world-tools: passed
cmake --build build --target test-world-tools: passed
Documentation findings: 0 errors, 0 warnings, 0 info
```

The 386 orphaned target files are pre-existing builder-owned data not named by the
normal indexes; the baseline does not alter them. The incomplete parse state and
41,468 findings are likewise a frozen pre-existing baseline, not findings introduced
by the evidence command.

The exact source-builder behavior drops three unterminated tails: 42 bytes in
`bloodtusk.soc`, 5 bytes in `swift.obj`, and 1 byte in `derro.qst`. Their paths,
sizes, and hashes are retained in the ignored run artifact.

### Reproduction

```sh
python3 scripts/world/wtool.py --world-root lib/world rol-baseline \
  --source-root EXAMPLE/RealmsOfLuminari \
  --output-dir lib/rol-conversion/runs/phase0-<revision> \
  --database-config lib/mysql_config \
  --created-at 2026-08-11T00:00:00+03:00
```

The output directory must be unique. Phase 1 subsequently parsed and typed every active
reference and source/runtime binding, satisfying the Phase 0 dependency-closure gate.

### Delivery commit

- `1619ccd8` - policy, baseline bundle generator, typed collision evidence, tests,
  permanent documentation, and build integration

## 2026-08-11 - Deterministic source-list inventory

Status: Completed

### Delivered

- Added `wtool rol-inventory`, using the shared byte-preserving source reader to
  reproduce the selection behavior in the RoL `build_areas.c` implementation.
- Parsed `AREA`, `AREA.mobobj`, `SHOP`, and `QUEST` with their traced column-zero
  comment and first-token basename rules.
- Enumerated the seven `zon`, `wld`, `mob`, `obj`, `shp`, `qst`, and `soc` source
  kinds with stable paths, sizes, SHA-256 hashes, manifest source lines, and zone
  header identities.
- Classified active, disabled, unlisted, missing-companion, and multi-zone inputs.
  Active companion-only object and quest data remains included even when its
  basename has no physical `.zon`.
- Added deterministic human and schema-versioned JSON output without timestamps,
  modification times, or unstable directory ordering.
- Added explicit status-2 errors with source lines for blank active manifest rows,
  unsafe or non-ASCII basenames, overlong rows and names, duplicate active entries,
  and non-column-zero asterisks that the source loader would not treat as comments.
- Added valid and malformed fixtures, unit and live-corpus acceptance tests,
  permanent CLI/testing documentation, and synchronized `Makefile.am` and
  `CMakeLists.txt` world-tool manifests.
- Bumped `wtool` to version 0.3.0 while retaining schema version 1 for existing
  validation output and assigning schema version 1 to the new inventory payload.

### Acceptance evidence

```text
Active zone scope: 252 files, 255 records
Excluded zone files: 30 disabled, 2 unlisted
Active basenames without .zon: 6
Active multi-zone files: 2
Included active companion-only files: 9
Python suite: 178 tests passed
make test-world-tools: passed
cmake --build build --target test-world-tools: passed
Autotools/CMake world-tool source lists: identical, 114 paths each
Documentation findings: 0 errors, 0 warnings, 0 info
```

The six active basenames without their own `.zon` are `foggy_woods2`,
`god_items`, `northern_highroad2`, `northern_highroad3`, `quest_1`, and
`quest_2`. The multi-zone inputs are `foggy_woods.zon`, containing records 900
and 901, and `northern_highroad.zon`, containing records 902, 903, and 904.

Repeated live-corpus runs produced byte-identical human and JSON output. The
source tree hash remained unchanged before and after inventory. The malformed
fixture returned status 2, emitted no standard output, and reported the exact
offending manifest lines.

### Delivery commits

- `56af654d` - core command, classifications, fixtures, tests, and build manifests
- `380eb353` - permanent documentation and verification coverage
- `e94a9a07` - final handoff record

### Subsequent project boundary

This milestone inventories source build-list membership. The later Phase 0 baseline
milestone completed target inventory, diagnostics, aggregate regression, range
reservation, and policy versioning. Phases 1-3 subsequently completed grammar-token
inventory, typed dependency closure, lineage reconciliation, deterministic planning,
and the no-clobber walking skeleton; Phase 4 remains active in the two plans.
