# Special Procedure Phase 06 Validation

**Status:** Complete (2026-08-07)

## Scope

Phase 06 audited concrete demand for two optional abstractions:

1. multiple special procedures persisted on one prototype; and
2. new C-level zone or world lifecycle callbacks.

Neither abstraction has a second real consumer or an approved content requirement. Phase 06
therefore closes them intentionally unimplemented. It retains and documents the one existing
composition contract: the runtime-only quest-over-shop-over-original mobile callback nesting.

No world grammar, database schema, persistence version, callback slot, registry event, OLC surface,
or production dispatch behavior changes in this phase. One production-linked regression extends the
compatibility evidence to a registered typed handler. Database-first `SPECIALS` help now states
explicitly that effective history is diagnostic rather than a persisted dispatch chain.

## Requirement Decisions

| Phase 06 requirement | Evidence | Decision |
|----------------------|----------|----------|
| Gather prototype-composition use cases | `room_data` and `index_data` each own one callback and one authored binding. Mobile `SpecProc`, object `Z`, and room `Z` each load and write one name. `SHOP_FUNC` and `QST_FUNC` are runtime-only saved callback fields. | The only concrete composition is the established shop/quest wrapper nesting. No persisted general chain is justified. |
| Design inner-chain order without changing outer traversal | Existing boot order installs original assignments, then shop wrappers, then quest wrappers. Each wrapper invokes its saved secondary first; nonzero consumes and zero falls through. | Preserve `questmaster -> shop_keeper -> original` exactly. Do not add an inner-chain dispatcher or alter outer command-owner traversal. |
| Migrate shop and quest secondaries deliberately | `assign_the_shopkeepers()` and `assign_the_quests()` save before replacing the prototype callback. Gateways preserve actor, owner, command, and argument. Existing and new CuTest coverage exercises stop and fallthrough. | Retain the explicit compatibility wrappers. A general-chain migration would add policy and persistence without another consumer. |
| Add only required zone/world hooks | DG Scripts already own room reset, enter, leave, login, time, and related localized events. Artifact and vessel boot/reset needs call direct owning-subsystem APIs. No approved content requests a new C hook. | Add no special-procedure lifecycle event, registry, or asynchronous bus. |
| Version affected persistence with backward loading | No persistence contract changes. The single-name mobile, object, and room formats remain authoritative. Shop and quest secondary pointers are reconstructed at boot and are never serialized as chains. | No version bump is applicable. A future multi-name format must be versioned and backward-compatible before it can ship. |

## Composition Contract

The current boot sequence is deliberate:

1. `assign_mobiles()` installs the original direct callback.
2. `assign_the_shopkeepers()` saves that callback in `SHOP_FUNC` and installs `shop_keeper`.
3. `assign_the_quests()` saves the then-current callback in `QST_FUNC` and installs `questmaster`.

The resulting runtime call order for an overlapping mobile is:

```text
questmaster
  -> shop_keeper
       -> original callback
       -> shop behavior after original returns zero
  -> quest behavior after shop returns zero
```

The wrappers forward the same actor, mobile owner, command number, and argument pointer through
`spec_gateway_quest_secondary()` and `spec_gateway_shop_secondary()`. A nonzero secondary result
propagates as command consumed. `no_specials` controls boot assignment; changing it after boot does
not disable an already installed wrapper.

This contract is not a prototype handler chain:

- `struct index_data` and `struct room_data` still have one `func` callback slot;
- the authored `spec_binding` still represents one persisted identity;
- shop and quest structs hold one runtime-only saved function pointer each;
- effective binding history observes contributions but does not execute them; and
- OLC has no multiple-selection, reorder, duplicate, or chain-save operation.

## Persistence Boundary

The audit traced all three authored formats and their writers:

| Owner | Loader | Writer | Cardinality |
|-------|--------|--------|-------------|
| Mobile | `SpecProc:` in `src/db.c` | `src/olc/genmob.c` | Zero or one name |
| Object | `Z` in `src/db.c` | `src/olc/genobj.c` | Zero or one name |
| Room | `Z` in `src/db.c` | `src/olc/genwld.c` | Zero or one name |

Room moving data remains a separate `M` parser hook and is still mutually exclusive with a named
room `Z` binding. Phase 06 introduces no alternate record, delimiter, list field, schema table, or
implicit conversion. There is therefore no changed persistence to version or migrate.

## Lifecycle Audit

| Concrete lifecycle need | Existing owner |
|-------------------------|----------------|
| Room reset after zone commands | `WTRIG_RESET` via `reset_wtrigger()` in the zone reset owner |
| Room enter, leave, login, random, and time behavior | Existing DG world triggers |
| Mobile and object load, death, timer, movement, and time behavior | Existing DG mobile/object triggers |
| Artifact boot, save, and zone-load suppression | Direct APIs owned by `src/obj/spec_artifacts.c` |
| Vessel boot, persistence reconstruction, encounter setup, and post-reset relink | Direct APIs owned by `src/vessels/` |

DG Scripts remain the correct content surface for localized narrative and sequencing. Engine
subsystems with strong state or ordering guarantees already call their owner directly. Adding a
zone/world special-procedure callback would duplicate these paths without a shared typed contract.

A future C lifecycle hook must begin at its actual lifecycle owner, define order relative to reset
commands and DG Scripts, define veto and re-entry behavior, and state pointer lifetime. It may move
into a shared registry only after a second consumer demonstrates the same contract.

## Production-Linked Coverage

Existing `test_spec_combat_secondary.c` coverage proves:

- exact shop and quest secondary context forwarding;
- nonzero stop propagation and zero fallthrough;
- quest-over-shop-over-original nesting;
- save-before-install assignment order, boot order, and `no_specials` gates; and
- typed Bank dispatch through the complete quest/shop nesting.

`Test_spec_typed_bank_survives_quest_shop_compatibility_nesting` installs the registered Bank
adapter as the original shop secondary, wraps it with shop and quest behavior, issues `deposit all`,
and verifies the typed handler consumes the command and transfers the complete balance. This closes
the compatibility gap created when Bank became typed in Phase 05.

Additional evidence remains in:

- `Test_spec_dispatch_secondary_gateways_forward_context_unchanged`;
- `TestSpecEffectiveBindingProductionPrecedenceAndSecondaries`; and
- Phase 00 persistence and OLC tests proving single-name authored identity.

The new regression brings the root production-linked suite to 589 tests.

## Reopen Conditions

General composition remains out of scope until approved content needs more than the existing
wrapper nesting. Before implementation, it must specify deterministic persisted-ID order, binding
source and collision behavior, per-entry event compatibility, stop and invalidation semantics,
mutation during dispatch, duplicates and bounds, versioned loading, and complete OLC operations.

Zone/world lifecycle work remains out of scope until approved content cannot be expressed through
DG Scripts or a direct owner hook. A broad event bus is not an acceptable substitute for a traced
consumer contract.

## Verification Gates

Run from the repository root:

```sh
make clean
make -j"$(nproc)"
make test
make install
cmake -S . -B <fresh-build-directory> -DBUILD_TESTS=ON -DCMAKE_BUILD_TYPE=Debug
cmake --build <fresh-build-directory> --target circle cutest --parallel "$(nproc)"
ctest --test-dir <fresh-build-directory> --output-on-failure
```

The close also requires synchronized Autotools/CMake manifests, ASCII UTF-8 LF documentation,
the database-first `SPECIALS` migration/verifier gate, untouched local configuration and credential
files, `bin/circle` installed, and no root-level `circle` artifact.

## Verification Results

The 2026-08-07 close passed every required gate:

- clean Autotools production build with no compiler warnings;
- root production-linked CuTest suite: 589 tests passed;
- `make install`: `bin/circle` installed and the root-level `circle` artifact removed;
- fresh CMake Debug builds of `circle` and `cutest`;
- CTest: 12 of 12 tests passed;
- world documentation checker: zero findings;
- database-first `SPECIALS` help migration and verifier: four of four contracts passed;
- changed documentation and SQL: ASCII, UTF-8, and LF; and
- protected local configuration and credential files unchanged.
