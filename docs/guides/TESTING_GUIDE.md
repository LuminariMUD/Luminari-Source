# LuminariMUD Testing Guide

## Overview

The enforced test path has two parts:

1. A production-linked CuTest suite that compiles and links the game sources.
2. A focused protocol parser harness that links the production
   `src/net/protocol.c` implementation with minimal socket and logging doubles.

Legacy standalone vessel, autopilot, and vehicle mirror sources have been
removed. Their historical result documents remain under `docs/testing/`.

## Production-Linked Tests

From the repository root:

```sh
make test
```

This builds `cutest` with `-DLUMINARI_CUTEST`, links the same game source files
as the server, and runs every `void Test...` function discovered in the files
listed by `cutest_test_files` in `Makefile.am`.

To build and run the executable directly:

```sh
make -j"$(nproc)" cutest
./cutest
```

The suite covers production code for bounds handling, character rename,
argument and command parsing, transport and autopilot behavior, combat,
spells and skills, MariaDB prepared statements, DG scripts, and world index
lookups.

After `make test`, always run:

```sh
make install
```

This retains the tested binary and matching symbols under its immutable
`bin/releases/<ELF-build-ID>/` directory, atomically activates `bin/circle`,
and removes the root-level `circle` artifact that the test build may leave
behind.

## Special Procedure Regression Ownership

Phase 00 registry safety and observability is owned by eight production-linked test sources plus one
shared fixture source:

- `test_spec_registry_persistence.c` - 10 registry, persistence, loader, and baseline OLC tests;
- `test_spec_command_pulse.c` - 13 command, activity, auto-pulse, moving-room, and schedule tests;
- `test_spec_combat_secondary.c` - 15 combat-token, ignored-return, shop, quest, and typed-nesting
  tests;
- `test_spec_registry_validation.c` - 13 immutable metadata, bounds, and boot-failure tests;
- `test_spec_owner_aware_olc.c` - 7 filtered-menu, description, selection, and flag tests;
- `test_spec_authored_bindings.c` - 7 owned authored-state, loader, diagnostic, and lifecycle tests;
- `test_spec_binding_round_trip.c` - 7 writer-to-loader identity and explicit-action tests; and
- `test_spec_effective_binding.c` - 8 provenance, precedence, module-boundary, mode, secondary, and
  room-safety tests.

Phase 01 adds `test_spec_dispatch.c` with 12 gateway and extraction-safety tests. Phase 02 adds
`test_spec_assign_table.c` with 11 declarative-row, owner/source validation, diagnostic, and stable
source-label tests. The exact inventory through Phase 02 is 101 dedicated `Test` functions.
Phase 04 adds nine mechanics/context tests, Phase 05 adds five typed-handler tests, Phase 06 adds
one typed-through-secondary test, and Phase 07 adds one assignment-module boundary test. The
completed Phase 00-07 inventory is 117 dedicated `Test`
functions across the files above plus `test_spec_mechanics.c` and `test_spec_typed_handlers.c`.
`test_spec_fixtures.c` is production-linked support and is not counted as a test owner. CuTest has
no per-function filter, so the supported focused development run is still the complete
production-linked executable:

```sh
make -j"$(nproc)" cutest
./cutest
```

Before Phase 00 or a later special-procedure change is released, run `make test`, immediately run
`make install`, and run the complete independent CTest matrix. The CTest pass includes the Python
world-tool consumer of `src/spec/spec_registry.c`; this protects source-inspection tooling as well
as the compiled server. Database-first `SPECIALS` help changes also require the temporary-table SQL
idempotency and verifier gate.

See [Special Procedure Phase 00 Validation](../testing/SPECIAL_PROCEDURE_PHASE_00_VALIDATION.md) for
the requirement-to-test map, exact manifest contract, SQL procedure, and integrity checks, and
[Special Procedure Phase 01 Validation](../testing/SPECIAL_PROCEDURE_PHASE_01_VALIDATION.md) for the
gateway translation, flow, and extraction-safety coverage, and
[Special Procedure Phase 02 Validation](../testing/SPECIAL_PROCEDURE_PHASE_02_VALIDATION.md) for the
declarative-assignment inventory, binding-chain diagnostics, and help verification,
[Special Procedure Phase 03 Validation](../testing/SPECIAL_PROCEDURE_PHASE_03_VALIDATION.md) for
content extraction,
[Special Procedure Phase 04 Validation](../testing/SPECIAL_PROCEDURE_PHASE_04_VALIDATION.md) for
shared mechanics,
[Special Procedure Phase 05 Validation](../testing/SPECIAL_PROCEDURE_PHASE_05_VALIDATION.md) for
typed handlers, and
[Special Procedure Phase 06 Validation](../testing/SPECIAL_PROCEDURE_PHASE_06_VALIDATION.md) for the
composition/lifecycle audit and final compatibility boundary, and
[Special Procedure Phase 07 Validation](../testing/SPECIAL_PROCEDURE_PHASE_07_VALIDATION.md) for
assignment ownership, direct-header boundaries, exact manifest membership, and final source
consolidation.

The manifest parity gate compares all compiled C paths, not incidental header listings. The current
inventory is 288 production C sources in both `circle_SOURCES` and `SRC_C_FILES`, plus the same 41
test-owner sources in `cutest_SOURCES`, `cutest_test_files`, and `CUTEST_TEST_SOURCES`.

## Bardic Performance Regression Ownership

`unittests/CuTest/test_bardic_performance.c` is the production-linked owner for
the base performance engine and its Spellsinger and Warchanter integrations.
It covers both performance slots, command and action transitions, lifecycle
cleanup, all thirteen base performances, source-owned refresh, duration and
target defenses, affect batching and bounded `AFFECTS` serialization, spell
scope, group auras, and perk damage/save direction.

Behavior changes in `src/bardic_performance.c`, Bard performance registrations,
or performance-linked perk helpers must extend this suite rather than creating
a standalone mirror of production behavior. Structured frame construction and
descriptor backpressure remain owned by the focused protocol parser harness.

## Structured Web Onboarding

`unittests/CuTest/test_web_onboarding.c` is part of the production-linked
suite. Every normal build includes protocol v2 and exercises the role-play
screens, private editor transfers, checked persistence, and compatibility
behavior:

```sh
make clean
./configure
make test
make install
```

The retired `WEB_ONBOARDING_ENABLE_V2` compiler definition is rejected at
compile time. This prevents a configure or deployment command from silently
removing the role-play suite. See
[WEB_ONBOARDING_SYSTEM.md](../systems/WEB_ONBOARDING_SYSTEM.md) for the
maintained behavior and security matrix.

## Standalone World-Data Tools

The Python world-data suite requires neither MariaDB nor a `circle` build. Run
its complete enforced gate from the repository root:

```sh
make test-world-tools
```

The target runs the standard-library unit suite, verifies the source-derived
constants manifest, checks the audited world-building documentation and
generated HTML, and smoke-tests `lib/world/validate-zone.sh`. Python 3.10 or
newer and Pandoc are required.

The tracked complete fixture covers all eight validator datasets: `.zon`,
`.wld`, `.mob`, `.obj`, `.shp`, `.trg`, `.qst`, and `.hlq`. The RoL inventory
fixtures separately cover the four source manifests, all seven source kinds,
active/disabled/unlisted membership, missing companions, multi-zone inputs,
stable JSON, and malformed manifest diagnostics. Quest tests lock canonical
and legacy QST grammar, malformed recovery, all HLQ entry/command types,
physical versus runtime order, reference roles, semantic boundaries, lookup
aliases, and unchanged JSON for the original six record types.

RoL baseline tests cover exact source aggregate reconstruction, including the
source C reader's unterminated-tail behavior, deterministic target index/hash
inventory, missing and orphaned target paths, and the versioned conversion
policy. Phase 0 does not connect to a database.

RoL discovery and planning tests cover all seven grammar adapters, compact and
same-line quest forms, false-reset rejection, typed references, source-defect
classification, command identities, lineage evidence, ambiguity-preserving
actions, reserved identity allocation, collision failures, and complete
non-writing action ledgers. Phase 1 does not connect to a database or accept a
database configuration.

RoL walking-skeleton tests cover deterministic target-tree inventories, unsafe
path rejection, hash-guarded `KEEP` preconditions, and repeatable zero-write
applies. The operational Phase 3 gate additionally runs the command twice with
the same controlled timestamp and compares every hashed artifact, stages the
complete target world, validates the selected package in both trees, and proves
the authoritative tree hash is unchanged.

RoL Phase 4 selection tests cover package-level action, reset, SOC mode/action,
uncommon-extension, and binding metrics plus failure when any required pilot category
is absent. The operational selection gate verifies every Phase 1/2 artifact hash and
requires exactly 3-5 packages covering the conventional, settlement, SOC, custom-reset,
extension, special-procedure, and prior-lineage roles before pilot emission can begin.

RoL isolation tests cover the universal zone and entity formulas, evidence-backed
normalization, sparse and multi-band packages, overflow and malformed inputs, exact
Trail/Hulburg/Jotunheim coexistence, distinct Luminari and RoL artifact identities,
all typed reference classes, and target-preservation preconditions. Persistence tests
cover the fixed local-development configuration, rejection of every non-read-only
query shape, enforced read-only database sessions, schema discovery, and unique
candidate resolution for persisted RoL VNUMs. No active conversion test performs a
database migration or recovery.

### Canonical RoL maintenance gate

The completed conversion remains accepted only while all of these conditions hold:

1. Every active source zone has one evidence-backed normalized identity at the source
   zone VNUM plus 20000.
2. Every active, non-excluded room, mobile, and object is at its typed source VNUM plus
   2000000; distinct source identities remain distinct.
3. `mytheast` remains zone 20817 with entities 2081700-2081899.
4. Existing Luminari Trail 1507, Hulburg 1591, Jotunheim 1960, and artifacts
   169901-169910 remain byte-preserved while the similarly named RoL packages use
   independent reserved identities.
5. No RoL action, including a source-internal `MERGE`, targets an existing Luminari
   record.
6. Every typed cross-zone, key, quest, shop, reset, portal, SOC, DG, mobile, and object
   edge either resolves inside the RoL namespace or has an explicit source-invalid
   disposition; cross-world typed references are zero.
7. RoL compatibility markers occur only on reserved-namespace owners, and every
   hard-coded seven-digit identity in the RoL mechanics modules is in 2000000-2999999.
8. Preserved target and OLC content changes only through an explicit, evidence-backed
   record action; the final import patches zero preserved Luminari records.
9. The read-only persistence gate proves that every RoL VNUM currently stored by the
   development game resolves to exactly one candidate definition.
10. The assembled world adds no normalized baseline finding, and touched records have
    no unresolved finding.
11. Syntax and local-development-database boots, reset and walkthrough evidence, focused tests,
    world tools, production-linked CuTests, and installation pass.
12. Regeneration is byte-identical for identical inputs, repeat application is safe,
    and the applied development target passes the same audits.
13. Maintained documentation states the isolation rule and never treats target name
    similarity or a matching low VNUM as lineage.
14. No unexplained exception, unresolved decision, or final blocked identity remains.

### RoL persistence validation

The conversion stages that generate files do not connect to MariaDB. Before release,
check persisted RoL VNUMs read-only against the candidate world:

```sh
python3 scripts/world/wtool.py \
  --world-root <candidate-lib>/world \
  --json rol-persistence-check
```

The command uses only `lib/mysql_config`, requires `APP_ENV=development`, rejects
non-read-only query shapes, and fails if any persisted RoL VNUM is missing or
duplicated in the candidate. Phase 8 runs the same check while its assembled
candidate exists.

Equivalent CMake and CTest entry points are:

```sh
cmake --build build --target test-world-tools
ctest --test-dir build --output-on-failure -R '^world-tool'
```

Focused checks are also available:

```sh
make check-world-docs
python3 scripts/world/wtool.py constants sync --check
python3 scripts/world/wtool.py docs --check
python3 scripts/world/wtool.py --json rol-inventory \
  --source-root scripts/world/tests/fixtures/rol_inventory/valid >/dev/null
lib/world/validate-zone.sh 100 \
  --world-root scripts/world/tests/fixtures/phase2/complete
```

Run only the quest-system parser, graph, semantic, lookup, and reporting tests
while developing with:

```sh
PYTHONPATH=scripts/world python3 -m unittest \
  scripts.world.tests.test_quests \
  scripts.world.tests.test_hlquests \
  scripts.world.tests.test_semantics \
  scripts.world.tests.test_lookup \
  scripts.world.tests.test_reporting -v
```

Before an operational validation of ignored development data, hash
`lib/world/qst` and `lib/world/hlq`; repeat the same path-and-content hash after
`validate`, `show`, and `refs`. These commands are read-only, so any change is
a failed safety check. Retain only aggregate counts, timing, peak memory, and
hash evidence in repository documentation; do not add builder-owned files.

Tests use tracked synthetic fixtures plus the tracked artifact and minimal
bundles. CI cannot validate the ignored builder-owned files under the live
`lib/world/` type directories; a green workflow verifies the parser, fixtures,
constants, documentation, and wrapper contracts only. See the
[World Validator CLI](../utilities/WORLD_VALIDATOR_CLI.md) for validation,
lookup, RoL inventory, baseline, discovery, and action-planning evidence, JSON,
and exit-status usage, and the
[QST](../world_game-data/QUEST_FILE_FORMAT.md) and
[HLQ](../world_game-data/HLQUEST_FILE_FORMAT.md) references for their exact
test contracts.

## Protocol Parser Harness

Run the focused parser harness from the repository root:

```sh
make -C unittests/CuTest protocol-parser
```

The harness exercises the production protocol parser without booting the MUD
or opening a live network socket. See
`docs/testing/PROTOCOL_PARSER_HARNESS.md` for its fixture and case matrix.

Run every maintained test path from the repository root with:

```sh
make test-all
```

This authoritative target runs the production-linked CuTest suite, the
focused protocol parser harness, the character-rename static checks, and the
isolated MariaDB schema test. It finishes with `make install`, so the tested
server is installed as `bin/circle` and no root-level `circle` artifact is
left behind. The `unittests/CuTest` target of the same name delegates here.

## MariaDB Persistence Test

The persistence round trip is disabled unless an explicit test-only flag is
set. This prevents an ordinary local test run from writing to any configured
database.

CI supplies an isolated MariaDB service and these variables:

```sh
LUMINARI_TEST_MYSQL_ENABLE=1
LUMINARI_TEST_MYSQL_HOST=127.0.0.1
LUMINARI_TEST_MYSQL_USER=luminari_test
LUMINARI_TEST_MYSQL_PASSWORD=test_password
LUMINARI_TEST_MYSQL_DATABASE=luminari_test
LUMINARI_TEST_MYSQL_PORT=3306
```

The test creates a connection-local temporary table, performs an insert and
select through the production prepared-statement wrappers, and closes the
connection. Never point these variables at a production database.

## Isolated CI Boot Runtime

The behavioral, production-linked, coverage, and integration jobs prepare a
minimal runtime under `.ci-runtime/lib` with
`scripts/ci/prepare_test_runtime.sh`. The script accepts only a local database
host, requires a database name containing `test` or `ci`, and refuses to write
under the repository's protected `lib/` directory. It applies
`sql/master_schema.sql`, seeds one encounter-event row, copies the tracked
minimal world bundle, and creates test-only configuration and text files in
the isolated directory.

The isolated runtime provisions artifact objects from `1699.obj`, `20010.obj`,
`20053.obj`, and `20197.obj`. Keep all four object packages and the `1699` zone,
room, and mobile packages in sync when changing the canonical artifact registry.

The syntax-check boot test uses these CI-only overrides:

```sh
LUMINARI_TEST_DATA_DIR="$PWD/.ci-runtime/lib"
LUMINARI_TEST_CONFIG_FILE=.ci-runtime/lib/etc/config
```

An ordinary development run continues to boot from `lib/`. ASan and Valgrind
set `LUMINARI_TEST_SKIP_SYNTAX_BOOT=1` because their production-linked suites
run inside specialized instrumentation; the behavioral, authoritative,
coverage, and integration jobs retain the real boot gate.

The named SpecProc inventory test scans the ignored development world by
default and requires every discovered binding to resolve to a registry entry
that permits world-data ownership. Clean CI checkouts set
`LUMINARI_TEST_SPEC_WORLD_ROOT` to the tracked five-binding snapshot under
`unittests/CuTest/fixtures/spec_world_inventory/`. This keeps the parser and
exact baseline inventory contract reproducible without treating builder-owned
world data as source-controlled content.

## Coverage

The GitHub Actions coverage job:

- builds the production-linked suite with gcov instrumentation;
- runs the MariaDB persistence round trip;
- runs the covered protocol parser harness;
- creates Cobertura XML, HTML details, and a JSON summary with gcovr;
- uploads every report as a GitHub Actions workflow artifact.

Repeated Luminari measurements with gcovr 8.6 establish fixed floors of 10.50
percent for lines and 7.16 percent for branches. Exact executed counts vary
slightly because game tests exercise randomized paths; the floors use the
lowest observed results rounded down to two decimals. gcovr enforces them
before the artifact upload, so a lower result fails the job. Whenever the
stable coverage range increases, update the fixed floors in
`.github/workflows/test.yml`; the gates only move upward.

## Campaign Builds

CI builds and runs the complete production-linked behavioral suite for the
supported Luminari configuration. Retired compile-time campaign variants are
not supported or tested. The build uses no campaign define, and validation
must never modify the protected `src/campaign.h` configuration header.

## Memory Checking

CI runs the production-linked suite under ASan and UBSan with leak detection,
then fuzzes production protocol input, output, and public helper paths with
libFuzzer. It separately runs the suite under Valgrind. The sanitizer build
uses:

```sh
./configure \
  CFLAGS='-O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer' \
  LDFLAGS='-fsanitize=address,undefined'
make -j"$(nproc)" cutest
ASAN_OPTIONS='detect_leaks=1:halt_on_error=1' \
UBSAN_OPTIONS='print_stacktrace=1:halt_on_error=1' \
LUMINARI_TEST_ROOT="$PWD" ./cutest
```

The bounded protocol fuzz target copies its synthetic seed corpus to a
temporary directory before running, so it never adds generated inputs to the
repository. It sets ASan and UBSan to halt on the first finding:

```sh
make -C unittests/CuTest protocol-fuzz FUZZ_SECONDS=15
```

The equivalent Valgrind command is:

```sh
make -j"$(nproc)" cutest
valgrind \
  --leak-check=full \
  --show-leak-kinds=definite \
  --errors-for-leak-kinds=definite \
  --track-origins=yes \
  --error-exitcode=1 \
  ./cutest
```

The focused protocol harness also has a convenience target:

```sh
make -C unittests/CuTest valgrind-protocol
```

## Realms of Luminari Release Validation

The final RoL conversion gate uses the normal full suites plus a staged copy of the
complete candidate world:

```sh
make test-world-tools
make test
make install
python3 scripts/world/wtool.py \
  --world-root <candidate-lib>/world validate --all --strict
python3 scripts/world/wtool.py \
  --world-root <candidate-lib>/world --json rol-persistence-check
bin/circle -c -d <candidate-lib>
timeout --signal=INT 30 bin/circle -d <candidate-lib> <test-port>
```

The candidate lib root must use the repository's local development database
configuration from `lib/mysql_config`, and these commands must never point at
production. The syntax and bounded runtime logs must show a complete boot; the
runtime log must enter the game loop, reset the converted corpus, terminate normally,
and contain no
converted-VNUM `SYSERR`, zone error, invalid-reference, or missing-reference
diagnostic.

`rol-phase8` records the suite, install, syntax, and runtime logs with the static
structure, reference, reset, quest, shop, SOC, trap, special, path, persistence,
preservation, mechanics-isolation, and determinism audits. Its persistence gate runs
read-only against the existing local development database and requires every
persisted RoL VNUM to resolve exactly once in the candidate. After the accepted
additive overlay is applied to development,
`rol-phase8-completion` requires an identical validator result and a hash-preconditioned
repeat-apply no-op.

These automated gates prove structure, isolation, deterministic generation, bootability,
and reference closure; they do not by themselves prove authored dialogue, encounter
balance, quest intent, shop behavior, rewards, or ambience. A release that changes
converted behavior still requires risk-based development walkthroughs and spawned-state
or gameplay checks for the affected packages before production deployment.

## Adding Tests

1. Put the test in `unittests/CuTest/test_*.c`.
2. Call production functions rather than copying their implementation into the
   test.
3. Use synthetic fixtures and restore any modified globals before returning.
4. Add the file to `cutest_SOURCES` and `cutest_test_files` in `Makefile.am`.
5. Add the file to `CUTEST_TEST_SOURCES` in `CMakeLists.txt`.
6. Run `autoreconf -fvi`, `./configure`, `make test`, and the relevant focused
   harness.
7. Run Valgrind for code that allocates, frees, or mutates global registries.

Tests must include positive, negative, boundary, and cleanup assertions where
they are meaningful. An unconditional passing placeholder is not a test and
must not be added to the enforced suite.

## CI Jobs

`.github/workflows/test.yml` enforces:

- standalone world-data unit, fixture, constants, documentation, and wrapper
  checks;
- the supported Luminari behavioral suite;
- root `make test-all`;
- ASan, UBSan, and bounded protocol fuzzing;
- Valgrind on the production-linked suite;
- MariaDB-backed fixed gcovr floors and coverage-artifact upload.

The behavioral, authoritative, and coverage jobs also run the syntax-check
boot against an isolated MariaDB service and tracked minimal world. The
integration workflow independently starts the network server and proves that
it accepts a TCP connection.

Any change to test sources, build lists, covered documentation, or the
workflow triggers this pipeline.
