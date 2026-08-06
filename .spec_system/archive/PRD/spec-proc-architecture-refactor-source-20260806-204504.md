# Special Procedure Architecture Refactor

Status: Architectural proposal
Last verified: 2026-08-06 against source commit `af9f79d2`

## Scope and Evidence

This proposal targets the supported Luminari build. Legacy campaign branches that remain in the
source are compatibility inventory, not additional design targets.

Current-state statements in this document were checked against the following implementation
surfaces:

- Callback declarations and owner storage in `src/structs.h`, `src/db.h`, and `src/utils.h`.
- Command, pulse, combat, identification, and maneuver call sites in `src/interpreter.c`,
  `src/mob/mob_act.c`, `src/comm.c`, `src/combat/fight.c`, `src/combat/act.offensive.c`, and
  `src/obj/act.item.c`.
- Registry, hard-coded assignment, world loading, boot ordering, and OLC persistence in
  `src/spec_assign.c`, `src/db.c`, and `src/olc/`.
- Shop and quest composition in `src/obj/shop.c`, `src/quest/quest.c`, and
  `src/olc/genqst.c`.
- Cooldown, affect-source, damage, and object-save contracts in `src/limits.c`, `src/handler.c`,
  `src/handler.h`, `src/combat/fight.c`, `src/utils.h`, and `src/obj/objsave.c`.
- Artifact mechanics and tests in `src/obj/spec_artifacts.c`, `src/obj/spec_artifacts.h`,
  `unittests/CuTest/test_artifacts.c`, and `unittests/CuTest/test_artifact_integration.c`.
- Persisted binding inventory under `lib/world/mob/`, `lib/world/obj/`, and `lib/world/wld/`, plus
  source/test membership in `Makefile.am` and `CMakeLists.txt`.

Line counts and world-data counts below are a dated snapshot, not architectural invariants. Symbol
names and behavioral contracts are preferred over line-number references because line numbers will
move during the refactor.

## Purpose

LuminariMUD expects to add substantial custom behavior for mobiles, objects, rooms, zones, and
possibly world-level events. The current special-procedure implementation can support more code,
but continuing to add behavior directly to the existing large files will make registration,
testing, reuse, and safe composition progressively harder.

This document records a proposed direction for reorganizing special procedures and extracting
selected reusable mechanics from the artifact system. It is an architectural recommendation, not
an implementation specification. No behavior or world-data migration is implied merely by this
document.

## Executive Recommendation

Create a small special-procedure subsystem with five clearly separated responsibilities:

1. Explicit invocation gateways and compatibility with the existing `SPECIAL` callback ABI.
2. A typed, validated definition registry with stable persisted identities.
3. A separate binding layer used by boot code, OLC, world files, and legacy assignments.
4. Narrow reusable mechanics such as eligibility checks, cooldowns, target resolution, and safe
   combat results.
5. Authored content organized either by reusable owner type or by a cohesive feature or zone.

Do not create one large `spec_utils.c` file. That would eventually become another general-purpose
dumping ground. Utilities should be small, semantic modules with clear contracts.

Do not make the artifact subsystem the generic parent of all item procedures. The artifact work
contains several excellent reusable patterns, but artifact ownership, persistence, binding,
progression, and custody are intentionally specialized mechanics.

## Current State

The four files central to this discussion contain 24,150 lines in the verified snapshot:

| File | Verified lines | Current responsibilities |
| --- | ---: | --- |
| `src/spec_procs.c` | 12,212 | Mixed abilities, entity procs, features, and helpers |
| `src/obj/spec_artifacts.c` | 6,489 | Artifact systems and test seams |
| `src/zone_procs.c` | 4,178 | Bespoke zone and encounter behavior |
| `src/spec_assign.c` | 1,271 | Assignments, name registry, and accessors |

File size is not the central problem by itself. The more important issue is that unrelated kinds of
behavior share the same ABI, registry, and files without communicating their actual invocation
requirements.

### One overloaded callback ABI

`SPECIAL_DECL` has the following effective shape:

```c
int handler(struct char_data *ch, void *me, int cmd, const char *argument);
```

The same callback is invoked from all of the following paths. The source paths for these symbols are
listed under Scope and Evidence.

| Invocation | Call site | Legacy signal | Return meaning |
| --- | --- | --- | --- |
| Command | `special()` | Command and argument | Nonzero consumes |
| Mobile activity | `mobile_activity()` | `cmd == 0`; `""` | Nonzero skips default AI |
| Mobile combat turn | Combat turn | `cmd == 0`; `""` | Ignored |
| Object auto-proc | `proc_update()` | `cmd == 0`; `""` | Controls carried fallback |
| Item identification | Item display | `cmd == 0`; `"identify"` | Ignored |
| Weapon hit | `weapon_special()` | `cmd == 0`; hit token | Caller ignores |
| Defense reaction | Combat messaging | `cmd == 0`; reaction token | Ignored |
| Shield maneuver | Shield commands | `cmd == 0`; maneuver token | Ignored |
| Mounted charge | `perform_charge()` | `cmd == 0`; `"charge"` | Ignored |
| Moving room | `moving_rooms_update()` | Nulls; `cmd == 0`; state in `me` | Ignored |
| Shop secondary | `shop_keeper()` | Incoming context unchanged | Nonzero propagates |
| Quest secondary | `questmaster()` | Incoming context unchanged | Nonzero propagates |

The defense tokens are `"shieldblock"`, `"parry"`, `"glance"`, and `"dodge"`. The shield
maneuver tokens are `"shieldpunch"`, `"shieldcharge"`, and `"shieldslam"`.

Scheduling is also observable. The heartbeat calls moving-room relocation every ten seconds. On
`PULSE_MOBILE` it calls `mobile_activity()` before `proc_update()`. The mobile combat callback runs
after that combatant's normal attacks and cleave handling. Gateways must preserve these positions
unless a separately tested change intentionally moves them.

Command dispatch has another observable ordering contract: room, equipped objects in wear-slot
order, carried objects, mobiles in room-list order, and room contents. The first nonzero result
stops the traversal. A refactor must preserve that order until an intentional behavior change is
separately specified and tested.

Most internal events use `cmd == 0`; their string in `argument`, caller location, and surrounding
state are the only discriminators. A weapon-hit callback receives neither the actual target nor the
damage dealt and commonly reconstructs the target from `FIGHTING(ch)`. The object auto-proc path
may first call a handler with a null `ch`, then call it again with `carried_by` if the first result
is zero.

The moving-room path selects a room function pointer but passes `struct moving_room_data *`, not the
room, through `me`. These are compatibility facts, not contracts to copy into the typed API.

This makes incorrect casts and accidental execution in the wrong path too easy. It also makes a
general helper difficult to write because the helper cannot reliably identify why a procedure was
called. It also means that a single adapter located only at the handler cannot recover the lost
event information. Typed migration must introduce event-specific gateways at the call sites and
translate to the legacy strings only for handlers that still need them.

### Return values are not one contract

The callback's `int` result currently means "command handled," "skip default mobile activity," or
"do not make the fallback auto-proc call," depending on the caller. Several combat and diagnostic
callers discard it entirely. This is especially dangerous when a procedure deals lethal damage or
extracts an object: the outer caller may continue unless it has an independent safety check.

The artifact combat hook demonstrates the safer pattern. `artifact_weapon_proc()` receives the
real attacker, target, weapon, damage, and critical state, and its lethal result stops the
surrounding hit pipeline. The general design needs similarly explicit, event-specific control
semantics; it should not assign one universal meaning to a legacy nonzero return.

The command object/mobile traversals and `proc_update()` also advance through live list pointers
without consistently caching the successor before calling a procedure. A handler that extracts its
owner and then returns zero can leave the caller advancing through freed storage. Characterization
must establish which handlers can extract; the gateway migration should cache successors and honor
typed invalidation before making extraction a supported contract.

### One handler per prototype

`room_data` and the mobile/object `index_data` each store one special-procedure function pointer.
This means a prototype cannot naturally have two independent C behaviors.

The shop and quest systems already work around this limitation by saving an existing procedure as a
secondary callback and manually chaining it from `shop_keeper` or `questmaster`. That workaround is
reasonable for legacy compatibility, but it will not scale cleanly to many independently authored
features.

This limitation is per prototype. Command dispatch can already run procedures owned by several
different room occupants and objects; future composition must not accidentally change that outer
owner traversal while adding a chain within one prototype.

The room pointer also doubles as a moving-room lifecycle hook. Parsing an `M` room record installs
`moving_rooms`; parsing a later `Z` record writes the same slot. The OLC writer emits moving-room
data before any registered `Z` name, so a room carrying both records would reload with the `Z`
procedure in the slot that `moving_rooms_update()` later calls with moving-room state. The binding
design must model this system-owned use and reject or separate that collision; a general handler
chain would not make the incompatible payload safe. No numeric moving-room `M` records are present
in the checked-in world snapshot, so this is a latent format/runtime collision rather than a
current content migration.

### Procedure pointers and activation flags are independent

A mobile's procedure pointer and `MOB_SPEC` flag are separate state:

- Command dispatch calls a mobile procedure when the pointer exists, without checking `MOB_SPEC`.
- Mobile activity and combat-turn calls require `MOB_SPEC` as well as a non-null pointer.
- `mobile_activity()` removes `MOB_SPEC` at runtime when the flag is present but the pointer is
  null.

Objects have a similar distinction. Command, identification, and combat call sites use the object
procedure pointer directly, while periodic `proc_update()` additionally requires `ITEM_AUTOPROC`.
Selecting a function in OLC does not by itself establish these prerequisite flags.

Owner metadata alone is therefore insufficient. Definitions and OLC validation must also describe
the events a procedure expects and any prototype flags or placement requirements needed to receive
those events.

The `-s`/`no_specials` mode is not one global dispatch gate. It skips hard-coded assignment and
shop loading, bypasses command special dispatch, and suppresses the mobile-activity call.
World-file names are still resolved while the world loads, and the direct identification,
auto-proc, moving-room, and combat call sites do not check `no_specials`. Compatibility tests and
binding diagnostics must therefore cover normal and `-s` boot modes rather than treating the flag
as "all specs disabled."

### Registry and owner types are disconnected

`spec_func_list[]` maps one field that serves as both display text and persisted identity to a
function pointer. It does not record whether a procedure is valid for a mobile, object, or room, nor
does it describe which invocation events or prototype flags it expects.

The same list is presented by medit, oedit, and redit. A builder can therefore select an
incompatible procedure. Current documentation notes that a mismatched procedure may simply do
nothing. Silent failure is undesirable once special procedures become a major content mechanism.

In the verified snapshot the registry exposes 29 persisted names backed by 28 distinct handlers;
`Guildmaster` and `Guild` both resolve to `guild`. All description fields are empty, and there is no
description accessor or OLC display path. Filling the strings without wiring the UI would therefore
not improve the builder experience.

The registry covers only a small subset of the procedures implemented and hard-coded in
`spec_procs.c` and `zone_procs.c`. Correspondingly, the checked-in world currently contains only one
mob `SpecProc` field, two object `Z` bindings, and two room `Z` bindings. World-file binding exists,
but it is not yet the dominant source of truth.

The five bindings are `Postmaster` in `lib/world/mob/12.mob`, `Greyhawk Ship` in
`lib/world/obj/14.obj` and `lib/world/obj/700.obj`, and `Greyhawk Ship Commands` in
`lib/world/wld/14.wld` and `lib/world/wld/700.wld`. This inventory should be regenerated during
implementation rather than treated as a permanent migration list.

Lookup is case-insensitive, while writers recover a name by reverse-looking-up the function pointer.
This makes the first matching name an implicit canonical name. An unknown persisted name resolves to
`NULL` without a diagnostic, and the raw name is then discarded from runtime state. A later OLC save
can omit that unresolved binding entirely. The future API must distinguish canonical persisted IDs,
display labels, and aliases, and it must never silently erase an unresolved ID.

Writers also serialize the current effective function pointer rather than the authored binding and
its source. After boot-time overwrites, an unrelated OLC save can therefore promote a registered
hard-coded assignment into world data, omit an unregistered effective handler, or lose the world
name that preceded it. Binding provenance is required for safe round trips even when every name is
known.

### Registration and assignment are mixed together

There are two separate concepts in `src/spec_assign.c`:

- Registration: making a stable procedure name resolvable from world files and OLC.
- Assignment: attaching a procedure to a particular hard-coded VNUM at boot.

These should not be treated as the same responsibility. Mob, object, and room files already support
persisting a registered procedure name. New content should normally attach a registered procedure
through world data rather than add another hard-coded `ASSIGNMOB`, `ASSIGNOBJ`, or `ASSIGNROOM`
call.

Hard-coded assignments remain necessary as a compatibility layer until existing content is audited
and migrated. They should not be the default growth path.

With specials enabled, boot precedence is currently implicit and order-dependent:

1. `boot_world()` parses persisted mob, object, and room names into function pointers.
2. `assign_mobiles()` overwrites matching mobile pointers.
3. `assign_the_shopkeepers()` saves a mobile's current pointer as a shop secondary, then installs
   `shop_keeper`.
4. `assign_objects()` and `assign_rooms()` overwrite matching object and room pointers.
5. `assign_the_quests()` saves a quest master's current pointer as a quest secondary, then installs
   `questmaster`.

This ordering can intentionally compose a quest master over a shop keeper over an earlier mobile
procedure. It can also silently replace a world-file binding. The migration must inventory and test
the effective binding after the complete boot sequence, not merely compare individual assignment
calls. A future binding layer should record the binding source and report collisions; current
precedence must remain explicit until each collision is deliberately resolved.

### `zone_procs.c` is not a zone callback system

The current `zone_procs.c` name means "special content associated with particular zones." The file
contains mobile and object procedures plus shared encounter state and helpers. The `zone_data`
structure in `src/db.h` does not contain a special-procedure callback.

This distinction matters when discussing future zone or world procedures. An actual zone event such
as reset, entry, departure, or pulse should have an explicit zone-event interface. It should not be
encoded as another ambiguous `SPECIAL` call with a zone pointer hidden in `void *me`.

### `spec_procs.c` owns unrelated systems

The existing file contains several functions whose primary job is not special-procedure dispatch or
content. Examples include:

- Spell, skill, and ability listing and calculation.
- Moving-room and legacy ship behavior.
- Vendor item construction and naming.
- Crafting-mold purchasing and construction.

During a reorganization, these functions should move to their actual owning subsystem where
practical. Merely dividing the existing file into `mob_specs.c`, `obj_specs.c`, and `room_specs.c`
would leave unrelated responsibilities hidden inside the new files.

The moving-room callback is the clearest boundary case. Room parsing installs `moving_rooms` for an
`M` record, but the ten-second heartbeat caller passes moving-room state through `me`. It should
receive a temporary compatibility gateway, then move to a direct typed hook owned by the moving-room
or vessel subsystem rather than become a general builder-selectable room procedure.

## Design Principles

### Preserve behavior before redesigning dispatch

The current callback ABI, persisted names, world-file formats, and hard-coded assignments should
remain compatible during the first migration steps. File moves, registry metadata, and validation
can deliver value without a simultaneous rewrite of every procedure.

Compatibility includes more than the callback signature. The command-owner traversal order, boot
assignment precedence, `MOB_SPEC` and `ITEM_AUTOPROC` prerequisites, secondary shop and quest
callbacks, legacy event strings, and caller-specific return handling are all observable behavior.
Each must be characterized before it is changed.

### Separate definitions, bindings, and invocation

A procedure definition answers "what behavior and events does this stable name represent?" A
binding answers "which prototype uses it, from which source, and with what collision result?" An
invocation gateway answers "why is it running now, what data is valid, and what may the caller do
after it returns?" Keeping these records separate avoids another table that mixes content identity,
boot mutation, and runtime control flow.

### Prefer typed context over convention

New framework code should explicitly describe:

- What kind of owner is being invoked.
- What event caused the invocation.
- Who the actor and target are.
- Which object, mobile, room, or zone owns the behavior.
- Whether command arguments, damage, critical-hit state, or other event data are available.

Legacy `SPECIAL` functions can be adapted into this model gradually.

Typed contexts must be constructed while complete event data is still available. A handler-side
wrapper cannot recover a weapon victim, damage result, or event identity after a legacy caller has
reduced them to `cmd == 0` and a magic string.

### Make pointer lifetime and control flow explicit

Context pointers are borrowed for one synchronous invocation. A handler may invalidate an owner,
actor, or target, so the gateway contract must state which results stop the surrounding pipeline and
which pointers the caller may inspect afterward. Legacy handlers that cannot report invalidation
remain compatibility code, not proof that post-call dereferences are safe.

### Prefer narrow semantic helpers

A helper should represent a stable game rule, not merely shorten a few lines. Good candidates
include "is this equipped object eligible for a combat proc?" and "resolve an offensive room target
and apply aggression rules." Poor candidates include wrappers that only rename `damage()` or
`send_to_char()` without adding a contract.

### Keep authored content close to its primary owner

Reusable behavior should normally be grouped by owner type:

- General mobile behaviors together.
- General item behaviors together.
- General room behaviors together.

Bespoke encounter content should instead remain grouped by feature or zone when several mobiles,
objects, and rooms share state, VNUMs, sequencing, and helpers. Splitting a single encounter across
three distant entity files would reduce cohesion rather than improve it.

### Prefer world-data binding for new content

The registry should define what a procedure is. World data should normally define where it is used.
This keeps prototype-to-procedure binding visible to builders and avoids recompiling merely to add a
second mobile that uses an existing behavior.

World data is the preferred authoring source, not yet the guaranteed winner during boot. Until the
legacy assignment layer is retired, collisions must preserve and report the verified current
precedence rather than silently changing it.

### Keep DG Scripts in the decision process (for future specs)

Not every custom behavior should become C code. DG Scripts remain a better fit for many narrative,
dialogue, sequencing, puzzle, and localized zone behaviors. C special procedures are most valuable
when behavior:

- Requires engine-level state or hooks not exposed to scripts.
- Must be reused broadly and consistently.
- Is performance-sensitive.
- Needs strong combat, persistence, or lifecycle guarantees.
- Would otherwise require fragile duplication across many triggers.

The goal is not to replace DG Scripts. The goal is to make the C behavior that is genuinely needed
safe and maintainable.

## Non-Goals

This proposal does not require any of the following in its first implementation:

- Rewriting all legacy `SPECIAL` handlers.
- Changing the mob, object, or room file format.
- Adding multiple handlers to every prototype.
- Creating a generic asynchronous event bus.
- Moving artifact ownership, progression, or persistence into the spec subsystem.
- Migrating every hard-coded assignment into world data in one change.
- Editing local configuration headers such as `src/vnums.h`; any template-level VNUM change belongs
  in `src/vnums.example.h` and must be justified independently.

## Proposed Architecture

The exact filenames can change during implementation planning, but the responsibilities should
remain separate. A possible shallow layout is:

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

This introduces only one new flat feature directory and no second-level nesting. Files outside that
directory would include its headers with path-qualified names such as
`#include "spec/spec_registry.h"`.

The existing top-level `spec_procs.c` and `spec_procs.h` can act as compatibility surfaces while
code is migrated. They should become smaller over time rather than receive new unrelated behavior.

`spec_cooldown.c` and `spec_effects.c` should be created only when their contracts have at least two
real consumers. The layout is a responsibility map, not a requirement to create empty modules. Any
source-file addition or removal must update both `Makefile.am` and `CMakeLists.txt` in the same
change.

The intended dependency and control flow is:

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

The definition registry does not mutate prototypes, and the runtime gateway does not decide where a
procedure is bound.

### Runtime context

A future typed API should initially model only the invocation paths that exist. Zone and world
lifecycle events use a separate API if concrete consumers later justify them. One illustrative shape
is:

```c
enum spec_owner_type
{
  SPEC_OWNER_MOBILE,
  SPEC_OWNER_OBJECT,
  SPEC_OWNER_ROOM
};

enum spec_event
{
  SPEC_EVENT_COMMAND,
  SPEC_EVENT_MOBILE_ACTIVITY,
  SPEC_EVENT_MOBILE_COMBAT_TURN,
  SPEC_EVENT_OBJECT_AUTO_PULSE,
  SPEC_EVENT_ITEM_IDENTIFY,
  SPEC_EVENT_WEAPON_HIT,
  SPEC_EVENT_DEFENSE_REACTION,
  SPEC_EVENT_COMBAT_MANEUVER,
  SPEC_EVENT_MOUNT_CHARGE,
  SPEC_EVENT_MOVING_ROOM_RELOCATION
};

enum spec_flow
{
  SPEC_FLOW_CONTINUE,
  SPEC_FLOW_STOP
};

enum spec_invalidation
{
  SPEC_INVALIDATE_NONE = 0,
  SPEC_INVALIDATE_OWNER = (1 << 0),
  SPEC_INVALIDATE_ACTOR = (1 << 1),
  SPEC_INVALIDATE_TARGET = (1 << 2)
};

struct spec_context
{
  enum spec_owner_type owner_type;
  enum spec_event event;
  union
  {
    struct char_data *mobile;
    struct obj_data *object;
    struct room_data *room;
  } owner;
  struct char_data *actor;
  union
  {
    struct
    {
      int cmd;
      const char *argument;
    } command;
    struct
    {
      struct char_data *target;
      int damage;
      int is_critical;
    } weapon_hit;
    struct
    {
      struct char_data *target;
      const char *reaction;
    } defense;
    struct
    {
      struct char_data *target;
      const char *maneuver;
    } combat_maneuver;
    struct
    {
      struct moving_room_data *state;
      struct room_data *destination;
    } moving_room;
  } data;
};

struct spec_outcome
{
  enum spec_flow flow;
  unsigned int invalidated;
};
```

This is illustrative, not a final data model. The important point is that owner type, event, and
event payload become explicit. Event-specific payload structures can replace the union if that
produces clearer call sites.

`SPEC_FLOW_STOP` means "stop this gateway," not one universal game action. Each gateway defines its
scope:

| Gateway | `SPEC_FLOW_STOP` contract |
| --- | --- |
| Command | Consume the command and stop later owner traversal |
| Mobile activity | Skip the remaining default activity for this mobile |
| Object auto-pulse | Skip the carried-object fallback invocation |
| Typed combat action | Abort only the surrounding action explicitly named by that gateway |
| Notification-only compatibility event | Invalid; log a contract error and continue safely |

Invalidation flags are separate from control flow. A typed handler can report that it extracted an
owner or target even when no command was consumed. Callers must cache iteration successors before
invocation and must not probe an object after a handler may have freed it. Character pending-
extraction flags can be checked where available; there is no equivalent safe post-free object test.

The initial implementation does not need to replace `SPECIAL_DECL`. Compatibility happens in the
gateway: typed handlers receive the complete context, while legacy handlers receive the exact
current `ch`, `me`, `cmd`, and argument translation. The gateway must preserve existing handling of
the legacy return value until a handler is converted and its typed contract is tested.

### Invocation gateways

Every verified call path should route through a small, event-specific gateway before handler
conversion begins. The initial gateway set should cover:

- Command owner traversal.
- Mobile activity and mobile combat turns.
- Object auto-proc worn/carried fallback.
- Item identification.
- Weapon hits and defensive reactions.
- Shield maneuvers and mounted charge.
- Moving-room relocation pulses.
- Shop and quest secondary callback forwarding.

Gateways are the testable compatibility seam. They validate owner and actor shape, construct typed
payloads while full data is available, translate legacy tokens, and enforce the caller's exact stop
and lifetime rules. They must not reorder command owners or alter pulse scheduling.

### Definition registry

The registry should describe immutable behavior definitions rather than assignments. Metadata could
have a shape similar to:

```c
struct spec_event_contract
{
  enum spec_event event;
  unsigned int required_prototype_flags;
  unsigned int required_placement;
};

struct spec_definition
{
  const char *canonical_name;
  const char *display_name;
  const char *const *aliases;
  size_t alias_count;
  unsigned int owner_mask;
  const struct spec_event_contract *events;
  size_t event_count;
  unsigned int binding_source_mask;
  SPECIAL_DECL(*legacy_handler);
  spec_handler typed_handler;
  const char *description;
  const char *category;
};
```

The exact representation is less important than the following guarantees:

- Every definition has a stable, case-insensitively unique canonical name. For definitions that
  permit world-data binding, that name is the persisted identity.
- Aliases are explicit, case-insensitively unique, and never selected accidentally by reverse
  function-pointer lookup.
- Exactly one legacy or typed handler is present for each definition.
- Empty names, empty descriptions, invalid masks, invalid event contracts, and handlerless
  definitions are rejected before world files are parsed.
- Definitions that share implementation code retain distinct definition identity. While effective
  storage is only a function pointer, alternate persisted names for one behavior are aliases rather
  than duplicate definitions that reverse lookup cannot distinguish.
- OLC lists only procedures compatible with the edited prototype type.
- OLC lists only definitions explicitly marked for builder/world-data binding; hard-coded-only or
  internal compatibility handlers do not become selectable merely because they have metadata.
- OLC displays descriptions, per-event support, and prerequisites such as `MOB_SPEC`,
  `ITEM_AUTOPROC`, equipped placement, or combat state.
- Loading a procedure on the wrong owner type reports its persisted name, owner type, and VNUM.
- Unknown persisted names retain their raw text for diagnosis and round-trip safety instead of
  silently becoming `NULL`.
- Registry accessors perform complete bounds checking.

The current reverse lookup emits `Guild` because that row precedes `Guildmaster`. The compatibility
default should therefore make `Guild` canonical and retain `Guildmaster` as an accepted alias,
subject to an external content audit. Loading either name must not rewrite an unrelated OLC save;
an explicit re-selection may save the canonical name.

Invalid definition metadata is a programmer error and should fail boot early. An unknown world-data
name is a content error: report it with source location and preserve it in the binding record. Until
raw-name preservation exists, OLC must refuse to overwrite that prototype's unresolved field unless
the builder explicitly replaces or clears it; a log message alone does not prevent data loss.

### Binding records

A binding record should contain at least:

- Owner type and prototype identity.
- The requested persisted name, including unresolved raw text.
- The resolved definition, if any.
- Source: named world data, system-owned parser hook, legacy assignment, shop wrapper, or quest
  wrapper.
- The effective result after precedence and collision handling.

During compatibility migration, the existing function pointer in `room_data` or `index_data` remains
the effective dispatch slot. The binding layer records how that pointer was selected; it does not
require an immediate world-file format change.

Binding metadata should travel with the prototype or its OLC copy, not live only in a global array
indexed by mutable rnums. Known definitions can reference interned registry strings; unresolved raw
names need owned storage with explicit copy/free handling. Writers should consult the binding record
first and use reverse function-pointer lookup only as a legacy fallback. This is what makes alias
identity and unresolved-name round trips possible while OLC inserts, copies, or reloads prototypes.

Collision handling must initially reproduce existing effective bindings while making them
observable. A world-data binding overwritten by a legacy assignment should generate one structured
diagnostic showing both sources and the chosen result. A newly encountered `M`/`Z` room collision
should be rejected until moving-room state has its own typed hook. Shop and quest wrappers must
record their saved secondary callback so the existing quest-over-shop-over-original chain remains
traceable. Once an individual collision is migrated, policy may change deliberately and with a
regression test.

### Assignment data

Hard-coded compatibility assignments should use declarative tables where practical:

```c
struct legacy_spec_assignment
{
  enum spec_owner_type owner_type;
  int vnum;
  const char *spec_name;
};
```

This would allow one validator and assignment loop to replace hundreds of repetitive calls while
preserving existing behavior. A row is valid only after its handler has a definition; adding
metadata must not make a zone-private or hard-coded-only procedure builder-selectable. `vnum`
entries must use traced symbolic constants rather than numeric literals. Zone-specific computed
assignments and special setup can remain in their owning modules.

The compact `int vnum` above is illustrative. Implementation should use separate owner-typed
tables or a tagged union of `mob_vnum`, `obj_vnum`, and `room_vnum` so a room constant cannot be
assigned to a mobile row without an explicit conversion.

`spec_name` refers to the definition's canonical name. It is a diagnostic identity for a
hard-coded-only definition and a persisted identity only when its binding-source policy permits
world data.

If an existing numeric assignment has no symbolic constant, defer that table conversion or handle a
template/configuration change separately under repository policy. The refactor must not edit the
local `src/vnums.h`; a template change belongs in `src/vnums.example.h` and still requires the build
environment to supply the matching local configuration.

New assignments should normally be persisted in mob, object, or room files. A migration audit can
later determine which hard-coded bindings are already represented in world data, which intentionally
wrap shop or quest behavior, and which must be retained. The audit unit is the effective post-boot
binding, not an isolated `ASSIGN*` call.

## Reusable Mechanics

The following are strong candidates for central helpers because the same safety rules recur in
`spec_procs.c`, `zone_procs.c`, and `spec_artifacts.c`.

### Context validation

Provide helpers that validate an already-typed invocation context, for example:

- Mobile command context.
- Mobile activity or combat-turn context.
- Object auto-pulse context.
- Equipped-object command context.
- Equipped-object weapon-hit or defense context.
- Room command context.

These helpers should reject invalid owner/event combinations, missing required actors or targets,
invalid rooms, absent combat state, missing prototype flags, and unsupported placement. They must
not claim to detect an object that has already been freed.

The existing `obj_proc_ready()` is only a partial starting point: it recognizes equipment by
matching the object's VNUM through `is_wearing()`, not by proving that the invoking instance is the
one worn by the actor. A generalized equipped-object check should use pointer identity such as
`obj->worn_by == actor`, then apply the event's combat and command requirements.

### Command and phrase matching

Centralize common rules for:

- Exact command matching.
- Argument splitting.
- Normalizing case and trailing punctuation.
- Prefix phrases with a required target argument.
- Distinguishing a handled invocation from unrelated speech or commands.

The artifact called-effect dispatcher is a useful example: phrase, invocation channel, target rule,
description, and runtime effect all come from one data row.

Normalization must be opt-in. Changing abbreviation, punctuation, or case behavior globally can
alter existing commands and speech-triggered procedures, so each migrated handler needs a
characterization test for its accepted input.

### Cooldowns

Cooldown APIs should make clock, units, storage, and persistence explicit. The code currently has at
least two materially different models:

- Legacy object `spec_timer[]` counters decrement once per `point_update()`, currently once per MUD
  hour (`SECS_PER_MUD_HOUR`, currently 75 real seconds). They live on an object instance and are not
  serialized by `src/obj/objsave.c`, so they reset when the object is recreated or the server
  restarts.
- Artifact `time_t` stamps use real seconds. Ability, generic-proc, signature-proc, and
  called-effect recharge stamps are written to and restored from artifact persistence.

The engine also has event-backed and character-specific cooldown mechanisms. These models should not
be silently treated as interchangeable. A cooldown contract must declare:

- Clock and unit: update ticks, pulses, MUD hours, or wall-clock seconds.
- Storage owner and slot bounds.
- Persistence and reboot behavior.
- When the cooldown is committed.
- How remaining time is formatted for players and diagnostics.

Cooldowns should normally be spent only after validation and successful execution. Failed target
resolution, stacking rejection, immunity, or another no-effect outcome should not consume a power
unless that behavior is explicitly part of the design.

### Combat and target safety

Common helpers should cover recurring safety requirements:

- Actor and target are alive and not pending extraction.
- Actor and target remain in the same room when required.
- Offensive actions pass `aoeOK` or the appropriate aggression rule.
- Required combat state exists.
- Equipped objects are the exact instances worn by the actor invoking them.
- Damage results report whether a target died or was extracted.
- Multi-target loops retain the next pointer before effects can remove an entry.
- Extra-attack procedures cannot recurse into themselves indefinitely.

These helpers should add contracts around existing combat primitives rather than create a second
combat engine. In particular, `damage()` already distinguishes no damage, damage, and death through
its return value; wrappers should preserve that result and name the caller action that must stop.

### Temporary affects and stacking

The core affect system already exposes `affect_to_char_source()`, `affect_from_char_source()`,
`affected_by_spell_source()`, and `affect_join_source()` using
`affected_type.source_id`. General spec helpers should build on those APIs rather than add a second
source field.

Artifact code currently uses `affected_type.specific` in two distinct ways: passive/permanent
artifact affects store a registry-derived artifact tag, while temporary surge affects store a
stacking-group value. That is useful precedent for explicit ownership and stacking, but it is not a
single source-tagged temporary-affect model. A general implementation should support:

- A namespaced source identity with a documented runtime or persistence lifetime.
- An explicit stacking group.
- Bonus type, location, modifier, duration, and flags.
- Removal by source without stripping unrelated effects.
- A clear result when another effect already occupies the group.

If `specific` is reused for stacking, its namespace and range must be coordinated with existing
spell and artifact users. Source ownership and stacking group must never be overloaded into one
number. Artifact-specific XP and progression remain outside this helper.

### Chance and proc policy

Reusable chance handling may include:

- Percentage rolls with validated ranges.
- One-in-N rolls with clear naming.
- Optional bad-luck protection.
- Independent versus shared cooldown policy.
- Test injection for deterministic rolls.

The policy must be visible in data or function names. Hidden interactions between a generic proc,
signature proc, and nested extra attack are difficult to balance and test.

## Lessons From the Artifact System

The artifact implementation demonstrates several approaches worth carrying into general special
procedure development.

It is also an important architectural counterexample to the overloaded `SPECIAL` ABI. Artifact
combat and item hooks use explicit core APIs. For example, `artifact_weapon_proc()` receives the
attacker, victim, weapon, damage, and critical state directly, and the hit pipeline consumes its
lethal result. The general spec migration should copy that explicit-hook shape, not route artifacts
back through `SPECIAL`.

### Patterns worth extracting

- Registry membership as the single source of truth.
- Table-driven templates and called-effect content contracts.
- Central target resolution before effect dispatch.
- One channel-aware matcher for phrase, target rule, recharge, description, and effect dispatch.
- Reusable proc shapes selected by data.
- Explicit, persisted cooldown stamps and recharge queries.
- Existing affect-source APIs for ownership where appropriate.
- Explicit stacking groups.
- Boot-time metadata validation.
- Deterministic test seams for random combat behavior.
- Validation first, execution second, cooldown spending last.

### Mechanics that should remain artifact-specific

- Unique-instance enforcement.
- Ownership and account binding.
- Custody and provenance history.
- Artifact XP and levels.
- Artifact save-file format and dirty-state persistence.
- Artifact class rejection and burn behavior.
- Chronicle and recovery policy.
- Artifact-specific effect text and content contracts.

The general subsystem should depend on neither `artifact_data` nor artifact VNUMs. Artifact code may
call general helpers, but general helpers should not call back into artifact progression or custody.

### Splitting the artifact implementation

Independently of the general spec refactor, the artifact module is now large enough to benefit from
responsibility-based files under the existing `src/obj/` directory, for example:

```text
src/obj/artifact_registry.c
src/obj/artifact_persistence.c
src/obj/artifact_ownership.c
src/obj/artifact_effects.c
src/obj/artifact_combat.c
src/obj/artifact_commands.c
```

This split should preserve one public artifact API and should be performed only with strong
production-linked regression coverage. Private declarations should move to one internal header
rather than become new public APIs merely to make the split compile. Existing coverage in
`unittests/CuTest/test_artifacts.c` and `unittests/CuTest/test_artifact_integration.c` should remain
production-linked, and each new source file must be added to both build manifests.

## Content Organization

### Reusable mobile procedures

General behaviors such as guards, pets, janitors, practice targets, common undead behaviors, and
service NPCs are candidates for `spec_mobiles.c` or their true owning subsystem.

If a behavior already belongs to a mature subsystem, that subsystem should own it. For example, shop
and quest entry points should remain with the shop and quest systems rather than move merely because
their owner happens to be a mobile.

### Reusable object procedures

The contiguous object-procedure section of `src/spec_procs.c` is a useful first extraction target.
It already has a partial common eligibility helper in `obj_proc_ready()`, and many procedures repeat
cooldown, combat, chance, affect, and messaging patterns.

Items belonging to established subsystems should remain there:

- Artifact behavior under `src/obj/`.
- Vessel objects and controls under `src/vessels/`.
- Shop and trade mechanics under `src/obj/`.
- Crafting-specific purchase and construction behavior under `src/craft/`.

### Reusable room procedures

General room behaviors can live in a shared room-procedure module. Moving rooms and vessel control
rooms should instead live with the vessel subsystem because their primary responsibility is vessel
operation.

### Zone packages

The existing `zone_procs.c` has natural packages that can be split without imposing an entity-only
organization:

- King's Castle.
- Abyss.
- Crimson Flame.
- The Prisoner.
- Celestial Leviathan.
- Fire Giant content.
- Jot.
- Mad Drow.
- Temple of Twisted Flesh.

Each package should keep private helpers and encounter state file-local where possible. Shared
mechanics should move outward only after at least two real packages need the same contract.

## Multiple Procedure Composition

Supporting many custom features may eventually require more than one procedure per prototype. This
means an inner chain for one owner; it must not change the existing outer command traversal across
the room, equipment, inventory, mobiles, and room contents.

The current shop and quest wrappers are compatibility composition, not a general chain. Because boot
ordering can produce quest-over-shop-over-original nesting, those secondary callbacks must be
represented explicitly before replacing them.

A future per-owner chain must define:

- A deterministic order based on persisted definition IDs and explicit policy, never function
  addresses or accidental registry order.
- Binding source and collision behavior for every entry.
- Event compatibility for each entry; an object command handler must not automatically receive a
  weapon-hit event.
- Gateway-specific continue and stop semantics.
- Owner, actor, and target invalidation behavior, including whether a stable snapshot or live chain
  is used if a handler changes bindings during dispatch.
- Whether a command result stops only the inner chain or also the existing outer owner traversal.
- Duplicate-handler policy and a bounded chain length.
- A versioned, backward-compatible persistence format for multiple canonical names.
- OLC display, reorder, clear, unresolved-name, and save behavior.

This is a larger migration because it affects:

- `room_data` and `index_data`.
- Mob, object, and room file parsers and writers.
- medit, oedit, and redit.
- Shops and quests with existing secondary-procedure chaining.
- Stat and diagnostic commands.
- Reload and OLC save behavior.

For that reason, handler composition should follow registry typing and compatibility extraction. It
should not be bundled into the first file-reorganization change. A chain should not ship until shop
and quest nesting, extraction during dispatch, and single-name world-file compatibility have
production-linked tests.

## True Zone and World Events

Zone and world behavior should use an event-oriented API rather than the command-oriented `SPECIAL`
signature. These are not initial values in `enum spec_event`: no zone callback is stored today, and
adding speculative variants would imply support that does not exist.

Potential zone events include:

- Zone boot.
- Before and after zone reset.
- Player enter and leave.
- Periodic zone pulse.
- Mobile death within the zone.
- Object load or extraction within the zone.
- Zone empty or first player arrival.

Potential world events include:

- World boot complete.
- Periodic world pulse.
- Day, night, weather, or calendar transitions.
- Global encounter or event lifecycle.
- Server shutdown preparation.

Only events backed by concrete use cases should be added. A broad event bus created in advance could
become harder to understand than direct hooks. Event payloads should be typed, synchronous behavior
should be explicit, and extraction safety should be part of each event contract.

For the first real consumer, prefer a direct typed hook at the lifecycle owner. Its contract must
define ordering relative to reset commands and DG Scripts, whether failure can veto the lifecycle
step, whether re-entry is allowed, and which zone or world data remains valid after the callback.
Generalize into a registry only after a second consumer demonstrates a shared contract.

## Migration Strategy

### Phase 1: Freeze observable behavior

1. Add production-linked characterization tests for all verified invocation paths, including exact
   legacy tokens, null-actor auto-proc behavior, return handling, and command-owner order.
2. Add registry tests for case-insensitive lookup, unknown names, both ends of accessor bounds, and
   the `Guild`/`Guildmaster` alias behavior.
3. Produce a diagnostic inventory of the effective post-boot binding and source for each bound
   prototype, including shop and quest secondary callbacks.
4. Record owner type, event paths, prototype flags, and placement requirements for every registered
   definition.

Exit condition: the current behavior is executable as tests and diagnostics; no callback or
world-file format has changed.

### Phase 2: Build the definition and binding control plane

1. Move definition data and accessors to a dedicated registry module.
2. Separate canonical name, display label, aliases, owner mask, per-event contracts, binding
   visibility, category, and description.
3. Validate all definition metadata before world parsing and make every accessor fully bounds-safe.
4. Preserve unresolved raw world-data names in binding state, or block OLC save until the builder
   explicitly replaces or clears them.
5. Record binding source and current precedence outcomes without changing those outcomes.
6. Filter medit, oedit, and redit by owner type and show event/prerequisite metadata and
   descriptions.
7. Diagnose unknown and incompatible bindings with source, owner type, and VNUM.

Exit condition: builders cannot silently select an incompatible definition or erase an unresolved
name, and startup can explain each effective binding. Legacy handlers still use `SPECIAL_DECL`.

### Phase 3: Introduce call-site gateways

1. Define the current event contexts, gateway-specific flow rules, and invalidation flags.
2. Route every verified command, pulse, identification, combat, maneuver, charge, moving-room, and
   shop/quest forwarding call site through its gateway without converting handlers.
3. Translate each gateway back to the exact legacy `ch`, `me`, `cmd`, argument, and return behavior.
4. Cache iteration state before callbacks and remove unsafe post-call dereferences where the current
   contract permits extraction.

Exit condition: all characterized non-extraction behavior remains identical, and complete event data
now reaches one testable compatibility seam at every call site. Any extraction-safety change is
documented and covered as an intentional fix.

### Phase 4: Make legacy assignments declarative and observable

1. Convert repetitive assignments to validated tables where behavior is a direct owner/VNUM/name
   binding.
2. Use existing symbolic VNUM constants; do not introduce numeric VNUM literals.
3. Keep computed assignments and special setup with their owning systems.
4. Preserve named-world, system-hook, hard-coded, shop, and quest precedence while reporting
   collisions.
5. Migrate a binding to world data only after comparing its effective pre- and post-boot behavior.

Exit condition: compatibility assignments are traceable and collision-tested; no intentional
shop/quest chain has been flattened.

### Phase 5: Extract content without behavior changes

1. Extract the general object-procedure section first, after its invocation gateways are covered.
2. Extract reusable mobile and room procedures.
3. Split `zone_procs.c` by its existing zone packages, including Celestial Leviathan.
4. Move vessels, vendors, crafting, abilities, and other unrelated functions to their true owners.
5. Preserve exported names, static-state ownership, initialization order, and callback signatures.

Exit condition: source responsibilities are coherent, characterization tests remain unchanged, and
every new or removed source is present in both `Makefile.am` and `CMakeLists.txt`.

### Phase 6: Introduce narrow shared mechanics

1. Add pointer-identity context validation for representative object and mobile procedures.
2. Add opt-in command or phrase parsing only where tests establish the old accepted input.
3. Add explicit cooldown operations with named clock, storage, and persistence contracts.
4. Add safe target and combat-result helpers around existing primitives.
5. Build affect helpers on the existing `source_id` APIs and keep stacking-group identity separate.
6. Migrate a small representative set before expanding usage.

Exit condition: each helper owns a documented rule used by at least two consumers and has focused
tests. Avoid mechanical conversion of every procedure.

### Phase 7: Add typed handlers incrementally

1. Implement new procedures with typed handlers behind the call-site gateways.
2. Convert legacy procedures when they are otherwise being changed or when conversion removes a
   demonstrated safety risk.
3. Preserve canonical persisted identities and compare typed behavior with characterization tests.
4. Track the remaining legacy population before considering removal of `SPECIAL_DECL` compatibility.

Exit condition: converted handlers no longer infer event data from magic strings or ambient combat
state, and their callers honor typed flow and invalidation results.

### Phase 8: Evaluate composition and lifecycle events

1. Gather concrete cases that require multiple handlers on one prototype.
2. Design and test inner-chain order without changing outer command-owner traversal.
3. Migrate shop and quest secondary callbacks deliberately.
4. Add only the zone and world hooks required by approved content.
5. Version any affected persistence format and retain backward-compatible loading.

Exit condition: each new abstraction has a real consumer, a complete ordering/lifetime contract, and
compatibility coverage. Composition and lifecycle events may remain unimplemented if no consumer
justifies them.

## Testing Expectations

The refactor should use the root production-linked CuTest suite for behavior that interacts with
real game structures and systems.

Important coverage includes:

- Registry canonical-name and alias uniqueness, case-insensitive lookup, and extreme accessor
  bounds.
- Registry validation timing before world parsing.
- Owner, event, and prerequisite compatibility in medit, oedit, and redit.
- OLC display of descriptions and round trips for known and unresolved names.
- OLC save after a hard-coded override, proving that authored provenance is neither promoted nor
  erased.
- Legacy world-file loading and canonical save behavior.
- Effective boot precedence across world, hard-coded, shop, and quest bindings.
- Exact gateway translation for every current magic string and empty-argument invocation.
- Room, equipped, carried, mobile, and room-object command traversal order.
- `MOB_SPEC` and `ITEM_AUTOPROC` activation behavior.
- Normal versus `-s`/`no_specials` loading, assignment, and per-call-site suppression behavior.
- Worn-then-carried auto-proc fallback, including null actors and return values.
- Notification-only legacy calls whose return value is intentionally ignored.
- Moving-room null-actor/null-argument translation and rejection of an incompatible `M`/`Z` binding.
- Exact equipped-object pointer identity when duplicate-VNUM instances exist.
- Cooldown units, reboot/persistence behavior, slot bounds, and spending only on intended outcomes.
- Target death, character pending extraction, and immediate object extraction during proc execution.
- Multi-target iteration safety.
- Recursive extra-attack suppression.
- Affect source removal, source namespace separation, and stacking rejection.
- Multiple-handler ordering if composition is introduced.
- Shop and quest secondary behavior during migration.

After root `make test`, always run `make install` so the tested server is installed as `bin/circle`
and no root-level `circle` artifact remains.

## Documentation Expectations

Implementation should update:

- `docs/guides/OLC_SpecProcs.md`.
- The `SPECIALS` entry in `lib/text/help/help.hlp`, which currently describes hard-coded assignment
  as the only path.
- Architecture and developer documentation describing dispatch and registration.
- System documentation for any procedure moved into an established subsystem.
- `docs/systems/ARTIFACT_SYSTEM.md` if artifact APIs or persistence responsibilities move.
- `docs/TECHNICAL_DOCUMENTATION_MASTER_INDEX.md` if a new long-lived architecture document is
  added.

Persisted procedure names, canonical aliases, event prerequisites, collision diagnostics, and
migration compatibility should be documented for builders and staff. Documentation must describe
implemented phases as current behavior and leave later phases clearly labeled as proposals.

## Risks and Guardrails

### Behavior drift during file moves

Moving large legacy procedures can accidentally change includes, declarations, static state,
initialization order, or callback ordering. Initial extraction commits should be behavior-preserving
and independently buildable.

### Information loss at legacy handlers

A wrapper around only the handler cannot reconstruct data discarded by the caller. Typed gateways
must land at invocation sites before claiming target, damage, critical-state, or lifecycle safety.

### Hidden binding precedence

Changing boot order or preferring world data immediately can remove shop, quest, or legacy behavior.
Record the effective post-boot chain and preserve current precedence until each collision is
intentionally migrated.

### Independent activation flags

Assigning a function pointer does not set `MOB_SPEC` or `ITEM_AUTOPROC`. Registry and OLC metadata
must expose these prerequisites; automatically changing flags would be a separate content migration.

### Shared moving-room callback slot

Room `M` and `Z` records currently target the same function pointer but require incompatible `me`
payloads. OLC and loading must reject that combination until moving-room relocation has a separate
typed hook; composition alone cannot make the callbacks compatible.

### Unknown-name data loss

Logging an unknown persisted name is insufficient if OLC later saves a null function and erases the
raw text. Preserve unresolved bindings or prevent an implicit overwrite.

### Lifetime violations

Objects can be extracted and freed during a callback. Callers must cache iteration state and follow
event-specific invalidation contracts rather than testing the old pointer after return.

### Over-generalizing artifact mechanics

Artifact rules may look broadly reusable while relying on artifact levels, ownership, or
persistence. Only extract a helper when its contract can be stated without artifact-specific data.

### Replacing one monolith with several dumping grounds

Entity files should not absorb code whose primary job belongs to vessels, shops, crafting, magic, or
another established subsystem. File membership should follow primary responsibility.

### Ambiguous cooldown units

Legacy object timers and real-time artifact timestamps use different models. APIs must name units
and storage, persistence, and reboot behavior explicitly.

### Affect source and stacking collisions

The general affect system already uses `source_id`, while artifact and other spell code use
`specific` for subsystem-specific tags. A new helper needs explicit namespaces and range validation
so one subsystem cannot remove or suppress another's effects.

### Unbounded event infrastructure

A generic world event bus should not be created without concrete consumers. Direct, typed lifecycle
hooks may be easier to trace and safer to maintain.

### Persistence compatibility

Stable names in mob, object, and room files are content-facing identifiers. Renaming or removing
them requires aliases, migration, or clear boot failure. Multiple-handler support would require a
versioned format change.

### Repository integration drift

Source splits must update both build manifests, retain path-qualified includes from outside
`src/spec/`, and avoid modifying local configuration headers. Documentation-only planning must not
be mistaken for permission to change world data or production configuration.

## Recommended First Implementation Slice

The first implementation should be a registry-safety and observability slice, deliberately smaller
than the physical reorganization:

1. Add characterization tests for current registry lookup, aliases, bounds, OLC selection, and
   known/unknown world-data handling.
2. Separate canonical name, display label, explicit aliases, owner mask, per-event contracts,
   binding visibility, description, and category in the existing registry.
3. Add pre-world-load metadata validation and bounds-safe, type-aware accessors.
4. Wire descriptions and prerequisites into owner-filtered medit, oedit, and redit views.
5. Report unknown or incompatible names with owner type and VNUM, preserve authored binding
   provenance for writers, and either retain unresolved raw text or prevent OLC from silently
   overwriting it.
6. Reject an `M`/`Z` room combination while moving-room relocation still shares the callback slot.
7. Add a startup diagnostic that exposes effective binding source and current collision outcomes
   without changing precedence.
8. Update `docs/guides/OLC_SpecProcs.md` and `lib/text/help/help.hlp`.

This slice must not move handlers, alter callback dispatch, set prototype flags automatically,
change world-file syntax, add handler chains, or migrate bindings. It creates a safe control plane
while retaining the single-handler storage model.

The second slice should add call-site gateways for every existing invocation path. Only after those
gateways and characterization tests are in place should a third slice extract the general object
procedures. Common eligibility or cooldown helpers should be introduced from audited consumers, not
bundled into the file move.

## Core Refactor Acceptance Criteria

The core refactor, excluding optional composition and zone/world events, is complete when:

- Every current invocation path uses an event-specific gateway with tested legacy translation.
- Command-owner order, return handling, activation flags, and boot precedence remain unchanged
  unless a separately documented migration intentionally changes them.
- Every persisted definition has one canonical identity, explicit aliases, valid owner and per-event
  metadata, binding visibility, and a builder-visible description.
- Unknown or incompatible world bindings are diagnosable and cannot be erased by an unrelated OLC
  save.
- OLC writers preserve authored binding provenance instead of serializing a boot-time override by
  reverse function-pointer lookup.
- The effective binding and its source can be inspected for any mob, object, or room prototype.
- Typed handlers receive complete event payloads and have explicit flow and pointer-lifetime
  contracts.
- Any shared cooldown, affect, targeting, or combat helper states its clock, ownership,
  persistence, and invalidation rules and is backed by multiple real consumers.
- File organization follows primary subsystem ownership, and both build systems contain the same
  production and test sources.
- Root production-linked tests pass, `make install` leaves the tested binary at `bin/circle`, and
  the builder, help, system, and architecture documentation matches the implemented phase.

## Conclusion

The codebase should be reorganized before a large new wave of custom procedures is added. The useful
boundary is not simply mobile versus object versus room. The durable boundary is:

- Event-specific invocation gateways and legacy translation.
- Immutable definitions and observable bindings.
- Narrow reusable mechanics.
- Reusable entity behavior.
- Cohesive feature or zone content.
- Separate artifact-specific state and progression.
- Explicit future zone and world lifecycle events.

The artifact subsystem provides a strong example of data-driven definitions, centralized validation,
explicit core hooks, cooldown discipline, stacking policy, persistence, and testability. Those
patterns should inform the general system without making every special procedure an artifact
procedure or obscuring the current behavior that compatibility code must preserve.
