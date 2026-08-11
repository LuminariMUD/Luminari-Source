# Realms of Luminari Conversion Worknotes

- Updated: 2026-08-11
- Environment: development
- Branch: `master`
- Current task: deterministic `wtool rol-inventory` source inventory

## Locked inventory contract

The command reads the RoL authoring root without modifying it. It follows the
selection behavior traced in `EXAMPLE/RealmsOfLuminari/src/build_areas.c`:

- `AREA` selects `zon`, `wld`, and `soc` inputs.
- `AREA.mobobj` selects `mob` and `obj` inputs.
- `SHOP` selects `shp` inputs.
- `QUEST` selects `qst` inputs.
- Only a column-zero `*` makes a source manifest line a comment.
- The first whitespace-delimited token is the basename; later columns are
  annotations and order remains significant in the manifest record.
- Missing selected companions are reported rather than invented.

The JSON form contains hashes, manifest source lines, file and package
classifications, zone header identities, and aggregate counts. The human form
is a stable compact summary of the same inventory.

## 2026-08-11 implementation checkpoint

Core implementation checkpoint `56af654d` was pushed to `origin/master`.
Documentation and verification checkpoint `380eb353` was also pushed to
`origin/master`.

Implemented:

- Added `scripts/world/wtool_lib/rol_inventory.py` and the `rol-inventory`
  command with `--source-root` plus the existing global `--json` switch.
- Added active, disabled, unlisted, missing-companion, and multi-zone
  classifications across all seven source kinds.
- Added source-located manifest errors for blank active lines, unsafe or
  non-ASCII basenames, overlong lines and names, and duplicate active entries.
- Added deterministic fixture coverage and a local ignored-corpus acceptance
  test.
- Added the new module, test, and fixtures to both `Makefile.am` and
  `CMakeLists.txt`.
- Bumped the command version to 0.3.0.
- Added permanent CLI, testing, documentation-index, utilities-index, and
  changelog coverage. Updated both conversion plans without claiming that the
  broader target inventory or grammar inventory is finished.

Verified evidence:

```text
Active zone scope: 252 files, 255 records
Excluded zone files: 30 disabled, 2 unlisted
Active basenames without .zon: 6
Active multi-zone files: 2
Included active companion-only files: 9
Python suite: 178 tests passed
```

The six missing-zone basenames are `foggy_woods2`, `god_items`,
`northern_highroad2`, `northern_highroad3`, `quest_1`, and `quest_2`. The
multi-zone inputs are `foggy_woods.zon` with records 900 and 901, and
`northern_highroad.zon` with records 902, 903, and 904.

Final verification evidence:

```text
make test-world-tools: passed
cmake --build build --target test-world-tools: passed
Autotools/CMake world-tool source lists: identical, 114 paths each
Documentation findings: 0 errors, 0 warnings, 0 info
Repeated human output SHA-256: 00d88a81519b1606742a95d2088119f128b7121f16a7df1e69a4a734afb36cd8
Repeated JSON output SHA-256: 39416e3fedf5b8794004b928748913cc6d2e35803726e000dd053626854c32bb
Source tree before/after SHA-256: 40c671dcb0e5144ea00b1ca4db66e338b094aa617a647cbd1125f12d0f2f1d8b
```

The malformed fixture returned status 2, no standard output, and explicit
diagnostics for unsafe `areas/AREA:2`, blank `areas/AREA:3`, and the non-column-zero
asterisk on `areas/AREA:4`. ASCII, LF, Python compilation, and `git diff --check`
checks passed.

## Next actions

The `rol-inventory` task is complete. The final committed-tree audit repeated
`make test-world-tools`, confirmed an 883,711-byte JSON payload was byte-identical
across two live-corpus runs, rechecked every locked count and identity, and confirmed
local `HEAD` equaled `origin/master` at `380eb353` before this handoff-only update.

Continue Phase 0 with the separate development-target inventory and baseline
   diagnostics; do not expand `rol-inventory` into that later deliverable.
