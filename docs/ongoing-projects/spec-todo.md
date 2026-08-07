# LuminariMUD - Special Procedure Architecture Refactor PRD

## Overview

The `SPECIAL` callback serves command, pulse, combat, identification, maneuver, moving-room, shop,
and quest paths even though those callers pass different data and interpret return values
differently. This initiative makes special-procedure behavior observable and incrementally typed
while preserving the callback ABI, persisted names, world-file formats, boot precedence, activation
flags, traversal order, and runtime scheduling until a separately tested migration changes them.

Phases 00-02 are complete. Phase 03 is in progress. Phases 04-06 are not started.

## Goals

1. Make every invocation path explicit, testable, and safe to migrate.
2. Give every procedure a validated canonical identity, owner and event contracts, prerequisites,
   binding policy, and builder-visible description.
3. Make authored bindings, effective bindings, precedence, collisions, and unresolved names
   observable without silent OLC data loss.
4. Reorganize content by primary responsibility while preserving behavior and keeping cohesive
   feature and zone packages together.
5. Introduce shared mechanics and typed handlers only after real consumers establish their contracts.
6. Keep builder, operator, developer, helpfile, and architecture documentation aligned per phase.

## Non-Goals

- Rewriting every legacy `SPECIAL` handler, or migrating every hard-coded assignment, in one change.
- Changing mob, object, or room world-file format during the core refactor.
- A generic asynchronous event bus or speculative zone/world event catalog.
- Moving artifact ownership, custody, progression, or persistence into the general spec subsystem.
- Changing command traversal, heartbeat scheduling, activation flags, or boot precedence incidentally.
- Replacing DG Scripts for localized narrative, dialogue, puzzle, or sequencing behavior.
- Modifying local configuration headers or production configuration.

## Users

- **Engine maintainer**: evolves dispatch, registries, bindings, combat safety, and source
  organization without behavior drift.
- **Builder/staff**: selects compatible procedures in OLC and diagnoses binding state, event
  support, prerequisites, and collisions.
- **Content developer**: adds reusable or cohesive behavior through the correct subsystem and
  binding source.
- **Operator/test maintainer**: validates boot metadata, effective bindings, `-s` mode, runtime
  safety, and production-linked regression coverage.

## Phases

| Phase | Name | Status |
|-------|------|--------|
| 00 | Registry Safety and Observability | Complete (2026-08-07) |
| 01 | Call-Site Gateway Compatibility | Complete (2026-08-07) |
| 02 | Declarative Legacy Assignments | Complete (2026-08-07) |
| 03 | Behavior-Preserving Content Extraction | In Progress (2026-08-07) |
| 04 | Narrow Shared Mechanics | Not Started |
| 05 | Incremental Typed Handlers | Not Started |
| 06 | Conditional Composition and Lifecycle Hooks | Not Started |

### Phase 00 - Registry Safety and Observability (Complete)

Delivered a validated control plane around the unchanged `SPECIAL` ABI: 28 immutable canonical
definitions with 29 lookup names in `src/spec/spec_registry.c`; boot validation before world
parsing; owner-filtered OLC via `src/olc/spec_menu.c`; prototype-owned authored records preserving
aliases, unknown names, and source locations (`src/spec/spec_binding.c`); authored-first writers;
effective-binding provenance and collision reporting (`src/spec/spec_effective_binding.c`);
order-independent moving-room `M` plus room `Z` rejection; database-first `SPECIALS` help
(`sql/components/help_specproc_entries.sql`); and 78 dedicated production-linked tests.

Phase 00 did not add gateways, typed contexts, invalidation results, declarative assignment tables,
content extraction, shared mechanics, typed-handler conversion, or composition.

Acceptance evidence:
[Special Procedure Phase 00 Validation](../testing/SPECIAL_PROCEDURE_PHASE_00_VALIDATION.md).

### Phase 01 - Call-Site Gateway Compatibility (Complete)

Delivered one testable seam between every engine call site and the unchanged `SPECIAL` ABI:
`struct spec_event_context` with gateway-local flow and independent invalidation
(`src/spec/spec_dispatch.h`); fourteen gateways covering command, mobile activity, mobile combat
turn, object auto-pulse, item identify, weapon hit, defense reaction, combat maneuver, mount charge,
moving-room relocation, and shop/quest secondary forwarding (`src/spec/spec_dispatch.c`); exact
translation of every `ch`, `me`, `cmd`, argument token, and caller-specific return; typed target
payloads captured at the call site rather than inferred from `FIGHTING()`; and 12 dedicated
production-linked tests.

Two intentional extraction-safety corrections shipped with the routing: `special()` and
`proc_update()` now cache the iteration successor before invoking a callback, so a handler that
extracts its owner and returns zero can no longer leave the caller following cleared storage.

Phase 01 converted no handlers and registered no typed handler, so the notification-only contract
error has no producer yet. Traversal order, stop rules, pulse scheduling, activation flags, boot
precedence, and `-s` behavior are unchanged.

Acceptance evidence:
[Special Procedure Phase 01 Validation](../testing/SPECIAL_PROCEDURE_PHASE_01_VALIDATION.md).

### Phase 02 - Declarative Legacy Assignments (Complete)

Delivered owner-specific mobile, object, and room row contracts plus registry-backed validation in
`src/spec/spec_assign_table.c`. Boot validates every declarative row after the definition registry
and before world parsing. The Luminari table converts the two assignments that meet both required
prerequisites: `NOOB_CRAFTING_KIT` / `Crafting Kit` and `VAMPIRE_CLOAK_OBJ_VNUM` / `Vampire Cloak`.
Both use the same assignment and effective-provenance path as direct compatibility calls.

The source inventory explains the intentionally narrow conversion. After those two rows moved, 783
`ASSIGNMOB` / `ASSIGNOBJ` / `ASSIGNROOM` tokens remain in `src/spec_assign.c`; 777 carry numeric
literals. The other non-literal occurrences are macro definitions, computed setup, or
campaign-compatibility branches. Unsupported rows remain direct as required: inventing literals in a
table would not make them traceable, and adding hundreds of registry identities or local VNUM
configuration changes is outside this phase. Every direct callback write still records ordered
effective provenance and collisions.

Immortal staff can inspect the full recorded post-boot chain with
`specbind <mob|obj|room> <vnum>`, including source locations, outcomes, collision count, shop/quest
saved secondaries, and final source. The established named-world, parser-hook, legacy-assignment,
shop, and quest precedence is unchanged and no binding moved to world data. Eleven new
production-linked tests cover row resolution, aliases, owner/source rejection, table diagnostics,
null handling, and stable source labels.

Acceptance evidence:
[Special Procedure Phase 02 Validation](../testing/SPECIAL_PROCEDURE_PHASE_02_VALIDATION.md).

### Phase 03 - Behavior-Preserving Content Extraction

Checkpoints 1-8 extracted the complete audited general object section to
`src/spec/spec_objects.c`, moved legacy route/ferry/Greyhawk behavior to
`src/vessels/vessels_legacy.c`, and placed Neverwinter, vendor, crafting-mold, vampire-cloak, and
quest-service callbacks with their true owners. `floating_teleport` is a reusable cross-zone object
callback, not vessel behavior. Ability calculations and skill display/training now live under
`src/character/`; spell sorting and display live under `src/magic/`. The legacy moving-room `M`
loader, runtime list, zone-pulse scheduler, relocation helpers, and callback now form one package in
`src/vessels/vessels_moving_rooms.c`. General legacy mobiles and rooms now live in
`src/spec/spec_mobiles.c` and `src/spec/spec_rooms.c`; class guild services, wizard spellbook
research, and pet-shop commerce live with `src/character/`, `src/magic/`, and `src/obj/`. Reusable
combat and companion archetypes now live in `src/spec/spec_mobile_archetypes.c`, while clan-hall
cleric and guard services live in `src/clan_services.c`. The Celestial Leviathan stub remains with
the legacy file until its encounter package moves from `zone_procs.c`. The complete King's Castle,
Abyss, and Crimson Flame packages now live in dedicated `src/spec/spec_zone_*` owners;
`zone_procs.c` is 3,123 lines, down 1,079 lines from its Phase 03 baseline. Autotools and CMake link
every new source for production and CuTest. The callback ABI, exported symbols, registry identities,
assignments, world grammar, scheduling, and behavior remain unchanged. `src/spec_procs.c` is 1,960
lines, down 10,252 lines from the Phase 03 baseline.

1. Extract general object procedures first, after gateway coverage.
2. Extract reusable mobile and room procedures.
3. Split `zone_procs.c` by its existing cohesive zone packages.
4. Move vessel, vendor, crafting, and ability work to its true owners.
5. Preserve exported names, callback ABI, static-state ownership, initialization order, behavior.

Exit when responsibilities are coherent, characterization is unchanged, and both build manifests
list identical production and test sources.

Current evidence and the interruption-safe resume point are recorded in
[Special Procedure Phase 03 Progress](../testing/SPECIAL_PROCEDURE_PHASE_03_PROGRESS.md).

### Phase 04 - Narrow Shared Mechanics

1. Pointer-identity context validation for representative object and mobile procedures.
2. Opt-in phrase/command parsing, only after accepted-input characterization.
3. Cooldown operations with explicit clock, units, storage, bounds, persistence, and commit rules.
4. Safe target and combat-result contracts wrapping existing primitives.
5. Affect helpers built on `source_id`, keeping stacking identity separate.

Exit when every helper names a documented rule, has at least two consumers, and has focused tests.
Do not mechanically convert every procedure.

### Phase 05 - Incremental Typed Handlers

1. Implement new procedures as typed handlers behind existing gateways.
2. Convert a legacy procedure when it is otherwise changing, or when conversion removes a proven
   safety risk.
3. Preserve canonical persisted identities; compare against characterization tests.
4. Track the remaining legacy population before considering removal of compatibility support.

Exit when converted handlers no longer infer event data from magic strings or ambient combat state.

### Phase 06 - Conditional Composition and Lifecycle Hooks

1. Gather concrete prototype-composition and lifecycle use cases.
2. Design and test inner-chain order without changing outer command-owner traversal.
3. Migrate shop and quest secondaries deliberately.
4. Add only zone and world hooks required by approved content.
5. Version affected persistence; retain backward-compatible loading.

Exit when each abstraction has a real consumer with complete ordering, lifetime, and compatibility
coverage. This phase may close with composition or lifecycle events intentionally unimplemented.

### Sequencing Guardrail

Gateways (01) precede general object extraction (03). Helpers emerge from audited consumers rather
than being bundled into a file move.

## Non-Functional Requirements

- **Compatibility**: characterization must cover all 12 invocation categories, exact legacy argument
  tokens, caller-specific return handling, command-owner order, heartbeat positions, activation
  flags, boot precedence, and normal versus `-s` behavior before contracts change.
- **Registry integrity**: all registered definitions pass pre-world-load validation; no duplicate
  canonical names or aliases, empty required metadata, invalid masks, or handlerless definitions may
  reach world parsing.
- **Data integrity**: no unresolved authored name or provenance record may be erased or promoted by
  an unrelated OLC save, proven by production-linked round-trip tests.
- **Runtime safety**: every migrated gateway permitting mutation caches iteration successors before
  invocation and tests owner, actor, target, death, and extraction paths. No migrated path may probe
  a freed object after return.
- **Build quality**: each slice compiles with zero new `-Wall -Wextra` warnings, passes root
  `make test`, then `make install` with no root-level `circle` artifact left.
- **Build parity**: every added or removed production and CuTest source is reflected in both
  `Makefile.am` and `CMakeLists.txt` in the same change.
- **Documentation**: ASCII-only UTF-8, LF endings, distinguishing implemented behavior from proposals.

## Constraints and Dependencies

- Luminari is the only design target; legacy campaign branches are compatibility inventory.
- GNU C23 and the CircleMUD/tbaMUD architecture, macros, structures, and callback ABI constrain the
  migration. MariaDB/MySQL is required for the server and the full test environment.
- Stable procedure names and mob/object/room file formats are content-facing compatibility contracts.
- Command traversal order, pulse scheduling, event strings, caller-specific returns, `MOB_SPEC`,
  `ITEM_AUTOPROC`, `no_specials`, shop/quest chaining, and boot precedence are observable behavior.
- Moving-room `M` records and room `Z` bindings share a callback slot with incompatible payloads.
- New sources use the shallow `src/spec/` directory; no second-level source nesting.
- Use existing symbolic VNUMs. Never modify `src/vnums.h`, `src/campaign.h`, `src/mud_options.h`,
  `lib/.env`, or `lib/mysql_config`; edit the example templates instead.
- DG Scripts remain preferred where behavior needs no engine-level state, reuse, performance,
  combat, persistence, or lifecycle guarantee.

## Whole-Program Success Criteria

Phase 00 satisfies several criteria within its own scope; they must continue to hold as later phases
land.

| Criterion | Status |
|-----------|--------|
| Every invocation category has characterization coverage before its gateway migration changes flow. | Met: all 12 categories characterized in Phase 00 and re-verified against their Phase 01 gateways. |
| Every persisted definition has one canonical identity, aliases, owner/event metadata, visibility, prerequisites, category, and description. | Met by Phase 00. |
| Unknown or incompatible names are diagnosable and cannot be silently erased by unrelated OLC saves. | Met by Phase 00. |
| OLC writers preserve authored provenance instead of serializing an effective override via reverse pointer lookup. | Met by Phase 00. |
| Effective bindings and sources are inspectable for every mob, object, and room prototype. | Met by Phase 00. |
| Gateway callers honor flow and pointer-lifetime contracts while preserving scheduling, traversal, activation, and returns. | Met by Phase 01. |
| Shared helpers state clock, ownership, persistence, stacking, and invalidation rules and have at least two real consumers with tests. | Open (Phase 04). |
| File organization follows primary responsibility, with both build systems synchronized. | Open (Phase 03). |
| Root `make test` and `make install` pass with the server installed at `bin/circle`. | Standing gate; passed at Phase 03 Checkpoint 8 (574 tests). |
| Builder, help, system, and architecture documentation matches every implemented phase. | Standing gate; met through Phase 03 Checkpoint 8. |

## Risks and Guardrails

- **Behavior drift during extraction**: require independently buildable, behavior-preserving moves
  after characterization exists, preserving includes, declarations, static state, and init order.
- **Handler-side information loss**: build typed context at each call site; never claim typed target,
  damage, critical, or lifecycle safety from a wrapper placed after callers discarded that data.
- **Hidden binding precedence**: record the complete post-boot chain and preserve current outcomes
  until each collision has a deliberate, regression-tested migration.
- **Unknown-name data loss**: a log is insufficient; retain unresolved authored identity or block
  implicit OLC overwrite until the builder explicitly replaces or clears it.
- **Lifetime violations**: cache successors and obey event-specific invalidation; never probe freed
  objects after a callback.
- **Moving-room payload collision**: reject combined `M` and `Z` ownership until relocation has a
  separate typed hook.
- **Over-generalized helpers**: require a semantic contract and two consumers before adding shared
  cooldown, affect, targeting, combat, or chance modules.
- **Activation flags**: expose `MOB_SPEC`, `ITEM_AUTOPROC`, combat, and placement prerequisites;
  automatic flag mutation is a separate content migration.
- **Cooldown ambiguity**: name units, clock, storage, persistence, reboot behavior, commit timing.
- **Affect collisions**: coordinate source and stacking namespaces across subsystems.
- **Artifact coupling**: keep artifact identity, custody, progression, persistence, and recovery
  outside the general subsystem.
- **New dumping grounds**: assign files by primary subsystem responsibility; keep cohesive zone
  packages together.
- **Unbounded event infrastructure**: prefer direct typed hooks until multiple consumers prove a
  registry or bus is needed.
- **Persistence incompatibility**: preserve canonical names and single-name formats; version any
  later composition format with backward-compatible loading.

## Open Decisions

1. Whether content needs justify multiple procedures on one prototype; Phase 06 stays conditional
   until ordering and persistence requirements have a real consumer.
2. Which zone or world lifecycle hooks, if any, approved content needs after direct typed hooks
   prove a shared contract.
3. Whether the independent artifact file split is scheduled inside or outside this initiative.

## Current-State Evidence

Baseline analysis was verified on 2026-08-06 against commit `af9f79d2`, covering callback
declarations and owner storage (`src/structs.h`, `src/db.h`, `src/utils.h`); call sites
(`src/interpreter.c`, `src/mob/mob_act.c`, `src/comm.c`, `src/combat/fight.c`,
`src/combat/act.offensive.c`, `src/obj/act.item.c`); registry, assignment, world loading, and OLC
persistence (`src/spec_assign.c`, `src/db.c`, `src/olc/`); shop and quest composition
(`src/obj/shop.c`, `src/quest/quest.c`, `src/olc/genqst.c`); cooldown, affect, damage, and
object-save contracts; artifact code and tests; and persisted bindings under `lib/world/`.

Phases 00-02 have since changed registry, OLC, dispatch, provenance, and eligible assignment behavior
described in that baseline. Statements below are marked where a completed phase superseded them.
Dated counts are snapshots, not invariants; re-trace symbols during implementation.

### Source Inventory

| File | Lines (2026-08-07) | Responsibilities |
|------|-------------------:|------------------|
| `src/spec_procs.c` | 12,212 | Mixed abilities, entity procedures, features, helpers |
| `src/obj/spec_artifacts.c` | 6,489 | Artifact systems and test seams |
| `src/zone_procs.c` | 4,202 | Bespoke zone and encounter behavior |
| `src/spec_assign.c` | 1,217 | Assignment macros and boot assignment |

File size is a review signal, not the defect. The problem is that unrelated behaviors share files
and one callback ABI without declaring their event or owner contracts.

### Legacy Callback and Invocation Matrix

```c
int handler(struct char_data *ch, void *me, int cmd, const char *argument);
```

This one ABI serves 12 invocation categories:

| Invocation | Caller | Legacy Signal | Return Meaning |
|------------|--------|---------------|----------------|
| Command | `special()` | Command and argument | Nonzero consumes the command |
| Mobile activity | `mobile_activity()` | `cmd == 0`, `""` | Nonzero skips default AI |
| Mobile combat turn | Combat turn | `cmd == 0`, `""` | Ignored |
| Object auto-proc | `proc_update()` | `cmd == 0`, `""` | Controls carried fallback |
| Item identification | Item display | `cmd == 0`, `"identify"` | Ignored |
| Weapon hit | `weapon_special()` | `cmd == 0`, hit token | Ignored |
| Defense reaction | Combat messaging | `cmd == 0`, reaction token | Ignored |
| Shield maneuver | Shield commands | `cmd == 0`, maneuver token | Ignored |
| Mounted charge | `perform_charge()` | `cmd == 0`, `"charge"` | Ignored |
| Moving room | `moving_rooms_update()` | Nulls, `cmd == 0`, state in `me` | Ignored |
| Shop secondary | `shop_keeper()` | Incoming context unchanged | Nonzero propagates |
| Quest secondary | `questmaster()` | Incoming context unchanged | Nonzero propagates |

Defense tokens: `"shieldblock"`, `"parry"`, `"glance"`, `"dodge"`. Maneuver tokens:
`"shieldpunch"`, `"shieldcharge"`, `"shieldslam"`.

### Scheduling and Traversal

- The heartbeat invokes moving-room relocation every ten seconds.
- On `PULSE_MOBILE`, `mobile_activity()` runs before `proc_update()`.
- A mobile combat callback runs after that combatant's attacks and cleave handling.
- Command dispatch order: room, equipped objects in wear-slot order, carried objects, mobiles in
  room-list order, then room contents. The first nonzero result stops later owner traversal.
- Object auto-proc may invoke once with a null `ch`, then with `carried_by` if the first result is 0.
- The moving-room path selects a room function but passes `struct moving_room_data *` through `me`.

These positions and stop rules must not change without their own specification and tests.

### Lost Context and Return Semantics

Most internal events use `cmd == 0`; only the argument string, call site, and ambient state identify
the event. A weapon-hit callback receives neither target nor damage and commonly infers the target
from `FIGHTING(ch)`. A handler-side wrapper cannot reconstruct data the caller already discarded, so
typed context must be built at each call site.

The integer return is not a universal contract: command consumed, mobile activity skipped, carried
fallback skipped, or nothing. Typed flow must stay gateway-specific.

**Superseded by Phase 01 for the command and auto-proc traversals**, which now cache successors
before every callback. The general rule still holds elsewhere: a handler that extracts its owner and
returns zero can leave a caller following freed storage. Character pending-extraction flags may be
inspected where available; no safe post-free object probe exists.

### Single-Handler Storage and Compatibility Composition

`room_data` and mobile/object `index_data` each store one function pointer. Shops and quests compose
by saving an existing callback as a secondary and invoking it from `shop_keeper` or `questmaster`;
boot order can produce quest-over-shop-over-original nesting. This limit is per prototype - command
dispatch already traverses several owners, and future composition must not alter that outer
traversal.

The room pointer also carries moving-room lifecycle behavior: an `M` record installs `moving_rooms`
and a later `Z` record writes the same slot. Composition cannot make these payloads safe.
**Phase 00 rejects the combination at load, edit, and write boundaries.** The verified world
snapshot contains no numeric moving-room `M` records, so this is a latent collision, not a migration
inventory.

### Activation Flags and `no_specials`

- Mobile command dispatch uses a non-null pointer without checking `MOB_SPEC`.
- Mobile activity and combat-turn dispatch require both `MOB_SPEC` and a non-null pointer.
- `mobile_activity()` removes `MOB_SPEC` when the flag exists but the pointer is null.
- Object command, identification, and combat callers use the pointer directly; periodic
  `proc_update()` additionally requires `ITEM_AUTOPROC`.
- Selecting a procedure in OLC does not establish required flags or placement.

`-s` / `no_specials` is not a global gate. It skips hard-coded assignment and shop loading, bypasses
command special dispatch, and suppresses mobile activity. World-file names still resolve during
loading, and identification, auto-proc, moving-room, and combat call sites do not check it. Tests
must cover both modes per call site.

### Registry and Persisted Bindings

**Superseded by Phase 00.** The baseline `spec_func_list[]` in `src/spec_assign.c` exposed 29
persisted names over 28 handlers with empty descriptions, no description accessor, and one untyped
list shown by medit, oedit, and redit - allowing incompatible owner selections. Unknown names
resolved to `NULL` silently, raw text was discarded, and writers serialized the effective callback
via reverse pointer lookup, so boot-time overrides could be promoted into world data.

Current behavior: `src/spec/spec_registry.c` holds 28 immutable canonical definitions with 29
lookup names (`Guildmaster` is an explicit alias of canonical `Guild`), validated before world
parsing. `src/spec/spec_binding.c` owns authored identity including unresolved raw names;
`src/spec/spec_effective_binding.c` records effective provenance and collisions;
`src/olc/spec_menu.c` filters selection by owner and shows descriptions and prerequisites. Lookup
remains case-insensitive.

Persisted world bindings (verify again before Phase 02):

| Owner | Persisted Name | Source Files |
|-------|----------------|--------------|
| Mobile | `Postmaster` | `lib/world/mob/12.mob` |
| Object | `Greyhawk Ship` | `lib/world/obj/14.obj`, `lib/world/obj/700.obj` |
| Room | `Greyhawk Ship Commands` | `lib/world/wld/14.wld`, `lib/world/wld/700.wld` |

World-file binding is not yet the dominant source; the registry covers only a subset of the
hard-coded procedures in `spec_procs.c` and `zone_procs.c`.

### Registration, Assignment, and Boot Precedence

Registration makes an identity resolvable by world files and OLC. Assignment binds a procedure to a
hard-coded prototype VNUM through `ASSIGNMOB`, `ASSIGNOBJ`, and `ASSIGNROOM` in `src/spec_assign.c`.
New reusable content should normally use a registered world-data binding; hard-coded assignment is a
compatibility layer until audited. The two responsibilities must not stay conflated.

With specials enabled, preserve this effective boot precedence:

1. `boot_world()` loads persisted mobile, object, and room names into callbacks.
2. `assign_mobiles()` overwrites matching mobile callbacks.
3. `assign_the_shopkeepers()` saves the current mobile callback as a secondary, installs
   `shop_keeper`.
4. `assign_objects()` and `assign_rooms()` overwrite matching object and room callbacks.
5. `assign_the_quests()` saves the current quest-master callback as a secondary, installs
   `questmaster`.

Inventory and test the result of the complete sequence, including saved secondaries. Do not infer
behavior from an isolated `ASSIGN*` call. Collisions must preserve the verified outcome while
reporting every contributing source.

### File Ownership Problems

`zone_procs.c` holds zone-associated mobile and object procedures, encounter state, and helpers; it
is not a zone callback system, and `struct zone_data` has no special-procedure callback. Future zone
events need an explicit lifecycle interface, not a zone pointer hidden in `void *me`.

At the Phase 03 baseline, `spec_procs.c` also held work owned elsewhere: spell/skill/ability listing
and calculation; moving-room and legacy ship behavior; vendor item construction and naming; and
crafting-mold purchase and construction. Checkpoints 1-8 have moved every item in that list, the
traced general mobile/room slice, reusable combat/companion archetypes, and clan services. The
King's Castle, Abyss, and Crimson Flame packages have also moved intact from `zone_procs.c`. The
remaining legacy callbacks are cohesive zone content. Splitting them by owner type alone would still
preserve zone-ownership mistakes. Moving rooms retain their temporary gateway and now live in the
vessel subsystem; a direct typed hook remains a later behavior-changing phase.

## Design Principles

1. Preserve observable behavior before redesigning dispatch.
2. Keep immutable definitions, authored/effective bindings, and invocation gateways separate.
3. Construct typed context where complete event data still exists.
4. Treat context pointers as borrowed for one synchronous invocation; report invalidation separately
   from control flow.
5. Create narrow semantic helpers that own game rules, not wrappers that rename primitives. Safe
   offensive-target resolution qualifies; a rename of `damage()` or `send_to_char()` does not.
6. Group reusable behavior by owner type, but keep cohesive feature and zone content together.
7. Prefer world-data binding for new content while preserving legacy precedence during migration.
8. Keep DG Scripts in the design decision. C procedures are preferred only for engine-level hooks,
   broad reuse, performance-sensitive execution, strong combat/persistence guarantees, or lifecycle
   contracts scripts cannot safely provide.

## Target Architecture

### Responsibility Boundaries

1. Event-specific invocation gateways plus exact legacy `SPECIAL` translation.
2. A typed, validated, immutable definition registry with stable persisted identities.
3. A binding layer shared by boot code, world data, OLC, legacy assignments, parser hooks, shops,
   and quests.
4. Narrow reusable mechanics for audited eligibility, parsing, cooldown, targeting, combat, affect,
   and chance rules.
5. Authored behavior organized by reusable owner type or cohesive feature/zone ownership.

Do not create a general `spec_utils.c`. Do not make the artifact subsystem the parent of general
object procedures. The registry must not mutate prototypes; gateways must not decide where a
procedure is bound.

### Layout

Shipped control plane (Phases 00-02):

```text
src/spec/spec_registry.c|.h
src/spec/spec_binding.c|.h
src/spec/spec_effective_binding.c|.h
src/spec/spec_dispatch.c|.h
src/spec/spec_assign_table.c|.h
src/olc/spec_menu.c|.h
```

Shipped content ownership (Phase 03 Checkpoints 1-8):

```text
src/spec/spec_objects.c
src/spec/spec_mobile_archetypes.c|.h
src/spec/spec_mobiles.c|.h
src/spec/spec_rooms.c|.h
src/spec/spec_zone_abyss.c|.h
src/spec/spec_zone_crimson_flame.c|.h
src/spec/spec_zone_kings_castle.c|.h
src/spec/spec_zone_neverwinter.c
src/vessels/vessels_legacy.c
src/vessels/vessels_moving_rooms.c|.h
src/obj/player_shop.c
src/obj/vendor.c|.h
src/craft/crafting_molds.c
src/character/abilities.c|.h
src/character/guild_services.c|.h
src/character/skill_lists.c|.h
src/character/vampire_cloak.c
src/clan_services.c|.h
src/magic/spellbook_scroll.c|.h
src/magic/spell_lists.c|.h
src/quest/quest_services.c
```

Proposed for the remainder of Phase 03 and later phases, subject to traced ownership:

```text
src/spec/spec_cooldown.c|.h        (needs two real consumers)
src/spec/spec_effects.c|.h         (needs two real consumers)
src/spec/spec_zone_<package>.c     (The Prisoner, Celestial Leviathan, Fire Giant, Jot,
                                    Mad Drow, Cube Slider, TTF)
```

This is a responsibility map, not permission to create empty modules. Top-level `spec_procs.c` and
`spec_procs.h` remain compatibility surfaces and shrink over time. External includes are
path-qualified (`#include "spec/spec_registry.h"`). Every source addition or removal updates both
build manifests.

### Control Flow

```text
engine call site -> event gateway -> typed handler
                              `----> legacy event translator -> SPECIAL handler

world data ----\
OLC ------------> binding resolver --lookup--> definition registry
legacy assign --/          |
system parser hook --------|
shop/quest wrappers -------| (explicit compatibility composition)
                           `--apply current precedence--> effective prototype callback
```

### Typed Runtime Context

The first typed API models only current invocation paths. Exact C representation is a session-level
choice; the semantic separation is not.

| Concept | Required Values or Fields |
|---------|---------------------------|
| Owner type | Mobile, object, room |
| Event | Command, mobile activity, mobile combat turn, object auto-pulse, item identify, weapon hit, defense reaction, combat maneuver, mount charge, moving-room relocation |
| Owner | Typed pointer matching owner type |
| Actor | Current character when supplied; otherwise explicitly null |
| Command payload | Command identifier and argument |
| Weapon-hit payload | Actual target, damage, critical state |
| Defense payload | Actual target and reaction identity |
| Maneuver payload | Actual target and maneuver identity |
| Moving-room payload | Moving-room state and destination room |
| Outcome flow | Continue or gateway-local stop |
| Outcome invalidation | Independent owner, actor, and target flags |

The non-binding reference design used `SPEC_OWNER_*`, `SPEC_EVENT_*`, `SPEC_FLOW_CONTINUE`/`_STOP`,
and `SPEC_INVALIDATE_NONE|OWNER|ACTOR|TARGET`. Event-specific payload structs may replace a union.
Zone and world lifecycle events are not initial event values.

### Gateway Flow and Invalidation

`STOP` is never one universal action:

| Gateway | Stop Contract |
|---------|---------------|
| Command | Consume the command, stop later owner traversal |
| Mobile activity | Skip remaining default activity for this mobile |
| Object auto-pulse | Skip the carried-object fallback invocation |
| Typed combat action | Abort only the surrounding action named by that gateway |
| Notification-only compatibility event | Invalid; log a contract error and continue safely |

Invalidation is independent of flow: a handler can invalidate an owner or target without consuming a
command. Callers cache successors and do not inspect potentially freed objects. Legacy handlers keep
receiving the exact current `ch`, `me`, `cmd`, argument, and return interpretation until individually
converted and tested.

Initial gateway coverage: command traversal; mobile activity and combat turns; object auto-proc
worn/carried fallback; item identification; weapon hits and defensive reactions; shield maneuvers and
mounted charge; moving-room relocation; shop and quest secondary forwarding.

### Registry and Binding Contracts (Implemented)

Implemented in Phase 00; the invariants below must survive later phases. Field-level details live in
`src/spec/spec_registry.h`, `src/spec/spec_binding.h`, and the Phase 00 validation document.

Each immutable definition carries a persisted `canonical_name` plus separate display name, explicit
aliases, owner mask, per-event prototype-flag and placement prerequisites, binding-source mask and
builder visibility, exactly one handler, and non-empty description and category. Boot validation runs
before world parsing: invalid metadata is a programmer error and fails boot; an unknown world-data
name is a content error whose source location and raw identity are preserved. Aliases never become
canonical through reverse pointer lookup, and accessors are bounds-safe at both extremes. Builders
explicitly replace or clear an unresolved request; the effective callback stays empty until content
resolves it.

Each binding record carries owner type and prototype identity, requested name including owned
unresolved raw text, resolved definition when known, source (world data, parser hook, legacy
assignment, shop wrapper, quest wrapper), effective result after precedence, and saved secondary
callback information. The prototype function pointer remains the effective slot during compatibility
migration. Binding metadata travels with the prototype or OLC copy, not only a global mutable-rnum
array; unresolved names require explicit copy and free ownership. Writers consult authored state
first and reverse pointer lookup only as a legacy fallback. A world binding overwritten by a legacy
assignment produces one structured diagnostic naming both sources and the chosen result.

### Declarative Assignment Contract (Implemented for Eligible Rows)

Hard-coded assignments may become validated data holding owner type, a typed VNUM, and canonical
definition name. Prefer owner-typed tables or a tagged VNUM union so a room constant cannot enter a
mobile row - use `mob_vnum`/`obj_vnum`/`room_vnum`, not a compact `int vnum`. Every row requires a
registered definition, but metadata must not make hard-coded-only or zone-private behavior
builder-selectable.

Use traced symbolic VNUM constants, never literals. Computed assignments and special setup stay with
their owning systems. If no symbolic constant exists, defer conversion or separately change
`src/vnums.example.h`. Phase 02 therefore converted two eligible Luminari object rows and left the
unsupported numeric and computed inventory on the observable compatibility path. Audit complete
effective post-boot bindings before deciding which hard-coded entries move to world data.

## Reusable Mechanics Contracts

These are Phase 04 targets. Each helper needs at least two real consumers before it is written.

### Context Validation

Validators reject owner/event mismatches, missing actors or targets, invalid rooms, absent combat
state, missing flags, and unsupported placement.

The existing `obj_proc_ready()` matches equipment by VNUM through `is_wearing()` and does not prove
the invoking instance is worn. General validation must use pointer identity (`obj->worn_by ==
actor`). No validator may claim to recognize an object after it is freed.

### Command and Phrase Matching

Shared parsing may cover exact command matching, argument splitting, case and punctuation
normalization, target-bearing prefix phrases, and handled-versus-unrelated input. The artifact
called-effect dispatcher is precedent for one data row owning phrase, channel, target rule, recharge,
description, and effect dispatch. Normalization is opt-in: every migrated procedure characterizes its
current abbreviation, punctuation, and case behavior before accepting a shared matcher.

### Cooldowns

Two incompatible models exist:

- Legacy object `spec_timer[]` counters decrement once per `point_update()`, currently once per MUD
  hour (`SECS_PER_MUD_HOUR` is 75 real seconds). They belong to the object instance, are not
  serialized by `src/obj/objsave.c`, and reset on recreation or restart.
- Artifact `time_t` stamps use wall-clock seconds and are persisted and restored by artifact
  persistence.

Event-backed and character-specific mechanisms also exist and are not interchangeable. Every shared
cooldown contract names its clock and units, storage owner and slot bounds, persistence and reboot
behavior, commit point, and remaining-time display. Validation and successful execution normally
precede spending; target failure, stacking rejection, immunity, or another no-effect result does not
spend cooldown unless explicitly required.

### Combat and Target Safety

Verify as applicable: actor and target alive and not pending extraction; both in the same room;
offensive actions pass `aoeOK` or the applicable aggression rule; required combat state exists;
equipped objects are the exact instances worn by the actor; damage results distinguish no damage,
damage, death, and extraction as existing primitives expose them; multi-target loops retain their
next pointer before effects can remove an entry; extra-attack procedures cannot recursively trigger
themselves without a bound.

Helpers wrap existing combat primitives rather than build a second combat engine. Preserve the result
of `damage()` and name the exact caller action that must stop.

### Temporary Affects and Stacking

Build on `affect_to_char_source()`, `affect_from_char_source()`, `affected_by_spell_source()`, and
`affect_join_source()` with `affected_type.source_id`; do not add a second general source field.

Artifact code uses `affected_type.specific` for two concepts: a registry-derived tag on passive or
permanent affects, and a stacking group on temporary surge affects. A general contract must keep
source ownership and stacking group separate and provide a namespaced source identity with documented
lifetime; an explicit stacking group whose namespace and range are coordinated with spell and
artifact use; bonus type, location, modifier, duration, and flags; removal by source without
stripping unrelated effects; and an explicit result when another effect occupies the stacking group.
Artifact XP and progression stay outside this helper.

### Chance and Proc Policy

Reusable policy may support validated percentage rolls, clearly named one-in-N rolls, optional
bad-luck protection, independent versus shared cooldowns, and deterministic test injection. The
policy must be visible in data or function names so generic, signature, and nested extra attacks
cannot interact invisibly.

## Artifact Boundary

Generalizable from artifact code: registry membership as source of truth; table-driven templates and
called-effect contracts; central target resolution before effect dispatch; data-selected procedure
shapes; persisted cooldown stamps; affect-source ownership with explicit stacking groups; boot-time
metadata validation; deterministic random-combat test seams; validate-then-execute-then-spend
ordering; and explicit core hooks such as `artifact_weapon_proc()`.

Artifact-specific and non-generalizable: unique-instance enforcement; owner and account binding;
custody and provenance history; XP and levels; save format and dirty-state persistence; class
rejection and burn behavior; chronicle and recovery policy; artifact-specific effect text.

The general subsystem depends on neither `artifact_data` nor artifact VNUMs. Artifact code may call
general helpers; general helpers never call artifact progression or custody.

If separately scheduled, an artifact split under `src/obj/` may use `artifact_registry.c`,
`artifact_persistence.c`, `artifact_ownership.c`, `artifact_effects.c`, `artifact_combat.c`, and
`artifact_commands.c`, retaining one public API, one internal header, production-linked
`test_artifacts.c` and `test_artifact_integration.c`, and both build manifests updated.

## Content Ownership Map

- **Mobiles**: guards, pets, janitors, practice targets, common undead, service NPCs are candidates
  for `spec_mobiles.c` or their true owning subsystem. Shop and quest entry points stay with those
  systems even though mobiles own them.
- **Objects**: the contiguous object-procedure section of `src/spec_procs.c` is the first extraction
  candidate after gateway coverage; it already has the partial `obj_proc_ready()` helper and repeats
  cooldown, combat, chance, affect, and messaging patterns. Artifacts stay under `src/obj/`; vessel
  objects and controls under `src/vessels/`; shop and trade under `src/obj/`; crafting purchase and
  construction under `src/craft/`.
- **Rooms**: general room behavior may share a room-procedure module. Moving rooms and vessel control
  rooms belong to the vessel subsystem.
- **Zones**: keep the natural packages in `zone_procs.c` intact with private helpers and encounter
  state file-local. Extract shared mechanics only when two packages need the same contract.

## Conditional Composition and Lifecycle Hooks

A future chain is internal to one prototype and must not change outer command traversal. Current
shop and quest secondaries are explicit compatibility composition, not a general chain.

Before a chain ships, define and test: deterministic order from persisted definition IDs and explicit
policy (never function addresses or registry order); binding source and collision behavior per entry;
per-entry event compatibility; gateway-specific continue/stop semantics; owner, actor, and target
invalidation including stable-snapshot versus live-chain behavior when a handler mutates bindings
during dispatch; whether a command result stops only the inner chain or outer traversal too;
duplicate-handler policy and bounded chain length; a versioned backward-compatible persistence format
for multiple names; OLC display, reorder, clear, unresolved-name, and save behavior; and deliberate
migration of quest-over-shop-over-original nesting.

Composition touches `room_data`, mobile and object `index_data`, world parsers and writers, medit,
oedit, redit, shop and quest secondaries, stat and diagnostic commands, reload behavior, and OLC
saves. It follows registry typing and gateway extraction safety.

No zone callback exists today, so zone and world lifecycle events are not initial event values.
Potential zone events: zone boot; before and after reset; player enter and leave; periodic pulse;
mobile death; object load or extraction; zone-empty or first-player arrival. Potential world events:
world boot complete; periodic world pulse; day/night/weather/calendar transitions; global encounter
lifecycle; shutdown preparation.

Add only events backed by approved consumers. Start with a direct typed hook at the lifecycle owner,
stating its ordering relative to reset commands and DG Scripts, whether failure can veto the step,
whether re-entry is allowed, and which data stays valid. Generalize to a registry only after a second
consumer proves a shared contract. Do not build a broad asynchronous event bus.

## Test Coverage Requirements

Use the root production-linked CuTest suite for behavior touching real game structures. Phase 00
covers registry identity and validation, accessor bounds, owner-aware OLC, authored round trips,
effective precedence, moving-room rejection, command/pulse/combat characterization, and `-s` mode
(78 tests). Phase 01 adds gateway translation exactness, gateway-local flow, null-safety, secondary
forwarding, and both successor-caching corrections (12 tests). Phase 02 adds declarative-row,
owner/source, table-diagnostic, and source-label coverage (11 tests). Remaining coverage required as
later phases land:

- Exact equipped-object pointer identity with duplicate-VNUM instances.
- Cooldown units, slot bounds, reboot and persistence behavior, and intended spending outcomes.
- Target death, character pending extraction, and immediate object extraction during execution.
- Multi-target iteration safety and recursive extra-attack suppression.
- Affect source removal, source namespace separation, and stacking rejection.
- Multiple-handler ordering if composition is introduced.
- Shop and quest secondary behavior throughout migration.

After root `make test`, always run `make install` so the tested server is installed at `bin/circle`
and no root-level `circle` artifact remains.

## Documentation Deliverables

Update when behavior changes:

- `docs/guides/OLC_SpecProcs.md`.
- The `SPECIALS` help entry, now database-first via `sql/components/help_specproc_entries.sql` with
  `sql/components/verify_help_specproc_entries.sql`; `lib/text/help/help.hlp` is the legacy fallback.
- Architecture and developer documentation for dispatch, registration, and binding.
- System documentation for procedures moved into established subsystems.
- `docs/systems/ARTIFACT_SYSTEM.md` when artifact APIs or persistence responsibilities move.
- `docs/TECHNICAL_DOCUMENTATION_MASTER_INDEX.md` when a new long-lived architecture document is added.

Builder and staff documentation covers canonical identities, aliases, event prerequisites, collision
diagnostics, provenance, unresolved-name behavior, and migration compatibility, labeling implemented
phases as current and future work as proposed.
