# Session 01: Registry and Persistence Characterization

**Session ID**: `phase00-session01-registry-and-persistence-characterization`
**Status**: Complete
**Completed**: 2026-08-06
**Tasks**: 23
**Estimated Duration**: 2-4 hours

---

## Objective

Establish a production-linked regression baseline for current registry identity, lookup, accessor,
world-binding, and OLC persistence behavior before the registry data model changes.

---

## Scope

### In Scope (MVP)

- Add focused CuTest coverage for registry count, case-insensitive lookup, null and unknown input,
  reverse lookup, both accessor boundaries, and the Guild and Guildmaster shared handler identity.
- Capture current canonical save behavior without relying on function-pointer ordering as a future
  contract.
- Exercise known mobile, object, and room world bindings through existing load and save paths.
- Exercise existing medit, oedit, and redit select and clear behavior through production code.
- Record the current named-binding inventory and reusable fixtures needed by later Phase 00 tests.
- Keep Makefile.am and CMakeLists.txt test membership synchronized.

### Out of Scope

- Definition metadata redesign or boot-time metadata validation.
- Runtime invocation, scheduling, activation-flag, and no_specials characterization.
- OLC owner filtering, binding provenance, or effective-source diagnostics.

---

## Prerequisites

- [x] The master PRD remains the compatibility source for stable persisted names and world syntax.
- [x] The root production-linked CuTest harness builds against the current game sources.

---

## Deliverables

1. A production-linked registry and persistence characterization suite.
2. Reusable mobile, object, room, and OLC fixtures for later binding tests.
3. A checked inventory of current registered names, aliases, and named world bindings.
4. Matching Automake and CMake test-source membership.

---

## Success Criteria

- [x] Lookup, unknown-name, reverse-lookup, alias, and boundary behavior is executable as tests.
- [x] Known mobile, object, and room bindings load and save through production paths.
- [x] OLC select and clear behavior is characterized without introducing new policy.
- [x] The suite passes in the root test binary with zero new compiler warnings.
