# PRD Phase 00: Registry Safety and Observability

**Status**: In Progress
**Sessions**: 9 (initial estimate)
**Estimated Duration**: 3-5 working days

**Progress**: 8/9 sessions (89%)

---

## Overview

Phase 00 makes the existing special-procedure registry and binding control plane safe,
observable, and testable before runtime dispatch is migrated. It freezes legacy behavior in
production-linked tests, introduces validated immutable definition metadata, restricts OLC to
compatible procedures, preserves authored and unresolved identities, and explains effective
post-boot bindings without changing the SPECIAL callback ABI, single-handler storage, world-file
syntax, activation behavior, or boot precedence.

---

## Progress Tracker

| Session | Name | Status | Est. Tasks | Validated |
|---------|------|--------|------------|-----------|
| 01 | Registry and Persistence Characterization | Complete | 23 | 2026-08-06 |
| 02 | Command and Pulse Characterization | Complete | 24 | 2026-08-06 |
| 03 | Combat and Secondary Characterization | Complete | 22 | 2026-08-06 |
| 04 | Validated Definition Registry | Complete | 24 | 2026-08-07 |
| 05 | Owner-Aware OLC | Complete | 23 | 2026-08-07 |
| 06 | Authored Binding Model | Complete | 24 | 2026-08-07 |
| 07 | Binding Round-Trip Persistence | Complete | 21 | 2026-08-07 |
| 08 | Effective Binding Observability | Complete | 24 | 2026-08-07 |
| 09 | Documentation and Phase Validation | Not Started | ~14-18 | - |

---

## Completed Sessions

- Session 01: Registry and Persistence Characterization - completed 2026-08-06
- Session 02: Command and Pulse Characterization - completed 2026-08-06
- Session 03: Combat and Secondary Characterization - completed 2026-08-06
- Session 04: Validated Definition Registry - completed 2026-08-07
- Session 05: Owner-Aware OLC - completed 2026-08-07
- Session 06: Authored Binding Model - completed 2026-08-07
- Session 07: Binding Round-Trip Persistence - completed 2026-08-07
- Session 08: Effective Binding Observability - completed 2026-08-07

---

## Upcoming Sessions

- Session 09: Documentation and Phase Validation

---

## Objectives

1. Freeze registry, persistence, invocation, scheduling, activation, and return behavior in
   production-linked tests before changing control-plane structures.
2. Establish immutable definitions with canonical identity, aliases, owner and event contracts,
   visibility, prerequisites, descriptions, categories, and boot-time validation.
3. Make mobile, object, and room OLC selection compatible, descriptive, and explicit about runtime
   prerequisites.
4. Preserve authored and unresolved binding identities through load, edit, save, and boot-time
   overrides while exposing provenance and collision outcomes.
5. Reject incompatible moving-room and room-procedure ownership and align builder, help, developer,
   and architecture documentation with delivered behavior.

---

## Prerequisites

- The master special-procedure architecture PRD and its verified current-state evidence exist.
- The current legacy callback ABI, world-file syntax, activation flags, and boot order are the
  compatibility baseline.
- The root production-linked CuTest suite, Autotools build, CMake manifest, and MariaDB-backed test
  environment remain available.

---

## Planning Assumptions And Resolutions

### Working Assumptions

- Nine sessions are required for Phase 00. The master PRD deliberately combines behavior
  characterization and registry/binding control-plane safety into the first delivery. Nine bounded
  objectives preserve the 12-25 task and 2-4 hour session limits better than combining provenance,
  persistence, and diagnostics into oversized sessions.
- The PRD evidence verified at commit af9f79d2 remains a valid planning baseline. Comparison through
  the current HEAD found no implementation changes in the traced registry, call-site, OLC, shop,
  quest, or CuTest paths, so phase planning can proceed without re-scoping completed work.

### Conflict Resolutions

- The initialized state and placeholder phase file call Phase 00 "Foundation" with zero sessions,
  while the later generated master PRD explicitly defines Phase 00 as Registry Safety and
  Observability and instructs phasebuild to replace the placeholder. The generated PRD is the
  intended scope source; this phase PRD, state tracking, and the master phase table are reconciled
  to that definition and nine sessions.
- The master PRD leaves preservation versus save blocking open for unresolved names, but its binding
  contract requires owned unresolved raw identity and its MVP requires unrelated OLC saves not to
  erase names. Phase 00 therefore preserves unresolved authored names and reserves explicit replace
  or clear actions for builders instead of making save refusal the primary design.

---

## Technical Considerations

### Architecture

Phase 00 separates definition metadata, authored binding state, and effective binding diagnostics
while leaving invocation on the existing SPECIAL ABI. Existing mobile, object, and room callback
slots remain the runtime authority. World loaders, OLC copies, legacy assignments, parser hooks,
shops, and quests must contribute traceable binding sources without changing their current outcome.
Runtime gateways and typed handlers remain Phase 01 and later work.

### Technologies

- GNU C23 and the established CircleMUD/tbaMUD data model
- Root production-linked CuTest suite with LUMINARI_CUTEST
- GNU Autotools and Automake, with matching CMake source membership
- Oasis OLC and existing mobile, object, and room world-file formats
- MariaDB/MySQL-backed integration environment

### Risks

- Characterization gaps: Cover all verified invocation categories, exact tokens, return handling,
  traversal order, pulse positions, flags, and normal versus -s behavior before claiming a freeze.
- Persisted identity drift: Keep canonical names and explicit aliases stable, and store authored
  identity independently from effective function pointers.
- Lifetime and ownership errors: Give unresolved names explicit copy and free rules across prototype
  and OLC lifecycles, with production-linked round-trip tests.
- Hidden precedence changes: Observe world, parser-hook, hard-coded, shop, and quest sources without
  changing the verified boot sequence or saved secondary callbacks.
- Moving-room payload collision: Reject combined M and Z room ownership until relocation receives a
  separate typed hook in a later phase.
- Build drift: Update both Makefile.am and CMakeLists.txt for every source or test membership
  change, then run make test followed by make install.

---

## Success Criteria

Phase complete when:

- [ ] All 9 sessions are completed and validated.
- [ ] All verified invocation categories and registry compatibility behaviors have production-linked
  characterization coverage.
- [ ] Every persisted definition has valid canonical identity, aliases, owner and event metadata,
  visibility, prerequisites, category, and a non-empty description.
- [ ] Invalid definition metadata fails before world parsing, and accessors are type-aware and
  bounds-safe.
- [ ] Medit, oedit, and redit show only compatible definitions and explain event and placement
  prerequisites.
- [ ] Known, aliased, incompatible, and unresolved authored identities survive their defined load,
  edit, save, and explicit clear or replace paths.
- [ ] Effective sources, collisions, and shop or quest secondary callbacks are diagnosable without
  changing current precedence.
- [ ] Combined moving-room M data and room Z binding are rejected safely.
- [ ] Root make test and make install pass with no root-level circle artifact, and documentation
  matches implemented behavior.

---

## Dependencies

### Depends On

- The approved master PRD and the current legacy special-procedure implementation baseline.

### Enables

- Phase 01: Call-Site Gateway Compatibility
