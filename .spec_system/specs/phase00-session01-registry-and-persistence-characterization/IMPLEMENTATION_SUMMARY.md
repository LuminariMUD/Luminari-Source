# Implementation Summary

**Session ID**: `phase00-session01-registry-and-persistence-characterization`
**Completed**: 2026-08-06
**Duration**: ~1 hour

---

## Overview

Established a production-linked compatibility baseline for the legacy special-procedure registry,
named mobile/object/room persistence, checked-in binding inventory, and all three Oasis OLC
selection paths. Added reusable fixtures that isolate mutable game globals and production parser
state without changing application behavior.

---

## Deliverables

### Files Created

| File | Purpose | Lines |
|------|---------|-------|
| `unittests/CuTest/test_spec_fixtures.h` | Reusable owner-aware fixture interface | 43 |
| `unittests/CuTest/test_spec_fixtures.c` | Production parser, writer, descriptor, and sandbox fixture | 778 |
| `unittests/CuTest/test_spec_registry_persistence.c` | Registry, persistence, inventory, OLC, and cleanup tests | 681 |

### Files Modified

| File | Changes |
|------|---------|
| `Makefile.am` | Added both sources to CuTest compilation and generated-test membership. |
| `CMakeLists.txt` | Added matching production-linked CuTest membership. |

---

## Technical Decisions

1. **Use production paths**: The fixtures call the real registry, world parsers, OLC writers, and
   editor parsers so the baseline measures observable compatibility rather than an approximation.
2. **Contain parser state at the process boundary**: Each parser-backed scenario runs in a bounded
   child because the legacy loaders retain private static indexes that cannot be reset safely.
3. **Keep sandbox ownership in the parent**: The parent validates, owns, and checks cleanup of each
   private mode-0700 world tree, including abnormal child exits.

---

## Test Results

| Metric | Value |
|--------|-------|
| CuTests | 482 |
| Passed | 482 |
| Auxiliary checks | 7/7 passed |
| Coverage | N/A - not configured |

Both supported build manifests compiled and passed the production-linked suite. `make test` and
`make install` passed, checked-in world data remained byte-identical, and no root-level `circle`
artifact or fixture sandbox remained.

---

## Lessons Learned

1. Restoring global pointers is insufficient for loaders with function-local static counters;
   process isolation preserves the parent suite's parser state without a production test hook.
2. Real OLC menu execution requires complete descriptor protocol initialization and destruction,
   even when the test only inspects callback selection.
3. Cleanup ownership must survive signals and direct process exits, not only ordinary fixture
   teardown.

---

## Future Considerations

Items for future sessions:

1. Reuse the fixture foundation for command, pulse, combat, and secondary-callback characterization.
2. Treat the exact 29-row order and `Guild`/`Guildmaster` shared pointer as compatibility evidence
   while Session 04 replaces duplicate rows with validated canonical/alias metadata.
3. Preserve the five checked-in named bindings and their canonical save syntax through later
   authored/effective binding work.

---

## Session Statistics

- **Tasks**: 23 completed
- **Implementation files created**: 3
- **Implementation files modified**: 2
- **Tests added**: 10
- **Blockers**: 2 resolved
