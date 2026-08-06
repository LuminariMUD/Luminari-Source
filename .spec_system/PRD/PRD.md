# LuminariMUD - Special Procedure Architecture Refactor PRD

## Overview

LuminariMUD needs a safer architecture for custom mobile, object, room, and cohesive zone behavior
before a substantial new wave of special procedures is added. The current `SPECIAL` callback is
used by command, pulse, combat, identification, maneuver, moving-room, shop, and quest paths even
though those callers provide different data and interpret return values differently.

This initiative gives engine maintainers, builders, content developers, and operators an observable
and incrementally typed special-procedure system. It separates immutable procedure definitions,
authored and effective bindings, and event-specific invocation while preserving the current callback
ABI, persisted names, world-file formats, boot precedence, activation flags, traversal order, and
runtime scheduling until a separately tested migration intentionally changes them.

## Goals

1. Make every current special-procedure invocation path explicit, testable, and safe to migrate.
2. Give every procedure a validated canonical identity, owner compatibility, event contracts,
   prerequisites, binding policy, and builder-visible description.
3. Make authored bindings, effective bindings, precedence, collisions, aliases, and unresolved names
   observable without silent OLC data loss.
4. Reorganize special-procedure content by primary responsibility while preserving behavior and
   keeping cohesive feature and zone packages together.
5. Introduce narrow shared mechanics and typed handlers only after real consumers and compatibility
   tests establish their contracts.
6. Keep builders, operators, developers, helpfiles, and architecture documentation aligned with each
   implemented phase.

## Non-Goals

- Rewrite every legacy `SPECIAL` handler in the first implementation.
- Change the existing mob, object, or room world-file format during the core refactor.
- Add multiple handlers to every prototype before concrete composition use cases exist.
- Create a generic asynchronous event bus or speculative zone and world event catalog.
- Move artifact ownership, custody, progression, or persistence into the general spec subsystem.
- Migrate every hard-coded assignment into world data in one change.
- Change command-owner traversal, heartbeat scheduling, activation flags, or boot precedence as an
  incidental side effect.
- Make C special procedures replace DG Scripts for localized narrative, dialogue, puzzle, or
  sequencing behavior.
- Modify local configuration headers or production configuration as part of this initiative.

## Users and Use Cases

### Primary Users

- **Engine maintainer**: Evolves callback dispatch, registries, bindings, combat safety, and source
  organization without behavior drift.
- **Builder and staff member**: Selects compatible procedures in OLC and diagnoses binding state,
  event support, prerequisites, and collisions.
- **Content developer**: Adds reusable or cohesive custom behavior through the correct subsystem and
  binding source.
- **Server operator and test maintainer**: Validates boot metadata, effective bindings,
  compatibility modes, runtime safety, and production-linked regression coverage.

### Key Use Cases

1. A builder selects a procedure that is valid for the edited owner type and understands when it can
   run.
2. A builder saves an unrelated field without losing an unknown binding name or promoting a
   boot-time override into authored world data.
3. An operator identifies the requested and effective procedure, binding source, and collision
   outcome for any bound prototype.
4. A maintainer characterizes a legacy invocation and then routes it through an event-specific
   compatibility gateway without changing observable behavior.
5. A content developer adds reusable behavior through world data or places subsystem-owned behavior
   with its primary owner instead of expanding a monolithic file.
6. A maintainer converts a legacy handler to a typed context while callers honor event-specific flow
   and pointer invalidation.
7. A test maintainer verifies normal and `-s` modes, extraction behavior, iteration safety, aliases,
   OLC round trips, and boot precedence through production-linked tests.

## Requirements

### MVP Requirements

- Engine maintainer can execute production-linked characterization tests for registry lookup,
  canonical names, aliases, accessor bounds, OLC selection, and known and unknown world bindings.
- Engine maintainer can identify every registered definition by a stable canonical name plus
  explicit display name, aliases, owner mask, event contracts, binding visibility, category, and
  non-empty description.
- Server operator can receive an early boot failure for duplicate, empty, incompatible, or
  handlerless definition metadata before world files are parsed.
- Builder can list only definitions compatible with the edited mobile, object, or room and can view
  supported events and prototype or placement prerequisites.
- Builder can round-trip a known authored binding without an effective hard-coded, shop, or quest
  override changing its persisted identity.
- Builder can encounter an unresolved persisted name without an unrelated OLC save silently erasing
  it.
- Server operator can diagnose unknown or incompatible names with source location, owner type, VNUM,
  and requested identity.
- Server operator can inspect each effective post-boot binding, its provenance, collision outcome,
  and any saved shop or quest secondary callback.
- Room builder can receive an explicit rejection when moving-room `M` data and a registered `Z`
  binding would share the incompatible callback slot.
- Maintainer can preserve the current single-handler storage model, callback ABI, world-file syntax,
  and boot precedence throughout the registry-safety slice.
- Builder and staff member can consult updated OLC guidance and the in-game `SPECIALS` help entry
  for implemented registration, binding, compatibility, and diagnostic behavior.

### Deferred Requirements

- Engine maintainer can route all current command, pulse, combat, identification, maneuver, charge,
  moving-room, shop, and quest invocation paths through event-specific gateways.
- Runtime caller can distinguish gateway-local stop behavior from owner, actor, and target
  invalidation and can avoid unsafe post-callback dereferences.
- Engine maintainer can represent direct legacy owner, VNUM, and definition assignments as
  validated, observable data where setup is not computed or subsystem-specific.
- Content maintainer can migrate a hard-coded binding to world data only after comparing its
  complete effective pre-boot and post-boot behavior.
- Engine maintainer can extract reusable mobile, object, room, and zone content into
  responsibility-owned modules without changing exports, static state, initialization, or callback
  behavior.
- Procedure developer can reuse audited context, phrase, cooldown, target, combat-result, affect,
  and chance contracts when at least two consumers need the same rule.
- Procedure developer can implement or migrate handlers with typed event payloads behind the
  compatibility gateways while retaining stable persisted identities.
- Builder can compose multiple procedures on one prototype only after ordering, duplication,
  invalidation, persistence, shop and quest nesting, and OLC behavior are fully specified and
  tested.
- Content designer can use a typed zone or world lifecycle hook only when an approved concrete
  consumer establishes its ordering, re-entry, failure, and lifetime contract.
- Artifact maintainer can reuse genuinely general mechanics while artifact identity, custody,
  progression, persistence, and recovery remain isolated in the artifact subsystem.

## Non-Functional Requirements

- **Compatibility**: Characterization coverage must include all 12 verified invocation categories,
  exact legacy argument tokens, caller-specific return handling, command-owner order, heartbeat
  positions, activation flags, boot precedence, and normal versus `-s` behavior before their
  contracts change.
- **Registry integrity**: 100 percent of registered definitions must pass pre-world-load validation;
  zero duplicate canonical names or aliases, empty required metadata, invalid masks, or handlerless
  definitions may reach world parsing.
- **Data integrity**: Zero unresolved authored names or binding provenance records may be erased or
  promoted by an unrelated OLC save, as proven by production-linked round-trip tests.
- **Runtime safety**: Every migrated gateway that permits mutation must cache iteration successors
  before invocation and have tests for applicable owner, actor, target, death, and extraction paths;
  no migrated path may probe a freed object after return.
- **Build quality**: Each completed implementation slice must compile with zero new
  `-Wall -Wextra` warnings, pass root `make test`, and pass `make install` with no root-level
  `circle` artifact left.
- **Build parity**: 100 percent of added or removed production and CuTest sources must be reflected
  in both `Makefile.am` and `CMakeLists.txt` in the same change.
- **Documentation quality**: Updated project documentation and helpfiles must be ASCII-only UTF-8
  with LF endings and must distinguish implemented behavior from later proposals.

## Constraints and Dependencies

- The supported Luminari build is the only design target; legacy campaign branches are compatibility
  inventory.
- GNU C23 and the established CircleMUD/tbaMUD architecture, macros, data structures, and callback
  ABI constrain the migration.
- MariaDB/MySQL remains required for the server and the full production-linked test environment.
- Existing stable procedure names and mob, object, and room file formats are content-facing
  compatibility contracts.
- Command traversal order, pulse scheduling, event strings, caller-specific returns, `MOB_SPEC`,
  `ITEM_AUTOPROC`, `no_specials`, shop and quest chaining, and boot assignment precedence are
  observable behavior.
- Moving-room `M` records and room `Z` bindings currently share a callback slot with incompatible
  payloads.
- New feature sources may use one shallow `src/spec/` directory; no second-level source nesting may
  be introduced.
- Every source-file addition or removal must update both supported build manifests.
- Existing symbolic VNUM definitions must be used; local `src/vnums.h`, `src/campaign.h`, and
  `src/mud_options.h` must not be modified.
- Credential-bearing `lib/.env` and `lib/mysql_config` must not be modified; example files are the
  permitted configuration templates.
- Root production-linked CuTest coverage is required for behavior touching real game structures;
  `make test` must be followed by `make install`.
- DG Scripts remain the preferred tool for behavior that does not require engine-level state,
  strong reuse, performance, combat, persistence, or lifecycle guarantees.

## Phases

This system delivers the product via phases. Each phase is implemented via multiple 2-4 hour
sessions with 12-25 tasks each.

| Phase | Name | Sessions | Status |
|-------|------|----------|--------|
| 00 | Registry Safety and Observability | TBD | Not Started |
| 01 | Call-Site Gateway Compatibility | TBD | Not Started |
| 02 | Declarative Legacy Assignments | TBD | Not Started |
| 03 | Behavior-Preserving Content Extraction | TBD | Not Started |
| 04 | Narrow Shared Mechanics | TBD | Not Started |
| 05 | Incremental Typed Handlers | TBD | Not Started |
| 06 | Conditional Composition and Lifecycle Hooks | TBD | Not Started |

## Phase 00: Registry Safety and Observability

### Objectives

1. Freeze registry, alias, bounds, OLC selection, world-binding, and effective boot behavior in
   production-linked tests and diagnostics.
2. Establish immutable, validated definition metadata with canonical identities, explicit aliases,
   type-aware accessors, owner and event compatibility, prerequisites, visibility, descriptions, and
   categories.
3. Filter and explain procedure choices in medit, oedit, and redit without changing the legacy
   callback ABI or single-handler prototype storage.
4. Preserve authored binding provenance and unresolved names, expose collisions and effective
   sources, and reject the unsafe moving-room `M` plus room `Z` combination.
5. Update builder guidance, the `SPECIALS` help entry, and architecture documentation for the
   behavior actually delivered in this phase.

### Sessions (To Be Defined)

Sessions are defined via `phasebuild` as `session_NN_name.md` stubs under
`.spec_system/PRD/phase_00/`.

**Note**: This command does not create phase directories or session stubs. Run `phasebuild` after
creating the PRD.

## Technical Stack

- GNU C23 - Established implementation language and required language level.
- CircleMUD/tbaMUD engine - Existing select-based runtime, world model, and `SPECIAL` compatibility
  surface.
- GNU Autotools and Automake - Preferred incremental build and production-linked test path.
- CMake - Supported secondary build whose source and test manifests must stay synchronized.
- CuTest - Root full-integration suite linked against all game sources with `LUMINARI_CUTEST`.
- MariaDB/MySQL C client - Required persistence dependency for the running server and test setup.
- Flat world files and Oasis OLC - Existing authored binding and builder persistence surfaces.
- DG Scripts - Complementary trigger system retained for localized scripted behavior.

## Success Criteria

- [ ] Every verified invocation category has production-linked characterization coverage before its
  gateway migration changes control flow.
- [ ] Every persisted definition has one canonical identity, explicit aliases, valid owner and event
  metadata, binding visibility, prerequisites, category, and builder-visible description.
- [ ] Unknown or incompatible names are diagnosable and cannot be silently erased by unrelated OLC
  saves.
- [ ] OLC writers preserve authored binding provenance instead of serializing an effective boot-time
  override through reverse function-pointer lookup.
- [ ] Effective bindings and their sources can be inspected for every mob, object, and room
  prototype.
- [ ] Gateway callers honor explicit flow and pointer-lifetime contracts while preserving existing
  scheduling, traversal, activation, and return behavior.
- [ ] Shared helpers state clock, ownership, persistence, stacking, and invalidation rules and have
  at least two real consumers with focused tests.
- [ ] File organization follows primary responsibility and keeps both build systems synchronized.
- [ ] Root `make test` and `make install` pass with the tested server installed at `bin/circle` and
  no root-level `circle` artifact.
- [ ] Builder, help, system, and architecture documentation matches every implemented phase.

## Risks

- **Behavior drift during extraction**: Require independently buildable, behavior-preserving moves
  after characterization coverage exists.
- **Information lost before a legacy handler**: Construct typed event data at each call site and
  translate to legacy arguments only at the compatibility seam.
- **Hidden binding precedence**: Record the complete post-boot chain and preserve current outcomes
  until each collision has an intentional regression-tested migration.
- **Unknown-name data loss**: Retain unresolved authored identity or block implicit OLC overwrite
  until the builder explicitly replaces or clears it.
- **Lifetime violations**: Cache successors and obey event-specific invalidation instead of testing
  objects after callbacks may have freed them.
- **Moving-room payload collision**: Reject combined `M` and `Z` ownership until relocation has a
  separate typed hook.
- **Over-generalized helpers**: Require a semantic contract and at least two consumers before adding
  a shared cooldown, affect, targeting, combat, or chance module.
- **Artifact coupling**: Keep artifact-specific ownership, custody, progression, persistence, and
  recovery outside the general subsystem.
- **New dumping grounds**: Assign files by primary subsystem responsibility and keep cohesive zone
  packages together.
- **Persistence incompatibility**: Preserve stable canonical names and existing single-name formats;
  version any later composition format and keep backward-compatible loading.

## Assumptions

- The consolidated current-state evidence in this PRD is the requirements baseline: it was verified
  on 2026-08-06 against commit `af9f79d2`, supplies implementation evidence and acceptance criteria,
  and is sufficiently complete for phase planning without additional arbitration.
- Phase 00 follows the proposal's explicit Recommended First Implementation Slice: that section
  combines early registry characterization with definition, OLC, provenance, and diagnostics work
  and is the strongest evidence for the first deliverable despite the broader numbered migration
  stages.
- Compatibility is the default until an intentional migration is separately specified and tested:
  the proposal repeatedly identifies current ABI, persistence, ordering, activation, and precedence
  as observable contracts, so planning can proceed on that invariant.
- This is a single-repository initiative: the local detector found no workspace configuration and
  the source describes one C server with shared root build manifests, so package-scoped planning is
  unnecessary.

### Conflict Resolutions

- The numbered migration strategy places declarative assignment work before full content extraction,
  while the recommended delivery narrative calls general object procedures the third slice after
  registry safety and gateways. The phase table preserves the numbered dependency order; the slice
  narrative is retained as a prioritization guardrail that forbids object extraction before gateway
  characterization but does not require it to precede assignment observability work.

## Open Decisions

1. Whether the first binding implementation retains owned unresolved raw names or temporarily blocks
   OLC save until a builder explicitly replaces or clears the unresolved binding.
2. Whether concrete content needs eventually justify multiple procedures on one prototype; Phase 06
   remains conditional until ordering and persistence requirements have a real consumer.
3. Which zone or world lifecycle hooks, if any, approved content needs after direct typed hooks
   prove a shared contract.
4. Whether the independent artifact implementation split should be scheduled alongside or outside
   this core special-procedure initiative.

## Consolidated Current-State Evidence

This section preserves the implementation evidence and architectural contracts behind the product
requirements. Dated counts are inventory snapshots, not architectural invariants. Symbol names,
ordering, persistence behavior, and runtime contracts are authoritative until traced again during
implementation.

### Verification Basis

The current-state analysis was verified on 2026-08-06 against source commit `af9f79d2` across:

- Callback declarations and owner storage in `src/structs.h`, `src/db.h`, and `src/utils.h`.
- Command, pulse, combat, identification, and maneuver call sites in `src/interpreter.c`,
  `src/mob/mob_act.c`, `src/comm.c`, `src/combat/fight.c`,
  `src/combat/act.offensive.c`, and `src/obj/act.item.c`.
- Registry, hard-coded assignment, world loading, boot order, and OLC persistence in
  `src/spec_assign.c`, `src/db.c`, and `src/olc/`.
- Shop and quest composition in `src/obj/shop.c`, `src/quest/quest.c`, and `src/olc/genqst.c`.
- Cooldown, affect-source, damage, and object-save contracts in `src/limits.c`, `src/handler.c`,
  `src/handler.h`, `src/combat/fight.c`, `src/utils.h`, and `src/obj/objsave.c`.
- Artifact mechanics and tests in `src/obj/spec_artifacts.c`, `src/obj/spec_artifacts.h`,
  `unittests/CuTest/test_artifacts.c`, and
  `unittests/CuTest/test_artifact_integration.c`.
- Persisted bindings under `lib/world/mob/`, `lib/world/obj/`, and `lib/world/wld/`, plus source and
  test membership in `Makefile.am` and `CMakeLists.txt`.

### Dated Source Inventory

The four central files contained 24,150 lines in the verified snapshot:

| File | Verified Lines | Current Responsibilities |
|------|---------------:|--------------------------|
| `src/spec_procs.c` | 12,212 | Mixed abilities, entity procedures, features, and helpers |
| `src/obj/spec_artifacts.c` | 6,489 | Artifact systems and test seams |
| `src/zone_procs.c` | 4,178 | Bespoke zone and encounter behavior |
| `src/spec_assign.c` | 1,271 | Assignments, name registry, and accessors |

File size is a review signal, not the root defect. The architectural problem is that unrelated
behaviors share files, one callback ABI, and one registry without declaring their event or owner
contracts.

### Legacy Callback and Invocation Matrix

`SPECIAL_DECL` has this effective ABI:

```c
int handler(struct char_data *ch, void *me, int cmd, const char *argument);
```

The same ABI currently serves 12 distinct invocation categories:

| Invocation | Current Caller | Legacy Signal | Current Return Meaning |
|------------|----------------|---------------|------------------------|
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

The defense tokens are `"shieldblock"`, `"parry"`, `"glance"`, and `"dodge"`. The shield maneuver
tokens are `"shieldpunch"`, `"shieldcharge"`, and `"shieldslam"`.

### Observable Scheduling and Traversal

- The heartbeat invokes moving-room relocation every ten seconds.
- On `PULSE_MOBILE`, `mobile_activity()` runs before `proc_update()`.
- A mobile combat callback runs after that combatant's normal attacks and cleave handling.
- Command dispatch order is room, equipped objects in wear-slot order, carried objects, mobiles in
  room-list order, then room contents.
- The first nonzero command result stops later owner traversal.
- The object auto-proc path may invoke once with a null `ch`, then invoke with `carried_by` when the
  first result is zero.
- The moving-room path selects a room function but passes `struct moving_room_data *`, not a room,
  through `me`.

These positions, translations, and stop rules must remain unchanged until an intentional change has
its own specification and regression tests.

### Lost Context and Return Semantics

Most internal events use `cmd == 0`; only the argument string, call site, and ambient state identify
the event. A weapon-hit callback receives neither the actual target nor damage and commonly infers
the target from `FIGHTING(ch)`. A handler-side wrapper therefore cannot reconstruct data already
discarded by the caller; typed context must be built at each call site.

The legacy integer return is not a universal contract. It can mean command consumed, default mobile
activity skipped, carried auto-proc fallback skipped, or nothing when the caller discards it. Typed
flow must remain gateway-specific.

Command and auto-proc traversals do not consistently cache successors before a callback. A handler
that extracts its owner and returns zero can leave the caller following freed storage. Gateways must
cache live-list successors before invocation and honor explicit invalidation. Character pending-
extraction flags may be inspected where available, but no safe post-free object probe exists.

### Single-Handler Storage and Compatibility Composition

`room_data` and mobile and object `index_data` each store one special-procedure function pointer.
Shops and quests compose behavior by saving an existing callback as a secondary, then invoking it
from `shop_keeper` or `questmaster`. Current boot order can produce
quest-over-shop-over-original nesting.

This limitation applies within one prototype. Existing command dispatch already traverses several
owners in the room, and future per-owner composition must not alter that outer traversal.

The room function pointer also carries moving-room lifecycle behavior. Parsing an `M` record installs
`moving_rooms`; a later `Z` record writes the same slot. The OLC writer emits `M` data before the
registered `Z` name, so a combined record can reload a `Z` procedure where
`moving_rooms_update()` later passes moving-room state. Composition cannot make these incompatible
payloads safe. Loading and OLC must reject the combination until relocation has a separate hook.
The verified world snapshot contained no numeric moving-room `M` records, so this is a latent
format/runtime collision rather than a current migration inventory.

### Activation Flags and `no_specials`

- Mobile command dispatch uses a non-null procedure pointer without checking `MOB_SPEC`.
- Mobile activity and combat-turn dispatch require both `MOB_SPEC` and a non-null pointer.
- `mobile_activity()` removes `MOB_SPEC` when the flag exists but its pointer is null.
- Object command, identification, and combat callers use the procedure pointer directly.
- Periodic `proc_update()` additionally requires `ITEM_AUTOPROC`.
- Selecting a procedure in OLC does not automatically establish the required flags or placement.

The `-s` and `no_specials` behavior is not a global dispatch gate. It skips hard-coded assignment and
shop loading, bypasses command special dispatch, and suppresses mobile activity. World-file names
still resolve during loading, while direct identification, auto-proc, moving-room, and combat call
sites do not check `no_specials`. Tests and diagnostics must cover this per-call-site behavior in
both normal and `-s` modes.

### Registry and Persisted Binding Snapshot

The verified `spec_func_list[]` exposed 29 persisted names backed by 28 distinct handlers.
`Guildmaster` and `Guild` both resolved to `guild`. All description fields were empty, with no
description accessor or OLC presentation path. The same untyped list was shown by medit, oedit, and
redit, allowing incompatible owner selections.

Only five named world bindings existed in the verified snapshot: one mobile `SpecProc` field, two
object `Z` bindings, and two room `Z` bindings.

| Owner | Persisted Name | Source Files |
|-------|----------------|--------------|
| Mobile | `Postmaster` | `lib/world/mob/12.mob` |
| Object | `Greyhawk Ship` | `lib/world/obj/14.obj`, `lib/world/obj/700.obj` |
| Room | `Greyhawk Ship Commands` | `lib/world/wld/14.wld`, `lib/world/wld/700.wld` |

This inventory must be regenerated during implementation. World-file binding exists but is not yet
the dominant source; the registry represents only a subset of hard-coded procedures in
`spec_procs.c` and `zone_procs.c`.

Lookup is case-insensitive. Writers recover names by reverse function-pointer lookup, so the first
matching row becomes the implicit canonical identity. In the current table that makes `Guild` the
compatibility canonical output and `Guildmaster` the accepted alias, subject to a content audit.

An unknown persisted name currently resolves to `NULL` without a diagnostic, and its raw text is
discarded. An OLC save can then erase it. Writers also serialize the effective callback rather than
the authored name and source, so boot-time overrides can be promoted into world data, unregistered
handlers can disappear, and a preceding world name can be lost. Binding provenance and unresolved
raw identity are therefore data-integrity requirements, not optional diagnostics.

### Registration, Assignment, and Boot Precedence

Registration makes a stable procedure identity resolvable by world files and OLC. Assignment binds
a procedure to a hard-coded prototype VNUM. New reusable content should normally use a registered
world-data binding; hard-coded assignment remains a compatibility layer until audited.

The existing assignment entry points are `ASSIGNMOB`, `ASSIGNOBJ`, and `ASSIGNROOM`. Registration
and these assignment operations must not remain conflated in one responsibility.

With specials enabled, preserve this effective boot precedence:

1. `boot_world()` loads persisted mobile, object, and room names into callbacks.
2. `assign_mobiles()` overwrites matching mobile callbacks.
3. `assign_the_shopkeepers()` saves the current mobile callback as a secondary and installs
   `shop_keeper`.
4. `assign_objects()` and `assign_rooms()` overwrite matching object and room callbacks.
5. `assign_the_quests()` saves the current quest-master callback as a secondary and installs
   `questmaster`.

Inventory and test the effective result after the complete sequence, including saved secondaries.
Do not infer behavior from an isolated `ASSIGN*` call. Collisions must initially preserve the
verified outcome while reporting every contributing source.

### Existing File Ownership Problems

`zone_procs.c` contains zone-associated mobile and object procedures, encounter state, and helpers;
it is not a zone callback system. `struct zone_data` has no special-procedure callback. Future zone
events need an explicit lifecycle interface rather than a zone pointer hidden in `void *me`.

`spec_procs.c` also contains work whose primary responsibility belongs elsewhere, including spell,
skill, and ability listing and calculation; moving-room and legacy ship behavior; vendor item
construction and naming; and crafting-mold purchase and construction. A split by owner type alone
into names such as `mob_specs.c`, `obj_specs.c`, and `room_specs.c` would preserve these ownership
mistakes. The moving-room path should receive a temporary gateway, then move to a direct typed hook
in the moving-room or vessel subsystem.

## Normative Design Principles

1. Preserve observable behavior before redesigning dispatch.
2. Keep immutable definitions, authored/effective bindings, and invocation gateways separate.
3. Construct typed context where complete event data still exists.
4. Treat context pointers as borrowed for one synchronous invocation and report invalidation
   separately from control flow.
5. Create narrow semantic helpers that own game rules, not wrappers that merely rename primitives.
6. Group reusable behavior by owner type, but keep cohesive feature and zone content together.
7. Prefer world-data binding for new reusable content while preserving legacy precedence during
   migration.
8. Keep DG Scripts in the design decision for narrative and localized behavior.

C special procedures are preferred only when behavior needs engine-level hooks, broad consistent
reuse, performance-sensitive execution, strong combat or persistence guarantees, or lifecycle
contracts that scripts cannot safely provide.

For example, eligibility or safe offensive-target resolution can be semantic helpers; wrappers that
only rename `damage()` or `send_to_char()` without adding a contract are not.

## Target Architecture Contracts

### Responsibility Boundaries

The subsystem has five distinct responsibilities:

1. Event-specific invocation gateways plus exact legacy `SPECIAL` translation.
2. A typed and validated immutable definition registry with stable persisted identities.
3. A binding layer shared by boot code, world data, OLC, legacy assignments, parser hooks, shops,
   and quests.
4. Narrow reusable mechanics for audited eligibility, parsing, cooldown, targeting, combat, affect,
   and chance rules.
5. Authored behavior organized by reusable owner type or cohesive feature and zone ownership.

Do not create a general `spec_utils.c`. Do not make the artifact subsystem the parent of general
object procedures. The definition registry must not mutate prototypes, and runtime gateways must not
decide where a procedure is bound.

### Candidate Shallow Layout

Exact filenames may change during session planning, but responsibility boundaries must remain:

```text
src/spec_procs.c
src/spec_procs.h

src/spec/
  spec_dispatch.c
  spec_dispatch.h
  spec_registry.c
  spec_registry.h
  spec_binding.c
  spec_binding.h
  spec_context.c
  spec_context.h
  spec_cooldown.c
  spec_cooldown.h
  spec_effects.c
  spec_effects.h
  spec_mobiles.c
  spec_objects.c
  spec_rooms.c
  spec_zone_kings_castle.c
  spec_zone_abyss.c
  spec_zone_crimson_flame.c
  spec_zone_prisoner.c
  spec_zone_celestial_leviathan.c
  spec_zone_fire_giant.c
  spec_zone_jot.c
  spec_zone_mad_drow.c
  spec_zone_ttf.c
```

This is a responsibility map, not permission to create empty modules. `spec_cooldown.c` and
`spec_effects.c` require at least two real consumers with shared contracts. Existing top-level
`spec_procs.c` and `spec_procs.h` remain compatibility surfaces and shrink over time. External
includes use path-qualified names such as `#include "spec/spec_registry.h"`. Every source addition
or removal updates both build manifests.

### Dependency and Control Flow

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

The first typed API models only current invocation paths. Its exact C representation remains a
session-level design choice, but it must preserve these concepts:

The non-binding reference design used `SPEC_OWNER_MOBILE`, `SPEC_OWNER_OBJECT`, and
`SPEC_OWNER_ROOM`; events `SPEC_EVENT_COMMAND`, `SPEC_EVENT_MOBILE_ACTIVITY`,
`SPEC_EVENT_MOBILE_COMBAT_TURN`, `SPEC_EVENT_OBJECT_AUTO_PULSE`, `SPEC_EVENT_ITEM_IDENTIFY`,
`SPEC_EVENT_WEAPON_HIT`, `SPEC_EVENT_DEFENSE_REACTION`, `SPEC_EVENT_COMBAT_MANEUVER`,
`SPEC_EVENT_MOUNT_CHARGE`, and `SPEC_EVENT_MOVING_ROOM_RELOCATION`; flow values
`SPEC_FLOW_CONTINUE` and `SPEC_FLOW_STOP`; and independent flags `SPEC_INVALIDATE_NONE`,
`SPEC_INVALIDATE_OWNER`, `SPEC_INVALIDATE_ACTOR`, and `SPEC_INVALIDATE_TARGET`. These names may
change during implementation, but their semantic separation may not.

| Concept | Required Values or Fields |
|---------|---------------------------|
| Owner type | Mobile, object, room |
| Event | Command, mobile activity, mobile combat turn, object auto-pulse, item identify, weapon hit, defense reaction, combat maneuver, mount charge, moving-room relocation |
| Owner | Typed mobile, object, or room pointer matching owner type |
| Actor | Current character when the event supplies one; otherwise explicitly null |
| Command payload | Command identifier and argument |
| Weapon-hit payload | Actual target, damage, and critical state |
| Defense payload | Actual target and reaction identity |
| Maneuver payload | Actual target and maneuver identity |
| Moving-room payload | Moving-room state and destination room |
| Outcome flow | Continue or gateway-local stop |
| Outcome invalidation | Independent owner, actor, and target flags |

Event-specific payload structures may replace a union when they make call sites clearer. Zone and
world lifecycle events are not initial runtime event values and require separate concrete consumers.

### Gateway Flow and Invalidation

`STOP` never means one universal game action:

| Gateway | Stop Contract |
|---------|---------------|
| Command | Consume the command and stop later owner traversal |
| Mobile activity | Skip remaining default activity for this mobile |
| Object auto-pulse | Skip the carried-object fallback invocation |
| Typed combat action | Abort only the surrounding action named by that gateway |
| Notification-only compatibility event | Invalid; log a contract error and continue safely |

Invalidation is independent of flow. A handler can invalidate an owner or target without consuming a
command. Callers cache successors before invocation and do not inspect potentially freed objects.
Legacy handlers continue to receive the exact current `ch`, `me`, `cmd`, argument, and caller-
specific return interpretation until individually converted and tested.

Initial gateway coverage includes command owner traversal; mobile activity and combat turns; object
auto-proc worn/carried fallback; item identification; weapon hits and defensive reactions; shield
maneuvers and mounted charge; moving-room relocation; and shop and quest secondary forwarding.

### Definition Registry Contract

Each immutable definition records:

- `canonical_name` as persisted identity and a separate `display_name`.
- Explicit `aliases` and `alias_count`.
- Compatible `owner_mask`.
- Per-event contracts with `required_prototype_flags` and `required_placement`.
- Allowed `binding_source_mask` and builder visibility.
- Exactly one `legacy_handler` or typed `spec_handler`.
- Non-empty `description` and `category`.

Registry validation before world parsing guarantees:

- Canonical names and aliases are non-empty and case-insensitively unique.
- Aliases never become canonical through accidental reverse pointer lookup.
- Shared implementation code does not erase distinct definition identity.
- Empty descriptions, invalid masks or events, and handlerless definitions fail boot.
- OLC shows only owner-compatible definitions explicitly allowed for builder/world binding.
- OLC presents descriptions, events, and prerequisites such as `MOB_SPEC`, `ITEM_AUTOPROC`,
  equipped placement, and combat state.
- Wrong-owner loads report the persisted name, owner type, and VNUM.
- Unknown names retain raw identity for diagnostics and round-trip safety.
- Every accessor is bounds-safe, including both extreme ends.

Invalid definition metadata is a programmer error and fails boot. An unknown world-data name is a
content error: report source location and preserve it. Until owned raw-name storage exists, OLC must
refuse an implicit overwrite unless the builder explicitly replaces or clears the field.

### Binding Record Contract

Each binding record contains:

- Owner type and prototype identity.
- Requested persisted name, including owned unresolved raw text.
- Resolved definition when known.
- Source: named world data, system parser hook, legacy assignment, shop wrapper, or quest wrapper.
- Effective result after precedence and collision handling.
- Saved secondary callback information for shop and quest composition.

During compatibility migration the existing prototype function pointer remains the effective slot.
Binding metadata travels with the prototype or OLC copy rather than only in a global mutable-rnum
array. Known strings may be interned; unresolved names require explicit copy and free ownership.
Writers consult authored binding state first and reverse pointer lookup only as a legacy fallback.

A world binding overwritten by a legacy assignment produces one structured diagnostic containing
both sources and the chosen result. Shop and quest secondaries remain traceable. Each policy change
requires a deliberate migration and regression test.

### Declarative Assignment Contract

Direct hard-coded compatibility assignments may become validated data containing owner type, a typed
VNUM, and canonical definition name. Prefer separate owner-typed tables or a tagged VNUM union so a
room constant cannot enter a mobile row implicitly. Every row requires a registered definition, but
metadata must not make hard-coded-only or zone-private behavior builder-selectable.

The non-binding reference shape named this record `struct legacy_spec_assignment` with
`owner_type`, `vnum`, and `spec_name` fields. Production design should replace its compact `int vnum`
with owner-specific `mob_vnum`, `obj_vnum`, and `room_vnum` representations or a tagged union.

Use traced symbolic VNUM constants, never numeric literals. Computed assignments and special setup
remain with their owning systems. If no symbolic constant exists, defer conversion or separately
change `src/vnums.example.h`; never edit local `src/vnums.h`. Audit complete effective post-boot
bindings before deciding which hard-coded entries move to world data.

## Reusable Mechanics Contracts

### Context Validation

Potential validators cover mobile commands, mobile activity and combat turns, object auto-pulses,
equipped-object commands, equipped-object weapon and defense events, and room commands. They reject
owner/event mismatches, missing actors or targets, invalid rooms, absent combat state, missing flags,
and unsupported placement.

The existing `obj_proc_ready()` matches equipment by VNUM through `is_wearing()` and does not prove
that the invoking instance is worn. General validation must use pointer identity such as
`obj->worn_by == actor`. No validator may claim to recognize an object after it has been freed.

### Command and Phrase Matching

Shared parsing may cover exact command matching, argument splitting, case and trailing-punctuation
normalization, target-bearing prefix phrases, and handled-versus-unrelated input. The artifact
called-effect dispatcher is precedent for one data row owning phrase, channel, target rule,
recharge, description, and effect dispatch.

Normalization is opt-in. Every migrated speech or command procedure characterizes its current
abbreviation, punctuation, and case behavior before accepting a shared matcher.

### Cooldowns

At least two incompatible models exist today:

- Legacy object `spec_timer[]` counters decrement once per `point_update()`, currently once per MUD
  hour. `SECS_PER_MUD_HOUR` is currently 75 real seconds. Timers belong to the object instance, are
  not serialized by `src/obj/objsave.c`, and reset when the object is recreated or the server
  restarts.
- Artifact `time_t` timestamps use wall-clock seconds. Ability, generic-proc, signature-proc, and
  called-effect recharge stamps are persisted and restored by artifact persistence.

Event-backed and character-specific cooldown mechanisms also exist and are not interchangeable.
Every shared cooldown contract names its clock and units, storage owner and slot bounds, persistence
and reboot behavior, commit point, and remaining-time display. Validation and successful execution
normally precede spending; target failure, stacking rejection, immunity, or another no-effect result
does not spend cooldown unless the behavior explicitly requires it.

### Combat and Target Safety

Shared contracts must verify as applicable:

- Actor and target are alive and not pending extraction.
- Actor and target remain in the same room.
- Offensive actions pass `aoeOK` or the applicable aggression rule.
- Required combat state exists.
- Equipped objects are the exact instances worn by the actor.
- Damage results identify no damage, damage, death, or extraction as exposed by existing primitives.
- Multi-target loops retain their next pointer before effects can remove an entry.
- Extra-attack procedures cannot recursively trigger themselves without a bound.

Helpers wrap existing combat primitives rather than create a second combat engine. In particular,
preserve the result of `damage()` and name the exact caller action that must stop.

### Temporary Affects and Stacking

Build on `affect_to_char_source()`, `affect_from_char_source()`,
`affected_by_spell_source()`, and `affect_join_source()` with
`affected_type.source_id`; do not add a second general source field.

Artifact code uses `affected_type.specific` for two different concepts: a registry-derived tag on
passive or permanent artifact affects and a stacking group on temporary surge affects. A general
contract must keep source ownership and stacking group separate and provide:

- A namespaced source identity with documented runtime or persistence lifetime.
- An explicit stacking group whose namespace and range are coordinated with spell and artifact use.
- Bonus type, location, modifier, duration, and flags.
- Removal by source without stripping unrelated effects.
- An explicit result when another effect already occupies the stacking group.

Artifact XP and progression remain outside this helper.

### Chance and Proc Policy

Reusable policy may support validated percentage rolls, clearly named one-in-N rolls, optional
bad-luck protection, independent versus shared cooldowns, and deterministic test injection. The
chosen policy must be visible in data or function names so generic, signature, and nested extra
attacks cannot interact invisibly.

## Artifact Boundary and Reuse

### Patterns Available for Generalization

- Registry membership as source of truth.
- Table-driven templates and called-effect contracts.
- Central target resolution before effect dispatch.
- One channel-aware matcher for phrase, target rule, recharge, description, and effect.
- Reusable procedure shapes selected by data.
- Explicit persisted cooldown stamps and recharge queries.
- Existing affect-source ownership APIs and explicit stacking groups.
- Boot-time metadata validation.
- Deterministic random-combat test seams.
- Validation first, execution second, and cooldown spending last.
- Explicit core hooks such as `artifact_weapon_proc()` that receive attacker, target, weapon,
  damage, and critical state and return a lethal result consumed by the caller.

### Artifact-Specific Responsibilities

The following remain artifact-specific: unique-instance enforcement; owner and account binding;
custody and provenance history; XP and levels; artifact save format and dirty-state persistence;
class rejection and burn behavior; chronicle and recovery policy; and artifact-specific effect text
and content contracts.

The general subsystem depends on neither `artifact_data` nor artifact VNUMs. Artifact code may call
general helpers, but general helpers do not call artifact progression or custody.

### Independent Artifact File Split

If separately scheduled, a responsibility-based split under `src/obj/` may use:

```text
src/obj/artifact_registry.c
src/obj/artifact_persistence.c
src/obj/artifact_ownership.c
src/obj/artifact_effects.c
src/obj/artifact_combat.c
src/obj/artifact_commands.c
```

The split retains one public artifact API, uses one internal header for private declarations, keeps
`unittests/CuTest/test_artifacts.c` and `unittests/CuTest/test_artifact_integration.c`
production-linked, and updates both build manifests for every new source.

## Content Ownership Map

### Reusable Mobile Procedures

Guards, pets, janitors, practice targets, common undead behavior, and service NPCs are candidates for
`spec_mobiles.c` or their true owning subsystem. Shop and quest entry points remain with those
systems even though mobiles own them.

### Reusable Object Procedures

The contiguous object-procedure section of `src/spec_procs.c` is the first extraction candidate
after gateway coverage. It already has the partial `obj_proc_ready()` helper and repeats cooldown,
combat, chance, affect, and messaging patterns.

Artifacts remain under `src/obj/`; vessel objects and controls under `src/vessels/`; shop and trade
mechanics under `src/obj/`; and crafting-specific purchasing and construction under `src/craft/`.

### Reusable Room Procedures

General room behavior may use a shared room-procedure module. Moving rooms and vessel control rooms
belong to the vessel subsystem because vessel operation is their primary responsibility.

### Cohesive Zone Packages

Natural packages in the current `zone_procs.c` are King's Castle, Abyss, Crimson Flame, The
Prisoner, Celestial Leviathan, Fire Giant, Jot, Mad Drow, and Temple of Twisted Flesh. Keep private
helpers and encounter state file-local. Extract shared mechanics only after at least two packages
need the same contract.

## Conditional Multiple-Procedure Composition

A future chain is internal to one prototype and must not change outer command traversal across room,
equipment, inventory, mobiles, and room contents. Current shop and quest secondaries are explicit
compatibility composition, not a general chain.

Before a chain can ship, define and test:

- Deterministic order from persisted definition IDs and explicit policy, never function addresses
  or accidental registry order.
- Binding source and collision behavior for every entry.
- Per-entry event compatibility.
- Gateway-specific continue and stop semantics.
- Owner, actor, and target invalidation, including stable-snapshot versus live-chain behavior when a
  handler mutates bindings during dispatch.
- Whether a command result stops only the inner chain or also outer owner traversal.
- Duplicate-handler policy and a bounded chain length.
- A versioned, backward-compatible persistence format for multiple canonical names.
- OLC display, reorder, clear, unresolved-name, and save behavior.
- Preservation and deliberate migration of quest-over-shop-over-original nesting.

Composition affects `room_data`, mobile and object `index_data`, world parsers and writers, medit,
oedit, redit, shop and quest secondaries, stat and diagnostic commands, reload behavior, and OLC
saves. It follows registry typing and gateway extraction safety and requires production-linked tests
for shop/quest nesting, extraction during dispatch, and old single-name loading.

## Conditional Zone and World Lifecycle Hooks

No zone callback exists today, so zone and world lifecycle events are not initial `spec_event`
values. Potential zone events are zone boot; before and after reset; player enter and leave;
periodic pulse; mobile death; object load or extraction; and zone-empty or first-player arrival.

Potential world events are world boot complete; periodic world pulse; day, night, weather, or
calendar transitions; global encounter or event lifecycle; and shutdown preparation.

Add only events backed by approved consumers. Start with a direct typed hook at the lifecycle owner.
Its contract states ordering relative to reset commands and DG Scripts, whether failure can veto the
step, whether re-entry is allowed, and which data remains valid afterward. Generalize to a registry
only after a second consumer proves a shared contract. Do not create a broad asynchronous event bus.

## Detailed Phase Contracts

### Phase 00 - Registry Safety and Observability

Phase 00 consolidates the original freeze-behavior and definition/binding-control-plane stages into
the recommended first delivery.

Deliverables:

1. Characterize every verified invocation path, exact legacy token, null-actor auto-proc behavior,
   return interpretation, scheduling position, and command-owner order.
2. Test case-insensitive registry lookup, unknown names, both accessor bounds, and the
   `Guild`/`Guildmaster` alias.
3. Inventory effective post-boot binding sources and shop and quest secondaries.
4. Record owner type, event paths, flags, and placement for every registered definition.
5. Separate canonical name, display name, explicit aliases, owner mask, events, visibility,
   description, and category in the registry.
6. Validate metadata before world parsing and make accessors type-aware and bounds-safe.
7. Preserve unresolved names or block implicit save, preserve authored provenance, and report
   unknown, incompatible, and collided bindings with owner and VNUM.
8. Filter medit, oedit, and redit by owner and show descriptions and prerequisites.
9. Reject the incompatible moving-room `M` plus room `Z` combination.
10. Add startup diagnostics for effective source and collision outcomes without changing precedence.
11. Update OLC guidance and the `SPECIALS` help entry.

Phase 00 does not move handlers, alter dispatch, set activation flags automatically, change world
syntax, add chains, or migrate bindings. Exit when current behavior is executable as tests and
diagnostics, builders cannot silently choose incompatible definitions or erase unresolved names,
and startup explains every effective binding while legacy handlers still use `SPECIAL_DECL`.

### Phase 01 - Call-Site Gateway Compatibility

1. Define contexts, gateway-local flow, and independent invalidation for every current event.
2. Route command, pulse, identification, combat, maneuver, charge, moving-room, shop, and quest
   callers through gateways without converting handlers.
3. Translate exactly to current `ch`, `me`, `cmd`, argument, and return behavior.
4. Cache iteration state before callbacks and remove unsafe post-call dereferences where extraction
   is an established contract.

Exit when characterized non-extraction behavior is unchanged and complete event data reaches one
testable seam at every caller. Document and test every intentional extraction-safety correction.

### Phase 02 - Declarative Legacy Assignments

1. Convert repetitive direct owner, VNUM, and name assignments to validated typed tables.
2. Use traced symbolic VNUM constants and leave unsupported numeric rows unconverted.
3. Keep computed assignments and special setup with their owners.
4. Preserve and diagnose named-world, parser-hook, hard-coded, shop, and quest precedence.
5. Move a binding to world data only after comparing complete effective behavior.

Exit when compatibility assignments and collisions are traceable and tested without flattening an
intentional shop or quest chain.

### Phase 03 - Behavior-Preserving Content Extraction

1. Extract general object procedures first, after gateway coverage.
2. Extract reusable mobile and room procedures.
3. Split `zone_procs.c` by existing cohesive zone packages, including Celestial Leviathan.
4. Move vessels, vendors, crafting, abilities, and unrelated work to true owners.
5. Preserve exported names, callback ABI, static-state ownership, initialization order, and behavior.

Exit when responsibilities are coherent, characterization remains unchanged, and both build
manifests contain identical production and test source membership.

### Phase 04 - Narrow Shared Mechanics

1. Add pointer-identity context validation for representative object and mobile procedures.
2. Add opt-in phrase or command parsing only after accepted-input characterization.
3. Add cooldown operations with explicit clock, units, storage, bounds, persistence, and commit
   rules.
4. Add safe target and combat-result contracts around existing primitives.
5. Build affect helpers on `source_id` while keeping stacking identity separate.
6. Migrate a small representative set before broader adoption.

Exit when every helper names a documented rule, has at least two consumers, and has focused tests.
Do not mechanically convert every procedure.

### Phase 05 - Incremental Typed Handlers

1. Implement new procedures as typed handlers behind existing gateways.
2. Convert a legacy procedure when it is otherwise changing or typed conversion removes a proven
   safety risk.
3. Preserve canonical persisted identities and compare behavior against characterization tests.
4. Track the remaining legacy population before considering removal of compatibility support.

Exit when converted handlers no longer infer event data from magic strings or ambient combat state
and callers honor their flow and invalidation outcomes.

### Phase 06 - Conditional Composition and Lifecycle Hooks

1. Gather concrete prototype composition and lifecycle use cases.
2. Design and test inner-chain order without changing outer command-owner traversal.
3. Migrate shop and quest secondaries deliberately.
4. Add only zone and world hooks required by approved content.
5. Version affected persistence and retain backward-compatible loading.

Exit when each abstraction has a real consumer and complete ordering, lifetime, and compatibility
coverage. This phase may close with composition or lifecycle events intentionally unimplemented when
no consumer justifies them.

### Delivery Sequencing Guardrail

The first implementation slice is Phase 00 registry safety and observability. The second introduces
gateways across every current invocation path. General object extraction occurs only after gateway
and characterization coverage. Eligibility, cooldown, and other helpers emerge from audited
consumers rather than being bundled into the file move.

## Required Test Coverage Matrix

Use the root production-linked CuTest suite for behavior interacting with real game sources and
structures. Required coverage includes:

- Canonical and alias uniqueness, case-insensitive lookup, and extreme accessor bounds.
- Definition validation before world parsing.
- Owner, event, flag, and placement compatibility in medit, oedit, and redit.
- Builder-visible descriptions and known and unresolved OLC round trips.
- OLC save after a hard-coded override without promoting or erasing authored provenance.
- Legacy world-file loading and canonical save behavior.
- Effective precedence across world, parser-hook, hard-coded, shop, and quest sources.
- Exact translation for every current magic string and empty-argument invocation.
- Room, equipped, carried, mobile, and room-object command traversal order.
- `MOB_SPEC` and `ITEM_AUTOPROC` activation behavior.
- Normal and `-s` loading, assignment, and per-call-site suppression behavior.
- Worn-then-carried auto-proc fallback with null actor and return variations.
- Notification-only legacy calls whose return value is intentionally ignored.
- Moving-room null actor and argument translation plus incompatible `M` and `Z` rejection.
- Exact equipped-object pointer identity with duplicate-VNUM instances.
- Cooldown units, slot bounds, reboot and persistence behavior, and intended spending outcomes.
- Target death, character pending extraction, and immediate object extraction during execution.
- Multi-target iteration safety.
- Recursive extra-attack suppression.
- Affect source removal, source namespace separation, and stacking rejection.
- Multiple-handler ordering if composition is introduced.
- Shop and quest secondary behavior throughout migration.

After root `make test`, always run `make install` so the tested server is installed at `bin/circle`
and no root-level `circle` artifact remains.

## Required Documentation Deliverables

Implementation updates the following when its behavior changes:

- `docs/guides/OLC_SpecProcs.md`.
- The `SPECIALS` entry in `lib/text/help/help.hlp`, which currently describes hard-coded assignment
  as the only path.
- Architecture and developer documentation for dispatch, registration, and binding.
- System documentation for procedures moved into established subsystems.
- `docs/systems/ARTIFACT_SYSTEM.md` when artifact APIs or persistence responsibilities move.
- `docs/TECHNICAL_DOCUMENTATION_MASTER_INDEX.md` when a new long-lived architecture document is
  added.

Builder and staff documentation covers canonical identities, aliases, event prerequisites,
collision diagnostics, provenance, unresolved-name behavior, and migration compatibility. It labels
implemented phases as current and future work as proposed.

## Detailed Risk Guardrails

- **Behavior drift during moves**: Preserve includes, declarations, static state, initialization,
  callback order, and buildability in independently testable changes.
- **Handler-side information loss**: Never claim typed target, damage, critical, or lifecycle safety
  from a wrapper placed after callers discarded that data.
- **Hidden precedence**: Preserve complete effective boot chains until each collision is deliberately
  migrated.
- **Independent activation flags**: Expose `MOB_SPEC`, `ITEM_AUTOPROC`, combat, and placement
  prerequisites; automatic flag mutation is a separate content migration.
- **Moving-room slot collision**: Reject incompatible `M` and `Z` use until relocation has its own
  typed hook.
- **Unknown-name loss**: A log is insufficient; preserve raw identity or prevent implicit overwrite.
- **Lifetime violations**: Cache iteration state and follow invalidation without probing freed
  objects.
- **Artifact over-generalization**: Extract only contracts that do not depend on artifact ownership,
  levels, custody, or persistence.
- **New dumping grounds**: Follow primary ownership rather than mechanically distributing by entity
  type.
- **Cooldown ambiguity**: Name units, clocks, storage, persistence, reboot behavior, and commit
  timing.
- **Affect collisions**: Coordinate source and stacking namespaces so one subsystem cannot remove or
  suppress another's effects.
- **Unbounded event infrastructure**: Prefer direct typed hooks until multiple consumers prove a
  registry or bus is needed.
- **Persistence incompatibility**: Retain aliases or explicit migration for stable content-facing
  names and version any future multi-handler format.
- **Repository integration drift**: Synchronize both builds, use path-qualified feature includes,
  avoid local configuration headers, and never treat planning as permission to modify world or
  production configuration.
