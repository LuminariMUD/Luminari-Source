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

## Checkpoint 6 - Reusable Mobile Archetypes and Clan Services

Checkpoint 6 completes the reusable combat/companion archetype slice and moves clan-hall behavior
to the clan subsystem:

- `src/spec/spec_mobile_archetypes.c` and `.h` own `dracolich_mob`, `vampire_mob`, `lich_mob`,
  `cityguard`, `dog`, `practice_dummy`, `planewalker`, `phantom`, `mercenary`, `ethereal_pet`, and
  the summoned-companion callbacks `wraith`, `skeleton_zombie`, `vampire`, `totemanimal`, `shades`,
  `solid_elemental`, `wraith_elemental`, and `bonedancer`;
- the legacy `perform_lichdrain()` helper moves with the lich archetype while retaining its global
  name and type; and
- `src/clan_services.c` and `.h` own `clan_cleric` and `clan_guard`, including clan-hall lookup,
  membership policy, spell pricing, and entrance blocking.

The dedicated archetype file keeps both general mobile sources below the 1,000-line review prompt.
Assignment and registry code include the archetype owner header directly. `src/spec_procs.h` keeps
the owner headers as compatibility includes and no longer redeclares the moved callbacks. Both build
manifests link the two new implementation files for production and CuTest.

This checkpoint preserves callback bodies, function-local static state, signatures, assignments,
registry identities, command and pulse behavior, player-visible messages, spell and combat effects,
follower lifecycle, clan prices, and access rules. No player helpfile changed because no command or
behavior contract changed.

The checkpoint removes 1,055 more lines from `src/spec_procs.c`, reducing it from 3,015 to 1,960
lines and from the Phase 03 baseline of 12,212 lines by 10,252 lines.

### Checkpoint 6 verification

| Gate | Result |
|------|--------|
| warning-clean Autotools production build | PASS |
| `make test` | PASS, 574 tests plus all root script gates |
| `make install` | PASS; `bin/circle` installed and root `circle` removed |
| CMake production and `cutest` rebuild | PASS |
| CMake `ctest --output-on-failure` | PASS, 12/12 tests |
| complete exported global-symbol comparison against Checkpoint 5 | PASS, no symbol added, removed, or retyped |
| `git diff --check` | PASS |

## Checkpoint 7 - King's Castle Zone Package

Checkpoint 7 begins the `src/zone_procs.c` split by moving the complete King's Castle package to
`src/spec/spec_zone_kings_castle.c` and `.h`. The owner now contains:

- `assign_kings_castle()` and its recorded mobile-assignment helper;
- `king_welmar`, `training_master`, `tom`, `tim`, `James`, `cleaning`, `CastleGuard`,
  `DicknDavid`, `peter`, and `jerry`;
- the exported `do_npc_rescue()` helper; and
- all castle-private VNUM conversion, guard and staff predicates, target selection, entrance
  blocking, cleaning, twin, combat, movement-path, and function-local static state.

`src/spec_assign.c` includes the owner header directly, while `src/spec_procs.h` retains it as a
compatibility include and no longer redeclares the two exported owner functions. Both build
manifests link the new implementation for production and CuTest. The recorded assignment source
label now names `src/spec/spec_zone_kings_castle.c`, its actual source owner; assignment order,
handler selection, and effective-binding outcomes are unchanged.

This checkpoint preserves callback bodies, signatures, global symbol names, castle-relative VNUM
calculation, initialization order, command and pulse behavior, movement paths, combat decisions,
messages, and static state. No player or builder helpfile changed because no command, authored-data,
or behavior contract changed.

The checkpoint removes 818 lines from `src/zone_procs.c`, reducing it from 4,202 to 3,384 lines.
The new cohesive implementation is 851 lines, below the 1,000-line review prompt.

### Checkpoint 7 verification

| Gate | Result |
|------|--------|
| warning-clean Autotools production build | PASS |
| `make test` | PASS, 574 tests plus all root script gates |
| `make install` | PASS; `bin/circle` installed and root `circle` removed |
| CMake production and `cutest` rebuild | PASS |
| CMake `ctest --output-on-failure` | PASS, 12/12 tests |
| complete exported global-symbol comparison against Checkpoint 6 | PASS, no symbol added, removed, or retyped |
| `git diff --check` | PASS |

## Checkpoint 8 - Abyss and Crimson Flame Zone Packages

Checkpoint 8 moves the next two complete package boundaries from `src/zone_procs.c`:

- `src/spec/spec_zone_abyss.c` and `.h` own `abyss_randomizer` and its private room-number
  conversion helper; and
- `src/spec/spec_zone_crimson_flame.c` and `.h` own `cf_trainingmaster`, `cf_alathar`, and the
  exported `cf_converter()` helper used to resolve their zone-relative mobile VNUMs.

`src/spec_assign.c` and `src/spec/spec_registry.c` include both owner headers directly.
`src/spec_procs.h` retains them as compatibility includes and no longer redeclares the three moved
callbacks. Both build manifests link both implementations for production and CuTest. The Crimson
Flame source explicitly includes `magic/spells.h` for the combat type and damage constants that the
legacy monolith supplied indirectly.

The ownership trace confirms that `tia_rapier` is object VNUM 132125 in The Prisoner package, not
Crimson Flame content, so it remains with the following Prisoner boundary. This checkpoint preserves
callback bodies, signatures, global symbol names, zone-number conversion, assignment order,
registry identity, hunting and follower behavior, combat calls, messages, and pulse behavior. No
player or builder helpfile changed because no command, authored-data, or behavior contract changed.

The checkpoint removes another 261 lines from `src/zone_procs.c`, reducing it from 3,384 to 3,123
lines and by 1,079 lines from the Phase 03 baseline. The new Abyss and Crimson Flame implementations
are 158 and 124 lines respectively.

### Checkpoint 8 verification

| Gate | Result |
|------|--------|
| warning-clean Autotools production build | PASS |
| `make test` | PASS, 574 tests plus all root script gates |
| `make install` | PASS; `bin/circle` installed and root `circle` removed |
| CMake production and `cutest` rebuild | PASS |
| CMake `ctest --output-on-failure` | PASS, 12/12 tests |
| complete exported global-symbol comparison against Checkpoint 7 | PASS, no symbol added, removed, or retyped |
| `git diff --check` | PASS |

## Checkpoint 9 - Prisoner and Celestial Leviathan Packages

Checkpoint 9 moves two adjacent, explicitly coupled encounter packages:

- `src/spec/spec_zone_prisoner.c` and `.h` own `tia_rapier`, `the_prisoner`,
  `prisoner_dracolich`, `prisoner_heads`, `eq_loaded`, the death and form transition, attack and
  rejuvenation helpers, treasury gear loading, item transfer, and all package-local VNUM and loot
  definitions; and
- `src/spec/spec_zone_celestial_leviathan.c` and `.h` own the existing no-op
  `celestial_leviathan` callback plus its currently unused rejuvenation and attack helpers.

Combat and staff-event code include the Prisoner owner header directly for
`prisoner_on_death()` and `prisoner_heads`. Assignment and registry code include both owner headers
directly. `src/spec_procs.h` retains both as compatibility includes and no longer redeclares the
moved state, API, or callbacks. Both build manifests link both implementations for production and
CuTest.

The Celestial attack helper already selected breath attacks from `prisoner_heads`; this checkpoint
preserves that dormant behavior and makes the dependency explicit by including the Prisoner owner
header. It does not activate either unused helper or change the assigned callback's no-op result.
All callback bodies, exported names and types, state initialization, assignment and registry
identity, death timing, loot construction, combat calls, messages, and event completion behavior
remain unchanged. No player or builder helpfile changed because no command, authored-data, or
behavior contract changed.

The checkpoint removes another 881 lines from `src/zone_procs.c`, reducing it from 3,123 to 2,242
lines and by 1,960 lines from the Phase 03 baseline. It also removes the final 17-line zone stub
section from `src/spec_procs.c`, reducing that file from 1,960 to 1,943 lines and by 10,269 lines
from its Phase 03 baseline. The new Prisoner and Celestial Leviathan implementations are 790 and
144 lines respectively.

### Checkpoint 9 verification

| Gate | Result |
|------|--------|
| warning-clean Autotools production build | PASS |
| `make test` | PASS, 574 tests plus all root script gates |
| `make install` | PASS; `bin/circle` installed and root `circle` removed |
| CMake production and `cutest` rebuild | PASS |
| CMake `ctest --output-on-failure` | PASS, 12/12 tests |
| complete exported global-symbol comparison against Checkpoint 8 | PASS, no symbol added, removed, or retyped |
| `git diff --check` | PASS |

## Checkpoint 10 - Fire Giant Zone Package

Checkpoint 10 moves the complete Fire Giant boundary to
`src/spec/spec_zone_fire_giant.c` and `.h`. The owner now contains:

- `fg_invasion_loader` and every invasion limit, treasure, room, mobile, and equipment definition
  used by its load sequence;
- all four elite-squad construction paths, distributed attackers, throne guards, and the Valkyrie
  arrival; and
- `flamekissed_instrument`, its private subtype parser, transformation messages, and hit-point and
  move-action cost.

`src/spec_assign.c` and `src/spec/spec_registry.c` include the owner header directly.
`src/spec_procs.h` retains it as a compatibility include and no longer redeclares the two moved
callbacks. Both build manifests link the new implementation for production and CuTest.

This is an ownership-only move. The callback bodies, exported names and types, package constants,
load counts and order, assignment and registry references, equipment placement, command matching,
instrument subtype resolution, messages, resource costs, and pulse behavior remain unchanged. No
player or builder helpfile changed because no command, authored-data, or behavior contract changed.

The checkpoint removes another 634 lines from `src/zone_procs.c`, reducing it from 2,242 to 1,608
lines and by 2,594 lines from the Phase 03 baseline. The new cohesive implementation is 657 lines.

### Checkpoint 10 verification

| Gate | Result |
|------|--------|
| warning-clean Autotools production build | PASS |
| `make test` | PASS, 574 tests plus all root script gates |
| `make install` | PASS; `bin/circle` installed and root `circle` removed |
| CMake production and `cutest` rebuild | PASS |
| CMake `ctest --output-on-failure` | PASS, 12/12 tests |
| complete exported global-symbol comparison against Checkpoint 9 | PASS, no symbol added, removed, or retyped |
| `git diff --check` | PASS |

## Remaining Phase 03 Work

1. Continue splitting `src/zone_procs.c` along its existing zone-package boundaries while retaining
   private static state with each package.
2. Move the remaining cohesive zone-specific content from `src/spec_procs.c` with its packages,
   then relocate the remaining cross-file equipment helper.
3. Re-run source ownership, exported-symbol, Autotools, CMake, and full test validation.
4. Replace this progress record with final Phase 03 acceptance evidence and mark the PRD phase
   complete.

## Resume Point

Move the complete Jot boundary beginning with `JOT_VNUM` and ending after `giantslayer` and the Jot
end marker. Keep invasion state and position counters, the converter and loader, mobile callbacks,
object callbacks, path table, package constants, and private helpers together. The boundary is
approximately 983 legacy lines and may cross the 1,000-line review prompt after owner includes and
file documentation; review its cohesion rather than splitting shared encounter state by owner type.
Preserve all exported names, callback signatures, assignment and registry references, and compare
the complete symbol set before proceeding to the Mad Drow package.
