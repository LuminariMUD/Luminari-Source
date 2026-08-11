# Realms of Luminari Conversion Worknotes

- Updated: 2026-08-12
- Environment: development
- Branch: `master`
- Current task: Phase 5 shared capability rollout
- Completed milestone record: [RoL-Changelog.md](RoL-Changelog.md)
- Phase 4 manual test matrix: [PHASE4_MANUAL_TESTING.md](PHASE4_MANUAL_TESTING.md)

## Current committed checkpoint

Phases 0-4 are implemented, committed, and pushed:

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
Phase 5 room restage: lib/rol-conversion/runs/phase5-room-e100bdff
                      rol-phase4-build-1dc8a681fa1595d5
Phase 5 capability audit: lib/rol-conversion/runs/phase5-audit-88edf75e
                          rol-phase5-audit-13d7727804344e9a
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
- The world-tool suite passes 232 tests; the production-linked CuTest suite passes 605;
  `make install` succeeds and leaves no root-level `circle` artifact.
- The measured remaining forecast is 104-170 sessions: Phase 5 is 8-14, Phase 6 is
  48-80, Phase 7 is 42-66, and Phase 8 is 6-10.
- Phase 5 now handles argument-free quest attacks, configured experience, signed
  quest-point deltas, all 29 active spell/skill reward identities, and explicit SOC
  `LISTDONE` termination. Existing HLQuest persisted command indexes remain stable.
- Phase 5 now handles all three active room level ranges. The one active fall-chance
  row and 36 obsolete mana rows are source-inert and are omitted with explicit
  diagnostics after source runtime tracing.
- Phase 5 now converts all 29 valid active object-trap payloads into persistent target
  object values and runtime behavior. Four empty source rows are omitted explicitly.
  The five pilots contain no active object traps, so their verified restage remains
  byte-identical with run ID `rol-phase4-build-1dc8a681fa1595d5`.
- The Phase 5 full-corpus audit emits all 69,920 convertible active records, totaling
  42,075,289 bytes, with zero transform exceptions and zero writes. It inventories
  26,006 unmapped symbolic observations for the remaining compatibility passes.
- All 1,467 active quest item-reward directions carry one fixed object VNUM. The source
  engine's optional random-range upper bound is unused by active content and blocks no
  record.

## Immediate next actions

1. Resolve the measured room, mobile, object, affect, apply, item-type, wear, and sector
   symbolic gaps by traced equivalence, bounded adapters, or explicit dispositions.
2. Separate record-specific missing-reference repairs from reusable capability work and
   attach those repairs to their Phase 7 dependency-closure batches.
3. Preserve the six locked malformed record exclusions as explicit, logged
   smallest-unit exclusions.
4. Regenerate a deterministic capability-complete bundle and repeat structural,
   syntax-boot, isolated behavioral, reset, and walkthrough gates.
5. Begin Phase 6 only when no active record remains blocked by a generic shared
   capability.
