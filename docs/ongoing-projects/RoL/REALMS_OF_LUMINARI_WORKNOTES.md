# Realms of Luminari Conversion Worknotes

- Updated: 2026-08-12
- Environment: development
- Branch: `master`
- Current task: Phase 6 special-procedure reconciliation
- Completed milestone record: [RoL-Changelog.md](RoL-Changelog.md)
- Phase 4 manual test matrix: [PHASE4_MANUAL_TESTING.md](PHASE4_MANUAL_TESTING.md)

## Current committed checkpoint

Phases 0-5 and the listed Phase 6 checkpoints are implemented, committed, and pushed:

```text
Phase 0 baseline commit: 1619ccd8
Phase 0 checkpoint commit: 57f29005
Phase 1 grammar commit: e6101445
Phase 1/2 discovery and planning commit: 0c3753ee
Phase 3 walking-skeleton commit: a5419818
Phase 4 selection commit: fe523532
Phase 4 selection checkpoint commit: a0b54009
Phase 4 payload preservation commit: 0e869fda
Phase 4 record transform commit: 3a6b7602
Phase 4 reset compatibility commit: 72c2ef24
Phase 4 shop conversion commit: 10a30cf2
Phase 4 quest conversion commit: d5609b1c
Phase 4 SOC compiler commit: 56393f8c
Phase 4 native special commit: 8136c71b
Phase 4 special archive commit: aeaf79d2
Phase 4 complete pilot build commit: 694cf84f
Phase 4 runtime validation commit: 3fe7015f
Phase 5 quest/SOC compatibility commit: 7bbfba3d
Phase 5 rare room compatibility commit: e100bdff
Phase 5 object-trap compatibility commit: 6a1ddb6d
Phase 5 full-corpus audit commit: 51b7bd13
Phase 5 room/zone/affect compatibility commit: f0fb9d8f
Phase 5 object-property compatibility commit: ca037b15
Phase 5 object-apply/affect compatibility commit: f7eabca4
Phase 5 mobile-action compatibility commit: 1f6020de
Phase 5 reset mobile-chain compatibility commit: afeea9d7
Phase 5 exit-trap compatibility commit: c647c5f4
Phase 5 shop compatibility commits: ec1a8cd8, fe38a56e
Phase 6 inventory/shared-service commit: 368adc90
Phase 6 shared-mobile commit: 960f5602
Phase 6 implicit-race evidence commit: ae867c47
```

The authoritative ignored runs are:

```text
Phase 0: lib/rol-conversion/runs/phase0-1619ccd8
         rol-phase0-02a84b2da28503c1
Phase 1: lib/rol-conversion/runs/phase1-e6ea7982
         rol-phase1-1c287d5073293f7c
Phase 2: lib/rol-conversion/runs/phase2-e6ea7982
         rol-phase2-39a6d6d253950dff
Phase 3: lib/rol-conversion/runs/phase3-a5419818-a
         lib/rol-conversion/runs/phase3-a5419818-b
         rol-phase3-11336f1832d8765c
Phase 4 selection: lib/rol-conversion/runs/phase4-select-e6ea7982
                   rol-phase4-select-6f7ae16e5df665ec
Phase 4 build: lib/rol-conversion/runs/phase4-build-e6ea7982
               rol-phase4-build-a2c341dfaa743b26
Phase 5 room/zone/affect audit: lib/rol-conversion/runs/phase5-room-zone-df35cb1f
                                rol-phase5-audit-719a67acc4cb6b01
Phase 5 room/zone/affect pilot: lib/rol-conversion/runs/phase5-room-flags-f0fb9d8f
                                rol-phase4-build-e5d61111edbfcd9d
Phase 5 object-property audit: lib/rol-conversion/runs/phase5-object-flags-e5b998bd
                                 rol-phase5-audit-fb713f798161a78b
Phase 5 object-property pilot: lib/rol-conversion/runs/phase5-object-flags-e5b998bd-pilot
                                 rol-phase4-build-4e6f5f9a132e06cc
Phase 5 object-apply/affect audit: lib/rol-conversion/runs/phase5-object-applies-affects-20260812
                                    rol-phase5-audit-ae0fdf51ee3707ee
Phase 5 object-apply/affect pilot: lib/rol-conversion/runs/phase5-object-applies-affects-20260812-pilot
                                    rol-phase4-build-87b9c7b1b8e214bd
Phase 5 mobile-action audit: lib/rol-conversion/runs/phase5-mobile-actions-20260812-v2
                               rol-phase5-audit-fc1c1ddc402d3800
Phase 5 mobile-action pilot: lib/rol-conversion/runs/phase5-mobile-actions-20260812-pilot
                               rol-phase4-build-f11ba7e2f3909645
Phase 5 reset-chain audit: lib/rol-conversion/runs/phase5-reset-chain-20260812-audit
                           rol-phase5-audit-ed84cba825215e4f
Phase 5 reset-chain pilot: lib/rol-conversion/runs/phase5-reset-chain-20260812-pilot
                           rol-phase4-build-0036becbb939e3ad
Phase 5 exit-trap audit: lib/rol-conversion/runs/phase5-exit-traps-20260812-audit
                         rol-phase5-audit-c6c7050ef434f7b8
Phase 5 exit-trap pilot: lib/rol-conversion/runs/phase5-exit-traps-20260812-pilot
                         rol-phase4-build-7e8fa263dff52098
Phase 5 shop/final audit: lib/rol-conversion/runs/phase5-shop-20260812-audit
                           rol-phase5-audit-3fb8de2d9afd067b
Phase 5 shop/final pilot: lib/rol-conversion/runs/phase5-shop-20260812-pilot
                           rol-phase4-build-35c9c879af63b8d1
Phase 6 special reconciliation: lib/rol-conversion/runs/phase6-special-20260812-inventory-v3
                                rol-phase6-special-7e0556903754990d
Phase 6 shared mobile: lib/rol-conversion/runs/phase6-special-20260812-shared-mobile
                       rol-phase6-special-0f4f1274d95a2941
Phase 6 implicit race: lib/rol-conversion/runs/phase6-special-20260812-race-composition
                       rol-phase6-special-caf72346b7ac8119
Policy:  rol-conversion-policy-1
```

## Current evidence state

- Phase 0 reproduces all seven source aggregates byte for byte, reserves the candidate
  entity and zone ranges across world, code, and 50 database columns, and freezes the
  41,468-finding pre-existing target baseline.
- Phase 1 parses all 71,680 active records and 420,124 directives, types 355,042
  references, owns the full active dependency closure, inventories runtime and
  persistent bindings, and gives all 89 capabilities a disposition.
- Phase 2 gives every active record a final action: 68,146 `ADD`, 2,458 `KEEP`, 1,070
  `MERGE`, and six `EXCLUDE`, with zero live target writes.
- Phase 3 preserves Jotun source zone 960 as target zone 1960 through a real `KEEP`,
  stages the complete target, validates equivalence, applies twice with zero writes,
  and proves the authoritative target tree is unchanged.
- The two controlled Phase 3 runs have identical manifests, all 12 hashed artifacts
  are byte-identical, and both produce run ID `rol-phase3-11336f1832d8765c`.
- Phase 4 disposes all 3,001 selected actions, all 245 selected SOC records, and all 91
  selected special bindings. It emits 25 files and 194 DG triggers, patches 73 preserved
  mobiles, appends 14 shops, and performs zero implicit overwrites or live target writes.
- The generated overlay parses completely and adds zero active staged errors. The 79
  active staged errors are inherited target-baseline findings; they remain repair work
  for the relevant Phase 7 batches and are not waived.
- Reset-reference and scripted walkthrough evidence passes for all five pilot zones and
  all 1,160 selected rooms. The isolated test-database boot enters the game loop,
  observes eligible resets for zones 1591 and 20586, and terminates normally with no
  pilot-related spell, reference, reset, trigger, extraction, or `SYSERR` diagnostics.
- The world-tool suite passes 254 tests; the production-linked CuTest suite passes 620;
  `make install` succeeds and leaves no root-level `circle` artifact.
- The measured remaining forecast is 96-156 sessions: Phase 6 is 48-80, Phase 7 is
  42-66, and Phase 8 is 6-10.
- Phase 5 now handles argument-free quest attacks, configured experience, signed
  quest-point deltas, all 29 active spell/skill reward identities, and explicit SOC
  `LISTDONE` termination. Existing HLQuest persisted command indexes remain stable.
- Phase 5 now handles all three active room level ranges. The one active fall-chance
  row and 36 obsolete mana rows are source-inert and are omitted with explicit
  diagnostics after source runtime tracing.
- Phase 5 now converts all 29 valid active object-trap payloads into persistent target
  object values and runtime behavior. Four empty source rows are omitted explicitly.
  The five pilots contain no active object traps; the trap-only checkpoint's restage
  was byte-identical with run ID `rol-phase4-build-1dc8a681fa1595d5`.
- The current Phase 5 full-corpus audit emits all 69,920 convertible active records,
  totaling 42,089,791 bytes, with zero transform exceptions and zero writes. Complete
  ownership of room, zone, sector, mobile action, object type, object wear, object extra,
  object apply, and mobile/object affect values reduces unmapped symbolic observations
  from 26,006 to zero.
- Nine appended object flags preserve RoL identify, summon, sleep, charm, two-handed,
  race-restriction, whole-body, and whole-head behavior. Source `NOSHOW` maps to hidden;
  source-inert `DARK` is explicitly omitted. The refreshed five-zone pilot exposes five
  of these behavior families for manual testing.
- Source agility and power applies now map to Dexterity and Wisdom; maximum-stat applies
  map to their target attributes, and the four active race-factor tuples use bounded
  fixed-stat equivalents. Karma and Luck fields are explicitly source-only. Corrected
  object parsing prevents post-extension numeric payload from becoming false affects.
- RoL slow poison and docile behavior now use extensible target secondary affects;
  meditation uses the target rapid-preparation affect. Transient/inert prototype
  affects and three malformed source rows have explicit, reproducible dispositions.
- All 15,243 active mobile-action observations now have an explicit disposition. New
  compatibility flags preserve nice-thief, stay-sector, delayed-hunter, archer,
  independent class-role, and race-aggression behavior; protector expands to existing
  helper/listener behavior. `ACT_SPEC` is explicitly owned by Phase 6 binding
  reconciliation, while relationship-only or source-inert flags are logged omissions.
- Converted zones now carry `RoL-Reset-Compat`: their `E` and `G` chains remain bound
  to the most recent successful `M`, matching the source even after an intermediate
  equipment failure. Native zones retain result-offset behavior, and the three active
  non-boolean source dependencies are normalized to the source's actual boolean rule.
- All 34 valid active exit-trap payloads now use a persistent target room record and
  runtime adapter. Blade, poison, rock, fire, lightning, random, falling, area, boot
  load, detection, disabling, open/pick triggering, and reset rearming are preserved.
  The one malformed source row is excluded with a source-located diagnostic.
- All 1,467 active quest item-reward directions carry one fixed object VNUM. The source
  engine's optional random-range upper bound is unused by active content and blocks no
  record.
- Phase 5 is complete. The final audit emits all 453 active shops, preserves fixed and
  roaming operation, exact source price formulas, authored customer restrictions,
  adverse `CHEATS` pricing, and the converted `CASTING` policy. It reports zero generic
  capability gaps, zero unmapped symbolic observations, zero transform exceptions, and
  zero live target writes.
- The Phase 6 inventory accounts for 1,234 active direct bindings across 605 source
  handlers and locates all 605 source definitions. The current checkpoint resolves 231
  bindings and 43 handlers, leaving 1,003 bindings and 562 handlers. Shared service
  reuse accounts for 72 bindings; source-inert dump and cityguard callbacks account for
  22; bounded corpse-devourer, poison-bite, and thief adapters account for 29.
- The independent `ACT_SPEC` cross-check resolves 462 of 848 records: source boot
  clears 444 unbound flags and 18 directly assigned records are resolved. The remaining
  386 are 343 direct-only mobiles, 10 mobiles combining direct and implicit race
  procedures, and 33 implicit-race-only records.
- Source boot attaches race procedures to 247 active prototypes independently of the
  authored `ACT_SPEC` flag: 134 demons, 101 devils, and 12 umber hulks. Twenty-three
  also have direct assignments and 224 are implicit-only. All 247 remain pending a
  composition-safe runtime port; the earlier 33 count covered only the `ACT_SPEC`
  subset.
- The 804 record-specific reference gaps remain owned by Phase 7 dependency batches.

## Immediate next actions

1. Reconcile the remaining 1,003 direct bindings by shared behavior family and
   consuming package, while porting the 247 implicit race procedures through a path
   that composes with direct assignments; continue with the next high-reuse families
   and reuse current target procedures before adapting or porting.
2. Preserve record-specific missing-reference repairs for their Phase 7
   dependency-closure batches.
3. Preserve the six locked malformed record exclusions as explicit, logged
   smallest-unit exclusions.
4. Regenerate the special-binding inventory after each shared-family checkpoint and
   repeat structural, syntax-boot, isolated behavioral, reset, and walkthrough gates.
