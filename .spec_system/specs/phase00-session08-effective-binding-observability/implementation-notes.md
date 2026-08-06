# Implementation Notes

**Session ID**: `phase00-session08-effective-binding-observability`
**Started**: 2026-08-07
**Base Commit**: 2ee93973a299772dc29e301e04c7dfb98340ae01
**Status**: Complete

---

## Planning And Trace

- Confirmed a clean development checkout at the published Session 07 commit.
- Confirmed Sessions 02, 03, 04, 06, and 07 are complete and provide the required compatibility,
  registry, authored-state, and persistence baselines.
- Traced production boot precedence as world/parser load, legacy mobile assignment, shop wrapper,
  object assignment, room assignment, and quest wrapper.
- Confirmed `-s` still parses world and moving-room fields but skips shop loading and the entire
  assignment block; reporting must remain outside that block.
- Confirmed shop and quest wrappers preserve the callback present immediately before installation
  in `SHOP_FUNC` and `QST_FUNC`, respectively.
- Confirmed room `M` installs `moving_rooms` into the same callback slot later used by room `Z`, and
  the writer can currently emit both fields.
- Confirmed REdit shallow-copies mover state and separately copies authored binding state; room
  copy/free and prototype insertion/deletion paths require effective-record ownership integration.

## Frozen Decisions

- Effective provenance is separate from authored identity and never controls serialization or
  invocation.
- Contributions are recorded in the exact order their existing callback writes occur.
- Legacy assignments retain table syntax and gain stringized handler/call-site provenance through
  narrow helpers rather than declarative conversion.
- Shop and quest diagnostics reflect their actual saved secondary slots, including null and nested
  wrapper states.
- Allocation failure in observability logs an error but never suppresses an existing callback
  assignment.
- Startup output is one bounded structured line per contribution and one final line per prototype.
- `no_specials` changes which sources contribute, not whether an already-installed callback may be
  invoked outside characterized global gates.
- Moving-room and named room-procedure ownership is rejected in either parser order, at REdit
  selection/internal save, and in a preflight disk-writer scan.

## Implementation Log

- Added an owned effective-binding model with ordered contributions, final handler identity,
  collision counts, wrapper secondaries, deep-copy/free operations, validated bounded text, and
  deterministic normal or `no_specials` formatters.
- Attached effective records to all three prototype owner structures and integrated initialization,
  deletion, database shutdown, room insertion, room copy, REdit scratch-copy, and OLC cleanup paths.
- Instrumented exact named-world requests, unresolved resolutions, moving-room parser hooks, all
  legacy assignment helpers and the castle/death-trap direct paths, shop wrappers, and quest
  wrappers without changing callback assignment order.
- Added unconditional post-assignment startup reporting with contribution and final-winner lines
  plus aggregate prototype, contribution, and collision counts.
- Rejected moving-room `M` plus room `Z` ownership in either loader order, at REdit menu and
  defensive internal-save boundaries, and in the zone writer before opening output or mutating
  mover state.
- Added production-linked model, loader, precedence, secondary, mode-placement, REdit, and writer
  tests and synchronized the Autotools and CMake manifests.
- Updated the OLC SpecProc guide and added an authoritative database help migration with read-only
  verification queries for moving-room exclusivity and builder selection guidance. Classified both
  SQL files in the exhaustive component-schema CI manifest.
- Apex code review repaired formatter truncation reporting, first-resolution outcome selection,
  wrapper invariants, parser-sized text bounds, latest-authored final output, exact world-definition
  identity, stable call-site paths, fixture cleanup and guards, authoritative help routing, and SQL
  manifest coverage.
- Independent CTest exposed a stale world-tool parser for the removed legacy registry. The extractor
  now reads canonical definitions and referenced alias arrays from `src/spec/spec_registry.c`,
  ignores commented declarations, and fails closed when a referenced alias initializer is absent.

## Verification Log

- Apex prerequisite and clean-base checks pass.
- Base HEAD and upstream both resolve to `2ee93973a299772dc29e301e04c7dfb98340ae01`.
- Focused production translation-unit builds pass with `-Wall -Wextra` and no warnings.
- The production-linked `cutest` executable passes all 550 tests after implementation and initial
  review repairs.
- `make test` passes seven auxiliary checks and all 550 production-linked CuTests; the required
  follow-up `make install` installs the versioned server and removes the root binary.
- An independent CMake build compiles the production-linked suite and all 11 CTest targets pass.
- The full world-tool suite passes 173/173 tests after the registry-consumer repair.
- Restricted `clang-tidy` exits successfully with only inherited structure-padding advice.
- `code-review.md` is RESOLVED with no open finding.
- Encoding, protected-path, world-data digest, artifact, SQL behavior, unsafe-API, C formatting,
  Python lint, and exact manifest-parity checks pass.
- Apex validation is PASS and authoritative PRD/state tracking marks Session 08 complete.
