# Implementation Notes

**Session ID**: `phase00-session05-owner-aware-olc`
**Started**: 2026-08-07
**Base Commit**: 03a356db5a16e9c1c6fce6510d79b05a1f9fcb4e
**Status**: Complete

## Planning And Prerequisites

- Apex prerequisite checks passed and `lib/.env` confirms `APP_ENV=development` without exposing a
  credential value.
- Session 04 is complete and published; the worktree was clean at the recorded base commit.
- Session 01 keeps the 29-name legacy accessor inventory frozen. Session 05 changes only the three
  editor-specific presentation and selection mappings.
- Medit, oedit, and redit currently duplicate the same global list renderer and `atoi` parser. All
  three write only their existing OLC-time callback slot and use `0` to clear and `Q` to quit.

## Filtered Inventory Trace

| Owner | Count | Canonical filtered order |
|-------|-------|--------------------------|
| Mobile | 18 | Bank; Bounty Missions; Bulk Identify; Buy Armor; Buy Weapons; Cryogenicist; Guild Guard; Guild; Hunts Master; Identify Mob; Janitor; New Supply Orders; Player Shop; Postmaster; Practice Dummy; Questmaster; Receptionist; Temple Healer |
| Object | 5 | Bank; Crafting Kit; Pet Object; Vampire Cloak; Greyhawk Ship |
| Room | 6 | Bazaar; Crafting Quest; Dump; Pet Shop; Wizard Library; Greyhawk Ship Commands |

All current definitions are builder-visible and allow world binding. The owner mask therefore
drives current membership; the visibility and binding checks remain explicit so later metadata
changes cannot bypass builder policy. `Guildmaster` remains a loader alias and is intentionally not
a second canonical menu row.

## Prerequisite Rendering Trace

- Janitor: mobile activity requires `MOB_SPEC`.
- Practice Dummy: mobile activity requires `MOB_SPEC`; combat turn additionally requires combat
  placement.
- Pet Object: object auto-pulse requires `ITEM_AUTOPROC` and carried placement.
- Crafting Kit: command requires carried placement.
- Vampire Cloak: command requires equipped placement.
- All other currently displayed event contracts have no prototype or placement prerequisite.

The shared renderer will state prerequisites per event so multi-event definitions do not imply that
one event's activation rule applies to another.

## Compatibility And Deferred Scope

- Existing writers continue reverse lookup through canonical handler identity and keep their current
  mobile `SpecProc:` and object/room `Z` syntax.
- No editor sets scheduling or placement flags automatically.
- Raw unresolved identities and authored binding ownership are introduced in Sessions 06-07.
- Final in-game `SPECIALS` help replacement is coordinated with the complete Phase 00 behavior in
  Session 09; the focused builder guide is updated in this session.

## Implementation Log

- Created the bounded 23-task Session 05 specification after tracing all three editor paths and the
  complete validated registry.
- Added `src/olc/spec_menu.h` and `src/olc/spec_menu.c` as the single owner-aware filtering,
  bounds-safe mapping, strict decimal parsing, and builder metadata presentation surface.
- Integrated the mobile, object, and room views into medit, oedit, and redit without changing their
  callback slots, explicit clear behavior, quit behavior, dirty-state rules, or activation flags.
- Added seven production-linked tests covering exact inventories, signed bounds, malformed and
  empty input, unsupported owners, representative full metadata, all valid editor selections,
  clear and quit paths, and prerequisite-bearing flag neutrality.
- Moved the existing fork-isolated parser scenario runner into the shared fixture module. The
  legacy parser loaders retain process-global counters, so each parser-backed scenario now receives
  one private child process and one private sandbox.
- Rebased the Session 01 deliberate editor selections on the filtered mobile, object, and room
  positions while retaining the exact 29-name legacy accessor characterization.
- Synchronized Automake and CMake membership and updated the builder guide with filtered counts,
  canonical numbering, aliases, prerequisites, and deliberate non-mutation of scheduling flags.
- Formal review repaired the non-reentrant fixture use, replaced a cloned fixture switch with a
  bounded owner-to-mode table, added explicit empty and whitespace checks, and added defensive
  pointer guards and the direct standard-library include.
- Focused Autotools compilation and all 529 production-linked tests pass. An independent GNU C23
  CMake build and `production-cutest` also pass; formatting, manifest parity, changed-code static
  analysis, encoding, protected-path, and diff-hygiene checks are clean.
- Full validation passed all seven auxiliary checks and 529 CuTests, repeated the independent CMake
  test, and installed release `7afe336cad160a369c676549bfb6daba634bae27`.
- The root `circle` artifact is absent, the installed alias is executable, protected configuration
  and credentials are unchanged, and the world digest remains
  `28d30cda73e9dd19e6ea1bf85260aefba0f621d6144401eda421a9fca2be2d98`.

**Next task**: Session 06 planning - Authored Binding Model.
