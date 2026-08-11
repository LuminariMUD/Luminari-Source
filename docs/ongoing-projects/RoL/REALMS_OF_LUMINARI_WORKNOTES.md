# Realms of Luminari Conversion Worknotes

- Updated: 2026-08-11
- Environment: development
- Branch: `master`
- Current task: Remaining Phase 4 capability, bundling, validation, and reforecast work
- Completed milestone record: [RoL-Changelog.md](RoL-Changelog.md)

## Current committed checkpoint

Phases 0-3 are implemented, committed, and pushed:

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
```

The authoritative ignored runs are:

```text
Phase 0: lib/rol-conversion/runs/phase0-1619ccd8
         rol-phase0-02a84b2da28503c1
Phase 1: lib/rol-conversion/runs/phase1-0c3753ee
         rol-phase1-d1957e3cf91b8aa2
Phase 2: lib/rol-conversion/runs/phase2-0c3753ee
         rol-phase2-befefd85d1ceee35
Phase 3: lib/rol-conversion/runs/phase3-a5419818-a
         lib/rol-conversion/runs/phase3-a5419818-b
         rol-phase3-11336f1832d8765c
Phase 4 selection (superseded numeric evidence):
                   lib/rol-conversion/runs/phase4-select-fe523532
                   rol-phase4-select-821d842548312f9f
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
- Pilot room/mobile/object/zone emitters and reset compatibility are implemented. The
  focused conversion/parser suite passes 44 tests; the production CuTest suite passes
  598 tests; and `make install` leaves no root-level `circle` artifact.
- The source reset oracle no longer treats numeric comment text as reset arguments.
  Consequently, the prior Phase 4 selection run remains evidence for the package lock
  but its numeric reset/reference totals must be regenerated before further use.

## Immediate next actions

The locked selection is `swamp_two`, `hulburg`, `muspel`, `theswamp`, and `cemetery`.
It covers all five SOC modes, all five special SOC action codes, all three custom
`F/T/X` reset families, uncommon object/room extensions, a confirmed-lineage
settlement, and a compact conventional-reset oracle. Regenerate its source-oracle and
selection totals after `72c2ef24` before citing numeric evidence.

1. Regenerate Phase 1, Phase 2, and pilot-selection evidence with the corrected parser.
2. Implement only the shop, quest, SOC, and special-procedure capabilities that block
   these five packages.
3. Generate and stage deterministic pilot bundles with explicit record actions.
4. Run source-oracle, structural, reset-observation, walkthrough, reference, and
   record-action evidence for each pilot.
5. Reforecast Phases 5-8 from measured target reuse, ambiguous matches, runtime gaps,
   and review throughput.

Do not start broad Phase 5+ capability or corpus work before the Phase 4 pilot and its
evidence-based reforecast are complete.
