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

This installs the tested binary as `bin/circle` and removes the root-level
`circle` artifact that the test build may leave behind.

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

## Coverage

The GitHub Actions coverage job:

- builds the production-linked suite with gcov instrumentation;
- runs the MariaDB persistence round trip;
- runs the covered protocol parser harness;
- creates Cobertura XML, HTML details, and a JSON summary with gcovr;
- uploads every report as a GitHub Actions workflow artifact.

The measured Luminari baseline with gcovr 8.6 is 24,791 of 235,600 lines
(10.52 percent) and 15,477 of 215,463 branches (7.18 percent). gcovr enforces
those rounded-down fixed floors before the artifact upload, so a lower result
fails the job. Whenever coverage increases, update the counts and fixed floors
in `.github/workflows/test.yml`; the fixed gates only move upward.

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

Any change to test sources, build lists, covered documentation, or the
workflow triggers this pipeline.
