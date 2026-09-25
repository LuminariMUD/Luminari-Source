# Arm Count Plan

Status: implementation plan, written 2026-09-24, reviewed against the checkout 2026-09-25.
Implementation in progress (2026-09-25): steps 1 to 4 are in the code and the existing suite
passes; step 5 (new regression coverage) and step 6 (documentation and help) remain. This
replaces the three separate mechanics in
[EXTRA_LIMB_MECHANICS.md](EXTRA_LIMB_MECHANICS.md) with an arm count, answering
[What a limb count would need](EXTRA_LIMB_MECHANICS.md#what-a-limb-count-would-need) with
equipment capped at four arms.

## Outcome

- Arm-dependent equipment and combat capability come from `arm_count(ch)`; consumers do not
  independently test Four Arms, Extra Arms, or a race. Existing anatomy restrictions still apply.
- Four Arms, Extra Arms, and race data feed the count. The Vestigial Arm discovery adds carrying
  capacity through `hands_have()`, not a full arm, a wear position, or an attack.
- Counts below two work, including zero. No race is assigned fewer arms by this change, and no
  injury or arm-loss command is added.
- `NUM_WEARS` stays 51. No wear position, saved `Loc`, attack type, or feat number is added.
- Preserve ordinary two-armed behavior, with the explicit corrections below for hand budgets,
  unsheathing, and completed object restoration. Extra Arms changing meaning and monks gaining
  an unarmed third-hand routine are intentional behavior changes.

## The model

### Count and grant sources

`arm_count(const struct char_data *ch)` in `src/core/utils.c` replaces `has_four_arms()`.
It returns 0 for NULL and a nonnegative count otherwise.

1. Start with 2 plus a signed `race_data.arm_adjust`, initialized to 0 in
   `initialize_races()`. For PCs use `GET_REAL_RACE()`, or the valid disguise race while
   `AFF_WILD_SHAPE` is set, matching the feat-source switch below. An ordinary disguise
   must not change arms. Bounds-check the race index; an invalid index contributes 0 adjustment.
   NPC `GET_RACE()` values are `RACE_TYPE_*` family IDs, not `race_list[]` race IDs
   (`read_mobile()` and `medit.c`); NPCs therefore start at 2 without a PC race adjustment.
2. Add 2 if the character's own Four Arms feat is present and one per nonnegative rank of its
   own Extra Arms. Use `MOB_HAS_FEAT()` for NPCs and PCs with both wild shape and a disguise
   race; otherwise use `HAS_REAL_FEAT()`, as the current `get_feat_value()` does.
   Clamp this intrinsic total to at least 0.
3. Ordinary PCs can also receive item arms. Scan equipped `APPLY_FEAT` entries only in
   positions whose numeric arm requirement is at most the intrinsic total. Each eligible
   item contributes at most one Extra Arms rank, even if that modifier is repeated.
   Four Arms contributes +2 **once across all sources**, so duplicate items or an item plus
   the real feat do not stack it. An item bearing both feats can contribute both, subject to
   that Four Arms limit. NPCs and the wild-shaped feat-source branch ignore item grants.

Eligibility uses the intrinsic total, never the item-enhanced count or a recursive call to
`character_can_use_wear_slot()`. An item cannot sustain its own position or unlock a position
for another arm-granting item to contribute. This keeps a single deferred restore pass sufficient
for valid equipment. A provider in an intrinsically open position can still be lost when the
hand budget shrinks, so reconciliation must handle cascading losses.

`hands_have()` becomes the count plus one with Vestigial Arm. Export it in `src/act/act.h`
beside `hands_used()` and `hands_available()`; handle NULL and retain the `player_specials` guard
needed during character construction. Both counts are derived, never serialized. A future loss mechanic
would change the intrinsic term and invoke the same lifecycle; it is outside this plan.

### Positions

`wear_slot_arms_needed(pos)` replaces `is_four_arm_wear_slot()`. All table entries below have
the `WEAR_` prefix. Other valid positions require 0 arms and never close because of arm count;
callers must still reject invalid indices and apply existing race, form, and item restrictions.

| Needs | Positions |
| -- | -- |
| 1 arm | `WIELD_1`, `HOLD_1`, `HOLD_2`, `SHIELD`, `HANDS`, `ARMS`, `WRIST_R`, `FINGER_R`, `FINGER_L` |
| 2 arms | `WIELD_OFFHAND`, `WIELD_2H`, `HOLD_2H`, `WRIST_L` |
| 3 arms | `WIELD_3`, `HANDS_2`, `ARMS_2`, `WRIST_R2` |
| 4 arms | `WIELD_4`, `WIELD_2H_2`, `WRIST_L2` |

`HOLD_2` deliberately needs only one full arm: the total hand budget prevents a normal
one-armed character from holding two items, while Vestigial Arm can supply the second hand.
It still opens no position and grants no offhand attack. Rings remain two item positions on
any body with at least one arm; sleeves and gloves cover the available arms in their pair.

Arms past four open nothing. They add hands for the existing held and shield positions and
the existing spare-hand bonuses. With melee weapon pairs, the current slots can consume up to
nine hands: four for weapons, four across `HOLD_1`, `HOLD_2`, and `HOLD_2H`, and one shield.
Slot availability alone never proves that an item fits the remaining hand budget.

### What each count gets

These are capabilities before other anatomy restrictions, and without Vestigial Arm.

| Arms | Weapons | Other positions | Attacks |
| -- | -- | -- | -- |
| 0 | none | no shield, held item, gloves, sleeves, wrist, or ring | existing unarmed/natural routine; no weapon or lower-pair swing |
| 1 | one one-hander | one hand, so a shield or held item replaces the weapon; gloves, sleeves, one wrist, two rings | first-pair routine without an equipped offhand or two-hander |
| 2 | first pair | as today | as today, except removal of the old Extra Arms bonus-attack path |
| 3 | adds third-hand weapon position; each weapon pair is exclusive | lower gloves and sleeves, third wrist | third-hand mirror, no fourth-hand attack |
| 4 | both weapon pairs | fourth wrist | existing Four Arms routine, plus the monk change below |
| 5+ | same weapon positions as 4 | same positions as 4, with more hand capacity | same candidates as 4 for the same equipment, class, and feats |

From three arms up, each weapon pair holds its one-handers or its two-hander, never both.
Retain the existing first-pair placement rules at two arms, including Vestigial Arm's extra
capacity. The second pair keeps its existing ranged-weapon exclusions. A lower double weapon
still supplies third- and fourth-hand attacks, not a new attack type.

### Monks

Use the two-armed unarmed routine as the baseline: base, haste, BAB, and flurry opportunities
in the primary role, with monk dice under the existing rules.

| Arms | Monk |
| -- | -- |
| 0 | same unarmed attack opportunities and flurry: strikes can use feet, knees, or head. No glove position, so no glove item bonus. |
| 1 | same unarmed opportunities; at most one monk weapon and no two-hand weapon position |
| 2 | unchanged |
| 3 or 4 | new: an empty third weapon position supplies unarmed third-hand candidates for a `MONK_TYPE()` character passing `monk_gear_ok()`, mirroring base, haste, BAB, and flurry at the existing second-pair chance |
| 5+ | same candidates as 4 |

The fourth-hand candidate still requires a weapon, including the offhand end of a lower double
weapon. Other characters' empty lower hands make no swing. Keep the existing Vital Strike,
wild-shape, polymorph, and ranged-routine exclusions.

Bare-hand bonuses still use the existing whole-character `is_bare_handed()` behavior.
A monk with an upper monk weapon and an empty lower pair can receive the new unarmed third-hand
routine without gaining whole-character bare-hand bonuses. Existing spare-hand bonuses follow
the available-hand count; unchanged attack opportunities at zero or one arm do not promise
identical damage bonuses to an empty-handed two-armed character.

## Steps

Implement the count and lifecycle together before treating the new counts as usable. Update
production-linked tests with each behavior change; keep the full suite green at completed steps.

1. **Count, positions, and equipment entry points.** Add the helpers and race field in
   `src/core/utils.c`, `src/core/utils.h`, `src/core/structs.h`, and `src/character/race.c`.
   Delete `has_four_arms()` and `is_four_arm_wear_slot()` and convert every caller.

   - `character_wear_slot_restriction()`: check the required count before the NPC return,
     reporting "You do not have enough arms to use that equipment slot." Map lower positions
     to their base position for existing race restrictions, using the existing
     `four_arm_slot_base()` mapping.
   - `src/obj/act.item.c`: derive `hands_have()` from the count. The wield pickers and
     sleeve/glove/wrist overflow consider only open positions. Handle "no eligible position"
     without indexing message arrays with -1 or returning an inaccessible fourth-hand slot.
     Full-slot messages must work at one and three arms too, including `WRIST_R`,
     `WRIST_R2`, and `WIELD_1`, whose current messages can say "YOU SHOULD NEVER SEE THIS".
   - Keep the final anatomy check after slot selection. Apply the hand-budget check to direct
     lower-slot requests as well as automatic overflow, and count a launcher's two-hand cost
     even when it occupies `WIELD_1`; `hands_needed()` alone omits that surcharge. Route
     size-required two-hand melee/held items to an open two-hand position in the selected
     pair, or refuse them; an explicit lower-slot request must not place one in a one-hand
     position that `hands_used()` would charge only one hand for.
   - Enforce the pair exclusivity above in `equip_char()` (`src/core/handler.c`) as well as
     command selection. Today that function checks anatomy and lower ranged exclusions, but
     does **not** reject a two-hander beside a one-hander in the same pair. Reuse one conflict
     predicate for commands, low-level equipping, and reconciliation. Preserve two-armed
     first-pair behavior and defer aggregate budget checks during provider restoration.
   - Allow `perform_second_pair_attacks()` from three arms up; gate fourth-hand candidates on
     four arms, including double weapons. Remove the Extra Arms full-BAB bonus-attack block
     from `perform_attacks()`.

2. **Lifecycle and persistence.** Rename the runtime four-arm machinery to `limb_reconcile()`,
   `limb_defer_begin()`, `limb_defer_end()`, `limb_restore_deferred()`, `ch->limb_*`, and
   `obj->limb_restore_slot` in `act.item.c`, `handler.c`, `handler.h`, `structs.h`,
   `objsave.c`, `players.c`, and their tests.

   - Close slots in today's lower-slot order, skipping slots still open: `WIELD_2H_2`,
     `WIELD_4`, `WIELD_3`, `WRIST_L2`, `WRIST_R2`, `HANDS_2`, `ARMS_2`; then
     `WIELD_2H`, `HOLD_2H`, `WIELD_OFFHAND`, `WRIST_L`; then one-arm positions,
     leaving `WIELD_1` last.
   - Track `limb_last_hands` in place of `four_arms_active`. Trim excess hand usage when
     capacity fell since the last completed check or an illegal slot was displaced. Keep
     the existing trim preference: `HOLD_2H`, `HOLD_2`, `HOLD_1`, `WIELD_OFFHAND`,
     `SHIELD`, `WIELD_2H`, then `WIELD_1`. Lower positions must already satisfy the
     count. This covers losses such as 6 to 5 and Vestigial Arm loss, even if no slot closes.
   - Recompute after each displacement and revisit earlier slots when another provider is
     removed. For example, losing a ring provider can force removal of a held Extra Arms
     provider, which can then close an already-visited third-hand slot. Finish only with
     stable slot eligibility and a legal budget; update the last-hand snapshot then.
     Preserve nested deferrals, affect batching, dead/NULL/reentrancy guards, transfer
     ownership, trigger side effects, and inventory fallback even when a remove trigger vetoes.
   - Reconcile pair conflicts when the three-arm rule becomes active, including a gain from
     two arms with Vestigial Arm and a pre-existing mixed first pair. Prefer removing the
     conflicting two-hander so the primary one-hander survives. Do not turn ordinary
     `affect_total()` calls on an unchanged two-armed body into a general equipment audit;
     the temporary staging in
     `Test_spec_rol_abyss_forged_weapons_dissolve_before_corpse_creation` stays valid.
   - `auto_equip()`: defer any otherwise-valid saved position closed by the current count,
     not just the seven lower slots. Retry once after the entire record set is read, keeping
     pending objects out of bags until retry. Clear every restore marker and retain contents
     and bag-sort fallback on failure. This shared path serves player flat files, database
     records, and pet records.
   - At completed object restoration, explicitly validate the final hand budget even on a fresh
     character with no prior high-capacity snapshot. Missing providers can leave an over-budget
     loadout entirely in ordinary positions; neither a count decrease nor a closed lower slot
     is guaranteed. This final validation must wait until all providers and retries finish.
   - `save_char_checked()`: during its silent re-equip loop, postpone positions that the count
     currently closes, restore all other equipment, then retry postponed entries once before
     ending deferral. Providers can only contribute from intrinsically open slots, so valid
     equipment needs no retry-until-success loop. Run wear triggers at most once per attempted
     restore, keep rejected items in inventory, and preserve ownership on all save exits.
     Repeated saves must leave valid gear in its original slots without messages.

3. **Hand-budget consumers.** Correct `is_weapon_wielded_two_handed()` in `utils.c`:
   a first-pair one-hander needs an actual spare hand before doubling its item bonuses, in
   addition to existing exclusions. A test for `hands_have() >= 2` alone misses hands already
   occupied elsewhere. Preserve genuine two-hand slots and the existing lower-pair rules.
   In `do_unsheath()`, check each object's size/hand cost, target-slot eligibility, pair
   conflicts, and remaining capacity after any successful first draw. Keep its first-pair
   targeting. An item that cannot be drawn stays sheathed; clear only successfully transferred
   sheath pointers, and report only items actually drawn. Cover primary-only, secondary-only,
   and partial success, including a secondary shield and an oversized or two-hand item.

4. **Monk third hand.** In `perform_second_pair_attacks()`, a third-hand candidate exists with
   a weapon or with an eligible monk's empty third position. Pass explicit eligibility to
   `second_pair_candidate()` instead of using a NULL weapon to mean "no candidate".
   `get_wielded()`, `compute_dam_dice()` (monk dice and the "Bare-hands" display row), and
   `skill_message()` already route a weaponless `ATTACK_TYPE_THIRD`; prove the complete
   attack path rather than adding another unarmed attack type.
   Keep candidate ordinals stable when chance rolls fail, roll once in the candidate's phase,
   and keep count mode's `floor(sum of candidate percentages / 100)` behavior. A single
   50-percent third-hand candidate can therefore add zero to the displayed numerical count
   while remaining a real combat opportunity. Preserve 50/75/100-percent training behavior,
   iterative penalties, and the existing exceptions for NPC and Wilderness Warrior extras.

5. **Regression coverage.** Extend `unittests/CuTest/test_four_arms.c` with `TestArmCount*`
   cases using its existing static fixtures; no new test file or shared fixture framework is
   needed. Save and restore any modified `race_list[].arm_adjust` values around each case.
   Adapt `test_race_equivalence.c` so an ordinary two-armed body's closed slots are those with
   requirements above 2, not all slots with a nonzero requirement. Replace
   `TestExtraArmsAddMeleeAttacksPerRankOnly` in `test_racial_innate_feats.c` while retaining its
   ranged non-regression coverage.

   - Sources: NULL, PC real feats, NPC mob feats, wild shape entering/leaving, ordinary disguise,
     race adjustment and invalid race bounds; one/two/many Extra Arms ranks; duplicate Four Arms
     across real and item sources; repeated modifiers and both feats on one item; item providers
     above intrinsic capacity and attempted provider chains; Vestigial Arm changes hands only.
     Change a PC race's adjustment and prove that an NPC with the same numeric family ID is
     unaffected.
   - Positions and commands: all `NUM_WEARS` at counts 0 through 5, with count 2 preserving
     anatomy restrictions; wear/wield at 0, 1, 3, and 6; full-slot messages; direct equip and
     explicit lower-slot requests; pair collisions and lower ranged refusal. At 6, four
     one-handers plus one held item and one shield use exactly six hands.
   - Loss: 4 to 3, 3 to 2, 2 to 1, 1 to 0, 6 to 5, and Vestigial Arm; a held provider removed
     during trimming; the two-to-three mixed-pair transition; repeated reconciliation and
     nested deferrals. Assert slots, hand usage, item ownership, and primary-weapon retention
     whenever that slot remains legal.
   - Persistence: repeated real `save_char()` calls; provider-last flat-file round trips for
     Extra Arms, including an intrinsically one-armed and zero-armed PC whose provider sorts
     after dependent gear; missing/rejected/self-supporting providers; contents, bag sorts,
     cleared restore markers, and over-budget ordinary positions on a fresh restore. Keep
     existing pet and unknown-`Loc` regressions.
   - Combat: counts 0/1, 3 without a fourth swing, and 5 equal to 4 for identical equipment;
     empty non-monk lower hands grant no replacement for the removed Extra Arms swings.
     Check count mode, display, and actual hits across phases 1/2/3, including failed mirror
     rolls, haste, BAB, flurry, training, double weapons, Vital Strike, shapes, and ranged exits.
   - Monks: bare-handed third attacks at 3 and 4 with monk dice; an upper monk weapon and an
     empty third hand; a fourth-hand monk weapon with an empty third; non-monks, disallowed
     armor/weapons, and empty fourth hands; zero/one arms retain the two-armed unarmed attack
     opportunities. Use PC monk and sacred fist fixtures to assert monk damage dice: NPC
     bare-hand damage uses the mob's own dice in `compute_barehand_dam_dice()`. Check glove
     bonuses and weapon-specific effects do not come from the wrong weapon or slot.
   - Consumers: lone one-arm item bonuses, no spare hand despite a count above two, and
     unsheath success/refusal/partial success without lost or duplicated items. One arm plus
     Vestigial Arm may hold two one-hand items but gains no offhand or two-hand position.

6. **Documentation and help.** Rewrite `EXTRA_LIMB_MECHANICS.md` as the implemented record,
   including monk behavior and provider eligibility. Update `GAME_MECHANICS_SYSTEMS.md`,
   `SAVE_SYSTEMS_BREAKDOWN.md` (restore rules and renamed reconciliation), both arm feats'
   descriptions in `PLAYER_RACES_REFERENCE.md` and `DURIS_RACIAL_IMPORTS.md`, and both feats'
   text/comments in `feats.c` and `structs.h`. Review linked Thri-Kreen pricing and
   no-double-grant notes in `DURIS_RACE_FEAT_PROPOSAL.md` and `DURIS_RACE_SPECIFICATIONS.md`;
   publish only confirmed balance decisions.
   Rewrite EXTRA-ARMS and FOUR-ARMS in `lib/text/help/help.hlp`,
   `sql/components/help_other_racial_innate_entries.sql`, and the development help database,
   then verify the two entries agree. No production publication is implied.
   Once implementation and verification are complete, replace inbound plan links in
   `EXTRA_LIMB_MECHANICS.md` and `docs/TECHNICAL_DOCUMENTATION_MASTER_INDEX.md` with the
   durable mechanics documentation before deleting this plan.

## Decisions and remaining balance work

- Extra Arms becomes one full arm per rank and loses its old full-BAB melee swing. On a normal
  two-arm baseline, two ranks alone and Four Arms alone produce the same count. Granting both
  produces six arms; it must not be used to represent the same two extra limbs twice.
- Extra Arms has no racial assignment in the current `race.c`. This is not evidence that no
  saved character or staff-created item uses it; account for existing grants when deploying.
- Arms past four add hand capacity only. A third weapon pair would need positions and attack
  types outside this cap.
- The monk third hand uses the existing second-pair chance: 50 percent, 75 and 100 with effective
  two-weapon training. Flurry is not training, and these percentages select attack attempts;
  normal attack rolls still decide whether they hit.
- Rings and the shield need one full arm. Vestigial Arm opens no positions, so at zero arms it
  cannot restore hand equipment; at one arm it permits two held one-hand items, or a weapon
  and a shield, but no offhand weapon or two-hand position.
- **Price remains unconfirmed.** The suggestion is 7.5 RP per rank that adds the third or fourth
  arm and 1 RP per rank past four, with a cap exception like Four Arms. Equal pricing is not
  mechanically established: the third arm already opens lower gloves and sleeves, and the monk
  change increases Four Arms' value. Define how mixed Four Arms/Extra Arms/race adjustments are
  priced and whether the existing Thri-Kreen-specific exception applies before changing the
  published pricing table. Do not present this suggestion as a decision already taken.

## Compatibility and boundaries

- Saved `Loc` values, zone `E` positions, pet record formats, and feat/attack constants stay
  unchanged. No constants regeneration or new save schema is required.
- Unchanged formats do not make rollback behavior-neutral: the old binary interprets Extra Arms
  as attacks and may reject count-enabled lower gear. Before deployment or rollback, inspect
  affected grants/loadouts and preserve recoverable saves; verify restoration on an isolated
  copy. Rolling back past the original Four Arms slots still needs the separate procedure in
  `SAVE_SYSTEMS_BREAKDOWN.md`.
- Trelux retains the ordinary baseline of 2: its existing race restrictions still close its hand
  equipment positions, and its pincer dual-wield exception remains.
- The first-pair-only consumers documented in `GAME_MECHANICS_SYSTEMS.md` remain unchanged.
- Somatic components, grapple's free-hand penalty, and the light, instrument, and crafting-tool
  positions remain under their current rules; the count does not imply a new anatomy system for
  every action. No NPC family anatomy table is added.

## Ablation

Keep the four-arm equipment cap, existing attack types, existing transfer/deferral machinery,
and a single post-provider restore pass. Drop a new test file and fixture-sharing infrastructure:
the private fixtures in `test_four_arms.c` already exercise the required production paths.
Do not add a third wear set, a larger `NUM_WEARS`, another attack pair, an arms `APPLY_*`
location, an injury system, or a compatibility wrapper for `has_four_arms()`.

Keep `arm_adjust` to express counts below two without inventing negative feat ranks. Keep
cascade reconciliation, completed-restore budget validation, and the save second pass because
the traced equipment paths otherwise leave invalid gear or make valid saves order-dependent.
The `HOLD_2` threshold resolves the original table's conflict with its Vestigial Arm example.
Pricing is an open balance decision, not extra implementation machinery.

## Verification

These are implementation acceptance checks, not claims that the feature is already implemented.
Use the root production-linked CuTest suite and its seed replay mechanism; count assertions alone
do not establish real attack execution or save/restore correctness.

```bash
make -j$(nproc) cutest
CUTEST_FILTER=ArmCount ./cutest
CUTEST_FILTER=FourArms ./cutest
make -j$(nproc) test
make install
python3 scripts/ci/check_build_parity.py
python3 scripts/world/wtool.py constants sync --check
```

Follow the root test build with `make install` and leave no root-level `luminari` artifact.
Run the configured formatting/hygiene hooks on changed files; the help seed must pass the
`sqlfluff-fix` hook. The selected test approach adds no source file, so existing manifests
suffice; if a file is added or removed, update both `cutest_SOURCES` and `cutest_test_files` in
`Makefile.am` and `CUTEST_TEST_SOURCES` in `CMakeLists.txt` before the parity check.
