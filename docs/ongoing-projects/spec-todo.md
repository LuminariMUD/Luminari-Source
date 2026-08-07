# LuminariMUD - Special Procedure Architecture Refactor PRD

## Overview

The `SPECIAL` callback serves command, pulse, combat, identification, maneuver, moving-room, shop,
and quest paths even though those callers pass different data and interpret return values
differently. This initiative makes special-procedure behavior observable and incrementally typed
while preserving the callback ABI, persisted names, world-file formats, boot precedence, activation
flags, traversal order, and runtime scheduling until a separately tested migration changes them.

Phases 00-06 are complete. The project closed on 2026-08-07 with optional general composition and
zone/world lifecycle events intentionally unimplemented after a concrete-consumer audit.

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
| 03 | Behavior-Preserving Content Extraction | Complete (2026-08-07) |
| 04 | Narrow Shared Mechanics | Complete (2026-08-07) |
| 05 | Incremental Typed Handlers | Complete (2026-08-07) |
| 06 | Conditional Composition and Lifecycle Hooks | Complete (2026-08-07) |

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

### Phase 03 - Behavior-Preserving Content Extraction (Complete)

Checkpoints 1-31 extracted the complete audited general object section to
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
cleric and guard services live in `src/clan_services.c`. The complete King's Castle, Abyss, Crimson
Flame, Prisoner, Celestial Leviathan, Fire Giant, Jot, Mad Drow, TTF, Shadow Dragon, Banshee,
Quicksand, Tower of Kenjin, Hive of Passion, Fey-Branche, Abyssal Vortex, House Agrach-Dyrr, House
Shobalar, Earth Plane, Air Plane, Zusuk, Orc Ruins, Illithid Enclave, Kobold Caverns, Bandit
Castle, Secomber, Longsaddle, Flaming Tower, Mere of Dead Men, Battlemaze, Fire Plane, Water Plane,
Snake Pit, and Menzoberranzan packages now live in dedicated `src/spec/spec_zone_*` owners. The
final TTF move retired all 4,202 baseline lines and removed `src/zone_procs.c` from both build
manifests. The final alarm-group and Menzoberranzan move retired all 12,212 baseline lines and
removed `src/spec_procs.c` from both manifests after auditing its residual commented blocks as
dormant. Autotools and CMake link every new source for production and CuTest. The callback ABI,
exported symbols, registry identities and assignments, world grammar, scheduling, and behavior
remain unchanged.
The cross-file `is_wearing()` equipment predicate now lives with `equip_char()` and
`unequip_char()` in `src/handler.c`. `src/spec_procs.h` remains the compatibility include surface.

1. Extract general object procedures first, after gateway coverage.
2. Extract reusable mobile and room procedures.
3. Split `zone_procs.c` by its existing cohesive zone packages.
4. Move vessel, vendor, crafting, and ability work to its true owners.
5. Preserve exported names, callback ABI, static-state ownership, initialization order, behavior.

Exit when responsibilities are coherent, characterization is unchanged, and both build manifests
list identical production and test sources.

Acceptance evidence:
[Special Procedure Phase 03 Validation](../testing/SPECIAL_PROCEDURE_PHASE_03_VALIDATION.md).

### Phase 04 - Narrow Shared Mechanics (Complete)

Delivered five focused modules under `src/spec/`: typed event, exact-worn-instance, and live combat
context validation (`spec_context`); opt-in exact command/phrase matching that preserves case,
punctuation, tabs, and trailing whitespace (`spec_phrase`); instance-owned legacy `spec_timer[]`
operations with explicit MUD-hour and restart semantics (`spec_cooldown`); a current-target damage
wrapper preserving and classifying `damage()` results (`spec_combat`); and negative namespaced
`source_id` ownership with spell-scoped stacking groups and atomic modifier batches
(`spec_effects`).

Real consumers establish every contract. `stability_boots` and `hellfire` share exact phrase,
exact-instance, and cooldown behavior; `monk_glove` and `monk_glove_cold` share safe damage handling;
`snake` and `wizard` share live combat validation; all gateway events pass typed payload validation;
and artifact passives plus six temporary-power paths use source ownership independently from stacking
identity. Weapon-hit dispatch now carries the actual combat victim instead of reconstructing it from
ambient `FIGHTING()` state. Nine focused production-linked tests cover the helpers and their
representative callbacks without mechanically converting the remaining procedures.

Acceptance evidence:
[Special Procedure Phase 04 Validation](../testing/SPECIAL_PROCEDURE_PHASE_04_VALIDATION.md).

### Phase 05 - Incremental Typed Handlers (Complete)

Delivered a dual-shape registry contract: a definition owns either one complete legacy handler or a
typed handler plus a unique legacy-shaped callback-slot adapter. `spec_dispatch()` now selects typed
dispatch for registered adapters and exact compatibility translation for every other callback.
Typed dispatch validates owner/event support, preserves gateway-local flow separately from pointer
invalidation, and rejects STOP on notification-only events.

Bank and Vampire Cloak are the first production typed handlers. Both identify through explicit
`SPEC_EVENT_ITEM_IDENTIFY` context instead of command zero plus the magic string `identify`.
Vampire Cloak command handling also validates the exact context owner as the cloak worn in
`WEAR_ABOUT`, so a same-VNUM carried copy cannot stand in for the invoking object. Their callback
pointers, canonical persisted names, assignments, OLC rows, accepted commands, output, and legacy
return interpretation remain stable.

The post-conversion inventory contains 196 source-level `SPECIAL` definitions: two safe adapters
and 194 legacy behavior implementations. The canonical registry contains 28 definitions: two typed
and 26 legacy. Compatibility support therefore remains required. Five focused production-linked
tests bring the root suite to 588 tests.

Acceptance evidence:
[Special Procedure Phase 05 Validation](../testing/SPECIAL_PROCEDURE_PHASE_05_VALIDATION.md).

### Phase 06 - Conditional Composition and Lifecycle Hooks (Complete)

The final audit found one concrete composition contract: the existing runtime-only
`questmaster -> shop_keeper -> original callback` nesting. Mobile, object, and room prototypes still
persist at most one authored procedure name, and no approved content needs a second general handler
on one prototype. Shop and quest secondaries therefore remain deliberate compatibility wrappers,
not entries in a new chain. Their save-before-install boot order, secondary-first flow, nonzero stop,
zero fallthrough, and exact context forwarding remain covered by production-linked tests. A new
test proves the Phase 05 typed Bank handler also works through the full quest/shop nesting.

The lifecycle audit likewise found no approved need for a new C-level zone or world procedure hook.
DG Scripts already cover localized room reset, enter, leave, login, time, mobile/object load, death,
timer, and related behavior. Strong artifact and vessel lifecycle requirements call direct APIs at
their owning subsystem. No event catalog, registry, or asynchronous bus was added.

Because no chain or lifecycle persistence changed, there is no format or schema version to bump.
The single-name mobile `SpecProc`, object `Z`, and room `Z` formats remain backward-compatible as-is;
`SHOP_FUNC` and `QST_FUNC` are reconstructed runtime pointers and are not serialized. General
composition or a shared C lifecycle registry may reopen only when approved content supplies the
required consumer and all ordering, lifetime, OLC, and versioning contracts.

Acceptance evidence:
[Special Procedure Phase 06 Validation](../testing/SPECIAL_PROCEDURE_PHASE_06_VALIDATION.md).

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
| Shared helpers state clock, ownership, persistence, stacking, and invalidation rules and have at least two real consumers with tests. | Met by Phase 04. |
| File organization follows primary responsibility, with both build systems synchronized. | Met by Phase 03. |
| Root `make test` and `make install` pass with the server installed at `bin/circle`. | Standing gate; passed at Phase 06 (589 tests). |
| Builder, help, system, and architecture documentation matches every implemented phase. | Standing gate; met through Phase 06. |

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

## Resolved Decisions

1. Current content does not justify a persisted multiple-procedure prototype chain. Preserve the
   explicit quest/shop compatibility nesting and reopen only for an approved additional consumer.
2. No new zone or world special-procedure lifecycle hook is required. Use DG Scripts for localized
   content and direct owning-subsystem hooks for strong engine lifecycle guarantees.
3. Artifact file decomposition is outside this initiative. Artifact identity, custody, progression,
   persistence, and future source maintenance remain owned by the artifact subsystem.

## Current-State Evidence

Baseline analysis was verified on 2026-08-06 against commit `af9f79d2`, covering callback
declarations and owner storage (`src/structs.h`, `src/db.h`, `src/utils.h`); call sites
(`src/interpreter.c`, `src/mob/mob_act.c`, `src/comm.c`, `src/combat/fight.c`,
`src/combat/act.offensive.c`, `src/obj/act.item.c`); registry, assignment, world loading, and OLC
persistence (`src/spec_assign.c`, `src/db.c`, `src/olc/`); shop and quest composition
(`src/obj/shop.c`, `src/quest/quest.c`, `src/olc/genqst.c`); cooldown, affect, damage, and
object-save contracts; artifact code and tests; and persisted bindings under `lib/world/`.

Phases 00-06 have since changed registry, OLC, dispatch, provenance, eligible assignment behavior,
source ownership, shared mechanics, and typed-handler implementation described in that baseline.
Statements below are marked where a completed phase superseded them.
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

At the Phase 03 baseline, `zone_procs.c` held zone-associated mobile and object procedures,
encounter state, and helpers; it was not a zone callback system, and `struct zone_data` has no
special-procedure callback. Checkpoint 13 completed its package split and retired the file. Future
zone events still need an explicit lifecycle interface, not a zone pointer hidden in `void *me`.

At the Phase 03 baseline, `spec_procs.c` also held work owned elsewhere: spell/skill/ability listing
and calculation; moving-room and legacy ship behavior; vendor item construction and naming; and
crafting-mold purchase and construction. Checkpoints 1-31 moved every item in that list, the traced
general mobile/room slice, reusable combat/companion archetypes, clan services, and every cohesive
zone package intact from the legacy files. The final residual audit found only dormant commented
guild and Emporium blocks after the compiled callbacks moved, so `src/spec_procs.c` was retired from
both manifests. Its former cross-file equipment predicate now lives in `src/handler.c`. Moving rooms
retain their temporary gateway and now live in the vessel subsystem; a direct typed hook remains a
later behavior-changing phase.

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

Shipped content ownership (Phase 03):

```text
src/spec/spec_objects.c
src/spec/spec_mobile_archetypes.c|.h
src/spec/spec_mobiles.c|.h
src/spec/spec_rooms.c|.h
src/spec/spec_zone_abyss.c|.h
src/spec/spec_zone_abyssal_vortex.c|.h
src/spec/spec_zone_agrach_dyrr.c|.h
src/spec/spec_zone_air_plane.c|.h
src/spec/spec_zone_alarm_group.c
src/spec/spec_zone_bandit_castle.c|.h
src/spec/spec_zone_banshee.c|.h
src/spec/spec_zone_battlemaze.c|.h
src/spec/spec_zone_celestial_leviathan.c|.h
src/spec/spec_zone_crimson_flame.c|.h
src/spec/spec_zone_earth_plane.c|.h
src/spec/spec_zone_feybranche.c|.h
src/spec/spec_zone_fire_giant.c|.h
src/spec/spec_zone_fire_plane.h
src/spec/spec_zone_flaming_tower.c|.h
src/spec/spec_zone_hive_of_passion.c|.h
src/spec/spec_zone_illithid_enclave.c|.h
src/spec/spec_zone_jot.c|.h
src/spec/spec_zone_kenjin_tower.c|.h
src/spec/spec_zone_kings_castle.c|.h
src/spec/spec_zone_kobold_caverns.c|.h
src/spec/spec_zone_longsaddle.c|.h
src/spec/spec_zone_mad_drow.c|.h
src/spec/spec_zone_menzoberranzan.c|.h
src/spec/spec_zone_mere_of_dead_men.c|.h
src/spec/spec_zone_neverwinter.c
src/spec/spec_zone_orc_ruins.c|.h
src/spec/spec_zone_prisoner.c|.h
src/spec/spec_zone_quicksand.c|.h
src/spec/spec_zone_secomber.c|.h
src/spec/spec_zone_shadow_dragon.c|.h
src/spec/spec_zone_shobalar.c|.h
src/spec/spec_zone_snake_pit.h
src/spec/spec_zone_ttf.c|.h
src/spec/spec_zone_water_plane.h
src/spec/spec_zone_zusuk.c|.h
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

Implemented in Phase 04 after tracing real consumers:

```text
src/spec/spec_combat.c|.h
src/spec/spec_context.c|.h
src/spec/spec_cooldown.c|.h
src/spec/spec_effects.c|.h
src/spec/spec_phrase.c|.h
```

This is a responsibility map, not permission to create empty modules. Top-level `spec_procs.c` is
retired; `spec_procs.h` remains a compatibility include surface and shrinks over time. The private
`zone_yell()` helper and its Fire Plane, Water Plane, and Snake Pit consumers deliberately share
`spec_zone_alarm_group.c`, while each zone publishes its own header. External includes are
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

The shipped `struct spec_event_context` models only current invocation paths. Its semantic fields
remain deliberately narrower than a general event system.

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

The implementation uses `SPEC_OWNER_*`, `SPEC_EVENT_*`, `SPEC_FLOW_CONTINUE`/`_STOP`, and
`SPEC_INVALIDATE_NONE|OWNER|ACTOR|TARGET`. Zone and world lifecycle events are not current event
values.

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
builder visibility, exactly one legacy behavior or typed-adapter/handler pair, and non-empty
description and category. Boot validation runs before world parsing: invalid metadata is a
programmer error and fails boot; an unknown world-data name is a content error whose source location
and raw identity are preserved. Aliases never become canonical through reverse pointer lookup, and
accessors are bounds-safe at both extremes. Builders explicitly replace or clear an unresolved
request; the effective callback stays empty until content resolves it.

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

These contracts were delivered in Phase 04 after each helper had at least two traced consumers.

### Context Validation

Validators reject owner/event mismatches, missing actors or targets, invalid rooms, absent combat
state, missing flags, and unsupported placement.

The former `obj_proc_ready()` matched equipment by VNUM through `is_wearing()` and did not prove the
invoking instance was worn. It now uses `spec_context_validate_worn_object()`, which requires both
`obj->worn_by == actor` and the actor's wear slot to point to that exact object. Character validation
rejects dead or pending-extraction actors and invalid rooms. No validator claims to recognize an
object after it is freed.

### Command and Phrase Matching

`spec_phrase_match()` compares one resolved canonical command and phrase. Leading ASCII spaces are
skipped only when requested; case, punctuation, tabs, and trailing whitespace remain exact. Its
result distinguishes matched, unrelated, and invalid input. `stability_boots` and `hellfire` retain
their characterized accepted input through this opt-in contract. Broader splitting or normalization
remains unimplemented until real consumers require it.

### Cooldowns

Two incompatible models remain:

- Legacy object `spec_timer[]` counters decrement once per `point_update()`, currently once per MUD
  hour (`SECS_PER_MUD_HOUR` is 75 real seconds). They belong to the object instance, are not
  serialized by `src/obj/objsave.c`, and reset on recreation or restart.
- Artifact `time_t` stamps use wall-clock seconds and are persisted and restored by artifact
  persistence.

`spec_object_cooldown_read()` and `spec_object_cooldown_commit()` implement only the first model.
They validate `[0, SPEC_TIMER_MAX)` slots, expose remaining MUD hours, and commit a positive duration
only after the caller's effect succeeds. `stability_boots` and `hellfire` are the initial consumers.
Artifact, event-backed, and character-specific mechanisms remain separate and are not
interchangeable.

### Combat and Target Safety

Verify as applicable: actor and target alive and not pending extraction; both in the same room;
offensive actions pass `aoeOK` or the applicable aggression rule; required combat state exists;
equipped objects are the exact instances worn by the actor; damage results distinguish no damage,
damage, death, and extraction as existing primitives expose them; multi-target loops retain their
next pointer before effects can remove an entry; extra-attack procedures cannot recursively trigger
themselves without a bound.

`spec_context_validate_combat_target()` checks live and pending-extraction state, room validity,
colocation, and optional current-opponent identity. `spec_damage_current_target()` then wraps the
existing `damage()` primitive and classifies invalid input, no effect, applied damage, and possible
target invalidation while preserving the raw return. The two monk-glove procedures consume the
damage result contract; snake and wizard consume combat validation. Weapon-hit dispatch carries the
caller's actual victim into the typed context. No second combat engine was introduced.

### Temporary Affects and Stacking

Build on `affect_to_char_source()`, `affect_from_char_source()`, `affected_by_spell_source()`, and
`affect_join_source()` with `affected_type.source_id`; do not add a second general source field.

`spec_effect_source_id()` assigns stable negative identities as
`-(namespace * 1000000 + owner_key)` for keys 1 through 999999, leaving positive runtime identities
separate. `spec_effect_apply_group()` stores source ownership in `source_id`, the spell-scoped
stacking group in `specific`, and atomically rejects a conflicting group before inserting any of up
to eight validated modifiers. Artifact passives now remove by source, with backward cleanup for the
old persisted tag; six temporary artifact paths use the grouped helper. Artifact XP, progression,
custody, and persistence remain outside the general module.

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
- **Objects**: the former contiguous object-procedure section now lives under its reusable or true
  feature owner. Its `obj_proc_ready()` helper now requires the invoking object instance itself to be
  worn; the generic `is_wearing()` predicate retains same-VNUM semantics for callers that want that
  policy. Artifacts stay under `src/obj/`; vessel objects and
  controls under `src/vessels/`; shop and trade under `src/obj/`; crafting purchase and construction
  under `src/craft/`.
- **Rooms**: general room behavior may share a room-procedure module. Moving rooms and vessel control
  rooms belong to the vessel subsystem.
- **Zones**: the natural packages formerly in `zone_procs.c` and `spec_procs.c` now live intact under
  `src/spec/` with their private helpers and encounter state. Extract shared mechanics only when two
  packages need the same contract.

## Conditional Composition and Lifecycle Hooks

Phase 06 closed without a general prototype chain. Current shop and quest secondaries are explicit
runtime compatibility composition, not persisted entries in a general chain. Boot deliberately
produces quest-over-shop-over-original nesting, and each wrapper invokes its saved secondary before
its own behavior. This remains internal to one mobile and does not change outer command traversal.

If approved content reopens a chain, define and test deterministic order from persisted definition
IDs and explicit policy (never function addresses or registry order); binding source and collision
behavior per entry; per-entry event compatibility; gateway-specific continue/stop semantics; owner,
actor, and target invalidation including stable-snapshot versus live-chain behavior when a handler
mutates bindings during dispatch; whether a command result stops only the inner chain or outer
traversal too; duplicate-handler policy and bounded chain length; a versioned backward-compatible
persistence format for multiple names; OLC display, reorder, clear, unresolved-name, and save
behavior; and deliberate migration of quest-over-shop-over-original nesting.

Composition touches `room_data`, mobile and object `index_data`, world parsers and writers, medit,
oedit, redit, shop and quest secondaries, stat and diagnostic commands, reload behavior, and OLC
saves. It follows registry typing and gateway extraction safety.

No zone callback exists, and Phase 06 added no zone or world event values. The audit found that DG
room/mobile/object triggers already serve localized reset, movement, load, death, login, and time
content, while stateful artifact and vessel lifecycles already call direct owner APIs.

If approved content reopens a C lifecycle hook, start with a direct typed hook at the lifecycle
owner, stating its ordering relative to reset commands and DG Scripts, whether failure can veto the
step, whether re-entry is allowed, and which data stays valid. Generalize to a registry only after a
second consumer proves a shared contract. Do not build a broad asynchronous event bus.

## Test Coverage Requirements

Use the root production-linked CuTest suite for behavior touching real game structures. Phase 00
covers registry identity and validation, accessor bounds, owner-aware OLC, authored round trips,
effective precedence, moving-room rejection, command/pulse/combat characterization, and `-s` mode
(78 tests). Phase 01 adds gateway translation exactness, gateway-local flow, null-safety, secondary
forwarding, and both successor-caching corrections (12 tests). Phase 02 adds declarative-row,
owner/source, table-diagnostic, and source-label coverage (11 tests). Phase 04 adds nine focused tests
for typed context rejection, exact object and combat identity, phrase and cooldown behavior, damage
results, and affect source/stacking separation. Phase 05 adds five tests for mixed dispatch, stable
callback identities, flow/invalidation, explicit identify events, and exact Vampire Cloak ownership.
Phase 06 adds one typed-through-secondary regression, bringing the root suite to 589 tests. Existing
secondary tests retain exact forwarding, stop/fallthrough, nesting, boot-order, and `no_specials`
coverage. No multiple-handler or lifecycle-hook tests were added because neither optional
abstraction was implemented.

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
