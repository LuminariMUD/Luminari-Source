# Architecture

## System Overview

LuminariMUD is one GNU C23 server process built on tbaMUD/CircleMUD. Its
single-threaded `select()` loop owns network I/O, heartbeat scheduling, command
dispatch, and game-state mutation. MariaDB/MySQL is a required runtime
dependency; flat files remain the authored world-data source.

The detailed source of truth is the
[core server architecture](systems/CORE_SERVER_ARCHITECTURE.md). Individual
subsystems are indexed in the
[technical documentation master index](TECHNICAL_DOCUMENTATION_MASTER_INDEX.md).

## Components

| Component | Location | Purpose |
|-----------|----------|---------|
| Game loop and networking | `src/comm.c`, `src/net/` | Connections, protocol handling, heartbeats, and shutdown |
| Command dispatch | `src/interpreter.c`, `src/act.*.c`, feature directories | Command parsing, authorization, and behavior |
| World boot and persistence | `src/db.c`, `src/mysql.c`, `lib/world/`, `sql/` | Flat-file world loading and required MariaDB state |
| Core data and mutation | `src/structs.h`, `src/utils.h`, `src/handler.c` | Shared structures, macros, and object/character lifecycle |
| Game systems | `src/combat/`, `src/magic/`, `src/character/`, `src/obj/` | Combat, spells and skills, characters, items, shops, and trade |
| Content behavior | `src/dgscript/`, `src/spec/`, feature owners | DG Scripts, special-procedure control/runtime compatibility, and feature-owned callbacks |
| Online creation | `src/olc/` | In-game room, mobile, object, zone, and related editors |
| Wilderness and transport | `src/wilderness/`, `src/vessels/`, `src/movement/` | Overworld, spatial support, vessels, vehicles, and movement |
| Operations | `scripts/autorun/`, `scripts/deployment/`, `scripts/operations/` | Supervision, immutable installation, deployment, and readiness |

## Data and Dependency Flow

```text
client -> comm.c -> interpreter.c -> command/game subsystem -> shared game state
                          |
                          `-> DG Script or special procedure when configured

lib/world/* -> db.c -> in-memory rooms, mobiles, objects, zones, shops, triggers
MariaDB <-> mysql.c and subsystem persistence <-> accounts and runtime data
```

World boot validates special-procedure definitions before parsing world files.
It also validates the owner-typed declarative compatibility table against those
definitions before any assignment can run.
Definitions, exact authored binding intent, and ordered effective boot
provenance are separate state layers; the existing callback slot remains the
runtime dispatch authority. Every engine call site now reaches that slot through
an event gateway in `src/spec/spec_dispatch.c`, which builds typed event context
where complete data still exists. The generic dispatcher reverse-resolves a
registered typed adapter and otherwise performs the exact legacy `SPECIAL`
translation. See the
[Phase 00 validation matrix](testing/SPECIAL_PROCEDURE_PHASE_00_VALIDATION.md)
and the
[Phase 01 gateway matrix](testing/SPECIAL_PROCEDURE_PHASE_01_VALIDATION.md), and
the
[Phase 02 assignment matrix](testing/SPECIAL_PROCEDURE_PHASE_02_VALIDATION.md).
Phase 03 moved unchanged legacy callbacks to coherent owners:
`src/spec/spec_objects.c` contains the audited general object procedures, while
`src/vessels/vessels_legacy.c` contains legacy route, ferry, and Greyhawk ship
behavior. `src/vessels/vessels_moving_rooms.c` owns legacy world `M` loading, the
moving-room runtime list and pulse scheduler, relocation helpers, and callback.
Player shops, commerce, crafting molds, vampire-cloak customization,
quest reward replacement, and the Neverwinter puzzle now live with their feature owners.
Character ability calculation and skill listing/training now live in
`src/character/abilities.c` and `src/character/skill_lists.c`; spell sorting and
display live in `src/magic/spell_lists.c`. General legacy mobile and room callbacks live in
`src/spec/spec_mobiles.c` and `src/spec/spec_rooms.c`; reusable combat and companion callbacks live
in `src/spec/spec_mobile_archetypes.c`. Guild services, clan-hall services, wizard research, and
pet-shop commerce live with their character, clan, magic, and object owners. The complete King's
Castle assignment and mobile package lives in `src/spec/spec_zone_kings_castle.c`, including its
private helpers and runtime state. Abyss exit randomization and Crimson Flame encounter behavior
likewise live in `src/spec/spec_zone_abyss.c` and `src/spec/spec_zone_crimson_flame.c`.
The Prisoner raid state, item and mobile callbacks, death transition, and treasury loading live in
`src/spec/spec_zone_prisoner.c`; the dormant Celestial Leviathan helpers and no-op callback live in
`src/spec/spec_zone_celestial_leviathan.c`. All other cohesive zone packages likewise live under
`src/spec/`; Fire Plane, Water Plane, and Snake Pit share `spec_zone_alarm_group.c` only to keep
their common helper private, while publishing separate owner headers. Both legacy
`src/spec_procs.c` and `src/zone_procs.c` are retired. `src/spec_procs.h` retains compatibility
includes while direct consumers migrate to owner APIs.
See the
[Phase 03 validation matrix](testing/SPECIAL_PROCEDURE_PHASE_03_VALIDATION.md).

Phase 04 adds focused context, exact phrase, object cooldown, safe damage-result, and source-owned
affect helpers under `src/spec/`. The helpers retain the production combat and affect engines,
separate `source_id` ownership from spell-scoped stacking identity, and have multiple object, mobile,
gateway, and artifact consumers. Weapon-hit context now receives the actual combat victim. See the
[Phase 04 validation matrix](testing/SPECIAL_PROCEDURE_PHASE_04_VALIDATION.md).

Phase 05 converts Bank and Vampire Cloak to typed handlers behind their unchanged callback-slot
adapters. Identification now uses explicit event identity instead of a magic argument, and Vampire
Cloak commands validate the exact invoking object. The registry contains 2 typed and 26 legacy
definitions; 194 source-level legacy behavior implementations remain, so compatibility dispatch is
still required. See the
[Phase 05 validation matrix](testing/SPECIAL_PROCEDURE_PHASE_05_VALIDATION.md).

Phase 06 retains the runtime-only `questmaster -> shop_keeper -> original callback` composition and
closes without a persisted general procedure chain. The single callback slot and single-name mobile,
object, and room formats remain authoritative. No new zone/world procedure event was added: DG
Scripts own localized lifecycle content, while stateful artifact and vessel lifecycles call their
owning subsystem directly. See the
[Phase 06 validation matrix](testing/SPECIAL_PROCEDURE_PHASE_06_VALIDATION.md).

## Operational Boundary

The existing Terrain API listener shares the main game loop and binds only to
loopback. It exposes readiness and liveness routes for systemd and local
operators. See the [operational API contract](api/README_api.md) and
[incident runbook](runbooks/incident-response.md).

## Build Boundaries

Autotools is the preferred incremental build and CMake is supported. Source
membership must remain synchronized in `Makefile.am` and `CMakeLists.txt`.
Production-linked regression tests compile against the real server sources;
the required root gate is `make test` followed by `make install`.

Architectural decisions with long-term tradeoffs belong in [ADRs](adr/). Durable special-procedure
contracts live in the [developer API](guides/DEVELOPER_GUIDE_AND_API.md),
[project considerations](CONSIDERATIONS.md), and phase validation matrices. Only the unfinished
top-level source consolidation remains in the
[special-procedure todo](ongoing-projects/spec-todo.md).
