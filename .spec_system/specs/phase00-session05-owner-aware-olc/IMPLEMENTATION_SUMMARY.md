# Implementation Summary

**Session ID**: `phase00-session05-owner-aware-olc`
**Completed**: 2026-08-07

## Overview

Replaced the shared unfiltered legacy SpecProc picker with one canonical owner-aware menu used by
medit, oedit, and redit. Builders now see only compatible world-bindable definitions and can read
each procedure's category, description, events, and runtime prerequisites before selection.

## Deliverables

### Files Created

| File | Purpose | Lines |
|------|---------|-------|
| `src/olc/spec_menu.h` | Shared filtered menu and selection contract | 42 |
| `src/olc/spec_menu.c` | Filtering, strict parsing, mapping, and renderer | 245 |
| `unittests/CuTest/test_spec_owner_aware_olc.c` | Exact helper and three-editor production coverage | 543 |

### Principal Files Modified

| File | Change |
|------|--------|
| `src/olc/medit.c`, `oedit.c`, `redit.c` | Use owner-specific shared display and selection mapping. |
| `unittests/CuTest/test_spec_fixtures.c/.h` | Share parser isolation and expose menu/activation observations. |
| `unittests/CuTest/test_spec_registry_persistence.c` | Rebase deliberate editor selections on filtered numbering. |
| `Makefile.am`, `CMakeLists.txt` | Add synchronized production and test membership. |
| `docs/guides/OLC_SpecProcs.md` | Document filtering, canonical numbering, aliases, and prerequisites. |

## Technical Decisions

1. Canonical registry order defines stable numbering independently for each owner view.
2. Visibility, world-binding permission, owner support, and a legacy handler are all required for
   selection while the current editor callback slots remain untyped.
3. Prerequisites are rendered per event and remain descriptive; selection never mutates prototype
   flags or placement.
4. Strict parsing separates invalid, explicit clear, and definition outcomes and clears its output
   pointer before every attempted parse.
5. Parser-backed tests run in child processes because the legacy world parsers retain global load
   counters that are not safely reusable within one process.

## Compatibility Baseline

- The legacy indexed accessor remains 29 names and `Guildmaster` remains a loadable alias.
- World-file syntax and canonical persisted identities are unchanged.
- Medit, oedit, and redit still store one legacy callback and retain established save behavior.
- Explicit clear, quit, dirty-state, dispatch order, `MOB_SPEC`, and `ITEM_AUTOPROC` behavior are
  unchanged.

## Test Results

| Metric | Value |
|--------|-------|
| Session tests added | 7 |
| Total CuTests | 529 |
| Passed | 529 |
| Root auxiliary checks | 7/7 passed |
| CMake production CTest | Passed |
| Review findings | 1 Medium and 2 Low, all resolved |

`make test`, `make install`, independent CMake/CTest, formatting, changed-code static analysis,
encoding, security, world-data integrity, and artifact hygiene all passed.

## Future Considerations

1. Session 06 can attach owned authored identities and provenance without changing the filtered
   presentation contract.
2. Session 07 can preserve unresolved authored text and use canonical names only after explicit
   replacement.
3. Session 09 can expose the completed control-plane behavior through the in-game `SPECIALS` help.

## Session Statistics

- **Tasks**: 23 completed
- **Production files created**: 2
- **Test files created**: 1
- **Tests added**: 7
- **Filtered views**: 18 mobile, 5 object, 6 room
- **Blockers**: None
