# Special Procedure Phase 07 Validation

**Status:** Implementation and Autotools validation complete; final CMake validation in progress
(2026-08-08)

## Scope

Phase 07 completes the source-ownership refactor without changing special-procedure behavior. It
removes the transitional top-level assignment source and declaration umbrella, gives compiled
assignments shallow owners under `src/spec/`, and makes every former umbrella consumer include the
interface that actually declares its symbols.

The legacy callback ABI, persisted procedure names, world grammar, prototype callback slots,
command and pulse traversal, activation flags, return handling, wrapper composition, boot
precedence, and single-name persistence remain unchanged.

## Final Assignment Ownership

| Owner | Responsibility |
|-------|----------------|
| `src/spec/spec_assign.c` | Shared owner-typed callback writes and effective-binding provenance. |
| `src/spec/spec_assign_mobiles.c` | Mobile inventory, retained compatibility branches, and the historical object write that must occur during the mobile phase. |
| `src/spec/spec_assign_objects.c` | Object inventory, the two-row declarative Luminari table, and its boot validation. |
| `src/spec/spec_assign_rooms.c` | Room inventory and death-trap assignment loop. |
| `src/spec/spec_assign.h` | Public boot API: table validation plus mobile, object, and room assignment entry points. |
| `src/spec/spec_assign_internal.h` | Private owner-typed helpers used only by the three inventory modules. |

The original inventory was moved without deleting or reordering live rows. Each inventory owns its
source-location macro, so effective diagnostics now report the real mobile, object, or room source
and line. Missing-prototype diagnostics, mini-mud suppression, collision history, declarative-row
failure behavior, and callback writes still use the same shared path.

## Header Ownership

The removed umbrella supplied 46 transitive includes to 52 production files and 10 test files. Its
replacement is not another aggregator:

- assignment entry points live in `spec/spec_assign.h`;
- legacy indexed-name projections live with the canonical registry in `spec/spec_registry.h`;
- general object callbacks and `weapons_spells()` live in `spec/spec_objects.h`;
- legacy vessel and Neverwinter callbacks have narrow adjacent owner headers; and
- crafting molds, object save, player shops, trade, quests, abilities, skills, and spells use their
  existing or new subsystem owner headers.

Five dormant declarations with no implementation were not republished. Repository search of
production and production-linked test sources finds no transitional path or include.

## Boot and Compatibility Evidence

`boot_db()` retains this effective order when specials are enabled:

```text
named world and moving-room parser bindings
  -> mobile assignments
  -> shop wrapping
  -> object assignments
  -> room assignments
  -> quest wrapping
```

The complete block remains behind the existing `no_specials` gate, while world parsing and final
reporting retain their prior positions. Shop and quest wrappers still save the current callback
before installing themselves. Direct assignments still override earlier named bindings in the same
order, and effective history remains diagnostic rather than executable or persisted.

## Production-Linked Coverage

`TestSpecAssignmentModulesExposeNarrowBoundaries` verifies:

- all four assignment modules and their unique public function ownership;
- actual mobile, object, and room provenance labels;
- the narrow public assignment and registry projection interfaces;
- direct boot-database inclusion;
- membership of all four modules in both build manifests; and
- absence of the two transitional files and manifest entries.

Existing tests continue to cover exact boot order, `no_specials`, direct-write precedence, shop and
quest saved secondaries, bounded effective diagnostics, assignment table validation, callback
results, authored single-name persistence, and moving-room collisions. Phase 07 raises the dedicated
special-procedure inventory to 117 tests and the complete production-linked suite to 590 tests.

## Manifest Parity

The compiled source comparison is exact:

- 288 production C sources occur in both Automake `circle_SOURCES` and CMake `SRC_C_FILES`;
- the same 41 CuTest owner sources occur in Automake `cutest_SOURCES` and `cutest_test_files` and in
  CMake `CUTEST_TEST_SOURCES`; and
- each of the four assignment sources occurs once in both production lists.

Incidental header listings are not compiled membership: Automake lists `src/player_rename.h`, while
CMake lists `src/net/msdp_json.h`. They are excluded from the C-source parity comparison.

## Verification Gates

| Gate | Current result |
|------|----------------|
| Inventory, direct-include, transitional-path, and protected-file audit | PASS |
| Incremental Autotools build | PASS |
| Root production-linked suite | PASS - 590 tests |
| `make install`; executable `bin/circle`; no root `circle` | PASS |
| Exact production and CuTest C-source manifest parity | PASS - 288 production, 41 test owners |
| Clean warning-free Autotools build and repeat test/install | PASS - zero warnings, 590 tests, installed binary active |
| Fresh CMake `circle`, `cutest`, and complete CTest matrix | Pending final gate |
| Documentation links, generated guides, ASCII/LF, diff hygiene, and hooks | Pending final gate |

The database-first `SPECIALS` help contract is unchanged. Neither the help text nor its migration
and verifier SQL was modified, so no help-content migration is part of Phase 07.
