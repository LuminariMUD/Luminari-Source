# Thri-Kreen four-arm wielding: Duris study and LuminariMUD mapping

Status: mechanic implemented on branch
`feat/168-thri-kreen-four-arm-wielding`, updated 2026-09-14. Steps 1 to 3
of the sequence in Part 4 are implemented and tested and the step 4
documentation and checks are done; the Thri-Kreen race release itself waits
on the balance decisions listed in Part 0. See
"Part 0: progress and handoff" for the exact state. The extra-attack
stand-in first proposed for this issue was rejected; the target is the full
mechanic: real weapon slots, real doubled limb slots, real extra swings, and
a save format that carries them. Duris source verified at
`/home/aiwithapex/projects/duris`; our side traced in `src/structs.h`,
`src/obj/act.item.c`, `src/obj/objsave.c`, `src/handler.c`,
`src/combat/fight.c`, `src/character/race.c`, `src/constants.c`,
`src/act.informative.c`, `src/players.c`, and `src/db.c`.

Review baseline: LuminariMUD `6a048b0d34fe0f17faea30577d87bac72b5ec3d3`,
Duris `9e0bfac624aa19eccbfc8045edbfa8cfddfb575f` (both clean when traced).
Line numbers below refer to those revisions. Parts 1 to 4 preserve the
original design study as reviewed; Part 0 records what the branch actually
implements. Race conversion and RP prices below remain proposals.

Review disposition: retain the seven appended slots, feat gate and numeric
save format. Use shared hand capacity and small weapon-pair helpers; do not
assign existing hold/shield slots to fixed anatomical pairs or introduce a
new save format. The required additions are lifecycle-safe reconciliation,
complete armor/weapon consumers, and integration tests. Full race release
needs the separate balance decisions listed below.

Companion references: the Duris racial mechanics gap list and race conversion
study (revision-pinned links in issue #168), race point budgets in
`docs/guides/PLAYER_RACES_REFERENCE.md`, save format in
`docs/systems/SAVE_SYSTEMS_BREAKDOWN.md`.

## Part 0: progress and handoff

Read this first when taking the work over. Parts 1 to 4 are the reviewed
design; the line numbers in them are from the review baseline and have
moved. Every item below is on the branch; nothing here is on `master`.

### Done: step 1 (constants, feat, tables, eligibility, hand budget, placement)

Snapshot at the end of step 1. Two rows mention work that was still open at
that point (attack types, deferred cleanup); steps 2 and 3 below record where
it landed.

| Area | What exists at the end of step 1 |
|------|-----------------|
| Constants | `WEAR_WIELD_3` 44 .. `WEAR_WRIST_L2` 50, `NUM_WEARS` 51, `FEAT_FOUR_ARMS` 1317, `FEAT_LAST_FEAT` 1318, `NUM_FEATS` 1319 in `src/structs.h`. Attack types THIRD/FOURTH are not added yet (step 3). |
| Capability | `has_four_arms()`, `is_four_arm_wear_slot()`, `is_second_pair_wield_slot()`, `four_arm_slot_base()`, `second_pair_rejects_object()` in `src/utils.c`, declared in `src/utils.h`. Grant sources: mob feats (NPC, disguised wild shape), `HAS_REAL_FEAT`, `APPLY_FEAT` gear in ordinary slots only. |
| Feat | `feato(FEAT_FOUR_ARMS, ...)` in `assign_feats()`: innate, in game, not learnable, not stackable. `test_racial_innate_feats.c` sentinel moved to `FEAT_FOUR_ARMS + 1`. |
| Anatomy gate | `character_wear_slot_restriction()` refuses the seven slots without the capability before the NPC early return, then maps each doubled slot to its base slot for the race table (Trelux cannot use lower hands). |
| Hand budget | `hands_have()` +2 with four arms (Vestigial Arm still stacks); `hands_used()` counts WIELD_3/WIELD_4 as one and WIELD_2H_2 as two. |
| Placement | In `src/obj/act.item.c`: `pick_one_hand_wield_slot()` and `pick_two_hand_wield_slot()` implement the pair rules; `perform_wear_impl()` re-runs the anatomy gate and the second-pair check on the resolved slot; sleeves, gloves and wrists overflow to the lower slots; all seven positions are in `wear_bitvectors`, `already_wearing`, `wear_message` and the `find_eq_pos` keyword table (as `!RESERVED!`, reached through the base keywords). `is_wielding_type()` and the `wield` ranged policy see the second pair. |
| Shared boundary | `equip_char()` drops to inventory on the anatomy gate or `second_pair_rejects_object()`, so zone `E`, `auto_equip()` and pets cannot bypass them. |
| Armor consumers | `apply_ac()`, `compute_gear_enhancement_bonus()` (lower piece counted only when worn), spell failure, armor penalty, max Dex, `is_proficient_with_sleeves()` (both pieces), the AC enhancement and dragonskin DR blocks in `fight.c`, and `rol_object_wear_conflicts()`. `do_equipment()` marks proficiency on all wield slots and lower sleeves. |
| Display | `wear_where`, `equipment_types` (zedit lists them), `eq_ordering_1` (lower arms after arms, lower wrists after wrists, lower hands after hands, second pair after the first pair). |
| Persistence | Seven `auto_equip()` cases in `src/obj/objsave.c` (`Loc` 45..51). No provider/dependent ordering or deferred cleanup yet (step 2). |
| Help/docs | `FOUR-ARMS` entry in `lib/text/help/help.hlp` and `sql/components/help_duris_racial_innate_entries.sql`, applied to the development database and verified identical. `GAME_MECHANICS_SYSTEMS.md`, `PLAYER_RACES_REFERENCE.md` (stand-in text reconciled, Four Arms price marked provisional) and `SAVE_SYSTEMS_BREAKDOWN.md` updated. `wtool_constants.json` regenerated. |
| Tests | `unittests/CuTest/test_four_arms.c` (eight `TestFourArms*` cases through `perform_wear()`, `equip_char()` and `test_auto_equip_loaded_object()`), registered in `Makefile.am` and `CMakeLists.txt`. `test_race_equivalence.c` expects the seven slots closed for a plain human. Full `make test` passes (1462 tests). |

Decisions taken in step 1 that Part 3 left open:

- Two-armed characters keep the old first-pair placement exactly (a
  Vestigial Arm alchemist may still put a one-hander beside a two-hander).
  Pair exclusivity is enforced only while the character has four arms.
- A one-hander fills WIELD_1, OFFHAND, WIELD_3, WIELD_4 in that order,
  skipping a pair whose 2H position is used. A two-hander takes the first
  pair with no weapons at all; with a one-hander in each pair it is refused
  with "free a pair of hands of weapons" even when two hands are free.
- Launchers and fire-weapons are refused in the second pair; the one-ranged
  and no-mixing policy checks every wield slot through `is_wielding_type()`.
- Held items and shields stay on the existing slots and share the budget.

### Done: step 2 (loss handling, deferral, order-independent restoration)

| Area | What exists now |
|------|-----------------|
| Reconciliation | `four_arms_reconcile()` in `src/obj/act.item.c`, declared in `src/handler.h`. Runs at the end of `affect_total()` (every completed equipment, affect, feat or form change) and at the close of an affect batch. Re-entry guarded by `ch->four_arms_reconciling`; skipped for characters being extracted (`DEAD()`). |
| Deferral | `four_arms_defer_begin()` / `four_arms_defer_end()` on a runtime counter `ch->four_arms_defer` in `struct char_data`; a loss noticed while deferred sets `four_arms_dirty` and is acted on when the outermost deferral ends. `save_char_checked()` in `src/players.c` brackets its unequip/re-equip cycle (no early returns exist between the two loops). |
| Loss action | Order: WIELD_2H_2, WIELD_4, WIELD_3, WRIST_L2, WRIST_R2, HANDS_2, ARMS_2; then, only when the character had four arms at the last completed check (`four_arms_active`) and the old positions exceed the budget: HOLD_2H, HOLD_2, HOLD_1, WIELD_OFFHAND, SHIELD, WIELD_2H, WIELD_1 until it fits. Each displacement wraps a `domain_object_transfer_begin/finish` (`DOMAIN_TRANSFER_RESTORE`), runs `remove_otrigger()` for its side effects but ignores a veto, re-reads the slot in case the trigger moved or purged the object, then `obj_to_char(unequip_char())`: inventory, never the room, inventory limits bypassed. Message: "You can no longer keep hold of $p and tuck it into your inventory." (suppressed under `mute_equip_messages`). |
| Restoration | `auto_equip()` marks four-arm gear whose slot is closed at that moment with `obj->four_arms_restore_slot` (runtime-only field on `struct obj_data`) and holds it in inventory. `crash_restore_records()` (shared by `Crash_load_objs()`, `pet_load_objs()` and the `test_restore_loaded_objects()` hook) runs the record loop under deferral, then `four_arms_restore_deferred()` retries those objects with their contents, then ends the deferral so capacity is checked once. Gear whose provider never arrives stays in inventory with its marker cleared. Copyover reconnects through `Crash_load()`, so it shares the path. |
| Tests | `TestFourArmsLossClosesExtraSlots`, `TestFourArmsLossTrimsOldPositionsToCapacity`, `TestFourArmsDeferralSpansProviderCycle` (nested deferral and affect batch), `TestFourArmsLossIgnoresRemoveTriggerVeto` (real DG trigger returning 0), `TestFourArmsRestoreIsOrderIndependent` (flat-file round trip through `objsave_parse_objects()`: provider after dependents, container contents, missing provider). Full suite: 1467 tests pass. |

Decisions taken in step 2:

- NPCs (and PCs in a disguised wild shape) take the capability from mob
  feats only, matching `get_feat_value()`, which never reads items for them.
  An "NPC item grant" therefore does not exist in this codebase; pets and
  zone `E` gear validate against the mob feat set at load.
- The hand-budget trim never audits a two-armed character who never had four
  arms: `Test_spec_rol_abyss_forged_weapons_dissolve_before_corpse_creation`
  stages an over-budget mob directly and must keep working.
- `hands_have()` tolerates a NULL `player_specials` because reconciliation
  now runs from `affect_total()` on partially built characters.
- The "pet fingerprint stabilizes within the new binary" check and a
  `save_char()` failure-path fixture were not added: `save_char_checked()`
  has no exit between its two loops, and the deferral test covers the same
  mechanism directly. Revisit if a save path with an early exit appears.

### Done: step 3 (combat routing and second-pair attacks)

| Area | What exists now |
|------|-----------------|
| Attack types | `ATTACK_TYPE_THIRD` 23, `ATTACK_TYPE_FOURTH` 24 in `src/structs.h`. |
| Pair helpers | In `src/combat/fight.c`, declared in `fight.h`: `is_second_pair_attack()`, `attack_is_offhand_role()`, `attack_pair_two_hand_slot()`, `is_dual_wielding_second_pair()`, `second_pair_dual_wielding_penalty()` (shares `dual_wielding_penalty_for()` with the first pair); static `pair_two_hander()` and `spare_hand_for_attack()`. `is_using_double_weapon_at(ch, slot)` in `assign_wpn_armor.c`. |
| Weapon lookup | `get_wielded()`: THIRD is WIELD_3 then WIELD_2H_2; FOURTH is the lower double weapon or WIELD_4. `skill_message()` picks the same weapon for THIRD/FOURTH messages. |
| Attack bonus | `compute_attack_bonus_full_with_weapon()`: the two-weapon block uses the attacking pair's dual test and penalty table; THIRD/FOURTH join the finesse case; the oversized-weapon check compares against the attack's own 2H slot. `is_using_light_weapon()` treats WIELD_4 like OFFHAND for Oversized Two-Weapon Fighting. |
| Damage bonus | THIRD mirrors PRIMARY (1.5x Strength only for the pair's own real two-hander, spare-hand +2 by primary-pair-first allocation, tinker); FOURTH mirrors OFFHAND (half Strength, tinker, ranger Dual Strike with the second pair's dual test). Power Attack doubles only for the attacking pair's two-hander. |
| Hit damage | `compute_hit_damage()` rewrites to TWOHAND only for first-pair types when WIELD_2H is worn; THIRD/FOURTH keep their identity. `compute_dam_dice()` display rows show the lower weapon. |
| Routine | `perform_attacks()` records the planned penalty, bonus count, max-BAB count and haste before the first pair's loops consume them, then calls `perform_second_pair_attacks()` after every ordinary attack of the round. Candidates: THIRD base, FOURTH base when the pair is dual, THIRD haste, THIRD per planned bonus attack (consuming max-BAB first, then -5 each), trained FOURTH extras (Improved/Greater/Epic, PCs only like the first pair). Each real candidate takes the next ordinal, rolls once in its phase after `valid_fight_cond()`, and hits with its own type. Chance: 50 + 25 (`MODE_2_WPN`) + 25 (`MODE_IMP_2_WPN`) via `is_skilled_dualer()`. Whole routine off under Vital Strike, wild shape and morph. Count mode returns the ordinary count plus floor(sum of chances); display mode prints "Third hand"/"Fourth hand" rows with the chance and never rolls. |
| Other consumers | Two-Weapon Defense counts WIELD_4; Weapon Mastery deflection/CMB use `is_wielding_type()`; grapple light-weapon rule covers every hand; `is_bare_handed()`/`monk_gear_ok()` see the second pair; speed, defending, ghost touch, keen, lucky and agile "any weapon" checks include it; `is_weapon_wielded_two_handed()` has a second-pair rule; disarm/sunder targets and the unarmed-disarm, whip and backstab/circle weapon checks include the lower arms. |
| Tests | `TestFourArmsGetWieldedRoutesSecondPair`, `TestFourArmsSecondPairBonusesReadOwnPair`, `TestFourArmsAttackRoutineCountsAndDisplays`, `TestFourArmsSecondPairAttacksLandWithOwnWeapons` (real `perform_attacks()` rounds on an NPC rogue with four arms: 50d1 lower weapons land in their own phases, no empty third swing, nothing swings without the arms). Full suite: 1471 tests pass. |

Decisions and deviations taken in step 3:

- The stochastic ranger Wilderness Warrior offhand procs (10 percent perks)
  are not mirrored; they keep their first-pair proc only. The trained
  extras (Improved/Greater/Epic and the ranger `DUAL_WEAPON_FIGHTING`
  equivalents inside `is_skilled_dualer()`) are.
- Spare-hand allocation is primary-pair-first: the first pair's primary
  claims a free hand unless a two-hander sits in WIELD_2H; the third hand
  gets the next free hand.
- Mirror chance uses `is_skilled_dualer()` so NPC rangers and rogues reach
  100 percent like trained players; NPCs still get no trained extra
  fourth-hand swings, matching the first pair's `!IS_NPC` gate.
- First-pair double-weapon quirks are untouched (the TWOHAND rewrite still
  applies to a first-pair double weapon's offhand end).
- Still first-pair only (documented, not extended): the parry weapon pick in
  `skill_message()`, the monk weapon AC pick, reach-weapon detection,
  sunder's attacker weapon, `mob_spells.c`, `spec_abilities.c`, `magic.c`,
  `feats.c`, `perks.c` and the `spec_rol_*` explicit slot checks.
- A test fixture note: `equip_char()`/`unequip_char()` recompute affects and
  reset `GET_HITROLL()`, so combat tests must set the hit roll after the
  last equipment change.

### Step 4 (help, save-format record, checks; release decisions open)

Done: help entry in both copies and the development database (verified
identical), `GAME_MECHANICS_SYSTEMS.md`, `PLAYER_RACES_REFERENCE.md`,
`SAVE_SYSTEMS_BREAKDOWN.md` (format note and the rollback procedure),
constants sync, build parity, source hygiene, full test suite,
`make install`. Equipment display labels and typed-bonus stacking across the
four wrists are covered by `TestFourArmsEquipmentDisplayAndTypedBonuses`; the
player-path downgrade fallback by `TestFourArmsUnknownSavedSlotFallsBackToInventory`
(pet rejection is covered by the existing pet persistence tests).

Open, and not something the code can decide: the Thri-Kreen race release.
Nothing in the mechanic references a race, so the feat can be granted today
through `set_race_feat()`-style registration, an item `APPLY_FEAT`, or a
mob feat. Registering a playable Thri-Kreen needs these explicit choices,
each recorded in Part 3:

1. Tier and RP: an 8 RP Four Arms trait breaks the single-trait cap of
   every tier (Advanced 4.2, Epic 7.2) and the subtotal lands outside both
   bands. Options: price Four Arms lower with a documented exception,
   accept an Epic race with an exception note, or hold the race until the
   guide's Epic formula/table discrepancy is reconciled.
2. Psionic defence: choose and price a mapping for Duris's psionic damage
   reduction, or omit it and say so.
3. Venom: `FEAT_POISON_BITE` (1-in-6 poison proc on any damaging hit) is
   amplified by the extra swings; keep it, rescale its gate, or build a real
   bite/paralysis later.
4. Cannot ride: separate mount-only rule or omit.
5. Ability adjustments: re-evaluate the study's +2/+1/-4/-4/+3/-3 proposal
   after the psionic/cold interpretation is corrected.

Review follow-ups (PR #181, applied after step 4): the armor-class
enhancement average divides by five only when lower sleeves are worn
(`TestFourArmsLowerSleevesAverageIntoArmorEnhancement`); a deferred four-arm
item stays in `ch->carrying` for the retry and only then takes its saved bag
sort, which `obj_from_char()` would otherwise clear
(`TestFourArmsDeferredRestoreHonorsBagSort`); `save_char_checked()` routes
every buffer failure through the shared restoration (`save_char_restore`),
so gear, affects and the four-arm deferral are always put back;
`NUM_COMBAT_ATTACK_TYPES` (25) counts the `ATTACK_TYPE_*` modes separately
from the weapon hit types, `attack_types[]` has "Third hand"/"Fourth hand"
labels with a size check, the damage-trigger mode name honors them, the
`attacks`/`damage` display commands accept `third` and `fourth`, and the RoL
weapon procs deliver extra attacks with the slot's own attack type.

Everything else in the acceptance list is either covered by a test named
above or recorded as a deliberate deviation in the step notes. Not done, by
design: the pet fingerprint check across binaries and a `save_char()`
failure-path fixture (see step 2 notes), and the stochastic ranger offhand
procs for the second pair (see step 3 notes).

### How to verify

```
make -j$(nproc) cutest && CUTEST_FILTER=FourArms ./cutest
make -j$(nproc) test && make install
python3 scripts/ci/check_build_parity.py
python3 scripts/world/wtool.py constants sync --check
```

## Part 1: how Duris does it

### The race

Duris Thri-Kreen (`RACE_THRIKREEN`, mob race code `TK`) are seven-foot
insectoid nomads with four arms. Their help entry (`help/duris_help_parsed.hlp`)
promises: wield up to four weapons at once, "including dual two-handed weapons
or archery combinations"; four wrist items, two sets of sleeves, two sets of
gloves; no body armor, footwear, finger rings, or earrings.

| Duris element | Where | Value |
|---------------|-------|-------|
| Stats (Str/Agi/Dex/Con/Pow/Int/Wis/Cha/Luck) | help entry | 115/130/125/105/70/65/65/75/90 |
| Innates | `src/classes/innates.c:657` | Dayvision 1, Ultravision 1, Bite 11, Leap 21, Vulnerable to Cold 1 |
| Psionic damage taken | `src/combat/dam_mods.c:336` | -0.3 multiplier adjustment under the `SPLDAM_PSI` predicate; this is not cold resistance |
| Cold vulnerability | `src/combat/dam_mods.c:468` | `DF_VULNCOLD` damage adjustment, plus a paralysis-category save against a slow effect |
| Lost slots | `src/cmd/actinf.c` `has_eq_slot()` | finger (both), body, feet, ear (both) |
| Cannot ride | `src/classes/mount.c:171` | "You cannot ride." |
| Leap dodge | `src/combat/fight.c` `leapSucceed()` | agility/7 percent, bounded 1 to 20, level difference adjusts |
| Epic skill deny | `src/classes/epic_skills.c:229` | cannot learn Devastating Critical |
| Creation side | `src/core/constant.c:1606` | neutral: player picks a racewar side |
| Combat pulse | help entry | "very bad" (slowest attack round in the game) |

### The four-hand predicate

```
#define HAS_FOUR_HANDS(ch) \
    ((GET_RACE(ch) == RACE_THRIKREEN) || (IS_AFFECTED3((ch), AFF3_FOUR_ARMS)))
```
(`src/core/utils.h:945`). `AFF3_FOUR_ARMS` is an equipment affect flag that
item enhancement (`src/item/enhance.c:1381`) and auction search
(`src/economy/auction_houses.c:3566`, "that grant the wearer four arms to
fight with") expose, so any race can gain the mechanic from gear. Every
consumer tests the predicate, never the race, except the slot denials above.

### Wear positions

`src/core/defines.h`:

| Position | Number | Note |
|----------|--------|------|
| `PRIMARY_WEAPON` / `WIELD` | 16 | |
| `SECONDARY_WEAPON` / `WIELD2` | 17 | |
| `HOLD` | 18 | one held slot for everyone |
| `THIRD_WEAPON` / `WIELD3` | 25 | four-hand only |
| `FOURTH_WEAPON` / `WIELD4` | 26 | four-hand only |
| `WEAR_ARMS_2` | 31 | "worn on lower arms" |
| `WEAR_HANDS_2` | 32 | "worn on lower hands" |
| `WEAR_WRIST_LR`, `WEAR_WRIST_LL` | 33, 34 | "worn on lower wrist" |

Display strings are in `where[]` (`src/core/common.c:341`): `<third weapon>`,
`<fourth weapon>`, `<worn on lower arms>`, `<worn on lower hands>`,
`<worn on lower wrist>`. `has_eq_slot()` (`src/cmd/actinf.c:8334`) returns
false for all six extra positions unless `HAS_FOUR_HANDS()`.

### Hand accounting and placement (`src/cmd/actobj.c`)

`get_numb_free_hands()` (line 6989): capacity is 2, plus 2 for four hands.
Each of HOLD, SHIELD, WIELD, WIELD2, WIELD3, WIELD4 subtracts
`wield_item_size()`, which is 2 for a two-handed weapon or a `ITEM_TWOHANDS`
object (1 for giants), else 1. So a four-handed character can carry two
two-handers, or a shield plus three one-handers, or a held item plus three
weapons, and so on.

`free_hand_slot()` (line 7022) picks the slot. With four hands it walks the
pairs (PRIMARY, SECONDARY) then (THIRD, FOURTH): a one-hander fills the first
empty primary of a pair, or the pair's secondary if the primary holds a
one-hander; a two-hander prefers a primary whose secondary is empty. There
is then a fallback scan for any empty weapon slot within the shared hand
budget; pair placement is a preference, not a strict invariant. Without
four hands, only the first pair is considered. Held implements may also sit
in weapon slots (they are storage roles, not anatomy).

The wield case (line 7870) then applies the training gate. The whole
dual-wield block (`SKILL_DUAL_WIELD` requirement, and the off-hand weight rule
"weight x3 must not exceed the strength wield limit") is skipped for four-hand
characters: `if (!HAS_FOUR_HANDS(ch)) { ... }`. Thri-Kreen never need the
dual wield skill to fill four slots and never hit the off-hand weight cap.

Wearing (same file): hands (case 7), arms (case 8), and wrists (case 11) each
branch on `HAS_FOUR_HANDS()` to fill the second set, with messages "You can't
wear any more on your hands/arms" and "You already wear something around all
your wrists".

### The attack round (`src/combat/fight.c` `calculate_attacks()`, line 9117)

Duris builds an array of weapon slots to swing this round with
`ADD_ATTACK(slot)`. For the non-monk path:

| Trigger | Base swing | Four-hand mirror | Chance |
|---------|------------|------------------|--------|
| Every round (unless slowed) | PRIMARY | THIRD | dual wield skill / 2 + 50 percent |
| Dual wield roll succeeds | SECONDARY | FOURTH | same |
| Improved two-weapon roll | SECONDARY | FOURTH | same |
| Double attack | PRIMARY | THIRD, and FOURTH | strict `>` roll: skill/2 + 50, skill/2 + 45 |
| Triple attack | PRIMARY | THIRD, and SECONDARY | strict `>` roll: skill/2 + 45 each |
| Quadruple attack | PRIMARY | THIRD | strict `>` roll: skill/2 + 45 |
| Haste | PRIMARY | THIRD | skill/2 + 50 |

The base and haste mirrors use `>=` against a 1..100 roll: 50 percent at
zero skill and 100 percent at 100 skill. The strict comparisons instead give
49/99 or 44/94 percent. This table is not the whole routine: class-specific
and high-Dexterity attacks are not all mirrored, and the normal monk path
does not use this non-monk table. "Roughly doubles" is a heuristic, not an
exact attack count or damage-per-second result. Duris has no off-hand
strength or hit penalty tied to a slot: `pv_common()` receives only the weapon, so the third weapon is as
good as the first and the fourth as the second.

Other combat consumers: `proccing_slots[]` (defensive procs check all four
weapon slots and the doubled slots), parry requires a weapon in any of the
four slots (line 8793), riposte picks the fourth weapon 1 in 5, then the third
1 in 4, then the secondary 1 in 3 (line 4152), `mangle` and
`critical_disarm()` can strip any of the four, thrown-weapon lookup and count
(`src/combat/range.c`) scan all four, the two-weapon hit/dam bonus
(`src/classes/epic_skills.c:1195`) counts a weapon in any of the three extra
slots, and object specials that must be wielded accept any weapon slot
(`src/specs/specs.object.c`).

### Persistence

Duris does not save the slot number as a stable equipment position. Rent
files write the equipment array index, and `restore_wear[MAX_WEAR]`
(`src/core/files.c:4026`) maps THIRD_WEAPON and FOURTH_WEAPON to wear keyword
12 (wield). On load `wear(ch, obj, 12)` is called, which re-runs
`free_hand_slot()`; so a saved third weapon lands in whichever weapon slot is
free at restore time. The doubled limb slots map to their normal keywords
(7 hands, 8 arms, 11 wrist) and refill the second set the same way. This is
also why a non-Thri-Kreen who loses the item granting four hands can fail to
re-wear the extras on next login. A Thri-Kreen keeps its racial four hands.

### Mobs

`empty_slot_for_weapon()` (`src/mob/mobact.c:10521`) has a four-hand branch
that never returns WIELD3 or WIELD4 (the branch is broken: it returns WIELD2
whether or not it is occupied). Mobs with four arms therefore only get four
weapons through paths that bypass that selector, such as direct zone
equipment. This selector is not a model for Luminari mob equipment.

## Part 2: what LuminariMUD has today

### Wear positions and hands

`src/structs.h:1738` defines 44 positions (`NUM_WEARS 44`). Hands are modeled
as six slots: `WEAR_WIELD_1` 16, `WEAR_HOLD_1` 17, `WEAR_WIELD_OFFHAND` 18,
`WEAR_HOLD_2` 19, `WEAR_WIELD_2H` 20, `WEAR_HOLD_2H` 21, plus `WEAR_SHIELD`
11. Positions 28 to 31 and 42 are marked "currently unused; reserved for
compatibility" but each already has a wear flag, a keyword, and a display
string, so they are not free numbers; new positions append at 44.

`hands_have()` (`src/obj/act.item.c:4193`) is already the extension point:
a `switch (GET_RACE(ch))` with only a default of 2, then +1 for the alchemist
Vestigial Arm discovery. `hands_used()` counts one per wield/hold/shield slot,
two per 2H slot, and an extra one for a two-handed ranged weapon.
`hands_needed_full()` decides one or two hands from object size versus
character size (Monkey Grip and Powerful Build reduce it) or the
`ITEM_ROL_TWO_HANDED` flag.

`perform_wear_impl()` places hand gear: a two-hander asked for `WEAR_WIELD_1`
is redirected to `WEAR_WIELD_2H`; a one-hander asked for an occupied
`WEAR_WIELD_1` or `WEAR_HOLD_1` goes to `where + 2` (the offhand pair). The
paired slots (finger, neck, wrist, ear, ankle) use the "if right is taken,
`where++`" rule, which relies on the left slot being the next number.

`character_wear_slot_restriction()` (`src/character/race.c:144`) is the
anatomy gate: the tail slot, the Leonine Frame feat, and the per-race
`wear_slot_restrictions[NUM_WEARS]` messages set by
`set_race_wear_restriction()`. `equip_char()` refuses a slot the character
cannot use and drops the item to inventory, so zone `E` commands and loads
cannot bypass it. NPCs currently bypass the race table (the function returns
NULL for `IS_NPC()` before the lookup) but not the feat checks above it.

### Attack routine

`perform_attacks()` (`src/combat/fight.c:15847`) counts attacks first
(`bonus_mainhand_attacks` from BAB, flurry, haste, and so on) then executes.
`dual = is_dual_wielding(ch)`: true with an offhand weapon, a double weapon in
the 2H slot, or the Trelux race. When dual, the base is one
`ATTACK_TYPE_PRIMARY` hit and one `ATTACK_TYPE_OFFHAND` hit; then the
main-hand bonus attacks; then, still under `dual`, extra offhand hits for
Improved (-5), Greater (-10), and Perfect (0) Two-Weapon Fighting, and the
Wilderness Warrior perks. Extra offhands use
`attack_number_runs_in_phase()`, but the mainhand bonus loop still uses a
hardcoded phase switch with `j = numAttacks + i`; the whole routine is not
yet driven by the helper.

Weapon lookup is by attack type: `get_wielded()` (line 10545) maps PRIMARY to
`WEAR_WIELD_1` then `WEAR_WIELD_2H`, OFFHAND to `WEAR_WIELD_OFFHAND` or the
double weapon. `compute_attack_bonus()` applies
`dual_wielding_penalty(ch, offhand)` (-6/-10 base, -4/-8 light weapon;
training gives -2 with a light/oversized exception, otherwise -4). The helper
currently reads only the first pair's one-hand slots, even for double weapons.
`compute_damage_bonus()` has half-strength offhand and conditional 1.5x
two-hand rules, plus a +2 mainhand rule when a hand is free. The `attacks`
command runs the same routine in display mode. These are explicit slot/type
branches; weapon-object lookup alone does not extend them.

### Persistence

Player object files (`lib/plrobjs/*/name.objs`) write each object with
`Loc : <n>` where `n` is wear position + 1 (0 inventory, negative bag depth),
via `Crash_save(obj, ch, fp, j + 1)`. `auto_equip()` (line 594) re-validates
the object's wear flag for the saved slot, dropping to inventory on mismatch.
The strict `Loc` bound at line 4224 belongs to `objsave_parse_objects_db_pet()`;
it rejects the entire pet object record set on malformed input. The player
flat-file and general database parsers read `Loc` without that bound, and
unknown positive slots fall through `auto_equip()` to inventory.

Related serializers serve the optional MySQL object backup (`OBJSAVE_DB`),
house files, pet equipment (`Crash_save_pet()`), and copyover. They need
separate path tests. `player_save_objs_sheathed.sheathed_position` is a
position inside a sheath (1 or 2), not a character wear position; it does not
gain seven new values.
`object_database_wear_slots` stores item wear flags, not positions. Zone
files carry the position in `E` commands; `zedit` lists `equipment_types[]`
and bounds on `NUM_WEARS`. Player files store no equipment positions
themselves.

Many ownership and transfer loops use `NUM_WEARS` and scale automatically.
Other loops also contain explicit slot lists: armor spell failure, armor
penalties, max Dexterity, enhancement and sleeve proficiency in
`src/combat/assign_wpn_armor.c`, for example. `apply_ac()` in `src/handler.c`
explicitly recognizes `WEAR_ARMS`. These must include the lower sleeves.

`save_char()` (`src/players.c:2488,3914`) temporarily unequips and re-equips
all gear to serialize base character data. This is not a loss of anatomy.
Any new cleanup must defer across the complete save operation, including
error paths. Otherwise saving an item-supported character can move valid
extra-slot equipment into inventory.

### The nearest race: Trelux

Trelux (ID 9, epic, 30000) is the existing insectoid: Ultravision, Vital,
Hardy, Vulnerable To Cold, Exoskeleton, Leap, Wings, Trelux Eq, Pincers,
Insectbeing; no finger, hands, shield, wield, hold, leg, or foot slots.
Thri-Kreen is the mirror image: the same insect body but with hands, and the
whole race is what the hands can do.

## Part 3: the mapping

### Design rules

1. Use an innate, in-game, non-learnable, non-stackable `FEAT_FOUR_ARMS`.
   Mechanics test a feat-based capability, never a Thri-Kreen race constant.
   Items use `APPLY_FEAT`; no new affect flag is needed.
2. Append wear positions and preserve every existing number. Restore valid
   equipment in its saved position, subject to final anatomy and hand limits.
3. Model the weapons as two pairs. Each pair has primary, offhand and
   two-handed storage alternatives. Pair-specific attack rules must use the
   attacking weapon and its pair, not whichever other weapon is equipped.
4. Preserve existing two-arm behavior. Extend shared consumers where needed;
   test that adding empty slots does not change existing combat or saves.
5. Hold and shield positions consume shared hand capacity. They are not
   permanently assigned to one weapon pair. Keep existing held-item limits.
6. Complete the four-arm mechanic before using it as a race replacement.
   Race pricing, venom and other conversion differences below are not proof
   that the race is ready to release.

### New wear positions (append; `NUM_WEARS` 44 to 51)

| Constant | Number | Wear flag | Display (`wear_where`) | `equipment_types` |
|----------|--------|-----------|------------------------|-------------------|
| `WEAR_WIELD_3` | 44 | `ITEM_WEAR_WIELD` | `{Wielded Third}` | Wielded in third hand |
| `WEAR_WIELD_4` | 45 | `ITEM_WEAR_WIELD` | `{Wielded Fourth}` | Wielded in fourth hand |
| `WEAR_WIELD_2H_2` | 46 | `ITEM_WEAR_WIELD` | `{Wielded Twohanded 2}` | Wielded two-handed, second pair |
| `WEAR_ARMS_2` | 47 | `ITEM_WEAR_ARMS` | `{Worn On Lower Arms}` | Worn on lower arms |
| `WEAR_HANDS_2` | 48 | `ITEM_WEAR_HANDS` | `{Worn On Lower Hands}` | Worn on lower hands |
| `WEAR_WRIST_R2` | 49 | `ITEM_WEAR_WRIST` | `{Worn Around Lower R Wrist}` | Worn around lower right wrist |
| `WEAR_WRIST_L2` | 50 | `ITEM_WEAR_WRIST` | `{Worn Around Lower L Wrist}` | Worn around lower left wrist |

Reuse existing WIELD, ARMS, HANDS and WRIST flags. There is no new item wear
bit or `object_database_wear_slots` schema change, but OLC equipment labels,
slot tables and zone equipment validation still need coverage.

The extra 2H position preserves the separate storage representation. Each
pair's one-hand positions and its 2H position must be mutually exclusive in
both directions. Existing `WEAR_HOLD_1`, `WEAR_HOLD_2`, `WEAR_HOLD_2H` and
`WEAR_SHIELD` remain governed by the total hand budget. This permits two
one-handed weapons and two held items, three weapons and a shield, or two
two-handed weapons. Vestigial Arm still adds one hand, allowing a shield
alongside the latter combination.

### Predicate, feat identity and hand budget

Expose `bool has_four_arms(const struct char_data *ch)` in `src/utils.h`,
with its implementation in `src/utils.c`; return false for NULL. Do not
assume that a bare `HAS_FEAT()` call implements every promised grant:
`get_feat_value()` scans worn `APPLY_FEAT` objects only for ordinary PCs.
NPCs and PCs in a wild shape with a disguise race use `MOB_HAS_FEAT()`.
Define the four-arm predicate's grant sources explicitly and test each;
prefer a scoped helper over changing all feats' semantics.

Proposed dependency rule: intrinsic/effective-form grants and qualifying
items in ordinary slots can provide the arms. An item in an extra slot may
benefit from the arms but cannot itself sustain them. This avoids circular
self-support and makes restoration independent of record order. Multiple
independent providers keep the arms until the last provider is lost. Honor
existing suppression of worn gear in transformed forms; do not make otherwise
inactive equipment grant usable arms. This item dependency rule is a deliberate
addition to the conversion and must be explained in help.

At the review baseline, `FEAT_EXTRA_ARMS` is 1316, `FEAT_LAST_FEAT` is 1317,
and `NUM_FEATS` is 1318, all in `src/structs.h`. Append `FEAT_FOUR_ARMS`
before the sentinel and advance both bounds (1317/1318/1319 respectively if
nothing else is appended first). Do not reuse or renumber Extra Arms: it
already has runtime behavior and tests. Four Arms grants replace the proposed
Thri-Kreen stand-in grant, not the meaning of existing saved feat IDs. Do not
automatically grant both traits. Update the sentinel assertion in
`test_racial_innate_feats.c` and reconcile the stale stand-in paragraph in
`PLAYER_RACES_REFERENCE.md` when implementing the feature.

`hands_have()` adds 2 once when the predicate holds; Vestigial Arm still
stacks. `hands_used()` counts the new one-hand positions as one each and
`WEAR_WIELD_2H_2` as two. Keep size, Monkey Grip, Powerful Build and fixed
2H requirements consistent between placement and validation. Extra slots
accept melee weapons; reject launchers there at the shared equip boundary.
Do not add ranged hand-cost exceptions to slots that forbid ranged weapons.

The anatomy gate checks all seven new slots before the NPC early return.
Also map lower gloves/sleeves/wrists and new wield positions to their base
slot restrictions where applicable: an item-granted feat must not silently
bypass another racial or form restriction. The gate proves eligibility;
it does not by itself enforce hand capacity, pair exclusivity, object type
or feat-provider dependencies.

### Placement and armor behavior

In `perform_wear_impl()`:

- A one-hander tries WIELD_1, OFFHAND, WIELD_3, WIELD_4, skipping any pair
  whose 2H position is occupied. A two-hander tries WIELD_2H then
  WIELD_2H_2, requiring that pair's one-hand weapon positions to be empty.
  Apply the shared hand budget in addition to these storage checks.
- Sleeves and gloves fill the original position, then the lower position.
  Wrists try all four in order. Adding adjacent constants alone is not
  enough: the existing `where++` whitelist does not include WRIST_R2.
- Revalidate the resolved position, not only the requested one. Extend the
  size-exemption list for the new wield and wrist positions; lower sleeves
  and gloves keep their original size rules. Extend
  `rol_object_wear_conflicts()` so whole-body armor conflicts with lower
  sleeves in both equip orders.
- Extend `wear_bitvectors`, `already_wearing`, `wear_message` and
  `find_eq_pos` keywords, keeping keyword sentinels intact. Add all seven
  positions exactly once to `eq_ordering_1` and extend proficiency marking
  in `do_equipment()`. Check both `equipment` and looking at another character.

`do_wield()` currently checks first-pair slots explicitly. Its policy of
one ranged weapon and no ranged/melee mixing stays, but its implementation
cannot stay unchanged: it must see weapons remaining in the second pair.
Apply the same policy to direct wear, load and zone `E` paths. Preserve
throwable melee weapons as melee equipment; additional launcher combinations
and additional thrown attacks are outside this mechanic. Trace the existing
throw/draw/sheath selectors so none selects or detaches the wrong object.

Lower sleeves must participate in `apply_ac()`, armor enhancement, armor
category, spell failure, armor penalties, max Dexterity, sleeve proficiency
and whole-body conflicts just as original sleeves do. Some armor routines
average over pieces; include the extra piece in both numerator and denominator
according to the existing formula. Gloves and wrists use their existing
slot families' rules, including typed bonus stacking. Four equipped items do
not imply four stacking copies of the same typed bonus.

Use one small validation path for the new eligibility, weapon-pair and hand
rules at the shared equip boundary. Commands may preview messages, but zone
loads and object restoration must not bypass the checks. Preserve ownership
and container contents when an object is redirected to inventory.

### Removal, save bookkeeping and loss of the feat

Normal removal keeps the existing command behavior. Mandatory anatomy
cleanup needs stronger guarantees: `perform_remove(ch, pos, TRUE)` bypasses
curses and inventory limits but still calls a vetoable `remove_otrigger()`.
It cannot be the only mechanism for restoring a valid equipment state.

Reconcile at a completed equipment/form/feat transition. `affect_total()`
can signal that a check is needed, but must not blindly remove gear during
its own recomputation. `unequip_char()` calls it again, and `save_char()`
temporarily removes every provider while saving base stats. Use bounded,
per-character deferral/re-entry protection, integrated with existing affect
batching where suitable, and an outer completion check. Cover save success
and failure, initial load, copyover, pet staging, race/feat changes and form
entry/exit. Message suppression alone is not deferral.

On actual loss, remove extra-slot equipment in the proposed order:
WIELD_2H_2, WIELD_4, WIELD_3, WRIST_L2, WRIST_R2, HANDS_2, ARMS_2. Then
recheck total capacity. Removing only those slots is insufficient: four arms
can hold WIELD_1, OFFHAND, HOLD_1 and HOLD_2 entirely in old positions.
Remove only enough remaining hand gear to fit, with a documented deterministic
priority that prefers retaining the primary weapon. Preserve the first pair
when it remains legal; do not promise that it is always untouched.

Forced transfers must use the existing object-transfer ownership machinery,
remain safe when triggers move/extract an object, and finish with no duplicate
or orphaned objects. Recheck providers after removals; do not loop forever on
a trigger veto. Move displaced equipment to inventory, preserving contents,
and describe that destination accurately in messages. Do not say it was
"dropped" when it was not put into the room.

### Combat routing and attack generation

Append `ATTACK_TYPE_THIRD 23` and `ATTACK_TYPE_FOURTH 24`. Add small helpers
for an attack's weapon pair and primary/offhand role, keeping first-pair
callers compatible. `is_dual_wielding()` remains the first-pair test;
second-pair dual detection must honor the existing double-weapon size/type
rule and form restrictions.

| Consumer | Required extension |
|----------|--------------------|
| `get_wielded()` | THIRD resolves WIELD_3 or WIELD_2H_2; FOURTH resolves WIELD_4 or the qualifying second-pair double weapon |
| Attack bonus | Add both types to Strength/finesse and relevant weapon cases; select penalties/training from the attacking pair |
| Damage bonus | Mirror primary/offhand rules, including half strength, conditional 1.5x two-hand strength, Agile, tinker bonuses and ranger perks |
| Two-hand mechanics | Use the actual weapon/pair for Power Attack, double-weapon focus/specialization/critical bonuses and `is_weapon_wielded_two_handed()` |
| Damage dice and display | Preserve THIRD/FOURTH identity through `compute_hit_damage()` and `compute_dam_dice()`; display the actual extra weapon |
| On-hit effects | Trace normal sneak damage, weapon focus/mastery, poison, specials and artifact procs with the selected weapon; never borrow another slot's weapon |

A concrete blocker is `compute_hit_damage()` at line 9090: the presence of
WIELD_2H rewrites any non-ranged attack to `ATTACK_TYPE_TWOHAND`. It would
also rewrite THIRD/FOURTH, even when they use another weapon. Fix this in
the feature path and test mixed 2H/one-hand equipment through actual hits,
not just isolated damage helpers. `MODE_DISPLAY_PRIMARY/OFFHAND` also select
first-pair objects directly; new row labels alone cannot fix display.

Resolve free-hand bonuses per weapon pair. The current global
`hands_available() > 0` +2 primary damage rule would otherwise grant the
same spare hand to both primaries. Do not let spare arms or an unrelated
2H weapon change another weapon's handedness or damage multiplier. Keep the
first-pair rules for ordinary characters and document the four-arm grip rule.
The exact spare-hand allocation remains a design decision; a deterministic
primary-pair-first allocation is a candidate, not an established rule.

Build second-pair attack opportunities from the same eligible sources as the
first pair: base primary, base offhand when that pair is dual, BAB/flurry
bonus primaries and other existing bonus-mainhand sources, the separate
haste/speed primary, and trained extra offhands.
Honor ranger equivalents via `is_skilled_dualer()` and armor conditions, not
only the named Two-Weapon Fighting feat constants. Do not depend on first-pair
`dual` to unlock second-pair offhands. Each opportunity requires its actual
weapon: WIELD_4 alone must not generate an empty THIRD weapon attack.

The proposed mirror chance is 50 percent, +25 for effective basic two-weapon
training, +25 for effective improved training. This is a balance proposal,
not an exact conversion of every Duris roll. Use the same effective training
checks for both attack eligibility and the chance (including NPC behavior).
Keep first-pair double-weapon quirks from leaking into unrelated weapons;
cover any intentional correction with a baseline regression.

Scheduling requirements:

- The current routine executes base/haste, then evolution attacks, then
  mainhand bonuses, then extra offhands. There is no location "after the
  first pair and before evolutions." Haste is not part of
  `bonus_mainhand_attacks`, and the bonus loop consumes its penalty counters.
  Do not replay mutated counters in a pasted second block. Reuse a bounded
  weapon-attack enumerator/helper where needed, without duplicating the
  entire combat routine or changing natural-attack behavior.
- Give each candidate a stable ordinal and iterative penalty before rolling.
  Use `attack_number_runs_in_phase()`; consume its ordinal even on a failed
  mirror roll so later attacks cannot move phases. Roll a mirror only in
  normal execution, in its assigned phase, after fight/weapon validity checks.
  Each candidate gets one roll, not a new chance in each of phases 1..3.
- Apply `!VITAL_STRIKING(ch)` to the entire extra-arm routine. Zeroing bonus
  attacks later does not suppress a base THIRD/FOURTH swing. Preserve action,
  staggered, death, movement, transformation and target-validity checks.
- Include flurry/haste and the eligible ranger offhand sources deliberately.
  Stochastic sources (Air Embodiment, monk/ranger procs) need a shared source
  outcome when mirrored, not a second independent source roll. Trace their
  mode/phase handling rather than copying `mode != 2`: that condition also
  runs randomness and messages in count mode. Reserve stable candidate
  positions for stochastic sources and resolve any shared source outcome
  once per round when its attacks span multiple phase calls.
- Display potential THIRD/FOURTH rows with weapon, penalty and chance without
  rolling, spending actions, applying effects or sending combat messages.

`perform_attacks()` returns an `int`; its count mode also supplies
`TOTAL_DEFENSE` in `hit()`. Fractional expected values cannot be returned
silently. Preserve the integer API: proposed count semantics are ordinary
attack count plus the floor of the summed extra expected attacks (sum first,
round once), with no mirror rolls. Keep candidate ordinals separate from
that count. Show fractional expectations in the text display if useful.
Test the resulting Total Defense behavior explicitly. Audit pre-existing
stochastic count paths touched by this work; the Air Embodiment branch is
not a safe precedent for a pure query.

Audit explicit wield-slot consumers in `src/combat/assign_wpn_armor.c`,
`src/combat/fight.c`, `src/combat/act.offensive.c`, `src/utils.c`,
`src/obj/act.item.c` and `src/act.informative.c`: speed, bare-handed/monk
eligibility, weapon requirements for parry and disarm, double-weapon defenses,
`skill_message()` weapon selection, and score/equipment output. Distinguish "any weapon equipped" from
"the weapon delivering this hit"; broadening a global special-ability check
must not transfer one weapon's proc to every hand. Ownership/corpse loops
using `NUM_WEARS` still need conservation tests, not new implementations.

### Persistence and rollback

- New equipment positions 44..50 serialize as `Loc` 45..51. Old positions
  stay fixed; `Loc` 44 remains the existing tail position, subject to its
  original tail eligibility and wear flags.
- Add seven `auto_equip()` cases and final placement validation. Restore
  independent feat providers before dependent slots, and defer capacity
  cleanup until the complete equipment set is known. Preserve parser/container
  order; retry only deferred equipment with its saved slot. A test with a
  provider after dependent items must work just as well as the normal order.
  Missing/deleted providers leave dependent gear in inventory with contents.
- Cover player flat-file and optional DB restoration, strict pet records,
  copyover and house/container ownership separately. House and sheath contents
  are not extra character equipment merely because they share serialization.
  NPC item grants must work before pet/zone equipment is validated; a helper
  that only uses current NPC `HAS_FEAT()` does not satisfy this.
- `players.c` pet hashing includes each wear index and an empty-list marker,
  even for empty slots. Increasing `NUM_WEARS` therefore changes all equipment
  fingerprints, not just pets with extra gear. Verify normal save/no-change
  detection within the new binary; do not assume cross-version hashes match.
- Downgrade behavior differs by reader: baseline player `auto_equip()`
  redirects unknown positive slots to inventory, whereas the strict pet
  parser rejects the record set. Before rollback, back up and normalize new
  wear locations using the new binary or a reviewed conversion, including
  offline player/pet records and zone `E` commands. Account for the new feat
  ID as well. Removing gear from online players alone is insufficient.
  Record the procedure in `SAVE_SYSTEMS_BREAKDOWN.md` and test on isolated
  fixtures before any release. This plan does not authorize production work.
- Resync `scripts/world/wtool_constants.json` with the appended constants and
  moved bounds. No object wear-bit schema migration is justified by these
  seven positions alone.

### Race data for a Thri-Kreen defined as data

Gameplay mechanics need no race-specific checks. Registering a playable
race still needs a unique registry ID, bounds and creation/account-unlock
wiring using the existing registration path. The mapping below is provisional;
it is not a complete race registration patch.

| Duris | Proposed Luminari mapping or unresolved difference |
|-------|---------------------------------------------------|
| Stats 115/130/125/105/70/65/65/75/90 | Study proposal +2/+1/-4/-4/+3/-3 (Str/Con/Int/Wis/Dex/Cha); re-evaluate after correcting the psionic/cold interpretation |
| Size Medium | `SIZE_MEDIUM` |
| Four arms, four wrists, two sleeve and glove sets | `FEAT_FOUR_ARMS` at level 1 |
| Ultravision | `FEAT_ULTRAVISION` |
| Dayvision | No separate gameplay mapping proposed |
| Bite at 11, paralysing venom | `FEAT_POISON_BITE` (59) is an approximation, not a bite attack: the hit path has a 1-in-6 poison-spell proc on damaging hits, including weapons. Gate 6 is the study's proposed level rescaling. Extra weapon hits amplify it; a bite/paralysis implementation is separate work |
| Leap at 21 | `FEAT_LEAP` at proposed gate 11 |
| Psionic damage reduction | Duris has a -0.3 `SPLDAM_PSI` multiplier adjustment; select and price a separate psionic-defense mapping before full race release |
| Cold vulnerability and failed-save slow | `FEAT_VULNERABLE_TO_COLD` gives -20 cold damage reduction; it does not supply Duris's save-against-slow rider |
| No body, feet, finger, ear | `set_race_wear_restriction()` for BODY, FEET, FINGER_R/L, EAR_R/L |
| Cannot ride | Do not grant `FEAT_QUADRUPED_BODY` as a substitute: its knockdown resistance is an unrelated benefit. A mount-only rule is separate conversion work |
| Devastating Critical deny | No Duris epic-skill mapping proposed |
| Very bad combat pulse | No race-specific timing proposed; fixed Luminari rounds mean actual damage/proc output needs balancing |

### Race point price and release decisions

The proposed Four Arms price of 8 RP is unvalidated. Extra attacks, extra
item effects, two two-handers, poison/proc frequency, haste, high BAB and
training interact; compare low/high-level loadouts and damage per round
before adding a settled price to `PLAYER_RACES_REFERENCE.md`.

The earlier 11 RP total was only an uncapped subtotal: 6 positive ability
points + 16 trait points - 4 ability penalty credit - 7 drawbacks = 11.
It is below the Advanced band (12..16), not its bottom. Composition rule 3
caps ability penalty credit and drawback refund together at 25 percent of
the tier budget. Using the listed target budgets gives 22 - 3.5 = 18.5 RP
for Advanced (14), or 22 - 6 = 16 RP for Epic (24), before pricing omitted
psionic defense or revising Four Arms. Neither falls inside its tier band.

An 8 RP trait also exceeds the 30 percent single-trait limit for both
Advanced (4.2) and Epic (7.2). Calling the race Epic at 30000 does not resolve
that rule. The guide's formula gives a different Epic target from its table;
use the explicit table for these calculations and reconcile that discrepancy
when updating the guide. Final tier/cost requires an explicit balancing
choice or a documented exception, not similarity to Trelux. Mirrored BAB
attacks do scale with level, but feat-dependent chance alone does not prove
balanced progression across classes.

## Part 4: implementation sequence and verification

### Change inventory

| Area | Files | Change |
|------|-------|--------|
| Constants | `src/structs.h` | Seven positions, `NUM_WEARS`, Four Arms feat and both feat bounds, two attack types; any scoped lifecycle state |
| Slot tables and display | `src/constants.c`, `src/act.informative.c`, `src/obj/act.item.c` | Labels, ordering, messages, keywords, slot/size classification and proficiency output |
| Feat and capability | `src/character/feats.c`, `src/utils.c`, `src/utils.h` | Registration, effective grant sources, pair-aware two-hand utility |
| Equipment lifecycle | `src/obj/act.item.c`, `src/obj/item.h`, `src/handler.c`, `src/handler.h`, `src/players.c` | Placement, shared validation, mandatory cleanup, save/load deferral, AC |
| Anatomy and race data | `src/character/race.c`, relevant registry headers | Extra-slot eligibility and base restriction mapping; playable race registration after conversion/balance decisions |
| Combat | `src/combat/fight.c`, `src/combat/fight.h`, `src/combat/assign_wpn_armor.c`, `src/combat/assign_wpn_armor.h`, affected offensive selectors | Weapon routing, attack opportunities, phase/count/display behavior, armor and explicit weapon consumers |
| Persistence | `src/obj/objsave.c`, `src/players.c` | Provider/dependent restore order, final validation, pet lifecycle and fingerprint checks |
| Docs | `docs/systems/SAVE_SYSTEMS_BREAKDOWN.md`, `docs/guides/PLAYER_RACES_REFERENCE.md`, `docs/systems/GAME_MECHANICS_SYSTEMS.md` | Format/rollback notes, correct stand-in description and provisional pricing, mechanic rules |
| Help | `lib/text/help/help.hlp`, `sql/components/help_duris_racial_innate_entries.sql`, development help database | FOUR-ARMS entry; apply the SQL to the intended development DB and verify both copies agree |
| Constants sync | `scripts/world/wtool_constants.json` | Regenerate/resync changed constants and bounds |
| Tests/build lists | Existing root CuTest files, optionally `test_four_arms.c`; `Makefile.am`, `CMakeLists.txt` if a file is added | Production-linked regressions and build parity |

Only `wear_where` and `equipment_types` currently have the relevant
`CHECK_TABLE_SIZE` guards. Sized arrays such as `eq_ordering_1[NUM_WEARS]`
can silently zero-fill missing entries. Validate table coverage and uniqueness;
an assertion on an explicitly sized array alone cannot prove initialization.

### Sequence

1. Add constants, feat, tables and shared equipment eligibility/budget rules.
   Exercise command, direct equip and lower-armor behavior together.
2. Add guarded loss handling and restoration as one coherent change. Prove
   ordinary saves preserve item-supported equipment before enabling it.
3. Route the actual attacking weapon through hit/damage/display, then add
   second-pair scheduling, chance and count behavior. Prove first-pair
   regressions and real phased execution.
4. Complete help/database and save-format documentation, constants sync and
   full relevant checks. Register/release the race only after the provisional
   conversion and balance decisions above are resolved.

### Acceptance tests

Use root, production-linked CuTest tests. Prefer extending existing equipment,
racial, combat and save fixtures; use a new file only if it keeps the tests
coherent. Verify behavior through real consumers, not copied implementations.

- Equip four one-handers, two two-handers, 2H plus two one-handers, weapons
  plus held items/shield, and Vestigial Arm. Try both equip orders, removal
  holes and direct saved/zone positions. Reject overlap, a fifth hand use,
  forbidden ranged combinations and every new slot without the capability.
- Fill both glove/sleeve sets and all four wrists; reject the next item.
  Verify equipment/inspect/OLC displays, every ordering entry exactly once,
  sleeve AC/proficiency/penalties, typed bonus stacking, and whole-body armor
  conflicts in both directions. Verify base anatomy restrictions still apply.
- Grant intrinsically, through one or multiple ordinary-slot items, and to
  an NPC. Test the extra-slot provider rule and form entry/exit. Remove or
  extract the last provider with cursed gear, full inventory and vetoing or
  mutating triggers. Verify capacity also shrinks when all gear is in old
  positions, and repeated checks are stable with no recursion or item loss.
- Save a fully equipped item-supported PC repeatedly through `save_char()`;
  include a failed-save path. All equipment and object ownership remain as
  before. Round-trip player flat-file, applicable DB, pet and copyover paths,
  with provider records before and after dependents, missing providers and
  nested containers. Preserve `Loc` 45..51 exactly for valid equipment and
  retain old tail `Loc` 44 for an eligible character. Check pet fingerprints
  stabilize within the new binary.
- Test actual THIRD/FOURTH hits with distinguishable weapons and effects,
  both pairs' double weapons, negative/positive Strength, finesse/Agile,
  Power Attack, ranger equivalents and mixed 2H/one-hand gear. Verify correct
  weapon dice, penalties, damage, sneak and procs; no empty extra-hand attacks
  and no primary 2H overwrite of another attack's identity.
- Test 50/75/100 percent boundaries with controlled RNG and exact candidate
  counts/penalties. Test PHASE_0 and phases 1..3 with successes and failures:
  each candidate is eligible once, and failed rolls never shift later phases.
  Include high BAB, haste, flurry, ranger procs, second-pair-only dual wielding,
  Vital Strike, unavailable actions and a target dying during the routine.
- Verify display/count queries introduce no rolls or side effects from the
  new mechanic, show the correct extra weapons and chance, and use the
  documented integer rounding for Total Defense. Bows, thrown attack counts,
  eldritch blast, ordinary two-arm and existing Extra Arms behavior remain
  covered by regressions.
- Count objects and contents through corpse transfer, extraction, rejected
  loads and rollback fixtures. Test the baseline player fallback separately
  from strict pet rejection; a single serializer round trip proves neither.

During implementation run focused cases with `CUTEST_FILTER`, then
`make -j$(nproc) test` followed by `make install`. If adding a test file,
update `cutest_SOURCES` and `cutest_test_files` in `Makefile.am` and
`CUTEST_TEST_SOURCES` in `CMakeLists.txt`, then run
`python3 scripts/ci/check_build_parity.py`. A documentation-only review does
not require building or starting the MUD.

### Deviations from Duris, recorded

- Proposed mirrors use effective feat training (50/75/100 percent), not the
  various skill-scaled and strict-comparison rolls in Duris.
- The second pair uses Luminari pair penalties and offhand Strength rules.
  Duris's slower racial combat pulse is not implemented by these changes.
- No extra-hand launchers, third held item, or held implements in weapon slots.
  Throwable melee weapons retain their melee use; thrown counts do not grow.
- Valid slots restore by number, rather than re-wielding by keyword.
- Items use `APPLY_FEAT`, with an explicit independent-provider rule.
- The full race's psionic defense, venom/paralysis, cold slow rider, mount
  restriction and final RP/tier are separate unresolved conversion decisions.
