# Session Specification

**Session ID**: `phase00-session05-owner-aware-olc`
**Phase**: 00 - Registry Safety and Observability
**Status**: Complete
**Created**: 2026-08-07
**Base Commit**: 03a356db5a16e9c1c6fce6510d79b05a1f9fcb4e

---

## 1. Session Overview

This session changes the mobile, object, and room SpecProc selectors from one shared 29-name legacy
view to stable owner-filtered views over the validated canonical registry. Builders see only
visible, world-bindable legacy definitions compatible with the edited prototype, together with the
definition category, description, supported events, and per-event prototype or placement
prerequisites.

The persisted identities, legacy compatibility accessors, world-file syntax, callback slots, and
explicit clear behavior remain unchanged. Selection never mutates `MOB_SPEC`, `ITEM_AUTOPROC`, or
other prototype flags.

---

## 2. Objectives

1. Give medit, oedit, and redit deterministic owner-specific canonical selection views.
2. Centralize filtering, numeric parsing, selection mapping, and metadata rendering.
3. Explain when every visible procedure can run before a builder selects it.
4. Reject malformed, out-of-range, and unsupported-owner selections without changing editor state.
5. Prove all three production editor paths, bounds, clear paths, and activation-flag neutrality.

---

## 3. Prerequisites

- Session 04 is complete and provides validated immutable owner, event, visibility, binding,
  description, category, and prerequisite metadata.
- Session 01 provides the production-linked OLC selection, quit, clear, and persistence baseline.
- Development checkout and both build systems are available; protected configuration and world
  data remain unchanged.

---

## 4. Scope

### In Scope (MVP)

- A shared OLC SpecProc menu module under `src/olc/`.
- Filtering by exactly one owner bit, builder visibility, world binding permission, and current
  legacy-handler compatibility.
- Stable canonical-order numbering within each filtered view.
- Strict bounded decimal selection parsing with explicit invalid, clear, and definition outcomes.
- Shared display of display name, category, description, event names, prototype flags, and
  placement requirements.
- Integration in medit, oedit, and redit with no duplicated mapping logic.
- Production-linked exact inventory, display, selection, invalid-input, clear, and flag-neutrality
  tests.
- Builder guide updates for the delivered filtered menu.

### Out Of Scope (Deferred)

- Automatic mutation of `MOB_SPEC`, `ITEM_AUTOPROC`, or placement state.
- Raw authored-name ownership, unresolved-name preservation, and writer provenance (Sessions 06-07).
- Load-time incompatible-name diagnostics and effective collision reporting (Sessions 06-08).
- Multiple callbacks, typed handlers, dispatch gateways, or world-file syntax changes.
- Final replacement of the legacy in-game `SPECIALS` entry, which is coordinated in Session 09.

---

## 5. Technical Approach

### Shared Menu Contract

Create `src/olc/spec_menu.h` and `src/olc/spec_menu.c`. The module provides:

- a filtered count for exactly one owner type;
- a bounds-safe canonical definition lookup by zero-based filtered index;
- strict parsing of a one-based builder choice into invalid, clear, or definition outcomes; and
- one renderer used by all three editors.

A selectable definition must be builder-visible, support the requested owner, allow world binding,
and expose a legacy handler because the current OLC prototype slots still store `SPECIAL` function
pointers. Canonical registry order defines filtered numbering. Aliases remain accepted by world
loaders but do not create duplicate menu rows.

The expected production views are:

| Owner | Count | Definitions in order |
|-------|-------|----------------------|
| Mobile | 18 | Bank; Bounty Missions; Bulk Identify; Buy Armor; Buy Weapons; Cryogenicist; Guild Guard; Guild; Hunts Master; Identify Mob; Janitor; New Supply Orders; Player Shop; Postmaster; Practice Dummy; Questmaster; Receptionist; Temple Healer |
| Object | 5 | Bank; Crafting Kit; Pet Object; Vampire Cloak; Greyhawk Ship |
| Room | 6 | Bazaar; Crafting Quest; Dump; Pet Shop; Wizard Library; Greyhawk Ship Commands |

### Rendering Contract

Each row shows its one-based number, `display_name`, category, non-empty description, and every
event. Each event states either no prerequisites or its required prototype flags and placements.
Diagnostic labels use `MOB_SPEC`, `ITEM_AUTOPROC`, `carried`, `equipped`, `combat`, `mounted`, and
`moving room` exactly. An empty or unsupported owner view prints an explicit no-compatible-entry
message and still presents the clear/quit prompt.

### Editor Integration

Each editor opens the same renderer with its constant owner type and parses with the shared helper.
Clear sets only its existing OLC callback slot to null. A valid definition copies only its legacy
handler. Invalid syntax, overflow, bounds, or owner context emits `Invalid selection` and preserves
the previous handler and `OLC_VAL`. Quit returns to the existing main menu unchanged.

---

## 6. Deliverables

### Files To Create

| File | Purpose | Est. Lines |
|------|---------|------------|
| `src/olc/spec_menu.h` | Shared filtered menu and selection API | ~65 |
| `src/olc/spec_menu.c` | Filter, parser, mapping, and renderer | ~250 |
| `unittests/CuTest/test_spec_owner_aware_olc.c` | Exact helper and three-editor production coverage | ~500 |

### Files To Modify

| File | Change |
|------|--------|
| `src/olc/medit.c` | Use the mobile filtered menu and mapping. |
| `src/olc/oedit.c` | Use the object filtered menu and mapping. |
| `src/olc/redit.c` | Use the room filtered menu and mapping. |
| `unittests/CuTest/test_spec_fixtures.c/.h` | Add bounded menu-opening and activation-state support. |
| `unittests/CuTest/test_spec_registry_persistence.c` | Rebase deliberate selection numbers on filtered views. |
| `Makefile.am`, `CMakeLists.txt` | Add synchronized production and test membership. |
| `docs/guides/OLC_SpecProcs.md` | Document filtering, metadata, numbering, and prerequisites. |

---

## 7. Success Criteria

### Functional Requirements

- [x] All three editors list exactly their compatible, visible, world-bindable canonical definitions.
- [x] Displayed rows explain category, behavior, events, flags, placement, and combat requirements.
- [x] Filtered numbering maps every valid selection to the intended handler.
- [x] Clear and quit retain their established semantics.
- [x] Invalid input or unsupported owner context changes neither handler nor dirty state.

### Testing Requirements

- [x] Exact mobile, object, and room inventories and filtered bounds are production-linked.
- [x] Menu output proves representative descriptions and all current prerequisite labels.
- [x] All three production parser paths cover select, clear, quit, malformed, and high bounds.
- [x] Selection of procedures with runtime prerequisites does not mutate activation flags.
- [x] Root `make test`, CMake `production-cutest`, and `make install` pass.

### Non-Functional Requirements

- [x] Legacy indexed APIs, persisted names, callback ABI, world syntax, and writers remain compatible.
- [x] Shared code owns filtering and mapping; editor files supply only their owner and callback slot.
- [x] New production and test sources are synchronized across Automake and CMake.

### Quality Gates

- [x] All changed text is ASCII-compatible UTF-8 with LF endings.
- [x] GNU C23 formatting, compiler, and changed-code static checks pass without a new warning.
- [x] No protected configuration, credential, or world-data file changes.

---

## 8. Behavioral Quality Focus

Checklist active: Yes

- Inputs: empty, whitespace, signed, nonnumeric, partially numeric, huge, low, and high choices.
- State: previous callback and dirty bit stay unchanged on every rejected input and quit.
- Mapping: filtered counts and positions are derived from canonical definitions, never legacy alias
  indexes.
- Side effects: selecting Janitor, Practice Dummy, or Pet Object never sets scheduling flags.
- Output: all three owners receive readable metadata without overflowing the descriptor buffer.
- Cleanup: fixture globals, protocol state, output buffers, and sandbox data are restored before
  assertions.

---

## 9. Testing Strategy

- Pure helper tests assert exact filtered inventories, all returned definition constraints, signed
  bounds, invalid owners, and parser outcomes.
- Production editor scenarios enter each main menu through `Z`, inspect captured output, then drive
  the real medit/oedit/redit parser modes for valid, clear, quit, malformed, and boundary inputs.
- Activation tests snapshot the loaded prototype flags before selecting prerequisite-bearing
  procedures and compare them afterward.
- Session 01 compatibility tests retain the legacy 29-name accessor checks while updating only the
  deliberately changed editor-view positions.
- Final gates cover both generators, all root tests, installation, formatting, static analysis,
  manifest parity, encoding, world digest, and artifact hygiene.
