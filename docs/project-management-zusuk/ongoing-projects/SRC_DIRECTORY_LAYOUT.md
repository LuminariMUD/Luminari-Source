# src/ Directory Layout - Pilot Plan

Working notes. Nothing here describes current behavior; it proposes a change to
how source files are arranged on disk. No functional change is involved at any
step.

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

## 5) Decision point

Evaluate after the pilot lands and has survived a week of normal work. If
path-qualified includes prove more annoying than informative in practice, stop
at one directory rather than propagating the pattern to the other seven. The
remaining clusters are independent of each other and can be done one per commit
in any order.

## 6) Unrelated cleanups noticed

Not part of this plan; recorded so they are not lost.

- `src/castle.c.tbamud` - upstream leftover, not in any build list.
- `src/test_metamagic.c` - a test file living in the production source tree.
- `spatial_core.c` / `spatial_core.h` duplicated between `src/` and
  `src/systems/spatial/`; determine which is live before the `wilderness/`
  cluster is attempted.
- A large volume of stale `.o` files is checked into the working tree's ignore
  scope and clutters directory listings.
