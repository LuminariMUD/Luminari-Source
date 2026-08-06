# Session Specification

**Session ID**: `phase00-session01-registry-and-persistence-characterization`
**Phase**: 00 - Registry Safety and Observability
**Status**: Complete
**Created**: 2026-08-06
**Completed**: 2026-08-06
**Base Commit**: fced8f852d5ad1741a135ed1b24c67de08840937

---

## 1. Session Overview

This session establishes executable compatibility evidence for the current special-procedure
registry, named world bindings, and Oasis OLC selection behavior. It is first because later Phase 00
sessions replace registry metadata and binding state, and those changes need production-linked
tests that distinguish intentional policy changes from accidental persistence drift.

The session adds reusable CuTest fixtures that isolate mutable world globals, parse in-memory mobile,
object, and room records through the production loaders, save through the production OLC writers in
a temporary `world/` sandbox, and exercise the production medit, oedit, and redit parsers. It does
not change registry policy, world syntax, callback storage, or runtime dispatch.

---

## 2. Objectives

1. Freeze current registry count, ordering, lookup, reverse lookup, alias, and boundary behavior.
2. Prove known mobile, object, and room names load and save through production paths.
3. Characterize current medit, oedit, and redit select, invalid-select, and clear behavior.
4. Provide isolated fixtures that later Phase 00 characterization suites can reuse safely.

---

## 3. Prerequisites

### Required Sessions

- None - this is the first Phase 00 session.

### Required Tools Or Knowledge

- Root production-linked CuTest harness and generated `AllTests.c` registry.
- Existing registry accessors in `src/spec_assign.c` and declarations in `src/spec_procs.h`.
- Production loaders in `src/db.c`, writers in `src/olc/genmob.c`, `src/olc/genobj.c`, and
  `src/olc/genwld.c`, and editor parsers in `src/olc/medit.c`, `src/olc/oedit.c`, and
  `src/olc/redit.c`.

### Environment Requirements

- Development checkout with the MariaDB-backed build dependencies available.
- Temporary test files must remain outside `lib/world/`; `lib/.env` and `lib/mysql_config` remain
  read-only.
- `make test` must be followed by `make install`.

---

## 4. Scope

### In Scope (MVP)

- Engine maintainer can execute registry compatibility assertions for all 29 current name rows,
  case-insensitive lookup, null and unknown input, reverse lookup, and sentinel boundaries.
- Engine maintainer can verify that `Guild` is current reverse-lookup output and `Guildmaster` is an
  accepted name for the same legacy handler.
- Test maintainer can load `Postmaster`, `Greyhawk Ship`, and `Greyhawk Ship Commands` through the
  current mobile, object, and room parsers and save them through current OLC writers.
- Test maintainer can exercise current all-owner medit, oedit, and redit selection, invalid bounds,
  and explicit clear behavior through the production parser branches.
- Later Phase 00 sessions can reuse fixture setup, teardown, descriptor, parser-stream, and
  temporary-world helpers without leaking globals or writing repository world data.

### Out Of Scope (Deferred)

- Validated definition metadata and safe arbitrary-high accessors - Reason: Session 04 changes the
  registry contract after the legacy boundary is characterized.
- Owner-filtered OLC choices and prerequisite descriptions - Reason: Session 05 owns the builder
  policy change.
- Authored identity, unresolved-name retention, and effective binding provenance - Reason: Sessions
  06 through 08 own those data models and diagnostics.
- Runtime invocation, scheduling, activation, and `no_specials` behavior - Reason: Sessions 02 and
  03 provide those characterization suites.

---

## 5. Technical Approach

### Architecture

Add one reusable CuTest fixture module and one focused characterization suite. The fixture snapshots
and restores `world`, `mob_proto`, `mob_index`, `obj_proto`, `obj_index`, `zone_table`, their top
indexes, descriptor output state, and the process working directory. It builds minimal valid records
with `tmpfile()`, invokes `parse_mobile()`, `parse_object()`, and `parse_room()`, and creates a private
temporary directory containing `world/mob`, `world/obj`, and `world/wld` before calling
`save_mobiles()`, `save_objects()`, and `save_rooms()`.

Parser-backed scenarios run in bounded child processes because the production parsers retain private
static indexes. The parent creates and owns each private sandbox, waits for the child, and performs
checked cleanup even when the child exits or is signaled before fixture teardown. OLC tests use real
`medit_parse()`, `oedit_parse()`, and `redit_parse()` calls with isolated descriptor and
`oasis_olc_data` fixtures. Each scenario records failures, restores global state and the working
directory, and reports its result before the parent CuTest assertion runs. No production-only test
hook or application behavior change is required.

### Design Patterns

- Snapshot/restore fixture: Isolates the legacy global data model from other production-linked tests.
- Temporary filesystem sandbox: Executes actual relative-path OLC writers without touching
  `lib/world/` or checked-in data.
- Process-boundary isolation: Contains parser-local static indexes while parent ownership guarantees
  abnormal-exit sandbox cleanup.
- Characterization assertions: Freeze observable output and identity without making current table
  layout a permanent target architecture.

---

## 6. Deliverables

### Files To Create

| File | Purpose | Est. Lines |
|------|---------|------------|
| `unittests/CuTest/test_spec_fixtures.h` | Shared special-procedure test fixture API | ~120 |
| `unittests/CuTest/test_spec_fixtures.c` | Isolated globals, parser streams, sandbox, and OLC setup | ~450 |
| `unittests/CuTest/test_spec_registry_persistence.c` | Registry, load/save, inventory, and editor tests | ~350 |

### Files To Modify

| File | Changes | Est. Lines |
|------|---------|------------|
| `Makefile.am` | Add both new CuTest sources to compile and generated-test inputs | ~4 |
| `CMakeLists.txt` | Add both new CuTest sources to `CUTEST_TEST_SOURCES` | ~2 |

---

## 7. Success Criteria

### Functional Requirements

- [x] The suite proves the current 29-row name inventory, case-insensitive lookup, null and unknown
  rejection, reverse lookup, `Guild`/`Guildmaster` identity, and lower and sentinel bounds.
- [x] Known mobile, object, and room bindings load through production parsers and save with their
  current canonical names through production writers.
- [x] Medit, oedit, and redit select the same current registry entries, reject invalid indexes, and
  clear their temporary callback slots on selection zero.
- [x] The current five checked-in named bindings remain inventoried as one mobile, two object, and
  two room occurrences.

### Testing Requirements

- [x] New tests run from the generated root `cutest` executable.
- [x] Root `make test` passes, followed by a passing `make install`.
- [x] Test teardown leaves repository world data unchanged and no root-level `circle` artifact.

### Non-Functional Requirements

- [x] Fixture teardown restores global pointers, top indexes, descriptor buffers, and working
  directory on every exercised result path.
- [x] No production behavior, callback ABI, world-file syntax, registry name, or activation flag
  changes in this session.
- [x] Automake and CMake test membership remains synchronized.

### Quality Gates

- [x] All files ASCII-encoded.
- [x] Unix LF line endings.
- [x] Code follows project conventions.
- [x] Zero new `-Wall -Wextra` warnings.

---

## 8. Implementation Notes

### Working Assumptions

- Production paths can be characterized without adding application test seams: `parse_mobile()`,
  `parse_object()`, `parse_room()`, `save_mobiles()`, `save_objects()`, `save_rooms()`, and the three
  editor parsers are public, while their relative `world/` paths can be contained by changing into a
  temporary sandbox. This is supported by the traced declarations in `src/db.h`, `src/olc/*.h`, and
  the writers' `MOB_PREFIX`, `OBJ_PREFIX`, and `WLD_PREFIX` use.
- `Guild` is the current canonical save output for the shared `guild` pointer because it precedes
  `Guildmaster` in `spec_func_list[]`; the PRD explicitly retains this as the Phase 00 compatibility
  baseline pending an intentional registry redesign.

### Key Considerations

- Parser functions keep internal static indexes, so each parser-backed scenario runs in a bounded
  child process and no later test depends on resetting those private counters.
- Scenario checks record errors before fixture teardown; parent CuTest assertions run only after the
  child has exited and the parent has checked sandbox cleanup.
- Exact table count and ordering are characterization evidence, not permission for later code to
  keep aliases as duplicate definitions.

### Potential Challenges

- Legacy parsers exit on malformed records: Use minimal records copied from the current valid world
  grammar and validate fixture setup before testing policy assertions.
- OLC menu rendering touches broad prototype and descriptor state: Initialize all referenced strings,
  colors, zones, output buffers, and owner rnums in the reusable fixture.
- Writer functions use process-relative paths: Create all required sandbox directories before
  changing directory, restore the original directory during fixture teardown, and let the parent
  perform an idempotent checked cleanup after every child exit path.

### Behavioral Quality Focus

Checklist active: Yes
Top behavioral risks for this session:
- Shared global state or working-directory leakage can make later suites order-dependent.
- A failed setup or save can leave partial fixture state that hides the actual characterization
  result.
- Tests can accidentally inspect a reimplemented approximation instead of the production load,
  save, and OLC parser paths.

---

## 9. Testing Strategy

### Unit Tests

- Assert the full current indexed name sequence and count, null and unknown lookup, mixed-case
  lookup, reverse lookup, alias identity, and `-1`/`count` accessor behavior.

### Integration Tests

- Parse minimal valid named mobile, object, and room records through `src/db.c`.
- Save the parsed prototypes through all three production OLC writers into a private sandbox and
  assert exact current field syntax and canonical names.
- Drive all three editor parser branches through valid, invalid, quit, and clear inputs while
  checking their OLC callback slots and modification flags.

### Runtime Verification

- Run the generated root CuTest suite via `make test`, then install the tested binary with
  `make install`.

### Edge Cases

- Null, empty, mixed-case, and unknown registry names.
- Negative and sentinel registry indexes.
- Shared function pointer exposed by two accepted names.
- Invalid zero-based and one-based OLC bounds, explicit zero clear, and quit without mutation.
- Temporary directory, file, parser, or writer failure with full fixture restoration.

---

## 10. Dependencies

### Other Sessions

- Depends on: None.
- Depended by: Sessions 02 through 08 use the fixture and compatibility evidence; Session 09 audits
  the complete Phase 00 matrix.

---

## Next Steps

Session complete. Run `plansession` for Session 02.
