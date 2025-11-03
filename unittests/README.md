# LuminariMUD Unittests

This directory contains two complementary test styles following common C project practices:

1. CuTest suite tests (fast, library-like unit tests)
   - Location: `unittests/CuTest`
   - Framework: CuTest (`CuTest.c`, `CuTest.h`)
   - Aggregated runner: `AllTests` (built from `AllTests.c`)
   - Example tests: `test_bounds_checking.c`, `test.helpers.c`, `test_alignment.c`, etc.
   - Intended for: small, isolated tests with minimal dependencies, quick feedback.

2. Standalone subsystem tests (component-style, selective linkage and mocks)
   - Location: `unittests/` (top-level of this folder)
   - Each test compiles to its own executable with a dedicated Makefile
     - `test_clan.c` with `test_clan.c-Makefile`
     - `test_staff_events.c` with `test_staff_events.c-Makefile`
     - `test_act.comm.c` with `test_act.comm.c-Makefile`
   - Shared mocks live in `unittests/mocks.c`
   - Intended for: subsystem-level validation that links only the required parts of `src/` and specific mocks.

## Where to put new tests

- Add a small, fast, pure unit test: place it under `unittests/CuTest/` as `test_*.c` and register it via `AllTests.c` (or regenerate using the script if you adopt a generator).
- Add a component/subsystem test: place a `test_*.c` at `unittests/` and either:
  - create a dedicated `test_*.c-Makefile`, or
  - add it into the aggregated `unittests/Makefile` once one exists (future consolidation).

Use `-DTEST_BUILD` and limit includes to avoid pulling in the entire program. Prefer mocks in `unittests/mocks.c` where possible.

## Running tests

Top-level orchestration is provided via the repository root `Makefile`:

- Run all tests:
  - `make test`

This will:
1) Build and run the CuTest suite in `unittests/CuTest`.
2) Build and run each standalone subsystem test (e.g., `test_clan`, `test_staff_events`, `test_act.comm`).

## Notes and conventions

- Naming:
  - CuTest files: `unittests/CuTest/test_*.c`
  - Standalone tests: `unittests/test_*.c` with a matching `test_*.c-Makefile`
- Flags:
  - Use `-DTEST_BUILD` to compile `src/*.c` into test objects where the regular build would otherwise pull in `main` or heavy globals.
- Mocks:
  - Prefer `unittests/mocks.c` for common stubs. Add test-specific mocks next to the test file if needed.

## Maintenance checklist

- Keep CuTest framework files isolated in `unittests/CuTest`.
- Ensure `make test` remains the single entrypoint to validate all tests.
- Keep include paths consistent (typically `-I../src -I..` as seen in existing Makefiles).