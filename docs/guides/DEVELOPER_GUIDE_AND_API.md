# LuminariMUD Developer Guide and API Reference

Use [docs/development.md](../development.md) for the verified build, test,
source-map, and style entry point. This reference documents the special-procedure
control plane delivered through Phase 02; other subsystem APIs remain in their
source-linked system documents.

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

Phases 00-02 preserve the single callback slot, `SPECIAL` ABI, world grammar,
command traversal, heartbeat timing, caller-specific returns, activation flags,
shop/quest nesting, and boot precedence. Declarative validation applies to the
two currently eligible Luminari rows; unsupported assignments remain on the
observable compatibility path. Content extraction, shared mechanics,
typed-handler conversion, and general chains remain future work.

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
and [architecture](../ARCHITECTURE.md).
