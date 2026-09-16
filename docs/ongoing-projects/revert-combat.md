# revert-combat.md - Reverting Combat System Mechanics Without Losing Event System Changes

## Examples

### How combat system should look like with Action System and Queueing working correctly

```

kill fremlin
Your opponents superior initiative grants the first strike!
[R:20][CRIT!]  [6] A fremlin wounds you with his hit.
25/31H 830/830V XP:0 Gold:0 PrepTime: [smw] Time:6pm >
<T: Inquiy TC: good (Standing)> <E: a fremlin EC: perfect (Standing)>
[stum!][R: 1]A fremlin misses a wild punch at you.
25/31H 830/830V XP:0 Gold:0 PrepTime: [smw] Time:6pm >
cast 'mage armor' me
<T: Inquiy TC: good (Standing)> <E: a fremlin EC: perfect (Standing)> You begin casting your
spell...
26/31H 830/830V XP:0 Gold:0 PrepTime: 8 [s-w] Time:6pm >
<T: Inquiy TC: good (Standing)> <E: a fremlin EC: perfect (Standing)>
Casting: mage armor ....
A faint rift begins to form in reality...
26/31H 830/830V XP:0 Gold:0 PrepTime: 8 [s-w] Time:6pm >
<T: Inquiy TC: good (Standing)> <E: a fremlin EC: perfect (Standing)>
Casting: mage armor ...
The tear in space widens, revealing glimpses beyond...
26/31H 830/830V XP:0 Gold:0 PrepTime: 8 [s-w] Time:6pm >
<T: Inquiy TC: good (Standing)> <E: a fremlin EC: perfect (Standing)>
Casting: mage armor ..
Extraplanar energy pours through the opening...
26/31H 830/830V XP:0 Gold:0 PrepTime: 8 [s-w] Time:6pm >
<T: Inquiy TC: good (Standing)> <E: a fremlin EC: perfect (Standing)>
Casting: mage armor .
Reality bends dangerously around you!
You complete your spell...You have gained enough xp to advance, type 'gain' to level.
You feel someone protecting you.
26/31H 830/830V XP:-3875 Gold:0 PrepTime: 8 [s-w] Time:6pm >
<T: Inquiy TC: good (Standing)> <E: a fremlin EC: perfect (Standing)>
[stum!][R: 1]A fremlin misses a wild punch at you.
27/31H 830/830V XP:-3875 Gold:0 PrepTime: 8 [s-w] Time:7pm >
<T: Inquiy TC: good (Standing)> <E: a fremlin EC: perfect (Standing)>
[R: 2]A fremlin ducks under your fist as you try to hit him.
27/31H 830/830V XP:-3875 Gold:0 PrepTime: 8 [smw] Time:7pm >
kick
<T: Inquiy TC: good (Standing)> <E: a fremlin EC: perfect (Standing)> Attack queued.
27/31H 830/830V XP:-3875 Gold:0 PrepTime: 8 [smw] Time:7pm >
<T: Inquiy TC: good (Standing)> <E: a fremlin EC: perfect (Standing)>
[R:17]  [4] A fremlin hits you extremely hard.
23/31H 830/830V XP:-3875 Gold:0 PrepTime: 8 [smw] Time:7pm >
<T: Inquiy TC: fair (Standing)> <E: a fremlin EC: perfect (Standing)>
You miss your kick at a fremlin's groin, much to his relief...
23/31H 830/830V XP:-3875 Gold:0 PrepTime: 8 [smw] Time:7pm >
<T: Inquiy TC: fair (Standing)> <E: a fremlin EC: perfect (Standing)>
[R:18]  [5] A fremlin wounds you with his hit.
18/31H 830/830V XP:-3875 Gold:0 PrepTime: 8 [smw] Time:7pm >
<T: Inquiy TC: fair (Standing)> <E: a fremlin EC: perfect (Standing)>

```

### The buggy and broken state combat is in now

```
kill fremlin
29/29H 830/830V [smw] >
<T: Testcharr TC: perfect (Standing)> <E: a fremlin EC: perfect (Standing)>

A fremlin ducks under your fist as you try to hit him.

29/29H 830/830V [smw] >
<T: Testcharr TC: perfect (Standing)> <E: a fremlin EC: perfect (Standing)>

You have gained enough xp to advance, type 'gain' to level.
You injure a fremlin with your hit.
A fremlin hits you hard.

26/29H 830/830V [--w] >
<T: Testcharr TC: good (Standing)> <E: a fremlin EC: good (Standing)>
cast 'mage armor' me
26/29H 830/830V [--w] >
<T: Testcharr TC: good (Standing)> <E: a fremlin EC: good (Standing)>

You must wait for the required action before trying that command.

26/29H 830/830V [--w] >
<T: Testcharr TC: good (Standing)> <E: a fremlin EC: good (Standing)>

You may perform another standard action.
You may perform another move action.
[CRIT!]You have gained enough xp to advance, type 'gain' to level.
You nearly kill a fremlin with your deadly hit!!
You duck under a fremlin's fist as he takes a swing at you.

27/29H 830/830V [--w] >
<T: Testcharr TC: excellent (Standing)> <E: a fremlin EC: poor (Standing)>
cast 'mage armor' me
27/29H 830/830V [--w] >
<T: Testcharr TC: excellent (Standing)> <E: a fremlin EC: poor (Standing)>

You must wait for the required action before trying that command.

27/29H 830/830V [--w] >
<T: Testcharr TC: excellent (Standing)> <E: a fremlin EC: poor (Standing)>
kick
28/29H 830/830V [--w] >
<T: Testcharr TC: excellent (Standing)> <E: a fremlin EC: poor (Standing)>

Attack queued.

28/29H 830/830V [--w] >
<T: Testcharr TC: excellent (Standing)> <E: a fremlin EC: poor (Standing)>

You may perform another standard action.
You may perform another move action.
You miss your kick at a fremlin's groin, much to his relief...
A fremlin misses a wild punch at you.

28/29H 830/830V [--w] >
<T: Testcharr TC: excellent (Standing)> <E: a fremlin EC: poor (Standing)>

You may perform another standard action.
You may perform another move action.
You try to hit a fremlin who easily avoids the blow.
A fremlin injures you with his hit.

```

## Combat mechanics restoration plan

Status: 2026-09-16. Mechanics restoration is done and proven in tests. Remaining
work is live proof, documentation, help, and merge. Nothing else is in scope.

### 1. Outcome

Combat behaves as it did before the event-core refactor (`fbe9366fb`), on top of
the current event architecture. Concretely:

- Attack phases 1/2/3 run per character at the historical 2-second offsets, with
  the historical 2/4-second initial delay from initiative.
- Ordinary attacks cost nothing. Only commands, abilities, spells, and readied
  actions spend standard/move/swift, at their own durations. Action recovery
  deadlines are the native action events; there is no second balance and no
  rounding to six-second turns.
- The general command queue dispatches as soon as the head is eligible. The
  attack queue (`kick`, `headbutt`, ...) replaces the next eligible hit.
- A mortal can `cast` mid-fight after ordinary attacks, the cast suppresses
  automatic attacks, and attacks resume afterward.
- Nested damage (Life Shield, retorts, juxtaposition, damage shields) completes
  synchronously in historical order, within the retained 64-reaction bound.

Retained on purpose: the native scheduler, reactor, generation handles, encounter
ownership, typed domain events, activity manager, readied actions, counterspell,
ally defense, tactical hazards, Four Arms, pets, and every other feature added
after the baseline. Approved deviation from baseline: Greater Hostile
Juxtaposition keeps working (three charges) instead of the baseline's dead branch.

Not in scope: reverting PRs, restoring old files, a second engine, config
switches, load or performance frameworks, and proactive hardening of code paths
that behave identically in the baseline and today.

### 2. Reference revisions

| Role | Revision |
| -- | -- |
| Pre-refactor mechanics baseline | `fbe9366fb` |
| Refactor integration (#85) | `3ddc1efd7` |
| Principal mechanics change to undo | `67ae730bc` (semantic rounds, action tax, phase collapse) |
| Implementation start | `cff3350f2` on master `9c5a0223f` |

PRs #67 and #70-84 are closed review PRs consolidated in #85; they are not
revert candidates. Later #134, #139, and #193 (presentation split into
`combat_messages.c`) are retained.

### 3. What was done

Production diff since `9c5a0223f`: 20 source files, about 490 lines added and
760 removed. Tests: 78 `Test_combat_restoration_*` cases plus updated encounter,
activity, and gameplay tests.

| Commit | Change |
| -- | -- |
| `b5437100f` | Red regressions for cast/kick, phase deadlines, queued kicks. Freed dequeued attack entries (leak). |
| `48aa19e2e` | Phases 1/2/3 at historical offsets on the encounter driver. Removed the automatic action tax, phase collapse, second action balance, turn rounding, join-time cancellation, one-intent-per-turn gate, and the two `comm.c` fighter exclusions. Cowering and Perfect Tempo back to the periodic owner. Fixed stale-owner teardown. `initiative`/`eventdebug` show phases. |
| `cd4371ac3` | Bonus offhand attacks back to historical phase allocation; Four Arms unchanged. |
| `146b7e996` | Nested damage completes before the parent resumes, sharing the 64-reaction bound. Life Shield zero-damage activation and charge order restored. Projectile damage uses the same owner. |
| `e438c1eda` | Descriptor loop wakes on buffered input or an eligible queue head under both I/O drivers. Greater Hostile Juxtaposition kept working (user decision). |
| `1478a31c3` | Kick/headbutt revalidate participants after damage callbacks. |
| `81680681f` | Ordinary Hostile Juxtaposition affect removed after reflection (parity). Hit riders stop after invalidation. |
| `ae4ebc752` | Damage-shield chain, `call_magic()`, and `mag_damage_scaled()` revalidate participants after callbacks. |

Verified at `ae4ebc752`: `make test` passes all 1,583 CuTests; the 78 restoration
cases pass under Valgrind with zero errors or leaks; `make install` succeeds. The
`mortal_cast_and_kick_*` and `default_*deadline*` cases were rerun on 2026-09-16
and pass.

The last three commits are callback-safety guards, not mechanics parity. They
stay (they are committed, tested, and small), but that line of work is closed.
Divine Sacrifice, death-notification ordering, Crescendo/Shard Volley packets,
multi-target loops, and DG cast triggers have the same raw-pointer continuation
in the baseline and today. They are not part of this restoration. If wanted,
file one GitHub issue after merge.

### 4. Remaining work

Do these in order. Each is bounded; do not add steps.

1. Live proof on port 4100 (`MUD_PORT=4100 ./scripts/autorun/autorun.sh`,
   `APP_ENV=development`). With an ordinary mortal caster and a fremlin-class
   mob, reproduce the good transcript at the top of this document: initiative
   message, staggered attacks, `cast 'mage armor' me` mid-fight with progress
   lines, `kick` queued and dispatched in place of a hit, `[smw]` prompt and
   recovery notices matching real availability. Add one fight against an NPC
   caster and one with a pet or charmie assisting. Save the transcripts under
   `docs/testing/`. Fix only what those transcripts show broken, with a
   regression in the existing production-linked suites.
2. Documentation. Replace the semantic-turn descriptions (six-second turns, one
   queued intent per turn, rounded durations, join on next round, and the
   `LUMINARI_COMBAT_ROUNDS` rollback that no longer exists) in
   `docs/systems/COMBAT_SYSTEM.md`, `docs/systems/MUD_EVENTS.md`,
   `docs/systems/TACTICAL_EFFECT_CLOCKS_AND_HAZARDS.md`, and
   `docs/systems/COUNTERSPELL_AND_ALLY_READINESS.md` with the section 1
   behavior. Effect clocks that still use the six-second logical turn (Defensive
   Casting, bleeding, hazards, readied expiry) stay documented as such.
3. Help, both stores. Rewrite the entries in
   `sql/components/help_semantic_combat_entries.sql` and its verify file for
   `COMBAT`, `INITIATIVE`, `ACTION-QUEUE`, `ACTIONS`, `ATTACK-QUEUE`, and
   `READY`; keep the `help_<feature>_entries.sql` pattern, sqlfluff clean, no
   `DELIMITER`. Apply to the development database and mirror the same text in
   `lib/text/help/help.hlp` (the old-model lines are near 627, 7080, 17803,
   27372, and 42228-42248).
4. Gates and merge. `make -j$(nproc) test`, then `make install`; pre-commit on
   changed files; no compiler warnings. Push `revert-combat`, open the PR, run
   the triggered CI jobs locally. After merge, move nothing from this document
   (section 1 is what the system docs will say) and delete it per
   `docs/ongoing-projects/README.md`.

### 5. Ablation

Removed from the earlier plan because no requirement or observed defect needed it:

- The open-ended "full parity inventory" (every attack path, every effect
  clock, every damage path) and the matching 13-row acceptance matrix. The
  suite and the live transcripts are the proof; deviations get fixed when seen.
- Further callback-continuation hardening (Divine Sacrifice, death ordering,
  extra spell packets, DG triggers). Pre-existing in both baseline and current.
- Both-driver whole-loop and load measurement, perfmon comparisons, and a
  historical comparison executable. The queue wakeup tests already cover both
  drivers; no scheduling leak has been observed.
- The 19-row PR map, the pre-implementation diagnosis table, and the
  per-checkpoint log listings. Git history and the test names hold that detail.
