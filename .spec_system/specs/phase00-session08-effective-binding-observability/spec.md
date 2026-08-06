# Session Specification

**Session ID**: `phase00-session08-effective-binding-observability`
**Phase**: 00 - Registry Safety and Observability
**Status**: Complete
**Created**: 2026-08-07
**Base Commit**: 2ee93973a299772dc29e301e04c7dfb98340ae01

---

## 1. Session Overview

This session records every callback contribution made while prototypes boot and emits structured
startup diagnostics that explain authored requests, source order, overrides, wrapper composition,
saved secondaries, and the final effective callback. The existing callback slots remain runtime
authority and retain their verified world, parser, legacy, shop, and quest precedence.

Rooms use the callback slot for both named procedures and the moving-room parser hook. Because one
slot cannot safely own both, production loading, room selection, internal save, and disk output
reject combined moving-room `M` data and named room `Z` ownership before the unsafe state can be
loaded or persisted.

---

## 2. Objectives

1. Attach owned effective-binding provenance to mobile, object, and room prototypes.
2. Record named world, parser-hook, hard-coded assignment, shop, and quest contributions in boot
   order without changing callback assignment behavior.
3. Make replacements, repeated handlers, wrapper composition, and saved secondaries explicit.
4. Emit deterministic normal and `no_specials` startup summaries for all contributing prototypes.
5. Reject moving-room and named room-procedure slot collisions in every load/edit/save boundary.
6. Prove precedence, diagnostics, secondaries, mode behavior, ownership, and rejection with
   production-linked tests.

---

## 3. Prerequisites

- Sessions 02 and 03 characterize scheduling, `no_specials`, combat, and secondary callbacks.
- Session 04 provides owner/source metadata, stable names, and registry validation.
- Sessions 06 and 07 provide owned authored identity and authored-first persistence.
- The checkout is a clean development environment at published commit `2ee93973`; protected local
  configuration, credentials, and checked-in world data remain outside the change surface.

---

## 4. Scope

### In Scope

- An owned contribution chain and effective winner for each prototype with a boot contribution.
- Source, identity, location, outcome, installed callback, and saved secondary provenance.
- Best-effort provenance recording that never changes legacy callback behavior on allocation error.
- Instrumentation of production world parsing, moving-room parsing, assignment helpers, shop
  wrappers, and quest wrappers.
- An unconditional post-assignment reporting point whose mode describes normal or `no_specials`.
- Loader rejection for both `M` then `Z` and `Z` then `M` field order.
- REdit selection/internal-save rejection and a preflight disk-writer rejection before mutation.
- Deep-copy/free lifecycle integration for prototype array and room-copy paths.
- Production-linked CuTests, synchronized build manifests, and operator/builder documentation.

### Out Of Scope

- Declarative conversion of the legacy assignment tables.
- Callback ABI, boot order, callback result, wrapper nesting, or secondary invocation changes.
- A multiple-handler runtime chain, typed dispatch gateway, or moving-room hook migration.
- Treating `no_specials` as a new global invocation gate.
- Phase-wide documentation replacement and final audit, which remain Session 09 work.

---

## 5. Technical Approach

### Effective Provenance Model

Each prototype owns a separate effective-binding record alongside its Session 06 authored record.
The record contains the owner, VNUM, ordered contributions, current winner, and collision count.
Each contribution owns stable single-line identity and source-location strings and records one exact
source bit, an outcome, the installed legacy handler, and an optional saved secondary handler and
identity. This model never replaces the callback slot and is not serialized as authored content.

Outcome calculation is deterministic: the first non-null handler is selected, an unresolved world
request is recorded without a handler, a different later handler overrides the current value, the
same handler is reasserted, and shop or quest installation is marked as a wrapper with the actual
saved secondary. A provenance allocation failure logs a bounded `SYSERR` while the existing
assignment still occurs.

### Production Contribution Boundaries

World loaders contribute the exact requested authored name and resolved handler. The room `M`
parser contributes `moving_rooms`. Legacy `ASSIGNMOB`, `ASSIGNOBJ`, and `ASSIGNROOM` calls retain
their table shape but route through helpers that also receive a stringized handler symbol and call
site. Shop and quest assignment functions record their installed wrapper plus the exact secondary
slot they save. The instrumentation follows, but does not reorder, each existing assignment.

After the guarded assignment block, boot unconditionally reports all recorded bindings. In normal
mode the report includes every source that actually ran. Under `-s`, world and parser records remain
visible while skipped shop loading and assignment sources remain absent. This is observation of the
existing mode path, not a runtime dispatch gate.

### Structured Diagnostics

Startup output uses one bounded line per contribution plus one final line per prototype. Stable
fields include mode, owner, VNUM, contribution index, source, requested/installed identity, source
location, outcome, optional secondary, final source/identity, and collision count. Text fields are
sanitized as single-line values when records are created so diagnostics cannot forge log lines.

### Moving-Room Collision Policy

The room loader refuses `M` when a named binding already exists and refuses `Z` when mover state is
already present. The check is order-independent and terminates startup with a specific room/VNUM
diagnostic. REdit refuses entry to or selection in the named SpecProc menu for a moving room and
defensively refuses an internal save containing both. The disk writer scans the entire target zone
before opening or mutating moving-room state and returns failure if any room would emit both `M` and
`Z`. Registered callback-only fallback ownership is included in that writer check.

### Verification

Tests exercise the real loader, assignment helper surface, wrapper functions, secondary slots,
formatters, report modes, prototype ownership, and room writer/editor boundaries. Conflicting
world records are parsed in child processes so expected fatal startup rejection cannot terminate
the suite. Existing characterization suites remain the compatibility oracle for invocation and
secondary behavior.

---

## 6. Deliverables

### Files To Create

| File | Purpose |
|------|---------|
| `src/spec/spec_effective_binding.h/.c` | Owned contribution, winner, formatting, and reporting model. |
| `unittests/CuTest/test_spec_effective_binding.c` | Provenance, precedence, diagnostics, mode, and collision tests. |
| `sql/components/help_specproc_entries.sql` | Authoritative builder-facing SpecProc help migration. |
| `sql/components/verify_help_specproc_entries.sql` | Read-only help migration verification queries. |

### Files To Modify

| File | Change |
|------|--------|
| `src/structs.h`, prototype lifecycle paths | Attach, copy, initialize, and free effective records. |
| `src/db.c` | Record world/parser contributions, reject load conflicts, and report after assignment. |
| `src/spec_assign.c` | Record every successful hard-coded assignment with symbol and call site. |
| `src/obj/shop.c`, `src/quest/quest.c` | Record wrapper installation and actual saved secondaries. |
| `src/olc/redit.c`, `src/olc/genwld.c` | Reject moving-room selection/internal save/disk persistence conflicts. |
| `unittests/CuTest/test_spec_fixtures.c/.h` | Expose bounded effective-state and room-conflict fixture controls. |
| `Makefile.am`, `CMakeLists.txt` | Add production and test sources to both build systems. |
| `sql/components/ci_schema_manifest.txt` | Classify the help migration and read-only verifier for schema CI. |
| `docs/guides/OLC_SpecProcs.md` | Document startup provenance and moving-room ownership safety. |
| `scripts/world/wtool_lib/spec_registry.py` | Consume canonical definitions and aliases from the current registry. |
| `scripts/world/tests/test_constants.py` | Cover current-registry extraction and malformed/commented source handling. |

---

## 7. Success Criteria

### Functional

- [x] Every successful boot contribution is ordered and attributable by owner, VNUM, source,
      identity, and source location.
- [x] Final callbacks and shop/quest secondaries remain byte-for-byte behaviorally equivalent to
      the characterized boot sequence.
- [x] Normal summaries contain the sources that ran; `no_specials` summaries contain only the
      loader/parser sources that actually ran.
- [x] Unknown authored requests remain visible even when they install no handler.
- [x] Both field orders of room `M` plus `Z` are rejected during production loading.
- [x] REdit and the disk writer cannot select or persist named room ownership on a moving room.

### Compatibility And Safety

- [x] Callback ABI, single-slot runtime dispatch, boot precedence, wrapper nesting, and
      `no_specials` semantics are unchanged.
- [x] Provenance ownership survives prototype moves and room copies without leak, alias, or double
      free.
- [x] Diagnostic identities and locations cannot inject additional log lines.
- [x] Both build systems compile without new warnings and the complete suite passes.

---

## 8. Validation Plan

1. Compile the effective model and each instrumented production translation unit.
2. Run focused tests for contribution outcomes, copying, sanitization, formatting, normal and
   `no_specials` mode labels, assignment precedence, wrappers, and secondaries.
3. Run child-process production loader tests for both moving-room conflict orders and production
   writer/editor rejection tests with output-integrity checks.
4. Run `make test`, followed by `make install`, and an independent CMake/CTest build.
5. Run formatting, diff hygiene, ASCII/LF, manifest parity, protected-path, credential, unsafe-API,
   temporary-sandbox, root-artifact, and world-data integrity checks.
6. Run Apex `creview`, repair every material finding, rerun affected checks, then run Apex
   `validate` and `updateprd` gates.

---

## 9. Risks And Mitigations

| Risk | Mitigation |
|------|------------|
| Instrumentation changes assignment behavior. | Record best-effort around the existing assignment and preserve it on failure. |
| Secondary diagnostics describe a different callback than wrappers invoke. | Record the actual `SHOP_FUNC` or `QST_FUNC` slot after the existing save logic. |
| Unknown legacy callbacks have no registry identity. | Capture the stringized assignment symbol at the production call site. |
| Diagnostics become unbounded or forge lines. | Own validated single-line strings and emit one bounded record per line. |
| Writer rejection occurs after moving-room mutation. | Preflight the whole zone before opening output or unlinking a mover. |
| OLC clear removes the moving-room system callback. | Refuse the named SpecProc menu for mover-owned rooms before selection or clear. |
| Prototype copies alias contribution lists. | Provide deep-copy/free APIs and integrate every existing authored-binding lifecycle path. |

---

## 10. Next Step

Plan Session 09: Documentation and Phase Validation.
