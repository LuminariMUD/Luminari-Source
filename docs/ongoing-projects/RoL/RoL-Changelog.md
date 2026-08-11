# Realms of Luminari Project Changelog

This file records completed milestones removed from the active
[feature-first conversion plan](REALMS_OF_LUMINARI_FEATURE_FIRST_CONVERSION_PLAN.md)
and [zone conversion scope](REALMS_OF_LUMINARI_ZONE_CONVERSION_SCOPE.md). The plans
retain only forward-looking requirements, decisions, phases, and acceptance gates.

## 2026-08-11 - Phase 0 baseline evidence foundation

Status: Completed milestone; typed dependency closure remains active

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

The output directory must be unique. Phase 1 must still parse and type every active
reference and source/runtime binding before the Phase 0 dependency-closure exit gate
is satisfied.

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

### Remaining project boundary

This milestone inventories source build-list membership. The later Phase 0 baseline
milestone completed target inventory, diagnostics, aggregate regression, range
reservation, and policy versioning. Grammar-token inventory, typed dependency closure,
lineage reconciliation, and the conversion pipeline remain active in the two plans.
