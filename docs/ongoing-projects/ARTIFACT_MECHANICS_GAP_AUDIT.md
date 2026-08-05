# Artifact Mechanics Gap Audit

**Status:** Remediation in progress; ART-AUD-001, ART-AUD-002, and ART-AUD-013 resolved

**Audited:** 2026-08-06

**Scope:** All 17 artifacts in the live LuminariMUD artifact registry

## Outcome

The audit originally found three omissions of identity-defining combat
mechanics from the executable source material:

1. Fade had no life-draining combat strike. This was resolved by ART-AUD-002.
2. Doombringer has no extra-attack combat burst.
3. Avernus retains only its emergency full heal and is missing its main
   life-stealing strike, automatic stand-up behavior, and minor combat heal.

Doombringer and Avernus are the two confirmed identity packages still absent.

The audit also found three independent runtime defects: lethal artifact damage
was not propagated back to the combat caller, Earthcrier calculates but ignores
its declared save DC, and Kelrom's hand-written proc makes its advertised 14
percent generic proc unreachable. These defects affect more than source parity
and should be addressed before artifact placement. ART-AUD-001, the lethal-proc
boundary defect, was resolved first on 2026-08-06.

ART-AUD-013 was resolved next on 2026-08-06. A production-linked identity
contract now names the combat handler and table-owned odds, active ability,
called-effect slots, invocation channels, generic proc chance, and progressive
passives expected for each of the 17 artifacts. Missing packages are explicit
contract entries rather than behavior inferred from nonzero generic
percentages.

ART-AUD-002 was resolved after that contract was in place. Fade now owns a
separate 1-in-16 siphon against living non-dragon NPCs while retaining its 16
percent generic proc as an independent mechanic.

The initial audit changed no gameplay code. Remediation work is now tracked in
this document as each item is implemented, tested, live-validated, committed,
and closed one at a time.

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
| ART-AUD-003 | High | Confirmed source gap | Doombringer's five-extra-hit combat burst and its own recharge are absent. Its 20 percent value is only the generic proc table. | Doombringer |
| ART-AUD-004 | High | Confirmed source gap | Avernus implements only emergency healing; its primary life steal and auto-stand package are absent. | Avernus |
| ART-AUD-005 | High | Confirmed defect | Earthcrier computes `14 + artifact level` but sends a different, much higher level-derived DC to the save system. | Earthcrier |
| ART-AUD-006 | High | Confirmed defect | Kelrom's always-run signature path owns the shared cooldown, making its configured 14 percent generic proc unreachable. | Kelrom |
| ART-AUD-007 | Medium | Design decision | Nine mapped first-wave artifacts had permanent states in Realms, but the current first-wave has no progressive passive rows. | First wave except Gesen |
| ART-AUD-008 | Medium | Likely gap | Wyrmfang ports five of its six source senses but omits danger sense, which exists in the current engine. | Wyrmfang |
| ART-AUD-009 | Medium | Confirmed defect / design decision | Earthcrier says it requires two hands but is one-handed for a medium bearer; Wyrmfang and Aegis also have unresolved object-identity differences. | Earthcrier, Wyrmfang, Aegis |
| ART-AUD-010 | Medium | Confirmed UX defect | The displayed generic proc percentage is an attempt rate; successful rolls can consume cooldown with no visible or mechanical result. | Every generic-proc artifact |
| ART-AUD-011 | Medium | Design decision | Wrong-class wielders cannot use called effects but can use Amaukekel's and Doombringer's active commands while the artifact burns them. | Amaukekel, Doombringer |
| ART-AUD-012 | Low | Confirmed metadata defect | Called-effect `stack_group` is validated but never used; Wyrmfang declares none while its handler hardcodes the ward group. | Wyrmfang, future effects |
| ART-AUD-013 | High preventive | Resolved 2026-08-06 | All 17 artifacts now have an exact production-linked identity contract, including deliberate `none` entries and generic-proc separation. | Whole system |
| ART-AUD-014 | Low dormant | Confirmed library defect | Unclaimed `ART_SIG_WARD` skips its chance roll, so every eligible noncritical hit dispels despite documentation saying it only has a chance. | No live claimant |

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

### ART-AUD-003: Doombringer's combat burst is absent

Realms' executable combat branch rolls 1-in-31, makes five additional attacks,
reduces alignment when used against a good target, and starts a separate
`PULSE_HOUR / 3` recharge (`EXAMPLE/RealmsOfLuminari/src/specs.artifacts.c:2479-2524`).

Current Doombringer has its three called effects and `doomblast`, but no named
signature dispatch. Its 20 percent value is the generic table
(`src/obj/spec_artifacts.c:140-143`, `419-425`, and `3552-3575`).

The existing reusable flurry shape proves that bounded extra attacks and
re-entrancy suppression already exist. A remediation still needs an explicit
decision about attack count, alignment consequences, and whether Doombringer
uses the source's independent recharge or the current shared internal cooldown.

### ART-AUD-004: Avernus is missing most of Bladesong

Realms identifies Avernus as "the life stealer" and implements four weapon-hit
behaviors (`EXAMPLE/RealmsOfLuminari/src/specs.artifacts.c:1883-1972`):

- emergency full healing below 100 HP;
- an occasional style message with a small heal;
- automatic recovery to standing when struck down; and
- a 1-in-31 main proc that transfers a bounded amount to the wielder and deals
  three times that amount to the victim.

Current `artifact_proc_avernus()` accepts no victim and no hit damage. It only
implements the emergency full heal (`src/obj/spec_artifacts.c:3119-3139`). It
therefore cannot perform the artifact's main life-stealing strike or stand its
wielder up.

The replacement should use current damage, position, immunity, and healing
helpers. The source's direct HP writes and over-max healing are not safe porting
targets.

### ART-AUD-005: Earthcrier's declared DC is dead code

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

### ART-AUD-006: Kelrom's generic proc cannot fire

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

### ART-AUD-007: first-wave passive powers need a product decision

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
Because the current artifacts are level-scaled rebuilds, this is not an order
to restore every old flag. Decide per artifact which states are identity,
convert approved ones into progressive passives, and record rejected states in
the formal system document.

### ART-AUD-008: Wyrmfang drops one member of a six-state source package

Homeland Wyrmfang carries detect invisibility, sense life, infravision,
farsee, haste, and danger sense (`EXAMPLE/HomelandMUD/lib/world/obj/170.obj:400-424`).
The current progressive table ports the first five exactly, one per artifact
level (`src/obj/spec_artifacts.c:320-325`). It omits danger sense even though
the current engine defines `AFF_DANGERSENSE` (`src/structs.h:1600`).

This is a likely omission because it is the only dropped member of an otherwise
directly translated package. It still needs a balance decision about which
artifact level should unlock it.

### ART-AUD-009: object identity and handedness are inconsistent

Three records need content decisions:

- Earthcrier's current description says, "It takes two hands," and its
  acquisition hint also says it is swung two-handed
  (`lib/world/artifacts/1699.obj:264-285` and
  `src/obj/spec_artifacts.c:280-282`). Its object size is medium. The wielding
  code requires two hands only when the weapon is one size larger than the
  bearer, so a medium character uses one hand
  (`src/obj/act.item.c:4100-4126`; size constants at `src/structs.h:445-456`).
- Homeland Wyrmfang is explicitly a two-handed weapon, while current Wyrmfang
  is also medium-sized (`EXAMPLE/HomelandMUD/lib/world/obj/170.obj:400-424` and
  `lib/world/artifacts/1699.obj:287-309`). This is a source-parity decision,
  not an internal prose contradiction.
- The current Aegis prototype is body-worn breastplate armor
  (`lib/world/artifacts/1699.obj:220-240`), while its runtime content contract
  calls it a shield (`src/obj/spec_artifacts.c:270-272`) and the placement plan
  describes a pure defensive shield. Aegis has no historical counterpart to
  break the tie.

Earthcrier is a current defect. Wyrmfang and Aegis require a declared content
choice before prototype edits.

### ART-AUD-010: displayed generic chance overstates observable behavior

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

### ART-AUD-011: active abilities bypass class oaths

Called effects check `artifact_class_ok()` and refuse an unrecognized wielder
(`src/obj/spec_artifacts.c:4484-4494`). `do_artifact_ability()` finds the
equipped artifact and proceeds through binding, cooldown, and PSP checks without
the oath check (`src/obj/spec_artifacts.c:5365-5410`).

The formal system document already discloses this behavior. Its consequence is
still surprising: a wrong-class character can use Amaukekel's `divineward` or
Doombringer's `doomblast` while the artifact burns them. Decide whether the burn
is the entire penalty or whether all named powers require recognition, then make
called effects, active abilities, help, and tests agree.

### ART-AUD-012: stack-group metadata is not the runtime source of truth

`artifact_effect.stack_group` is range-validated, but no runtime dispatcher
reads it (`src/obj/spec_artifacts.c:367-378` and `897-902`). Wyrmfang's `hunt`
row declares `ART_STACK_NONE`, while `artifact_dragon_sight()` directly uses
`ART_STACK_WARD` (`src/obj/spec_artifacts.c:438-440` and `3983-4002`).

Use the table field in execution or remove it. Leaving decorative control data
invites the next artifact implementation to appear correct in validation while
behaving differently at runtime.

### ART-AUD-013: tests prove shapes, not artifact identities [resolved]

Resolution (2026-08-06):

- A 17-row integration-test matrix now states every artifact's reusable or
  hand-written combat handler and table-owned odds, active ability, generic
  proc chance, four called-effect and channel slots, and exact
  progressive-passive rows.
- A CuTest-only snapshot reads the booted production template, effect,
  passive, and hand-dispatch lookups. The test does not infer identity from
  the expected table or substitute a parallel runtime registry.
- `NULL`, zero, and `NOTHING` values are deliberate contract entries. The
  matrix exposed Fade's missing life drain until ART-AUD-002 updated runtime
  and expectation together; Doombringer's absent five-hit burst and Avernus's
  emergency-heal-only handler remain visible for ART-AUD-003 and ART-AUD-004.
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

### ART-AUD-014: the dormant ward shape ignores chance

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
| 169901 | Trorxek | Review passive decision | Current critical blind and four called effects cover the stated identity. Realms' identify text promised critical blind although its procedure omitted the executable branch; current code supplies it. Realms permanent states remain undecided (ART-AUD-007). |
| 169902 | Amaukekel | Review passive/oath decisions | Three called effects and `divineward` exist. No missing named combat branch was found. Realms permanent states are absent and the active ability bypasses its Cleric oath (ART-AUD-007, ART-AUD-011). |
| 169903 | Fade | Core mechanic restored; review passives | The separate 1-in-16 life siphon and all four called effects exist. The generic 16 percent table remains independent. Source passives are still unresolved (ART-AUD-007). |
| 169904 | Horn of Henekar | Source conflict; review passives | Four called effects exist. Realms identify text claims a hitpoint-sucking combat hit, but its executable procedure contains no such combat branch, so there is no reliable mechanic to port without a design decision. Source passives are absent (ART-AUD-007). |
| 169905 | Doombringer | Confirmed gap | `doomblast` and all three called effects exist; the five-hit combat burst does not (ART-AUD-003). Source passives and active-oath policy remain open (ART-AUD-007, ART-AUD-011). |
| 169906 | Kelrarin | Review source delta | Current code retains the returning lifesteal throw, holy mega blast, and `soulstrike`, with safer scaling and boss handling. Realms' returning throw also had a nested 1-in-6 second bounded strike that current code omits. Its passive package is unresolved (ART-AUD-007). |
| 169907 | Kelrom | Confirmed current defect; redesign review | Animal punishment and the current group healback exist, but the 14 percent generic proc is unreachable (ART-AUD-006). Realms instead had rare group full heal, lightning/execute, and bounded lifesteal branches; current behavior is a substantial rebuild whose intended identity should be confirmed. Source passives remain open (ART-AUD-007). |
| 169908 | Gesen | Covered | The returning `SPELL_HARM` procedure exists, and the source prototype had no permanent states. No artifact-specific gap was found beyond system-wide ART-AUD-001 and ART-AUD-010. |
| 169909 | Tiamat's Stinger | Core mechanic fixed; review passives | The separate lifesteal signature now uses actual damage, capped healing, a 10 percent roll, and a 15-hit guarantee. The generic 18 percent table is correctly separate. Realms' five permanent states remain a product decision (ART-AUD-007). |
| 169910 | Avernus | Confirmed gap | Emergency full heal exists; primary lifesteal, auto-stand, and minor Bladesong heal do not (ART-AUD-004). Source passives are absent (ART-AUD-007). |
| 169911 | Aegis of Ages | Current-original; identity decision | Its pure defensive numeric package is implemented and tested. It has no historical counterpart. Resolve whether it is a breastplate or shield (ART-AUD-009). |
| 169913 | Vengeance | Covered intentional rebuild | Current mercy signature and three progressive passives are an explicit safe redesign, not a literal Homeland port. No additional gap was found. |
| 169914 | Earthcrier | Confirmed defects | Knockdown exists, but its save DC ignores the declared constant and its prototype contradicts the two-handed current lore (ART-AUD-005, ART-AUD-009). |
| 169915 | Wyrmfang | Likely passive gap; identity review | Weighted signature, `invoke hunt`, and five source senses exist. Danger sense is the one omitted source state, and source handedness differs (ART-AUD-008, ART-AUD-009). |
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
- Vengeance, Earthcrier, Wyrmfang, Courage, Icedge, and Twilight were documented
  as rebuilds. Exact numeric parity with Homeland is not expected.
- Aegis of Ages is current-original and has no source-MUD mechanic to recover.
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
3. **In progress: restore the three confirmed missing packages.** Fade is
   complete. Implement safe, level-scaled versions of Doombringer and Avernus
   next (ART-AUD-003 and ART-AUD-004).
4. **Repair current contradictions.** Resolve Earthcrier's DC and handedness,
   then make an explicit Kelrom generic/healback choice
   (ART-AUD-005, ART-AUD-006, ART-AUD-009).
5. **Make product decisions.** Approve or reject first-wave passives,
   Wyrmfang danger sense, class-oath scope, Wyrmfang handedness, and Aegis
   identity. Record every rejection in `docs/systems/ARTIFACT_SYSTEM.md`.
6. **Clean the framework.** Make proc reporting accurate, use table stack-group
   data, and repair or remove the dormant ward shape
   (ART-AUD-010, ART-AUD-012, ART-AUD-014).

## Acceptance criteria for the future implementation pass

- Every one of the 17 artifact rows has an explicit test contract for named
  combat proc, active command, called effects, and progressive passives.
- A lethal signature or generic proc returns safely through the outer combat
  hook without later victim access.
- Fade produces its approved named behavior in a deterministic integration
  test; Doombringer and Avernus still require equivalent coverage.
- Earthcrier's tested DC matches one documented formula.
- Kelrom either has a reachable generic proc or advertises no generic proc; a
  no-heal event consumes neither cooldown nor proc XP.
- `artifact info` distinguishes attempt chance from successful/visible proc
  chance, or no-op attempts do not consume cooldown.
- Prototype type, size, lore, placement notes, and runtime content contracts
  agree for Earthcrier, Wyrmfang, and Aegis.
- Formal system documentation and help files are updated alongside the code.

The implementation workflow and the Tiamat's Stinger case study already live in
[`docs/systems/ARTIFACT_SYSTEM.md`](../systems/ARTIFACT_SYSTEM.md). Use that
playbook for the remediation pass; this audit is the open findings ledger.
