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

Implemented locally:

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
- Bumped the command version to 0.3.0; permanent documentation still needs its
  0.3.0 command section and acceptance record.

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

## Next actions

1. Complete the permanent CLI documentation, testing guide note, index wording,
   and changelog entry.
2. Audit the implementation and build-list synchronization.
3. Run `make test-world-tools`, deterministic full-corpus comparisons, ASCII/LF
   checks, and the completion-criteria audit.
4. Commit and push the documentation/verification checkpoint.
