# Implementation Notes

**Session ID**: `phase00-session06-authored-binding-model`
**Started**: 2026-08-07
**Base Commit**: 66ba63a13eeb50b00be3a77f4f4a51653d665937
**Status**: Complete

---

## Planning And Trace

- Confirmed a clean development checkout at the published Session 05 commit.
- Traced named world bindings to `mob_index[].func`, `obj_index[].func`, and `world[].func`.
- Traced boot allocation/destruction, OLC prototype insertion/deletion, array shifts, and the generic
  room shallow-copy path.
- Traced editor setup, explicit selection/clear, internal save, and descriptor cleanup for all three
  owners.
- Kept writer changes, effective precedence, moving-room conflict policy, and final help updates in
  their scheduled Sessions 07-09.

## Frozen Ownership Decisions

- Prototype records own authored bindings; immutable definitions are borrowed from the registry.
- Requested names and source locations are always deep-owned, including unresolved records.
- OLC owns independent records and never aliases prototype state.
- The redit scratch room pointer is null; OLC state carries the working authored record separately.
- Replacement allocates first and releases old state only after complete success.
- Compatible canonical/alias records derive callbacks; unknown and incompatible records do not.

## Implementation Log

- Added `src/spec/spec_binding.h` and `src/spec/spec_binding.c` as the sole owned record API. The
  model retains requested text and source location, borrows immutable definitions, classifies
  canonical, alias, unknown, wrong-owner, and wrong-source requests, and exposes a handler only for
  resolved records.
- Added authored record pointers to mobile/object prototype indexes and rooms, plus independent
  mobile/object/room pointers in Oasis OLC state.
- Integrated mobile `SpecProc`, object `Z`, and room `Z` parsing through the owned model. Unknown
  and incompatible content now emits a bounded warning with persisted field, owner, VNUM, requested
  identity, and reason while retaining the record and installing no callback.
- Integrated boot destruction, mobile/object insertion and deletion, room insertion/deletion/copy,
  redit scratch ownership, and both generic OLC cleanup modes. Raw index and room shifts transfer
  pointer ownership without duplicating allocations.
- Integrated all three editor setup, explicit selection, clear, and internal-save paths. Existing
  records are deep-copied, selected records use canonical identity, and prototype saves prepare a
  copy before mutation.
- Parameterized the production loader fixture and added seven production-linked tests for canonical
  resolution, alias retention, unknown and owner/source incompatibility, bounded diagnostics,
  transactional replacement, self-copy, repeated cleanup, and three-owner OLC setup/save/clear.
- Synchronized Automake and CMake source membership and documented the in-memory authored state and
  deliberately deferred writer behavior in `docs/guides/OLC_SpecProcs.md`.
- Formal review repaired two partial-mutation risks, one structure-only cleanup leak, and two
  conservative analyzer paths. `code-review.md` is RESOLVED with no open finding.

## Verification Log

- Focused Autotools compilation completed with no new warning and all 536 CuTests pass.
- An independent CMake GNU C23 build compiled the complete `cutest` target and
  `production-cutest` passed in 20.10 seconds.
- Restricted changed-code `clang-tidy`, new-file `clang-format --dry-run --Werror`, manifest parity,
  world-data digest, and diff-hygiene checks pass.
- Full `make test` passed all seven auxiliary checks and 536 CuTests. `make install` activated
  release `75d7b8a0c8df499279f2009594f6b8026b6f0c46` and removed the root `circle` artifact.
- Final integrity, encoding, security, and protected-path checks pass. The checked-in world digest
  remains `28d30cda73e9dd19e6ea1bf85260aefba0f621d6144401eda421a9fca2be2d98`.
- Apex validation passed all gates; `validation.md` and `security-compliance.md` record the evidence.

## Next Command

Session 07 planning - Binding Round-Trip Persistence.
