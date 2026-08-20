# LuminariMUD Unit Tests

This directory contains the maintained automated tests for the server.

1. Production-linked CuTest suite
   - Location: `unittests/CuTest`
   - Framework: CuTest (`CuTest.c`, `CuTest.h`)
   - Aggregated runner: `cutest`
   - `AllTests.c` is generated from the sources registered in `Makefile.am`.
   - The executable links the same production objects as the game server.

2. Focused protocol parser harness
   - Location: `unittests/CuTest/test_protocol_parser.c`
   - Runner: `unittests/CuTest/protocol_parser_tests`
   - This is intentionally source-linked because it exercises parser inputs
     without booting the full server.

## Where to put new tests

- Add production behavior tests under `unittests/CuTest/` as `test_*.c`.
- Register every new source in `cutest_SOURCES` and `cutest_test_files` in
  `Makefile.am`, and in `CUTEST_TEST_SOURCES` in `CMakeLists.txt`.
- Extend the focused harness only for protocol parser behavior.

## Running tests

The authoritative entry point is in the repository root:

```bash
make test-all
```

This will:
1. Build and run the production-linked CuTest suite.
2. Build and run the focused protocol parser harness.
3. Run the character-rename static and temporary-MariaDB schema tests.
4. Install the tested server to `bin/luminari` and remove the root build artifact.

Individual targets remain available:

```bash
make test
make test-protocol
make test-character-rename-static
make test-character-rename-schema
make test-protocol-fuzz
```
