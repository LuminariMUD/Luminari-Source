# Extra Limb Mechanics

Status: record of the code, verified 2026-09-25. The arm count replaced the three separate
extra-limb mechanics on 2026-09-25; the race point price of Extra Arms is still open (see
[Open balance work](#open-balance-work)).

Arms are a number. `arm_count()` in `src/core/utils.c` is the only source for arm-dependent
equipment and combat: consumers never test Four Arms, Extra Arms, or a race. Existing race and
form anatomy restrictions still apply on top of it. How the feats work in full is in
[GAME_MECHANICS_SYSTEMS.md](../systems/GAME_MECHANICS_SYSTEMS.md#racial-innate-feats-and-spell-like-abilities);
their race point prices are in
[PLAYER_RACES_REFERENCE.md](../guides/PLAYER_RACES_REFERENCE.md#race-point-rp-pricing-table).

| Source | Arms | Hands | Positions | Attacks |
| -- | -- | -- | -- | -- |
| Race (`race_data.arm_adjust`, 0 for every race today) | signed adjustment to the base of 2 | one per arm | by the count | by the count |
| Extra Arms (`FEAT_EXTRA_ARMS`, stackable) | +1 per rank | one per arm | by the count | by the count; no swing of its own |
| Four Arms (`FEAT_FOUR_ARMS`) | +2, once across all sources | one per arm | by the count | by the count |
| Vestigial arm (`ALC_DISC_VESTIGIAL_ARM`) | none | +1 | none | none |

## The count

`arm_count()` returns 0 for NULL and never less than 0:

1. Start at 2 plus the race's `arm_adjust`. A PC uses `GET_REAL_RACE()`, or its disguise race
   while `AFF_WILD_SHAPE` is set (the same switch that makes it read mob feats); an ordinary
   disguise changes nothing, and an invalid race index adds nothing. An NPC's `GET_RACE()` is a
   `RACE_TYPE_*` family, not a `race_list[]` index, so NPCs always start at 2.
2. Add 2 for the character's own Four Arms and 1 per nonnegative rank of its own Extra Arms:
   mob feats for NPCs and wild-shaped PCs, real feats otherwise. Clamp this intrinsic total at 0.
3. An ordinary PC also counts `APPLY_FEAT` gear, but only gear worn in positions whose
   requirement is at most the intrinsic total. Each item adds at most one Extra Arms rank however
   often the modifier repeats, and Four Arms adds its two arms once across the feat and every
   item. NPCs and wild shapes ignore item grants.

Eligibility uses the intrinsic total, never the item-enhanced count, so an item can never
sustain its own position or unlock a position for another provider. On a two-armed body, two
Extra Arms ranks and Four Arms give the same four arms; both together give six, and must not be
used to describe the same two extra limbs twice. No race is assigned fewer or more arms yet, and
there is no injury or arm-loss command.

`hands_have()` (exported from `src/act/act.h`) is the count plus one for the vestigial arm. Both
numbers are derived at use and never saved.

## Positions

`wear_slot_arms_needed()` gives each position's requirement. All other positions need no arm.

| Needs | Positions |
| -- | -- |
| 1 arm | `WIELD_1`, `HOLD_1`, `HOLD_2`, `SHIELD`, `HANDS`, `ARMS`, `WRIST_R`, `FINGER_R`, `FINGER_L` |
| 2 arms | `WIELD_OFFHAND`, `WIELD_2H`, `HOLD_2H`, `WRIST_L` |
| 3 arms | `WIELD_3`, `HANDS_2`, `ARMS_2`, `WRIST_R2` |
| 4 arms | `WIELD_4`, `WIELD_2H_2`, `WRIST_L2` |

`character_wear_slot_restriction()` checks the count for NPCs too and reports "You do not have
enough arms to use that equipment slot."; a lower position then follows its base position's race
anatomy (`four_arm_slot_base()`), so Trelux keeps its closed hand positions. `HOLD_2` needs only
one arm: the hand budget keeps a one-armed body to one item, and the vestigial arm can supply
the second hand. Rings are two positions on any body with an arm.

| Arms | Weapons | Other positions | Attacks |
| -- | -- | -- | -- |
| 0 | none | no shield, held item, gloves, sleeves, wrist or ring | the unarmed or natural routine |
| 1 | one one-hander | one hand, so a shield or held item replaces the weapon; gloves, sleeves, one wrist, two rings | the first-pair routine without an offhand weapon or two-hander |
| 2 | the first pair | as before the count existed | as before, without the old Extra Arms swing |
| 3 | adds the third hand; each weapon pair is exclusive | lower gloves and sleeves, a third wrist | third-hand mirror, no fourth hand |
| 4 | both weapon pairs | a fourth wrist | both second-pair hands |
| 5+ | as 4 | as 4, with more hand capacity | as 4 for the same equipment |

Arms past four open nothing; they add hands for the held and shield positions and the spare-hand
bonuses. The existing positions use at most nine hands: four weapons, four across `HOLD_1`,
`HOLD_2` and `HOLD_2H`, and a shield.

- Hand budget. `hands_used()` charges every hand position, a bow or crossbow two hands even in a
  one-hand position. The wear command checks the exact cost of the resolved position, for direct
  lower-position requests too.
- Weapon pairs. From three arms up a pair holds its one-handers or its two-hander, never both
  (`wield_pair_conflicts()`), in `perform_wear_impl()`, `equip_char()` and reconciliation alike.
  With two arms or fewer the first pair keeps its lenient placement, vestigial arm included.
  The second pair takes melee weapons only (`second_pair_rejects_object()`). A size-required
  two-hand item goes to its pair's two-hand position or is refused.
- Selection. The wield pickers and the sleeve, glove and wrist overflow consider only open
  positions, and a full set reports itself at every count ("around your wrist", "all three of
  your wrists", "Your hands are full.").
- Spare hands. A first-pair one-hander doubles its item bonuses only with an actual spare hand
  (`is_weapon_wielded_two_handed()`); a two-hand position or a launcher already holds its hands.
- Unsheathing. `do_unsheath()` keeps its first-pair targets and draws each item only into an
  open, empty position with no pair conflict and the hands to spare; the rest stays sheathed and
  only drawn items are reported.

## Losing arms

`limb_reconcile()` in `src/obj/act.item.c` runs from `affect_total()` once a change is complete.
Positions the count closes empty into inventory in this order, skipping open ones:
`WIELD_2H_2`, `WIELD_4`, `WIELD_3`, `WRIST_L2`, `WRIST_R2`, `HANDS_2`, `ARMS_2`; then
`WIELD_2H`, `HOLD_2H`, `WIELD_OFFHAND`, `WRIST_L`; then the one-arm positions with `WIELD_1`
last. The count is read again after every displacement, because a displaced provider can close a
position already passed. From three arms a two-hander sharing its pair with one-handers is
removed, so the primary one-hander survives. When the hands fell since the last completed check
(`limb_last_hands`) or a position closed, `HOLD_2H`, `HOLD_2`, `HOLD_1`, `WIELD_OFFHAND`,
`SHIELD`, `WIELD_2H` and last `WIELD_1` go until the hands fit. An unchanged body is never
audited. Displacement runs the remove trigger for its side effects but ignores a veto, keeps
ownership through the object-transfer machinery, and never drops gear in the room.
`limb_defer_begin()`/`limb_defer_end()` and affect batches hold the check until they end.

## Saving and restoring

Saved `Loc` values, zone `E` positions and pet record formats are unchanged; `Loc` 45 to 51 are
the seven lower positions.

- `save_char()` defers reconciliation over its unequip/re-equip cycle, re-equips every position
  the count currently opens, then retries the postponed positions once, so repeated saves leave
  valid gear in place without messages.
- `auto_equip()` holds any otherwise valid saved position the count closes; the item waits in
  inventory, out of its bag, until `crash_restore_records()` retries it once the whole record set
  is loaded. On failure the marker is cleared, contents stay with the item, and its bag sort
  applies. This path serves player flat files, database records and pet records.
- After the retry `limb_restore_validate()` trims an over-budget loadout even on a fresh
  character, because missing providers can leave one entirely in ordinary positions.

Rolling back is not behavior-neutral: an older binary reads Extra Arms as extra swings and may
reject count-enabled lower gear. Inspect affected grants and loadouts and keep recoverable saves
before deploying or rolling back; the procedure for rolling back past the lower positions is in
[SAVE_SYSTEMS_BREAKDOWN.md](../systems/SAVE_SYSTEMS_BREAKDOWN.md).

## Attacks

`perform_second_pair_attacks()` in `src/combat/fight.c` runs from three arms up after the first
pair. It mirrors the round's planned base, haste, BAB and flurry opportunities as
`ATTACK_TYPE_THIRD` and `ATTACK_TYPE_FOURTH`, each at 50 percent, 75 with two-weapon training
and 100 with improved training, with stable ordinals and one roll in the candidate's own phase.
The ordinals hold for the whole round: `draw_attack_round_plan()` draws the extra-attack procs
(the extra flurry, Air Embodiment and the Wilderness Warrior offhand attacks) once, when the
round's first attack routine runs, into `char_specials.attack_round`, which `perform_violence()`
clears when phase 1 begins. Phases 2 and 3 replay that draw and phase 1's second-pair numbering,
so no candidate changes phase within a round.
Count mode adds the floor of the summed chances, so a single 50 percent candidate adds nothing
to the displayed number while still attacking. The third hand needs a weapon or, for a monk, an
empty third position; the fourth hand needs four arms and a weapon, including the other end of a
lower double weapon. Other characters' empty lower hands make no swing, and Vital Strike, wild
shapes, polymorphs and the ranged routines still skip the second pair.

## Monks

LuminariMUD has no race class limits (#166), so a many-armed monk or sacred fist (`MONK_TYPE()`)
is possible. The two-armed unarmed routine is the baseline.

| Arms | Monk |
| -- | -- |
| 0 | the same unarmed opportunities and flurry (feet, knees, head); no glove position, so no glove bonus |
| 1 | the same unarmed opportunities; at most one monk weapon and no two-hand position |
| 2 | unchanged |
| 3 or 4 | an empty third position gives unarmed third-hand candidates with monk dice while `monk_gear_ok()` passes, mirroring base, haste, BAB and flurry |
| 5+ | as 4 |

- Bare hands. `is_bare_handed()` still requires every hand empty, so a monk with an upper monk
  weapon gets the unarmed third hand without the whole-character bare-hand bonuses (the glove
  bonus, the unarmed critical range).
- Monk weapons. `monk_gear_ok()` accepts monk-family weapons in either pair; a monk weapon in the
  fourth hand attacks beside an unarmed third.
- Support hand. An unarmed strike fills a hand that no position counts, so the unarmed third hand
  adds the free-hand strength bonus only when a hand remains after the equipped positions, the
  primary's strike and support hand, and its own strike: never at three arms, and at four arms
  only with empty hands or a lone primary weapon.
- First pair only. The monk glove bonus reads only `WEAR_HANDS`, and the One With Wood and Stone
  armor bonus reads only a first-pair weapon.

## Vestigial arm

`ALC_DISC_VESTIGIAL_ARM` is an alchemist discovery, available at alchemist level 9. It adds one
hand in `hands_have()`, so the character can hold one more item in the existing positions, and
looking at the character shows the extra arm. It adds no arm, no position and no attack: at zero
arms it holds nothing, and at one arm it allows two held one-hand items, or a weapon and a shield,
but no offhand weapon or two-hand position.

## Open balance work

- Extra Arms keeps its published price of 3 race points per rank, set when it was a swing; no
  race grants it. A suggestion is 7.5 RP for each rank that adds the third or fourth arm and 1 RP
  per rank past four, with a cap exception like Four Arms, but equal pricing is not established:
  the third arm alone opens lower gloves and sleeves. How mixed Four Arms, Extra Arms and race
  adjustments are priced, and whether the Thri-Kreen exception applies, is undecided.
- Four Arms stays at 15 race points and is exempt from the single-trait cap as a Thri-Kreen
  trait. The unarmed monk third hand raises its value for monks; that is not priced.
- Existing Extra Arms grants on saved characters or staff-built items change meaning; check them
  before deploying.

A third weapon pair or anything past four arms beyond extra hands would need new positions and
attack types; this system caps equipment at four arms. The [Duris race
specifications](DURIS_RACE_SPECIFICATIONS.md#thri-kreen) place Four Arms on Thri-Kreen.
