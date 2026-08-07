# Special Procedure Phase 03 Progress

Status: In progress  
Last updated: 2026-08-07  
Baseline: Phase 02 completion commit `9acda0ec`

## Purpose

This document is the durable handoff for Phase 03 while behavior-preserving source extraction is
underway. Phase 03 is not accepted yet. Each checkpoint records exact ownership moves and validation
so work can resume without repeating the source audit.

## Checkpoint 1 - General Objects and Legacy Vessels

The first checkpoint removes 1,052 lines from `src/spec_procs.c` and adds two production-linked
source files. It changes source ownership only: callback names, linkage, signatures, assignment
sites, registry identities, initialization order, and runtime behavior remain unchanged.

### General object ownership

`src/spec/spec_objects.c` now owns:

- `obj_proc_ready()` and `weapons_spells()`;
- `monk_glove` and `monk_glove_cold`;
- `spikeshield`, `viperdagger`, `ches`, and `courage`;
- `flamingwhip`, `helmblade`, `flaming_scimitar`, and `frosty_scimitar`;
- `disruption_mace` and `haste_bracers`.

The unused, commented `forest_idol` source remains beside the procedures it historically
accompanied. This checkpoint does not redesign `obj_proc_ready()`; the known same-VNUM versus
pointer-identity issue remains a Phase 04 contract.

### Vessel ownership

`src/vessels/vessels_legacy.c` now owns:

- the legacy route table and `find_ship()`, `move_ship()`, and `update_ship()`;
- `ship_lookout()` and `do_disembark`;
- the Chionthar ferry, Alandor ferry, and Moonshae carpet procedures;
- the Greyhawk boarding and bridge-command procedures.

The Neverwinter button and valve controls remain in `src/spec_procs.c` because they are a cohesive
zone puzzle, not general vessel mechanics. `floating_teleport` also remains pending its owning
package decision.

### Build manifests

Both production source manifests contain the same new implementation files:

- `Makefile.am`: `src/spec/spec_objects.c` and `src/vessels/vessels_legacy.c`;
- `CMakeLists.txt`: the identical two paths.

The root Autotools production binary and CuTest binary link both files. A fresh CMake build links
both the `circle` and `cutest` targets.

## Verification Evidence

Run from the repository root on 2026-08-07:

| Gate | Result |
|------|--------|
| `make -j$(nproc)` | PASS with `-Wall -Wextra` |
| `make test` | PASS |
| direct `./cutest` confirmation | PASS, 574 tests |
| `make install` | PASS; `bin/circle` installed and root `circle` removed |
| fresh CMake `circle` build | PASS |
| fresh CMake `cutest` build and `ctest --output-on-failure` | PASS, 12/12 tests |
| exported-symbol comparison against the Phase 02 installed binary | PASS, no moved symbol added, removed, or retyped |
| `git diff --check` | PASS |

The symbol comparison covers all moved helpers, commands, object callbacks, ferry callbacks, the
Greyhawk callbacks, and the legacy `ship_info` data symbol. Existing production-linked tests retain
registry identity, dispatch, boarding, vessel, assignment, and world-loading coverage.

## Remaining Phase 03 Work

1. Finish the contiguous general-object extraction without absorbing artifact, vessel, vendor,
   crafting, or zone-owned behavior.
2. Move vendor/trade and crafting-mold behavior to their actual subsystem owners.
3. Extract reusable mobile and room procedures.
4. Split `src/zone_procs.c` along its existing zone-package boundaries while retaining private
   static state with each package.
5. Re-run source ownership, exported-symbol, Autotools, CMake, and full test validation.
6. Replace this progress record with final Phase 03 acceptance evidence and mark the PRD phase
   complete.

## Resume Point

Start by inventorying the remaining `SPECIAL(...)` definitions in `src/spec_procs.c` by section
and tracing every non-static helper before moving the next contiguous group. Do not classify the
Neverwinter controls or `floating_teleport` as vessel-owned solely because they followed ferry code
in the legacy file.
