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

At this checkpoint the Neverwinter button and valve controls remained in `src/spec_procs.c`
because they are a cohesive zone puzzle, not general vessel mechanics. `floating_teleport` also
remained pending a traced ownership decision; Checkpoint 2 resolves both.

### Build manifests

Both production source manifests contain the same new implementation files:

- `Makefile.am`: `src/spec/spec_objects.c` and `src/vessels/vessels_legacy.c`;
- `CMakeLists.txt`: the identical two paths.

The root Autotools production binary and CuTest binary link both files. A fresh CMake build links
both the `circle` and `cutest` targets.

## Checkpoint 1 Verification Evidence

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

## Checkpoint 2 - Complete Object and Feature-Owner Split

Checkpoint 2 completes the contiguous object-section audit and moves each non-general group to its
primary subsystem. `src/spec_procs.c` shrinks from 11,162 lines at Checkpoint 1 to 6,357 lines.
The Phase 03 baseline was 12,212 lines.

The resulting ownership is:

- `src/spec/spec_objects.c`: the remaining reusable object callbacks, including `warbow` and the
  traced, cross-zone `floating_teleport`;
- `src/spec/spec_zone_neverwinter.c`: the button and valve halves of the Neverwinter sewage
  control puzzle;
- `src/obj/player_shop.c`: player-owned shop inventory, purchase, identification, and persistence;
- `src/obj/vendor.c`: bank service, generated armor and weapon vendors, pet-object conversion,
  and paid identification;
- `src/craft/crafting_molds.c`: mold construction, listing, validation, purchase, and delivery;
- `src/character/vampire_cloak.c`: vampire-only cloak customization;
- `src/quest/quest_services.c`: completed-quest reward replacement.

The no-op `celestial_leviathan` callback remains in `src/spec_procs.c` until it can move together
with its actual encounter state and helpers from `src/zone_procs.c`. This preserves the cohesive
zone-package rule. The historical commented `forest_idol` and `storage_chest` implementations
remain beside general object procedures and are not compiled.

Both build manifests add the same six new source files and continue to link
`src/spec/spec_objects.c` for production and CuTest. No declaration, assignment, registry entry,
or call site changes.

### Checkpoint 2 verification

| Gate | Result |
|------|--------|
| `make -j$(nproc)` | PASS with `-Wall -Wextra` |
| `make test` | PASS, 574 tests plus all root script gates |
| `make install` | PASS; `bin/circle` installed and root `circle` removed |
| CMake production and `cutest` incremental rebuild | PASS |
| CMake `ctest --output-on-failure` | PASS, 12/12 tests |
| complete exported global-symbol comparison against Checkpoint 1 | PASS, no symbol added, removed, or retyped |

## Checkpoint 3 - Ability, Skill, and Spell Ownership

Checkpoint 3 removes the non-procedure calculation and display block from `src/spec_procs.c` and
splits it by primary game-system responsibility:

- `src/character/abilities.c` and `.h` own `compute_ability()` and
  `compute_ability_full()`;
- `src/character/skill_lists.c` and `.h` own skill prerequisites, skill and ability lists,
  training effects, and the existing `cross_names` and `skills_alphabetic` data;
- `src/magic/spell_lists.c` and `.h` own spell sorting, spell-list display, and the existing
  `spell_sort_info` data.

Direct consumers now include the owner headers. `src/spec_procs.h` includes them as a compatibility
surface for callers that have not yet narrowed their dependency. This is an ownership-only move:
function and data names, linkage, signatures, list ordering, calculations, text, and runtime behavior
are unchanged.

Both build manifests add the same three implementation files for the production and CuTest links.
The checkpoint removes 2,012 more lines from `src/spec_procs.c`, reducing it from 6,357 to 4,345
lines and from the Phase 03 baseline of 12,212 lines by 7,867 lines.

### Checkpoint 3 verification

| Gate | Result |
|------|--------|
| `make test` | PASS, 574 tests plus all root script gates |
| `make install` | PASS; `bin/circle` installed and root `circle` removed |
| CMake production and `cutest` rebuild | PASS |
| CMake `ctest --output-on-failure` | PASS, 12/12 tests |
| complete exported global-symbol comparison against Checkpoint 2 | PASS, no symbol added, removed, or retyped |
| `git diff --check` | PASS |

## Remaining Phase 03 Work

1. Extract reusable mobile and room procedures, and move legacy moving-room behavior to vessels.
2. Split `src/zone_procs.c` along its existing zone-package boundaries while retaining private
   static state with each package.
3. Move the remaining cohesive mobile content and the Celestial Leviathan stub with their packages.
4. Re-run source ownership, exported-symbol, Autotools, CMake, and full test validation.
5. Replace this progress record with final Phase 03 acceptance evidence and mark the PRD phase
   complete.

## Resume Point

Start with the moving-room state and callback in `src/spec_procs.c`; keep them together when moving
them under `src/vessels/`. Then extract only genuinely reusable mobile and room groups to general
spec modules. Keep zone-specific callbacks with the cohesive packages that will move from
`src/zone_procs.c`.
