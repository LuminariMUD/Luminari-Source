# Realms of Luminari Project Changelog

This file records completed milestones removed from the active
[feature-first conversion plan](REALMS_OF_LUMINARI_FEATURE_FIRST_CONVERSION_PLAN.md)
and [zone conversion scope](REALMS_OF_LUMINARI_ZONE_CONVERSION_SCOPE.md). The plans
retain only forward-looking requirements, decisions, phases, and acceptance gates.

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

This milestone inventories source build-list membership; it does not complete
the development-target inventory, baseline diagnostics, aggregate regression
comparison, grammar-token inventory, dependency closure, lineage reconciliation,
or conversion pipeline. Those remain active in the two planning documents.
