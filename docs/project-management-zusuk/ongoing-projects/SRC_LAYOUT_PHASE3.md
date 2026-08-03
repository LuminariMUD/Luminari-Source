# src/ Directory Layout - Phase 3

Plan for one feature directory: `src/obj/`. No functional change at any step.

Phase 1 ([SRC_DIRECTORY_LAYOUT.md](SRC_DIRECTORY_LAYOUT.md)) dissolved the
`systems/` and `world/` nests into eleven directories, leaving 181 files flat.
Phase 2 ([SRC_LAYOUT_PHASE2.md](SRC_LAYOUT_PHASE2.md)) added `character/`,
`quest/`, and `comms/`, taking flat from 181 to 134 across fourteen
directories.

This phase takes flat from 134 to **120**, in **fifteen** directories.

## 1) Clearing the phase 2 finish line

Phase 2 section 9 states that no group of ten or more cohesive files remains,
and that any future proposal must clear two tests: **ten or more files**, and
**a name a newcomer would guess without being told**. It also lists
`treasure`/`traps`/`random` at "3 each" among the groups correctly left flat.

This proposal contradicts that listing, and the reason is worth stating plainly
rather than leaving it to look like drift. Phase 2 evaluated `treasure*` as a
filename-prefix group of three. That is the wrong unit. Membership in this tree
is by *primary job*, and the job here is the object lifecycle: define an object,
generate it, command it, trade it, store it, persist it. Anchored by
`act.item.c` and `objsave.c` - neither of which carries a `treasure` prefix and
so neither of which appeared in that count - the cluster is fourteen files, not
three.

Both tests pass:

- **Fourteen files** (8 `.c`, 6 `.h`), counted the same way phase 2 counted
  `character/` at 25 and `quest/` at 10.
- **`obj/`** is what a newcomer looks in for `act.item.c`. The codebase already
  uses `obj` as its own vocabulary for the concept - `obj_data`, `objsave.c`,
  `ITEM_*` flags hanging off object prototypes.

## 2) Scope - `src/obj/`, 14 files

| Files | Role |
|-------|------|
| `act.item.c` | object command surface, 285K - the anchor |
| `item.h` | object stat/identify interface (no `.c`; `do_stat_object` lives in `act.item.c`) |
| `objsave.c` | object persistence, rent and crash saves (no header) |
| `treasure.c/.h`, `treasure_const.c` | random object generation and its tables |
| `spec_artifacts.c/.h` | specific named objects, 186K |
| `shop.c/.h` | shopkeeper economy - an object vendor end to end |
| `trade.c/.h` | object transactions |
| `house.c/.h` | the room-keyed half of object persistence (see 2.2) |

134 - 14 = **120 flat**, in 15 directories.

### 2.1 What is deliberately excluded

**`traps.c/.h` and `traps_new.c` do not move.** They were in the original
proposal and are rejected on the membership test. `traps.c` references
`obj_data` 9 times and `traps_new.c` 10 times, across 16K and 53K of source
respectively; `traps_new.c` includes `combat/fight.h`, `magic/spells.h`, and
`character/perks.h`. Traps are room and encounter hazards that expose a small
object-detection surface. Filing them under `obj/` would be grouping by
adjacency - the exact failure mode phase 1 named when it kept `fight.h` from
pulling in its 57 includers. They stay flat, and belong under `combat/` if
anywhere.

### 2.2 `house.c` is a core member, not a borderline one

An earlier draft of this plan called `house.c` the weakest member on the
strength of an `obj_data` reference count of 11. That count is a bad proxy and
the conclusion drawn from it was wrong. `house.c` mostly operates through
`obj_save_data`, `obj_file_elem`, `GET_OBJ_*`, and `ITEM_*` rather than naming
the struct directly. Counted across the whole object vocabulary it is **77
object-domain references against 74 room-domain** - and the object half is the
substantially harder machinery.

The decisive evidence is in `objsave.c` itself:

```c
obj_save_data *objsave_parse_objects_db(char *name, room_vnum house_vnum)   /* objsave.c:2256 */
```

objsave.c's own public API takes a house vnum. These are not two systems that
touch each other - they are **one object-persistence system already split
across two files**, keyed by player in one and by room in the other.
`House_load` calls that parser directly, and `House_save` writes through
`objsave_save_obj_record_db`.

What `house.c` spends its lines on: `House_save`, `House_load`,
`House_crashsave`, `House_save_all`, `House_restore_weight`, `House_listrent`,
`handle_house_obj`, `ascii_convert_house`, and roughly 285 lines of
`can_hsort`/`perform_hsort`/`do_hsort` object sorting. The room-ownership
portion - `house_control_rec`, `find_house`, `House_can_enter`, the guest list,
and `hcontrol` build/destroy/pay - is the smaller share.

Moving `house.c` into `src/obj/` puts it beside the other half of its own
system. That is a reason to move it, not a caveat about it.

An earlier draft of this section also claimed `house.c` duplicated
`Obj_from_store` and the `cont_row` container-nesting algorithm from
`objsave.c`, and flagged silent drift between the two copies as a risk. **That
was wrong on both counts** and is corrected here so nobody goes hunting for it:

- `Obj_from_store` exists **only** in `house.c`. `objsave.c` has no such
  function. The `house.c` copy is live but reachable only from
  `ascii_convert_house`, the one-time legacy ASCII-to-database converter driven
  by `hcontrol convert`.
- The `cont_row` logic in `house.c` was entirely inside a commented-out
  `handle_house_obj`, dead since sorting was introduced. It has since been
  removed (see section 10). `objsave.c`'s `cont_row` usage is live and unique.

### 2.3 One admitted rough edge

**The `spec_*` prefix splits.** `spec_artifacts.c` moves while `spec_procs.c`
and `spec_assign.c` stay flat. This is correct under the job test - artifacts
are objects, `spec_procs` is a dispatch table - but it does mean the prefix no
longer sorts together.

## 3) Include rewrite cost

| Header | Includers | External | | Header | Includers | External |
|--------|----------:|---------:|-|--------|----------:|---------:|
| `shop.h` | 22 | 21 | | `house.h` | 15 | 14 |
| `treasure.h` | 21 | 18 | | `item.h` | 12 | 9 |
| `spec_artifacts.h` | 10 | 8 | | `trade.h` | 1 | 0 |

**49 distinct files** outside the moved set need path-qualified includes; 31 of
them already sit in subdirectories. Files inside `src/obj/` keep bare includes -
quote-includes search the including file's own directory first.

`trade.h` has exactly one includer, `trade.c`, which moves with it. Zero
external churn.

## 4) Reference categories to sweep

The six categories phase 1 established, with what each costs here:

1. **Bare `#include "name.h"`** - the bulk of the work; 49 distinct files.
2. **Relative form `../../src/name.h`** - two unit-test files, confirmed:
   - `unittests/CuTest/test_artifacts.c:23` - `../../src/spec_artifacts.h`
   - `unittests/CuTest/test_web_onboarding.c:34` - `../../src/shop.h`

   Both compile as part of `cutest_SOURCES`, so missing them breaks `make test`.
   Phase 2 hit this exact category and it was real there too.
3. **Path-qualified `systems/` or `world/` form** - none remain.
4. **Build files** - `Makefile.am` lists all 8 sources (lines 21, 116, 151, 193,
   197, 207, 210, 211); `CMakeLists.txt` lists the same 8 (lines 397, 480, 516,
   542, 546, 557, 561, 562). Neither globs; both must move together. Do not
   disturb `src/vessels/vessels_trade.c`, which sits adjacent in `Makefile.am`
   and matches a naive `trade` grep.
5. **Scripts and mirror trees** - verified none reference these files.
6. **`sql/components/*.sql` header comments** - verified none.

Also: 11 current documents reference these paths (section 8).
`docs/CHANGELOG.md` and `docs/previous_changelogs/` stay untouched - they record
the tree as it was.

## 5) Sequence

Two commits, ascending blast radius so the cheapest failure comes first. Five
files include headers from both subgroups and will be touched twice; that is
accepted in exchange for two independently reviewable diffs.

**Commit 1 - commerce (21 external files):** `shop.c/.h`, `trade.c/.h`.

**Commit 2 - object core and persistence (33 external files):** `act.item.c`,
`item.h`, `objsave.c`, `treasure.c/.h`, `treasure_const.c`,
`spec_artifacts.c/.h`, `house.c/.h`.

`house.c` ships in the same commit as `objsave.c` deliberately. Per section 2.2
they are one persistence system across two files, and `objsave.c` exports
`objsave_parse_objects_db(char *name, room_vnum house_vnum)` which `house.c`
calls. Splitting them across commits would leave an intermediate state where one
half of a single system is path-qualified against the other for no benefit.

Per commit:

1. `git mv` the files. Renames only - no content edits in the same step.
2. Rewrite includes outside the set; leave intra-directory includes bare.
3. Update `Makefile.am` and `CMakeLists.txt`.
4. Delete orphaned `.o` and `.deps` entries for the moved files - `make clean`
   will not remove them, since they are no longer derived from a listed source.
   All 8 sources currently have stale `.o` files in `src/`.
5. `autoreconf -fi && ./configure && make -j$(nproc)`, expect zero warnings.
6. `make test`, expect OK.
7. `make install`; confirm no root-level `circle` artifact remains.
8. Run pre-commit; accept clang-format's trailing-comment realignment where a
   longer include path shifted the comment column.
9. Rebuild and re-test after any reformat, then commit.

Then documentation (section 8), then a final verification: clean autotools
build, clean CMake build, `make test`, the protocol parser harness, both
character-rename targets, and `scripts/test_pubsub.sh`.

## 6) Branch prerequisite

Do not start this on `codex/vessel-help-refresh`. That branch has uncommitted
work in `AGENTS.md`, `sql/components/help_vessel_entries.sql`, and
`sql/components/verify_help_vessel_entries.sql`, and this phase edits `AGENTS.md`
again in section 8. A 49-file include sweep will collide with anything else in
flight. Land the vessel help work first, then branch from `master`.

## 7) Risk notes

- `act.item.c` at 285K becomes the largest single file in any feature
  directory. Moving it is mechanical; **splitting it is out of scope** and
  should not be folded into a rename commit.
- `treasure.h` reaches into `character/` (`race.c`, `backgrounds.c`) and
  `craft/` (3 files). Those are consumers of generated loot, not object logic -
  the same shape phase 2 accepted for `class.h` at 59 includers. No re-scoping
  needed, but expect the include diff to touch directories that look unrelated.
- `spec_artifacts.h` is included by `comm.c`, `db.c`, `handler.c`, `limits.c`,
  and `interpreter.c` - five core files. Verify the artifact cooldown and
  persistence paths still build cleanly before committing.
- `house.c`'s local forward declarations were resolved before the move (section
  10), so it now gets its prototypes from headers and the compiler will catch
  signature drift. Even so, exercise a house load and an `hsort` in-game before
  commit 2 is considered done - `make test` does not cover the house
  persistence path.

## 8) Documentation to update

- `AGENTS.md` (which `CLAUDE.md` symlinks to) - add `src/obj/` to the layout
  table.
- `SRC_DIRECTORY_LAYOUT.md` - final layout table and flat count.
- This file's status in the ongoing-projects `README.md`.
- The 11 documents identified in section 4, in descending hit count:

  | Document | Path hits |
  |----------|----------:|
  | `docs/systems/SHOP_SYSTEMS.md` | 12 |
  | `docs/project-management-zusuk/ongoing-projects/cbuild-issues.md` | 7 |
  | `docs/systems/ARTIFACT_SYSTEM.md` | 5 |
  | `docs/project-management-zusuk/ongoing-projects/artifacts.md` | 3 |
  | `docs/systems/CHARACTER_RENAME_SYSTEM.md` | 2 |
  | `docs/world_game-data/ROOM_FLAGS.md` | 1 |
  | `docs/world_game-data/OEDIT_GUIDE.md` | 1 |
  | `docs/web/guides/room_flags.html` | 1 |
  | `docs/web/guides/oedit.html` | 1 |
  | `docs/systems/TRAP_SYSTEM_IMPLEMENTATION.md` | 1 |
  | `docs/systems/CASTING_VISUALS_SYSTEM.md` | 1 |

## 9) What stays flat after this

120 files, and the finish-line claim from phase 2 section 9 still holds with
`obj/` carved out. Remaining prefix groups are all below the ten-file floor:
`ai_*` (7), `clan*` (7), `db*` (7), `mysql*` (4), `spec*` (3 after
`spec_artifacts` leaves), `traps*` (3), `random*` (2), and
`roleplay`+`char_descs`+`introduce` (5).

`db.h` has 209 includers, the most in the codebase; `db.c` is the world loader
and belongs at top level beside `comm.c`. The genuine core stays flat: `comm.c`,
`handler.c`, `interpreter.c`, `utils.c/.h`, `structs.h`, `constants.c`,
`limits.c`, and the `act.*` family minus `act.item.c`.

Phase 4 is not expected. The two tests stand for any future proposal.

## 10) Pre-move cleanup (done)

Completed on `src/house.c` before any file moves, so the rename commits stay
purely mechanical. Build clean with `-Wall -Wextra`, `make test` OK (307 tests),
`make install` run, no root-level `circle` artifact left behind.

| Change | Detail |
|--------|--------|
| Removed dead declaration | `obj_save_data *objsave_parse_objects(FILE *fl);` - unused in `house.c` and already declared in `db.h:323`, which `house.c` includes |
| Replaced forward declaration with header | `zone_rnum real_zone_by_thing(room_vnum vznum);` -> `#include "olc/genzon.h"`. The function is genuinely used (`house.c:751`); `genzon.h` is prototype-only with no includes of its own, so this adds no coupling |
| Deleted dead code | The commented-out `handle_house_obj` (83 lines), its commented call site in `House_load`, and a commented `num_objs` local |
| Documented intentional behavior | Replaced the deleted block with a comment explaining *why* `House_load` loads flat and ignores `<locate>` |

That last row is the one worth reading. `House_save` writes container nesting
via negative `<locate>` values, and `objsave_parse_objects_db` returns a flat
list carrying those values - but `House_load` ignores them and calls
`obj_to_room` on every record. That looks like a container-flattening bug until
you follow the function to its end: `perform_hsort` runs there and re-files
everything into the standard category containers, so any nesting rebuilt during
load would be discarded moments later. The behavior is intentional. It is now
commented as such, because the next reader will reach for the same false
conclusion.

No functional change was made. The only reachable code touched was the
declaration source for `real_zone_by_thing`.
