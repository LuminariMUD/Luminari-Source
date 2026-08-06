# Artifact Mechanics Gap Audit

**Status:** Complete; ART-AUD-001 through ART-AUD-014 resolved

**Audited:** 2026-08-06

**Scope:** All 17 artifacts in the live LuminariMUD artifact registry

## Outcome

The audit originally found three omissions of identity-defining combat
mechanics from the executable source material:

1. Fade had no life-draining combat strike. This was resolved by ART-AUD-002.
2. Doombringer had no extra-attack combat burst. This was resolved by
   ART-AUD-003.
3. Avernus retained only its emergency full heal. Its life-stealing strike,
   automatic stand-up behavior, and minor combat heal were restored by
   ART-AUD-004.

All three confirmed identity packages are now present in code and deterministic
integration coverage.

The audit also found three independent runtime defects: lethal artifact damage
was not propagated back to the combat caller, Earthcrier calculated but ignored
its declared save DC, and Kelrom's hand-written proc made its advertised 14
percent generic proc unreachable. These defects affect more than source parity
and were prioritized before artifact placement. ART-AUD-001, the lethal-proc
boundary defect, was resolved first on 2026-08-06. ART-AUD-005 resolved
Earthcrier's save defect, and ART-AUD-006 now resolves Kelrom's timestamp
collision. All three runtime defects are closed.

ART-AUD-013 was resolved next on 2026-08-06. A production-linked identity
contract now names the combat handler and table-owned odds, active ability,
called-effect slots, invocation channels, generic proc chance, and progressive
passives expected for each of the 17 artifacts. Missing packages are explicit
contract entries rather than behavior inferred from nonzero generic
percentages.

ART-AUD-002 was resolved after that contract was in place. Fade now owns a
separate 1-in-16 siphon against living non-dragon NPCs while retaining its 16
percent generic proc as an independent mechanic.

ART-AUD-003 was resolved in the current remediation pass. Doombringer now owns
a separate 1-in-31, artifact-level-scaled extra-attack burst while retaining
its 20 percent generic proc.

ART-AUD-004 was resolved in the current remediation pass. Avernus now owns an
always-checked Bladesong survival layer plus a separate 1-in-31 life-transfer
strike while retaining its 15 percent generic proc.

ART-AUD-005 was resolved in the current remediation pass. Earthcrier now passes
the save-system level component that produces its Reflex DC of
`21 + artifact level + wielder Strength bonus + wielder Constitution bonus`,
rather than the unrelated artifact effect level. Its non-good wielder gate and
30-second recharge are disclosed in runtime info and player help. Earthcrier
has no generic proc competing for that timer.

ART-AUD-006 was resolved in the current remediation pass. Kelrom's healback
now uses its own persisted 30-second recharge, leaving its 14 percent generic
proc independently reachable. A no-heal attempt spends neither recharge nor
artifact XP. Copyover preserved both clocks, and controlled combat produced
both mechanics within one healback recharge window.

ART-AUD-008 is resolved. Wyrmfang now gains danger sense alongside haste at
artifact level 5, completing the six-state package carried by its Homeland
prototype without changing the level 1 through 4 progression curve. A
production-linked test covers the locked and active boundaries, the real
directional danger check, player-visible information, and clean removal.

ART-AUD-009 is resolved. Earthcrier and Wyrmfang are Large, so the ordinary
size calculation assigns two hands to a Medium bearer while preserving the
intended size-feat exceptions. Aegis retains the detailed body-worn
breastplate prototype shipped in the deployment package; its older one-line
shield rumor and the placement brief now agree with that playable identity.

ART-AUD-012 is resolved. Called-effect handlers now take their stacking group
from the validated `artifact_effects[]` row, Wyrmfang declares
`ART_STACK_WARD`, and the identity contract records all four group slots for
every artifact. A production-linked interlock test and a controlled live
invocation both proved that hunter's sight blocks Icedge's rime without
spending Icedge's recharge.

ART-AUD-010 is resolved. The generic percentage is now labeled as a per-hit
attempt rate, and a selected branch that cannot affect anything remains silent
without spending the generic cooldown or awarding proc XP. Deterministic
production-linked coverage proves the full-health heal, repeated fear, and
ineligible ultimate cases as well as the successful-heal control.

ART-AUD-011 is resolved. A class-sworn artifact now applies the same oath to
every named power: called effects and active commands. Wrong-class bearers can
still equip the artifact and suffer its burn, but `artifact info` and
`artifact abilities` conceal the command name and a direct attempt stops before
cooldown, PSP, XP, or the ability-specific handler.

ART-AUD-014 is resolved. The dormant ward shape now bypasses its configured
chance only for the critical-hit ward branch; its ordinary dispel branch uses
the declared percentage. Exact-roll production coverage protects the rejection
and success boundary without assigning the shape to a live artifact.

ART-AUD-007 is resolved. The nine Realms first-wave permanent-state packages
are deliberately rejected from the current level-scaled rebuild; Gesen's
source prototype had no states to adjudicate. The shared source masks were
mostly equipment-tier bundles rather than distinctive named mechanics:
Doombringer and Kelrom used the exact same six-state mask, Henekar and Avernus
were small variants, and Kelrarin's two prototypes disagreed. Every artifact
now declares whether passives are absent, rejected legacy, or progressive,
and boot validation enforces that policy against `artifact_passives[]`.

The initial audit changed no gameplay code. Remediation work is now tracked in
this document. All findings have been implemented or explicitly rejected,
tested, live-validated, committed, and closed one at a time.

## Evidence baseline

The current LuminariMUD code is authoritative for what the game does today.
The two historical trees are evidence for artifact identity and intended
mechanics, not instructions to copy unsafe legacy code literally.

| Tree | Audited revision | Role |
| --- | --- | --- |
| LuminariMUD | `61c03285` | Live registry, object prototypes, runtime hooks, documentation, and tests |
| RealmsOfLuminari | `3f57e70c45327335187fd123c991388e8bab2661` | First-wave artifact procedures and prototypes |
| HomelandMUD | `0dfd8fc0053b2c5573463e43066ec3de669bd46f` | Second-wave prototypes, procedures, and permanent affects |

Primary paths inspected:

- `src/obj/spec_artifacts.c` and `src/obj/spec_artifacts.h`
- `src/combat/fight.c`
- `lib/world/artifacts/1699.obj`
- `unittests/CuTest/test_artifacts.c`
- `unittests/CuTest/test_artifact_integration.c`
- `docs/systems/ARTIFACT_SYSTEM.md`
- `EXAMPLE/RealmsOfLuminari/src/specs.artifacts.c`
- `EXAMPLE/RealmsOfLuminari/src/specs.assign.c`
- `EXAMPLE/RealmsOfLuminari/areas/obj/quests.obj`
- `EXAMPLE/RealmsOfLuminari/areas/obj/astral_main.obj`
- `EXAMPLE/RealmsOfLuminari/areas/obj/waterdeep_harbor.obj`
- `EXAMPLE/HomelandMUD/src/spec_procs.c`
- `EXAMPLE/HomelandMUD/src/jotunheim.c`
- the six matching Homeland object records under `EXAMPLE/HomelandMUD/lib/world/obj/`

The review traced six layers for every artifact: registry template, prototype,
passive table, named/called-effect dispatch, combat hook, and behavioral test.
A nonzero generic proc percentage was not accepted as proof that a named source
procedure exists. The Tiamat's Stinger failure showed why those are separate
contracts.

## Classification

- **Confirmed defect:** Current code contradicts its own declared behavior or
  leaves an unsafe runtime path.
- **Confirmed source gap:** Executable reference behavior central to the named
  artifact has no current equivalent and no documented rejection.
- **Likely gap:** Strong source and current-data evidence exists, but product
  intent is still needed before implementation.
- **Design decision:** The difference may be deliberate balancing or a rebuild;
  choose and document the intended contract before changing code.
- **Covered:** The identity-defining behavior is implemented, or the departure
  is explicitly documented as a rebuild.

## Prioritized findings

| ID | Severity | Classification | Finding | Affected artifacts |
| --- | --- | --- | --- | --- |
| ART-AUD-001 | Critical | Resolved 2026-08-06 | A lethal artifact proc did not report death to the combat caller, which then continued through later victim-dependent riders. | Any damaging proc |
| ART-AUD-002 | High | Resolved 2026-08-06 | Fade now has its separate 1-in-16 level-scaled life siphon; the 16 percent generic table remains independent. | Fade |
| ART-AUD-003 | High | Resolved 2026-08-06 | Doombringer now has its separate 1-in-31, level-scaled extra-attack burst and independent one-third-hour recharge. | Doombringer |
| ART-AUD-004 | High | Resolved 2026-08-06 | Avernus now has its primary life steal, minor Bladesong heal, safe knockdown recovery, and emergency healing package. | Avernus |
| ART-AUD-005 | High | Resolved 2026-08-06 | Earthcrier sends `21 + artifact level + wielder Strength bonus + wielder Constitution bonus` to the save system; alignment and recharge requirements are disclosed, and its generic proc chance is explicitly tested as zero. | Earthcrier |
| ART-AUD-006 | High | Resolved 2026-08-06 | Kelrom's healback and 14 percent generic proc now use independent persisted cooldowns, and no-heal attempts are free. | Kelrom |
| ART-AUD-007 | Medium | Resolved 2026-08-06 | Nine mapped first-wave state packages are explicitly rejected from the current rebuild; Gesen explicitly has none. | First wave |
| ART-AUD-008 | Medium | Resolved 2026-08-06 | Wyrmfang now unlocks source danger sense alongside haste at level 5, completing its six-state passive package. | Wyrmfang |
| ART-AUD-009 | Medium | Resolved 2026-08-06 | Earthcrier and Wyrmfang are Large and use two hands for a normal Medium bearer; Aegis is explicitly body-worn breastplate armor. | Earthcrier, Wyrmfang, Aegis |
| ART-AUD-010 | Medium | Resolved 2026-08-06 | Generic proc percentages are labeled as attempt rates, and selected no-op branches spend neither cooldown nor proc XP. | Every generic-proc artifact |
| ART-AUD-011 | Medium | Resolved 2026-08-06 | Amaukekel's and Doombringer's active commands now obey their class oaths, remain concealed from wrong-class bearers, and reject without cost. | Amaukekel, Doombringer |
| ART-AUD-012 | Low | Resolved 2026-08-06 | Called-effect handlers now use validated table-owned stack groups, and Wyrmfang declares the ward group it actually creates. | Wyrmfang, future effects |
| ART-AUD-013 | High preventive | Resolved 2026-08-06 | All 17 artifacts now have an exact production-linked identity contract, including deliberate `none` entries and generic-proc separation. | Whole system |
| ART-AUD-014 | Low dormant | Resolved 2026-08-06 | Unclaimed `ART_SIG_WARD` now bypasses chance only for its critical ward; ordinary dispels use `sig_chance`. | No live claimant |

## Detailed findings

### ART-AUD-001: proc death is discarded by the combat hook [resolved]

Resolution (2026-08-06):

- `artifact_weapon_proc()` now returns whether its secondary damage killed the
  victim. The generic soul, doom, and ultimate branches propagate the
  `damage()` death result just like signature helpers.
- `handle_successful_attack()` reports lethal artifact damage to `hit()` and
  returns immediately. `hit()` also returns immediately, before critical
  riders, triggers, or any other victim access.
- Tiamat's Stinger preserves a lethal result even when the victim was already
  at zero HP and therefore contributes no drain healing.
- Production-linked tests kill a disposable production mobile through both
  Stinger's reusable signature and the generic soul proc. They assert deferred
  extraction, the outer-hook death result, proc output, and absence of the
  downstream vampiric-touch rider.
- The full suite passes 418/418 tests. The installed binary survived a local
  copyover, and Kohdee exercised the real five-attack Stinger loop against a
  disposable Oaken Defender with clean combat and temporary-target cleanup.
  Measured test gold and artifact progression were restored afterward.
  `testartifact verify` validated table metadata but still reports duplicate
  instances already held by Kohdee, Zusuk, and Bwarg; this remediation did not
  mutate that unrelated development state.

Original evidence at audited revision `61c03285`:

- Signature helpers return true when `damage()` kills the victim
  (`src/obj/spec_artifacts.c:2966`, `3292-3337`, and `3486-3575`).
- `artifact_weapon_proc()` has a `void` API. It returns from its own function
  when a helper reports death, but cannot report that result to its caller
  (`src/obj/spec_artifacts.c:3577-3597`; declaration at
  `src/obj/spec_artifacts.h:541`).
- `fight.c` calls it after the original hit's `victim_is_dead` value has already
  been decided, then continues to `weapon_special()` and vampiric-affect checks
  against the same pointer (`src/combat/fight.c:13761-13788`).
- NPC death calls `extract_char()` (`src/combat/fight.c:2452-2484`). A lethal
  artifact proc can therefore leave the outer hit path dereferencing an
  extracted character.
- The generic soul, doom, and ultimate branches call `damage()` without
  checking its death result (`src/obj/spec_artifacts.c:3611-3672`).

Completed remediation contract:

1. [x] Make the artifact proc boundary report victim death to `fight.c`.
2. [x] Set or re-evaluate `victim_is_dead` immediately after the hook.
3. [x] Stop all later victim access on death.
4. [x] Add lethal tests for both a reusable signature and a generic proc.

This was fixed first because adding the missing Fade, Doombringer, or Avernus
damage paths would have increased exposure to the unsafe caller.

### ART-AUD-002: Fade's combat life drain [resolved]

Resolution (2026-08-06):

- Fade is now a callable row in the production hand-written handler table
  with its inherited 1-in-16 per-hit roll. Its existing 16 percent generic
  proc remains a separate roll and display line.
- The siphon accepts living NPCs, refuses players, dragons, and undead, and
  ignores the generic 30-second internal cooldown like the inherited per-hit
  procedure.
- Damage uses `damage()` with `DAM_NEGATIVE` and scales from 40 at artifact
  level 1 to the inherited 200-point ceiling at level 5. Healing is 25 percent
  of damage actually inflicted, rounded down and capped by missing hit points.
- `artifact info` states both chances, the damage formula, the target rule,
  and the healing rule. Player help distinguishes independent per-hit effects
  from the generic table.
- A deterministic production-linked test proves level 1 and level 5 amounts,
  healing, cap behavior, cooldown independence, player/dragon/undead refusal,
  display text, and the updated all-artifact identity row. The focused and
  full suites pass 420/420 tests; `make install` installed the tested binary
  and removed the root build artifact.
- The development server survived copyover, production table metadata
  validated, and Kohdee's normal five-attack rotation attributed every hit to
  Fade. A temporary 10,000-HP Oaken Defender received two visible 40-point
  siphons that each restored 10 hit points; Fade's independent generic soul
  strike also appeared in the same fight.
- The temporary mobile was purged. Kohdee returned to 632/632 hit points,
  1,378 movement, 89,990 gold, and no equipment; Fade returned to Zusuk at
  level 1 and 0/100 XP with its original binding and provenance fields. Full
  integrity verification still reports the pre-existing duplicate instances
  documented under ART-AUD-001.

Original evidence at audited revision `61c03285`:

Realms' `Fade2` procedure registers for weapon hits and, on a 1-in-16 roll
against an eligible NPC, removes up to 200 HP and heals its wielder by 50
(`EXAMPLE/RealmsOfLuminari/src/specs.artifacts.c:1065-1097`).

Current Fade has:

- a 16 percent generic proc chance;
- four called effects; and
- `ART_SIG_NONE`, with no Fade case in the hand-written signature dispatch
  (`src/obj/spec_artifacts.c:132-134`, `399-407`, and `3552-3575`).

The shared percentage does not implement the source mechanic. It enters the
random soul/heal/fear/doom/ultimate table. This is the same structural
failure that previously hid Tiamat's Stinger: an attractive percentage exists,
but the named behavior does not.

Completed remediation contract:

1. [x] Preserve the inherited 1-in-16 identity separately from the generic
   16 percent proc table.
2. [x] Use normal negative damage, actual-damage healing, and a missing-HP cap.
3. [x] State and test living-NPC eligibility plus player, dragon, and undead
   refusal.
4. [x] Scale the inherited 200 damage and 50 healing across artifact levels.

### ART-AUD-003: Doombringer's combat burst [resolved]

Implementation checkpoint (2026-08-06):

- Doombringer is now a callable row in the production hand-written handler
  table with its inherited exact 1-in-31 weapon-hit roll. Its existing 20
  percent generic proc remains a separate table entry and display line.
- The burst accepts NPC targets and refuses players. It makes one real
  main-hand attack per artifact level, reaching the inherited five-attack
  ceiling at level 5, and stops immediately when its target leaves combat or
  dies.
- The source's one-point alignment loss against a good target is preserved and
  capped at -1000.
- The burst preserves the source's independent one-third-MUD-hour recharge,
  currently 25 seconds, in a dedicated persisted signature timestamp. Burst
  attacks cannot recursively trigger another artifact proc; the original hit
  may still reach the separate generic table if its target survives.
- A first implementation shared the generic 30-second timestamp and passed its
  deterministic suite, but repeated local combat windows showed the 20 percent
  generic strike repeatedly taking the lockout before the 1-in-31 identity
  could fire. That live design finding rejected the shared-cooldown version.
- Registry format v2.4 persists the dedicated timestamp. The v2.3 loader
  remains compatible and initializes the new recharge as ready.
- `artifact info` states the named and generic chances, target rule, scaling,
  cooldown, and alignment consequence. Player help distinguishes the burst
  from the generic table.
- A deterministic production-linked test drives the real attack loop at
  levels 1 and 5 and proves exact attack counts, independent cooldown behavior,
  nested-proc suppression, player refusal, alignment loss, display text, and
  the updated identity row. Persistence tests cover v2.4 round trips, future
  timestamps, and v2.3 compatibility.
- The complete root suite passes 422/422 tests. `make install` installed the
  tested binary and removed the root `circle` artifact. Kohdee survived
  copyover on that binary, wrote v2.4, reloaded it, and retained all 16 baseline
  inventory objects.
- In a controlled live fight, Doombringer's generic soul strike fired first;
  the black-tendril frenzy then fired while the generic timer was active,
  proving the timers independent. At artifact level 1 it produced one extra
  main-hand attack and the good-target conscience message. The temporary
  weapon and 17,000-HP Oaken Defender were removed, and Kohdee's inventory,
  equipment, room, HP, movement, gold, alignment, and registry row were
  restored.

Original evidence at audited revision `61c03285`:

Realms' executable combat branch rolls 1-in-31, makes five additional attacks,
reduces alignment when used against a good target, and starts a separate
`PULSE_HOUR / 3` recharge (`EXAMPLE/RealmsOfLuminari/src/specs.artifacts.c:2479-2524`).

At the audited revision, Doombringer had its three called effects and
`doomblast`, but no named signature dispatch. Its 20 percent value is the
generic table
(`src/obj/spec_artifacts.c:140-143`, `419-425`, and `3552-3575`).

Completed implementation contract:

1. [x] Preserve the inherited 1-in-31 identity separately from the generic
   20 percent proc table.
2. [x] Scale one through five real main-hand attacks across artifact levels and
   stop safely when the target is unavailable.
3. [x] Preserve and test the one-point good-target alignment consequence.
4. [x] Preserve and disclose the independent persisted one-third-MUD-hour
   recharge.

### ART-AUD-004: Avernus is missing most of Bladesong [resolved]

Implementation (2026-08-06):

- Avernus now checks its survival reactions on every successful hit while its
  named life-transfer strike keeps a separate table-owned 1-in-31 roll. The
  existing 15 percent generic proc remains independent.
- The transfer ceiling grows from 50 at artifact level 1 to the inherited 250
  at level 5. It deals three times the transfer as normal negative damage,
  refuses undead and constructs, and heals from one third of damage actually
  inflicted without exceeding the wielder's missing hit points.
- Bladesong retains the 1-in-11 style event and two-point heal when more than
  ten HP are missing. It also recovers an active wielder from an ordinary
  knockdown without bypassing sleep, paralysis, or pinning.
- The emergency heal retains its below-100-HP threshold and
  `30 + 2 * artifact level` percent chance, caps at maximum HP, and takes
  priority over the named drain on that hit.
- A production-linked integration test covers both scaling endpoints, capped
  healing, nonliving immunity, generic-cooldown independence, all three
  survival reactions, identity metadata, and player-visible information. The
  complete root suite passes 423/423 tests.
- The installed binary passed copyover and exposed both chances and the exact
  rules through `artifact info`. Controlled live combat produced the generic
  proc and named drain independently; the named drain dealt 145 damage at
  artifact level 1 after target damage reduction, then 295 at level 2 and
  capped its heal at the wielder's 14 missing HP. The minor Bladesong event
  restored two HP, and the emergency reaction restored a 50-HP wielder to
  full. The deterministic test covers knockdown recovery because normal player
  auto-stand took priority in the live combat loop. All temporary XP,
  ownership, cooldown, and inventory changes were restored after verification.

Original evidence at audited revision `61c03285`:

Realms identifies Avernus as "the life stealer" and implements four weapon-hit
behaviors (`EXAMPLE/RealmsOfLuminari/src/specs.artifacts.c:1883-1972`):

- emergency full healing below 100 HP;
- an occasional style message with a small heal;
- automatic recovery to standing when struck down; and
- a 1-in-31 main proc that transfers a bounded amount to the wielder and deals
  three times that amount to the victim.

At audited revision `61c03285`, `artifact_proc_avernus()` accepted no victim
and no hit damage. It only implemented the emergency full heal
(`src/obj/spec_artifacts.c:3119-3139`), so it could not perform the artifact's
main life-stealing strike or stand its wielder up.

The replacement should use current damage, position, immunity, and healing
helpers. The source's direct HP writes and over-max healing are not safe porting
targets.

### ART-AUD-005: Earthcrier's declared DC is dead code [resolved]

Implementation (2026-08-06):

- The save system's common base DC is now named `SAVING_THROW_BASE_DC` instead
  of remaining an unexplained literal inside `savingthrow_full()`.
- Earthcrier passes `declared DC - SAVING_THROW_BASE_DC` as the weapon-spell
  level component. Its base Reflex challenge is therefore 15 at artifact
  level 1 and 19 at level 5. Legitimate situational save and DC modifiers
  remain part of the common save path.
- `artifact info`, player help, and the formal system guide disclose the
  formula and immunity rules.
- A production-linked test invokes the real knockdown handler at levels 1 and
  5 and observes the challenge calculated inside `savingthrow()`. The complete
  root suite passes 424/424 tests.
- The installed binary survived copyover, exposed a base DC of 15 at artifact
  level 1, and validated all 17 production artifact metadata rows. In a
  controlled normal combat loop, Earthcrier's natural proc produced challenge
  21 after legitimate situational modifiers against Reflex 14, failed the
  target's save, and left it sitting. Temporary and measured persistent state
  was restored afterward.

Follow-up (2026-08-06):

- The wielder's current Strength and Constitution bonuses now add to the
  corrected base challenge. Production-linked coverage observes DC 22 for a
  level-1 artifact with +0 ability modifiers and DC 33 for a level-5 artifact
  with +4 Strength and +3 Constitution modifiers.
- The regression also proves the existing non-good wielder gate, while runtime
  info and player help now disclose that gate and its 30-second recharge. It
  also confirms Earthcrier has no generic proc competing for that timer.

Original evidence at audited revision `61c03285`:

`ARTIFACT_KNOCKDOWN_DC` declares a DC of `14 + artifact level`
(`src/obj/spec_artifacts.h:310-312`). The handler calculates exactly that value,
then passes modifier zero and `artifact_effect_level()` to `savingthrow()` and
silences the unused variable with `(void)dc`
(`src/obj/spec_artifacts.c:3219-3239`).

For `CAST_WEAPON_SPELL`, the save system uses `10 + level`
(`src/magic/magic.c:616-630`). `artifact_effect_level()` has a floor of 20 and
scales to character level plus twice artifact level. The effective base DC is
therefore at least 30 and can reach 50 for a level-30 wielder with a level-5
artifact, rather than the declared 15 through 19.

Choose one DC model, delete the other, and assert boundary values in tests.

### ART-AUD-006: Kelrom's generic proc cannot fire [resolved]

Implementation checkpoint (2026-08-06):

- The current rebuild keeps both advertised mechanics. Healback now uses the
  persisted `last_signature_proc` clock for its own 30-second recharge, while
  the generic table continues to use `last_proc`.
- The handler totals hit points actually restored across eligible in-room
  group members. Only a positive total stamps the healback clock and awards
  proc XP; a full-health party spends neither.
- The two clocks are independent, so the 14 percent generic roll can run on
  the same hit as healback and throughout healback's recharge.
- `artifact info`, player help, and the formal system guide now state the
  generic chance, healback scaling, group share, recharge, and animal taboo.
- A production-linked test proves the no-heal rule, one successful heal and XP
  award, repeat-heal refusal, independent generic damage, and visible contract.
  The complete root suite passes 424/424 tests.
- The installed binary survived copyover, exposed both mechanics through
  `artifact info`, and validated all 17 production artifact metadata rows. In
  controlled combat, healback raised Kohdee from 500 to 521 HP on the opening
  attack; the natural generic soul strike fired less than 20 seconds later,
  while healback's 30-second recharge was still active. Temporary and measured
  persistent state was restored afterward.

Original evidence at audited revision `61c03285`:

Kelrom is configured with a 14 percent generic proc chance
(`src/obj/spec_artifacts.c:150-152`). On every eligible hit, the hand-written
signature dispatch runs first. Its healback handler:

1. returns while the shared cooldown is active;
2. otherwise stamps `last_proc` before checking whether anyone needs healing;
3. awards proc XP even when every eligible target is already at full HP; and
4. returns to the generic path (`src/obj/spec_artifacts.c:3038-3097`).

The generic path then sees the same 30-second cooldown and exits
(`src/obj/spec_artifacts.c:3595-3607`). When the cooldown expires, Kelrom's
signature immediately stamps it again. The 14 percent path is unreachable for
ordinary damaging hits.

This needs a contract decision, not only a reordered line. Either remove the
advertised generic proc, or give the healback and generic table independent,
well-tested gating. Do not consume cooldown or award proc XP when no healing
occurred.

### ART-AUD-007: first-wave passive powers need a product decision [resolved]

Resolution (2026-08-06):

- The nine historical packages are rejected from the current identities.
  Gesen explicitly uses the source-none policy. This adds no gameplay buffs;
  the current numeric bonuses, resistances, procs, abilities, and called
  effects remain authoritative.
- This is a per-artifact recorded decision, not an inference from missing
  rows. Each template now declares `ART_PASSIVE_REJECT_LEGACY`,
  `ART_PASSIVE_NONE`, or `ART_PASSIVE_PROGRESSIVE`. Validation rejects unset
  policies, progressive policies with no rows, and passive rows attached to
  either non-progressive policy.
- The source masks support rejection as legacy equipment-tier packages.
  Doombringer and Kelrom share the exact same six-state mask; Henekar and
  Avernus are close variants; and the two Kelrarin prototypes disagree on the
  states beyond their shared sense/haste core. Restoring the masks would give
  nearly the entire first wave the same haste and detection suite while
  duplicating the distinct progression identity already assigned to the
  second wave.
- A production-linked regression requires all nine rejection policies, zero
  passive rows for them, and Gesen's separate source-none policy. The
  test-first run passed 433/434 tests and failed only on the new policy
  regression; the corrected full root suite passes 434/434 tests.
- On the installed development binary, Kohdee validated all 17 metadata rows
  and inspected all nine carried first-wave artifacts. None displayed an
  Always-on Powers section, while runtime and paged help both stated that an
  absent section means no hidden states. Kohdee's measured player and
  inventory files, 16-row MySQL inventory order, exact rent header, and the
  registry file were restored before a final login-free restart.

Original evidence at audited revision `61c03285`:

Realms prototypes give permanent states to nine of the ten mapped first-wave
artifacts. Gesen is the exception. Current object prototypes contain no
permanent affect records, and `artifact_passives[]` contains only second-wave
rows (`src/obj/spec_artifacts.c:319-353`).

| Artifact | Executable prototype states present in Realms but absent from the current passive table |
| --- | --- |
| Trorxek | Detect invisibility, barkskin, elemental protections, detect good, detect evil |
| Amaukekel | Farsee, detect invisibility, infravision, elemental protections, detect good, detect evil |
| Fade | Detect invisibility, haste, sneak, fire protection, detect good, detect evil |
| Horn of Henekar | Farsee, detect invisibility, haste, sense life, fly, fire protection |
| Doombringer | Farsee, detect invisibility, haste, sense life, infravision, fire protection |
| Kelrarin | Both variants share farsee, detect invisibility, haste, and sense life; their remaining protections differ |
| Kelrom | Farsee, detect invisibility, haste, sense life, infravision, fire protection |
| Tiamat's Stinger | Farsee, detect invisibility, haste, sense life, sneak |
| Avernus | Farsee, detect invisibility, haste, sense life, fire protection |
| Gesen | None |

Prototype evidence is in Realms' `areas/obj/quests.obj:132-744`,
`areas/obj/astral_main.obj:129-148`, and
`areas/obj/waterdeep_harbor.obj:450-471`, decoded against the affect constants
in that tree.

Some legacy elemental protections may already have deliberate numeric
resistance analogues in current templates. Sensory states and haste do not.
Because the current artifacts are level-scaled rebuilds, this was not an order
to restore every old flag. The formal system document now records the rejected
states and the current identity retained for each artifact.

### ART-AUD-008: Wyrmfang drops one member of a six-state source package [resolved]

Resolution (2026-08-06):

- Wyrmfang now unlocks `AFF_DANGERSENSE` at artifact level 5 alongside haste.
  This completes the source package without changing its level 1 through 4
  progression.
- A production-linked test proves that danger sense is absent and silent at
  level 4, active at level 5, reaches the real `check_dangersense()` behavior
  against an aggressive mobile, appears in `artifact info`, and is removed
  cleanly with the other artifact passives.
- The complete root suite passes 427/427 tests. On the installed development
  binary, `artifact info wyrmfang` listed all six powers as active at level 5.
  With an aggressive mobile in the adjacent room, `look north` printed
  `You feel danger there.` The temporary mobile and staged artifact level were
  removed and the exact measured state was restored.

Original evidence at audited revision `61c03285`:

Homeland Wyrmfang carries detect invisibility, sense life, infravision,
farsee, haste, and danger sense (`EXAMPLE/HomelandMUD/lib/world/obj/170.obj:400-424`).
The current progressive table ports the first five exactly, one per artifact
level (`src/obj/spec_artifacts.c:320-325`). It omits danger sense even though
the current engine defines `AFF_DANGERSENSE` (`src/structs.h:1600`).

This was a likely omission because it was the only dropped member of an
otherwise directly translated package. Level 5 was selected because danger
sense joins the source's final haste state without increasing lower-level
power.

### ART-AUD-009: object identity and handedness are inconsistent [resolved]

Earthcrier resolution (2026-08-06):

- Earthcrier's tracked object size is now Large, matching both its description
  and acquisition hint. The ordinary wielding calculation therefore assigns
  two hands to a Medium bearer while preserving the engine's existing
  size-changing and weapon-size feat rules.
- A production-linked regression reads Earthcrier's `I` extension from the
  tracked world file and passes that value through `hands_needed_full()` for a
  Medium bearer. The test first failed against the old Medium prototype.
- The provisioner deliberately preserves existing builder-owned records, so
  an existing world must set VNUM 169914's size to Large through OLC or an
  equivalent reviewed world-data edit. Fresh worlds receive the corrected
  package value.
- The full suite passes 425/425 tests. The installed development world then
  cold-booted the corrected prototype, and `stat object earthcrier` reported
  Large. With Kohdee's Monkey Grip and Powerful Build ranks temporarily set to
  zero, Earthcrier occupied the two-handed slot and an attempted second weapon
  was refused for needing an extra hand. Both ranks were restored to 1.
- `help artifact` loaded the updated player contract, `testartifact verify`
  passed all 17 rows, and Kohdee's measured character, inventory, and artifact
  registry state was restored before a final login-free restart.

Wyrmfang resolution (2026-08-06):

- Wyrmfang's tracked prototype is now Large, matching the Homeland weapon's
  explicit two-handed flag and nearly eight-foot description. A normal Medium
  bearer therefore uses two hands, while size-changing abilities, Monkey Grip,
  and Powerful Build remain normal systemic exceptions.
- A production-linked regression reads Wyrmfang's `I` extension from the
  tracked world file and passes it through `hands_needed_full()`. It first
  failed against the old Medium value and passes against `SIZE_LARGE`.
- Existing builder-owned records must set VNUM 169915's size to Large through
  OLC or an equivalent reviewed edit. Fresh worlds receive the corrected
  package value.
- The full suite passes 427/427 tests. A cold development boot reported
  Wyrmfang as Large. With Kohdee's Monkey Grip and Powerful Build ranks set
  temporarily to zero, Wyrmfang occupied the two-handed slot and Kelrom was
  refused for needing another hand. Both ranks were restored to 1.
- The same session verified Wyrmfang's restored level-5 danger sense,
  `help artifact`, and all 17 metadata rows. Kohdee's measured character,
  inventory, and artifact registry state was restored before a final
  login-free restart.

Aegis resolution (2026-08-06):

- The tracked body-worn breastplate is authoritative. It is the playable
  prototype, carries detailed breastplate keywords and descriptions, and has
  armor values and body wear flags. Aegis has no historical counterpart that
  argues for changing those mechanics.
- The public chronicle line now describes the repaired breastplate, and the
  placement brief calls it pure defensive breastplate armor. The older shield
  wording was a one-line rumor and did not define a shield prototype.
- A production-linked world-package regression reads Aegis's real item type
  and wear flags, requiring takeable body armor and rejecting the shield slot.
  A booted-registry regression requires the public lore to say breastplate and
  reject shield wording. The existing resistance test now equips its synthetic
  Aegis on the body slot.
- The test-first run passed 431/432 tests and failed only because the stale
  public lore still said shield. The corrected suite passes 432/432 tests.
- On the installed development binary, `stat object aegis` reported takeable
  body armor and a body wear slot. Kohdee wore it on the body while equipment
  reported no shield, and the public chronicle printed the repaired-breastplate
  lore. `testartifact verify` passed all 17 rows. Kohdee's player, inventory,
  and registry files were restored byte-for-byte before a login-free restart.

Original Aegis evidence at audited revision `61c03285`:

- The current Aegis prototype was body-worn breastplate armor
  (`lib/world/artifacts/1699.obj:220-240`), while its runtime content contract
  called it a shield (`src/obj/spec_artifacts.c:270-272`) and the placement
  plan described a pure defensive shield.
- Aegis had no historical counterpart to break the tie. Source history showed
  that the detailed playable breastplate package and the one-line shield rumor
  were independently authored; preserving the prototype avoided a gameplay
  and equipment-balance change.

### ART-AUD-010: displayed generic chance overstates observable behavior [resolved]

Resolution (2026-08-06):

- The shared generic dispatcher now stamps `last_proc` and dirties the registry
  only when the selected soul, heal, fear, doom, or ultimate branch actually
  fires. Full-health healing, repeated fear, and rejected ultimate attempts
  remain silent, award no proc XP, and leave the artifact ready.
- `artifact info` labels the configured percentage as a per-hit attempt rate
  and states that an attempt unable to affect anything spends no cooldown.
  Canonical player help and the formal system guide describe the same rule.
- A production-linked test forces all three no-op families through the real
  branch dispatcher, then forces a successful heal as a positive control. It
  asserts cooldown, output, XP, healing, and the player-visible information.
- The test-first checkpoint passed 428/429 tests and failed only on the old
  unconditional cooldown behavior. The corrected root suite passes 429/429.
- On the installed development binary, `artifact info kelrom` displayed the
  14 percent attempt rate and no-op cooldown rule, the paged `help artifact`
  entry carried the same contract, and `testartifact verify` passed all 17
  rows. Kohdee's player, supplier, inventory-order, and artifact-registry files
  were restored byte-for-byte before a final login-free restart.

Original evidence at audited revision `61c03285`:

`artifact info` says the configured percentage is a "chance per hit to unleash
a special strike" (`src/obj/spec_artifacts.c:4751-4753`). After that roll wins,
the selected generic branch can still do nothing:

- heal when the wielder is already at full HP;
- fear a target already feared;
- choose ultimate and fail its target, level-difference, or inner 5-percent
  gate.

The code stamps the shared cooldown after the switch even on these no-op paths
(`src/obj/spec_artifacts.c:3609-3679`). The shown percentage is therefore an
attempt rate, not a visible proc rate. This can make a working artifact feel
broken or dramatically rarer than advertised.

Either reroll/no-op without consuming cooldown, or label the number accurately
and expose a debug counter for attempted, rejected, and successful procs.

### ART-AUD-011: active abilities bypass class oaths [resolved]

Resolution (2026-08-06):

- The oath contract now covers every named power. A wrong-class bearer may
  still equip the artifact and suffer its periodic burn, but neither a called
  effect nor an active command will answer.
- The shared active-ability readiness path checks binding, then class
  recognition, then cooldown and PSP. Oath rejection therefore starts no
  cooldown, spends no PSP, grants no artifact XP, and never reaches the
  ability-specific handler.
- `artifact info` replaces the active-command name and description with a
  generic refusal for an unrecognized bearer. `artifact abilities` likewise
  omits the name and reports only that a sworn artifact withholds a named
  power. Runtime help, canonical player help, and the formal system document
  state the same rule.
- A production-linked regression uses the real `divineward` and `doomblast`
  command registrations. It proves concealment and cost-free rejection for
  both artifacts, then proves that a Cleric at Amaukekel's required depth
  applies sanctuary and spends its resources while a qualified Warrior reaches
  Doomblast's safe no-target handler boundary.
- The test-first checkpoint passed 432/433 tests and failed only on the new
  oath regression. The corrected complete suite passes 433/433 tests.
- On the installed development binary, Kohdee was temporarily made a level-30
  wrong-class bearer. Amaukekel concealed `divineward` in both information
  views and rejected the command; Doombringer did the same for `doomblast`,
  which never reached its no-target message. Current PSP and each artifact's
  post-equip XP were unchanged across its rejected command. Runtime and paged
  help showed the new contract, and all 17 artifact rows validated.
- Kohdee's player and mirrored inventory files, Zusuk's source files, and the
  artifact registry were restored byte-for-byte. The temporary MySQL
  Doombringer row was removed from Kohdee, restoring the 16-row Kohdee and
  21-row Zusuk inventories and Kohdee's baseline rent header before a final
  login-free restart.

Original evidence at audited revision `61c03285`:

Called effects checked `artifact_class_ok()` and refused an unrecognized
wielder (`src/obj/spec_artifacts.c:4484-4494`). `do_artifact_ability()` found
the equipped artifact and proceeded through binding, cooldown, and PSP checks
without the oath check (`src/obj/spec_artifacts.c:5365-5410`).

The formal system document disclosed that behavior. Its surprising consequence
was that a wrong-class character could use Amaukekel's `divineward` or
Doombringer's `doomblast` while the artifact burned them. The resolved policy
is that all named powers require recognition.

### ART-AUD-012: stack-group metadata is not the runtime source of truth [resolved]

Resolution (2026-08-06):

- Wyrmfang's `hunt` row now declares `ART_STACK_WARD`.
- The called-effect dispatcher passes each row's `stack_group` into the
  enrage, group-valor, frost-ward, and hunter's-sight handlers. Those handlers
  use the supplied value for both the exclusivity check and every temporary
  affect they create; no called-effect handler hardcodes its own group.
- The 17-artifact identity contract now records all four called-effect group
  slots, including deliberate `ART_STACK_NONE` entries. A behavioral test
  invokes Wyrmfang's hunt, confirms the ward group is active, then proves that
  Icedge's rime refuses without starting its recharge.
- The test-first run failed only on Wyrmfang's declared group, with 427 other
  tests passing. The corrected complete suite passes 428/428 tests.
- On the installed development binary, `invoke hunt` created the hunter's
  sight, whispered `rime` reported that one ward was already active, and
  `artifact info icedge` still reported rime ready. All 17 metadata rows
  validated, and the staged cooldown, XP, temporary target, and measured
  player state were restored.

Original evidence at audited revision `61c03285`:

`artifact_effect.stack_group` is range-validated, but no runtime dispatcher
reads it (`src/obj/spec_artifacts.c:367-378` and `897-902`). Wyrmfang's `hunt`
row declares `ART_STACK_NONE`, while `artifact_dragon_sight()` directly uses
`ART_STACK_WARD` (`src/obj/spec_artifacts.c:438-440` and `3983-4002`).

The table field needed to drive execution or be removed. Decorative control
data allowed an implementation to appear correct in validation while behaving
differently at runtime.

### ART-AUD-013: tests prove shapes, not artifact identities [resolved]

Resolution (2026-08-06):

- A 17-row integration-test matrix now states every artifact's reusable or
  hand-written combat handler and table-owned odds, active ability, generic
  proc chance, four called-effect, channel, and stack-group slots, and exact
  progressive-passive rows.
- A CuTest-only snapshot reads the booted production template, effect,
  passive, and hand-dispatch lookups. The test does not infer identity from
  the expected table or substitute a parallel runtime registry.
- `NULL`, zero, and `NOTHING` values are deliberate contract entries. The
  matrix exposed Fade's and Doombringer's missing combat packages until
  ART-AUD-002 and ART-AUD-003 updated runtime and expectation together; it
  likewise exposed Avernus's emergency-heal-only handler until ART-AUD-004
  restored the complete package.
- The test reports the first drift by artifact VNUM and field. The
  production-linked suite passes 419/419 tests.
- `make install` installed the tested binary and removed the root build
  artifact. The development server survived copyover, table metadata
  validated, and Kohdee's read-only `artifact info` output matched the
  contract for Fade, Avernus, and Wyrmfang. Kohdee logged out cleanly without
  changing artifact state. Full verification still reports the pre-existing
  duplicate instances described under ART-AUD-001; this test did not mutate
  them.

Original evidence at audited revision `61c03285`:

The integration suite was valuable but left the omission pattern unprotected:

- `artint_signature_cases[]` covers the six claimed reusable shapes only
  (`unittests/CuTest/test_artifact_integration.c:901-949`).
- The forced-proc test checks that each shape does something, not that every
  named artifact owns its required source behavior
  (`unittests/CuTest/test_artifact_integration.c:951-1022`).
- The generic cooldown test mutates zero-proc Aegis to exercise guards; it does
  not prove that each configured nonzero generic proc is reachable
  (`unittests/CuTest/test_artifact_integration.c:1236-1277`).
- Kelrom's test confirms only that healback observes cooldown. It does not
  detect that the same timestamp shadows its generic proc
  (`unittests/CuTest/test_artifact_integration.c:1679-1721`).
- There was no lethal artifact-proc test at the outer `fight.c` boundary;
  ART-AUD-001 supplied both signature and generic lethal cases before this
  identity matrix was added.

Completed remediation contract:

1. [x] Add a data-driven identity row for all 17 artifacts.
2. [x] State the named combat handler, active ability, called effects, and
   progressive passives expected for each row.
3. [x] Keep deliberate `none` values explicit rather than inferring them from
   generic proc percentages or empty registries.

### ART-AUD-014: the dormant ward shape ignores chance [resolved]

Resolution (2026-08-06):

- The reusable dispatcher now bypasses `sig_chance` only when
  `ART_SIG_WARD` receives a critical hit. A noncritical dispel goes through the
  same configured percentage gate as other reusable shapes.
- A production-linked exact-roll seam exercises the dormant library shape on
  a neutral Aegis test carrier without changing any live artifact identity.
  Roll 41 rejects a 40 percent noncritical dispel without cooldown, output, or
  XP; roll 40 fires; and a critical still raises `ART_STACK_WARD` despite a
  roll of 100.
- The test-first checkpoint passed 429/430 tests and failed only because the
  rejected noncritical roll still fired. The corrected root suite passes
  430/430.
- On the installed development binary, `artifact info aegis` confirmed that
  the neutral test carrier gained no live signature assignment, and
  `testartifact verify` passed all 17 production rows. Kohdee's player,
  supplier, inventory-order, and registry files were restored byte-for-byte
  before a final login-free restart.

Original evidence at audited revision `61c03285`:

No live artifact currently claims `ART_SIG_WARD`, so this is not a current
player-facing defect. The dispatcher deliberately skips the percentage roll
for ward, then the noncritical branch always attempts dispel
(`src/obj/spec_artifacts.c:3523-3537` and `3340-3373`). The system document says
the noncritical dispel has a chance.

Fix or delete the unused shape before assigning it to an artifact. Its
`sig_chance` validator currently creates the appearance of a control that the
runtime ignores.

## Per-artifact disposition

| VNUM | Artifact | Audit disposition | Findings and source delta |
| --- | --- | --- | --- |
| 169901 | Trorxek | Covered intentional rebuild | Current critical blind and four called effects cover the stated identity. Realms' identify text promised critical blind although its procedure omitted the executable branch; current code supplies it. The legacy state bundle is explicitly rejected (ART-AUD-007). |
| 169902 | Amaukekel | Covered intentional rebuild | Three called effects and `divineward` exist, and all four named powers obey its Cleric oath (ART-AUD-011). No missing named combat branch was found. The legacy state bundle is explicitly rejected (ART-AUD-007). |
| 169903 | Fade | Core mechanic restored | The separate 1-in-16 life siphon and all four called effects exist. The generic 16 percent table remains independent. The legacy state bundle is explicitly rejected (ART-AUD-002, ART-AUD-007). |
| 169904 | Horn of Henekar | Covered source conflict | Four called effects exist. Realms identify text claims a hitpoint-sucking combat hit, but its executable procedure contains no such combat branch, so that stale claim is not ported. The legacy state bundle is explicitly rejected (ART-AUD-007). |
| 169905 | Doombringer | Core mechanic restored | The separate 1-in-31 burst scales to five real main-hand attacks, uses an independent 25-second recharge, and preserves the good-target alignment cost. Its called effects and `doomblast` obey the Warrior oath. The legacy state bundle is explicitly rejected (ART-AUD-003, ART-AUD-007, ART-AUD-011). |
| 169906 | Kelrarin | Covered current rebuild | Current code retains the returning lifesteal throw, holy mega blast, and `soulstrike`, with safer scaling and boss handling. The nested second strike from one source variant and both conflicting legacy state remainders are not part of the current identity (ART-AUD-007). |
| 169907 | Kelrom | Covered current rebuild | Animal punishment and group healback remain, and the independent 14 percent generic proc is reachable. The unsafe source branches and shared legacy state bundle are superseded by this documented rebuild (ART-AUD-006, ART-AUD-007). |
| 169908 | Gesen | Covered | The returning `SPELL_HARM` procedure exists, and the source prototype had no permanent states. No artifact-specific gap remains after the system-wide ART-AUD-001 and ART-AUD-010 fixes. |
| 169909 | Tiamat's Stinger | Core mechanic fixed | The separate lifesteal signature uses actual damage, capped healing, a 10 percent roll, and a 15-hit guarantee. The generic 18 percent table remains separate, and the legacy state bundle is explicitly rejected (ART-AUD-007). |
| 169910 | Avernus | Core mechanic restored | The independent 1-in-31 life transfer, emergency heal, minor Bladesong heal, and safe knockdown recovery are implemented and live-verified. The legacy state bundle is explicitly rejected (ART-AUD-004, ART-AUD-007). |
| 169911 | Aegis of Ages | Covered current-original | Its pure defensive numeric package is implemented and tested. The tracked body-worn breastplate is authoritative over the retired shield rumor; it has no historical counterpart (ART-AUD-009). |
| 169913 | Vengeance | Covered intentional rebuild | Current mercy signature and three progressive passives are an explicit safe redesign, not a literal Homeland port. No additional gap was found. |
| 169914 | Earthcrier | DC and handedness fixed | Knockdown uses its declared level-scaled base DC (ART-AUD-005). Its Large prototype now makes a normal Medium bearer wield it with two hands, matching current lore (Earthcrier portion of ART-AUD-009). |
| 169915 | Wyrmfang | Source package restored | Its weighted signature and `invoke hunt` remain safe rebuilds. All six source passive states now exist, culminating in level-5 haste and danger sense, and its Large prototype uses two hands for a normal Medium bearer. Hunter's sight declares and executes through the shared ward group (ART-AUD-008, ART-AUD-009, ART-AUD-012). |
| 169916 | Courage | Covered intentional rebuild | The current group valor effect and progressive defenses replace a Homeland combat branch that contained no finished mechanic. No additional gap was found. |
| 169917 | Icedge | Covered intentional rebuild | Cold defenses, rime, and a bounded reusable flurry are implemented. The flurry shape was recovered from a mechanics-only Homeland artifact rather than claimed as a literal Icedge source proc. |
| 169918 | Twilight | Covered intentional rebuild | Progressive awareness and a bounded surge replace Homeland's unsafe stat-doubling behavior. Current large size preserves two-handed use. No additional gap was found. |

## Intentional differences that should not be treated as regressions

- Current mechanics should use `damage()`, saving throws, affect helpers,
  immunity checks, legal-target checks, and capped healing. The legacy direct HP
  writes, over-max healing, and unconditional deaths are evidence of intent, not
  safe implementations.
- Tiamat's Stinger deliberately drains actual post-mitigation damage and caps
  healing. Its dry-streak guarantee is a current usability improvement.
- Kelrarin's mega damage scales with artifact level and does not execute bosses.
- Kelrarin's nested second strike from one Realms prototype is not part of the
  current returning-throw package; the two source prototypes also disagree on
  their broader state packages.
- Kelrom's current animal-punishment and bounded group-heal identity supersedes
  the source's unsafe full-heal, execute, and direct-hit-point branches.
- The nine first-wave Realms state bundles are explicitly rejected. The
  current template's numeric resistance is authoritative where present, and
  no unlisted haste, sense, movement, or protection state is implied.
- Vengeance, Earthcrier, Wyrmfang, Courage, Icedge, and Twilight were documented
  as rebuilds. Exact numeric parity with Homeland is not expected.
- Aegis of Ages is current-original and has no source-MUD mechanic to recover.
  Its body-worn breastplate prototype is the explicit form contract.
- Trorxek's critical blind is present even though the Realms identify text and
  executable procedure disagreed.
- Henekar's source procedure does not substantiate its identify claim of a
  hitpoint-sucking strike. Do not invent one merely to satisfy stale text.

## Recommended remediation order

1. **Completed 2026-08-06: make the combat boundary death-safe.** ART-AUD-001
   now has lethal signature and generic-proc outer-hook tests.
2. **Completed 2026-08-06: add identity-contract tests.** All 17 expected
   identities are explicit, so a generic percentage cannot masquerade as a
   named mechanic again (ART-AUD-013).
3. **Completed 2026-08-06: restore the three confirmed missing packages.**
   Fade, Doombringer, and Avernus are covered by deterministic integration
   tests and controlled in-game verification (ART-AUD-002 through
   ART-AUD-004).
4. **Completed 2026-08-06: repair confirmed current contradictions.**
   Earthcrier's DC and handedness, Kelrom's independent generic/healback
   contract, and Wyrmfang's passive and handedness package now agree with their
   declared behavior. Aegis is explicitly the shipped body-worn breastplate.
5. **Completed 2026-08-06: make product decisions.** Aegis identity is closed
   by ART-AUD-009, ART-AUD-011 applies class oaths to all named powers, and
   ART-AUD-007 rejects the first-wave legacy passive bundles through an
   explicit validated template policy and per-artifact system documentation.
6. **Completed 2026-08-06: clean the framework.** Table-owned called-effect
   stack groups, accurate generic-proc reporting, and the dormant ward chance
   contract are complete (ART-AUD-012, ART-AUD-010, and ART-AUD-014).

## Acceptance criteria

- Every one of the 17 artifact rows has an explicit test contract for named
  combat proc, active command, called effects with channels and stack groups,
  and progressive passives.
- Every artifact declares a validated passive policy; the nine audited Realms
  bundles remain explicitly rejected, and Gesen remains explicitly
  source-none.
- A lethal signature or generic proc returns safely through the outer combat
  hook without later victim access.
- Fade, Doombringer, and Avernus produce their approved named behavior in
  deterministic integration tests.
- Earthcrier's tested DC matches one documented formula.
- Earthcrier's tracked Large prototype requires two hands for a normal Medium
  bearer through the production wielding calculation.
- Wyrmfang exposes all six source passive states at their documented levels,
  its danger sense reaches the production directional-look check, and its
  tracked Large prototype requires two hands for a normal Medium bearer.
- Kelrom either has a reachable generic proc or advertises no generic proc; a
  no-heal event consumes neither cooldown nor proc XP.
- `artifact info` labels generic percentages as attempt rates, and a selected
  no-op branch consumes neither cooldown nor proc XP.
- Called effects use their validated table-owned stack groups for both
  exclusivity checks and temporary-affect tags.
- `ART_SIG_WARD` bypasses `sig_chance` for its otherwise-eligible critical ward
  and applies `sig_chance` to its ordinary dispel branch.
- Class-sworn called effects and active commands share one recognition rule;
  rejected active commands stay concealed and consume no cooldown, PSP, or XP.
- Prototype type, size, lore, placement notes, and runtime content contracts
  agree for Earthcrier, Wyrmfang, and the body-worn Aegis breastplate.
- Formal system documentation and help files are updated alongside the code.

The implementation workflow and the Tiamat's Stinger case study live in
[`docs/systems/ARTIFACT_SYSTEM.md`](../systems/ARTIFACT_SYSTEM.md). Use that
playbook for future artifact work; this audit is the closed findings ledger.
