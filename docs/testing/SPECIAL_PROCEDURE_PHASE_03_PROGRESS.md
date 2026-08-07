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

## Checkpoint 4 - Legacy Moving-Room Ownership

Checkpoint 4 consolidates the complete legacy moving-room package under
`src/vessels/vessels_moving_rooms.c` and `.h`. The owner now contains:

- the `movingRoomList` runtime list;
- world `M` record loading and `setup_moving_room()`;
- the zone-pulse scheduler in `moving_rooms_update()`;
- `prepMovingRoom()`, `unlinkMovingRoom()`, and `linkMovingRoom()`; and
- the legacy `moving_rooms` callback reached through the Phase 01 gateway.

The loader and scheduler moved out of `src/db.c`; the relocation helpers and callback moved out of
`src/spec_procs.c`. `src/comm.c`, `src/db.c`, `src/olc/genwld.c`, and the production-linked
moving-room tests include the owner header directly. `src/spec_procs.h` retains it as a compatibility
include, while `src/olc/oasis.h` no longer publishes declarations owned by vessels.

This is an ownership-only move. The world `M` grammar, allocation and list order, callback ABI,
gateway translation, zone-pulse position, pulse reset, route selection, `currentInbound` updates,
exit mutation order, messages, and global symbol names are unchanged. Both build manifests add the
same implementation file. No player or builder helpfile changed because no command, authored data,
or visible behavior changed.

The checkpoint removes 325 more lines from `src/spec_procs.c`, reducing it from 4,345 to 4,020
lines and from the Phase 03 baseline of 12,212 lines by 8,192 lines.

### Checkpoint 4 verification

| Gate | Result |
|------|--------|
| `make test` | PASS, 574 tests plus all root script gates |
| `make install` | PASS; `bin/circle` installed and root `circle` removed |
| CMake production and `cutest` rebuild | PASS |
| CMake `ctest --output-on-failure` | PASS, 12/12 tests |
| complete exported global-symbol comparison against Checkpoint 3 | PASS, no symbol added, removed, or retyped |
| `git diff --check` | PASS |

## Checkpoint 5 - General Mobiles, Rooms, and Feature Services

Checkpoint 5 completes the traced general mobile and room slice while keeping feature-specific
services with their primary systems:

- `src/spec/spec_mobiles.c` and `.h` own `mayor`, `snake`, `hound`, `thief`, `wizard`, `wall`,
  `puff`, `fido`, and `janitor`, plus the private `npc_steal()` helper;
- `src/spec/spec_rooms.c` and `.h` own the general `dump` room callback;
- `src/character/guild_services.c` and `.h` own class training through `guild` and entrance policy
  through `guild_guard`;
- `src/magic/spellbook_scroll.c` and its new owner header own `wizard_library` research; and
- `src/obj/vendor.c` and its new owner header own room-based `pet_shops` commerce alongside the
  existing pet-object and equipment-vendor services.

Assignment and registry code include the owner headers directly. `src/spec_procs.h` retains those
headers as a compatibility surface and no longer redeclares the moved callbacks. Both build
manifests add the same three new implementation files; the existing magic and vendor sources were
already linked by both systems.

This checkpoint preserves callback bodies, static state, signatures, registry identities,
assignments, command matching, player-visible messages, costs, rewards, spellbook mutation, pet
scaling and follower setup, guild rules, and mobile pulse behavior. System documentation now points
pet and vendor ownership at the feature files. No player helpfile changed because no command or
behavior contract changed.

The checkpoint removes 1,005 more lines from `src/spec_procs.c`, reducing it from 4,020 to 3,015
lines and from the Phase 03 baseline of 12,212 lines by 9,197 lines.

### Checkpoint 5 verification

| Gate | Result |
|------|--------|
| `make test` | PASS, 574 tests plus all root script gates |
| `make install` | PASS; `bin/circle` installed and root `circle` removed |
| CMake production and `cutest` rebuild | PASS |
| CMake `ctest --output-on-failure` | PASS, 12/12 tests |
| complete exported global-symbol comparison against Checkpoint 4 | PASS, no symbol added, removed, or retyped |
| `git diff --check` | PASS |

## Remaining Phase 03 Work

1. Finish the traced ownership audit for common archetypes, guards, clan services, and named mobile
   packages still in `src/spec_procs.c`.
2. Split `src/zone_procs.c` along its existing zone-package boundaries while retaining private
   static state with each package.
3. Move the remaining cohesive mobile content and the Celestial Leviathan stub with their packages.
4. Re-run source ownership, exported-symbol, Autotools, CMake, and full test validation.
5. Replace this progress record with final Phase 03 acceptance evidence and mark the PRD phase
   complete.

## Resume Point

Trace the remaining mobile callbacks by assignment and shared state. Likely general candidates are
the reusable monster archetypes, `cityguard`, `dog`, and `practice_dummy`; clan services belong with
the clan subsystem, while named encounters and guards must move with their cohesive zone packages.
Keep private encounter state beside its callbacks when splitting `src/zone_procs.c`.
