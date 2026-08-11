# Realms of Luminari Project Changelog

This file records completed milestones removed from the active
[feature-first conversion plan](REALMS_OF_LUMINARI_FEATURE_FIRST_CONVERSION_PLAN.md)
and [zone conversion scope](REALMS_OF_LUMINARI_ZONE_CONVERSION_SCOPE.md). The plans
retain only forward-looking requirements, decisions, phases, and acceptance gates.

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

The next Phase 5 compatibility pass begins with random quest item-reward `R:I`
ranges and a full-corpus dry build for symbolic values not exercised by the pilot.

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
