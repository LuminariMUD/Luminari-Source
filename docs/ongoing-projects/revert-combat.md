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

Status: implementation in progress, 2026-09-16. The examples above are preserved
as supplied. Sections 1-8 retain the reviewed scope and acceptance criteria;
section 9 records current implementation evidence and the next work.

Plan review, 2026-09-16: every revision, pull-request state, file path, symbol,
test name, help keyword, and CI job cited below was checked against the
`revert-combat` worktree at `a71165e20`, `origin/master` at `b48fa7413`, and
GitHub. Corrections and additions from that review are integrated in place and
summarized in section 8.

### 1. Outcome and scope

Restore the observable behavior of combat that existed before the event-core
refactor while retaining the current event architecture, lifecycle protections,
and unrelated changes. The implementation must restore combat timing, action
availability, attack allocation, command/attack queues, casting interactions,
reactions, and combat consequences. Making only the fremlin example work is not
sufficient.

Keep the current native scheduler, reactor, generation-aware entity handles,
encounter ownership, typed domain events, activity manager, damage/death result
interfaces, bounded reaction processing, and demand-driven NPC scheduling. Keep
later features, including readied actions, counterspell, ally defense, tactical
hazards, pet improvements, racial abilities, Four Arms, and casting-speed feats.
Existing combat rules must use the historical contract; features absent from that
baseline must remain functional under the restored timing model.

"Retain changes" does not mean preserving the new automatic-attack action tax or
whole-round attack batching. Those are mechanics policies inside retained systems.
Do not revert PRs, replace whole files with historical copies, restore a global
fighting roster, reintroduce heartbeat population scans, or add a second combat
engine. No new combat mode, environment selector, or player toggle is proposed.

Where a retained bug fix changes an observable historical outcome, record the
conflict explicitly. A change described as a bug fix is not automatically exempt
from the requested mechanics parity. Unsafe behavior such as stale dereferences
or unbounded recursion is not a useful parity target; retain the protection and
identify any resulting gameplay difference. Section 4 lists known cases requiring
particular care.

### 2. Pinned historical reference

Use these immutable revisions, rather than interpreting "two weeks ago" as a date
filter:

| Role | Revision | Reason |
| -- | -- | -- |
| Pre-refactor mechanics reference | `fbe9366fbde8180b49a211cbcfdf2b2b531a15bd` | 2026-08-29 documentation-only commit, immediately before the first scheduler implementation, `b099838ae`. Its parent, `8cac912bf`, has the same source. |
| Last master revision before integration | `668c9d4adb5601466407c65d335493368ea11fd6` | First parent of the actual #85 merge. The inspected combat, actions, queues, interpreter, spell parser, MUD event, and main-loop files are identical to the reference above. |
| Event-core integration | `3ddc1efd7780edd0f8f7d83f870e800c54d49c81` | #85 merged on 2026-09-05. This is the integration boundary, not the first implementation commit. Its second parent, `34ab01685`, is the #85 review head. |
| Branch point of this plan | `a71165e20afc49a3461d38df3876fb45df52c6bc` | Starting revision of the `revert-combat` branch, which also exists on `origin` at this commit. |
| Implementation start | `cff3350f2` | Plan commit rebased onto freshly fetched `origin/master` at `9c5a0223f` on 2026-09-16. Includes the presentation split and subsequent string-safety changes. |
| Latest master at review time | `b48fa741384b565a665dd16c3eb5aeb7f19a2e4c` | 2026-09-16, thirty commits after the branch point. Includes the #193 merge `886703f1b`, which moved combat presentation out of `src/combat/fight.c` into `src/combat/combat_messages.c`, and the later string-safety commits `805b20496` and `773261f7d`, which touched `src/core/comm.c`, `src/core/interpreter.c`, `src/combat/encounters.c`, and `src/events/ready_action.c`. None of those changes touch combat timing, actions, or queues. Implementation starts by rebasing `revert-combat` onto master and recording the new start revision here. |

The baseline is available as a
[pinned source tree](https://github.com/LuminariMUD/Luminari-Source/tree/fbe9366fbde8180b49a211cbcfdf2b2b531a15bd/src).
`b099838ae^` resolves to that baseline. The comparison with `668c9d4ad` was checked
for `src/combat/fight.c`, `src/combat/act.offensive.c`, `src/actions.c`,
`src/actions.h`, `src/actionqueues.c`, `src/interpreter.c`,
`src/magic/spell_parser.c`, `src/mud_event.c`, and `src/comm.c`.

The review stack and the integration history must not be treated as independent
revert candidates. GitHub metadata identifies #67 and #70-84 as closed, unmerged
review PRs, with the work consolidated through #85. The following maps every PR
identified in the request; review-head hashes identify review boundaries, not
separate master merges.

| PR | Review head or actual merge | Relevance |
| -- | -- | -- |
| [#67](https://github.com/LuminariMUD/Luminari-Source/pull/67) | Review head `310a648c8` | Specification review; its base already contains scheduler work, so it is not the old-combat reference. |
| [#70](https://github.com/LuminariMUD/Luminari-Source/pull/70) | Review head `dc8f57c35` | Scheduler foundation. |
| [#71](https://github.com/LuminariMUD/Luminari-Source/pull/71) | Review head `7919cf0b9` | Owner lifecycle and reactor bridge. |
| [#72](https://github.com/LuminariMUD/Luminari-Source/pull/72) | Review head `7bc2d9d67` | Typed domain events; pubsub retired. |
| [#73](https://github.com/LuminariMUD/Luminari-Source/pull/73) | Review head `3418e4772` | NPC, object, and DG owners. |
| [#74](https://github.com/LuminariMUD/Luminari-Source/pull/74) | Review head `f07ec8120` | Character and room owners. |
| [#75](https://github.com/LuminariMUD/Luminari-Source/pull/75) | Review head `e0d46dad4` | Vessel/hour work; preserve ownership and noncombat behavior. |
| [#76](https://github.com/LuminariMUD/Luminari-Source/pull/76) | Review head `81c84fa01` | Encounter scheduling and changed combat-round semantics. |
| [#77](https://github.com/LuminariMUD/Luminari-Source/pull/77) | Review head `7ec6844fb` | Activity ownership and combat/action admission. |
| [#78](https://github.com/LuminariMUD/Luminari-Source/pull/78) | Review head `eed0450d8` | Handle migration and lifetime protections. |
| [#79](https://github.com/LuminariMUD/Luminari-Source/pull/79) | Review head `eaf51d14d` | Removal of heartbeat discovery. |
| [#80](https://github.com/LuminariMUD/Luminari-Source/pull/80) | Review head `3f1bbb31d` | Demand-driven architecture enforcement. |
| [#81](https://github.com/LuminariMUD/Luminari-Source/pull/81) | Review head `35ef02049` | Native runtime ownership. |
| [#82](https://github.com/LuminariMUD/Luminari-Source/pull/82) | Review head `dd3beca1d` | Native DG, MUD timer, and AI jobs. |
| [#83](https://github.com/LuminariMUD/Luminari-Source/pull/83) | Review head `2b2a11066` | Native architecture enforcement. |
| [#84](https://github.com/LuminariMUD/Luminari-Source/pull/84) | Review head `05b77bf90` | Damage, reactions, death, and deliberate offhand timing changes. |
| [#85](https://github.com/LuminariMUD/Luminari-Source/pull/85) | Merge `3ddc1efd7` | Consolidated integration, including subsequent casting/readied-action work. |
| [#134](https://github.com/LuminariMUD/Luminari-Source/pull/134) | Merge `8983b9dee` | Counterspell, ally readiness, effect clocks, hazards, and other native repairs. |
| [#139](https://github.com/LuminariMUD/Luminari-Source/pull/139) | Merge `d513bc638`; fix `2713c0428` | Keeps RoL special-procedure activity scheduled during combat. |
| [#193](https://github.com/LuminariMUD/Luminari-Source/pull/193) | Merge `886703f1b` | Presentation split into `combat_messages.c`; move-only for this plan's behavioral diff. |

The selected baseline predates the #70-74 infrastructure segments as well as the
combat segments.

Useful change points already located:

- `a8333d8bb`: encounter-owned combat scheduling.
- `67ae730bc`: semantic combat rounds, action/reaction budgets, queue turn limits,
  automatic-attack action spending, and the staggered attack collapse in
  `perform_attacks()`. This is the principal mechanics change.
- `0d86b4d2b`: primary activity manager.
- `25770ccca`: timed casting through native activities.
- `c383917c8` (2026-09-05): removed the `LUMINARI_COMBAT_EVENTS` and
  `LUMINARI_COMBAT_ROUNDS` boot selectors. Since then
  `configured_encounter_mode()` and `configured_semantic_rounds()` in
  `combat_encounters.c` return true outside `LUMINARI_CUTEST`; only test seams
  select the other paths.
- `e64c773b9`: tactical readied attacks against casting.
- `cbb7f89fe`, `e72c62097`, `0f687abc1`, `23464c711`: stable turn clocks, Defensive
  Casting, Bleeding Critical, and cloud exposure in the #134 work.

Historical `actions.c`, `actionqueues.c`, `mud_event.c`, and activity/runtime files
now live under `src/events/`; `comm.c`, `interpreter.c`, `limits.c`, and `utils.c`
now live under `src/core/`. Combat and magic files already occupied their subsystem
directories in the baseline, and #193 later split `combat_messages.c` from
`fight.c`. Use `git show REV:path` and inspect move-aware diffs; file moves,
formatting, and later feature additions are not mechanics reversions.

### 3. Confirmed behavior differences and preserved behavior

The source proves the following contracts. Runtime timing and complete output
still need the validation described below.

| Area | Before the refactor | Current source and restoration implication |
| -- | -- | -- |
| Opening attack | `do_hit()` rolls initiative and immediately calls `hit()` for the winning side, with visibility/posture exceptions and existing initiative/reach bonuses. | This opening path and its message still exist in `src/combat/act.offensive.c`. Preserve and test it; an absent initiative message in one random transcript does not prove its removal. |
| First scheduled attack phase | `set_fighting()` chooses a 2- or 4-second initial delay from its initiative comparison and attaches the per-character round event with phase "1". | `set_fighting()` still computes that delay and passes it to `combat_encounter_join()`, but `activate_participant()` in `combat_encounters.c` discards it under semantic rounds and uses the encounter's next shared round, initially six seconds. The compatibility path honors the delay but adds a six-second `COMBAT_ENCOUNTER_JOIN_GUARD` for joins during a callback. Restore the historical delay and participant offsets without discarding encounter ownership. |
| Sustained attacks | `event_combat_round()` calls `execute_next_action(ch)` then `perform_violence(ch, phase)` with phases 1, 2, 3, advancing every two seconds. Attack types/counts are allocated across those phases. | `combat_run_semantic_round()` calls `perform_violence(ch, 0)` every six seconds, resolving the entire rotation together. The encounter already carries per-participant `phase`/`next_due` state and a two-second `COMBAT_ENCOUNTER_PHASE_DELAY` for its compatibility path. Restore phase allocation and interleaving, not merely average damage per six seconds. |
| Automatic attack cost | `perform_attacks()` in the normal routine only checks `is_action_available()` for standard, and for move outside phase 1; nothing in the routine consumes actions. Explicit abilities retain their own costs. | The current branch records available actions, calls `perform_attacks()`, and when `combat_encounter_semantic_manages()` is true consumes standard and usually move. `perform_attacks()` also collapses phase 0 to phase 1 for staggered or move-less combatants. Remove that policy while retaining costs inside commands and abilities. |
| Action recovery | `start_action_cooldown()` attaches elapsed-time action events. The standard macros request six seconds; callers may specify other durations. Recovery notices come from the action-event expiry callbacks in `actions.c`. | `combat_encounter_action_consume()` rounds durations up to whole turns through `semantic_rounds_for_delay()`. `import_semantic_state()` cancels the three action events and the four round-flag events on join; the departure path recreates only the action events, rounded. `combat_encounter_action_query()` is a second action balance consulted first by `is_action_available()`, so the prompt, `command_actions_available()`, and `update_msdp_actions()` all read it. Notices come from `announce_recovered_actions()` at turn start. Restore exact remaining deadlines across all those transitions. |
| General command queue | `execute_next_action()` runs before each combat phase and from ordinary command-loop servicing when the head command becomes eligible. | `combat_encounter_intent_claim()` restricts it to one intent during a semantic turn, and two `comm.c` sites (the wait-deadline computation and the queue-service branch) skip fighters for whom `combat_encounter_semantic_manages()` is true. Restore eligibility-based progress with native wakeups and existing input/wait rules. |
| Attack queue | `kick` is an `ACTION_NONE` command routed through `do_process_attack()`, which queues an attack. The next applicable `hit()` dispatches the queued attack instead of the default hit. | That separate queue still exists: `hit()` calls `resolve_hit()` with queued dispatch enabled, and only `combat_readied_attack()` disables it. Preserve its replacement semantics; do not merge it with the general command queue or invent a new action cost to fix the transcript. |
| Casting admission | `cast` requires `ACTION_MOVE`; `command_actions_available()` can substitute standard when move is unavailable. Its queue preflight is `NULL`, so an unavailable action already caused the same wait message. The interpreter gate applies only to players. | Both actions now become unavailable after automatic attacks. Fix availability rather than bypassing the command gate or making every spell automatically queue. Staff skip casting time entirely, so proof needs a mortal player. |
| Casting progress | Timed casting has its own event, initially one second for PCs and two for NPCs, with its existing checks/costs. `IS_CASTING` suppresses automatic attacks. | The native casting activity preserves those declared step intervals and wall-clock ownership. Retain it and prove the interpreter can reach it during a real fight. |
| Reaction/reset cadence | `perform_violence()` resets AoO count, Energy Retort use, demoralizing-strike state, and fear affects on each phase. Cowering and Perfect Tempo ran in `proc_d20_round()` from the heartbeat every `PULSE_VIOLENCE`. Other effects have separate clocks. | Semantic rounds reset AoO budgets at a shared six-second boundary through `prepare_semantic_round()` and the bounded `combat_encounter_reaction_try_use()` API. Cowering and Perfect Tempo were duplicated into `combat_run_semantic_round()`, and `proc_d20_round_one()` skips managed fighters. Recover each historical reset rule; comments saying "per round" are insufficient evidence. |

Relevant current code:
[combat driver](../../src/combat/combat_encounters.c),
[combat mechanics](../../src/combat/fight.c),
[action deadlines](../../src/events/actions.c),
[general queue](../../src/events/actionqueues.c),
[command admission](../../src/core/interpreter.c),
[command servicing](../../src/core/comm.c), and
[casting](../../src/magic/spell_parser.c).

The likely starvation sequence is directly visible in the current code:

1. A turn starts and `announce_recovered_actions()` prints the standard and move
   recovery notices together, which is why the bad transcript shows both lines
   immediately before each attack burst.
2. The automatic attack routine runs in that same callback.
3. It spends standard and move again before another player command is serviced.
4. A subsequent `cast` fails the move-or-standard requirement.
5. `kick` can still enter the separate attack queue because its command-table
   action requirement is `ACTION_NONE`.

This explains the supplied symptoms without assuming a missing spell handler.
It is a source-derived diagnosis, not a claimed live reproduction.

The sample characters have different names, HP, and potentially preferences.
`show_combat_roll()` / `send_combat_roll_info()` still gate `[R:...]` and numeric
damage on `PRF_COMBATROLL`; charmed followers have a separate preference. The
`[smw]` prompt segment depends on `PRF_DISPACTIONS`. Match combat-roll,
condensed-output, prompt, color, client, equipment, feats, and casting
configuration before comparing output. Do not force those settings on all players
or require the same random rolls, XP, time-of-day, or flavor lines as the sample.

### 4. Full parity inventory and known conflicts

Before changing mechanics, classify every relevant difference from the baseline
through #85, #134, #139, #193, and the implementation start revision. Maintain
that disposition in this document during implementation: historical contract,
current difference, source/caller, intended restoration, retained
protection/feature, and proving test. Every difference must end as restored,
retained with a concrete reason, or an explicit unresolved parity conflict. Do
not hide the last category behind green tests.

The inventory must extend beyond the rows above:

- Trace player and NPC entry through hit/kill, assist, spell damage, ranged/thrown
  attacks, scripted damage, target switches, and automatic aggression. Preserve
  initiative roll sites, flat-footed transitions, reach and Improved Initiative,
  legality checks, existing PvP consent, peaceful/single-file rooms, and attack
  consequences. Do not introduce a new initiative formula or reroll policy.
- Enumerate main/offhand, unarmed, natural/evolution, ranged, reload, thrown,
  haste/slow, two-weapon, flurry, vital-strike, cleave, grappling, and extra-attack
  paths. Compare ordered attacks, bonuses, resource use, and targets, not only
  totals. Keep later Four Arms and racial additions at their current capabilities.
- Trace all phase-sensitive side effects: fear/confusion, cowering, autostand,
  Warbeat, Perfect Tempo, Deflective Screen, Smash Defense, Relentless Assault,
  Unstable Mutagen, AoOs, Energy Retort, DG fight/hit triggers, and special
  procedures. For example, baseline Unstable Mutagen checks occur in the phase
  callback despite its "per round" comment; Deflective Screen uses a ten-second
  marker; Warbeat, Deflect Arrows, Smash Defense, and autostand key on phase 1.
  Avoid silently normalizing these to six-second turns.
- Audit `src/core/limits.c`, `src/events/character_periodic.c`,
  `src/events/affected_owners.c`, `src/combat/tactical_effects.c`, and relevant
  magic/perk code for effect duration, bleeding, regeneration during combat,
  Defensive Casting, cloud exposure, and directional walls. Distinguish a legacy
  phase, a six-second `PULSE_VIOLENCE` update, an individual effect deadline, and
  a native activity step. Retain owner scheduling while restoring established
  effect magnitudes, frequency, attribution, and expiry.
- Compare direct, reflected, transferred, periodic, and lethal damage through
  `combat_damage.c`, `combat_reactions.c`, `combat_state.c`, and the death paths in
  `fight.c`. Include saves, resistances, DR, concentration, target redirection,
  combat stopping, XP, kill credit, corpse/loot creation, quests, and triggers.
  Keep the later common reward API and exactly-once notifications.

Specific conflicts already found must have named tests and dispositions:

| Difference | Required disposition |
| -- | -- |
| Offhand phase allocation | The historical offhand clauses put attacks numbered 3, 6, 9, 12, and 15 in phase 1. The new `attack_number_runs_in_phase()` puts them in phase 3, and `Test_compatibility_attack_numbers_map_to_one_phase` asserts the new mapping. This is a finite, observable mechanics change even though it fixes a typo. Baseline timing is the restoration default for existing attacks; retain the helper/safe structure and test the chosen mapping. Preserving the new mapping would be an explicit exception to exact parity. |
| Staggered and move-less attack collapse | `67ae730bc` added a clause to `perform_attacks()`: in phase 0, a staggered combatant or one without a move action is collapsed to the phase 1 portion. The baseline only coupled staggered costs inside `start_action_cooldown()`, and a staggered combatant with a move action still attacked in every phase. Once phases return, the normal routine has no phase 0 caller; remove the clause or classify the residual difference, and assert staggered attack order. |
| Cowering and Perfect Tempo cadence | Baseline: `proc_d20_round()` in `src/limits.c`, called from the heartbeat every `PULSE_VIOLENCE`. Current: duplicated at the top of `combat_run_semantic_round()`, while `proc_d20_round_one()` in `src/core/limits.c` skips managed fighters. `src/events/character_periodic.c` already schedules `proc_d20_round_one()` on that cadence, so restore the single six-second owner and delete the duplicate. Do not run either check per phase. |
| Test-only rollback paths | Three drivers exist today: semantic rounds, the encounter compatibility phases (`run_compatibility_phase()` and `COMBAT_ENCOUNTER_PHASE_DELAY`), and the per-character `event_combat_round` MUD event, which only runs when `encounter_mode` is false under CuTest. Cancel sites for `eCOMBAT_ROUND` remain in `src/act/act.other.c`, `src/magic/spells.c`, and `src/act/act.wizard.c`. End with one production driver, and decide explicitly whether the legacy callback, the test selectors, and `Test_combat_encounter_rollback_selector_keeps_legacy_path_exclusive` are deleted or kept as test seams. |
| Deferred reactions | Nested damage now yields `COMBAT_DAMAGE_QUEUED` / legacy zero and drains later. Trace callers that depend on immediate HP, death, relocation, or return values. Restore required observable continuation ordering within bounded, handle-validated processing; do not restore recursive unbounded `damage()` calls. Queue-only FIFO tests cannot establish gameplay parity. |
| Life Shield and Greater Hostile Juxtaposition | Keep recursion guards and safe spell-affect lookup. Separately classify corrected activation/charge behavior and reflect timing against the baseline; do not treat every behavior change in a safety patch as invisible. |
| Divine Sacrifice and killer-less death | Keep valid lifetime checks and typed causes/outcomes. Explicitly document the change from ignored/unresolved deaths to correctly finalized deaths, including transferred lethal damage. Do not recreate a crash or leave dead entities active to imitate a faulty old path. Such retained outcomes must be visible in the parity disposition. |
| Death notification ordering | Trace the final integrated `raw_kill_with_cause()`, which publishes through `domain_event_runtime_character_died_with_cause()`, and its actual `DOMAIN_EVENT_CHARACTER_DIED` subscribers: `combat_encounters.c` (priority 20), `activity_manager.c` (priority 100), `ready_action.c`, and `magic/buff_sequence.c`. Intermediate refactor commits changed ordering again; do not implement from an isolated commit or PR description. Preserve coherent, exactly-once cleanup and verify listener-visible state. |

The goal is baseline behavior for well-defined existing combat, with retained
safety guarantees and new features. Any remaining finite mechanics deviation
requires an explicit resolution before claiming exact parity. This plan does not
silently approve those deviations.

### 5. Implementation sequence

#### Step A: Establish behavioral evidence

- [ ] Rebase `revert-combat` onto `origin/master` (at least `b48fa7413`), record
  the resulting start revision in section 2, and finish the disposition inventory
  above. Treat the #193 presentation split as move-only. Exclude move/format-only
  changes from the behavioral diff, while retaining all subsequent feature code.
- [ ] Turn the supplied good/bad examples into repeatable scenarios using equivalent
  ordinary player characters and a controlled opponent. Staff action/casting
  exemptions must not mask the bug.
- [x] Add failing production-linked regressions for sustained combat followed by
  `cast 'mage armor' me`, then `kick`, plus phase timing and elapsed cooldowns.
  Drive `command_interpreter()` and advance the actual native scheduler. Tests
  that call `cast_spell()` directly cannot prove command admission works.
- [ ] Use controlled random outcomes and logical pulse advances for timing checks.
  Record ordered attacks, action state, cast transitions, queue sizes, HP, and
  terminal effects. Reuse current fixtures and test seams; do not copy the combat
  implementation into a standalone test harness.

Existing seams to reuse: `event_test_advance()` in `src/dgscript/dg_event.c`
runs the scheduler once at the current `pulse`; `combat_encounter_test_select()`,
`combat_encounter_test_select_semantic()`, and
`combat_encounter_test_set_phase_callback()` in `combat_encounters.c` select and
observe the driver; `circle_srandom()` seeds deterministic rolls;
`encounter_test_begin_semantic()` in `test_combat_encounters.c` and
`begin_gameplay_fixture()` in `test_gameplay_e2e.c` build rooms and actors. The
gameplay fixture's `initialize_test_npc()` flags actors as NPCs, and the
interpreter action gate runs only for players, so the cast regression needs the
connected-descriptor player pattern `test_gameplay_e2e.c` already uses in its
pet damage-feedback test (`MOB_ISNPC` removed,
`descriptor.connected = CON_PLAYING`).

A historical executable is optional supplementary evidence, not the source of a
new production path. If needed later, build it in an isolated development
snapshot with isolated database/world/player data. Never run it against current
production or newer saves. Only one local MUD at a time uses port 4100; an old
comparison run does not justify another game port.

#### Step B: Restore phase scheduling and action deadlines together

Primary files: `src/combat/combat_encounters.c`, `src/combat/fight.c`,
`src/events/actions.c`, and the associated headers only where needed.

- [ ] Adapt the existing encounter participant `phase`, `next_due`, and due-list
  machinery to dispatch phases 1, 2, 3 at the historical per-character offsets.
  The initial delay is already plumbed from `set_fighting()` into
  `combat_encounter_join()`; restore it inside `activate_participant()` rather
  than adding a parameter. Reuse/reconcile `combat_run_compatibility_phase()`
  with the current safe path; do not copy the old event queue or retain two
  production implementations.
- [ ] Preserve one combat-driver event per active encounter. Wake at the earliest
  actual participant/required consumer deadline. Encounter merges must retain
  pending attack deadlines and phase identity; joining must neither reset other
  fighters nor give an extra immediate attack. Compare the current six-second
  callback-join guard with the baseline instead of assuming it was historical.
- [ ] Remove the automatic standard/move spend after ordinary attack generation
  and the staggered/move-less phase collapse. Retain explicit command, special
  attack, spell, ready-action, and activity costs. A kill ending membership must
  not create a new automatic-attack cooldown on exit.
- [ ] Use the existing native action events as the authoritative elapsed-time
  availability/deadline state. Remove whole-turn rounding and combat-entry/exit
  cancellation/recreation of these deadlines, including the round-flag import of
  `ePERFECT_TEMPO_HIT_THIS_ROUND`, `eDEFLECTIVE_SCREEN_HIT_THIS_ROUND`,
  `eSMASH_DEFENSE`, and `eRELENTLESS_ASSAULT`. If action query APIs remain, make
  them report that same state instead of keeping a second action balance.
- [ ] Preserve standard-for-move substitution, swift independence, full-round
  costs, staggered coupling, and each caller's actual duration. Do not infer units
  from command-table numbers or replace all costs with one constant.
- [ ] Keep queue dispatch and automatic attack checks in their historical order.
  Revalidate actors/targets after callbacks that can kill, move, or extract them.

Do not simply set `configured_semantic_rounds()` to false. It is unconditionally
true outside CuTest, and current activities, readied actions, tactical effects,
and tests use turn snapshots and hooks that would then be bypassed. Preserve the
existing logical full-round clock where a retained consumer requires it,
separately from attack phases and action deadlines. Reuse existing clock fields
and owner events; every additional scheduled callback must have a traced
consumer. A logical turn hook must not fire three times merely because attacks
again have three phases.

#### Step C: Restore queue and casting interaction

Primary files: `src/core/interpreter.c`, `src/core/comm.c`,
`src/events/actionqueues.c`, `src/events/actions.c`,
`src/events/activity_manager.c`, `src/magic/spell_parser.c`, and relevant attack
entry points in `src/combat/act.offensive.c` / `src/combat/fight.c`.

- [ ] Remove the one-general-intent-per-semantic-turn restriction for historical
  command queue behavior. Let an eligible head command progress at the appropriate
  native dispatch boundary, including between attack phases. Change both
  `comm.c` sites together; they run under both `LUMINARI_IO_DRIVER_SELECT` and
  `LUMINARI_IO_DRIVER_LIBEVENT`. Reuse action recovery and existing
  command-service wakeups; avoid global polling or an idle retry loop.
- [ ] Preserve FIFO ordering, queue limits, existing non-mutating preflights,
  unavailable-action messages, input priority, wait-state deadlines, and editor/
  pager restrictions. Validate targets/conditions again when executing delayed
  commands. Do not add cast queueing merely because the transcript contains a cast.
- [ ] Keep the attack queue independent. Verify `do_process_attack()` admission,
  `hit()` replacement, and the eventual `do_kick()`/other skill execution. Include
  multiple queued attacks, explicit targets, target loss, clear/list commands,
  failed prerequisites, and interactions with ranged and reactive attacks.
- [ ] Keep timed casts on the native activity owner, preserving player/NPC step
  rates, preparation/slot/PSP costs, quickened/instant spells, metamagic, and both
  values of `CONFIG_SPELLCASTING_TIME_MODE`, a runtime `cedit` setting rather
  than a header. Retain casting-start identities, interruption, concentration,
  readied-counterspell hooks, and exactly-once completion.
- [ ] Prove a player's spell begins after ordinary attacks without waiting for an
  action that those attacks should never have consumed. While casting, preserve
  suppressed automatic attacks; afterward resume the remaining schedule without
  a duplicate round or catch-up burst.
- [ ] Make prompt `[smw]` state, availability queries, recovery notices, and MSDP
  agree. Fix state/notification timing rather than hiding the repeated messages.

#### Step D: Complete mechanics parity and retained-feature integration

- [ ] Implement every restoration in the disposition inventory, including attack
  allocation, phase side effects, reaction ordering, and effect clocks. Complete
  the section 4 conflict dispositions before declaring mechanics parity.
- [ ] Update logical-turn consumers deliberately: `primary_activity_on_semantic_turn`,
  `ready_action_on_semantic_turn`, `combat_encounter_get_turn`, and the
  `tactical_defense_on_turn`, `tactical_bleeding_on_turn_end`, and
  `tactical_room_hazards_on_turn_end` hooks. Existing mechanics use their baseline
  clocks; newer features retain their declared lifecycle and costs. Neither
  disabling the hooks nor applying every effect at every attack phase is
  acceptable. `Test_primary_activity_turn_hook_is_semantic_only` and
  `Test_primary_activity_semantic_turn_requires_existing_action_budget` pin the
  activity side of that contract.
- [ ] Preserve #139's independent mobile-activity work in `src/mob/mob_act.c` and
  the active-world owner. Keep RoL monster special activity during fights while
  wandering/scavenging and unrelated autonomous work remain suppressed. Separate
  activity callbacks from the `spec_gateway_mobile_combat_turn()` call inside
  `perform_violence()` to prevent double dispatch.
- [ ] Verify ordinary NPC melee, NPC casting, waits, aggression, groups, automatic
  assist, charmies, pets, and followers. Preserve subsequent pet policies and
  racial/weapon features rather than restoring their old entire files.
- [ ] Verify fight teardown, flee, relocation, opponent switches, disconnect,
  target extraction, death, and shutdown/copyover. Cancel transient work exactly
  once, retain the intended durable cooldown/effect state, and never resolve a
  stale entity handle or reset a spent action by leaving/rejoining combat.

Steps B-D are one coherent restoration before release. A temporary state in which
phased attacks work but casting, tactical effects, or NPC hooks are disabled is
not a completed fix.

#### Step E: Align tests, documentation, and help

- [ ] Update semantic-round tests whose expected behavior is explicitly being
  restored: `Test_combat_semantic_round_starts_six_seconds_after_idle_join`,
  `Test_combat_semantic_join_uses_next_shared_round`,
  `Test_combat_semantic_round_transfers_action_cooldown_across_combat`,
  `Test_combat_semantic_round_dispatches_one_buffered_intent`,
  `Test_combat_semantic_merge_coalesces_offset_clocks`,
  `Test_combat_semantic_staggered_spend_couples_standard_and_move`,
  `Test_combat_semantic_round_owns_action_and_reaction_budgets`, and
  `Test_compatibility_attack_numbers_map_to_one_phase`. Keep their ownership,
  lifetime, admission, and cancellation coverage, including the
  `Test_combat_encounter_*` lifecycle cases. Replace changed gameplay assertions
  with baseline-derived assertions; do not delete failing tests wholesale or
  select test-only compatibility mode as proof.
- [ ] Extend existing production-linked suites first:
  `test_combat_encounters.c`, `test_combat_production.c`,
  `test_activity_manager.c`, `test_gameplay_e2e.c`,
  `test_domain_events.c`, `test_spells_skills_production.c`, and the readied,
  counterspell, ally-readiness, Defensive Casting, and Bleeding Critical cases
  already in `test_gameplay_e2e.c` and `test_domain_events.c`. Use the ordinary
  production selection for end-to-end regressions, without replacing combat with
  a phase recorder.
- [ ] Update [COMBAT_SYSTEM.md](../systems/COMBAT_SYSTEM.md),
  [MUD_EVENTS.md](../systems/MUD_EVENTS.md),
  [TACTICAL_EFFECT_CLOCKS_AND_HAZARDS.md](../systems/TACTICAL_EFFECT_CLOCKS_AND_HAZARDS.md),
  [COUNTERSPELL_AND_ALLY_READINESS.md](../systems/COUNTERSPELL_AND_ALLY_READINESS.md),
  and affected current activity/command documentation to describe the final
  behavior. The combat document currently describes semantic budgets and a
  `LUMINARI_COMBAT_ROUNDS=compatibility` boot rollback that has had no
  implementation since `c383917c8`; remove that claim and do not use the document
  as the baseline mechanics authority.
- [ ] Update affected help entries in both the development database and
  `lib/text/help/help.hlp`. The SQL sources are
  `sql/components/help_semantic_combat_entries.sql` with
  `verify_help_semantic_combat_entries.sql`, `help_counterspell_readiness.sql`,
  `help_bleeding_critical_clock.sql`, `help_defensive_casting_clock.sql`, and
  `help_billowing_cloud_exposure.sql`. Follow the `help_<feature>_entries.sql`
  plus read-only `verify_help_<feature>_entries.sql` pattern in
  `docs/systems/HELP_SYSTEM.md`; new SQL must pass sqlfluff and contain no
  `DELIMITER` block. The current entries describe six-second turns, one queued
  command per turn, joining on the next round, and rounded durations:
  - `COMBAT COMBAT-PHASE COMBAT-ROUNDS FIGHTING`
  - `ACTION-QUEUE QUEUE`
  - `ACTIONS ACTIONS-IN-COMBAT SMW`
  - `ATTACK-QUEUE`
  - `INITIATIVE INITIATIVE-ORDER`
  - `READIED-ACTION READY COUNTERSPELL`
- [ ] Update diagnostics and counters where their meaning changes from a full
  attack turn to a phase callback. Keep historical acceptance reports under
  `docs/testing/` unchanged; document fresh restoration evidence in this plan or
  the existing test-doc area.
- [ ] When the work settles, follow `docs/ongoing-projects/README.md`: move the
  enduring content into the system documents above, file any remaining work as
  GitHub issues, and delete this document.

### 6. Required acceptance coverage

All timing assertions use the pinned baseline for existing behavior and the
retained feature contract for additions. Compare events at due-minus-one, due,
and due-plus-one ticks, not arbitrary wall-clock sleeps.

| Case | Required evidence |
| -- | -- |
| Supplied user workflow | Ordinary PC enters melee, sustains several cycles, casts mage armor on self, receives progress/completion, queues kick, executes it in place of an eligible normal attack, and continues fighting. Exercise commands immediately before and after phase/action deadlines. |
| Opening and sustained initiative | Controlled win/loss/tie, unseen or incapacitated victim, Improved Initiative, reach, late join, target switch, and assist. Opening strikes and subsequent per-character phase offsets match baseline. |
| Automatic attacks and actions | Idle melee leaves standard/move/swift available unless another real action/effect consumes them. Explicit cooldowns block only the historical actions and expire at the original deadlines, including across combat entry, exit, merge, and reentry. |
| Partial actions and attack counts | No standard means no normal routine; spent move suppresses the appropriate later phases; a staggered combatant with a move action still attacks in every phase. Cover staggered, swift, full-round costs, dual wield, high BAB, haste/slow, natural attacks, flurry, ranged/reload/thrown, and later extra-arm attacks. Assert order as well as totals. |
| General queue | Valid queued command executes when eligible, without a six-second turn-only gate. Cover FIFO, full queue, invalid preflight, command input priority, wait state, editor/pager, stale target, cancellation, and no busy loop. |
| Attack queue | Kick and another maneuver replace the correct hit without an extra attack or double cost. Cover several queued attacks, no target, changed/dead target, list/clear, and opening versus ongoing combat. |
| Casting and activities | Real interpreter admission by a mortal player, PC/NPC progression, zero-time/quickened spells, resources consumed once, no autoattacks during casting, success/failure of concentration, final-deadline damage, target movement/death/extraction, and clean recast. Test both casting-time modes. |
| Retained readiness | Cast-triggered ready attacks, counterspell, designated-ally defense, and their expiry/reservation/cost behavior still work. One cast/attack fact must not produce duplicate reactions after restoring phases. |
| Phase side effects and effects | AoO/reset budgets, Energy Retort, fear/cowering, Perfect Tempo, Deflective Screen, Smash Defense, mutagen, bleeding, Defensive Casting, affect expiry, cloud/wall exposure, and regeneration retain the correct individual cadence and attribution. Cowering and Perfect Tempo fire once per six seconds, not per phase. |
| Damage and terminal outcomes | Baseline damage/save/DR results on ordinary paths; bounded Life Shield/retort chains; juxtaposition and sacrifice; death during a queued action or reaction; exactly one corpse, reward, quest credit, and death/cleanup notification. Test the explicit section 4 dispositions. |
| NPCs and companions | Melee, spellcasting, special procedures, DG triggers, RoL activity during fights, group assist, pets, and charmies run at the intended frequency. Retain `TestActiveWorldKeepsRolSpecialActivityScheduledDuringCombat`. |
| Lifecycle and persistence | Callback-time joins/merges, every participant removed in a callback, flee/move/stop, disconnect, extraction and generation reuse, copyover, and shutdown leave no stale callback, ghost fight, free cooldown reset, or duplicate completion. |
| Output and clients | Same preferences yield the expected initiative/action/casting/queue messages; `[smw]` and MSDP match actual availability. Match mechanics and message order without freezing random flavor or player-specific numbers. |
| Architecture and load | One physical scheduler, no restored global scans, one combat-driver event per live encounter, bounded callbacks, and cleanup back to baseline. Idle populations remain dormant. Extra active-fight phase wakeups are expected and must be measured separately from a scheduling leak. |

Existing tests are useful but insufficient on their own. For example,
`Test_combat_semantic_round_owns_action_and_reaction_budgets` intentionally asserts
the new budget model, while `Test_casting_activity_completes_once_on_native_clock_in_combat`
starts a cast directly and synthesizes combat state. Neither establishes that a
normal player's `cast` command succeeds after automatic attacks in the default
runtime. Keep relevant lifecycle coverage and add the missing joined-up scenario.

### 7. Validation and completion gates

These are future implementation checks, not commands executed for this plan:

1. Run focused production-linked CuTests while implementing. Use the root
   `cutest` target and case-sensitive `CUTEST_FILTER`; verify the filter matches.
   Tests must exercise the default production path as well as helper boundaries.
2. Run `make -j"$(nproc)" test`, followed by `make install` so no root-level
   `luminari` binary remains. The full command ignores an exported filter and
   includes the native architecture, demand-driven, and admission checks. Record
   all failures/skips and their relevance; historical green reports are not fresh
   evidence of gameplay parity.
3. If adding/removing a source or test file, update both `Makefile.am` and
   `CMakeLists.txt`, including the CuTest source/test-file lists, and run
   `python3 scripts/ci/check_build_parity.py`. Validate the affected CMake build/test
   path when its manifest or build integration changes. Keep compiler warnings
   clean and use the repository pre-commit hooks on changed files.
4. Run focused memory checks on encounter teardown and nested damage/casting
   lifetimes using the existing setup: the `sanitizers` job in
   `.github/workflows/test.yml` configures with
   `CFLAGS='-O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer'` and runs
   `LUMINARI_TEST_ROOT="$PWD" ./cutest`; the `memory-check` job runs the same
   binary under Valgrind with `--leak-check=full`, `--track-origins=yes`, and
   `--error-exitcode=1`; CMake exposes the same flags through
   `LUMINARI_SANITIZERS`. Do not replace these scenarios with helper-only queue
   tests or silently omit lifecycle failures.
5. In development, verify environment identity before mutation/running. Never
   print or alter credentials or customized headers. Run the MUD with
   `MUD_PORT=4100 ./scripts/autorun/autorun.sh`; use equivalent ordinary PCs for the
   complete user workflow and representative multi-combatant/NPC scenarios.
   Record timed transcripts, action state, and relevant `eventdebug`/`perfmon`
   evidence. Recheck the involved paths with both supported I/O drivers if their
   command-wakeup behavior is changed.
6. Check retained noncombat work through the full suite and the existing native
   architecture checks. Add a bounded active-combat/idle comparison using existing
   diagnostics when scheduling changes require it; a new performance framework or
   an unrelated full burn-in is not part of this restoration plan.
7. Verify both help stores and the final documentation describe the delivered
   mechanics. Audit the final diff against the baseline disposition and later
   feature inventory. Do not deploy, publish help to production, or rewrite
   history as part of this planning task.

Implementation is complete only when all of the following hold:

- [ ] The user workflow works through ordinary commands in the default runtime.
- [ ] Existing combat mechanics match the pinned baseline across the inventory,
  and any specifically retained safety-related differences are explicitly
  resolved and documented; no unclassified finite mechanics deviation remains.
- [ ] Newer features and event-system ownership/lifecycle guarantees remain intact.
- [ ] The production-linked regressions, full required suite, focused lifetime
  checks, and local gameplay evidence pass with limitations stated accurately.
- [ ] Documentation and both help stores agree with the implementation.
- [ ] Only the intended restoration and its necessary tests/docs/help are changed;
  no old engine, duplicate state owner, speculative switch, or unrelated cleanup
  has been added.

### 8. Ablation check

Applied before writing this plan. Keep the historical baseline, complete mechanics
inventory, focused restorations, retained-feature integration, and end-to-end
proof because each addresses an observed change or an explicit retention
requirement. Simplify implementation by reusing existing encounter phase/deadline
state, native action timers, activity owners, and production-linked test fixtures.

Drop wholesale PR reverts, copying old source files, a parallel combat engine,
new configuration switches, a generic compatibility framework, a new test runner,
and mandatory broad burn-in/performance infrastructure. None is required to
restore the requested mechanics. Do not reduce scope to only casting or only the
fremlin transcript; that would leave proven combat changes unaddressed.

The 2026-09-16 review removed no scope and added no new machinery. It simplified
the plan in four places: the initial-delay restoration reuses the value
`set_fighting()` already passes instead of new plumbing; cowering and Perfect
Tempo return to the periodic owner that already runs them for unmanaged
fighters instead of gaining a phase hook; the cast regression reuses the
existing connected-player fixture instead of a new harness; and the three
present-day drivers collapse to one production path with an explicit decision
about the test-only seams. It also corrected three inaccuracies: the current
revision is now the branch point with master's later #193 split recorded, the
casting-time modes are runtime configuration rather than headers, and the
`LUMINARI_COMBAT_ROUNDS` selector the combat document describes no longer
exists.

### 9. Implementation checkpoint

2026-09-16:

- Initial worktree was clean at `90b6ba714`. Rebased the plan commit onto
  `origin/master` (`9c5a0223f`), producing implementation start `cff3350f2`.
  No production mechanics have been changed yet.
- At the user's request, copied the local master checkout's complete `lib/`
  into this checkout with `rsync -a`, including hidden files and world data.
  Source and destination are development environments (`APP_ENV=development`).
  Existing customized configuration headers were preserved; there were no
  missing configuration source/header files. Compiled object files were excluded.
  Credentials were neither printed nor committed. This setup is local only;
  database identity and runtime readiness still need checking before gameplay.
- Ablation: reuse the connected-player gameplay fixture and native scheduler
  for the command regression, and existing encounter fixtures for phase and
  exact-deadline tests. No new test runner or production selector is needed.
- Added the failing default-runtime regressions below. Complete the source
  disposition inventory before changing production mechanics. Steps B-E and
  all completion gates remain open.

Additional source findings and ablation before the queue ownership repair:

- `resolve_hit()` removes an `attack_action_data` from the attack queue and calls
  its command, but never frees the entry or its duplicated argument. Valgrind
  reproduced one leaked allocation pair per dispatched kick through the new
  ordinary-command scenario. Free both after dispatch returns; the queue no
  longer owns them. This is a lifetime repair with no timing/rules change.
- The baseline `dg_event.c:queue_enq()` inserts before equal deadlines (`<`,
  not `<=`), so ties follow reverse scheduling order. The restoration phase
  regression pins that order. The current encounter comparison's initiative,
  dexterity, runtime-ID, and FIFO tie sorting is another finite timing
  difference to restore within the encounter, without replacing the scheduler.
- Baseline `event_create_named_with_cleanup()` clamps to one future tick, with
  no six-second callback-join guard. Restore the supplied initial delay for
  callback joins while retaining deferred membership mutation and handles.

Current regression evidence (production-linked `cutest`, no compatibility
selection in the new scenarios):

| Test suffix after `Test_combat_restoration_` | Current result and coverage |
| -- | -- |
| `default_preserves_individual_phase_deadlines` | Fails: no callback at the supplied two-second deadline. Pins two/four-second offsets, phases 1/2/3, equal-deadline order, and one encounter event. Uses the existing phase observer only for this scheduler boundary test. |
| `default_keeps_elapsed_action_deadline` | Fails: standard action is still unavailable at its original deadline after entry/exit/reentry. Checks due-minus-one, due, and due-plus-one for a non-round-aligned duration. |
| `mortal_cast_fixture_without_combat` | Passes: a connected mortal wizard casts mage armor through the interpreter, completes the native activity, and spends the preparation. Positive control for the combat fixture. |
| `mortal_cast_and_kick_seconds_mode` | Fails at cast admission after 18 seconds of real melee: "You must wait for the required action before trying that command." Subsequent completion/queue assertions remain unproven until the restoration. |
| `mortal_cast_and_kick_actions_mode` | Same admission failure with `CONFIG_SPELLCASTING_TIME_MODE=0`. |
| `queued_kicks_each_replace_one_hit` | Passes: two ordinary kick commands enqueue two entries, and two real `hit()` calls each dispatch one replacement. Valgrind reports zero errors and zero lost/possibly-lost bytes after the queue ownership repair. |

The two scenario files reuse existing fixtures and manifests; no source/test
file was added. `src/combat/fight.c:resolve_hit()` now frees the dequeued attack
and argument after the command returns. This is the only production change so
far. The command fixture supplies an actual prepared spell, both actors' attack
queues, a nonzero arcane preparation setting, and a playing descriptor. Neither
staff exemptions nor a direct `cast_spell()` call bypass the interpreter.

Commands/evidence available for continuation:

- `make -j"$(nproc)" cutest`: succeeds without compiler warnings.
- `CUTEST_FILTER=combat_restoration LUMINARI_TEST_ROOT="$PWD" ./cutest`:
  six selected, two pass and four expected restoration failures. Local output:
  `/tmp/revert-combat-regressions.log`.
- `CUTEST_FILTER=combat_restoration_queued_kicks valgrind --leak-check=full --track-origins=yes --error-exitcode=99 ./cutest`: passes. Local memory report:
  `/tmp/revert-combat-kick-valgrind.log`.
- The preliminary memory run of the intentionally failing scenarios also
  reported CuTest failure-message allocations. Do not count that red run as a
  passing lifetime gate. Repeat the full focused lifetime set after mechanics
  pass; do not suppress the failures or remove the regressions.
- `make -j"$(nproc)" test`: 1,511 CuTests run, 1,507 pass, and only the four
  new restoration regressions fail. The make target consequently returns 2;
  this is an intentionally red evidence checkpoint, not release validation.
  Native architecture and the other prerequisite checks passed. Local output:
  `/tmp/revert-combat-full-test.log`. No new compiler warnings were emitted.
- `make install`: passes after that full run; the root `luminari` artifact is
  removed and the local installed server includes the queue ownership fix.
  The MUD has not been started for this work, and help stores are unchanged.
- All applicable changed-file pre-commit hooks pass after formatting the new
  test code. No build manifests changed. No push or production action has been
  performed.

Next implementation boundaries:

1. Finish the disposition inventory in section 4. In addition to the known
   conflicts, preserve native handle checks, damage notifications, active-world
   reconsideration, periodic timer sync, common rewards, and the presentation
   split. Classify Greater Hostile Juxtaposition activation separately from
   safe affect lookup, and Life Shield's positive-damage gate separately from
   its recursion guard. The baseline greater-shield branch was unreachable;
   preserving its corrected activation is an observable exception, not merely
   a memory-safety repair.
2. Restore phase scheduling plus elapsed action events together. The existing
   encounter `next_round_due` and turn serial consumers must keep a distinct
   six-second logical clock: readied expiry, primary activities, Defensive
   Casting, Bleeding Critical, and room hazards cannot be called on every
   two-second attack phase. `combat_encounter_get_turn()` currently reports
   participant `next_due`; decouple that snapshot when `next_due` becomes an
   attack-phase deadline.
3. Restore native queue servicing in both `comm.c` sites and remove the intent
   restriction. Finish attack allocation/side-effect parity, then expand the
   acceptance coverage, both local help stores, documentation, and gameplay
   validation. The four red tests are the immediate targets, not the entire
   completion definition.
