# LuminariMUD Testing Guide

## Overview

The enforced test path has two parts:

1. A production-linked CuTest suite that compiles and links the game sources.
2. A focused protocol parser harness that links the production
   `src/net/protocol.c` implementation with minimal socket and logging doubles.

Legacy standalone vessel, autopilot, and vehicle mirror sources have been
removed. Their historical result documents remain under `docs/testing/`.

## Fast development loop

Use `make -j"$(nproc)" test-all` for the complete local check. Its prerequisites run
concurrently, then install the server after every check passes. Process-memory tests run
once through vessel tooling; `make test-process-memory` remains available separately.
CuTest overlaps the shell checks after its build/check prerequisites finish.

For a focused edit:

```sh
make -j"$(nproc)" cutest
CUTEST_FILTER=Test_mob_autoroll ./cutest
```

`CUTEST_FILTER` matches a case-sensitive substring of the registered test name. Unset or
empty runs all tests; no matches returns failure. A filtered run prints
`CUTEST_FILTER=<value>: N of M tests selected` before the results. The `make test`,
`make test-all`, and CTest `production-cutest` entry points clear the variable, so an
exported filter cannot narrow full validation.
The runner lists each test taking more than one second after the result summary, including
failed tests and wall time spent in child processes. Timing is diagnostic, not a pass gate.

CTest also supports parallel execution: `ctest -j"$(nproc)" --preset dev`.
Use incremental builds normally; clean after compiler/flag changes or suspect dependency
files. See the setup guide for ccache and optional `-O0` development builds.

## Production-Linked Tests

From the repository root:

```sh
make -j"$(nproc)" test
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

Global RoL reset removal also covers mixed prototypes, combat guards, pending
extractions, repeated removals and empty prototypes. Prototype counts include
guarded and pending mobiles when bounding the global traversal.

### Event Runtime Boot Matrix

Changes to scheduling, the game loop, wait state, persistence cadence, or
copyover must first syntax-boot the ordinary scheduler-only build. The old
queue, compatibility heartbeat, and population-loop implementations are not
compiled into that executable:

```sh
cmake --preset dev
cmake --build --preset dev -j"$(nproc)"
build/dev/bin/luminari -c
```

The loop-based rollback executable and its build/runtime selectors were
retired by maintainer decision on 2026-09-05. Test the native scheduler with
both supported I/O drivers; there is no legacy timing-backend matrix.
Production-linked tests now exercise native handles directly. Existing player
save formats remain migration inputs, but older-binary save output is not
supported. Use the current full-world acceptance report for migration checks.

Run these from a prepared `lib/` data directory or pass its absolute path with
`-d`. Database-linked tests must use the repository's isolated test fixture;
do not point a test run at a shared development or production database.
Database tests are enabled with `LUMINARI_TEST_MYSQL_ENABLE=1` and the
`LUMINARI_TEST_MYSQL_HOST/USER/PASSWORD/DATABASE/PORT` variables. The
prepared-statement and account persistence tests create only temporary tables
in their session; other database tests, such as the live-schema pet migration
check described under "MariaDB Persistence Test", need the isolated schema
fixture that CI loads from `sql/master_schema.sql`. `make test` also runs the SQL
interpolation ratchet (`scripts/ci/check_sql_interpolation.py`), which blocks
new formatted SQL with data values; bind values through `PREPARED_STMT`
instead (see `docs/systems/DATABASE_INTEGRATION.md`). A game-
loop release candidate also requires a logged-in live test and a real copyover
that verifies descriptor survival, service reconstruction, callback progress,
handoff cleanup, and port closure.

The complete native architecture acceptance matrix and compact immortal command
card are maintained in
[`EVENT_DRIVEN_CORE_ACCEPTANCE.md`](../testing/EVENT_DRIVEN_CORE_ACCEPTANCE.md).

After `make test`, always run:

```sh
make install
```

This retains the tested binary and matching symbols under its immutable
`bin/releases/<ELF-build-ID>/` directory, atomically activates `bin/luminari`,
and removes the root-level `luminari` artifact that the test build may leave
behind.

## Thrown-Weapon Regression Ownership

`unittests/CuTest/test_thrown_weapons.c` is the production-linked owner for launcher/thrown
classification, append-only dart/blowgun identity, projectile mode state, mixed-pouch compatibility
and capacity, pouch/inventory/wielded selection, persistence, detachment/finalization, Returning,
Snatch Arrows, and collection. The suite calls production helpers in `src/combat/projectiles.c` and
the production object persistence path; do not create a standalone mirror.

The converter half is owned by `scripts/world/tests/test_rol_weapon_mapping.py`. With the ignored
canonical RoL corpus installed at `EXAMPLE/RealmsOfLuminari`, it checks all relevant source records,
including the dart/blowgun split, all 44 quivers, and all 42 `MOB_ROL_ARCHER` loadout outcomes. Run
the complete world-tool suite because constant and documentation drift tests are cross-file:

```sh
make test-world-tools
make -j"$(nproc)" test
make install
```

Thrown-weapons help also requires applying its migration twice against a development database and
running `sql/components/verify_help_thrown_weapons_entries.sql`; every verifier row must report
`PASS`.

## Special Procedure Regression Ownership

Phase 00 registry safety and observability is owned by eight production-linked test sources plus one
shared fixture source:

- `test_spec_registry_persistence.c` - 10 registry, persistence, loader, and baseline OLC tests;
- `test_spec_command_pulse.c` - 13 command, activity, automatic-object, moving-room, and schedule tests;
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
`test_spec_fixtures.c` is production-linked support and is not counted as a test owner.
For focused development, select test names with a case-sensitive substring:

```sh
make -j"$(nproc)" cutest
CUTEST_FILTER=Test_spec ./cutest
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
inventory is 288 production C sources in both `luminari_SOURCES` and `SRC_C_FILES`, plus the same 41
test-owner sources in `cutest_SOURCES`, `cutest_test_files`, and `CUTEST_TEST_SOURCES`.

## Bardic Performance Regression Ownership

`unittests/CuTest/test_bardic_performance.c` is the production-linked owner for
the base performance engine and its Spellsinger and Warchanter integrations.
It covers both performance slots, command and action transitions, lifecycle
cleanup, all thirteen base performances, source-owned refresh, duration and
target defenses, affect batching and bounded `AFFECTS` serialization, spell
scope, group auras, and perk damage/save direction.

Behavior changes in `src/character/bardic_performance.c`, Bard performance registrations,
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
make -j"$(nproc)" test
make install
```

The retired `WEB_ONBOARDING_ENABLE_V2` compiler definition is rejected at
compile time. This prevents a configure or deployment command from silently
removing the role-play suite. See
[WEB_ONBOARDING_SYSTEM.md](../systems/WEB_ONBOARDING_SYSTEM.md) for the
maintained behavior and security matrix.

## Standalone World-Data Tools

The Python world-data suite requires neither MariaDB nor a `luminari` build. Run
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

01. Every active source zone has one evidence-backed normalized identity at the source
    zone VNUM plus 20000.
02. Every active, non-excluded room, mobile, and object is at its typed source VNUM plus
    2000000; distinct source identities remain distinct.
03. `mytheast` remains zone 20817 with entities 2081700-2081899.
04. Existing Luminari Trail 1507, Hulburg 1591, Jotunheim 1960, and artifacts
    169901-169910 remain byte-preserved while the similarly named RoL packages use
    independent reserved identities.
05. No RoL action, including a source-internal `MERGE`, targets an existing Luminari
    record.
06. Every typed cross-zone, key, quest, shop, reset, portal, SOC, DG, mobile, and object
    edge either resolves inside the RoL namespace or has an explicit source-invalid
    disposition; cross-world typed references are zero.
07. RoL compatibility markers occur only on reserved-namespace owners, and every
    hard-coded seven-digit identity in the RoL mechanics modules is in 2000000-2999999.
08. Preserved target and OLC content changes only through an explicit, evidence-backed
    record action; the final import patches zero preserved Luminari records.
09. The read-only persistence gate proves that every RoL VNUM currently stored by the
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
  --json rol-persistence-check \
  --development-lib-root <development-lib>
```

The command uses `lib/mysql_config` by default or an explicitly selected
`--development-lib-root`, requires `APP_ENV=development` in that selected lib root, rejects
non-read-only query shapes, and fails if any persisted RoL VNUM is missing or duplicated in the
candidate. Phase 8 runs the same check while its assembled candidate exists.

Equivalent CMake and CTest entry points are:

```sh
cmake --build build/dev --target test-world-tools
ctest -j"$(nproc)" --preset dev -R '^world-tool'
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

When the ignored `EXAMPLE/RealmsOfLuminari` source corpus is installed, corpus
integration tests create a fresh discovery, plan, pilot selection, and capability
audit in a temporary directory. They share these inputs within the test process
and clean them up at exit; historical `lib/rol-conversion/runs/` folders are not
required. The target fixture starts from the tracked minimal world plus one
actually converted Hulburg room, exercising canonical identity preservation.
Current source expectations cover 71,680 records, 69,922 emitted records, all
333 pilot quests, and the complete special-procedure reconciliation ledgers.
Only absence of the ignored source corpus permits these integration tests to
skip; generation or reconciliation failures fail the tests.

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

Time the same parser paths with the flags under evaluation:

```sh
make -C unittests/CuTest protocol-bench
```

`PROTOCOL_BENCH_CFLAGS` and `PROTOCOL_BENCH_LDFLAGS` select the profile;
`PROTOCOL_BENCH_ITERATIONS` bounds the run. The target prints the median,
minimum, and maximum nanoseconds per iteration over five rounds. Measure on an
idle host; the result is not comparable while other builds are running.

## Production Profile Contract

`make test` includes `test-production-profile`, which checks that
`scripts/deployment/production_profile.sh` emits its four output keys with the
`-O2 -g` policy, that a program built with those flags passes
`scripts/deployment/verify_hardened_binary.sh`, that the same program built
with the compiler's default flags fails it (the profile marker section is
missing, whatever hardening the distribution applies on its own), and that the
verifier names every property a deliberately degraded build lacks. Set `CC` to
run it against another compiler. The CMake test is named `production-profile`.

Run every maintained test path from the repository root with:

```sh
make -j"$(nproc)" test-all
```

This authoritative target runs the production-linked CuTest suite, the
focused protocol parser harness, the character-rename static checks, and the
isolated MariaDB schema test. It finishes with `make install`, so the tested
server is installed as `bin/luminari` and no root-level `luminari` artifact is
left behind. The `unittests/CuTest` target of the same name delegates here.

The world-tool suite also compiles the maintained `shopconv` utility with
warnings treated as errors. Its regression fixtures cover empty and multiline
messages, CRLF and final-line terminators, invalid-header recovery, and complete
filename diagnostics. Set `SHOPCONV_TEST_CFLAGS` to add sanitizer flags when
running `scripts/world/tests/test_shops.py`.

The production DG regressions distinguish explicit waits from automatic runaway-loop
yields: a script that deliberately waits can continue past 100 iterations, while a
busy loop still stops at the limit. RoL conversion tests also verify that autonomous
SOC routes suppress ordinary wandering, halt when blocked, and can complete a closed lap.

Saved-object regressions exercise file and isolated house-database restoration with
flags and values that change both automatic-procedure and mud-hour timer membership.
The legacy ferry regression checks membership when its countdown expires and restarts
at a dock, preserving the existing movement and waiting delays.
Buff-target cleanup covers connected and link-dead players through the maintained
player registry, with the original full-list fallback before registry startup.

Help-content verification checks that BLAST resolves to eldritch-blast help,
while FIRE and ammunition keywords resolve to launcher help. Superseded ranged
and special-procedure articles retain distinct LEGACY- keywords; maintained
command keywords must not resolve to those old articles. The local burn-in also
compares each repaired database entry with its help.hlp fallback and verifies
that repeated content migrations are idempotent.

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

Most pet persistence tests also isolate themselves with temporary tables. The
exception is `Test_pet_live_schema_cascades_objects_and_rejects_orphans`, which
needs the live `pet_data` and `pet_save_objs` tables from
`sql/master_schema.sql` because InnoDB refuses foreign keys on temporary tables.
It runs the migration runner and validator against those tables, then checks
the cascade and orphan rejection inside a transaction that is rolled back. The
rollback covers only the fixture rows; the live schema stays migrated, which
is the state the booted suite already left it in. CI loads the master schema into its
isolated database before the suite runs; the master schema carries no foreign
key, so the booted suite applies migration `2026091007` for real on every
fresh database and this test observes its outcome.

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

An ordinary development run continues to boot from `lib/`. Only Valgrind sets
`LUMINARI_TEST_SKIP_SYNTAX_BOOT=1`; the behavioral, authoritative, coverage,
sanitizer, and integration jobs retain the real boot gate.

The named SpecProc inventory test scans the ignored development world by
default and requires every discovered binding to resolve to a registry entry
that permits world-data ownership. Clean CI checkouts set
`LUMINARI_TEST_SPEC_WORLD_ROOT` to the tracked five-binding snapshot under
`unittests/CuTest/fixtures/spec_world_inventory/`. This keeps the parser and
exact baseline inventory contract reproducible without treating builder-owned
world data as source-controlled content.

The CMake and Autotools production-linked test entry points always set this
fixture explicitly. They also run the event architecture contracts under
`scripts/events/`: native one-wheel ownership and semantic registration,
demand-driven mobile work, default-build rollback exclusion, and retired
PubSub exclusion. Run the consolidated event check directly with:

```sh
scripts/events/test_native_event_architecture.sh
```

## Build System Parity

```sh
make check-build-parity
make distcheck-archive
```

The first command compares every hand-maintained source list in `Makefile.am`
with its `CMakeLists.txt` counterpart and fails on missing, extra, duplicate,
nonexistent, or untracked entries. Variable references are expanded first, so
it also proves that `cutest_SOURCES` and the CMake `cutest` target both
compile every production source plus the harness and test files. It runs
inside `make test`, as the `build-parity` CTest entry, and as a blocking CI
job. The second exports `git archive HEAD` to a temporary directory and, for
Autotools and then the CMake `dev` preset (override with `CMAKE_PRESET=<name>`),
configures, builds, runs `make test` or `ctest`, and installs, so a
distribution never depends on repository-only files. It runs as the blocking
`Clean archive, both build systems` CI job after the isolated MariaDB runtime
is prepared; locally, export `LUMINARI_TEST_SKIP_SYNTAX_BOOT=1` when no world
data is available. It is not part of `make test` because it rebuilds the tree
twice. The CMake `sanitizers` and `coverage` presets provide the same
instrumentation as the Autotools `CFLAGS` recipes in the workflow.

The archive job retains the real encounter-world boot and pet persistence tests.
Its MariaDB service, `LUMINARI_TEST_MYSQL_*` variables, and prepared runtime are
required together. `LUMINARI_TEST_DATA_DIR` must be absolute because the check
changes directories into the archive. `LUMINARI_TEST_CONFIG_FILE` may be absolute
or relative to the invoking directory: the check copies it to `lib/etc/config` in
the archive and passes that relative path to the server. Passing the original
absolute config path directly to the server caused the boot failure reported in
issue #157.

`python3 scripts/ci/test_clean_archive.py -v` checks this handoff using a temporary
Git repository and recording build tools. It covers both test entry points,
absolute and relative config inputs, database environment inheritance, and a
missing config failing before any build step. It runs in `make test` and as the
`clean-archive-runtime` CTest entry; the full archive job supplies the real build,
boot, and database verification.

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
must never modify the protected `src/config/campaign.h` configuration header.

## Sanitizers and Fuzzing

Three sanitizer builds of the production-linked suite run on every pull
request as the `sanitizers` matrix in `.github/workflows/test.yml`, and a
fourth job fuzzes the runtime trust boundaries. Valgrind runs separately (see
below). Every job fails on the first unsuppressed finding: `halt_on_error=1`
is set for ASan, UBSan, and TSan, no suppression file exists, and a finding is
fixed at its root or the test that exposed it is corrected; a suppression that
hides a class of report is not accepted.

| Job | Compiler | Flags | Runs |
| -- | -- | -- | -- |
| ASan+UBSan (gcc) | GCC 13 (the runner image) | `-fsanitize=address,undefined` | suite with the syntax boot, installed server boot, pre-authentication client interaction, graceful shutdown |
| ASan+UBSan (clang) | Clang 18 (the runner image) | `-fsanitize=address,undefined` | the same |
| ThreadSanitizer (clang) | Clang 18 | `-fsanitize=thread` | the same; the AI worker thread, the Intermud3 client thread, logging, and the database handoff are all in the suite |
| Fuzz the trust boundaries | Clang 18 | `-fsanitize=fuzzer-no-link,address,undefined` | seed and regression replay, then 15 s per production-linked target, then the standalone protocol and binary format fuzzers |

The supported compiler generations are the ones in the
[compiler policy](SETUP_AND_BUILD_GUIDE.md#compiler-policy-and-warning-tiers);
the sanitizer jobs use the minimum generation of each family on x86-64 Linux,
which is the platform the sanitizer runtimes are validated on. Every job runs
`scripts/ci/check_sanitizer_build.sh`, which prints the compiler identity, the
configured `CFLAGS` and `LDFLAGS`, and the sanitizer runtime each binary links
(the `__asan_init`, `__ubsan_handle_*`, `__tsan_init`, or libFuzzer entry
points, static or through the shared runtime), and fails when any is missing.
Installing Clang without building with it is therefore a failure, not a green
run.

### Reproducing a sanitizer job

```sh
CC=clang   # or gcc
autoreconf -fvi
./configure CC=$CC \
  CFLAGS='-O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer' \
  LDFLAGS='-fsanitize=address,undefined'
make -j"$(nproc)" cutest luminari
scripts/ci/check_sanitizer_build.sh --cc $CC --sanitizers address,undefined \
  --binary cutest --binary luminari
scripts/ci/prepare_test_runtime.sh "$PWD/.ci-runtime/lib"   # needs the MariaDB variables
ASAN_OPTIONS='detect_leaks=1:halt_on_error=1' \
UBSAN_OPTIONS='print_stacktrace=1:halt_on_error=1' \
LUMINARI_TEST_DATA_DIR="$PWD/.ci-runtime/lib" LUMINARI_TEST_CONFIG_FILE=.ci-runtime/lib/etc/config \
LUMINARI_TEST_ROOT="$PWD" ./cutest
make install
LUMINARI_STARTUP_TIMEOUT=180 LUMINARI_TEST_DATA_DIR="$PWD/.ci-runtime/lib" \
  scripts/ci/test_server_startup.sh
```

For ThreadSanitizer replace both sanitizer lists with `thread` and export
`TSAN_OPTIONS='halt_on_error=1:second_deadlock_stack=1'`. The syntax-check
boot runs inside the instrumented suite in all three jobs; only the Valgrind job
still sets `LUMINARI_TEST_SKIP_SYNTAX_BOOT=1`.

`scripts/ci/test_server_startup.sh` boots the installed server through
`autorun.sh` on port 4100, waits for the health endpoint, runs
`scripts/ci/smoke_client.py` (a stranger's conversation in front of the
password prompt: a declined new account name, an invalid name, an over-long
line, Telnet negotiation, and an empty name that closes the connection), checks
the health endpoint again, then stops the server and requires a clean exit
code. Under a sanitizer the server's report goes to its log, the exit code is
non-zero, and the job uploads the logs from `LUMINARI_STARTUP_LOG_DIR`.

### Production-linked fuzz targets

`luminari_fuzz` is a libFuzzer executable that links the cutest objects, so
each target drives the parser the server compiles; there is no mirror to
drift. The table lives in `unittests/CuTest/test_fuzz_targets.c` and
`LUMINARI_FUZZ_TARGET` selects one:

| Target | Boundary | Production entry points |
| -- | -- | -- |
| `dotenv` | `lib/.env` | `get_env_value`, `get_env_int`, `get_env_bool` |
| `config` | `lib/etc/config` | `load_config` (behind the exit guard) |
| `dg` | DG script expressions and variables | `process_eval`, `var_subst`, `matching_quote` |
| `world` | world, mobile, object, zone, and trigger files | `discrete_load`, `parse_room`, `parse_mobile`, `parse_object`, `load_zones`, `parse_trigger` (behind the exit guard) |
| `command` | socket line assembly and command tokenizers | `process_input`, `ProtocolInput`, `half_chop`, `find_command`, `one_argument`, `two_arguments`, `three_arguments`, `reserved_word`, `fill_word`, `delete_doubledollar` |
| `i3` | Intermud3 gateway framing and JSON-RPC | `i3_process_input`, `i3_parse_response` |
| `ai` | AI provider responses | `parse_json_response`, `parse_ollama_json_response` |
| `discord` | Discord bridge inbound JSON | `parse_discord_json` |
| `onboarding` | web onboarding envelopes and controls | `web_onboarding_handle_action`, `web_onboarding_set_capability`, `web_onboarding_set_version_list`, `web_onboarding_handle_catalog_control` |

The world loader and the configuration reader end the process on a rejected
file. Those two targets run behind an exit guard: the harness interposes
`exit()` and returns to the target, and they run with leak detection off
because the records a rejected file leaves behind are abandoned by the
production exit as well. Every other target keeps leak detection on. The
telnet parser and the durable binary formats keep their standalone harnesses
(`protocol-fuzz`, `binary-formats-fuzz` in `unittests/CuTest/Makefile`).

Seeds live in `unittests/CuTest/fuzz_corpus_game/<target>/`, dictionaries in
`unittests/CuTest/fuzz_dictionaries/<target>.dict`, and every fixed finding
keeps its reproducer in `unittests/CuTest/fuzz_regressions/<target>/`. Both
the seeds and the regression inputs are replayed deterministically by
`Test_fuzz_targets_replay_seed_and_regression_inputs` in the ordinary
production-linked suite, in a forked child that fails on a signal, an
unexpected exit status, or a sanitizer report (the child ends without an
exit-time leak check, which would see the parent's memory; per-input leak
detection is the fuzz job's replay step). A finding therefore becomes a
permanent test the moment its input is saved under `fuzz_regressions/`, with a
focused CuTest case added when the fix has a checkable contract (for example
`Test_world_loading_production_affect_flag_letters_convert_in_flag_width`).

```sh
./configure CC=clang \
  CFLAGS='-O1 -g -fsanitize=fuzzer-no-link,address,undefined -fno-omit-frame-pointer' \
  LDFLAGS='-fsanitize=address,undefined'
make -j"$(nproc)" luminari_fuzz
scripts/ci/run_fuzz_targets.sh --seconds 0      # replay seeds and regressions
scripts/ci/run_fuzz_targets.sh --seconds 15     # the pull request smoke
scripts/ci/run_fuzz_targets.sh --seconds 600 world dg   # a longer local campaign
LUMINARI_FUZZ_TARGET=world ./luminari_fuzz fuzz-artifacts/world/crash-<sha>   # reproduce
```

`scripts/ci/run_fuzz_targets.sh` copies the seeds to a scratch corpus, runs
each target with its dictionary and regression directory, writes every
reproducer and the fuzzer log under `--artifacts` (default `fuzz-artifacts/`),
minimizes each reproducer with `-minimize_crash=1`, and exits 1 after all
requested targets have run. The CI job uploads that directory on failure. The
libFuzzer run uses `-timeout=10` and `-max_len=16384`; the CMake build offers
the same executable through `-DLUMINARI_FUZZ=ON` with
`-DLUMINARI_SANITIZERS=fuzzer-no-link,address,undefined`.

Pull requests get the deterministic replay and 15 s per target.
`.github/workflows/fuzz-campaign.yml` runs weekly and on demand
(`workflow_dispatch` with a per-target duration, default 900 s) with a larger
input limit and always uploads its artifacts. The campaigns that introduced
these targets, and the review of them, found and fixed twelve defects: an
affect-flag shift past the int width, an unbounded mob `Feat`/`MFeat` index, a
null short description in the object checks, an empty trigger script, a
dangling large output buffer after the `--` command (a repeatable server
hang), the same command discarding a player's pending output along with the
queued commands, an unterminated `%variable` that read past the substitution
buffer, a quoted script token ending in a backslash that read past its
terminator, integer overflow in script arithmetic, a leak and double free when
the configuration defaults were loaded again, a legacy record conversion
indexing `zone_table[-1]`, and a room exit direction outside `dir_option[]`;
the last one came from the fuzz job's first run on GitHub, whose uploaded,
minimized reproducer became the regression input.

### MemorySanitizer and OSS-Fuzz

MemorySanitizer is not enabled. It reports any read of uninitialized memory,
including reads inside uninstrumented libraries, so every dependency must be
built with MSan: the MariaDB client, json-c, libevent, libcurl, OpenSSL, libgd,
and libcrypt. None of those ships instrumented on the supported platforms, and
the false reports from the first uninstrumented call would have to be
suppressed wholesale, which the policy above forbids. Enabling MSan requires an
instrumented build of that dependency set; until then its results would be
unreliable and it is not part of CI.

The production-linked targets are compatible with OSS-Fuzz's build model (a
libFuzzer entry point per target, seeds, dictionaries, and reproducers), but
the harness links the whole server against seven shared libraries, so an
OSS-Fuzz project would need a build script that provides those libraries and
a target-per-binary split of `luminari_fuzz`. The scheduled campaign covers
the same ground on the project's own runners.

## Memory Checking

The Valgrind job builds the suite without sanitizers and runs it with
`LUMINARI_TEST_SKIP_SYNTAX_BOOT=1`:

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

The standalone protocol and binary file format fuzzers copy their seed corpora
to a temporary directory before running (the binary seeds are hex text, decoded
first) and halt on the first finding:

```sh
make -C unittests/CuTest protocol-fuzz FUZZ_SECONDS=15
make -C unittests/CuTest binary-formats-fuzz FUZZ_SECONDS=15
make -C unittests/CuTest binary-formats
```

See [BINARY_FILE_FORMATS.md](../systems/BINARY_FILE_FORMATS.md#verification)
for the golden-fixture harness, which CI also runs on AArch64.

## Realms of Luminari Release Validation

The final RoL conversion gate uses the normal full suites plus a staged copy of the
complete candidate world:

```sh
make test-world-tools
make -j"$(nproc)" test
make install
python3 scripts/world/wtool.py \
  --world-root <candidate-lib>/world validate --all --strict
python3 scripts/world/wtool.py \
  --world-root <candidate-lib>/world --json rol-persistence-check \
  --development-lib-root <development-lib>
bin/luminari -c -d <candidate-lib>
timeout --signal=INT 30 bin/luminari -d <candidate-lib> <test-port>
```

The persistence gate uses the repository's local development database configuration by default;
an isolated worktree may name an established development lib root explicitly. These commands must
never point at production. The syntax and bounded runtime logs must show a complete boot; the
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
6. Run `python3 scripts/ci/check_build_parity.py`; it fails until both lists
   match.
7. Run `autoreconf -fvi`, `./configure`, `make test`, and the relevant focused
   harness.
8. Run Valgrind for code that allocates, frees, or mutates global registries.

Tests must include positive, negative, boundary, and cleanup assertions where
they are meaningful. An unconditional passing placeholder is not a test and
must not be added to the enforced suite.

## CI Jobs

`.github/workflows/test.yml` enforces:

- standalone world-data unit, fixture, constants, documentation, and wrapper
  checks;
- Autotools/CMake manifest parity (`scripts/ci/check_build_parity.py`);
- blocking CMake configure, build, CTest, and install jobs with the strict
  `ci-gcc` and `ci-clang` presets (baseline warning tier with `-Werror`) in
  Debug and Release on GCC 13, Clang 18, GCC 16.2, and Clang 22.1.8;
- the migration warning budget on GCC 16.2 and Clang 22.1.8
  (`scripts/ci/check_warning_budget.py`), which fails when any warning class
  grows;
- a compiler identity and version check in every compiling job
  (`scripts/ci/check_compiler.sh`) and a check that strict flags cannot
  change Autotools or CMake feature-probe results
  (`scripts/ci/check_configure_probes.sh`);
- the supported Luminari behavioral suite;
- root `make test-all`;
- the hardened production profile on Autotools (GCC, GCC 14, Clang) and CMake
  (GCC, Clang), rejecting the retired `--enable-optimizations` option,
  verifying the linked server and test binaries, running the full
  production-linked suite against that artifact, and verifying the installed
  server;
- ASan+UBSan on GCC and on Clang and ThreadSanitizer on Clang, each with the
  syntax boot, the installed server boot, and the pre-authentication client
  interaction, with the compiler and linked runtimes verified;
- the production-linked fuzz targets (deterministic replay plus 15 s each) and
  the bounded protocol and binary format fuzzers, with reproducers uploaded;
- Valgrind on the production-linked suite;
- MariaDB-backed fixed gcovr floors and coverage-artifact upload;
- a clean `git status` after the configure, build, test, install, and clean
  cycle, and a source-hygiene scan of the `make dist` tarball.

`.github/workflows/hygiene.yml` runs `scripts/ci/check_source_hygiene.py` on
every push: no tracked build products, valid UTF-8 with LF endings, and ASCII
documentation. See the Source Tree Hygiene section of
[SETUP_AND_BUILD_GUIDE.md](SETUP_AND_BUILD_GUIDE.md).

The behavioral, authoritative, CMake, and coverage jobs also run the
syntax-check boot against an isolated MariaDB service and tracked minimal
world. The
integration workflow independently starts the network server and proves that
it accepts a TCP connection.

Any change to test sources, build lists, covered documentation, or the
workflow triggers this pipeline.

## CI job map and local containers

The production-linked job runs `make -j test-all` with libevent, reruns the same CuTest
binary with select, verifies the installed server's real-port startup, health endpoint,
and graceful shutdown through autorun, then checks clean-tree and source-distribution
hygiene. Both I/O drivers retain the complete behavioral suite.

The strict GCC/Clang CMake jobs still fail on warnings. `quality.yml` runs every pinned
formatter hook and the clang-tidy baseline, which analyzes the translation units a pull request
changes and the whole tree weekly, after refusing a change that raises any static-analysis
baseline. `toolchain-analysis.yml` runs the analysis warning tier and the
ISO C23 extension report weekly; only its GCC analyzer classes are budgeted. `make test` and CTest
check that every header outside its baseline compiles on its own, and the CodeQL job fails when
its database lacks a production source; see
[Static Analysis](SETUP_AND_BUILD_GUIDE.md#static-analysis). All five production-profile server
builds retain binary hardening verification; hardened tests run with Autotools/GCC 14 and
CMake/Clang. Each build system has an independent clean-archive job. The sanitizer matrix,
the fuzz job, Valgrind, coverage floors, CodeQL, world tools, parity, formatting, source
hygiene, database migrations, and world validation remain; `fuzz-campaign.yml` runs the
longer scheduled fuzz campaign.

`.github/actions/setup-build` supplies dependencies, missing example headers, and compiler
caching by job, compiler/profile, build configuration, and commit. Cache restoration never
replaces running a check.

For the local matrix, install Docker and Python's PyYAML, then build the dependency image
once (rebuild when its Dockerfile, help-sync requirements, clang-tidy pin, or pre-commit
configuration changes):

```sh
docker build -t luminari-ci:local-fast -f scripts/ci/local/Dockerfile .
docker build -t luminari-ci:local-gcc-16.2 -f scripts/ci/local/Dockerfile.gcc-16.2 .
python3 scripts/ci/local/run.py --list
python3 scripts/ci/local/run.py --jobs 3 --cpus 4
```

A job that GitHub runs inside a compiler container (`container: gcc:16.2`)
runs locally in `luminari-ci:local-gcc-16.2`; every other job uses the
`--image` default.

The runner exports committed HEAD, executes the actual build/integration/format/hygiene/security-scan
workflow shell commands in separate containers, and keeps the local world and credentials
outside those containers. Each database job gets its own disposable MariaDB. Every game
smoke test uses port 4100 inside its container; no host port is published. The image includes
the workflow dependencies, the pre-commit hooks, and the PHP and PowerShell runtimes their
formatters need. A shared compiler cache defaults to
`~/.cache/luminari-ci/ccache`; `--cache` overrides it. Jobs use a stable `/workspace` path.

`--job NAME` selects one name from `--list`. `--results DIR` retains per-job logs, coverage
artifacts, and a timed `summary.json`; failures produce a nonzero exit. Each snapshot's parent
commit is the merge base with `--base` (default `origin/master`), so a job that diffs against
`HEAD^1`, such as the clang-tidy baseline, sees the branch's changes as it does on GitHub. Run the complete
matrix on the final commit after iterating with the host suite. GitHub action downloads,
cache/upload services, CodeQL, and dependency review are verified on GitHub rather than
emulated locally. Unsupported workflow expressions or actions fail explicitly.
