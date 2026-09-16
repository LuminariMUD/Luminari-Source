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

Status: 2026-09-16. Mechanics restoration is done and proven in tests and
live. Documentation and help are updated. Remaining work is the final gates
and merge. Nothing else is in scope.

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
| `Restore NPC combat behavior` (this session, see git log) | NPC race/class/spell behavior in combat restored. The baseline mobile heartbeat ran `npc_racial_behave` / `npc_ability_behave` / `npc_assigned_spells` / `wizard_combat_ai` / `npc_class_behave` for every fighting NPC once per six seconds; the refactor left that block only on the legacy path and `c383917c8` deleted it, so NPC casters never cast in fights. `npc_combat_behave()` in `mob_act.c` now runs from `perform_violence()` at phase 1, skipped when the special procedure handled the turn. Regressions: `Test_combat_restoration_npc_wizard_acts_once_per_rotation` (red without the hook) and `_npc_below_newbie_level_only_melees`. Found while the live NPC-caster run showed a level 3 wizard only punching; the level gate is `NEWBIE_LEVEL` (6), as in the baseline. |

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

Progress on 2026-09-16 (this session):

1. Live proof: done for the user workflow. `docs/testing/COMBAT_RESTORATION_LIVE_2026_09_16.md`
   holds transcripts from a level 1 mortal sorcerer against a fremlin on port
   4100: initiative message, alternating two-second phase offsets, a cast
   admitted after ordinary attacks with the full progress lines and completion,
   the move action back exactly six seconds later, automatic attacks suppressed
   while casting, a cast interrupted by damage, and queued kicks replacing the
   next hit. `[smw]` matched real availability and no recovery notices were
   spammed. Nothing needed fixing. Setup notes for a future session: the MUD
   was run inside `unshare -rn --pid --fork --mount-proc` with a `socat` bridge
   from the namespace to a unix socket, because the main checkout's dev server
   holds host port 4100; a premade wizard has no spellbook so its preparation
   never completes (use a sorcerer); room 145383 blocks magic and the tutorial
   start room is peaceful (room 145203 works). The NPC-caster run exposed the
   missing NPC combat behavior fixed in the row above; the rerun against the
   Red Magi (timed NPC casting, interruption, familiar) and the group-assist
   run (auto-join, three-way phase offsets) are in the evidence document.
2. Documentation: done. `COMBAT_SYSTEM.md` sections 2 and 3 now describe the
   phase driver, free automatic attacks, native action deadlines, the retained
   six-second logical turn, and eligibility-based queue dispatch; the removed
   `LUMINARI_COMBAT_ROUNDS` rollback claim is gone. The tactical doc's stale
   `run_semantic_round` reference now names `begin_semantic_round`. The other
   two docs describe the retained logical turn and needed no change.
3. Help: done. `help_semantic_combat_entries.sql` (COMBAT, INITIATIVE) and the
   ACTION-QUEUE paragraph in `help_command_sweep_entries.sql` describe phases,
   free attacks, exact deadlines, and immediate queue dispatch; the verifier's
   substrings were updated. Both migrations were applied twice to the
   development database (idempotent) and the verifier passes. `help.hlp`
   carries the same text.
4. Gates and merge: `make -j$(nproc) test` passed all 1,585 CuTests on the
   final source (including the two NPC regressions and the updated spec
   contract test) with no compiler warnings; `make install` succeeded and no
   root `luminari` artifact remains; pre-commit passes on every changed file.
   Pushed and opened as PR #197
   (https://github.com/LuminariMUD/Luminari-Source/pull/197). Still open:
   run the triggered CI jobs locally (`python3 scripts/ci/local/run.py`),
   merge, then delete this document per `docs/ongoing-projects/README.md`.

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
