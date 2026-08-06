# Task Checklist

**Session ID**: `phase00-session06-authored-binding-model`
**Total Tasks**: 24
**Estimated Duration**: 2-4 hours
**Created**: 2026-08-07

---

Legend: `[x]` completed; `[ ]` pending; `[P]` parallelizable; `[SNNMM]` session ref; `TNNN` task ID.

---

## Setup And Trace (4 tasks)

- [x] T001 [S0006] Verify the clean development base, Session 04/05 prerequisites, and Apex state
- [x] T002 [S0006] Trace mobile, object, and room named-loader callback assignments
- [x] T003 [S0006] Trace prototype insertion, shift, deletion, destroy, room-copy, and OLC ownership
- [x] T004 [S0006] Freeze resolution, diagnostic, compatibility, and deferred-writer contracts

## Owned Binding Model (5 tasks)

- [x] T005 [S0006] Define record fields, resolution states, and bounded public API
- [x] T006 [S0006] Implement canonical/alias resolution and owner/source classification
- [x] T007 [S0006] Implement transactional replace, deep copy, self-copy, and idempotent free
- [x] T008 [S0006] Implement effective legacy-handler derivation and stable source/status names
- [x] T009 [S0006] Implement bounded context-rich content diagnostics

## Prototype And Loader Integration (5 tasks)

- [x] T010 [S0006] Add authored records to mobile/object index entries and room prototypes
- [x] T011 [S0006] Populate and diagnose mobile `SpecProc`, object `Z`, and room `Z` records
- [x] T012 [S0006] Initialize new index slots and preserve single ownership through shifts
- [x] T013 [S0006] Free removed and boot-destroyed records for all three owners
- [x] T014 [S0006] Make room copy, insert, delete, and free paths ownership-safe

## OLC Lifecycle Integration (4 tasks)

- [x] T015 [S0006] Add independent mobile, object, and room binding pointers to OLC state
- [x] T016 [S0006] Deep-copy existing prototype records during all three editor setup paths
- [x] T017 [S0006] Transactionally replace/clear records during all three owner selections
- [x] T018 [S0006] Deep-copy records on internal save and free them during generic OLC cleanup

## Production-Linked Verification (3 tasks)

- [x] T019 [S0006] Parameterize shared loader fixtures and expose prototype/OLC authored state
- [x] T020 [S0006] Test canonical, alias, unknown, owner/source mismatch, diagnostics, and callbacks
- [x] T021 [S0006] Test independent copy, replacement, self-copy, editor save/clear, and cleanup paths

## Integration And Completion (3 tasks)

- [x] T022 [S0006] Synchronize build manifests and update the builder architecture guide
- [x] T023 [S0006] Pass targeted compilation/runtime/static checks and the Apex `creview` gate
- [x] T024 [S0006] Pass full builds/tests/install/integrity/security and the Apex `validate` gate

## Completion Checklist

- [x] All 24 tasks marked `[x]`
- [x] All tests and checks passing
- [x] All changed text is ASCII with LF endings
- [x] implementation-notes.md updated
- [x] `creview` and `validate` gates passed

## Next Step

After completion, run `plansession` for Session 07.
