# Implementation Notes

**Session ID**: `phase00-session07-binding-round-trip-persistence`
**Started**: 2026-08-07
**Base Commit**: 355152c102e885d75ff9372c626017e9546d5e95
**Status**: Complete

---

## Planning And Trace

- Confirmed a clean development checkout at the published Session 06 commit.
- Confirmed Session 05 editor selection and Session 06 authored ownership prerequisites are
  complete.
- Traced all three writers: each currently reverse-resolves only the effective callback pointer.
- Traced existing OLC setup and internal save paths; they already deep-copy authored state without
  requiring further lifecycle changes.
- Froze exact loaded aliases and unresolved names as stable authored content, canonical selector
  choices as replacement content, and null record plus null callback as explicit clear.
- Kept effective precedence, source summaries, collision diagnostics, and moving-room policy in
  Session 08.

## Frozen Persistence Decisions

- A present authored world record is the sole persistence identity, regardless of resolution or
  effective callback state.
- Loaded aliases preserve exact requested spelling across unrelated saves.
- Explicit menu selections persist the canonical registry name already stored by Session 06.
- Explicit clear removes both working authored state and callback, so no field is written.
- Function-pointer reverse lookup runs only when the authored pointer is absent.
- Tests reload production-emitted files in an untouched sibling parser lifecycle.

## Implementation Log

- Added `spec_binding_persisted_name()` as the output boundary for exact single-line world-authored
  identity. Resolved, alias, unknown, and incompatible world records return requested text;
  non-world, empty, and multi-line records do not.
- Updated all three production writers to use a present authored record without consulting the
  effective callback. Reverse lookup remains only when the authored pointer is null.
- Extended the shared fixture to override effective callbacks independently, capture production
  output, and reload emitted mobile, object, and room records.
- Added a seven-test production-linked suite. Fresh sibling parser lifecycles cover loaded aliases,
  canonical names, unknown and incompatible records, effective overrides, unrelated editor saves,
  explicit canonical selection, explicit clear, callback-only fallback, and output-boundary input
  validation.
- Added the suite to both Automake lists and CMake, and updated the builder guide with delivered
  authored-first persistence semantics.
- Apex review found and fixed one defense-in-depth output issue: direct model callers could create a
  multi-line requested name even though production loaders and OLC could not. Persistence now
  rejects CR/LF names and regression coverage freezes that boundary.

## Verification Log

- Apex prerequisite checks pass in the development environment.
- Base HEAD and upstream both resolve to `355152c102e885d75ff9372c626017e9546d5e95`.
- Focused Autotools compilation completes with no new warning and all 543 CuTests pass.
- Formatter, diff-hygiene, ASCII/LF, manifest-parity, protected-path, unsafe-API, sandbox-cleanup,
  root-artifact, and world-data checks pass.
- The checked-in world digest remains
  `28d30cda73e9dd19e6ea1bf85260aefba0f621d6144401eda421a9fca2be2d98`.
- Restricted analyzer review found only inherited whole-file diagnostics in untouched writer lines;
  changed-code inspection has no open finding.
- `code-review.md` is RESOLVED with no open Critical, High, Medium, or Low finding.
- Full `make test` passed all seven auxiliary checks and 543/543 production-linked CuTests.
- `make install` activated release `3d8d23672b3a49be41dfc6fb58e20eeb6981e5d5` and removed
  the root `circle` artifact.
- An initial fresh CMake configure correctly honored the default `BUILD_TESTS=OFF` and therefore
  had no test target; that disposable directory was removed. The corrected independent
  `-DBUILD_TESTS=ON` build compiled the complete target without new warning, and
  `production-cutest` passed in 20.63 seconds.
- Final security, encoding, manifest, artifact, temporary-directory, protected-path, and integrity
  checks pass. Apex validation and PRD reconciliation are complete.

## Next Command

Session 08 planning - Effective Binding Observability.
