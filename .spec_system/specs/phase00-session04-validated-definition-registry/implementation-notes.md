# Implementation Notes

**Session ID**: `phase00-session04-validated-definition-registry`
**Started**: 2026-08-06
**Base Commit**: e30cbb33eccb7a7546f1335003e2573183bd11d3
**Status**: Complete

---

## Planning And Prerequisites

- `check-prereqs.sh` passed for the development checkout, spec system, jq, and git.
- Sessions 01, 02, and 03 are complete; `.spec_system/state.json` had no active session.
- The worktree was clean on `master` at the Session 04 base commit.
- Session 03 left all 509 production-linked tests passing.
- `lib/.env` identifies this checkout as `APP_ENV=development`; no credential value was emitted.

## Frozen Compatibility Constraints

- The current sentinel table exposes 29 indexed names in a fixed order and maps them to 28 distinct
  canonical definitions.
- `Guild` and `Guildmaster` both resolve to `guild`; reverse function lookup returns the first row,
  `Guild`. The new representation must make that relationship explicit without changing the legacy
  indexed view.
- Existing accessors are case-insensitive but only guard negative indexes; Session 01 already
  expects the first out-of-range index to return null, and this session extends the guarantee to all
  arbitrarily high indexes.
- The legacy table stores empty descriptions and no ownership, event, prerequisite, binding, or
  visibility data. Every canonical row therefore requires traced metadata before migration.
- `boot_world()` begins by connecting to MySQL and then parses zones, triggers, rooms, mobiles,
  objects, and shops. Registry validation must occur in `boot_db()` before that call.

## Behavioral Quality Focus

- Keep the compatibility projection independent from canonical iteration semantics.
- Validate explicit counts before dereferencing any caller-provided definition or event array.
- Use one-hot event entries and verify each event has at least one compatible declared owner.
- Keep all strings static and immutable; do not add allocation or teardown to the registry.

## Definition Metadata Trace

The Luminari build path, checked-in `Z` records, handler bodies, and the invocation matrix from
Sessions 02 and 03 establish the following canonical contracts. Unsupported campaign-only
assignments do not broaden Luminari owner masks.

| Definition | Owners | Events | Special prerequisites | Binding sources |
|------------|--------|--------|-----------------------|-----------------|
| Bank | mobile, object | command, item identify | none | world, legacy assignment |
| Bazaar | room | command | none | world, legacy assignment |
| Bounty Missions | mobile | command | none | world, legacy assignment |
| Bulk Identify | mobile | command | none | world, legacy assignment |
| Buy Armor | mobile | command | none | world, legacy assignment |
| Buy Weapons | mobile | command | none | world, legacy assignment |
| Crafting Kit | object | command, item identify | carried for commands | world, legacy assignment |
| Crafting Quest | room | command | none | world, legacy assignment |
| Cryogenicist | mobile | command | none | world, legacy assignment |
| Dump | room | command | none | world, legacy assignment |
| Guild Guard | mobile | command | none | world, legacy assignment |
| Guild | mobile | command | none | world, legacy assignment |
| Hunts Master | mobile | command | none | world, legacy assignment |
| Identify Mob | mobile | command | none | world, legacy assignment |
| Janitor | mobile | mobile activity | `MOB_SPEC` | world, legacy assignment |
| New Supply Orders | mobile | command | none | world |
| Pet Object | object | object auto-pulse, item identify | `ITEM_AUTOPROC` and carried for pulse | world, legacy assignment |
| Pet Shop | room | command | following room contains pets | world, legacy assignment |
| Player Shop | mobile | command | configured player shop | world, legacy assignment |
| Postmaster | mobile | command | none | world, legacy assignment |
| Practice Dummy | mobile | mobile activity, mobile combat turn | `MOB_SPEC`; combat state for turn | world, legacy assignment |
| Questmaster | mobile | command | quest table entry | world, quest wrapper |
| Receptionist | mobile | command | none | world, legacy assignment |
| Temple Healer | mobile | command | none | world |
| Vampire Cloak | object | command, item identify | equipped for commands | world, legacy assignment |
| Wizard Library | room | command | none | world, legacy assignment |
| Greyhawk Ship | object | command | linked vessel state | world |
| Greyhawk Ship Commands | room | command | ship control room | world, legacy assignment |

The ten event constants cover the typed event concepts named by the PRD. Shop and quest secondary
composition forwards an incoming command context unchanged, so it remains binding/composition
metadata rather than adding two duplicate event values.

## Implementation Log

- Created the Session 04 specification and 24-task checklist.
- Confirmed the development environment, clean base, completed prerequisites, exact legacy
  inventory, and pre-world boot boundary.
- Retraced all registered handlers and Luminari assignments before populating metadata.
- Added `src/spec/spec_registry.h` and `src/spec/spec_registry.c` with 28 immutable canonical
  definitions, ten event types, explicit prerequisites, owner and binding masks, visibility,
  descriptions, categories, and one `Guildmaster` alias.
- Preserved the complete 29-name compatibility projection and moved the legacy accessor bodies out
  of `src/spec_assign.c`; the assignment implementation itself is unchanged.
- Added reusable definition validation and fatal production validation in `boot_db()` before
  `boot_world()` can initialize MySQL or parse world files.
- Added canonical, owner-aware, event-aware, binding-aware, alias, handler, and diagnostic APIs with
  exact one-bit query and signed-index guards.
- Added 13 production-linked tests covering the exact definition inventory, handler identities,
  complete masks, prerequisites, aliases, shared handlers, malformed tables, extreme bounds, and
  boot ordering.
- Synchronized the production and test sources in both build manifests.
- Documented the registry API and corrected the existing OLC guide's registry location while
  stating that owner-aware menu filtering remains pending Session 05.
- Focused compilation completed with GNU C23 `-Wall -Wextra` and no warning; all 522 CuTests passed.
- Completed the full Session 04 review. Replaced positional compatibility indexes with named,
  count-checked indexes; corrected the final stale registry-maintenance comments; and resolved
  changed-code analyzer false paths without changing runtime behavior.
- Rebuilt and reran the final source through both Autotools and an independent CMake tree; all 522
  CuTests and CMake `production-cutest` passed with no open review finding.
- Full validation passed: `make test`, `make install`, independent CMake/CTest, formatting, static
  analysis, encoding, manifest parity, protected-path checks, security review, world-data digest,
  and artifact hygiene.
- Installed release `84fca6e13ceb98d311fcbd43b3a6a81ca9c1a6ac`; root `circle` is absent and
  the world digest remains `28d30cda73e9dd19e6ea1bf85260aefba0f621d6144401eda421a9fca2be2d98`.

**Next task**: Session 05 planning - Owner-Aware OLC.
