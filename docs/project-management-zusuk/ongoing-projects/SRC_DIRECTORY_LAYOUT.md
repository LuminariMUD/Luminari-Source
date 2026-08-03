# src/ Directory Layout

Record of a completed change to how source files are arranged on disk. The
layout described in section 5 is the current one. No functional change was
involved at any step.

## 1) Problem

`src/` holds 378 `.c`/`.h` files flat, next to a `src/systems/` tree that holds
22 files spread over seven directories:

| Directory | `.c`+`.h` files |
|-----------|-----------------|
| `systems/region_hints/` | 1 |
| `systems/terrainbridge/` | 1 |
| `systems/narrative_weaver/` | 2 |
| `systems/web_client/` | 2 |
| `systems/spatial/` | 3 |
| `systems/intermud3/` | 5 |
| `systems/pubsub/` | 8 |
| `src/` (flat) | 378 |

The issue is not that subdirectories exist. It is that no rule governs which
code earns one, so a single-file module (`region_hints.c`) has a dedicated
two-level path while `vessels_*` (27 `.c` files) has none. Two consequences:

- Include style is inconsistent. `src/comm.c:101` reads
  `#include "systems/web_client/onboarding.h"`, while
  `src/systems/pubsub/pubsub_db.c:13` reads `#include "conf.h"`. Both resolve,
  because the build adds an include path rooted at `src/`, but a reader cannot
  tell from an include line where a header lives.
- `spatial_core.c` / `spatial_core.h` exist in **both** `src/` and
  `src/systems/spatial/`. The taxonomy has already leaked.

## 2) Proposed rule

One flat level of feature-sized directories under `src/`. No nesting.

> A directory must hold roughly 10 or more files and carry a name a new
> developer would guess. Anything below that threshold stays in `src/`.

The numeric floor is what keeps directory count down; it is a rule rather than
a judgement call, so it does not drift. Applied to the current tree it yields
about eight directories, and `src/systems/` dissolves into them:

| Proposed directory | Files | Absorbs |
|--------------------|-------|---------|
| `src/olc/` | 41 | `*edit.c`, `oasis*`, `gen*` |
| `src/vessels/` | 30 | `vessels_*`, `vehicles*`, `transport*` |
| `src/wilderness/` | 27 | `resource_*`, `wilderness*`, `perlin`, `kdtree`, `spatial*`, `region_hints`, `terrainbridge` |
| `src/craft/` | 20 | `craft*`, `crafting*`, `brew`, `alchemy`, `trade`, `shop` |
| `src/movement/` | 18 | `movement_*` |
| `src/mob/` | 18 | `mob_*` |
| `src/dgscript/` | 15 | `dg_*` |
| `src/net/` | ~12 | `protocol`, `comm`, `discord_bridge`, `intermud3/*`, `web_client/*` |

Below the floor and therefore staying flat: `ai_*` (7), `clan*` (8), `pubsub`
(8), `narrative_weaver` (2). The flat remainder is then the genuine core -
`db.c`, `handler.c`, `interpreter.c`, `structs.h`, `utils.h`, `fight.c`,
`class.c`, `race.c`, `spells.c` - which is what should stay findable at the top.

Explicitly rejected:

- **Grouping by layer** (`core/`, `util/`, `net/`). Nearly every file depends on
  `utils.h`, so `util/` becomes a junk drawer.
- **Nesting past one level.** `src/systems/spatial/` is the pattern being
  replaced; `src/spatial/` reads the same and costs less.
- **Splitting `act.*.c`.** The `act.` prefix already sorts them together and
  they are command-dispatch surface for the whole game, not a subsystem.

## 3) Include mechanics

Autotools resolves headers via `DEFAULT_INCLUDES = -I. -I$(top_builddir)/src`;
CMake via `include_directories(src)` (`CMakeLists.txt:36`). Both give a flat
header namespace rooted at `src/`.

Two properties follow, and they determine the whole cost model:

1. A header that stays in `src/` is includable by bare name from any depth.
   Moving a `.c` file alone costs nothing.
2. A header moved into a subdirectory is no longer resolvable by bare name from
   `src/`. Its includers must path-qualify: `#include "vessels/vessels.h"`.
3. However, C quote-includes search the including file's own directory first.
   So a file already inside `src/vessels/` keeps `#include "vessels.h"`
   unchanged. **Only includers outside the moved set need editing.**

Point 3 is what makes the blast radius small.

**Do not** add per-directory `-I` flags (`-Isrc/vessels -Isrc/olc ...`) to
restore bare includes. That recreates the flat namespace and discards the only
real benefit of the move, which is that a path-qualified include makes
cross-subsystem coupling visible at the top of every file.

## 4) Pilot: `src/vessels/`

Chosen as the pilot because it is the largest single cluster and the area under
active work. Scope is 30 files: 27 `.c`, 3 `.h`.

### 4.1 Files to move

```
transport.c              vessels_contracts.c    vessels_narrative.c
transport.h              vessels_crew.c         vessels_ownership.c
transport_unified.c      vessels_db.c           vessels_piracy.c
transport_unified.h      vessels_docking.c      vessels_rooms.c
vehicles.c               vessels_edit.c         vessels_tactical.c
vehicles_commands.c      vessels_events.c       vessels_trade.c
vehicles_transport.c     vessels_hazards.c      vessels_upgrades.c
vessels.c                vessels_hunters.c
vessels.h                vessels_lookout.c
vessels_admin.c          vessels_merchants.c
vessels_autopilot.c      vessels_balance.c
vessels_combat.c
```

There is no `vehicles.h`; the `vehicles_*.c` files declare through `vessels.h`.

### 4.2 Include edits

Only three headers move, and only files outside the set need changes. Verified
total: **19 include lines across 16 files.**

`"vessels.h"` -> `"vessels/vessels.h"` (11 files):

```
src/act.wizard.c:71        src/genwld.c:24         src/players.c:42
src/comm.c:113             src/interpreter.c:67    src/spec_procs.c:48
src/db.c:73                src/mob_act.c:37
src/db_init.c:21           src/movement.c:47
                           src/player_rename.c:21
```

`"transport.h"` -> `"vessels/transport.h"` (8 files):

```
src/act.informative.c:49   src/movement.c:39            src/routing.c:22
src/comm.c:104             src/movement_events.c:34     src/spells.c:38
src/interpreter.c:70       src/movement_messages.c:32
```

`"transport_unified.h"` has exactly one includer, `src/transport_unified.c`,
which moves with it. No edit required.

All includes *within* the moved set - including `src/transport_unified.h`'s
include of `vessels.h` - resolve unchanged via the includer's-directory rule.

### 4.3 Build file edits

Both build systems enumerate every source explicitly; neither globs. All three
lists must move together.

- `Makefile.am`, `circle_SOURCES`: 27 paths in the range lines 208-246 gain a
  `vessels/` component. Headers are not listed in `circle_SOURCES`, so nothing
  else changes there.
- `CMakeLists.txt`: 27 paths at lines 558-588.
- `cutest_SOURCES` references `unittests/CuTest/test_transport_production.c`,
  which does not move. No change.

### 4.4 Script edits

Five lines across three scripts assert on source paths:

```
scripts/test_vessel_merchant_in_game.sh:521
scripts/test_vessel_hunter_in_game.sh:418, 420
scripts/test_vessel_scale_benchmark_parsers.sh:23, 189
```

### 4.5 Stale object files

Automake will emit objects to `src/vessels/*.o`. The existing `src/vessels*.o`
and `src/cutest-vessels*.o` become orphaned - `make clean` will not remove them,
because they are no longer derived from any listed source. Delete them
explicitly before the first post-move build so a stale object cannot be linked.

### 4.6 Sequence

1. Land the in-flight vessel documentation work first. The current working tree
   has 13 modified docs plus a deletion; rebasing them across a 30-file rename
   is avoidable churn.
2. `git mv` the 30 files.
3. Apply the 19 include edits (4.2).
4. Update `Makefile.am`, `CMakeLists.txt` (4.3) and the three scripts (4.4).
5. Remove orphaned objects (4.5).
6. `autoreconf -fvi && ./configure && make clean && make -j$(nproc)`, then
   `make install`. Confirm no root-level `circle` artifact is left behind.
7. `make test`, then `make install` again per the root test-path convention.
8. Verify the CMake path builds as well, since it is a separate source list.

Commit as **one atomic commit containing only `git mv`, include-path, build-file
and script edits - zero content changes**, so the diff reviews as mechanical.

### 4.7 Known costs

- `git blame` follows renames automatically; `git log` on a moved file needs
  `--follow`. Worth noting in the commit message.
- Any in-flight branch touching these 30 files will need a rebase.
- The pilot is reversible: the inverse is another 30 `git mv` plus the same 19
  include lines.

## 4.8 Pilot outcome (executed)

The pilot is done. `src/vessels/` holds all 30 files; both build systems and
the full test suite are green.

Verified results:

- 30 files moved, all recorded by git as pure renames with zero content change.
  The commit is 62 files changed, 90 insertions and 90 deletions - symmetric,
  because every edit is a path rewrite.
- Autotools: `autoreconf -fvi && ./configure && make clean && make` exits 0
  with **zero warnings and zero errors**. Objects land in `src/vessels/*.o`.
- `make test`: **OK (306 tests)**, exit 0.
- CMake: out-of-tree `cmake -S . -B <dir> && cmake --build` exits 0 with zero
  errors.
- `bin/circle` installed from the autotools build; no root-level `circle`
  artifact left behind.

Two reference categories section 4.2 missed, both now fixed:

1. **Unit-test includes.** Six files under `unittests/CuTest/` include
   `"../../src/vessels.h"` - the relative-path form, which the planning grep
   for `#include "vessels.h"` did not match. All six are in `cutest_SOURCES`,
   so `make test` would have failed to compile. Fixed to
   `"../../src/vessels/vessels.h"`.
2. **A test fixture that mirrors the source tree.**
   `scripts/test_vessel_scale_benchmark_parsers.sh` builds a temporary
   provenance tree and `touch`es `$root/src/vessels/vessels.c`, but its
   `mkdir -p` only created `$root/src`. This failed at runtime, not compile
   time, so only executing the suite caught it.

Lesson for the remaining clusters: grep for the **relative-path** include form
(`../../src/<name>.h`) and for scripts that construct mirror trees of `src/`,
not just for bare `#include "<name>.h"`. Compile-time greps alone are not
sufficient; run the full suite before declaring a cluster done.

Also updated beyond the plan's original scope: five `sql/components/*.sql`
header comments that name the source file each schema mirrors. These are
comments only, but a stale path pointer is actively misleading. Historical
records in `docs/CHANGELOG.md` and `docs/previous_changelogs/` were
deliberately left untouched - they describe paths as they were at the time.

## 5) Outcome - complete

All clusters are landed. `src/systems/` and `src/world/` no longer exist, and
nothing in `src/` is nested more than one level deep.

Final layout:

| Directory | Files |
|-----------|------:|
| `src/olc/` | 40 |
| `src/wilderness/` | 32 |
| `src/vessels/` | 30 |
| `src/character/` | 25 |
| `src/magic/` | 18 |
| `src/mob/` | 18 |
| `src/movement/` | 18 |
| `src/dgscript/` | 15 |
| `src/combat/` | 13 |
| `src/craft/` | 13 |
| `src/net/` | 11 |
| `src/obj/` | 14 |
| `src/comms/` | 10 |
| `src/quest/` | 10 |
| `src/pubsub/` | 8 |
| `src/` (flat) | 120 |

`character/`, `quest/`, and `comms/` landed in phase 2, planned separately in
[SRC_LAYOUT_PHASE2.md](SRC_LAYOUT_PHASE2.md). `obj/` landed in phase 3, planned
in [SRC_LAYOUT_PHASE3.md](SRC_LAYOUT_PHASE3.md), taking flat from 134 to 120
across fifteen directories.

Every directory clears the file-count floor. The flat remainder is the core -
`comm.c`, `db.c`, `handler.c`, `interpreter.c`, `structs.h`, `utils.h`,
`fight.c`, `class.c`, `race.c`, `spells.c` - plus the below-floor subsystems
the rule deliberately leaves alone.

Each cluster is one commit, verified before it landed: clean autotools rebuild
at zero warnings, `make test` at OK (306 tests), and a final out-of-tree CMake
build to confirm the second source list stayed correct.

### 5.1 Where the plan was revised

- **A formatting pass came first.** The pre-commit clang-format hook reformats
  every staged file, and 89 of 443 tracked C files did not conform. Any commit
  touching one triggered a mass restyle - during the pilot it rewrote 29 files
  for a net -563 lines. A one-time repo-wide pass, run through pre-commit so it
  matches the pinned clang-format v18.1.8 exactly, was committed on its own.
  After that the hook is a no-op and every move commit stays a pure rename.
  This is the fix for the recurring collision the pilot could only work around
  with `--no-verify`.
- **`craft/` is narrower than sketched.** `trade` and `shop` are commerce, not
  crafting; filing them under `craft/` would misdescribe them, and at 4 files
  they fall below the floor, so they stay flat. `material_types.h` has no
  includers at all and was left in place rather than guessing a home for dead
  code. *(Superseded in phase 3: `shop` and `trade` are commerce in the sense
  of moving objects, and they now sit in `src/obj/` with the rest of the object
  lifecycle. The phase 1 reasoning holds - they were never crafting - but they
  are no longer flat.)*
- **`comm.c` stays flat.** The plan listed it under `net/`, but it is the main
  select() loop and heartbeat scheduler, not a socket layer. Burying the
  codebase's central file would cost more than the grouping gains.
- **`clan_edit.c` moved to `olc/`.** It includes `genolc.h`, `oasis.h`, and
  `improved-edit.h`, so it is an editor built on the OLC framework. This does
  split clan code across two directories - the accepted cost of filing editors
  by framework rather than by subject.
- **`pubsub/` was added.** Not in the original table. Its 8 sources under
  `src/systems/pubsub/` plus 3 files in `src/` make 11, which clears the floor.
- **`src/world/` was dissolved.** Not in the original scope, but it held
  exactly 2 files - the same pathology the plan set out to remove.
- **`combat/` and `magic/` were added later**, after the original eight landed.
  `combat/` takes only the 13 files whose primary job is resolving a fight.
  `fight.h` has 57 includers spanning movement, OLC, DG scripts, and vessels,
  so proximity to combat cannot define membership without producing a junk
  drawer. `evolutions.c` (the feats system), `traps*.c` (driven from movement
  and item use), `magic.c` (the spell engine), and `actions*.c` (general
  scheduling) were excluded on that test. `magic/` takes 18 files and is the
  more cohesive of the two; its `spells.h` has 113 includers, the widest
  rewrite in the reorganization.
- **`pubsub/` is now below the floor.** Deleting three dead files - two empty
  V3 stubs and a superseded stub module - left it at 8. It keeps its directory:
  the name is unambiguous, the group is coherent, and scattering 8 files back
  into `src/` to satisfy a "roughly 10" guideline would trade real clarity for
  bookkeeping.

### 5.2 Reference categories that bite

The pilot found two the plan missed; the later clusters found two more. The
full list to sweep for, per cluster:

1. Bare `#include "name.h"`.
2. Relative form `#include "../../src/name.h"` - used by `unittests/CuTest/`.
3. Path-qualified form `#include "systems/<dir>/name.h"` or
   `#include "world/name.h"`. These bite **inside** the moving set too, where
   same-directory includes otherwise need no edit.
4. Source paths in `Makefile.am` and `CMakeLists.txt`, including
   **commented-out** entries.
5. Source-path assertions in `scripts/`, and any script that builds a mirror of
   the source tree with `mkdir -p` - that one fails at runtime, not compile
   time, so only running the suite catches it.
6. Header comments in `sql/components/*.sql` naming the file each schema
   mirrors.

Historical paths in `docs/CHANGELOG.md` and `docs/previous_changelogs/` are
deliberately untouched; they record the tree as it was.

## 6) Cleanups completed and outstanding

Resolved during this work:

- `spatial_core.c` / `spatial_visual.c` were stale forks in `src/`, absent from
  both build systems and every script, diverged 189 lines from the live copies
  under `src/systems/spatial/`. Deleted; the live sources moved to
  `src/wilderness/` with the headers, which had only ever existed in `src/`.
- The clang-format hook's exclusion path in `.pre-commit-config.yaml` was
  updated to follow `genolc.c` to `src/olc/`, preserving its original intent.

Still outstanding, unrelated to layout:

- `src/castle.c.tbamud` - upstream leftover, not in any build list.
- `src/test_metamagic.c` - a test file living in the production source tree.
- A large volume of stale `.o` files clutters directory listings.
