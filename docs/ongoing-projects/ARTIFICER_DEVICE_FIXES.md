# Artificer Device Fixes (work item 4)

Status: in progress. Work item: https://gitlab.com/max757/Luminari-Source/-/work_items/4.
Branch `4-artificer-device-add-rejects-3rd-and-4th-circle-spells`, worktree
`../Luminari-Source-work-item-4`, based on master `b7152a7d9`. The work item text lists every
defect with file and line; item numbers below are the work item's.

## Rules for this work

- "Spell level" in the device code is a class level (`spell_info[].min_level[]`, 1-7 for
  circles 1-4); a circle is `(level + 1) / 2`. `max_spell_level` in `do_device()` is a circle.
- A device spell's level is the lower of its wizard and cleric levels; `device create` may fall
  back to the higher one when the lower circle is full. The chosen level is stored in
  `inv->spell_levels[]` and is what circle accounting reads.
- `weird_science_table` (`src/core/constants.c`) holds spell slots per circle for artificer
  levels 1-20, one row per level.
- Tests go in `unittests/CuTest/test_artificer_devices.c` (new; listed in `Makefile.am` twice
  and `CMakeLists.txt`). Run with `CUTEST_FILTER=artificer ./cutest` from the repo root.

## Plan (commit per step)

01. Headline bug + shared helpers (`src/act/act.other.c`). One helper for a spell's device
    level (0 for skills and spells on neither list: item 7), one for the circle limits (direct
    table index clamped to 20: item 25), one for circles in use, one that assigns circles to a
    set of new spells trying every lower/higher combination (item 14). `device add` uses them:
    circle cap, per-circle budget, stores `spell_levels[]`. Drop the unused
    `device_count_by_level`.
02. `device create`/`list`/`info`/`destroy`/`cooldown`: remove the dead per-device cooldown
    readers (items 15, 16, 19), build time from the chosen level (18), reject extra spells
    (20), destroy warning 20 minutes (21).
03. `device use`: stop on `call_magic() < 0` or target out of the room (3); failed and broken
    attempts use the standard action (17); explosion damage from device circles (13) and death
    handling (12). Remove the unreachable second `total < dc` branch.
04. Persistence (2): `dc_penalty` and `broken` appended to the numbers line of each device, so
    line counts (and the rename scanner) are unchanged; old files load as 0.
    `find_skill_num()`/`find_ability_num()` use `any_one_arg_c()` (1).
05. Metamagic Science (4, 5): wand/staff objects spend `1 + calculate_metamagic_charge_cost()`
    charges; UMD failure is `skill_check() == 0`; DC from the spell circle for objects and
    stored potions/scrolls.
06. `device create cancel` / `device repair cancel` pass the busy check (6).
07. Craft rolls (8): Elbow Grease and Jack of All Trades bonuses in one helper used by
    `compute_ability()` and by the craft roll sites; rank bookkeeping keeps raw ranks.
08. Starting gear (9) with the alchemist kit; premade build (22) buys Magical Aptitude,
    Improved Initiative (human) and Empower Spell instead of the free craft feats.
09. Design items, decided by the user 2026-09-27: Construct Stone Golem moves to artificer 15
    and Construct Iron Golem to 20 (10); Brilliance and Blunder becomes a Gnome racial next to
    Gnomish Tinkering, existing gnomes get it only on respec (11); Artificer Item Creation lets an
    artificer brew wizard/cleric spells up to its device circle cap, brewing's alchemy check being
    the emulation roll (23).
10. Help (24): CLASS-ARTIFICER entry and a real DEVICE entry, in `help.hlp` and the dev help DB.

## Decisions

- `device add` applies the same circle budget as `device create` (work item asked to decide).
- Per-device cooldown (15) had no writer and no spec; its readers are removed rather than a
  cooldown invented. The saved field stays in the file format.

## Progress

- Steps 1-4: commit `b5f07a3e8`. 8 of the first 10 tests fail on the old code; the other two
  (overlong spell word, level 25 table row) are memory-safety cases ASan catches.
- Item 3 (device use after target dies) has no unit test: it needs a real kill, which the
  CuTest fixtures cannot do cheaply. Covered by the live check.
- Steps 5-6: commit `54a3e42d8`; steps 7-8: `4313adaf8`; step 9: `e4946d92e`.
- Step 10: help written with the help-sync engine's delta writer (plan id
  `work-item-4-artificer-help`) to the dev help DB, then `help.hlp` rendered from it; the
  snapshot verified `file_matches`. New `class-artificer`; `devices` and `gnomes` rewritten;
  `class-roster` lost the two artificer keywords. Production help is not synced (no request).
- Item 9 (starting gear) has no unit test (object prototypes are not loaded in CuTest); live
  check.
- [ ] Full `make test`, live check on 4100, push, MR opened with `Closes #4`.
