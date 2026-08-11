# Realms of Luminari Project Changelog

This file records completed milestones removed from the active
[feature-first conversion plan](REALMS_OF_LUMINARI_FEATURE_FIRST_CONVERSION_PLAN.md)
and [zone conversion scope](REALMS_OF_LUMINARI_ZONE_CONVERSION_SCOPE.md). The plans
retain only forward-looking requirements, decisions, phases, and acceptance gates.

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
