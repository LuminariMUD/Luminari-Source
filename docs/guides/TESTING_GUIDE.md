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
- `test_spec_combat_secondary.c` - 14 combat-token, ignored-return, shop, and quest tests;
- `test_spec_registry_validation.c` - 13 immutable metadata, bounds, and boot-failure tests;
- `test_spec_owner_aware_olc.c` - 7 filtered-menu, description, selection, and flag tests;
- `test_spec_authored_bindings.c` - 7 owned authored-state, loader, diagnostic, and lifecycle tests;
- `test_spec_binding_round_trip.c` - 7 writer-to-loader identity and explicit-action tests; and
- `test_spec_effective_binding.c` - 7 provenance, precedence, mode, secondary, and room-safety tests.

Phase 01 adds `test_spec_dispatch.c` with 12 gateway and extraction-safety tests. Phase 02 adds
`test_spec_assign_table.c` with 11 declarative-row, owner/source validation, diagnostic, and stable
source-label tests. The exact inventory through Phase 02 is 101 dedicated `Test` functions.
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
declarative-assignment inventory, binding-chain diagnostics, and help verification. Later cooldown,
affect, content extraction, and general composition coverage remains explicitly deferred to its
owning implementation phases.

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
`.wld`, `.mob`, `.obj`, `.shp`, `.trg`, `.qst`, and `.hlq`. Quest tests lock
canonical and legacy QST grammar, malformed recovery, all HLQ entry/command
types, physical versus runtime order, reference roles, semantic boundaries,
lookup aliases, and unchanged JSON for the original six record types.

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
lookup, JSON, and exit-status usage, and the
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
default. Clean CI checkouts set `LUMINARI_TEST_SPEC_WORLD_ROOT` to the tracked
five-binding snapshot under
`unittests/CuTest/fixtures/spec_world_inventory/`. This keeps the parser and
inventory contract reproducible without treating builder-owned world data as
source-controlled content.

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
