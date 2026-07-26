# LuminariMUD Testing Guide

## Overview

The enforced test path has two parts:

1. A production-linked CuTest suite that compiles and links the game sources.
2. A focused protocol parser harness that links the production
   `src/protocol.c` implementation with minimal socket and logging doubles.

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

## Protocol Parser Harness

Run the focused parser harness from the repository root:

```sh
make -C unittests/CuTest protocol-parser
```

The harness exercises the production protocol parser without booting the MUD
or opening a live network socket. See
`docs/testing/PROTOCOL_PARSER_HARNESS.md` for its fixture and case matrix.

Run both enforced test paths with:

```sh
make -C unittests/CuTest test-all
```

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
- uploads the report as a workflow artifact and to Codecov.

The initial gcovr floor is deliberately low because this is a large legacy
codebase: 0.70 percent line coverage and 0.40 percent branch coverage. The
Codecov project status uses the parent result as its target with zero
threshold, and patch status is also enforced. Coverage regressions therefore
fail even when the fixed bootstrap floor still passes. Raise the fixed floors
as the suite grows.

## Campaign Builds

CI compiles the default, DragonLance, and Forgotten Realms variants. Campaign
selection is supplied through `CPPFLAGS`:

```sh
./configure CPPFLAGS="-DCAMPAIGN_DL"
./configure CPPFLAGS="-DCAMPAIGN_FR"
```

The default build uses no campaign define. Never edit `src/campaign.h` to
exercise a campaign build.

## Memory Checking

CI runs the production-linked suite under Valgrind. The equivalent local
command is:

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

- default, DragonLance, and Forgotten Realms builds;
- root `make test`;
- the focused protocol parser harness;
- Valgrind on the production-linked suite;
- MariaDB-backed gcovr and Codecov reporting.

Any change to test sources, build lists, coverage configuration, or the
workflow triggers this pipeline.
