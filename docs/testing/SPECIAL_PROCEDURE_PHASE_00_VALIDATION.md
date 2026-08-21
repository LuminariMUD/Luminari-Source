# Special Procedure Phase 00 Validation

## Purpose

This document is the durable acceptance and test-ownership matrix for Phase 00: Registry Safety and
Observability. It maps the phase exit criteria to production source, dedicated production-linked
tests, builder/operator documentation, and reproducible validation commands.

Phase 00 was fully completed and verified on 2026-08-07. This is the durable closeout record;
per-session workflow artifacts were removed after their enduring contracts and evidence were
consolidated into maintained project documentation. Git history retains the original execution logs.

## Delivered Boundary

Phase 00 delivers a safe control plane around the existing `SPECIAL` callback ABI:

- 28 immutable canonical definitions and 29 compatibility lookup names, including the explicit
  `Guildmaster` alias for canonical `Guild`;
- complete owner, event, prototype-flag, placement, binding-source, visibility, category, and
  description metadata validated before world parsing;
- owner-filtered mobile, object, and room OLC selection with explicit replace and clear behavior;
- prototype-owned authored records that preserve exact aliases, unknown names, incompatible names,
  source locations, and persistence intent;
- authored-first mobile, object, and room writers with fresh production-loader round trips;
- prototype-owned effective history for world, parser-hook, legacy-assignment, shop, and quest
  writes, including wrapper secondaries and final callback diagnostics; and
- order-independent moving-room `M` plus named room `Z` rejection at load, edit, and write
  boundaries.

The callback pointer remains runtime authority. Phase 00 does not add event gateways, typed
contexts, invalidation results, declarative assignment tables, content extraction, shared combat or
cooldown mechanics, typed-handler conversion, or general multiple-handler composition.

## Phase Exit Criteria

| Phase 00 Criterion | Implementation Evidence | Production-Linked Evidence |
|--------------------|-------------------------|----------------------------|
| Phase 00 is fully complete and verified. | The delivered control plane is identified below by production owner. | All 78 dedicated tests and every reproducible closeout gate passed on 2026-08-07. |
| Every verified invocation category and registry compatibility behavior is characterized. | Existing callers in `src/interpreter.c`, `src/mob/mob_act.c`, `src/comm.c`, `src/combat/`, `src/obj/act.item.c`, shops, quests, and moving rooms remain unchanged. | `test_spec_registry_persistence.c` (10), `test_spec_command_pulse.c` (13), and `test_spec_combat_secondary.c` (14). |
| Every definition has valid identity and complete metadata. | `src/spec/spec_registry.c` immutable definition table. | `Test_spec_registry_production_metadata_validates`, `Test_spec_registry_canonical_inventory_and_metadata`, and event/owner contract tests in `test_spec_registry_validation.c`. |
| Invalid metadata fails before world parsing and accessors are bounds-safe. | `spec_registry_boot_validate()` precedes `boot_world()` in `src/db.c`; registry accessors validate indices and one-bit masks. | `Test_spec_registry_accessors_reject_extreme_inputs`, all malformed-definition tests, and `Test_spec_registry_boot_validation_precedes_world_parsing`. |
| Medit, oedit, and redit list only compatible definitions and explain prerequisites. | `src/olc/spec_menu.c` shared filtered menu and three editor integrations. | All seven `test_spec_owner_aware_olc.c` tests, including exact inventories, menu metadata, strict selection, and activation-flag preservation. |
| Known, aliased, incompatible, and unresolved authored identities survive defined load/edit/save actions. | `src/spec/spec_binding.c`, three loaders/editors, and authored-first writers. | Seven `test_spec_authored_bindings.c` tests plus seven `test_spec_binding_round_trip.c` tests. |
| Effective sources, collisions, and wrapper secondaries are diagnosable without precedence drift. | `src/spec/spec_effective_binding.c`, boot instrumentation, shop and quest contributions, and startup report. | Seven `test_spec_effective_binding.c` tests plus shop/quest composition tests in `test_spec_combat_secondary.c`. |
| Moving-room `M` plus room `Z` is rejected safely. | Production parser, REdit, and whole-zone writer preflight. | `TestSpecEffectiveBindingRejectsBothRoomLoadOrders` and `TestSpecEffectiveBindingRejectsMovingRoomOlcAndWriter`. |
| Full tests/build/install pass and documentation matches current behavior. | Dual manifests, SQL help migration/verifier, builder/developer/architecture/testing docs. | Session 09 `make test`, `make install`, independent CMake/CTest, SQL, encoding, link, and integrity gates. |

## Dedicated Suite Inventory

The Phase 00 inventory is derived from functions whose names begin with `Test` in these eight
production-linked sources. The shared `test_spec_fixtures.c` supplies fixtures and is not counted as
a test owner.

| Test Source | Tests | Contract Owner |
|-------------|------:|----------------|
| `unittests/CuTest/test_spec_registry_persistence.c` | 10 | Legacy name inventory, alias/reverse lookup, accessor bounds, world loaders, source inventory, and baseline OLC. |
| `unittests/CuTest/test_spec_command_pulse.c` | 13 | Command traversal/stop, `no_specials`, mobile activity, object auto-pulse fallback, moving rooms, and heartbeat order. |
| `unittests/CuTest/test_spec_combat_secondary.c` | 14 | Identification, hit/reaction/maneuver/charge tokens, ignored returns, combat schedule, and shop/quest nesting. |
| `unittests/CuTest/test_spec_registry_validation.c` | 13 | Production metadata, canonical/alias integrity, events, masks, bounds, malformed definitions, and pre-world boot failure. |
| `unittests/CuTest/test_spec_owner_aware_olc.c` | 7 | Exact owner views, strict selection, descriptions/prerequisites, control paths, and unchanged activation flags. |
| `unittests/CuTest/test_spec_authored_bindings.c` | 7 | Transactional ownership, source compatibility, diagnostics, canonical/alias/unknown/incompatible loaders, and OLC lifetime. |
| `unittests/CuTest/test_spec_binding_round_trip.c` | 7 | Persistence accessor plus alias, unknown, incompatible, override, explicit selection/clear, and fallback reloads. |
| `unittests/CuTest/test_spec_effective_binding.c` | 7 | Outcomes, bounds, copy/escaping, real loaders, precedence/secondaries, mode reporting, and room conflict boundaries. |
| **Total** | **78** | Dedicated Phase 00 production-linked tests. |

All eight test owners and the fixture source appear twice in `Makefile.am` (`cutest_SOURCES` and
`cutest_test_files`) and once in `CMakeLists.txt` (`CUTEST_TEST_SOURCES`). The production sources
`src/spec/spec_registry.c`, `src/spec/spec_binding.c`, `src/spec/spec_effective_binding.c`, and
`src/olc/spec_menu.c` appear once in each server build manifest.

`Test_spec_world_binding_source_inventory` scans the ignored development world during local
acceptance and requires every discovered name to resolve for its owner and permit a world binding.
Clean CI jobs point the same scanner at the tracked five-binding snapshot in
`unittests/CuTest/fixtures/spec_world_inventory/`; this preserves the exact verified Phase 00
inventory without adding builder-owned world files to source control.

## Required Coverage Matrix

### Phase 00 Coverage

| Required Area | Exact Test Evidence |
|---------------|---------------------|
| Canonical and alias uniqueness, case-insensitive lookup, and extreme bounds | `Test_spec_registry_current_name_inventory`, `Test_spec_registry_guild_alias_and_reverse_lookup`, `Test_spec_registry_legacy_accessor_boundaries`, registry collision and extreme-input tests. |
| Definition validation before world parsing | `Test_spec_registry_production_metadata_validates`, every malformed-definition test, and `Test_spec_registry_boot_validation_precedes_world_parsing`. |
| Owner, event, flag, and placement compatibility in all editors | All seven `test_spec_owner_aware_olc.c` tests. |
| Builder descriptions and known/unresolved OLC round trips | Owner menu rendering/control tests, all authored-loader tests, and all binding round-trip tests. |
| OLC save after a hard-coded override without authored promotion | Alias, unknown, and incompatible override round-trip tests. |
| Legacy world loading and canonical save behavior | `Test_spec_world_binding_loaders_resolve_known_names`, `TestSpecAuthoredBindingCanonicalLoaders`, persistence-name and explicit-selection round trips. |
| Effective precedence across all five contribution sources | `TestSpecEffectiveBindingProductionLoaders`, `TestSpecEffectiveBindingProductionPrecedenceAndSecondaries`, and `TestSpecEffectiveBindingReportFollowsNoSpecialsAssignmentGate`. |
| Exact magic strings and empty-argument calls | The 13 command/pulse tests and 14 combat/secondary tests cover every characterized legacy event token and actor/return shape. |
| Room, equipped, carried, mobile, and room-object command order | `Test_spec_command_traverses_all_owners_in_order` and stop-at-each-owner coverage. |
| `MOB_SPEC` and `ITEM_AUTOPROC` gates | Mobile activity/combat activation tests, object auto-pulse gate tests, and OLC no-flag-mutation test. |
| Normal and `-s` path behavior | Command bypass, mobile suppression, object/moving-room unaffected paths, boot assignment gate, and effective-report mode tests. |
| Worn/carried auto-proc fallback with null actor and return variants | Worn-once, null-then-carrier, gate, and return-path tests in `test_spec_command_pulse.c`. |
| Notification-only returns | Identification, weapon, defense, maneuver, charge, and combat-turn tests in `test_spec_combat_secondary.c`. |
| Moving-room payload and `M` plus `Z` rejection | Moving-room timer/payload/schedule tests plus both parser orders and OLC/writer rejection tests. |
| Shop and quest secondary behavior | Five secondary/composition tests plus effective wrapper-secondary diagnostics. |

### Later-Phase Coverage

The master project matrix also contains requirements that Phase 00 deliberately does not claim.
They remain mandatory when their owning implementation phase begins:

| Deferred Area | Owning Phase |
|---------------|--------------|
| Exact equipped-object pointer identity and explicit owner/actor/target invalidation | Phase 01 gateway compatibility and Phase 04 validation helpers. |
| Target death, pending extraction, immediate owner extraction, and multi-target successor safety | Phase 01 gateways. |
| Cooldown units, bounds, persistence, reboot, and spend timing | Phase 04 narrow shared mechanics. |
| Recursive extra-attack suppression and safe combat-result contracts | Phase 04 narrow shared mechanics. |
| Affect source namespaces, removal, stacking groups, and rejection | Phase 04 narrow shared mechanics. |
| Multiple-handler ordering and versioned persistence | Conditional Phase 06 composition, only with an approved consumer. |

## Documentation Contract

| Audience | Authority |
|----------|-----------|
| Builder/staff workflow | `docs/guides/OLC_SpecProcs.md` and database tag `spec-proc` from `sql/components/help_specproc_entries.sql`. |
| Developer API and extension rules | `docs/guides/DEVELOPER_GUIDE_AND_API.md`. |
| Boot, state, ownership, and compatibility architecture | `docs/systems/CORE_SERVER_ARCHITECTURE.md`. |
| Database-first help maintenance | `docs/systems/HELP_SYSTEM.md`. |
| Test execution | `docs/guides/TESTING_GUIDE.md` and this matrix. |

All maintained documentation is ASCII-compatible UTF-8 with Unix LF endings. Current behavior is
written in present tense; later phases are labeled proposed or deferred.

## Security and Privacy Assessment

**Assessment date:** 2026-08-07

**Scope:** Phase 00 changes only

**Result:** PASS with no open finding

This is a targeted phase assessment, not a repository-wide security or privacy audit. All six
findings opened during Phase 00 were resolved before phase closeout; no Critical, High, Medium, or
Low finding remained open.

### Privacy Scope

Phase 00 introduced no player, account, or other personal-data processing. Persisted and logged
data added by the phase is limited to static procedure identities, source locations, owner types,
prototype VNUMs, and synthetic test fixtures. Authored and effective binding diagnostics exclude
player and account values.

| Requirement | Status | Phase 00 evidence |
|-------------|--------|-------------------|
| Data collection has a documented purpose | N/A | No personal-data collection was added. |
| Consent before data storage | N/A | No personal-data storage was added. |
| Data minimization | N/A | Diagnostics contain only static world and binding metadata. |
| Deletion or erasure path | N/A | No personal-data lifecycle was added. |
| No PII in application logs | PASS | New diagnostics exclude player and account data. |
| Third-party transfers | N/A | No third-party transfer was added. |

### Dependency and CI Security

No dependency manifest or vendored dependency changed in Phase 00. CodeQL and Gitleaks were green
on the transition commits. GitHub dependency review is pull-request scoped and was therefore
skipped on the final push event; that skip is not evidence of a dependency scan failure.

### Resolved Findings

| ID | Finding | Severity | Resolution |
|----|---------|----------|------------|
| P00-S07 | Persisted procedure names allowed multiline output | Low | The persistence boundary rejects CR/LF and tests the single-line contract. |
| P00-S08 | Structured diagnostic validation gaps | Medium | Added strict bounds, control-byte escaping, truncation failure, owner/source invariants, and stable paths. |
| P00-S08B | Room conflict validation could occur after mutation | Medium | Whole-zone preflight rejects `M` plus `Z` before output creation or mover mutation. |
| P00-S09 | Validation used a reusable fixed scratch path | Medium | Replaced it with a unique validated path, quoted operations, an exit trap, and deterministic cleanup. |
| P00-P01 | CI exposed undefined bit shifts and ownership leaks | Medium | Width-correct masks and complete object/room lifecycle cleanup pass ASan, UBSan, and Valgrind. |
| P00-I01 | Output-path override freed borrowed argv memory | Medium | The override duplicates the path, releases prior owned storage, and survives graceful shutdown under ASan. |

### Continuing Guardrails

1. Preserve the production-linked characterization matrix while introducing Phase 01 gateways,
   especially successor caching and event-specific post-callback invalidation rules.
2. Keep authored names and effective diagnostics bounded, single-line, and free of player or
   account values as gateway context expands.
3. Continue isolated loopback MariaDB fixtures and fail-closed path validation for CI and
   operational tests; never point them at protected repository data or production databases.
4. Complete the approved production health install, restart, and probe before closing the
   infrastructure exception.

Environment, credential, and loopback boundaries are maintained in
[environments.md](../environments.md). The managed-service procedure and activation status are in
[deployment.md](../deployment.md), while durable migration risks remain in
[Project Considerations](../CONSIDERATIONS.md#special-procedure-architecture-refactor).

## Reproducible Validation

### Autotools And Installation

Run from the repository root:

```sh
make test
make install
test -x bin/luminari
test -L bin/luminari
test ! -e luminari
```

`make test` runs the full production-linked binary, not only the 78 dedicated tests. Installation
must immediately follow because the test path can create a root-level `luminari`.

### Independent CMake And CTest

Use a fresh directory outside the source tree:

```sh
phase00_cmake_dir=$(mktemp -d /tmp/luminari-spec-phase00-cmake-XXXXXX)
case "$phase00_cmake_dir" in
  /tmp/luminari-spec-phase00-cmake-*) ;;
  *) exit 1 ;;
esac
cleanup_phase00_cmake() {
  rm -r -- "$phase00_cmake_dir"
}
trap cleanup_phase00_cmake EXIT

cmake -S . -B "$phase00_cmake_dir" \
  -DBUILD_TESTS=ON -DCMAKE_BUILD_TYPE=Debug
cmake --build "$phase00_cmake_dir" --target cutest --parallel "$(nproc)"
ctest --test-dir "$phase00_cmake_dir" --output-on-failure
cleanup_phase00_cmake
trap - EXIT
```

The validated prefix and exit trap remove only the generated temporary directory.

### Help SQL

Run both checks below. The isolated check proves repeatable SQL behavior without changing persistent
content; the development acceptance proves that builders can retrieve the maintained topic from the
authoritative database through the running game.

For the isolated migration check, create connection-local temporary `help_entries` and
`help_keywords` tables with the required columns and unique keys in one MariaDB development
connection. Source `help_specproc_entries.sql` twice, then source
`verify_help_specproc_entries.sql`. The entry, content, required-keyword, and conflicting-keyword
checks must all return `PASS`. Temporary tables must shadow the persistent names for the entire
operation.

For persistent development acceptance, first confirm that `lib/.env` identifies a development
environment and that the configured database is the intended development database. Apply
`help_specproc_entries.sql` twice through the normal migration process, then run
`verify_help_specproc_entries.sql` against the persistent tables. All four checks must return
`PASS`. Install the tested server, reload or restart it, and exercise every authoritative keyword:

```sh
./scripts/development/dev_kohdee_login_smoke.sh --help-check \
  SPECIALS SPEC SPEC-PROC SPECIAL-PROCEDURE SPECPROC
```

Each keyword must report `PASS` and resolve to database tag `spec-proc`. A temporary-table result by
itself does not satisfy the in-game help acceptance criterion. Never run this development procedure
against production.

### Integrity And Hygiene

- Count exactly 78 dedicated `Test` functions and assert the `10,13,14,13,7,7,7,7` suite split.
- Compare Phase 00 source/test membership exactly across Automake and CMake.
- Compare every `sql/components/*.sql` basename with `ci_schema_manifest.txt` exactly once.
- Resolve every relative Markdown link changed in Session 09.
- Scan changed text for non-ASCII and CR bytes and run `git diff --check`.
- Confirm no diff in `src/campaign.h`, `src/mud_options.h`, `src/vnums.h`, `lib/.env`,
  `lib/mysql_config`, or `lib/world/`.
- Confirm the checked-in world digest is unchanged, no validation sandbox remains, `bin/luminari` is
  executable, and root `luminari` is absent.

## Acceptance Rule

Phase 00 passes only when every applicable matrix row and reproducible gate above passes with no
open review or security finding. A later-phase row is not a Phase 00 failure, but it must remain
explicitly deferred and cannot be described as delivered. Any failing current-phase row keeps the
phase open until repaired and revalidated.
