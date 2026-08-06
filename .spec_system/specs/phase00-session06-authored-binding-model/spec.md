# Session Specification

**Session ID**: `phase00-session06-authored-binding-model`
**Phase**: 00 - Registry Safety and Observability
**Status**: Complete
**Created**: 2026-08-07
**Base Commit**: 66ba63a13eeb50b00be3a77f4f4a51653d665937

---

## 1. Session Overview

This session introduces an owned authored-binding record for named mobile, object, and room world
bindings. The record retains the requested text and provenance independently of the effective
legacy callback slot, resolves canonical names and aliases through the validated registry, and
classifies unknown, owner-incompatible, and source-incompatible requests without discarding them.

The three loaders populate authored records directly on prototypes. The three OLC editors clone,
replace, clear, save, and free independent working records. Writers continue using the existing
callback reverse lookup until Session 07, and hard-coded assignment, shop, quest, parser-hook, and
runtime dispatch behavior remain outside this session.

---

## 2. Objectives

1. Retain owner, prototype VNUM, requested persisted name, resolved definition, source kind, and
   source location for every named world binding.
2. Preserve the exact requested alias while resolving it to the canonical immutable definition.
3. Keep unknown and incompatible requests owned and diagnosable while preventing an incompatible
   legacy callback from entering the prototype slot.
4. Make replacement and copy operations transactional and cleanup idempotent across prototype and
   OLC lifecycles.
5. Prove loader, copy, replacement, and cleanup behavior for mobile, object, and room owners.

---

## 3. Prerequisites

- Session 04 is complete and provides boot-validated canonical, alias, owner, source, and handler
  metadata.
- Session 05 is complete and provides the shared owner-filtered OLC selection contract.
- Session 01 provides isolated production loader/editor fixtures and frozen current persistence
  behavior.
- The checkout is a development environment at clean published commit `66ba63a1`; protected local
  configuration and production data are not modified.

---

## 4. Scope

### In Scope

- `src/spec/spec_binding.h` and `src/spec/spec_binding.c` as the sole ownership and resolution API.
- An authored-binding pointer on mobile/object index entries and rooms.
- Independent mobile/object/room authored-binding pointers on OLC descriptor state.
- Loader creation for mobile `SpecProc`, object `Z`, and room `Z` records.
- Context-rich diagnostics for unknown, wrong-owner, and wrong-source requests.
- Compatible canonical and alias callback resolution through the authored record.
- Prototype destruction, OLC cleanup, OLC setup/save, prototype insertion/deletion, and room-copy
  ownership paths.
- Production-linked tests for all resolution classes and all three owner lifecycles.

### Out Of Scope

- Writer preference for authored identity or unresolved-name round trips, which are Session 07.
- Effective source contribution records, precedence, collision summaries, and secondary callbacks,
  which are Session 08.
- Moving-room `M` plus room `Z` rejection, which is coordinated with effective ownership in
  Session 08.
- Legacy assignment, shop, quest, parser-hook, dispatch, or world-file syntax changes.
- User-facing final SPECIALS help replacement, which is Session 09.

---

## 5. Technical Approach

### Authored Binding Contract

`struct spec_binding` owns `requested_name` and `source_location`. It borrows the immutable registry
definition and stores one exact owner bit, the unsigned prototype VNUM, one exact binding-source
bit, and one resolution state:

- resolved;
- unknown name;
- incompatible owner; or
- incompatible source.

No record represents an absent authored binding. A resolved record exposes its legacy handler;
every unresolved or incompatible state exposes a null handler. Known aliases keep the original
requested string while borrowing the canonical definition.

`spec_binding_replace()` validates inputs and fully allocates a replacement before freeing the old
record. `spec_binding_copy()` provides the same allocate-before-release guarantee, including
self-copy. `spec_binding_free()` accepts null state and nulls the caller's pointer. Bounded error and
diagnostic formatting never dereferences a missing field.

### Prototype Storage And Load

`struct index_data` receives one authored-binding pointer used by mobile and object prototype
indexes; trigger indexes leave it null. `struct room_data` receives one pointer adjacent to its
legacy callback slot. Existing zero-initialized boot allocation remains valid.

Each named world loader replaces the prototype record, emits one content diagnostic for an unknown
or incompatible request, and derives the legacy callback from the record. Compatible canonical and
alias requests retain existing callback behavior. Unknown requests remain null as before. A known
wrong-owner request is now explicitly rejected and cannot populate an incompatible callback.

Source locations identify the concrete persisted field (`mobile SpecProc`, `object Z`, or the room
loader filename and `Z` field); diagnostics also include owner, VNUM, requested text, and `world`
source.

### OLC Ownership

`struct oasis_olc_data` receives independent mobile, object, and room binding pointers. Existing
editor setup clones the corresponding prototype record. New editors start with null records.

Clear frees only the matching working record and nulls its callback. A valid selection
transactionally replaces the working record with the canonical name and editor source location,
then updates the callback. Internal save copies the working record back to the prototype after the
prototype insert/update succeeds. Generic OLC cleanup frees all three pointers regardless of editor
type.

The redit scratch `struct room_data` never owns the prototype authored record: setup nulls the
pointer immediately after the existing shallow copy, and internal save moves authored state only
through the independent OLC pointer. Generic room copy/insert/delete/free paths explicitly clone or
free records so future callers cannot alias owned strings.

### Lifecycle Integration

- Boot destruction frees room, object-index, and mobile-index records before their arrays.
- New mobile/object index slots initialize the record pointer to null.
- Mobile/object deletion frees the removed record before shifting index entries.
- Room deletion frees the removed record before shifting rooms.
- Room copy clones the source record before releasing the destination and performs no partial
  mutation when cloning fails.
- OLC clone/save failures are logged as `SYSERR` and preserve the prior owned record.

---

## 6. Deliverables

### Files To Create

| File | Purpose |
|------|---------|
| `src/spec/spec_binding.h` | Owned record, resolution, copy/free, handler, and diagnostic API. |
| `src/spec/spec_binding.c` | Transactional ownership and registry-backed resolution. |
| `unittests/CuTest/test_spec_authored_bindings.c` | Three-owner model, loader, OLC, and cleanup tests. |

### Files To Modify

| File | Change |
|------|--------|
| `src/structs.h`, `src/olc/oasis.h` | Add prototype and OLC binding pointers and access macros. |
| `src/db.c` | Populate loader records, diagnose invalid content, and destroy records. |
| `src/olc/medit.c`, `oedit.c`, `redit.c` | Clone, replace, clear, and save working records. |
| `src/olc/oasis.c` | Free all OLC authored records. |
| `src/olc/genmob.c`, `genobj.c`, `genwld.c` | Initialize, shift, copy, and free owned records safely. |
| `unittests/CuTest/test_spec_fixtures.c/.h` | Parameterize named records and expose binding state. |
| `Makefile.am`, `CMakeLists.txt` | Keep production and test source membership synchronized. |
| `docs/guides/OLC_SpecProcs.md` | Document in-memory authored identity and deferred writer behavior. |

---

## 7. Success Criteria

### Functional

- [x] Every compatible named world record has owned authored metadata and the expected callback.
- [x] An alias retains its requested text and resolves to the canonical definition.
- [x] Unknown names remain owned with null callbacks and complete diagnostics for every owner.
- [x] Wrong-owner names remain owned, are diagnosed, and never populate a callback.
- [x] Explicit OLC selection, clear, setup, and internal save keep callback and authored state in
  sync for all owners.

### Ownership And Safety

- [x] Copy results own distinct strings but borrow the same immutable definition.
- [x] Failed replacement leaves the prior record unchanged.
- [x] Repeated free and cleanup are safe.
- [x] Prototype insertion, deletion, destruction, room copying, and OLC teardown do not leak,
  alias, or double-free authored records.

### Compatibility

- [x] World-file grammar and writers are unchanged in this session.
- [x] Compatible canonical/alias and unknown callback outcomes remain characterized.
- [x] Hard-coded assignments, shops, quests, parser hooks, and runtime dispatch are unchanged.
- [x] Both build systems compile with no new warnings and the full production-linked suite passes.

---

## 8. Validation Plan

1. Compile the binding module and the changed loader/editor/lifecycle translation units.
2. Run a dedicated production-linked authored-binding suite covering model validation, canonical,
   alias, unknown, owner mismatch, source mismatch, copy, replacement, and repeated cleanup.
3. Run isolated production loaders with parameterized mobile/object/room names and inspect both
   callback slots and prototype records.
4. Exercise setup, selection, clear, internal save, and cleanup through all three production OLC
   paths.
5. Run `make test`, `make install`, an independent CMake/CTest build, formatting/diff checks,
   changed-file ASCII/LF checks, credential and unsafe-API scans, and world-data integrity checks.
6. Run the Apex `creview` gate, repair every material finding, rerun affected tests, then run the
   Apex `validate` and `updateprd` gates.

---

## 9. Risks And Mitigations

| Risk | Mitigation |
|------|------------|
| Shallow room copies alias an owned record. | Keep redit state independent and clone explicitly in generic room copy/insert paths. |
| Index shifts leak or duplicate ownership. | Free only the removed slot before shifting; moved pointers retain single ownership. |
| Unknown text is lost on lookup. | Allocate requested text before resolution and retain it for every resolution state. |
| Wrong-owner names silently execute. | Resolve definition identity separately, classify incompatibility, and return a null handler. |
| Allocation failure corrupts editor/prototype state. | Build replacements before release and preserve the old record on every failure. |
| Session 06 accidentally changes persisted output. | Keep all writers untouched and assert the Session 01 compatibility baseline. |

---

## 10. Next Step

Run `implement` for Session 06 using `tasks.md` in task order.
