# LuminariMUD Developer Guide and API Reference

Use [docs/development.md](../development.md) for the verified build, test,
source-map, and style entry point. This reference documents the special-procedure
control plane delivered through Phase 02, the completed Phase 03 ownership
extraction, and the Phase 04 shared mechanics; other subsystem APIs remain in
their source-linked system documents.

## Quick Start

```bash
./scripts/deployment/deploy.sh --auto --dev --init-world
make test
make install
```

The development mode installs GDB and Valgrind. MariaDB and world data remain
required.

## Special-Procedure Control Plane

### Definition Registry

World/OLC special procedures are defined in `src/spec/spec_registry.c`, with
public metadata and accessors in `src/spec/spec_registry.h`. Assignment remains
in `src/spec_assign.c`; runtime invocation still uses the legacy ABI:

```c
int handler(struct char_data *ch, void *me, int cmd, const char *argument);
```

Each immutable `struct spec_definition` declares:

- stable canonical and display names plus explicit aliases;
- compatible mobile, object, or room owner bits;
- supported events and their prototype/placement prerequisites;
- permitted binding sources and builder visibility;
- nonempty category and description; and
- exactly one legacy or typed handler.

`spec_registry_boot_validate()` runs in `boot_db()` before `boot_world()`.
Invalid programmer metadata terminates startup before MySQL world
initialization or world parsing. Validation rejects missing text,
case-insensitive collisions, invalid masks, incompatible owner/event pairs,
missing prerequisites, duplicate events, invalid visibility, and invalid
handler shape.

### Lookup API

```c
const struct spec_definition *definition;

definition = spec_registry_find_for_owner("Bank", SPEC_OWNER_OBJECT);
if (definition != NULL &&
    spec_definition_supports_event(definition, SPEC_OWNER_OBJECT, SPEC_EVENT_COMMAND))
{
  /* The name, owner, and event contract are compatible. */
}
```

- `spec_registry_count()` and `spec_registry_get()` iterate canonical rows.
- `spec_registry_find_by_name()` resolves canonical names and aliases without
  case sensitivity.
- `spec_registry_find_for_owner()` also requires one compatible owner type.
- `spec_registry_find_by_handler()` returns the first canonical identity for a
  legacy callback.
- `spec_definition_get_event()` exposes one supported event contract.
- `spec_definition_allows_binding()` checks one binding source.

Definitions, event contracts, aliases, and strings have immutable
process-lifetime storage and must not be modified or freed. The compatibility
surface exposes 29 historical indexed names for 28 canonical definitions:
`Guildmaster` is an alias of canonical `Guild`, and both resolve to `guild`.

### Authored Binding API

`src/spec/spec_binding.h` owns exact world-authoring intent. A
`struct spec_binding` retains owner, prototype VNUM, exact requested text,
source, location, resolution status, and an immutable definition when resolved.
Unknown name, incompatible owner, and incompatible source are retained states,
not allocation failures.

- `spec_binding_replace()` resolves and transactionally replaces a record.
- `spec_binding_legacy_handler()` returns a callback only for a resolved record.
- `spec_binding_copy()` and `spec_binding_free()` implement prototype and OLC
  ownership.
- `spec_binding_persisted_name()` returns only valid single-line world-authored
  identity.

Writers consult authored state first. Aliases, unknown names, and incompatible
names therefore survive unrelated saves, while explicit OLC selection stores a
canonical name and explicit clear removes the record. Never infer authored
intent from the effective callback pointer.

### Effective Binding API

`src/spec/spec_effective_binding.h` owns an ordered observation of boot-time
callback writes. `spec_effective_binding_contribute()` records validated source,
requested and installed identity, location, handler, outcome, and optional
saved secondary state. The record is diagnostic only: it never dispatches a
callback and is never serialized.

The sources are named world data, moving-room parser hook, legacy assignment,
shop wrapper, and quest wrapper. Formatters emit bounded `SPEC_BIND` and
`SPEC_BIND_FINAL` records within `SPEC_BIND_SUMMARY`. Allocation or formatting
failure may log an error but cannot suppress an established callback assignment.

With specials enabled, preserved write order is world/parser load, mobile
assignment, shop wrapping, object assignment, room assignment, and quest
wrapping. Under `-s`, world/parser records still load while the guarded
assignment block does not run; reporting remains outside the guard.

`specbind <mob|obj|room> <vnum>` exposes the same prototype-owned history to
immortal staff after boot. It reports every contribution, outcome, location,
saved secondary, collision count, and final source without mutating the slot or
recomputing history after an OLC edit.

### Declarative Legacy Assignment API

`src/spec/spec_assign_table.h` defines separate mobile, object, and room row
types. A row holds the corresponding VNUM type and a canonical definition name
or explicit alias. `spec_assign_table_resolve()` requires a registered
definition that supports the row owner and permits legacy assignment;
owner-specific table validators identify the first failing index and VNUM.
`spec_assign_table_boot_validate()` runs immediately after registry validation
and before world parsing, so invalid source data is a boot-fatal programmer
error.

Convert a direct `ASSIGNMOB`, `ASSIGNOBJ`, or `ASSIGNROOM` call only when its
VNUM has a traced symbolic constant and its handler has registry metadata.
Keep numeric, computed, campaign-compatibility, and special-setup assignments
on the legacy path until those prerequisites exist. Both paths call the same
owner-specific assignment helpers and therefore preserve callback writes,
source provenance, and collision reporting.

### OLC and Persistence

`src/olc/spec_menu.c` provides an owner-filtered, one-based view of
builder-visible world-bindable definitions. It shows description, event, flag,
and placement prerequisites. Selection never changes `MOB_SPEC`,
`ITEM_AUTOPROC`, placement, or combat state automatically.

Mobile, object, and room prototypes own authored and effective records. OLC
scratch state owns independent copies. Prototype deletion, database shutdown,
room insertion/copy, and OLC cleanup must use the matching copy/free APIs; do
not shallow-copy either record.

A moving-room `M` record and named room `Z` procedure cannot share
`room_data.func` because their `me` payloads differ. The loader rejects both
orders, REdit rejects selection/save, and `save_rooms()` validates the entire
zone before opening output or mutating mover state.

### Shared Mechanics API

Phase 04 exposes five narrow APIs. They supplement the gateway and production engines; they do not
replace combat, affects, artifact ownership, or the legacy callback ABI.

- `src/spec/spec_context.h` validates one typed gateway payload, an exact worn object, or live
  colocated combat participants. Worn validation requires both `obj->worn_by == actor` and the
  actor's wear slot to point to that object. `DEAD()` flags count as pending extraction. Pointers are
  borrowed and must not be retained after an effect.
- `src/spec/spec_phrase.h` compares a resolved canonical command and exact phrase. Only leading
  spaces may be skipped, and only when the rule requests it; case, punctuation, tabs, and trailing
  whitespace remain significant. Treat `SPEC_PHRASE_UNRELATED` as normal fallthrough.
- `src/spec/spec_cooldown.h` serves only legacy object `spec_timer[]` counters. Slots are
  `[0, SPEC_TIMER_MAX)`, values are MUD hours decremented by `point_update()`, storage belongs to the
  instance, and objsave does not persist it. Validate and execute first, then commit a positive
  duration.
- `src/spec/spec_combat.h` applies damage only to a live colocated current opponent. Preserve the
  returned `legacy_result`; after `SPEC_DAMAGE_TARGET_INVALIDATED`, do not dereference the target.
- `src/spec/spec_effects.h` creates stable negative source identities from a namespace and key, then
  applies up to eight temporary modifiers atomically. Source ownership belongs in `source_id`;
  stacking identity belongs in `specific` and is scoped by spell. A stacking conflict spends no
  caller cooldown.

Use `is_wearing()` when same-VNUM membership is intentionally the policy. Use
`spec_context_validate_worn_object()` when the callback must prove that its own object instance is
equipped. Do not invent a second affect source field or use pointer values as source keys.

## Adding or Changing a Registered Procedure

1. Characterize every affected invocation and exact legacy argument before
   changing behavior.
2. Add or update immutable definition/event metadata in
   `src/spec/spec_registry.c`.
3. Preserve the canonical persisted name; use an explicit alias for compatible
   historical input.
4. Derive owner, source, visibility, flag, and placement metadata from traced
   callers.
5. Add production-linked registry, OLC, persistence, and invocation coverage as
   applicable.
6. Update both `Makefile.am` and `CMakeLists.txt` for source membership changes.
7. Update builder documentation and database-first help migration/verifier when
   the contract changes.

## Compatibility Boundary

Phases 00-04 preserve the single callback slot, `SPECIAL` ABI, world grammar,
command traversal, heartbeat timing, caller-specific returns, activation flags,
shop/quest nesting, and boot precedence. Declarative validation applies to the
two currently eligible Luminari rows; unsupported assignments remain on the
observable compatibility path. Behavior-preserving content extraction is
complete: general object callbacks are under `src/spec/`; vessel callbacks are
under `src/vessels/`, including the complete legacy moving-room loader,
scheduler, relocation, and callback package. Player-shop, vendor, crafting,
vampire-cloak, quest-service, and Neverwinter callbacks are now owned by their feature
directories. Character ability calculations and skill list/training APIs are
declared by `src/character/abilities.h` and `src/character/skill_lists.h`;
spell sorting and display are declared by `src/magic/spell_lists.h`. General legacy mobile and room
callbacks use `src/spec/spec_mobiles.h` and `src/spec/spec_rooms.h`; guild services, wizard
research, and pet-shop commerce use their owner headers under `src/character/`, `src/magic/`, and
`src/obj/`. Reusable combat/companion callbacks use `src/spec/spec_mobile_archetypes.h`; clan-hall
services use `src/clan_services.h`. King's Castle assignments and mobile behavior use
`src/spec/spec_zone_kings_castle.h`; keep its relative-VNUM helpers and private runtime state with
that cohesive zone package. Abyss and Crimson Flame procedures use `src/spec/spec_zone_abyss.h` and
`src/spec/spec_zone_crimson_flame.h`; their room/mobile conversion helpers remain with their zone
owners. The Prisoner raid API and shared event state use `src/spec/spec_zone_prisoner.h`; the
Celestial Leviathan owner explicitly includes that header for its existing `prisoner_heads`
dependency. Fire Giant invasion loading and the transforming instrument use
`src/spec/spec_zone_fire_giant.h`; keep their shared load definitions and transformation cost with
that package. Jot invasion state, relative-VNUM conversion, mobile encounters, and zone-specific
objects use `src/spec/spec_zone_jot.h`; the owner intentionally remains cohesive even though its
implementation is just over the file-size review prompt. Mad Drow cube-slider state, row tables,
exit helpers, and callback use `src/spec/spec_zone_mad_drow.h`; keep the complete puzzle state and
mutation order in that owner. TTF AOE encounters, follower summoning, and patrol state use
`src/spec/spec_zone_ttf.h`. That final package retired the legacy `src/zone_procs.c` source. Use
`src/spec/spec_zone_shadow_dragon.h`, `src/spec/spec_zone_banshee.h`, and
`src/spec/spec_zone_quicksand.h` for the Shadow Dragon, Banshee, and Quicksand callbacks. Use
`src/spec/spec_zone_kenjin_tower.h` for the Tower of Kenjin mobile and room callbacks, and
`src/spec/spec_zone_hive_of_passion.h` for the Hive of Passion death callback. Fey-Branche combat
coordination uses `src/spec/spec_zone_feybranche.h`, and Abyssal Vortex exit rotation uses
`src/spec/spec_zone_abyssal_vortex.h`. House Agrach-Dyrr combat coordination uses
`src/spec/spec_zone_agrach_dyrr.h`, and House Shobalar coordination uses
`src/spec/spec_zone_shobalar.h`. Earth Plane reinforcement behavior uses
`src/spec/spec_zone_earth_plane.h`, and Air Plane combat and reinforcement behavior uses
`src/spec/spec_zone_air_plane.h`. Zusuk's Fzoul command callback uses
`src/spec/spec_zone_zusuk.h`, while the Orc Ruins Shar callbacks use
`src/spec/spec_zone_orc_ruins.h`. Illithid Enclave access control uses
`src/spec/spec_zone_illithid_enclave.h`. Kobold Caverns, Bandit Castle, and Secomber access guards
use `src/spec/spec_zone_kobold_caverns.h`, `src/spec/spec_zone_bandit_castle.h`, and
`src/spec/spec_zone_secomber.h`. Longsaddle Harpell coordination uses
`src/spec/spec_zone_longsaddle.h`; Flaming Tower load-room and mirror behavior uses
`src/spec/spec_zone_flaming_tower.h`; Mere of Dead Men summoning and daylight relocation use
`src/spec/spec_zone_mere_of_dead_men.h`; and Battlemaze access control uses
`src/spec/spec_zone_battlemaze.h`. Fire Plane, Water Plane, and Snake Pit publish
`src/spec/spec_zone_fire_plane.h`, `src/spec/spec_zone_water_plane.h`, and
`src/spec/spec_zone_snake_pit.h`. Their callbacks deliberately share
`src/spec/spec_zone_alarm_group.c` so `zone_yell()` remains private beside all three consumers.
Menzoberranzan movement and Narbondel state use `src/spec/spec_zone_menzoberranzan.h`. The final
move retired `src/spec_procs.c`; its header remains the compatibility include surface. Use
`is_wearing()` from `handler.h` for the established same-VNUM equipment predicate, and use the
Phase 04 context API when exact pointer identity is required. Typed-handler conversion and general
chains remain future work.

New engine call sites must go through a gateway in `src/spec/spec_dispatch.h`
rather than calling a prototype's callback slot directly. Each gateway names
what its STOP aborts, and a caller that iterates a list must cache the successor
before invoking one: context pointers are borrowed for exactly one synchronous
call and may be invalidated by the handler.

See [OLC SpecProc Editing](OLC_SpecProcs.md), the
[Phase 00 validation matrix](../testing/SPECIAL_PROCEDURE_PHASE_00_VALIDATION.md),
the
[Phase 01 gateway matrix](../testing/SPECIAL_PROCEDURE_PHASE_01_VALIDATION.md),
the
[Phase 02 assignment matrix](../testing/SPECIAL_PROCEDURE_PHASE_02_VALIDATION.md),
the
[Phase 03 validation matrix](../testing/SPECIAL_PROCEDURE_PHASE_03_VALIDATION.md),
the
[Phase 04 validation matrix](../testing/SPECIAL_PROCEDURE_PHASE_04_VALIDATION.md),
and [architecture](../ARCHITECTURE.md).
