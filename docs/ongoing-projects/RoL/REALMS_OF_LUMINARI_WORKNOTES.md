# Realms of Luminari Conversion Worknotes

- Updated: 2026-08-11
- Environment: development
- Branch: `master`
- Current task: Phase 1 grammar, typed dependency, binding, and lineage discovery
- Completed milestone record: [RoL-Changelog.md](RoL-Changelog.md)

## Current committed checkpoint

The Phase 0 evidence implementation is committed and pushed at `1619ccd8`.
The authoritative local baseline run is:

```text
Run ID: rol-phase0-02a84b2da28503c1
Run directory: lib/rol-conversion/runs/phase0-1619ccd8
Target revision: 1619ccd869934b8e0eeadd1effca91f3088e347c
Source revision: 3f57e70c45327335187fd123c991388e8bab2661
Policy: rol-conversion-policy-1
```

The run directory is intentionally ignored because its 17 MB validation payload and
world hashes describe builder-owned data. All six artifact hashes listed in
`run-manifest.json` were independently rechecked after generation.

## Current evidence state

- All seven source aggregates reproduce byte for byte from the active per-area files.
- The target inventory has 3,738 files: 3,352 indexed, 386 orphaned, and no missing
  normal-index entry.
- Candidate entity range `2000000-2999999` and zone range `20000-29999` have no
  typed world, VNUM-binding, or database collision. Fifty numeric database VNUM
  columns were checked.
- The target baseline has 41,468 pre-existing findings: 3,849 errors, 37,413 warnings,
  and 206 info findings. Parse completeness is false and is preserved as evidence.
- The source C aggregate builder drops unterminated tails in `bloodtusk.soc`,
  `swift.obj`, and `derro.qst`; the reconciliation report records their hashes.
- `make test-world-tools` and the CMake `test-world-tools` target pass all 183 tests.
- Documentation validation is clean.

## Remaining Phase 0 exit-gate item

Resolve the active dependency closure from grammar-aware typed references and traced
source/target bindings. This work is shared with Phase 1; disabled and unlisted content
must not enter the record ledger unless an active target dependency is independently
owned by the target world.

## Immediate next actions

1. Implement source-located typed parsers and normalized records for all seven active
   RoL kinds, preserving every observed token and malformed edge case.
2. Reconcile parser counts and build order against the exact active aggregates.
3. Extract typed references and classify missing/excluded dependencies.
4. Inventory target definitions plus code, configuration, database, and special-procedure
   bindings by type.
5. Seed lineage candidates without automatic overwrite decisions, then emit the
   record-action and capability discovery ledgers.

Do not begin Phase 4 pilot work. Phase 2 planning/bundle foundations and the single
Phase 3 walking skeleton must be accepted first.
