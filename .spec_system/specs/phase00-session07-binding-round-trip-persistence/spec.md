# Session Specification

**Session ID**: `phase00-session07-binding-round-trip-persistence`
**Phase**: 00 - Registry Safety and Observability
**Status**: Complete
**Created**: 2026-08-07
**Base Commit**: 355152c102e885d75ff9372c626017e9546d5e95

---

## 1. Session Overview

This session makes the mobile, object, and room OLC writers persist the owned authored world
binding when one exists. Exact unresolved names and loaded compatibility aliases survive unrelated
editor saves and later callback overrides. Reverse function-pointer lookup remains only for legacy
in-memory prototypes that have no authored record.

World-file grammar, callback slots, boot assignment order, shops, quests, parser hooks, and runtime
dispatch remain unchanged.

---

## 2. Objectives

1. Make every named world writer prefer authored world identity over the effective callback.
2. Preserve exact loaded aliases, unresolved names, and incompatible names across unrelated saves.
3. Keep explicit registry selections canonical and explicit clear operations absent on disk.
4. Prevent later hard-coded callback overrides from becoming authored world content.
5. Prove all three formats through writer and fresh-process production-loader round trips.

---

## 3. Prerequisites

- Session 06 is complete and provides owned prototype and OLC authored-binding records.
- Session 05 is complete and provides compatible canonical select, replace, and clear flows.
- Session 01 provides isolated production parser/writer fixtures and the persistence baseline.
- The checkout is a development environment at clean published commit `355152c1`; protected local
  configuration and production world data are not modified.

---

## 4. Scope

### In Scope

- A narrow binding API that returns a valid world-authored name for persistence.
- Authored-state-first mobile `SpecProc`, object `Z`, and room `Z` writers.
- Pointer reverse lookup only when the prototype has no authored record.
- Exact loaded alias and unresolved/incompatible name retention.
- Canonical explicit selection and absent explicit clear output.
- Fresh-process load-edit-save-reload tests for all three owner formats.
- Compatibility coverage for legacy callback-only prototypes.

### Out Of Scope

- World-file grammar changes or multiple world names per prototype.
- Effective source contribution records, boot precedence summaries, or collision diagnostics.
- Moving-room `M` plus room `Z` conflict policy.
- Shop, quest, parser-hook, hard-coded assignment, or runtime invocation changes.
- Final builder help and architecture documentation replacement scheduled for Session 09.

---

## 5. Technical Approach

### Persistence Identity Contract

`spec_binding_persisted_name()` returns the exact requested name only for a valid world-authored
record. It does not canonicalize aliases and does not require the record to resolve, because both
properties would destroy authored intent. A null record means no authored state is available.

Each writer follows one deterministic decision:

1. If an authored record exists, consult it and never inspect the effective callback for identity.
2. If no authored record exists, retain the legacy reverse function-pointer lookup fallback.
3. If neither produces a name, omit the existing single-name field.

Keeping the fallback conditional on record absence prevents a malformed or unresolved authored
record from promoting an unrelated callback. It also preserves callback-only prototypes created by
legacy code paths.

### Editor Semantics

Session 06 already copies authored records through medit, oedit, and redit. An unrelated internal
save therefore retains the same requested name even when a later boot source changed the callback.
An explicit selector choice replaces the record with the registry canonical name. An explicit
clear frees the record and clears the callback, so the writer omits the field. Unknown and
incompatible records remain until one of those explicit actions occurs.

### Round-Trip Verification

The shared fixture gains two bounded capabilities: install a different effective callback without
changing authored state, and parse the files emitted by the three production writers. Each
round-trip scenario creates an unparsed fixture, forks a writer child, and then parses the emitted
files in the untouched parent process. The sibling parser lifecycle avoids the production parsers'
static record counters while exercising real mobile, object, and room syntax.

Tests cover canonical records, a stable loaded alias, unresolved names, effective overrides,
unrelated OLC saves, explicit canonical replacement, explicit clear, and callback-only legacy
fallback. Output text is checked before reload, and reloaded authored records and callbacks are
checked afterward.

---

## 6. Deliverables

### Files To Create

| File | Purpose |
|------|---------|
| `unittests/CuTest/test_spec_binding_round_trip.c` | Three-owner persistence and reload tests. |

### Files To Modify

| File | Change |
|------|--------|
| `src/spec/spec_binding.h/.c` | Expose validated world-authored persistence identity. |
| `src/olc/genmob.c` | Prefer mobile authored identity in `SpecProc` output. |
| `src/olc/genobj.c` | Prefer object authored identity in `Z` output. |
| `src/olc/genwld.c` | Prefer room authored identity in `Z` output. |
| `unittests/CuTest/test_spec_fixtures.c/.h` | Support callback overrides and emitted-file reload. |
| `Makefile.am`, `CMakeLists.txt` | Add the new production-linked test source to both manifests. |
| `docs/guides/OLC_SpecProcs.md` | Document authored-first round-trip and explicit action behavior. |

---

## 7. Success Criteria

### Functional

- [x] Unrelated saves retain exact unresolved and incompatible authored names for every owner.
- [x] A loaded compatibility alias remains stable across an unrelated save and reload.
- [x] Later effective callback overrides never replace an available authored world identity.
- [x] Explicit selections write canonical names and explicit clears remove fields for every owner.
- [x] Callback-only prototypes retain the legacy pointer reverse-lookup behavior.

### Compatibility And Safety

- [x] Existing mobile, object, and room single-name syntax is unchanged.
- [x] Reloaded resolved records install the same callbacks as direct boot parsing.
- [x] Reloaded unresolved records retain owned identity and install no callback.
- [x] Fresh-process fixture lifecycles leave no root artifact or temporary sandbox.
- [x] Both build systems compile without new warnings and the full production-linked suite passes.

---

## 8. Validation Plan

1. Compile the binding module and three writer translation units.
2. Run focused authored-persistence CuTests for API, alias, unknown, override, select, clear, and
   callback-only fallback behavior across all owners.
3. Inspect production-emitted text and parse it in a fresh sibling process for each scenario.
4. Run `make test`, followed by `make install`, and an independent CMake/CTest build.
5. Run formatting, diff hygiene, ASCII/LF, manifest parity, protected-path, credential, unsafe-API,
   temporary-sandbox, root-artifact, and world-data integrity checks.
6. Run Apex `creview`, repair every material finding, rerun affected tests, then run Apex
   `validate` and `updateprd` gates.

---

## 9. Risks And Mitigations

| Risk | Mitigation |
|------|------------|
| Effective callback is accidentally serialized. | Reverse lookup only when the authored pointer itself is null. |
| Alias preservation conflicts with canonical output. | Loaded aliases retain exact text; explicit menu selections store canonical identity. |
| Unknown record disappears because its callback is null. | Writers key authored output on record presence, not handler availability. |
| Parser static counters invalidate reload tests. | Fork the writer from an unparsed fixture and reload in the untouched parent process. |
| Legacy callback-only prototypes stop persisting. | Keep and directly test the pointer reverse-lookup fallback. |
| Test files leak into the repository. | Use the existing private `/tmp` sandbox cleanup contract and verify cleanup. |

---

## 10. Next Step

Run `plansession` for Session 08: Effective Binding Observability.
